
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
/*  FILE:         khe_sr_single_resource.c                                   */
/*  DESCRIPTION:  Single resource solving                                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_p.h"
#include <limits.h>
#include <float.h>
#include <math.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define UNDEF_INT -1
#define UNDEF_FLOAT -1.0

#define DEBUG1 1		/* main functions		*/
#define DEBUG2 0		/* final solutions		*/
#define DEBUG3 0		/* checking adopted solutions	*/
#define DEBUG4 0		/* KheSrsPartialSolnMake	*/
#define DEBUG5 0		/* KheSingleResourceSolverDebug	*/
#define DEBUG6 0		/* KheSrsFullSolnAdopt		*/
#define DEBUG7 0		/* running time (testing only)	*/
#define DEBUG8 0		/* manual tests               	*/
#define DEBUG9 0		/* hash table efficiency      	*/
#define DEBUG10 0		/* day dom tests	      	*/
#define DEBUG11 1		/* brief print of costs		*/
#define DEBUG12 0		/* trie dominance tests 	*/
#define DEBUG13 0		/* monitor sorting              */
#define DEBUG14 1		/* cluster minimum solving      */

#define DEBUG_PS(ps) (HaArrayFirst((ps)->signature) == 1 &&		\
  (ps)->day->day_index == 27)

#define TESTING 1

/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations" (private)                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_KIND - the kind of dominance testing being used              */
/*                                                                           */
/*****************************************************************************/

/* *** defined in khe_solvers.h
typedef enum {
  KHE_SRS_DOM_WEAK,
  KHE_SRS_DOM_MEDIUM,
  KHE_SRS_DOM_STRONG,
  KHE_SRS_DOM_TRIE,
} KHE_SRS_DOM_KIND;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TYPE - the type of a dominance test                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SRS_DOM_UNUSED,		/* "-"      should never be used      */
  KHE_SRS_DOM_GE,		/* ">="     min limit only            */
  KHE_SRS_DOM_LE,		/* "<="     max limit only            */
  KHE_SRS_DOM_EQ,		/* "==",    for uncertain cases       */
  KHE_SRS_DOM_GE_LOOSE,		/* ">=*"    min limit with loosening  */
  KHE_SRS_DOM_EQ_LOOSE		/* "==*"    max + min with loosening  */
} KHE_SRS_DOM_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST - a dominance test                                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_dom_test_rec {
  KHE_SRS_DOM_TYPE	type;			/* type of test              */
  int			min_limit;		/* GE_LOOSE and EQ_LOOSE     */
} KHE_SRS_DOM_TEST;

typedef HA_ARRAY(KHE_SRS_DOM_TEST) ARRAY_KHE_SRS_DOM_TEST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_TAG                                                         */
/*                                                                           */
/*  Tags for expression types.                                               */
/*                                                                           */
/*  Subtypes of KHE_SRS_EXPR_INT have a stored value of type int on their    */
/*  active days; subtypes of KHE_SRS_EXPR_FLOAT have a stored value of type  */
/*  float.  These are not always the same as the result type of the node.    */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SRS_EXPR_BUSY_TIME_TAG,		/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_FREE_TIME_TAG,		/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_WORK_TIME_TAG,		/* subtype of KHE_SRS_EXPR_FLOAT     */
  KHE_SRS_EXPR_BUSY_DAY_TAG,		/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_FREE_DAY_TAG,		/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_WORK_DAY_TAG,		/* subtype of KHE_SRS_EXPR_FLOAT     */
  KHE_SRS_EXPR_OR_TAG,			/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_AND_TAG,			/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_INT_SUM_TAG,		/* subtype of KHE_SRS_EXPR_INT	     */
  /* KHE_SRS_EXPR_INT_SEQ_TAG, */
  KHE_SRS_EXPR_FLOAT_SUM_TAG,		/* subtype of KHE_SRS_EXPR_FLOAT     */
  KHE_SRS_EXPR_INT_DEV_TAG,
  KHE_SRS_EXPR_FLOAT_DEV_TAG,
  KHE_SRS_EXPR_COST_TAG,
  /* KHE_SRS_EXPR_COST_SUM_TAG, */
  KHE_SRS_EXPR_INT_SUM_COMB_TAG,	/* subtype of KHE_SRS_EXPR_INT	     */
  KHE_SRS_EXPR_INT_SEQ_COMB_TAG		/* subtype of KHE_SRS_EXPR_INT	     */
} KHE_SRS_EXPR_TAG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_COMB_TYPE - types of comb nodes                                  */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SRS_COMB_ORDINARY,
  KHE_SRS_COMB_NO_MAX,
  KHE_SRS_COMB_LINEAR,
  KHE_SRS_COMB_STEP
} KHE_SRS_COMB_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR - abstract supertype of all expression types                */
/*                                                                           */
/*  For each active day di of e, except its very last active day,            */
/*  sig_index[di - e->first_day_index] is the position of e's value in       */
/*  the signatures of partial solutions ending on day di.                    */
/*                                                                           */
/*  For each active day di of e, child_index[di - e->first_day_index] is     */
/*  the index of the first child whose last_day_index is di or more.  There  */
/*  is also a sentinel value, so for each active day di, the iterator        */
/*                                                                           */
/*    start_child_index = child_index[di - e->first_day_index];              */
/*    stop_child_index  = child_index[di + 1 - e->first_day_index];          */
/*    for( i = start_child_index;  i < stop_child_index;  i++ )              */
/*      child_e = HaArray(e->children, i);                                   */
/*                                                                           */
/*  visits the children of e whose last_day_index is di.                     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_rec *KHE_SRS_EXPR;
typedef HA_ARRAY(KHE_SRS_EXPR) ARRAY_KHE_SRS_EXPR;

#define INHERIT_KHE_SRS_EXPR						\
  KHE_SRS_EXPR_TAG	tag;			/* tag field      */	\
  bool			deleted;		/* deleted        */	\
  /* KHE_SRS_EXPR	parent; */		/* parent expr    */	\
  ARRAY_KHE_SRS_EXPR	children;		/* child exprs    */	\
  int			first_day_index;	/* first day      */	\
  int			last_day_index;		/* last day       */	\
  HA_ARRAY_INT		sig_indexes;		/* for each day   */	\
  HA_ARRAY_INT		child_indexes;		/* for each day   */	\
  KHE_SRS_DOM_TEST	dom_test;		/* dom test       */

struct khe_srs_expr_rec {
  INHERIT_KHE_SRS_EXPR
};


/*****************************************************************************/
/*                                                                           */
/*  INHERIT_KHE_SRS_EXPR_INT - abstract supertype of integer-valued exprs    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_int_rec *KHE_SRS_EXPR_INT;
typedef HA_ARRAY(KHE_SRS_EXPR_INT) ARRAY_KHE_SRS_EXPR_INT;

#define INHERIT_KHE_SRS_EXPR_INT					\
  INHERIT_KHE_SRS_EXPR							\
  int				value;		/* stored value */

struct khe_srs_expr_int_rec {
  INHERIT_KHE_SRS_EXPR_INT
};


/*****************************************************************************/
/*                                                                           */
/*  INHERIT_KHE_SRS_EXPR_FLOAT - abstract supertype of float-valued exprs    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_float_rec *KHE_SRS_EXPR_FLOAT;
typedef HA_ARRAY(KHE_SRS_EXPR_FLOAT) ARRAY_KHE_SRS_EXPR_FLOAT;

#define INHERIT_KHE_SRS_EXPR_FLOAT					\
  INHERIT_KHE_SRS_EXPR							\
  float				value;		/* stored value */

struct khe_srs_expr_float_rec {
  INHERIT_KHE_SRS_EXPR_FLOAT
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_TIME                                                   */
/*                                                                           */
/*  An expression whose value is 1 when the resource is busy at a given      */
/*  time, and 0 when the resource is free then.  The expression stores the   */
/*  time, but only for debugging.                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_busy_time_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_TIME			time;		/* the time we're busy at    */
} *KHE_SRS_EXPR_BUSY_TIME;

typedef HA_ARRAY(KHE_SRS_EXPR_BUSY_TIME) ARRAY_KHE_SRS_EXPR_BUSY_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_TIME                                                   */
/*                                                                           */
/*  An expression whose value is 1 when the resource is free at a given      */
/*  time, and 0 when the resource is busy then.  The expression stores the   */
/*  time, but only for debugging.                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_free_time_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_TIME			time;		/* the time we're free at    */
} *KHE_SRS_EXPR_FREE_TIME;

typedef HA_ARRAY(KHE_SRS_EXPR_FREE_TIME) ARRAY_KHE_SRS_EXPR_FREE_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_TIME                                                   */
/*                                                                           */
/*  An expression whose value is the workload incurred by the resource at    */
/*  a given time.  This will be 0.0 if the resource is free then.  The       */
/*  expression stores the time, but only for debugging.                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_work_time_rec {
  INHERIT_KHE_SRS_EXPR_FLOAT
  KHE_TIME			time;		/* the time of this workload */
} *KHE_SRS_EXPR_WORK_TIME;

typedef HA_ARRAY(KHE_SRS_EXPR_WORK_TIME) ARRAY_KHE_SRS_EXPR_WORK_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_DAY                                                    */
/*                                                                           */
/*  An expression whose value is 1 when the resource is busy on a given      */
/*  day, and 0 when the resource is free then.  The expression stores the    */
/*  day (as a time group), but only for debugging.                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_busy_day_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_TIME_GROUP		time_group;	/* the day we're busy at     */
} *KHE_SRS_EXPR_BUSY_DAY;

typedef HA_ARRAY(KHE_SRS_EXPR_BUSY_DAY) ARRAY_KHE_SRS_EXPR_BUSY_DAY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_DAY                                                    */
/*                                                                           */
/*  An expression whose value is 1 when the resource is free on a given      */
/*  day, and 0 when the resource is busy then.  The expression stores the    */
/*  day (as a time group), but only for debugging.                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_free_day_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_TIME_GROUP		time_group;	/* the day we're free at     */
} *KHE_SRS_EXPR_FREE_DAY;

typedef HA_ARRAY(KHE_SRS_EXPR_FREE_DAY) ARRAY_KHE_SRS_EXPR_FREE_DAY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_DAY                                                    */
/*                                                                           */
/*  An expression whose value is the workload incurred by the resource on    */
/*  a given day.  This will be 0.0 if the resource is free then.  The        */
/*  expression stores the day (as a time group), but only for debugging.     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_work_day_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_TIME_GROUP		time_group;	/* the day we're free at     */
} *KHE_SRS_EXPR_WORK_DAY;

typedef HA_ARRAY(KHE_SRS_EXPR_WORK_DAY) ARRAY_KHE_SRS_EXPR_WORK_DAY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_OR                                                          */
/*                                                                           */
/*  An expression whose value is the logical `or' of its children's values.  */
/*  Here Booleans are represented by integers (0 for false, 1 for true).     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_or_rec {
  INHERIT_KHE_SRS_EXPR_INT
} *KHE_SRS_EXPR_OR;

typedef HA_ARRAY(KHE_SRS_EXPR_OR) ARRAY_KHE_SRS_EXPR_OR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_AND                                                         */
/*                                                                           */
/*  An expression whose value is the logical `and' of its children's values. */
/*  Here Booleans are represented by integers (0 for false, 1 for true).     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_and_rec {
  INHERIT_KHE_SRS_EXPR_INT
} *KHE_SRS_EXPR_AND;

typedef HA_ARRAY(KHE_SRS_EXPR_AND) ARRAY_KHE_SRS_EXPR_AND;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SUM                                                     */
/*                                                                           */
/*  An expression whose value is the sum of its children's values.           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_int_sum_rec {
  INHERIT_KHE_SRS_EXPR_INT
} *KHE_SRS_EXPR_INT_SUM;

typedef HA_ARRAY(KHE_SRS_EXPR_INT_SUM) ARRAY_KHE_SRS_EXPR_INT_SUM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SEQ                                                     */
/*                                                                           */
/*  An expression whose value is the sequence of its children's values,      */
/*  although it does not remember all that; it remembers the length of       */
/*  the current sequence, and passes each length up as its sequence ends.    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB is used
typedef struct khe_srs_expr_int_seq_rec {
  INHERIT_KHE_SRS_EXPR
  int				history_before;	** history value             **
  int				history_after;	** history value             **
  int				tmp_value;	** defined only within day   **
} *KHE_SRS_EXPR_INT_SEQ;

typedef HA_ARRAY(KHE_SRS_EXPR_INT_SEQ) ARRAY_KHE_SRS_EXPR_INT_SEQ;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FLOAT_SUM                                                   */
/*                                                                           */
/*  An expression whose value is the sum of its children's values.  The      */
/*  expression's value, and its children's values, have type float.          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_float_sum_rec {
  INHERIT_KHE_SRS_EXPR_FLOAT
} *KHE_SRS_EXPR_FLOAT_SUM;

typedef HA_ARRAY(KHE_SRS_EXPR_FLOAT_SUM) ARRAY_KHE_SRS_EXPR_FLOAT_SUM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_DEV                                                     */
/*                                                                           */
/*  An expression whose value is the deviation of its integer-valued child.  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_int_dev_rec {
  INHERIT_KHE_SRS_EXPR_INT
  int				min_limit;	/* minimum limit             */
  int				max_limit;	/* maximum limit             */
  bool				allow_zero;	/* true if 0 has deviation 0 */
} *KHE_SRS_EXPR_INT_DEV;

typedef HA_ARRAY(KHE_SRS_EXPR_INT_DEV) ARRAY_KHE_SRS_EXPR_INT_DEV;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FLOAT_DEV                                                   */
/*                                                                           */
/*  An expression whose value is the integer-valued deviation of its         */
/*  float-valued child.                                                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_float_dev_rec {
  INHERIT_KHE_SRS_EXPR_INT
  int				min_limit;	/* minimum limit             */
  int				max_limit;	/* maximum limit             */
  bool				allow_zero;	/* true if 0 has deviation 0 */
} *KHE_SRS_EXPR_FLOAT_DEV;

typedef HA_ARRAY(KHE_SRS_EXPR_FLOAT_DEV) ARRAY_KHE_SRS_EXPR_FLOAT_DEV;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_COST                                                        */
/*                                                                           */
/*  An expression whose value is a cost based on its sole child's deviation. */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_cost_rec {
  INHERIT_KHE_SRS_EXPR
  KHE_MONITOR			monitor;	/* the monitor (debug only)  */
  KHE_COST_FUNCTION		cost_fn;	/* the cost function         */
  KHE_COST			combined_weight; /* the combined weight      */
} *KHE_SRS_EXPR_COST;

typedef HA_ARRAY(KHE_SRS_EXPR_COST) ARRAY_KHE_SRS_EXPR_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_COST_SUM                                                    */
/*                                                                           */
/*  An expression whose value is the sum of the values of its children.      */
/*  All these values are costs.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_srs_expr_cost_sum_rec {
  INHERIT_KHE_SRS_EXPR
} *KHE_SRS_EXPR_COST_SUM;

typedef HA_ARRAY(KHE_SRS_EXPR_COST_SUM) ARRAY_KHE_SRS_EXPR_COST_SUM;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SUM_COMB                                                */
/*                                                                           */
/*  Combines INT_SUM, INT_DEV, and COST into a single node.  This allows     */
/*  for important optimizations in some cases, depending on comb_type.       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_int_sum_comb_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_MONITOR			monitor;	/* the monitor (debug only)  */
  KHE_COST_FUNCTION		cost_fn;	/* the cost function         */
  KHE_COST			combined_weight; /* the combined weight      */
  int				min_limit;	/* minimum limit             */
  int				max_limit;	/* maximum limit             */
  bool				allow_zero;	/* true if 0 has deviation 0 */
  int				history_before;	/* history value before      */
  int				history_after;	/* history value after       */
  KHE_SRS_COMB_TYPE		comb_type;	/* type of combined value    */
} *KHE_SRS_EXPR_INT_SUM_COMB;

typedef HA_ARRAY(KHE_SRS_EXPR_INT_SUM_COMB) ARRAY_KHE_SRS_EXPR_INT_SUM_COMB;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SEQ_COMB                                                */
/*                                                                           */
/*  Combines INT_SEQ, INT_DEV (without allow_zero), and COST into a single   */
/*  node.  This allows the same optimizations as KHE_SRS_EXPR_INT_SUM_COMB.  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_expr_int_seq_comb_rec {
  INHERIT_KHE_SRS_EXPR_INT
  KHE_MONITOR			monitor;	/* the monitor (debug only)  */
  KHE_COST_FUNCTION		cost_fn;	/* the cost function         */
  KHE_COST			combined_weight; /* the combined weight      */
  int				min_limit;	/* minimum limit             */
  int				max_limit;	/* maximum limit             */
  int				history_before;	/* history value before      */
  int				history_after;	/* history value after       */
  KHE_SRS_COMB_TYPE		comb_type;	/* type of combined value    */
} *KHE_SRS_EXPR_INT_SEQ_COMB;

typedef HA_ARRAY(KHE_SRS_EXPR_INT_SEQ_COMB) ARRAY_KHE_SRS_EXPR_INT_SEQ_COMB;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_STEP - one step, either a task or some free time                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_step_rec {
  KHE_TASK			task;			/* the task, or NULL */
  KHE_DAILY_SCHEDULE		daily_schedule;		/* its daily schedule*/
  KHE_COST			cost_reduction;		/* its cost reduction*/
  int				domain_count;		/* its domain size   */
  bool				already_assigned;	/* already assigned r*/
} *KHE_SRS_STEP;

typedef HA_ARRAY(KHE_SRS_STEP) ARRAY_KHE_SRS_STEP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_TIME - one time                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_time_rec {
  KHE_TIME			time;		/* the time represented      */
  ARRAY_KHE_SRS_STEP		best_steps;	/* best tasks starting here  */
} *KHE_SRS_TIME;

typedef HA_ARRAY(KHE_SRS_TIME) ARRAY_KHE_SRS_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN - a partial solution                                */
/*                                                                           */
/*  The step field is a bit tricky.  It is NULL only in the special          */
/*  initial partial solution, the one representing an empty solution.        */
/*  In all other partial solutions it is non-NULL, but it may have a         */
/*  NULL task field, in which case it represents a free day.                 */
/*                                                                           */
/*****************************************************************************/
typedef struct khe_srs_day_rec *KHE_SRS_DAY;

typedef struct khe_srs_partial_soln_rec *KHE_SRS_PARTIAL_SOLN;

struct khe_srs_partial_soln_rec {
  KHE_SRS_PARTIAL_SOLN	prev_ps;		/* previous partial soln     */
  KHE_SRS_STEP		step;			/* the step leading to here  */
  KHE_SRS_DAY		day;			/* day containing this ps    */
  int			best_of;		/* how many this is best of  */
  KHE_COST		cost;			/* resource cost so far      */
  HA_ARRAY_INT		signature;		/* signature of this node    */
};

typedef HA_ARRAY(KHE_SRS_PARTIAL_SOLN) ARRAY_KHE_SRS_PARTIAL_SOLN;
typedef HP_TABLE(KHE_SRS_PARTIAL_SOLN) TABLE_KHE_SRS_PARTIAL_SOLN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PS_SET - a set of partial solutions                              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_ps_set_rec {
  /* int			busy_days; */		/* all have this     */
  ARRAY_KHE_SRS_PARTIAL_SOLN	partial_solns;		/* the solns         */
} *KHE_SRS_PS_SET;

typedef HA_ARRAY(KHE_SRS_PS_SET) ARRAY_KHE_SRS_PS_SET;
typedef HP_TABLE(KHE_SRS_PS_SET) TABLE_KHE_SRS_PS_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PS_TRIE - a trie of partial solutions                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_ps_trie_rec *KHE_SRS_PS_TRIE;

typedef HA_ARRAY(KHE_SRS_PS_TRIE) ARRAY_KHE_SRS_PS_TRIE;

struct khe_srs_ps_trie_rec {
  KHE_SRS_PARTIAL_SOLN		partial_soln;
  ARRAY_KHE_SRS_PS_TRIE		children;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN_SET - a set of partial solutions on a given day     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_srs_partial_soln_set_rec {
  int				count;			/* number of solns   */
  KHE_TIME_GROUP		time_group;		/* for debugging only*/
  TABLE_KHE_SRS_PARTIAL_SOLN	weak_dom_set;		/* weak dominance    */
  TABLE_KHE_SRS_PS_SET		medium_dom_set;		/* medium dominance  */
  ARRAY_KHE_SRS_PS_SET		strong_dom_set;		/* strong dominance  */
  KHE_SRS_PS_TRIE		trie_dom_set;		/* trie dominance    */
} *KHE_SRS_PARTIAL_SOLN_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DAY - one day and everything about it                            */
/*                                                                           */
/*****************************************************************************/

struct khe_srs_day_rec {
  int				day_index;		/* index of this day */
  KHE_TIME_GROUP		time_group;		/* day's time group  */
  ARRAY_KHE_SRS_TIME		times;			/* times of this day */
  /* int			partial_solns_count; */	/* table count       */
  int				sig_len;		/* length of sig     */
  ARRAY_KHE_SRS_DOM_TEST	dom_tests;		/* for each sig pos  */
  HA_ARRAY_INT			eq_dom_test_indexes;	/* where = tests are */
  /* ARRAY_KHE_SRS_EXPR		active_exprs; */	/* active on this day*/
  /* ARRAY_KHE_SRS_EXPR		leaf_exprs; */		/* leaf exprs for day*/
  ARRAY_KHE_SRS_EXPR		active_today;		/* active on this day*/
  ARRAY_KHE_SRS_EXPR		leaf_today;		/* leaf expressions  */
  KHE_SRS_STEP			free_step;		/* free on this day  */
  KHE_SRS_PARTIAL_SOLN_SET	partial_soln_set;	/* partial solns     */
  /* TABLE_KHE_SRS_PARTIAL_SOLN	partial_solns_table; */	/* weak dominance    */
  /* ARRAY_KHE_SRS_PARTIAL_SOLN	partial_solns_array; */	/* strong dominance  */
};

typedef HA_ARRAY(KHE_SRS_DAY) ARRAY_KHE_SRS_DAY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_FULL_SOLN - used when adopting and unadopting complete solutions */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_srs_full_item_rec {
  KHE_SRS_STEP			step;
  bool				previously_assigned;
} KHE_SRS_FULL_ITEM;

typedef HA_ARRAY(KHE_SRS_FULL_ITEM) ARRAY_KHE_SRS_FULL_ITEM;
*** */

typedef struct khe_srs_full_soln_rec {
  KHE_RESOURCE			resource;
  int				asst_count;
  ARRAY_KHE_SRS_STEP		steps;
  /* ARRAY_KHE_SRS_FULL_ITEM	items; */
  bool				adopted;
} *KHE_SRS_FULL_SOLN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SINGLE_RESOURCE_SOLVER - a single resource solver                    */
/*                                                                           */
/*****************************************************************************/
/* typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK; */
/* typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST; */
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_MONITOR)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR;

struct khe_single_resource_solver_rec {

  /* boilerplate fields */
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_OPTIONS			options;
  KHE_FRAME			days_frame;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_TASK_FINDER		task_finder;

  /* fields that vary with the resource and solve */
  /* ARRAY_KHE_COST		cost_limits; */
  KHE_CLUSTER_MINIMUM_SOLVER	cluster_solver;
  int				diversifier;
  KHE_RESOURCE			resource;	/* resource (if solving)     */
  KHE_SRS_DOM_KIND		dom_kind;
  int				min_assts;
  int				max_assts;
  KHE_COST			cost_limit;
  ARRAY_KHE_SRS_EXPR		exprs;
  /* KHE_SRS_EXPR_COST_SUM	root_expr; */
  /* KHE_COST			min_cost_reduction; */
  ARRAY_KHE_SRS_DAY		days;
  ARRAY_KHE_SRS_PARTIAL_SOLN	final_solns;
  KHE_SRS_FULL_SOLN		full_soln;
  bool				rerun;
  /* ***
  int				adopted_index;
  ARRAY_KHE_TASK		adopted_tasks;
  *** */

  /* free lists */
  ARRAY_KHE_SRS_EXPR_BUSY_TIME	free_expr_busy_time;
  ARRAY_KHE_SRS_EXPR_FREE_TIME	free_expr_free_time;
  ARRAY_KHE_SRS_EXPR_WORK_TIME	free_expr_work_time;
  ARRAY_KHE_SRS_EXPR_BUSY_DAY	free_expr_busy_day;
  ARRAY_KHE_SRS_EXPR_FREE_DAY	free_expr_free_day;
  ARRAY_KHE_SRS_EXPR_WORK_DAY	free_expr_work_day;
  ARRAY_KHE_SRS_EXPR_OR		free_expr_or;
  ARRAY_KHE_SRS_EXPR_AND	free_expr_and;
  ARRAY_KHE_SRS_EXPR_INT_SUM	free_expr_int_sum;
  /* ARRAY_KHE_SRS_EXPR_INT_SEQ	free_expr_int_seq; */
  ARRAY_KHE_SRS_EXPR_FLOAT_SUM	free_expr_float_sum;
  ARRAY_KHE_SRS_EXPR_INT_DEV	free_expr_int_dev;
  ARRAY_KHE_SRS_EXPR_FLOAT_DEV	free_expr_float_dev;
  ARRAY_KHE_SRS_EXPR_COST	free_expr_cost;
  /* ARRAY_KHE_SRS_EXPR_COST_SUM free_expr_cost_sum; */
  ARRAY_KHE_SRS_EXPR_INT_SUM_COMB free_expr_int_sum_comb;
  ARRAY_KHE_SRS_EXPR_INT_SEQ_COMB free_expr_int_seq_comb;
  ARRAY_KHE_SRS_STEP		free_steps;
  ARRAY_KHE_SRS_PARTIAL_SOLN	free_partial_solns;
  ARRAY_KHE_SRS_PS_SET		free_ps_sets;
  ARRAY_KHE_SRS_PS_TRIE		free_ps_tries;

#if TESTING
  KHE_TIMER			timer;
  HA_ARRAY_FLOAT		running_time_per_day;
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations and costs"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSrsSumDev(int min_limit, int max_limit, bool allow_zero,          */
/*    int history_after, int val)                                            */
/*                                                                           */
/*  Deviation function to use with sum nodes; it has allow_zero.             */
/*                                                                           */
/*****************************************************************************/

static int KheSrsSumDev(int min_limit, int max_limit, bool allow_zero,
  int history_after, int val)
{
  if( val == 0 && allow_zero )
    return 0;
  else if( val > max_limit )
    return val - max_limit;
  else if( val + history_after < min_limit )
    return min_limit - (val + history_after);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsSeqDev(int min_limit, int max_limit,                           */
/*    int history_after, int val)                                            */
/*                                                                           */
/*  Deviation function to use with sequence nodes; it has no allow_zero.     */
/*                                                                           */
/*****************************************************************************/

static int KheSrsSeqDev(int min_limit, int max_limit,
  int history_after, int val)
{
  if( val > max_limit )
    return val - max_limit;
  else if( val + history_after < min_limit )
    return min_limit - (val + history_after);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSrsCost(KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight, */
/*    int dev)                                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheSrsCost(KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,
  int dev)
{
  switch( cost_fn )
  {
    case KHE_STEP_COST_FUNCTION:

        return dev > 0 ? combined_weight : 0;

    case KHE_LINEAR_COST_FUNCTION:

        return dev * combined_weight;

    case KHE_QUADRATIC_COST_FUNCTION:

	return dev * dev * combined_weight;

    default:

	HnAbort("KheSrsCost internal error");
	return KheCost(0, 0);  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_DOM_TEST"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsDomTestMake(KHE_SRS_DOM_TYPE type)                */
/*                                                                           */
/*  Make and return a new dominance test object with no min limit attribute. */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_DOM_TEST KheSrsDomTestMake(KHE_SRS_DOM_TYPE type)
{
  KHE_SRS_DOM_TEST res;
  res.type = type;
  res.min_limit = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsDomTestMakeLoose(KHE_SRS_DOM_TYPE type, int val)  */
/*                                                                           */
/*  Make and return a new dominance test object with a min limit attribute.  */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_DOM_TEST KheSrsDomTestMakeLoose(KHE_SRS_DOM_TYPE type, int val)
{
  KHE_SRS_DOM_TEST res;
  res.type = type;
  res.min_limit = val;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsDomTestEqual(KHE_SRS_DOM_TEST dt1, KHE_SRS_DOM_TEST dt2)      */
/*                                                                           */
/*  Return true if dt1 and dt2 are equal.  This function assumes, as it      */
/*  may, that when the min_limit field is unused its value is always -1.     */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsDomTestEqual(KHE_SRS_DOM_TEST dt1, KHE_SRS_DOM_TEST dt2)
{
  return dt1.type == dt2.type && dt1.min_limit == dt2.min_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsDomTestDominates(KHE_SRS_DOM_TEST dt, int val1, int val2)     */
/*                                                                           */
/*  Return true if val1 dominates val2, according to dt.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsDomTestDominates(KHE_SRS_DOM_TEST dt, int val1, int val2)
{
  switch( dt.type )
  {
    case KHE_SRS_DOM_UNUSED:

      /* should never be called */
      HnAbort("KheSrsDomTestDominates internal error (KHE_SRS_DOM_UNUSED)");
      return false;  /* keep compiler happy */

    case KHE_SRS_DOM_GE:

      /* tight min limit */
      return val1 >= val2;

    case KHE_SRS_DOM_LE:

      /* max limit */
      return val1 <= val2;

    case KHE_SRS_DOM_EQ:

      /* values must be equal */
      return val1 == val2;

    case KHE_SRS_DOM_GE_LOOSE:

      /* loose min limit */
      return val1 >= val2 || val1 >= dt.min_limit;

    case KHE_SRS_DOM_EQ_LOOSE:

      /* max and loose min; both conditions must hold (basically equality) */
      return (val1 <= val2) && (val1 >= val2 || val1 >= dt.min_limit);

    default:

      HnAbort("KheSrsDomTestDominates internal error (%d)", dt.type);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDomTestDominatingRange(KHE_SRS_DOM_TEST dt, int val,          */
/*    int *from_val, int *to_val)                                            */
/*                                                                           */
/*  Set [*from_val, *to_val] to the range of values that dominate val        */
/*  under dt.  That is, set it to the largest range of values x such that    */
/*                                                                           */
/*    KheSrsDomTestDominates(dt, x, val)                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDomTestDominatingRange(KHE_SRS_DOM_TEST dt, int val,
  int *from_val, int *to_val)
{
  switch( dt.type )
  {
    case KHE_SRS_DOM_UNUSED:

      /* should never be called */
      HnAbort("KheSrsDomTestDominatingRange internal error (DOM_UNUSED)");
      *from_val = *to_val = 0;  /* keep compiler happy */
      break;

    case KHE_SRS_DOM_GE:

      /* largest range of values x such that x >= val */
      *from_val = val, *to_val = INT_MAX;
      break;

    case KHE_SRS_DOM_LE:

      /* largest range of values x such that x <= val */
      *from_val = 0, *to_val = val;
      break;

    case KHE_SRS_DOM_EQ:

      /* largest range of values x such that x == val */
      *from_val = val, *to_val = val;
      break;

    case KHE_SRS_DOM_GE_LOOSE:

      /* largest range of values x such that x >= val || x >= dt.min_limit */
      /* i.e. such that x >= min(val, dt.min_limit) */
      *from_val = min(val, dt.min_limit), *to_val = INT_MAX;
      break;

    case KHE_SRS_DOM_EQ_LOOSE:

      /* largest range of values x st x <= val && x >= min(val, dt.min_limit) */
      *from_val = min(val, dt.min_limit), *to_val = val;
      break;

    default:

      HnAbort("KheSrsDomTestDominatingRange internal error (%d)", dt.type);
      *from_val = *to_val = 0;  /* keep compiler happy */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDomTestDominatedRange(KHE_SRS_DOM_TEST dt, int val,           */
/*    int *from_val, int *to_val)                                            */
/*                                                                           */
/*  Set [*from_val, *to_val] to the range of values that are dominated by    */
/*  val under dt.  That is, set it to the largest range of values x s.t.     */
/*                                                                           */
/*    KheSrsDomTestDominates(dt, val, x)                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDomTestDominatedRange(KHE_SRS_DOM_TEST dt, int val,
  int *from_val, int *to_val)
{
  switch( dt.type )
  {
    case KHE_SRS_DOM_UNUSED:

      /* should never be called */
      HnAbort("KheSrsDomTestDominatedRange internal error (DOM_UNUSED)");
      *from_val = *to_val = 0;  /* keep compiler happy */
      break;

    case KHE_SRS_DOM_GE:

      /* largest range of values x such that val >= x */
      *from_val = 0, *to_val = val;
      break;

    case KHE_SRS_DOM_LE:

      /* largest range of values x such that val <= x */
      *from_val = val, *to_val = INT_MAX;
      break;

    case KHE_SRS_DOM_EQ:

      /* largest range of values x such that val == x */
      *from_val = val, *to_val = val;
      break;

    case KHE_SRS_DOM_GE_LOOSE:

      /* largest range of values x such that val >= x || val >= dt.min_limit */
      if( val >= dt.min_limit )
        *from_val = 0, *to_val = INT_MAX;
      else
        *from_val = 0, *to_val = val;
      break;

    case KHE_SRS_DOM_EQ_LOOSE:

      /* largest range of x st val <= x && (val >= x || val >= dt.min_limit) */
      if( val >= dt.min_limit )
	*from_val = val, *to_val = INT_MAX;
      else
        *from_val = val, *to_val = val;
      break;

    default:

      HnAbort("KheSrsDomTestDominatedRange internal error (%d)", dt.type);
      *from_val = *to_val = 0;  /* keep compiler happy */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSrsDomTestShow(KHE_SRS_DOM_TEST dt, char buff[100])             */
/*                                                                           */
/*  Show dt, using buff for scratch storage.                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheSrsDomTestShow(KHE_SRS_DOM_TEST dt, char buff[100])
{
  switch( dt.type )
  {
    case KHE_SRS_DOM_UNUSED:

      sprintf(buff, "-");
      break;

    case KHE_SRS_DOM_GE:

      sprintf(buff, ">=");
      break;

    case KHE_SRS_DOM_LE:

      sprintf(buff, "<=");
      break;

    case KHE_SRS_DOM_EQ:

      sprintf(buff, "==");
      break;

    case KHE_SRS_DOM_GE_LOOSE:

      sprintf(buff, ">=(%d)", dt.min_limit);
      break;

    case KHE_SRS_DOM_EQ_LOOSE:

      sprintf(buff, "=(%d)", dt.min_limit);
      break;

    default:

      sprintf(buff, "?");
      break;
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Helper code for expression types"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KheSrsExprInit(res, tg, dt, srs, free_list)                              */
/*                                                                           */
/*  Shared generic code for obtaining an expression object from a free list  */
/*  in srs and initializing those fields common to all expressions.          */
/*                                                                           */
/*****************************************************************************/

#define KheSrsExprInit(res, tg, dt, srs, free_list)			\
  if( HaArrayCount(srs->free_list) > 0 )				\
  {									\
    res = HaArrayLastAndDelete(srs->free_list);				\
    HaArrayClear(res->children);					\
    HaArrayClear(res->sig_indexes);					\
    HaArrayClear(res->child_indexes);					\
  }									\
  else									\
  {									\
    HaMake(res, srs->arena);						\
    HaArrayInit(res->children, srs->arena);				\
    HaArrayInit(res->sig_indexes, srs->arena);				\
    HaArrayInit(res->child_indexes, srs->arena);			\
  }									\
  res->tag = tg;							\
  res->deleted = false;							\
  res->first_day_index = res->last_day_index = UNDEF_INT;		\
  res->dom_test = dt


/*****************************************************************************/
/*                                                                           */
/*  Forward declarations of expression functions.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprReceiveValueLeaf(KHE_SRS_EXPR e, KHE_TASK task,
  KHE_SINGLE_RESOURCE_SOLVER srs);
static void KheSrsExprReceiveValueInt(KHE_SRS_EXPR e, int value,
  KHE_SINGLE_RESOURCE_SOLVER srs);
static void KheSrsExprReceiveValueFloat(KHE_SRS_EXPR e, float value,
  KHE_SINGLE_RESOURCE_SOLVER srs);
static void KheSrsExprReceiveValueCost(KHE_SRS_EXPR e, KHE_COST value,
  KHE_SINGLE_RESOURCE_SOLVER srs);
*** */

static void KheSrsExprDelete(KHE_SRS_EXPR e, KHE_SINGLE_RESOURCE_SOLVER srs);
static void KheSrsExprDebug(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsHasTimeExpr(KHE_SINGLE_RESOURCE_SOLVER srs,                   */
/*    KHE_SRS_EXPR_TAG tag, KHE_SRS_DOM_TEST dom_test, KHE_TIME time,        */
/*    KHE_SRS_EXPR *res_e)                                                   */
/*                                                                           */
/*  If a BUSY_TIME, FREE_TIME, or WORK_TIME expression exists with the       */
/*  given tag, dom_test, and time, then return true and set *res_e to it.    */
/*  Otherwise return false with *res_e set to NULL.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsHasTimeExpr(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_EXPR_TAG tag, KHE_SRS_DOM_TEST dom_test, KHE_TIME time,
  KHE_SRS_EXPR *res_e)
{
  int i;  KHE_SRS_DAY day;  KHE_SRS_EXPR e;
  KHE_SRS_EXPR_BUSY_TIME ebt;
  KHE_SRS_EXPR_FREE_TIME eft;
  KHE_SRS_EXPR_WORK_TIME ewt;
  i = KheFrameTimeIndex(srs->days_frame, time);
  day = HaArray(srs->days, i);
  HaArrayForEach(day->leaf_today, e, i)
    if( e->tag == tag && KheSrsDomTestEqual(e->dom_test, dom_test) )
    {
      switch( e->tag )
      {
	case KHE_SRS_EXPR_BUSY_TIME_TAG:

	  ebt = (KHE_SRS_EXPR_BUSY_TIME) e;
	  if( ebt->time == time )
	    return *res_e = e, true;
	  break;

	case KHE_SRS_EXPR_FREE_TIME_TAG:

	  eft = (KHE_SRS_EXPR_FREE_TIME) e;
	  if( eft->time == time )
	    return *res_e = e, true;
	  break;

	case KHE_SRS_EXPR_WORK_TIME_TAG:

	  ewt = (KHE_SRS_EXPR_WORK_TIME) e;
	  if( ewt->time == time )
	    return *res_e = e, true;
	  break;

	default:

	  HnAbort("KheSrsHasTimeExpr: invalid tag (%d)", e->tag);
      }
    }
  return *res_e = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsHasDayExpr(KHE_SINGLE_RESOURCE_SOLVER srs,                    */
/*    KHE_SRS_EXPR_TAG tag, KHE_SRS_DOM_TEST dom_test, KHE_TIME_GROUP tg,    */
/*    KHE_SRS_EXPR *res_e)                                                   */
/*                                                                           */
/*  If a BUSY_DAY, FREE_DAY, or WORK_DAY expression exists with the given    */
/*  tag, dom_test, and time group, then return true and set *res_e to it.    */
/*  Otherwise return false with *res_e set to NULL.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsHasDayExpr(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_EXPR_TAG tag, KHE_SRS_DOM_TEST dom_test, KHE_TIME_GROUP tg,
  KHE_SRS_EXPR *res_e)
{
  int i;  KHE_SRS_DAY day;  KHE_SRS_EXPR e;
  KHE_SRS_EXPR_BUSY_DAY ebd;
  KHE_SRS_EXPR_FREE_DAY efd;
  KHE_SRS_EXPR_WORK_DAY ewd;
  if( KheTimeGroupTimeCount(tg) > 0 )
  {
    i = KheFrameTimeIndex(srs->days_frame, KheTimeGroupTime(tg, 0));
    day = HaArray(srs->days, i);
    HnAssert(KheTimeGroupEqual(day->time_group, tg),
      "KheSrsHasDayExpr internal error");
    HaArrayForEach(day->leaf_today, e, i)
      if( e->tag == tag && KheSrsDomTestEqual(e->dom_test, dom_test) )
      {
	switch( e->tag )
	{
	  case KHE_SRS_EXPR_BUSY_DAY_TAG:

	    ebd = (KHE_SRS_EXPR_BUSY_DAY) e;
	    if( KheTimeGroupEqual(ebd->time_group, tg) )
	      return *res_e = e, true;
	    break;

	  case KHE_SRS_EXPR_FREE_DAY_TAG:

	    efd = (KHE_SRS_EXPR_FREE_DAY) e;
	    if( KheTimeGroupEqual(efd->time_group, tg) )
	      return *res_e = e, true;
	    break;

	  case KHE_SRS_EXPR_WORK_DAY_TAG:

	    ewd = (KHE_SRS_EXPR_WORK_DAY) e;
	    if( KheTimeGroupEqual(ewd->time_group, tg) )
	      return *res_e = e, true;
	    break;

	  default:

	    HnAbort("KheSrsHasDayExpr: invalid tag (%d)", e->tag);
	}
      }
  }
  return *res_e = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsAddTimeExpr(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME time,    */
/*    KHE_SRS_EXPR e)                                                        */
/*                                                                           */
/*  Add time expression e whose time is time to srs.                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsAddTimeExpr(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME time,
  KHE_SRS_EXPR e)
{
  int i;  KHE_SRS_DAY day;
  i = KheFrameTimeIndex(srs->days_frame, time);
  day = HaArray(srs->days, i);
  HaArrayAddLast(day->leaf_today, e);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsAddDayExpr(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP tg, */
/*    KHE_SRS_EXPR e)                                                        */
/*                                                                           */
/*  Add day expression e whose time group is tg to srs.                      */
/*                                                                           */
/*****************************************************************************/

static void KheSrsAddDayExpr(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP tg,
  KHE_SRS_EXPR e)
{
  int i;  KHE_SRS_DAY day;
  HnAssert(KheTimeGroupTimeCount(tg) > 0, "KheSrsAddDayExpr internal error 1");
  i = KheFrameTimeIndex(srs->days_frame, KheTimeGroupTime(tg, 0));
  day = HaArray(srs->days, i);
  HnAssert(KheTimeGroupEqual(day->time_group, tg),
    "KheSrsAddDayExpr internal error 2");
  HaArrayAddLast(day->leaf_today, e);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsExprIsActive(KHE_SRS_EXPR e, int day_index)                   */
/*                                                                           */
/*  Return true if e is active on day_index.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; good idea though
static bool KheSrsExprIsActive(KHE_SRS_EXPR e, int day_index)
{
  return e->first_day_index <= day_index && day_index <= e->last_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsExprInSignature(KHE_SRS_EXPR e, int day_index, int *si)       */
/*                                                                           */
/*  If e is in the signature on day_index, return true and set *si to its    */
/*  position in the signature.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsExprInSignature(KHE_SRS_EXPR e, int day_index, int *si)
{
  if( e->first_day_index <= day_index && day_index < e->last_day_index )
    return *si = HaArray(e->sig_indexes, day_index - e->first_day_index), true;
  else
    return *si = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsAddExprToTime(KHE_SINGLE_RESOURCE_SOLVER srs,                 */
/*    KHE_SRS_EXPR e, KHE_TIME time)                                         */
/*                                                                           */
/*  Add e to the list of expressions monitored by time.                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheSrsAddExprToTime(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_EXPR e, KHE_TIME time)
{
  KHE_SRS_TIME t;  KHE_SRS_DAY day;  int i;

  ** find the day **
  day = HaArray(srs->days, KheFrameTimeIndex(srs->days_frame, time));

  ** find the time, add e to it, and return **
  HaArrayForEach(day->times, t, i)
    if( t->time == time )
    {
      HaArrayAddLast(t->leaf_exprs, e);
      return;
    }

  ** error if we get to this point **
  HnAbort("KheSrsAddExprToTime internal error (time %s)", KheTimeId(time));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheExprMonotoneDomTest(KHE_SRS_EXPR e)                  */
/*                                                                           */
/*  Return a dom test suitable to a node whose function is monotone          */
/*  increasing in its arguments, and which has a parent.                     */
/*                                                                           */
/*****************************************************************************/

/* *** decided agains this kind of general case analysis
static KHE_SRS_DOM_TEST KheExprMonotoneDomTest(KHE_SRS_EXPR e)
{
  ** stil l to do: is this right? (probably not) **
  HnAssert(e->parent != NULL, "KheExprMonotoneDomTest internal error");
  return KheSrsDomTestMake(e->parent->dom_test.type, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsExprIntBeginDay(KHE_SRS_EXPR e, int day_index, int init_val,   */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  For expressions e whose value is an integer, return the value at the     */
/*  beginning of day day_index.  If this is e's first day, the value         */
/*  returned is init_val.  Otherwise the value returned is the value         */
/*  stored in prev_ps at the end of the previous day.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheSrsExprIntBeginDay(KHE_SRS_EXPR e, int day_index, int init_val,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs, char *label)
{
  int sig_index;
  HnAssert(e->first_day_index <= day_index && day_index <= e->last_day_index,
    "KheSrsExprIntBeginDay %s internal error (%d not in range %d..%d)",
    label, day_index, e->first_day_index, e->last_day_index);
  if( day_index == e->first_day_index )
  {
    ** this is the start of e's first day **
    return init_val;
  }
  else
  {
    ** this is not e's first day; get previous day's value from prev_ps **
    sig_index = HaArray(e->sig_indexes, day_index - 1 - e->first_day_index);
    return HaArray(prev_ps->signature, sig_index);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheSrsExprFloatBeginDay(KHE_SRS_EXPR e, int day_index,             */
/*    float init_val, KHE_SRS_PARTIAL_SOLN prev_ps,                          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  For expressions e whose value is a float, return the value at the        */
/*  beginning of day day_index.  If this is e's first day, the value         */
/*  returned is init_val.  Otherwise the value returned is the value         */
/*  stored in prev_ps at the end of the previous day.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static float KheSrsExprFloatBeginDay(KHE_SRS_EXPR e, int day_index,
  float init_val, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int sig_index;
  HnAssert(e->first_day_index <= day_index && day_index <= e->last_day_index,
    "KheSrsExprFloatBeginDay internal error");
  if( day_index == e->first_day_index )
  {
    ** this is the start of e's first day **
    return init_val;
  }
  else
  {
    ** this is not e's first day; get previous day's value from prev_ps **
    sig_index = HaArray(e->sig_indexes, day_index - 1 - e->first_day_index);
    return (float) HaArray(prev_ps->signature, sig_index) / 100.0;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsExprIntEndDay(KHE_SRS_EXPR e, int day_index, int final_val,    */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  For expressions e whose value is an integer, final_val is the final      */
/*  value of e on day day_index, which is now ending.  If this is e's last   */
/*  day, report final_val to the parent.  If this is not e's last day,       */
/*  store final_val in ps.                                                   */
/*                                                                           */
/*  The return value is always UNDEF_INT.  This is for the convenience of    */
/*  the callers.  The UNDEF_INT is used to replace final_val in e.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheSrsExprIntEndDay(KHE_SRS_EXPR e, int day_index, int final_val,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int sig_index;
  HnAssert(e->first_day_index <= day_index && day_index <= e->last_day_index,
    "KheSrsExprIntEndDay internal error 1");
  HnAssert(final_val >= 0, "KheSrsExprIntEndDay internal error 2");
  if( day_index == e->last_day_index )
  {
    ** this is the end of e's last day; report final_val to parent **
    KheSrsExprReceiveValueInt(e->parent, final_val, srs);
  }
  else
  {
    ** this is the end of a day other than e's last; store final_val in ps **
    sig_index = HaArray(e->sig_indexes, day_index - e->first_day_index);
    HnAssert(HaArrayCount(ps->signature) == sig_index,
      "KheSrsExprIntEndDay internal error 3");
    HaArrayAddLast(ps->signature, final_val);
  }
  return UNDEF_INT;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsExprFloatEndDay(KHE_SRS_EXPR e, int day_index,                 */
/*    int final_val, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)*/
/*                                                                           */
/*  For expressions e whose value is an integer, final_val is the final      */
/*  value of e on day day_index, which is now ending.  If this is e's last   */
/*  day, report final_val to the parent if report is true.  If this is not   */
/*  e's last day, store final_val in ps.                                     */
/*                                                                           */
/*  The return value is always UNDEF_FLOAT.  This is for the convenience of  */
/*  the callers.  The UNDEF_FLOAT is used to replace final_val in e.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static float KheSrsExprFloatEndDay(KHE_SRS_EXPR e, int day_index,
  float final_val, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int sig_index;
  HnAssert(e->first_day_index <= day_index && day_index <= e->last_day_index,
    "KheSrsExprFloatEndDay internal error 1");
  HnAssert(final_val >= 0.0, "KheSrsExprFloatEndDay internal error 2");
  if( day_index == e->last_day_index )
  {
    ** this is the end of e's last day; report final_val to parent **
    KheSrsExprReceiveValueFloat(e->parent, final_val, srs);
  }
  else
  {
    ** this is the end of a day other than e's last; store final_val in ps **
    sig_index = HaArray(e->sig_indexes, day_index - e->first_day_index);
    HnAssert(HaArrayCount(ps->signature) == sig_index,
      "KheSrsExprFloatEndDay internal error 3");
    HaArrayAddLast(ps->signature, (int) (final_val * 100.0));
  }
  return -1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntPartialSolnDebug(KHE_SRS_EXPR e, char *label,          */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, FILE *fp)                                               */
/*                                                                           */
/*  Debug print of e in ps onto fp.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntPartialSolnDebug(KHE_SRS_EXPR e, char *label,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, FILE *fp)
{
  int sig_index;
  sig_index = HaArray(e->sig_indexes, ps->day->day_index - e->first_day_index);
  fprintf(fp, "%s %d", label, HaArray(ps->signature, sig_index));
  ** ***
  if( tmp_value != -1 )
    fprintf(fp, "$%d", tmp_value);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatPartialSolnDebug(KHE_SRS_EXPR e, char *label,        */
/*    float tmp_value, KHE_SRS_PARTIAL_SOLN ps,                              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header part of e in ps onto fp.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatPartialSolnDebug(KHE_SRS_EXPR e, char *label,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, FILE *fp)
{
  int sig_index;
  sig_index = HaArray(e->sig_indexes, ps->day->day_index - e->first_day_index);
  fprintf(fp, "%s %.2f", label,
    (float) HaArray(ps->signature, sig_index) / 100.0);
  ** ***
  if( tmp_value != -1.0 )
    fprintf(fp, "$%.2f", tmp_value);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsExprFirstDebugDayIndex(KHE_SRS_EXPR e)                         */
/*                                                                           */
/*  The minimum, over e and its descendants, of the first_day_index field    */
/*  of e.                                                                    */
/*                                                                           */
/*  Implementation note.  We only search down the first children.  If that   */
/*  is wrong in some obscure case, it doesn't matter, because this function  */
/*  is called only when debugging.                                           */
/*                                                                           */
/*****************************************************************************/

static int KheSrsExprFirstDebugDayIndex(KHE_SRS_EXPR e)
{
  int res;
  res = e->first_day_index;
  while( HaArrayCount(e->children) > 0 )
  {
    e = HaArrayFirst(e->children);
    if( e->first_day_index < res )
      res = e->first_day_index;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprDebugChildren(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Carry out the part of KheSrsExprDebug which prints the children.         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprDebugChildren(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR child_e;  int i, di, first_day_index;  bool child_printed;
  if( ps == NULL )
  {
    /* print all children */
    di = e->first_day_index;
    HaArrayForEach(e->children, child_e, i)
    {
      for( ;  HaArray(e->child_indexes, di - e->first_day_index) <= i;  di++ )
	fprintf(fp, "%*sending on %s:\n", indent + 2, "",
	  KheTimeGroupId(KheFrameTimeGroup(srs->days_frame, di)));
      KheSrsExprDebug(child_e, ps, srs, verbosity, indent + 2, fp);
    }
  }
  else
  {
    /* print the children with a descendant with an entry in the signature */
    if( HaArrayCount(e->children) > 0 )
    {
      fprintf(fp, "(");
      child_printed = false;
      HaArrayForEach(e->children, child_e, i)
      {
	first_day_index = KheSrsExprFirstDebugDayIndex(child_e);
	if( first_day_index <= ps->day->day_index &&
	    ps->day->day_index <= child_e->last_day_index )
	{
	  if( child_printed )
	    fprintf(fp, ", ");
	  KheSrsExprDebug(child_e, ps, srs, verbosity, -1, fp);
	  child_printed = true;
	}
      }
      fprintf(fp, ")");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSrsExprDayRangeShow(KHE_SRS_EXPR e,                             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, char buff[100])                        */
/*                                                                           */
/*  Return a string showing the day range of e onto fp.  Use buff for        */
/*  scratch memory.                                                          */
/*                                                                           */
/*****************************************************************************/

static char *KheSrsExprDayRangeShow(KHE_SRS_EXPR e,
  KHE_SINGLE_RESOURCE_SOLVER srs, char buff[100])
{
  KHE_TIME_GROUP tg1, tg2;
  if( e->first_day_index == e->last_day_index )
  {
    tg1 = KheFrameTimeGroup(srs->days_frame, e->first_day_index);
    snprintf(buff, 100, "%s", KheTimeGroupId(tg1));
  }
  else
  {
    tg1 = KheFrameTimeGroup(srs->days_frame, e->first_day_index);
    tg2 = KheFrameTimeGroup(srs->days_frame, e->last_day_index);
    snprintf(buff, 100, "%s-%s", KheTimeGroupId(tg1), KheTimeGroupId(tg2));
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDebug(KHE_SRS_EXPR e, char *label, int val,            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug e, for expression types that store an integer value.  The value    */
/*  is passed to this function in val.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntDebug(KHE_SRS_EXPR e, char *label, int val,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR child_e;  int i;  char buff[100];
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheSrsExprDebugDayRange(e, srs, fp);
    fprintf(fp, " %s", KheSrsDomTestShow(e->dom_test, buff));
    fprintf(fp, " %s", label);
    if( val != UNDEF_INT )
      fprintf(fp, " (val %d)", val);
    fprintf(fp, "\n");
    HaArrayForEach(e->children, child_e, i)
      KheSrsExprDebug(child_e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "%s(%d children)", label, HaArrayCount(e->children));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatPartialSolnDebug(KHE_SRS_EXPR e, char *label,        */
/*    float tmp_value, KHE_SRS_PARTIAL_SOLN ps,                              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header part of e in ps onto fp.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatPartialSolnDebug(KHE_SRS_EXPR e, char *label,
  float tmp_value, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)
{
  int sig_index;
  sig_index = HaArray(e->sig_indexes, ps->day->day_index - e->first_day_index);
  fprintf(fp, "%s %.2f", label,
    (float) HaArray(ps->signature, sig_index) / 100.0);
  if( tmp_value != -1.0 )
    fprintf(fp, "$%.2f", tmp_value);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDebug(KHE_SRS_EXPR e, char *label, float val,        */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug e, for expression types that store a float value.  The value is    */
/*  passed to this function in val.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatDebug(KHE_SRS_EXPR e, char *label, float val,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR child_e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheSrsExprDebugDayRange(e, srs, fp);
    fprintf(fp, " %s", label);
    if( val != UNDEF_FLOAT )
      fprintf(fp, " (val %.2f)", val);
    fprintf(fp, "\n");
    HaArrayForEach(e->children, child_e, i)
      KheSrsExprDebug(child_e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "%s(%d children)", label, HaArrayCount(e->children));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_BUSY_TIME"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_TIME KheSrsExprBusyTimeMake(KHE_TIME time,             */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_BUSY_TIME object with these attributes.          */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_BUSY_TIME KheSrsExprBusyTimeMake(KHE_TIME time,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_BUSY_TIME res;  KHE_SRS_EXPR e;
  if( KheSrsHasTimeExpr(srs, KHE_SRS_EXPR_BUSY_TIME_TAG, dom_test, time, &e) )
    return (KHE_SRS_EXPR_BUSY_TIME) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_BUSY_TIME_TAG, dom_test, srs,
    free_expr_busy_time);
  res->value = UNDEF_INT;
  res->time = time;
  /* KheSrsAddExprToTime(srs, (KHE_SRS_EXPR) res, time); */
  KheSrsAddTimeExpr(srs, time, (KHE_SRS_EXPR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_TIME KheSrsExprBusyTimeCopy(KHE_SRS_EXPR_BUSY_TIME ebt,*/
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of ebt.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_BUSY_TIME KheSrsExprBusyTimeCopy(KHE_SRS_EXPR_BUSY_TIME ebt,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_BUSY_TIME res;
  KheSrsExprInit(res, KHE_SRS_EXPR_BUSY_TIME_TAG, ebt->dom_test, srs,
    free_expr_busy_time);
  res->first_day_index = ebt->first_day_index;
  res->last_day_index = ebt->last_day_index;
  res->value = ebt->value;
  res->time = ebt->time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeDelete(KHE_SRS_EXPR_BUSY_TIME ebt,                 */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ebt and its descendants (there are no descendants here).          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyTimeDelete(KHE_SRS_EXPR_BUSY_TIME ebt,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_busy_time, ebt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeDoDay(KHE_SRS_EXPR_BUSY_TIME ebt, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ebt on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyTimeDoDay(KHE_SRS_EXPR_BUSY_TIME ebt, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == ebt->first_day_index )
  {
    /* this is ebt's first day */
    ebt->value = (time == ebt->time ? 1 : 0);
  }
  else
  {
    /* this is not ebt's first day, so we need to retrieve a previous value */
    si = HaArray(ebt->sig_indexes, day_index - 1 - ebt->first_day_index);
    ebt->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < ebt->last_day_index )
  {
    /* this not ebt's last day; store value in next_ps */
    si = HaArray(ebt->sig_indexes, day_index - ebt->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprBusyTimeDoDay internal error");
    HaArrayAddLast(next_ps->signature, ebt->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeBeginDay(KHE_SRS_EXPR_BUSY_TIME eb, int day_index,*/
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform eb that day day_index is beginning.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprBusyTimeBeginDay(KHE_SRS_EXPR_BUSY_TIME eb, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  if( DEBUG8 )
    fprintf(stderr, "ExprBusyBeginDay(time %s)\n", KheTimeId(eb->time));
  if( day_index == e->first_day_index )
  {
    ** this is the start of e's first day **
    eb->value = (time == eb->time ? 1 : 0);
  }
  else
  {
    ** this is not the first day, so we need to retrieve the value **
    sig_index = HaArray(e->sig_indexes, day_index - 1 - e->first_day_index);
    eb->value = HaArray(prev_ps->signature, sig_index);
  }
  if( day_index == e->last_day_index )
  {
    ** this is the end of e's last day; the value will be retrieved by prnt **
  }
  else
  {
    ** this is the end of a day other than e's last; store final_val in ps **
    sig_index = HaArray(e->sig_indexes, day_index - e->first_day_index);
    HnAssert(HaArrayCount(ps->signature) == sig_index,
      "KheSrsExprIntEndDay internal error 3");
    HaArrayAddLast(ps->signature, final_val);

  KheSrsExprIntBeginDay((KHE_SRS_EXPR) eb, day_index, 0, prev_ps, srs, "Busy");
  KheSrsExprIntEndDay((KHE_SRS_EXPR) eb, day_index, eb->value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeReceiveValueLeaf(KHE_SRS_EXPR_BUSY_TIME eb,            */
/*    KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)                         */
/*                                                                           */
/*  Inform eb that the resource is busy, assigned to task, at its time.      */
/*  Or if task == NULL, the resource is free at its time.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprBusyTimeReceiveValueLeaf(KHE_SRS_EXPR_BUSY_TIME eb,
  KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eb->tmp_value = (task != NULL ? 1 : 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeEndDay(KHE_SRS_EXPR_BUSY_TIME eb, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform eb that day day_index is ending.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprBusyTimeEndDay(KHE_SRS_EXPR_BUSY_TIME eb, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eb->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) eb, day_index,
    eb->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeDebug(KHE_SRS_EXPR_BUSY_TIME ebt,                 */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for busy time expression ebt.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyTimeDebug(KHE_SRS_EXPR_BUSY_TIME ebt,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "BUSY_TIME(%s)", KheTimeId(ebt->time));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ebt, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ BUSY_TIME(%s) %s %s ]", KheTimeId(ebt->time),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ebt, srs, buff1),
      KheSrsDomTestShow(ebt->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyTimeDebug(KHE_SRS_EXPR_BUSY_TIME eb,                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eb onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprBusyTimeDebug(KHE_SRS_EXPR_BUSY_TIME eb,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  char buff[100];
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ BUSY_TIME(%s) %s ]", KheTimeId(eb->time),
    KheSrsDomTestShow(eb->dom_test, buff));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_FREE_TIME"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_TIME KheSrsExprFreeTimeMake(KHE_TIME time,             */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_FREE_TIME object with these attributes.          */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FREE_TIME KheSrsExprFreeTimeMake(KHE_TIME time,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FREE_TIME res;  KHE_SRS_EXPR e;
  if( KheSrsHasTimeExpr(srs, KHE_SRS_EXPR_FREE_TIME_TAG, dom_test, time, &e) )
    return (KHE_SRS_EXPR_FREE_TIME) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_FREE_TIME_TAG, dom_test, srs,
    free_expr_free_time);
  res->value = UNDEF_INT;
  res->time = time;
  KheSrsAddTimeExpr(srs, time, (KHE_SRS_EXPR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_TIME KheSrsExprFreeTimeCopy(KHE_SRS_EXPR_FREE_TIME eft,*/
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of eft.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FREE_TIME KheSrsExprFreeTimeCopy(KHE_SRS_EXPR_FREE_TIME eft,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FREE_TIME res;
  KheSrsExprInit(res, KHE_SRS_EXPR_FREE_TIME_TAG, eft->dom_test, srs,
    free_expr_free_time);
  res->first_day_index = eft->first_day_index;
  res->last_day_index = eft->last_day_index;
  res->value = eft->value;
  res->time = eft->time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeTimeDelete(KHE_SRS_EXPR_FREE_TIME ef,                 */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ef and its descendants (there are no descendants here).           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeTimeDelete(KHE_SRS_EXPR_FREE_TIME ef,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_free_time, ef);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprFreeDomTest(KHE_SRS_EXPR_FREE_TIME ef)        */
/*                                                                           */
/*  Return a suitable dom test for ef.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprFreeDomTest(KHE_SRS_EXPR_FREE_TIME ef)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) ef);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeTimeDoDay(KHE_SRS_EXPR_FREE_TIME eft, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node eft on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeTimeDoDay(KHE_SRS_EXPR_FREE_TIME eft, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == eft->first_day_index )
  {
    /* this is eft's first day */
    eft->value = (time == eft->time ? 0 : 1);
  }
  else
  {
    /* this is not eft's first day, so we need to retrieve a previous value */
    si = HaArray(eft->sig_indexes, day_index - 1 - eft->first_day_index);
    eft->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < eft->last_day_index )
  {
    /* this not eft's last day; store value in next_ps */
    si = HaArray(eft->sig_indexes, day_index - eft->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprFreeTimeDoDay internal error");
    HaArrayAddLast(next_ps->signature, eft->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeBeginDay(KHE_SRS_EXPR_FREE_TIME ef, int day_index,    */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform ef that day day_index is beginning.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFreeBeginDay(KHE_SRS_EXPR_FREE_TIME ef, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  if( DEBUG8 )
    fprintf(stderr, "ExprFreeBeginDay(time %s)\n", KheTimeId(ef->time));
  ef->tmp_value = KheSrsExprIntBeginDay((KHE_SRS_EXPR) ef, day_index,
    0, prev_ps, srs, "Free");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeReceiveValueLeaf(KHE_SRS_EXPR_FREE_TIME ef,           */
/*    KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)                         */
/*                                                                           */
/*  Inform ef that the resource is busy, assigned to task, at its time.      */
/*  Or if task == NULL, the resource is free at its time.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFreeReceiveValueLeaf(KHE_SRS_EXPR_FREE_TIME ef,
  KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ef->tmp_value = (task == NULL ? 1 : 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeEndDay(KHE_SRS_EXPR_FREE_TIME ef, int day_index,      */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform ef that day day_index is ending.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFreeEndDay(KHE_SRS_EXPR_FREE_TIME ef, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ef->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) ef, day_index,
    ef->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeTimeDebug(KHE_SRS_EXPR_FREE_TIME eft,                 */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for free time expression eft.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeTimeDebug(KHE_SRS_EXPR_FREE_TIME eft,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "FREE_TIME(%s)", KheTimeId(eft->time));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eft, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ FREE_TIME(%s) %s %s ]", KheTimeId(eft->time),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eft, srs, buff1),
      KheSrsDomTestShow(eft->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeDebug(KHE_SRS_EXPR_FREE_TIME eft,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eft onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFreeDebug(KHE_SRS_EXPR_FREE_TIME eft,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ FREE(%s) ]", KheTimeId(eft->time));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_WORK_TIME"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_TIME KheSrsExprWorkTimeMake(KHE_TIME time,             */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_WORK_TIME object with these attributes.          */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_WORK_TIME KheSrsExprWorkTimeMake(KHE_TIME time,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_WORK_TIME res;  KHE_SRS_EXPR e;
  if( KheSrsHasTimeExpr(srs, KHE_SRS_EXPR_WORK_TIME_TAG, dom_test, time, &e) )
    return (KHE_SRS_EXPR_WORK_TIME) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_WORK_TIME_TAG, dom_test, srs,
    free_expr_work_time);
  res->value = UNDEF_FLOAT;
  res->time = time;
  KheSrsAddTimeExpr(srs, time, (KHE_SRS_EXPR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_TIME KheSrsExprWorkTimeCopy(KHE_SRS_EXPR_WORK_TIME ewt,*/
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of ewt.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_WORK_TIME KheSrsExprWorkTimeCopy(KHE_SRS_EXPR_WORK_TIME ewt,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_WORK_TIME res;
  KheSrsExprInit(res, KHE_SRS_EXPR_WORK_TIME_TAG, ewt->dom_test, srs,
    free_expr_work_time);
  res->first_day_index = ewt->first_day_index;
  res->last_day_index = ewt->last_day_index;
  res->value = ewt->value;
  res->time = ewt->time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeDelete(KHE_SRS_EXPR_WORK_TIME ewt,                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ewt and its descendants (there are no descendants here).          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkTimeDelete(KHE_SRS_EXPR_WORK_TIME ewt,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_work_time, ewt);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprWorkTimeDomTest(KHE_SRS_EXPR_WORK_TIME ewt)   */
/*                                                                           */
/*  Return a suitable dom test for ewt.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprWorkTimeDomTest(KHE_SRS_EXPR_WORK_TIME ewt)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) ewt);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeBeginDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index,*/
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform ewt that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprWorkTimeBeginDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ewt->tmp_value = KheSrsExprFloatBeginDay((KHE_SRS_EXPR) ewt, day_index,
    UNDEF_FLOAT, prev_ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeReceiveValueLeaf(KHE_SRS_EXPR_WORK_TIME ewt,      */
/*    KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)                         */
/*                                                                           */
/*  Inform ewt that the resource is busy, assigned to task, at its time.     */
/*  Or if task == NULL, the resource is free at its time.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprWorkTimeReceiveValueLeaf(KHE_SRS_EXPR_WORK_TIME ewt,
  KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ewt->tmp_value = KheTaskWorkloadPerTime(task);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeEndDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index, */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform ewt that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprWorkTimeEndDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ewt->tmp_value = KheSrsExprFloatEndDay((KHE_SRS_EXPR) ewt, day_index,
    ewt->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeDoDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ewt on day_index.                                    */
/*                                                                           */
/*  Implementation note.  If time == ewt->time, then since ewt->time is      */
/*  always non-NULL, time is non-NULL, and this implies task is non-NULL.    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkTimeDoDay(KHE_SRS_EXPR_WORK_TIME ewt, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == ewt->first_day_index )
  {
    /* this is ewt's first day */
    ewt->value = (time == ewt->time ? KheTaskWorkloadPerTime(task) : 0.0);
  }
  else
  {
    /* this is not ewt's first day, so we need to retrieve a previous value */
    si = HaArray(ewt->sig_indexes, day_index - 1 - ewt->first_day_index);
    ewt->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < ewt->last_day_index )
  {
    /* this not ewt's last day; store value in next_ps */
    si = HaArray(ewt->sig_indexes, day_index - ewt->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprWorkTimeDoDay internal error");
    HaArrayAddLast(next_ps->signature, ewt->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeDebug(KHE_SRS_EXPR_WORK_TIME ewt,                 */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for work time expression ewt.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkTimeDebug(KHE_SRS_EXPR_WORK_TIME ewt,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "WORK_TIME(%s)", KheTimeId(ewt->time));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ewt, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%.2f$", si, (float) HaArray(ps->signature, si) / 100.0);
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ WORK_TIME(%s) %s %s ]", KheTimeId(ewt->time),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ewt, srs, buff1),
      KheSrsDomTestShow(ewt->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkTimeDebug(KHE_SRS_EXPR_WORK_TIME ewt,                 */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of ewt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprWorkTimeDebug(KHE_SRS_EXPR_WORK_TIME ewt,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ WORK(%s) ]", KheTimeId(ewt->time));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_BUSY_DAY"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_DAY KheSrsExprBusyDayMake(KHE_TIME_GROUP tg,           */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_BUSY_DAY object with these attributes.           */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_BUSY_DAY KheSrsExprBusyDayMake(KHE_TIME_GROUP tg,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_BUSY_DAY res;  KHE_SRS_EXPR e;
  if( KheSrsHasDayExpr(srs, KHE_SRS_EXPR_BUSY_DAY_TAG, dom_test, tg, &e) )
    return (KHE_SRS_EXPR_BUSY_DAY) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_BUSY_DAY_TAG, dom_test, srs,
    free_expr_busy_day);
  res->value = UNDEF_INT;
  res->time_group = tg;
  KheSrsAddDayExpr(srs, tg, (KHE_SRS_EXPR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_BUSY_DAY KheSrsExprBusyDayCopy(KHE_SRS_EXPR_BUSY_DAY ebd,   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of ebd.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_BUSY_DAY KheSrsExprBusyDayCopy(KHE_SRS_EXPR_BUSY_DAY ebd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_BUSY_DAY res;
  KheSrsExprInit(res, KHE_SRS_EXPR_BUSY_DAY_TAG, ebd->dom_test, srs,
    free_expr_busy_day);
  res->first_day_index = ebd->first_day_index;
  res->last_day_index = ebd->last_day_index;
  res->value = ebd->value;
  res->time_group = ebd->time_group;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyDayDelete(KHE_SRS_EXPR_BUSY_DAY ebd,                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ebd and its descendants (there are no descendants here).          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyDayDelete(KHE_SRS_EXPR_BUSY_DAY ebd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_busy_day, ebd);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyDayDoDay(KHE_SRS_EXPR_BUSY_DAY ebd, int day_index,    */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ebd on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyDayDoDay(KHE_SRS_EXPR_BUSY_DAY ebd, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == ebd->first_day_index )
  {
    /* this is ebd's first day */
    ebd->value = (time != NULL ? 1 : 0);
  }
  else
  {
    /* this is not ebd's first day, so we need to retrieve a previous value */
    si = HaArray(ebd->sig_indexes, day_index - 1 - ebd->first_day_index);
    ebd->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < ebd->last_day_index )
  {
    /* this not ebd's last day; store value in next_ps */
    si = HaArray(ebd->sig_indexes, day_index - ebd->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprBusyDayDoDay internal error");
    HaArrayAddLast(next_ps->signature, ebd->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBusyDayDebug(KHE_SRS_EXPR_BUSY_DAY ebd,                   */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for busy day expression ebd.                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprBusyDayDebug(KHE_SRS_EXPR_BUSY_DAY ebd,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "BUSY_DAY(%s)", KheTimeGroupId(ebd->time_group));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ebd, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ BUSY_DAY(%s) %s %s ]", KheTimeGroupId(ebd->time_group),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ebd, srs, buff1),
      KheSrsDomTestShow(ebd->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_FREE_DAY"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_DAY KheSrsExprFreeDayMake(KHE_TIME_GROUP tg,           */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_FREE_DAY object with these attributes.           */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FREE_DAY KheSrsExprFreeDayMake(KHE_TIME_GROUP tg,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FREE_DAY res;  KHE_SRS_EXPR e;
  if( KheSrsHasDayExpr(srs, KHE_SRS_EXPR_FREE_DAY_TAG, dom_test, tg, &e) )
    return (KHE_SRS_EXPR_FREE_DAY) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_FREE_DAY_TAG, dom_test, srs,
    free_expr_free_day);
  res->value = UNDEF_INT;
  res->time_group = tg;
  KheSrsAddDayExpr(srs, tg, (KHE_SRS_EXPR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FREE_DAY KheSrsExprFreeDayCopy(KHE_SRS_EXPR_FREE_DAY efd,   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of efd.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FREE_DAY KheSrsExprFreeDayCopy(KHE_SRS_EXPR_FREE_DAY efd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FREE_DAY res;
  KheSrsExprInit(res, KHE_SRS_EXPR_FREE_DAY_TAG, efd->dom_test, srs,
    free_expr_free_day);
  res->first_day_index = efd->first_day_index;
  res->last_day_index = efd->last_day_index;
  res->value = efd->value;
  res->time_group = efd->time_group;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeDayDelete(KHE_SRS_EXPR_FREE_DAY efd,                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete efd and its descendants (there are no descendants here).          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeDayDelete(KHE_SRS_EXPR_FREE_DAY efd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_free_day, efd);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeDayDoDay(KHE_SRS_EXPR_FREE_DAY efd, int day_index,    */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node efd on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeDayDoDay(KHE_SRS_EXPR_FREE_DAY efd, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == efd->first_day_index )
  {
    /* this is efd's first day */
    efd->value = (time == NULL ? 1 : 0);
  }
  else
  {
    /* this is not efd's first day, so we need to retrieve a previous value */
    si = HaArray(efd->sig_indexes, day_index - 1 - efd->first_day_index);
    efd->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < efd->last_day_index )
  {
    /* this not efd's last day; store value in next_ps */
    si = HaArray(efd->sig_indexes, day_index - efd->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprFreeDayDoDay internal error");
    HaArrayAddLast(next_ps->signature, efd->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFreeDayDebug(KHE_SRS_EXPR_FREE_DAY efd,                   */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for free time expression efd.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFreeDayDebug(KHE_SRS_EXPR_FREE_DAY efd,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "FREE_DAY(%s)", KheTimeGroupId(efd->time_group));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) efd, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ FREE_DAY(%s) %s %s ]", KheTimeGroupId(efd->time_group),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) efd, srs, buff1),
      KheSrsDomTestShow(efd->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_WORK_DAY"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_DAY KheSrsExprWorkDayMake(KHE_TIME_GROUP tg,           */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_WORK_DAY object with these attributes.           */
/*  Or retrieve an equivalent existing expression if there is one.           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused, although I really should be using it
static KHE_SRS_EXPR_WORK_DAY KheSrsExprWorkDayMake(KHE_TIME_GROUP tg,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_WORK_DAY res;  KHE_SRS_EXPR e;
  if( KheSrsHasDayExpr(srs, KHE_SRS_EXPR_WORK_DAY_TAG, dom_test, tg, &e) )
    return (KHE_SRS_EXPR_WORK_DAY) e;
  KheSrsExprInit(res, KHE_SRS_EXPR_WORK_DAY_TAG, dom_test, srs,
    free_expr_work_day);
  res->value = UNDEF_INT;
  res->time_group = tg;
  KheSrsAddDayExpr(srs, tg, (KHE_SRS_EXPR) res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_WORK_DAY KheSrsExprWorkDayCopy(KHE_SRS_EXPR_WORK_DAY ewd,   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a fresh and unshared copy of ewd.  Do not copy the sig_indexes    */
/*  and child_indexes arrays; they should be empty anyway.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_WORK_DAY KheSrsExprWorkDayCopy(KHE_SRS_EXPR_WORK_DAY ewd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_WORK_DAY res;
  KheSrsExprInit(res, KHE_SRS_EXPR_WORK_DAY_TAG, ewd->dom_test, srs,
    free_expr_work_day);
  res->first_day_index = ewd->first_day_index;
  res->last_day_index = ewd->last_day_index;
  res->value = ewd->value;
  res->time_group = ewd->time_group;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkDayDelete(KHE_SRS_EXPR_WORK_DAY ewd,                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ewd and its descendants (there are no descendants here).          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkDayDelete(KHE_SRS_EXPR_WORK_DAY ewd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_expr_work_day, ewd);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkDayDoDay(KHE_SRS_EXPR_WORK_DAY ewd, int day_index,    */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ewd on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkDayDoDay(KHE_SRS_EXPR_WORK_DAY ewd, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;
  if( day_index == ewd->first_day_index )
  {
    /* this is ewd's first day */
    ewd->value = (task != NULL ? KheTaskWorkloadPerTime(task) : 0.0);
  }
  else
  {
    /* this is not ewd's first day, so we need to retrieve a previous value */
    si = HaArray(ewd->sig_indexes, day_index - 1 - ewd->first_day_index);
    ewd->value = HaArray(prev_ps->signature, si);
  }
  if( day_index < ewd->last_day_index )
  {
    /* this not ewd's last day; store value in next_ps */
    si = HaArray(ewd->sig_indexes, day_index - ewd->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprWorkDayDoDay internal error");
    HaArrayAddLast(next_ps->signature, ewd->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprWorkDayDebug(KHE_SRS_EXPR_WORK_DAY ewd,                   */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for work day expression ewd.                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprWorkDayDebug(KHE_SRS_EXPR_WORK_DAY ewd,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "WORK_DAY(%s)", KheTimeGroupId(ewd->time_group));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ewd, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
  }
  else
  {
    /* indented version without a ps value */
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ WORK_DAY(%s) %s %s ]", KheTimeGroupId(ewd->time_group),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ewd, srs, buff1),
      KheSrsDomTestShow(ewd->dom_test, buff2));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_OR"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_OR KheSrsExprOrMake(KHE_SRS_DOM_TEST dom_test,              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_OR object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_OR KheSrsExprOrMake(KHE_SRS_DOM_TEST dom_test,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_OR res;
  KheSrsExprInit(res, KHE_SRS_EXPR_OR_TAG, dom_test, srs, free_expr_or);
  res->value = UNDEF_INT;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrDelete(KHE_SRS_EXPR_OR eo,                              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eo and its descendants.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprOrDelete(KHE_SRS_EXPR_OR eo,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eo->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_or, eo);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrDoDay(KHE_SRS_EXPR_OR eo, int day_index,                */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node eo on day_index.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprOrDoDay(KHE_SRS_EXPR_OR eo, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, start_index, stop_index;  KHE_SRS_EXPR_INT child_e;

  /* initialize eo->value depending on whether day_index is its first day */
  if( day_index == eo->first_day_index )
    eo->value = 0;
  else
  {
    /* this is not eo's first day, so we need to retrieve a previous value */
    si = HaArray(eo->sig_indexes, day_index - 1 - eo->first_day_index);
    eo->value = HaArray(prev_ps->signature, si);
  }

  /* accumulate the values of the children of eo that finalized today */
  start_index = HaArray(eo->child_indexes, day_index - eo->first_day_index);
  stop_index =  HaArray(eo->child_indexes, day_index + 1 - eo->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_INT) HaArray(eo->children, i);
    if( child_e->value == 1 )
      eo->value = 1;
  }

  if( day_index < eo->last_day_index )
  {
    /* this not eo's last day; store value in next_ps */
    si = HaArray(eo->sig_indexes, day_index - eo->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprOrDoDay internal error");
    HaArrayAddLast(next_ps->signature, eo->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprOrDomTest(KHE_SRS_EXPR_OR eo)                 */
/*                                                                           */
/*  Return a suitable dom test for eo.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprOrDomTest(KHE_SRS_EXPR_OR eo)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) eo);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrBeginDay(KHE_SRS_EXPR_OR eo, int day_index,             */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform eo that day day_index is beginning.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprOrBeginDay(KHE_SRS_EXPR_OR eo, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eo->tmp_value = KheSrsExprIntBeginDay((KHE_SRS_EXPR) eo, day_index, 0,
    prev_ps, srs, "Or");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrReceiveValueInt(KHE_SRS_EXPR_OR eo, int value,          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform eo that one of its children has just finalized its value.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprOrReceiveValueInt(KHE_SRS_EXPR_OR eo, int value,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(eo->tmp_value != UNDEF_INT, "KheSrsExprOrReceiveValueInt internal error");
  if( value == 1 )
    eo->tmp_value = 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrEndDay(KHE_SRS_EXPR_OR eo, int day_index,               */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform eo that day day_index is ending.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprOrEndDay(KHE_SRS_EXPR_OR eo, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eo->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) eo, day_index,
    eo->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrDebug(KHE_SRS_EXPR_OR eo,                               */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for or expression eo.                          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprOrDebug(KHE_SRS_EXPR_OR eo,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "OR");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eo, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eo, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ OR %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eo, srs, buff1),
      KheSrsDomTestShow(eo->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eo, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ OR %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eo, srs, buff1),
      KheSrsDomTestShow(eo->dom_test, buff2), HaArrayCount(eo->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprOrDebug(KHE_SRS_EXPR_OR eo,                               */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eo onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprOrDebug(KHE_SRS_EXPR_OR eo,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KheSrsExprIntDebug((KHE_SRS_EXPR) eo, "OR", eo->value, srs,
    verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_AND"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_AND KheSrsExprAndMake(KHE_SRS_DOM_TEST dom_test,            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_AND object with these attributes.                */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_AND KheSrsExprAndMake(KHE_SRS_DOM_TEST dom_test,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_AND res;
  KheSrsExprInit(res, KHE_SRS_EXPR_AND_TAG, dom_test, srs, free_expr_and);
  res->value = UNDEF_INT;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndDelete(KHE_SRS_EXPR_AND ea,                            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ea and its descendants.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprAndDelete(KHE_SRS_EXPR_AND ea,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(ea->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_and, ea);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprAndDomTest(KHE_SRS_EXPR_AND ea)               */
/*                                                                           */
/*  Return a suitable dom test for ea.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprAndDomTest(KHE_SRS_EXPR_AND ea)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) ea);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndBeginDay(KHE_SRS_EXPR_AND ea, int day_index,           */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform ea that day day_index is beginning.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprAndBeginDay(KHE_SRS_EXPR_AND ea, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ea->tmp_value = KheSrsExprIntBeginDay((KHE_SRS_EXPR) ea, day_index, 1,
    prev_ps, srs, "And");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndReceiveValueInt(KHE_SRS_EXPR_AND ea, int value,        */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform ea that one of its children has just finalized its value.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprAndReceiveValueInt(KHE_SRS_EXPR_AND ea, int value,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(ea->tmp_value != UNDEF_INT, "KheSrsExprOrReceiveValueInt internal error");
  if( value == 0 )
    ea->tmp_value = 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndEndDay(KHE_SRS_EXPR_AND ea, int day_index,             */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform ea that day day_index is ending.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprAndEndDay(KHE_SRS_EXPR_AND ea, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ea->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) ea, day_index,
    ea->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndDoDay(KHE_SRS_EXPR_AND ea, int day_index,              */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ea on day_index.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprAndDoDay(KHE_SRS_EXPR_AND ea, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, start_index, stop_index;  KHE_SRS_EXPR_INT child_e;

  /* initialize ea->value depending on whether day_index is its first day */
  if( day_index == ea->first_day_index )
    ea->value = 1;
  else
  {
    /* this is not ea's first day, so we need to retrieve a previous value */
    si = HaArray(ea->sig_indexes, day_index - 1 - ea->first_day_index);
    ea->value = HaArray(prev_ps->signature, si);
  }

  /* accumulate the values of the children of ea that finalized today */
  start_index = HaArray(ea->child_indexes, day_index - ea->first_day_index);
  stop_index =  HaArray(ea->child_indexes, day_index + 1 - ea->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_INT) HaArray(ea->children, i);
    if( child_e->value == 0 )
      ea->value = 0;
  }

  if( day_index < ea->last_day_index )
  {
    /* this not ea's last day; store value in next_ps */
    si = HaArray(ea->sig_indexes, day_index - ea->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprAndDoDay internal error");
    HaArrayAddLast(next_ps->signature, ea->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndDebug(KHE_SRS_EXPR_AND ea,                             */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for and expression ea.                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprAndDebug(KHE_SRS_EXPR_AND ea,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "AND");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ea, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) ea, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ AND %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ea, srs, buff1),
      KheSrsDomTestShow(ea->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) ea, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ AND %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ea, srs, buff1),
      KheSrsDomTestShow(ea->dom_test, buff2), HaArrayCount(ea->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAndDebug(KHE_SRS_EXPR_AND ea,                             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of ea onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprAndDebug(KHE_SRS_EXPR_AND ea,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KheSrsExprIntDebug((KHE_SRS_EXPR) ea, "AND", ea->value, srs,
    verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_INT_SUM"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SUM KheSrsExprIntSumMake(KHE_SRS_DOM_TEST dom_test,     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_INT_SUM object with these attributes.            */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_INT_SUM KheSrsExprIntSumMake(KHE_SRS_DOM_TEST dom_test,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_INT_SUM res;
  KheSrsExprInit(res, KHE_SRS_EXPR_INT_SUM_TAG, dom_test, srs,
    free_expr_int_sum);
  res->value = UNDEF_INT;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumDelete(KHE_SRS_EXPR_INT_SUM eis,                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eis and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumDelete(KHE_SRS_EXPR_INT_SUM eis,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eis->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_int_sum, eis);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprIntSumDomTest(KHE_SRS_EXPR_INT_SUM eis)       */
/*                                                                           */
/*  Return a suitable dom test for eis.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprIntSumDomTest(KHE_SRS_EXPR_INT_SUM eis)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) eis);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumBeginDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,   */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform eis that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumBeginDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eis->tmp_value = KheSrsExprIntBeginDay((KHE_SRS_EXPR) eis, day_index,
    ** eis->history_before ** 0, prev_ps, srs, "IntSum");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumReceiveValueInt(KHE_SRS_EXPR_INT_SUM eis,           */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform eis that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumReceiveValueInt(KHE_SRS_EXPR_INT_SUM eis,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eis->tmp_value += value;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumEndDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform eis that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumEndDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eis->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) eis, day_index,
    eis->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumDoDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,      */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node eis on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumDoDay(KHE_SRS_EXPR_INT_SUM eis, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, start_index, stop_index;  KHE_SRS_EXPR_INT child_e;

  /* initialize eis->value depending on whether day_index is its first day */
  if( day_index == eis->first_day_index )
    eis->value = 0;
  else
  {
    /* this is not eis's first day, so we need to retrieve a previous value */
    si = HaArray(eis->sig_indexes, day_index - 1 - eis->first_day_index);
    eis->value = HaArray(prev_ps->signature, si);
  }

  /* accumulate the values of the children of eis that finalized today */
  start_index = HaArray(eis->child_indexes, day_index - eis->first_day_index);
  stop_index =  HaArray(eis->child_indexes, day_index+1 - eis->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_INT) HaArray(eis->children, i);
    eis->value += child_e->value;
  }

  if( day_index < eis->last_day_index )
  {
    /* this not eis's last day; store value in next_ps */
    si = HaArray(eis->sig_indexes, day_index - eis->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprIntSumDoDay internal error");
    HaArrayAddLast(next_ps->signature, eis->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumDebug(KHE_SRS_EXPR_INT_SUM eis,                     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for busy time expression eis.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumDebug(KHE_SRS_EXPR_INT_SUM eis,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "INT_SUM");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eis, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eis, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ INT_SUM %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eis, srs, buff1),
      KheSrsDomTestShow(eis->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eis, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ INT_SUM %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eis, srs, buff1),
      KheSrsDomTestShow(eis->dom_test, buff2), HaArrayCount(eis->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumDebug(KHE_SRS_EXPR_INT_SUM eis,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eis onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumDebug(KHE_SRS_EXPR_INT_SUM eis,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KheSrsExprIntDebug((KHE_SRS_EXPR) eis, "INT_SUM", eis->value,
    srs, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_INT_SEQ"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SEQ KheSrsExprIntSeqMake(int history_before,            */
/*    int history_after, KHE_SRS_DOM_TEST dom_test,                          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_INT_SEQ object with these attributes.            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static KHE_SRS_EXPR_INT_SEQ KheSrsExprIntSeqMake(int history_before,
  int history_after, KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_INT_SEQ res;
  KheSrsExprInit(res, KHE_SRS_EXPR_INT_SEQ_TAG, dom_test, srs,
    free_expr_int_seq);
  res->history_before = history_before;
  res->history_after = history_after;
  res->tmp_value = UNDEF_INT;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqDelete(KHE_SRS_EXPR_INT_SEQ eis,                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eis and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static void KheSrsExprIntSeqDelete(KHE_SRS_EXPR_INT_SEQ eis,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eis->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_int_seq, eis);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprIntSeqDomTest(KHE_SRS_EXPR_INT_SEQ eis)       */
/*                                                                           */
/*  Return a suitable dom test for eis.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static KHE_SRS_DOM_TEST KheSrsExprIntSeqDomTest(KHE_SRS_EXPR_INT_SEQ eis)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) eis);  ** is this right? **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqBeginDay(KHE_SRS_EXPR_INT_SEQ eis, int day_index,   */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform eis that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static void KheSrsExprIntSeqBeginDay(KHE_SRS_EXPR_INT_SEQ eis, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eis->tmp_value = KheSrsExprIntBeginDay((KHE_SRS_EXPR) eis, day_index,
    eis->history_before, prev_ps, srs, "IntSeq");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqReceiveValueInt(KHE_SRS_EXPR_INT_SEQ eis,           */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform eis that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static void KheSrsExprIntSeqReceiveValueInt(KHE_SRS_EXPR_INT_SEQ eis,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(value == 0 || value == 1,
    "KheSrsExprIntSeqReceiveValueInt internal error");
  if( value == 1 )
  {
    ** a sequence is beginning or continuing **
    eis->tmp_value++;
  }
  else if( eis->tmp_value > 0 )
  {
    ** a non-empty sequence has just ended **
    KheSrsExprReceiveValueInt(eis->parent, eis->tmp_value, srs);
    eis->tmp_value = 0;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqEndDay(KHE_SRS_EXPR_INT_SEQ eis, int day_index,     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform eis that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static void KheSrsExprIntSeqEndDay(KHE_SRS_EXPR_INT_SEQ eis, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  eis->tmp_value = KheSrsExprIntEndDay((KHE_SRS_EXPR) eis, day_index,
    ** eis->tmp_value > 0, ** eis->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqDebug(KHE_SRS_EXPR_INT_SEQ eis,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eis onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; only KHE_SRS_EXPR_INT_SEQ_COMB_TAG is used
static void KheSrsExprIntSeqDebug(KHE_SRS_EXPR_INT_SEQ eis,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KheSrsExprIntDebug((KHE_SRS_EXPR) eis, "INT_SEQ", eis->tmp_value,
    srs, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_FLOAT_SUM"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FLOAT_SUM KheSrsExprFloatSumMake(                           */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_FLOAT_SUM object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FLOAT_SUM KheSrsExprFloatSumMake(
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FLOAT_SUM res;
  KheSrsExprInit(res, KHE_SRS_EXPR_FLOAT_SUM_TAG, dom_test, srs,
    free_expr_float_sum);
  res->value = UNDEF_FLOAT;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumDelete(KHE_SRS_EXPR_FLOAT_SUM efs,                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete efs and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatSumDelete(KHE_SRS_EXPR_FLOAT_SUM efs,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(efs->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_float_sum, efs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprFloatSumDomTest(KHE_SRS_EXPR_FLOAT_SUM efs)   */
/*                                                                           */
/*  Return a suitable dom test for efs.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprFloatSumDomTest(KHE_SRS_EXPR_FLOAT_SUM efs)
{
  return KheExprMonotoneDomTest((KHE_SRS_EXPR) efs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumBeginDay(KHE_SRS_EXPR_FLOAT_SUM efs,              */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform efs that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatSumBeginDay(KHE_SRS_EXPR_FLOAT_SUM efs,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  efs->tmp_value = KheSrsExprFloatBeginDay((KHE_SRS_EXPR) efs, day_index,
    0.0, prev_ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumReceiveValueFloat(KHE_SRS_EXPR_FLOAT_SUM efs,     */
/*    float value, KHE_SINGLE_RESOURCE_SOLVER srs)                           */
/*                                                                           */
/*  Inform efs that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatSumReceiveValueFloat(KHE_SRS_EXPR_FLOAT_SUM efs,
  float value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(efs->tmp_value >= 0.0,
    "KheSrsExprFloatSumReceiveValueFloat internal error");
  efs->tmp_value += value;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumEndDay(KHE_SRS_EXPR_FLOAT_SUM efs,                */
/*    int day_index, KHE_SRS_PARTIAL_SOLN ps,                                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform efs that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatSumEndDay(KHE_SRS_EXPR_FLOAT_SUM efs,
  int day_index, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  efs->tmp_value = KheSrsExprFloatEndDay((KHE_SRS_EXPR) efs, day_index,
    efs->tmp_value, ps, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumDoDay(KHE_SRS_EXPR_FLOAT_SUM efs, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node efs on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatSumDoDay(KHE_SRS_EXPR_FLOAT_SUM efs, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, start_index, stop_index;  KHE_SRS_EXPR_FLOAT child_e;

  /* initialize efs->value depending on whether day_index is its first day */
  if( day_index == efs->first_day_index )
    efs->value = 0.0;
  else
  {
    /* this is not efs's first day, so we need to retrieve a previous value */
    si = HaArray(efs->sig_indexes, day_index - 1 - efs->first_day_index);
    efs->value = (float) HaArray(prev_ps->signature, si) / 100.0;
  }

  /* accumulate the values of the children of efs that finalized today */
  start_index = HaArray(efs->child_indexes, day_index - efs->first_day_index);
  stop_index =  HaArray(efs->child_indexes, day_index+1 - efs->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_FLOAT) HaArray(efs->children, i);
    efs->value += child_e->value;
  }

  if( day_index < efs->last_day_index )
  {
    /* this not efs's last day; store value in next_ps */
    si = HaArray(efs->sig_indexes, day_index - efs->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprFloatSumDoDay internal error");
    HaArrayAddLast(next_ps->signature, (int) (efs->value * 100.0));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumDebug(KHE_SRS_EXPR_FLOAT_SUM efs,                 */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the header part of efs in ps onto fp.                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatSumDebug(KHE_SRS_EXPR_FLOAT_SUM efs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "FLOAT_SUM");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) efs, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%.2f$", si, (float) HaArray(ps->signature, si) /100.0);
    KheSrsExprDebugChildren((KHE_SRS_EXPR) efs, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ FLOAT_SUM %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) efs, srs, buff1),
      KheSrsDomTestShow(efs->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) efs, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ FLOAT_SUM %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) efs, srs, buff1),
      KheSrsDomTestShow(efs->dom_test, buff2), HaArrayCount(efs->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatSumDebug(KHE_SRS_EXPR_FLOAT_SUM efs,                 */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of efs onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatSumDebug(KHE_SRS_EXPR_FLOAT_SUM efs,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KheSrsExprFloatDebug((KHE_SRS_EXPR) efs, "FLOAT_SUM", efs->value,
    srs, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_INT_DEV"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_DEV KheSrsExprIntDevMake(int min_limit,                 */
/*    int max_limit, bool allow_zero, KHE_SINGLE_RESOURCE_SOLVER srs)        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_INT_DEV object with these attributes.            */
/*                                                                           */
/*  There is no dom_test because its type is always KHE_SRS_DOM_UNUSED.      */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_INT_DEV KheSrsExprIntDevMake(int min_limit,
  int max_limit, bool allow_zero, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_INT_DEV res;  KHE_SRS_DOM_TEST dom_test;
  dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  KheSrsExprInit(res, KHE_SRS_EXPR_INT_DEV_TAG, dom_test, srs,
    free_expr_int_dev);
  res->value = UNDEF_INT;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->allow_zero = allow_zero;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevDelete(KHE_SRS_EXPR_INT_DEV eid,                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eid and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntDevDelete(KHE_SRS_EXPR_INT_DEV eid,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eid->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_int_dev, eid);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprIntDevDomTest(KHE_SRS_EXPR_INT_DEV eid)       */
/*                                                                           */
/*  Return a suitable dom test for eid.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprIntDevDomTest(KHE_SRS_EXPR_INT_DEV eid)
{
  if( eid->parent == NULL || eid->parent->dom_test.type == KHE_SRS_DOM_NONE )
  {
    ** no inherited dom test, so base the dom test on the limits in eid **
    return KheDomTestForLimits(eid->min_limit, eid->max_limit, eid->allow_zero);
  }
  else
    return KheSrsDomTestMake(KHE_SRS_DOM_MIN_AND_MAX, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevBeginDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,   */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform eid that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntDevBeginDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprIntDevBeginDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevReceiveValueInt(KHE_SRS_EXPR_INT_DEV eid,           */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform eid that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntDevReceiveValueInt(KHE_SRS_EXPR_INT_DEV eid,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int dev;
  dev = KheSrsSumDev(eid->min_limit, eid->max_limit, eid->allow_zero, 0, value);
  KheSrsExprReceiveValueInt(eid->parent, dev, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevEndDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform eid that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntDevEndDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprIntDevEndDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevDoDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,      */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node eid on day_index.                                    */
/*                                                                           */
/*  Implementation note.  This code assumes (correctly) that there is a      */
/*  single child, which becomes final on eid'd first day.                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntDevDoDay(KHE_SRS_EXPR_INT_DEV eid, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;  KHE_SRS_EXPR_INT e;

  /* initialize eid->value depending on whether day_index is its first day */
  if( day_index == eid->first_day_index )
  {
    e = (KHE_SRS_EXPR_INT) HaArrayFirst(eid->children);
    eid->value = KheSrsSumDev(eid->min_limit, eid->max_limit,
      eid->allow_zero, 0, e->value);
  }
  else
  {
    /* this is not eid's first day, so we need to retrieve a previous value */
    si = HaArray(eid->sig_indexes, day_index - 1 - eid->first_day_index);
    eid->value = HaArray(prev_ps->signature, si);
  }

  if( day_index < eid->last_day_index )
  {
    /* this not eid's last day; store value in next_ps */
    si = HaArray(eid->sig_indexes, day_index - eid->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprIntDevDoDay internal error");
    HaArrayAddLast(next_ps->signature, eid->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevDebug(KHE_SRS_EXPR_INT_DEV eid,                     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the header part of eid in ps onto fp.                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntDevDebug(KHE_SRS_EXPR_INT_DEV eid,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value (actually no ps value here) */
    fprintf(fp, "INT_DEV");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eid, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eid, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ INT_DEV %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eid, srs, buff1),
      KheSrsDomTestShow(eid->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eid, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ INT_DEV %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eid, srs, buff1),
      KheSrsDomTestShow(eid->dom_test, buff2), HaArrayCount(eid->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntDevDebug(KHE_SRS_EXPR_INT_DEV eid,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eid onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntDevDebug(KHE_SRS_EXPR_INT_DEV eid,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ INT_DEV(min %d, max %d, allow_zero %s)\n", indent, "",
      eid->min_limit, eid->max_limit, bool_show(eid->allow_zero));
    HaArrayForEach(eid->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "INT_DEV(min %d, max %d, allow_zero %s)\n",
      eid->min_limit, eid->max_limit, bool_show(eid->allow_zero));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_FLOAT_DEV"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_FLOAT_DEV KheSrsExprFloatDevMake(int min_limit,             */
/*    int max_limit, bool allow_zero, KHE_SINGLE_RESOURCE_SOLVER srs)        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_FLOAT_DEV object with these attributes.          */
/*                                                                           */
/*  There is no dom_test because its type is always KHE_SRS_DOM_UNUSED.      */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_FLOAT_DEV KheSrsExprFloatDevMake(int min_limit,
  int max_limit, bool allow_zero, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_FLOAT_DEV res;  KHE_SRS_DOM_TEST dom_test;
  dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  KheSrsExprInit(res, KHE_SRS_EXPR_FLOAT_DEV_TAG, dom_test, srs,
    free_expr_float_dev);
  res->value = UNDEF_INT;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->allow_zero = allow_zero;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevDelete(KHE_SRS_EXPR_FLOAT_DEV efd,                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete efd and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatDevDelete(KHE_SRS_EXPR_FLOAT_DEV efd,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(efd->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_float_dev, efd);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprFloatDevDomTest(KHE_SRS_EXPR_FLOAT_DEV efd)   */
/*                                                                           */
/*  Return a suitable dom test for efd.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprFloatDevDomTest(KHE_SRS_EXPR_FLOAT_DEV efd)
{
  if( efd->parent == NULL )
    return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);
  else
    return efd->parent->dom_test;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevBeginDay(KHE_SRS_EXPR_FLOAT_DEV efd,              */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform efd that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatDevBeginDay(KHE_SRS_EXPR_FLOAT_DEV efd,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprFloatDevBeginDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorDev(KHE_SRS_EXPR_FLOAT_DEV efd,               */
/*    float workload)                                                        */
/*                                                                           */
/*  Return the deviation of workload wrt efd.                                */
/*                                                                           */
/*****************************************************************************/

static int KheLimitWorkloadMonitorDev(KHE_SRS_EXPR_FLOAT_DEV efd,
  float workload)
{
  if( efd->allow_zero && workload < 0.001 )
    return 0;
  else if( workload < efd->min_limit - 0.001 )
    return (int) ceil(efd->min_limit - 0.001 - workload);
  else if( workload > efd->max_limit + 0.001 )
    return (int) ceil(workload - efd->max_limit - 0.001);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevReceiveValueFloat(KHE_SRS_EXPR_FLOAT_DEV efd,     */
/*    float value, KHE_SINGLE_RESOURCE_SOLVER srs)                           */
/*                                                                           */
/*  Inform efd that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatDevReceiveValueFloat(KHE_SRS_EXPR_FLOAT_DEV efd,
  float value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int dev;
  dev = KheLimitWorkloadMonitorDev(efd, value);
  KheSrsExprReceiveValueInt(efd->parent, dev, srs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevEndDay(KHE_SRS_EXPR_FLOAT_DEV efd,                */
/*    int day_index, KHE_SRS_PARTIAL_SOLN ps,                                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform efd that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatDevEndDay(KHE_SRS_EXPR_FLOAT_DEV efd,
  int day_index, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprFloatDevEndDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevDoDay(KHE_SRS_EXPR_FLOAT_DEV efd, int day_index,  */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node efd on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatDevDoDay(KHE_SRS_EXPR_FLOAT_DEV efd, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si;  KHE_SRS_EXPR_FLOAT e;

  /* initialize efd->value depending on whether day_index is its first day */
  if( day_index == efd->first_day_index )
  {
    e = (KHE_SRS_EXPR_FLOAT) HaArrayFirst(efd->children);
    efd->value = KheLimitWorkloadMonitorDev(efd, e->value);
  }
  else
  {
    /* this is not efd's first day, so we need to retrieve a previous value */
    si = HaArray(efd->sig_indexes, day_index - 1 - efd->first_day_index);
    efd->value = HaArray(prev_ps->signature, si);
  }

  if( day_index < efd->last_day_index )
  {
    /* this not efd's last day; store value in next_ps */
    si = HaArray(efd->sig_indexes, day_index - efd->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprFloatDevDoDay internal error");
    HaArrayAddLast(next_ps->signature, efd->value);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevDebug(KHE_SRS_EXPR_FLOAT_DEV efd,                 */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for float dev expression efd.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprFloatDevDebug(KHE_SRS_EXPR_FLOAT_DEV efd,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value (actually no ps value here) */
    fprintf(fp, "FLOAT_DEV");
    if( KheSrsExprInSignature((KHE_SRS_EXPR) efd, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) efd, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ FLOAT_DEV %s %s\n", indent, "",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) efd, srs, buff1),
      KheSrsDomTestShow(efd->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) efd, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ FLOAT_DEV %s %s (%d children) ]",
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) efd, srs, buff1),
      KheSrsDomTestShow(efd->dom_test, buff2), HaArrayCount(efd->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprFloatDevDebug(KHE_SRS_EXPR_FLOAT_DEV efd,                 */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of efd onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprFloatDevDebug(KHE_SRS_EXPR_FLOAT_DEV efd,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ FLOAT_DEV(min %d, max %d, allow_zero %s)\n", indent, "",
      efd->min_limit, efd->max_limit, bool_show(efd->allow_zero));
    HaArrayForEach(efd->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "FLOAT_DEV(min %d, max %d, allow_zero %s)\n",
      efd->min_limit, efd->max_limit, bool_show(efd->allow_zero));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_COST"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_COST KheSrsExprCostMake(KHE_MONITOR m,                      */
/*    KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,                   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_COST object with these attributes.               */
/*                                                                           */
/*  This function has no dom_test parameter, because there is never any      */
/*  dominance testing at this node.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_COST KheSrsExprCostMake(KHE_MONITOR m,
  KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_COST res;  KHE_SRS_DOM_TEST dom_test;
  dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  KheSrsExprInit(res, KHE_SRS_EXPR_COST_TAG, dom_test, srs, free_expr_cost);
  res->monitor = m;
  res->cost_fn = cost_fn;
  res->combined_weight = combined_weight;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostDelete(KHE_SRS_EXPR_COST ec,                          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ec and its descendants.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprCostDelete(KHE_SRS_EXPR_COST ec,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(ec->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_cost, ec);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprCostDomTest(KHE_SRS_EXPR_COST ec)             */
/*                                                                           */
/*  Return a suitable dom test for ec.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprCostDomTest(KHE_SRS_EXPR_COST ec)
{
  return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostBeginDay(KHE_SRS_EXPR_COST ec, int day_index,         */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform ec that day day_index is beginning.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostBeginDay(KHE_SRS_EXPR_COST ec, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprCostBeginDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostReceiveValueInt(KHE_SRS_EXPR_COST ec,                 */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform ec that one of its children has just finalized its value.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostReceiveValueInt(KHE_SRS_EXPR_COST ec,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_COST cost;
  cost = KheSrsCost(ec->cost_fn, ec->combined_weight, value);
  if( cost > 0 )
  {
    if( srs->rerun )
      fprintf(stderr, "  %s reporting cost %.5f\n",
	KheMonitorId(ec->monitor), KheCostShow(cost));
    KheSrsExprReceiveValueCost(ec->parent, cost, srs);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostEndDay(KHE_SRS_EXPR_COST ec, int day_index,           */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform ec that day day_index is ending.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostEndDay(KHE_SRS_EXPR_COST ec, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** not in active_exprs, should never be called **
  HnAbort("KheSrsExprCostEndDay internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostDoDay(KHE_SRS_EXPR_COST ec, int day_index,            */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ec on day_index.                                     */
/*                                                                           */
/*  Implementation note.  Cost nodes always report their values directly     */
/*  to the new partial solution as soon as they are available.  They do      */
/*  not store values for retrieval by a parent; they have no parents.        */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprCostDoDay(KHE_SRS_EXPR_COST ec, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_COST cost;  KHE_SRS_EXPR_INT e;
  e = (KHE_SRS_EXPR_INT) HaArrayFirst(ec->children);
  cost = KheSrsCost(ec->cost_fn, ec->combined_weight, e->value);
  if( cost > 0 )
  {
    if( srs->rerun )
      fprintf(stderr, "  %s reporting cost %.5f\n",
	KheMonitorId(ec->monitor), KheCostShow(cost));
    next_ps->cost += cost;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostDebug(KHE_SRS_EXPR_COST ec,                           */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for cost expression ec.                        */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprCostDebug(KHE_SRS_EXPR_COST ec,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "COST %s", KheMonitorId(ec->monitor));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) ec, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) ec, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ COST %s %s %s\n", indent, "", KheMonitorId(ec->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ec, srs, buff1),
      KheSrsDomTestShow(ec->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) ec, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ COST %s %s %s (%d children) ]", KheMonitorId(ec->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) ec, srs, buff1),
      KheSrsDomTestShow(ec->dom_test, buff2), HaArrayCount(ec->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostDebug(KHE_SRS_EXPR_COST ec,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of ec onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostDebug(KHE_SRS_EXPR_COST ec,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ COST(%s, cost_fn %s, combined_weight %.5f)\n", indent, "",
      ec->monitor != NULL ? KheMonitorId(ec->monitor) : "-",
      KheCostFunctionShow(ec->cost_fn), KheCostShow(ec->combined_weight));
    HaArrayForEach(ec->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "COST(%s, cost_fn %s, combined_weight %.5f)\n",
      ec->monitor != NULL ? KheMonitorId(ec->monitor) : "-",
      KheCostFunctionShow(ec->cost_fn), KheCostShow(ec->combined_weight));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_COST_SUM"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_COST_SUM KheSrsExprCostSumMake(                             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_COST_SUM object with these attributes.           */
/*                                                                           */
/*  This function has no dom_test parameter, because there is never any      */
/*  dominance testing at this node.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_SRS_EXPR_COST_SUM KheSrsExprCostSumMake(
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_COST_SUM res;  KHE_SRS_DOM_TEST dom_test;
  dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  KheSrsExprInit(res, KHE_SRS_EXPR_COST_SUM_TAG, dom_test, srs,
    free_expr_cost_sum);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumDelete(KHE_SRS_EXPR_COST_SUM ecs,                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ecs and its descendants.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumDelete(KHE_SRS_EXPR_COST_SUM ecs,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(ecs->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_cost_sum, ecs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprCostSumDomTest(KHE_SRS_EXPR_COST_SUM ecs)     */
/*                                                                           */
/*  Return a suitable dom test for ecs.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprCostSumDomTest(KHE_SRS_EXPR_COST_SUM ecs)
{
  ** stil l to do **
  HnAbort("KheSrsExprCostSumDomTest st ill to do");
  return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumBeginDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index, */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform ecs that day day_index is beginning.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumBeginDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  if( day_index == ecs->first_day_index )
  {
    ** this is the start of the first day covered by this expression **
    ecs->tmp_value = KheCost(0, 0);
  }
  else
  {
    ** this is not the first day; get previous day's value **
    ecs->tmp_value = prev_ps->cost;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumReceiveValueCost(KHE_SRS_EXPR_COST_SUM ecs,        */
/*    KHE_COST value, KHE_SINGLE_RESOURCE_SOLVER srs)                        */
/*                                                                           */
/*  Inform ecs that one of its children has just finalized its value.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumReceiveValueCost(KHE_SRS_EXPR_COST_SUM ecs,
  KHE_COST value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ecs->tmp_value += value;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumEndDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index,   */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform ecs that day day_index is ending.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumEndDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(ecs->first_day_index <= day_index && day_index <=ecs->last_day_index,
    "KheSrsExprCostSumEndDay internal error");
  if( day_index == ecs->last_day_index )
  {
    ** this is the end of the last day covered by this expression **
    ** but there is nowhere else to report tmp_value to **
    ps->cost = ecs->tmp_value;
  }
  else
  {
    ** this is not the last day; store value in signature **
    ps->cost = ecs->tmp_value;
  }
  ecs->tmp_value = KheCost(-1, -1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumDoDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index,    */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active node ecs on day_index.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumDoDay(KHE_SRS_EXPR_COST_SUM ecs, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** nothing to do here; children post costs directly to next_ps **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumPartialSolnDebug(KHE_SRS_EXPR_COST_SUM ecs,        */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, FILE *fp)                                               */
/*                                                                           */
/*  Debug print of the header part of ecs in ps onto fp.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumPartialSolnDebug(KHE_SRS_EXPR_COST_SUM ecs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, FILE *fp)
{
  HnAbort("KheSrsExprCostSumPartialSolnDebug internal error (unexpected call)");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumDebug(KHE_SRS_EXPR_COST_SUM ecs,                   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of ecs onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumDebug(KHE_SRS_EXPR_COST_SUM ecs,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ COST_SUM", indent, "");
    fprintf(fp, "\n");
    HaArrayForEach(ecs->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "COST_SUM(%d children)", HaArrayCount(ecs->children));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_INT_SUM_COMB"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_COMB_TYPE KheSrsCombType(KHE_COST_FUNCTION cost_fn,              */
/*    int max_limit)                                                         */
/*                                                                           */
/*  Return the comb type, as determined by the cost function and max limit.  */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_COMB_TYPE KheSrsCombType(KHE_COST_FUNCTION cost_fn,
  int max_limit)
{
  if( max_limit == INT_MAX )
    return KHE_SRS_COMB_NO_MAX;
  else if( cost_fn == KHE_LINEAR_COST_FUNCTION )
    return KHE_SRS_COMB_LINEAR;
  else if( cost_fn == KHE_STEP_COST_FUNCTION )
    return KHE_SRS_COMB_STEP;
  else
    return KHE_SRS_COMB_ORDINARY;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SUM_COMB KheSrsExprIntSumCombMake(KHE_MONITOR m,        */
/*    KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,                   */
/*    int min_limit, int max_limit, bool allow_zero, int history_before,     */
/*    int history_after, KHE_SRS_DOM_TEST dom_test,                          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_INT_SUM_COMB object with these attributes.       */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_INT_SUM_COMB KheSrsExprIntSumCombMake(KHE_MONITOR m,
  KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,
  int min_limit, int max_limit, bool allow_zero, int history_before,
  int history_after, KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_INT_SUM_COMB res;
  KheSrsExprInit(res, KHE_SRS_EXPR_INT_SUM_COMB_TAG, dom_test, srs,
    free_expr_int_sum_comb);
  res->value = UNDEF_INT;
  res->monitor = m;
  res->cost_fn = cost_fn;
  res->combined_weight = combined_weight;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->allow_zero = allow_zero;
  res->history_before = history_before;
  res->history_after = history_after;
  res->comb_type = KheSrsCombType(cost_fn, max_limit);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombDelete(KHE_SRS_EXPR_INT_SUM_COMB eisc,          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eisc and its descendants.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumCombDelete(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eisc->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_int_sum_comb, eisc);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprIntSumCombDomTest(                            */
/*    KHE_SRS_EXPR_INT_SUM_COMB eisc)                                        */
/*                                                                           */
/*  Return a suitable dom test for eisc.                                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprIntSumCombDomTest(
  KHE_SRS_EXPR_INT_SUM_COMB eisc)
{
  ** s till to do **
  HnAbort("KheSrsExprIntSumCombDomTest s till to do");
  return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombBeginDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,        */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform eisc that day day_index is beginning.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumCombBeginDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  ** KHE_SRS_DAY_INFO di; **  int sig_index;
  HnAssert(eisc->first_day_index <= day_index &&
    day_index <= eisc->last_day_index,
    "KheSrsExprIntSumBeginDay internal error");
  if( day_index == eisc->first_day_index )
  {
    ** this is the start of the first day covered by this expression **
    eisc->tmp_value = eisc->history_before;
  }
  else
  {
    ** this is not the first day; get previous day's value **
    sig_index = HaArray(eisc->sig_indexes, day_index -1 -eisc->first_day_index);
    eisc->tmp_value = HaArray(prev_ps->signature, sig_index);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombReceiveValueInt(KHE_SRS_EXPR_INT_SUM_COMB eisc, */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform eisc that one of its children has just finalized its value.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumCombReceiveValueInt(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_COST cost;

  ** add value to tmp_value **
  eisc->tmp_value += value;

  ** adjust in various special cases **
  switch( eisc->comb_type )
  {
    case KHE_SRS_COMB_ORDINARY:

      ** no adjustment here **
      break;

    case KHE_SRS_COMB_NO_MAX:

      ** no maximum limit; store minimum limit at most **
      if( eisc->tmp_value > eisc->min_limit )
        eisc->tmp_value = eisc->min_limit;
      break;

    case KHE_SRS_COMB_LINEAR:

      ** linear cost; if over maximum, report excess immediately **
      if( eisc->tmp_value > eisc->max_limit )
      {
	cost = (eisc->tmp_value - eisc->max_limit) * eisc->combined_weight;
	if( cost > 0 )
	{
	  if( srs->rerun )
	    fprintf(stderr, "  %s reporting cost %.5f\n",
	      KheMonitorId(eisc->monitor), KheCostShow(cost));
	  KheSrsExprReceiveValueCost(eisc->parent, cost, srs);
	}
	eisc->tmp_value = eisc->max_limit;
      }
      break;

    case KHE_SRS_COMB_STEP:

      ** step cost; store maximum plus one at most **
      if( eisc->tmp_value > eisc->max_limit + 1 )
        eisc->tmp_value = eisc->max_limit + 1;
      break;

    default:

      HnAbort("KheSrsExprIntSumCombReceiveValueInt internal error");
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombEndDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,          */
/*    int day_index, KHE_SRS_PARTIAL_SOLN ps,                                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform eisc that day day_index is ending.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumCombEndDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int dev, sig_index;  KHE_COST cost;
  HnAssert(eisc->first_day_index <= day_index &&
    day_index <= eisc->last_day_index,
    "KheSrsExprIntSumCombEndDay internal error");
  if( day_index == eisc->last_day_index )
  {
    ** this is the end of the last day covered by this expression **
    dev = KheSrsSumDev(eisc->min_limit, eisc->max_limit, eisc->allow_zero,
      eisc->history_after, eisc->tmp_value);
    cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
    if( cost > 0 )
    {
      if( srs->rerun )
	fprintf(stderr, "  %s reporting cost %.5f\n",
	  KheMonitorId(eisc->monitor), KheCostShow(cost));
      KheSrsExprReceiveValueCost(eisc->parent, cost, srs);
    }
  }
  else
  {
    ** this is not the last day; store value in signature **
    sig_index = HaArray(eisc->sig_indexes, day_index - eisc->first_day_index);
    HnAssert(HaArrayCount(ps->signature) == sig_index,
      "KheSrsExprIntSumEndDay internal error");
    HaArrayAddLast(ps->signature, eisc->tmp_value);
  }
  eisc->tmp_value = UNDEF_INT;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombDoDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,           */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task,            */
/*    KHE_TIME time, KHE_SRS_PARTIAL_SOLN next_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Process active node eisc on day_index.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumCombDoDay(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, start_index, stop_index, dev;
  KHE_SRS_EXPR_INT child_e;  KHE_COST cost;

  /* initialize eisc->value depending on whether day_index is its first day */
  if( day_index == eisc->first_day_index )
    eisc->value = eisc->history_before;
  else
  {
    /* this is not eisc's first day, so we need to retrieve a previous value */
    si = HaArray(eisc->sig_indexes, day_index - 1 - eisc->first_day_index);
    eisc->value = HaArray(prev_ps->signature, si);
  }

  /* accumulate the values of the children of eisc that finalized today */
  start_index = HaArray(eisc->child_indexes, day_index - eisc->first_day_index);
  stop_index =  HaArray(eisc->child_indexes, day_index+1-eisc->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_INT) HaArray(eisc->children, i);
    eisc->value += child_e->value;
  }

  /* adjust in various special cases */
  switch( eisc->comb_type )
  {
    case KHE_SRS_COMB_ORDINARY:

      /* no adjustment here */
      break;

    case KHE_SRS_COMB_NO_MAX:

      /* no maximum limit; store minimum limit at most */
      if( eisc->value > eisc->min_limit )
	eisc->value = eisc->min_limit;
      break;

    case KHE_SRS_COMB_LINEAR:

      /* linear cost; if over maximum, report excess immediately */
      if( eisc->value > eisc->max_limit )
      {
	cost = (eisc->value - eisc->max_limit) * eisc->combined_weight;
	if( cost > 0 )
	{
	  if( srs->rerun )
	    fprintf(stderr, "  %s reporting linear cost %.5f\n",
	      KheMonitorId(eisc->monitor), KheCostShow(cost));
	  next_ps->cost += cost;
	}
	eisc->value = eisc->max_limit;
      }
      break;

    case KHE_SRS_COMB_STEP:

      /* step cost; store maximum plus one at most */
      if( eisc->value > eisc->max_limit + 1 )
	eisc->value = eisc->max_limit + 1;
      break;

    default:

      HnAbort("KheSrsExprIntSumCombDoDay internal error");
      break;
  }

  if( day_index < eisc->last_day_index )
  {
    /* this not eisc's last day; store value in next_ps */
    si = HaArray(eisc->sig_indexes, day_index - eisc->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprIntSumCombDoDay internal error");
    HaArrayAddLast(next_ps->signature, eisc->value);
  }
  else
  {
    /* this eisc's last day; report any cost immediately to next_ps */
    dev = KheSrsSumDev(eisc->min_limit, eisc->max_limit, eisc->allow_zero,
      eisc->history_after, eisc->value);
    cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
    if( cost > 0 )
    {
      if( srs->rerun )
	fprintf(stderr, "  %s reporting cost %.5f\n",
	  KheMonitorId(eisc->monitor), KheCostShow(cost));
      next_ps->cost += cost;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombDebug(KHE_SRS_EXPR_INT_SUM_COMB eisc,           */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Implement KheSrsExprDebug for int sum comb expression eisc.              */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSumCombDebug(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "INT_SUM_COMB %s", KheMonitorId(eisc->monitor));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eisc, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eisc, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ INT_SUM_COMB %s %s %s\n", indent, "",
      KheMonitorId(eisc->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eisc, srs, buff1),
      KheSrsDomTestShow(eisc->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eisc, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ INT_SUM_COMB %s %s %s (%d children) ]",
      KheMonitorId(eisc->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eisc, srs, buff1),
      KheSrsDomTestShow(eisc->dom_test, buff2), HaArrayCount(eisc->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSumCombDebug(KHE_SRS_EXPR_INT_SUM_COMB eisc,           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eisc onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSumCombDebug(KHE_SRS_EXPR_INT_SUM_COMB eisc,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ INT_SUM_COMB(%s, %s %.5f, lim %d-%d, az %s, h %d,%d)\n",
      indent, "", eisc->monitor != NULL ? KheMonitorId(eisc->monitor) : "-",
      KheCostFunctionShow(eisc->cost_fn), KheCostShow(eisc->combined_weight),
      eisc->min_limit, eisc->max_limit, bool_show(eisc->allow_zero),
      eisc->history_before, eisc->history_after);
    HaArrayForEach(eisc->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "INT_SUM_COMB(%s, h %d,%d, %d children)",
      eisc->monitor != NULL ? KheMonitorId(eisc->monitor) : "-",
      eisc->history_before, eisc->history_after, HaArrayCount(eisc->children));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR_INT_SEQ_COMB"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR_INT_SEQ_COMB KheSrsExprIntSeqCombMake(KHE_MONITOR m,        */
/*    KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight,                   */
/*    KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight, int min_limit,    */
/*    int max_limit, int history_before, int history_after,                  */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make a new KHE_SRS_EXPR_INT_SEQ_COMB object with these attributes.       */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR_INT_SEQ_COMB KheSrsExprIntSeqCombMake(KHE_MONITOR m,
  KHE_COST_FUNCTION cost_fn, KHE_COST combined_weight, int min_limit,
  int max_limit, int history_before, int history_after,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_INT_SEQ_COMB res;
  KheSrsExprInit(res, KHE_SRS_EXPR_INT_SEQ_COMB_TAG, dom_test, srs,
    free_expr_int_seq_comb);
  res->value = UNDEF_INT;
  res->monitor = m;
  res->cost_fn = cost_fn;
  res->combined_weight = combined_weight;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->history_before = history_before;
  res->history_after = history_after;
  res->comb_type = KheSrsCombType(cost_fn, max_limit);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombDelete(KHE_SRS_EXPR_INT_SEQ_COMB eisc,          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete eisc and its descendants.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSeqCombDelete(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR e;  int i;
  HaArrayForEach(eisc->children, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayAddLast(srs->free_expr_int_seq_comb, eisc);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_EXPR KheSrsExprOptionalCopy(KHE_SRS_EXPR e,                      */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  If e is a BUSY_TIME, FREE_TIME, WORK_TIME, BUSY_DAY, FREE_DAY, or        */
/*  WORK_DAY expression, return a fresh copy of it.  Otherwise return e.     */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_EXPR KheSrsExprOptionalCopy(KHE_SRS_EXPR e,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      return (KHE_SRS_EXPR) KheSrsExprBusyTimeCopy((KHE_SRS_EXPR_BUSY_TIME) e,
	srs);

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      return (KHE_SRS_EXPR) KheSrsExprFreeTimeCopy((KHE_SRS_EXPR_FREE_TIME) e,
	srs);

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      return (KHE_SRS_EXPR) KheSrsExprWorkTimeCopy((KHE_SRS_EXPR_WORK_TIME) e,
	srs);

    case KHE_SRS_EXPR_BUSY_DAY_TAG:

      return (KHE_SRS_EXPR) KheSrsExprBusyDayCopy((KHE_SRS_EXPR_BUSY_DAY) e,
	srs);

    case KHE_SRS_EXPR_FREE_DAY_TAG:

      return (KHE_SRS_EXPR) KheSrsExprFreeDayCopy((KHE_SRS_EXPR_FREE_DAY) e,
	srs);

    case KHE_SRS_EXPR_WORK_DAY_TAG:

      return (KHE_SRS_EXPR) KheSrsExprWorkDayCopy((KHE_SRS_EXPR_WORK_DAY) e,
	srs);

    default:

      /* other node types are not shared, just return e */
      return e;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprIntSeqCombDomTest(                            */
/*    KHE_SRS_EXPR_INT_SEQ_COMB eisc)                                        */
/*                                                                           */
/*  Return a suitable dom test for eisc.                                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static KHE_SRS_DOM_TEST KheSrsExprIntSeqCombDomTest(
  KHE_SRS_EXPR_INT_SEQ_COMB eisc)
{
  ** s till to do **
  HnAbort("KheSrsExprIntSeqCombDomTest s till to do");
  return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombBeginDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,        */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform eisc that day day_index is beginning.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSeqCombBeginDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int sig_index;
  HnAssert(eisc->first_day_index <= day_index &&
    day_index <= eisc->last_day_index,
    "KheSrsExprIntSeqBeginDay internal error");
  if( day_index == eisc->first_day_index )
  {
    ** this is the start of the first day covered by this expression **
    eisc->tmp_value = eisc->history_before;
  }
  else
  {
    ** this is not the first day; get previous day's value **
    sig_index = HaArray(eisc->sig_indexes, day_index -1 -eisc->first_day_index);
    eisc->tmp_value = HaArray(prev_ps->signature, sig_index);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombReceiveValueInt(KHE_SRS_EXPR_INT_SEQ_COMB eisc, */
/*    int value, KHE_SINGLE_RESOURCE_SOLVER srs)                             */
/*                                                                           */
/*  Inform eisc that one of its children has just finalized its value.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSeqCombReceiveValueInt(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  int value, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_COST cost;  int dev;

  HnAssert(value == 0 || value == 1,
    "KheSrsExprIntSeqReceiveValueInt internal error");
  if( value == 1 )
  {
    ** a sequence is beginning or continuing **
    eisc->tmp_value++;

    ** adjust in various special cases **
    switch( eisc->comb_type )
    {
      case KHE_SRS_COMB_ORDINARY:

	** no adjustment here **
	break;

      case KHE_SRS_COMB_NO_MAX:

	** no maximum limit; store minimum limit at most **
	if( eisc->tmp_value > eisc->min_limit )
	  eisc->tmp_value = eisc->min_limit;
	break;

      case KHE_SRS_COMB_LINEAR:

	** linear cost; if over maximum, report excess immediately **
	if( eisc->tmp_value > eisc->max_limit )
	{
	  dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit, 0,
	    eisc->tmp_value);
	  cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
	  if( cost > 0 )
	  {
	    if( srs->rerun )
	      fprintf(stderr, "  %s reporting cost %.5f\n",
		KheMonitorId(eisc->monitor), KheCostShow(cost));
	    KheSrsExprReceiveValueCost(eisc->parent, cost, srs);
	  }
	  eisc->tmp_value = eisc->max_limit;
	}
	break;

      case KHE_SRS_COMB_STEP:

	** step cost; store maximum plus one at most **
	if( eisc->tmp_value > eisc->max_limit + 1 )
	  eisc->tmp_value = eisc->max_limit + 1;
	break;

      default:

	HnAbort("KheSrsExprIntSeqCombReceiveValueInt internal error");
	break;
    }
  }
  else if( eisc->tmp_value > 0 )
  {
    ** a non-empty sequence not at the end of the cycle has just ended **
    dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit, 0, eisc->tmp_value);
    if( dev > 0 )
    {
      cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
      if( cost > 0 )
      {
	if( srs->rerun )
	  fprintf(stderr, "  %s reporting cost %.5f\n",
	    KheMonitorId(eisc->monitor), KheCostShow(cost));
	KheSrsExprReceiveValueCost(eisc->parent, cost, srs);
      }
    }
    eisc->tmp_value = 0;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombEndDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,          */
/*    int day_index, KHE_SRS_PARTIAL_SOLN ps,                                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform eisc that day day_index is ending.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSeqCombEndDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int dev, sig_index;  KHE_COST cost;
  HnAssert(eisc->first_day_index <= day_index &&
    day_index <= eisc->last_day_index,
    "KheSrsExprIntSeqCombEndDay internal error");
  if( day_index == eisc->last_day_index )
  {
    ** this is the end of the last day covered by this expression **
    if( eisc->tmp_value > 0 )
    {
      dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit,
	eisc->history_after, eisc->tmp_value);
      if( dev > 0 )
      {
	cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
	if( cost > 0 )
	{
	  if( srs->rerun )
	    fprintf(stderr, "  %s reporting cost %.5f\n",
	      KheMonitorId(eisc->monitor), KheCostShow(cost));
	  KheSrsExprReceiveValueCost(eisc->parent, cost, srs);
	}
      }
    }
  }
  else
  {
    ** this is not the last day; store value in signature **
    sig_index = HaArray(eisc->sig_indexes, day_index - eisc->first_day_index);
    HnAssert(HaArrayCount(ps->signature) == sig_index,
      "KheSrsExprIntSeqEndDay internal error");
    HaArrayAddLast(ps->signature, eisc->tmp_value);
  }
  eisc->tmp_value = UNDEF_INT;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombDoDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,           */
/*    int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task,            */
/*    KHE_TIME time, KHE_SRS_PARTIAL_SOLN next_ps,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Process active node eisc on day_index.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSeqCombDoDay(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  int day_index, KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int si, i, dev, start_index, stop_index;
  KHE_SRS_EXPR_INT child_e;  KHE_COST cost;

  /* initialize eisc->value depending on whether day_index is its first day */
  if( day_index == eisc->first_day_index )
    eisc->value = eisc->history_before;
  else
  {
    /* this is not eisc's first day, so we need to retrieve a previous value */
    si = HaArray(eisc->sig_indexes, day_index-1 - eisc->first_day_index);
    eisc->value = HaArray(prev_ps->signature, si);
  }

  /* accumulate the values of the children of eisc that finalized today */
  start_index = HaArray(eisc->child_indexes, day_index - eisc->first_day_index);
  stop_index =  HaArray(eisc->child_indexes, day_index+1-eisc->first_day_index);
  for( i = start_index;  i < stop_index;  i++ )
  {
    child_e = (KHE_SRS_EXPR_INT) HaArray(eisc->children, i);
    if( child_e->value == 1 )
    {
      /* a sequence is beginning or continuing */
      eisc->value++;

      /* adjust in various special cases */
      switch( eisc->comb_type )
      {
	case KHE_SRS_COMB_ORDINARY:

	  /* no adjustment here */
	  break;

	case KHE_SRS_COMB_NO_MAX:

	  /* no maximum limit; store minimum limit at most */
	  if( eisc->value > eisc->min_limit )
	    eisc->value = eisc->min_limit;
	  break;

	case KHE_SRS_COMB_LINEAR:

	  /* linear cost; if over maximum, report excess immediately */
	  if( eisc->value > eisc->max_limit )
	  {
	    dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit, 0,
	      eisc->value);
	    cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
	    if( cost > 0 )
	    {
	      if( srs->rerun )
		fprintf(stderr, "  %s reporting linear cost %.5f\n",
		  KheMonitorId(eisc->monitor), KheCostShow(cost));
	      next_ps->cost += cost;
	    }
	    eisc->value = eisc->max_limit;
	  }
	  break;

	case KHE_SRS_COMB_STEP:

	  /* step cost; store maximum plus one at most */
	  if( eisc->value > eisc->max_limit + 1 )
	    eisc->value = eisc->max_limit + 1;
	  break;

	default:

	  HnAbort("KheSrsExprIntSeqCombDoDay internal error");
	  break;
      }
    }
    else if( eisc->value > 0 )
    {
      /* a non-empty sequence not at the end of the cycle has just ended */
      dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit, 0, eisc->value);
      if( dev > 0 )
      {
	cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
	if( cost > 0 )
	{
	  if( srs->rerun )
	    fprintf(stderr, "  %s reporting cost %.5f\n",
	      KheMonitorId(eisc->monitor), KheCostShow(cost));
	  next_ps->cost += cost;
	}
      }
      eisc->value = 0;
    }
  }

  if( day_index < eisc->last_day_index )
  {
    /* this not eisc's last day; store value in next_ps */
    si = HaArray(eisc->sig_indexes, day_index - eisc->first_day_index);
    HnAssert(HaArrayCount(next_ps->signature) == si,
      "KheSrsExprIntSeqDoDay internal error 3");
    HaArrayAddLast(next_ps->signature, eisc->value);
  }
  else if( eisc->value > 0 )
  {
    /* this is the last day and we have a sequence to report */
    dev = KheSrsSeqDev(eisc->min_limit, eisc->max_limit,
      eisc->history_after, eisc->value);
    if( dev > 0 )
    {
      cost = KheSrsCost(eisc->cost_fn, eisc->combined_weight, dev);
      if( cost > 0 )
      {
	if( srs->rerun )
	  fprintf(stderr, "  %s reporting cost %.5f\n",
	    KheMonitorId(eisc->monitor), KheCostShow(cost));
	next_ps->cost += cost;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombDebug(KHE_SRS_EXPR_INT_SEQ_COMB eisc,           */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the header part of eisc in ps onto fp.                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprIntSeqCombDebug(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  char buff1[100], buff2[100];  int si;
  if( ps != NULL )
  {
    /* unindented version with a ps value */
    fprintf(fp, "INT_SEQ_COMB %s", KheMonitorId(eisc->monitor));
    if( KheSrsExprInSignature((KHE_SRS_EXPR) eisc, ps->day->day_index, &si) )
      fprintf(fp, " $%d:%d$", si, HaArray(ps->signature, si));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eisc, ps, srs, verbosity, -1, fp);
  }
  else if( indent >= 0 )
  {
    /* indented version without a ps value */
    fprintf(fp, "%*s[ INT_SEQ_COMB %s %s %s\n", indent, "",
      KheMonitorId(eisc->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eisc, srs, buff1),
      KheSrsDomTestShow(eisc->dom_test, buff2));
    KheSrsExprDebugChildren((KHE_SRS_EXPR) eisc, ps, srs,
      verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    /* unindented version without a ps value */
    fprintf(fp, "[ INT_SEQ_COMB %s %s %s (%d children) ]",
      KheMonitorId(eisc->monitor),
      KheSrsExprDayRangeShow((KHE_SRS_EXPR) eisc, srs, buff1),
      KheSrsDomTestShow(eisc->dom_test, buff2), HaArrayCount(eisc->children));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprIntSeqCombDebug(KHE_SRS_EXPR_INT_SEQ_COMB eisc,           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of eisc onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprIntSeqCombDebug(KHE_SRS_EXPR_INT_SEQ_COMB eisc,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ INT_SEQ_COMB(%s, %s %.5f, lim %d-%d, h %d,%d)\n",
      indent, "", eisc->monitor != NULL ? KheMonitorId(eisc->monitor) : "-",
      KheCostFunctionShow(eisc->cost_fn), KheCostShow(eisc->combined_weight),
      eisc->min_limit, eisc->max_limit, eisc->history_before,
      eisc->history_after);
    HaArrayForEach(eisc->children, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "INT_SEQ_COMB(%s, h %d,%d, %d children)",
      eisc->monitor != NULL ? KheMonitorId(eisc->monitor) : "-",
      eisc->history_before, eisc->history_after, HaArrayCount(eisc->children));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_EXPR"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAddChild(KHE_SRS_EXPR parent, KHE_SRS_EXPR child)         */
/*                                                                           */
/*  Make child a child of parent.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprAddChild(KHE_SRS_EXPR parent, KHE_SRS_EXPR child)
{
  /* HnAssert(child->parent == NULL, "KheSrsExprAddChild internal error"); */
  HaArrayAddLast(parent->children, child);
  /* child->parent = parent; */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprEnsureDayRangeCovers(KHE_SRS_EXPR e,                      */
/*    int first_day_index, int last_day_index)                               */
/*                                                                           */
/*  Ensure that e's day range covers first_day_index .. last_day_index.      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static void KheSrsExprEnsureDayRangeCovers(KHE_SRS_EXPR e,
  int first_day_index, int last_day_index)
{
  if( first_day_index < e->first_day_index )
    e->first_day_index = first_day_index;
  if( last_day_index > e->last_day_index )
    e->last_day_index = last_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprEnsureDayRangeCoversDayIndex(KHE_SRS_EXPR e,              */
/*    int last_day_index)                                                    */
/*                                                                           */
/*  Ensure that e's day range covers last_day_index.                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprEnsureDayRangeCoversDayIndex(KHE_SRS_EXPR e,
  int day_index)
{
  if( day_index < e->first_day_index )
    e->first_day_index = day_index;
  if( day_index > e->last_day_index )
    e->last_day_index = day_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsExprIncreasingLastDayIndexCmp(const void *t1, const void *t2)  */
/*                                                                           */
/*  Comparison function for sorting an array of expressions by increasing    */
/*  last day index.                                                          */
/*                                                                           */
/*****************************************************************************/

static int KheSrsExprIncreasingLastDayIndexCmp(const void *t1, const void *t2)
{
  KHE_SRS_EXPR e1 = * (KHE_SRS_EXPR *) t1;
  KHE_SRS_EXPR e2 = * (KHE_SRS_EXPR *) t2;
  return e1->last_day_index - e2->last_day_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprSetChildIndexes(KHE_SRS_EXPR e)                           */
/*                                                                           */
/*  Assuming that e's children's day indexes are set, set e's child indexes. */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprSetChildIndexes(KHE_SRS_EXPR e)
{
  int i, curr_last_day_index;  KHE_SRS_EXPR child_e;
  HaArrayClear(e->child_indexes);
  curr_last_day_index = e->first_day_index - 1;
  HaArrayForEach(e->children, child_e, i)
  {
    while( child_e->last_day_index > curr_last_day_index )
    {
      HaArrayAddLast(e->child_indexes, i);
      curr_last_day_index++;
    }
  }
  HaArrayAddLast(e->child_indexes, i);  /* sentinel */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprSetDayRange(KHE_SRS_EXPR e, int min_last_day_index,       */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Set the day range (first_day_index and last_day_index) of e.             */
/*                                                                           */
/*  If min_last_day_index != -1, ensure that e->last_day_index is at least   */
/*  min_last_day_index.  Used for the children of INT_SEQ and INT_SEQ_COMB   */
/*  nodes, this ensures that those children report in the order they appear. */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprSetDayRange(KHE_SRS_EXPR e, /* int min_last_day_index, */
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i, index /* , prev_last_day_index */;  KHE_SRS_EXPR child_e, prev_child_e;
  KHE_SRS_EXPR_BUSY_TIME eb;  KHE_SRS_EXPR_FREE_TIME ef;
  KHE_SRS_EXPR_WORK_TIME ew;  KHE_SRS_EXPR_BUSY_DAY ebd;
  KHE_SRS_EXPR_FREE_DAY efd;  KHE_SRS_EXPR_WORK_DAY ewd;
  KHE_SRS_EXPR_INT_SEQ_COMB eisc;

  /* initialize range to empty, or to just min_last_day_index */
  e->first_day_index = INT_MAX;
  e->last_day_index = 0;
  /* ***
  if( min_last_day_index != -1 )
    KheSrsExprEnsureDayRangeCoversDayIndex(e, min_last_day_index);
  *** */

  /* increase range depending on the expression type */
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      eb = (KHE_SRS_EXPR_BUSY_TIME) e;
      index = KheFrameTimeIndex(srs->days_frame, eb->time);
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      /* KheSrsExprEnsureDayRangeCovers(e, index, index); */
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      ef = (KHE_SRS_EXPR_FREE_TIME) e;
      index = KheFrameTimeIndex(srs->days_frame, ef->time);
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      /* KheSrsExprEnsureDayRangeCovers(e, index, index); */
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      ew = (KHE_SRS_EXPR_WORK_TIME) e;
      index = KheFrameTimeIndex(srs->days_frame, ew->time);
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      /* KheSrsExprEnsureDayRangeCovers(e, index, index); */
      break;

    case KHE_SRS_EXPR_BUSY_DAY_TAG:

      ebd = (KHE_SRS_EXPR_BUSY_DAY) e;
      index = KheFrameTimeIndex(srs->days_frame,
	KheTimeGroupTime(ebd->time_group, 0));
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      break;

    case KHE_SRS_EXPR_FREE_DAY_TAG:

      efd = (KHE_SRS_EXPR_FREE_DAY) e;
      index = KheFrameTimeIndex(srs->days_frame,
	KheTimeGroupTime(efd->time_group, 0));
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      break;

    case KHE_SRS_EXPR_WORK_DAY_TAG:

      ewd = (KHE_SRS_EXPR_WORK_DAY) e;
      index = KheFrameTimeIndex(srs->days_frame,
	KheTimeGroupTime(ewd->time_group, 0));
      KheSrsExprEnsureDayRangeCoversDayIndex(e, index);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      eisc = (KHE_SRS_EXPR_INT_SEQ_COMB) e;
      prev_child_e = NULL;
      HaArrayForEach(eisc->children, child_e, i)
      {
        KheSrsExprSetDayRange(child_e, /* prev_last_day_index, */ srs);
	if( prev_child_e != NULL &&
	    prev_child_e->last_day_index > child_e->last_day_index )
	{
	  child_e = KheSrsExprOptionalCopy(child_e, srs);
	  HaArrayPut(eisc->children, i, child_e);
          child_e->last_day_index = prev_child_e->last_day_index;
	}
        KheSrsExprEnsureDayRangeCoversDayIndex(e, child_e->last_day_index);
	prev_child_e = child_e;
      }
      /* ***
      prev_last_day_index = 0;
      eisc = (KHE_SRS_EXPR_INT_SEQ_COMB) e;
      HaArrayForEach(eisc->children, child_e, i)
      {
        KheSrsExprSetDayRange(child_e, prev_last_day_index, srs);
        KheSrsExprEnsureDayRangeCoversDayIndex(e, child_e->last_day_index);
	prev_last_day_index = child_e->last_day_index;
      }
      *** */
      /* children already have increasing last day index, must not sort them */
      break;

    default:

      HaArrayForEach(e->children, child_e, i)
      {
        KheSrsExprSetDayRange(child_e, /* -1, */ srs);
        KheSrsExprEnsureDayRangeCoversDayIndex(e, child_e->last_day_index);
	/* ***
	KheSrsExprEnsureDayRangeCovers(e, child_e->first_day_index,
	  child_e->last_day_index);
	*** */
      }
      HaArraySort(e->children, &KheSrsExprIncreasingLastDayIndexCmp);
      break;
  }

  /* set the child indexes */
  KheSrsExprSetChildIndexes(e);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DOM_TEST KheSrsExprChildDomTest(KHE_SRS_EXPR e,                  */
/*    KHE_SRS_DOM_TEST prnt_dom_test)                                        */
/*                                                                           */
/*  Return e's dom test, as a function of the type of e, any limits in e,    */
/*  and e's parent's dom test.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
KHE_SRS_DOM_TEST KheSrsExprDomTest(KHE_SRS_EXPR e)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TAG:

      return KheSrsExprBusyDomTest((KHE_SRS_EXPR_BUSY) e);

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      return KheSrsExprFreeDomTest((KHE_SRS_EXPR_FREE_TIME) e);

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      return KheSrsExprWorkTimeDomTest((KHE_SRS_EXPR_WORK_TIME) e);

    case KHE_SRS_EXPR_OR_TAG:

      return KheSrsExprOrDomTest((KHE_SRS_EXPR_OR) e);

    case KHE_SRS_EXPR_AND_TAG:

      return KheSrsExprAndDomTest((KHE_SRS_EXPR_AND) e);

    case KHE_SRS_EXPR_INT_SUM_TAG:

      return KheSrsExprIntSumDomTest((KHE_SRS_EXPR_INT_SUM) e);

    ** *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      return KheSrsExprIntSeqDomTest((KHE_SRS_EXPR_INT_SEQ) e);
    *** **

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      return KheSrsExprFloatSumDomTest((KHE_SRS_EXPR_FLOAT_SUM) e);

    case KHE_SRS_EXPR_INT_DEV_TAG:

      return KheSrsExprIntDevDomTest((KHE_SRS_EXPR_INT_DEV) e);

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      return KheSrsExprFloatDevDomTest((KHE_SRS_EXPR_FLOAT_DEV) e);

    case KHE_SRS_EXPR_COST_TAG:

      return KheSrsExprCostDomTest((KHE_SRS_EXPR_COST) e);

    case KHE_SRS_EXPR_COST_SUM_TAG:

      return KheSrsExprCostSumDomTest((KHE_SRS_EXPR_COST_SUM) e);

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      return KheSrsExprIntSumCombDomTest((KHE_SRS_EXPR_INT_SUM_COMB) e);

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      return KheSrsExprIntSeqCombDomTest((KHE_SRS_EXPR_INT_SEQ_COMB) e);

    default:

      HnAbort("KheSrsExprDomTest internal error: unknown tag %d", e->tag);
      return KheSrsDomTestMake(KHE_SRS_DOM_NONE, 0);  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprSetSigIndexes(KHE_SRS_EXPR e,                             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Set the signature indexes in e and its descendants.                      */
/*                                                                           */
/*  These are assigned in postorder, because that is the order that          */
/*  KheSrsExprEndDay visits the nodes.                                       */
/*                                                                           */
/*****************************************************************************/
static void KheSrsDayAddDomTest(KHE_SRS_DAY day, KHE_SRS_DOM_TEST dom_test);

static void KheSrsExprSetSigIndexes(KHE_SRS_EXPR e,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR child_e;  int i;  KHE_SRS_DAY day;

  /* find e's dom test, as a function of its type and its parent's dom test */
  /* e->dom_test = KheSrsExprDomTest(e); */

  /* set the signature indexes in e's descendants */
  HaArrayForEach(e->children, child_e, i)
    KheSrsExprSetSigIndexes(child_e, srs);

  /* set the signature indexes in e (NB < is correct, not <= here) */
  for( i = e->first_day_index;  i < e->last_day_index;  i++ )
  {
    day = HaArray(srs->days, i);
    HaArrayAddLast(e->sig_indexes, day->sig_len++);
    HnAssert(e->dom_test.type != KHE_SRS_DOM_UNUSED,
      "KheSrsExprSetSigIndexes internal error");
    KheSrsDayAddDomTest(day, e->dom_test);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsExprHasActiveDays(KHE_SRS_EXPR e)                             */
/*                                                                           */
/*  Return true if e is of a type that has active days, and hence needs      */
/*  to be called by BeginDay and EndDay.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheSrsExprHasActiveDays(KHE_SRS_EXPR e)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_INT_DEV_TAG:
    case KHE_SRS_EXPR_FLOAT_DEV_TAG:
    case KHE_SRS_EXPR_COST_TAG:

      return false;

    default:

      return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprAddExpressionsToDays(KHE_SRS_EXPR e,                      */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Add e and its descendants to the days they are active on.  Most          */
/*  importantly, do it in postorder.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprAddExpressionsToDays(KHE_SRS_EXPR e,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR child_e;  int i;  KHE_SRS_DAY day;

  /* do it for e's proper descendants (first!) */
  HaArrayForEach(e->children, child_e, i)
    KheSrsExprAddExpressionsToDays(child_e, srs);

  /* do it for e (NB <= is correct, not < here) */
  for( i = e->first_day_index;  i <= e->last_day_index;  i++ )
  {
    day = HaArray(srs->days, i);
    HaArrayAddLast(day->active_today, e);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprDelete(KHE_SRS_EXPR e, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Delete e and its descendants, taking care not to delete something that   */
/*  has already been deleted.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprDelete(KHE_SRS_EXPR e, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  if( !e->deleted )
  {
    e->deleted = true;
    switch( e->tag )
    {
      case KHE_SRS_EXPR_BUSY_TIME_TAG:

	KheSrsExprBusyTimeDelete((KHE_SRS_EXPR_BUSY_TIME) e, srs);
	break;

      case KHE_SRS_EXPR_FREE_TIME_TAG:

	KheSrsExprFreeTimeDelete((KHE_SRS_EXPR_FREE_TIME) e, srs);
	break;

      case KHE_SRS_EXPR_WORK_TIME_TAG:

	KheSrsExprWorkTimeDelete((KHE_SRS_EXPR_WORK_TIME) e, srs);
	break;

      case KHE_SRS_EXPR_BUSY_DAY_TAG:

	KheSrsExprBusyDayDelete((KHE_SRS_EXPR_BUSY_DAY) e, srs);
	break;

      case KHE_SRS_EXPR_FREE_DAY_TAG:

	KheSrsExprFreeDayDelete((KHE_SRS_EXPR_FREE_DAY) e, srs);
	break;

      case KHE_SRS_EXPR_WORK_DAY_TAG:

	KheSrsExprWorkDayDelete((KHE_SRS_EXPR_WORK_DAY) e, srs);
	break;

      case KHE_SRS_EXPR_OR_TAG:

	KheSrsExprOrDelete((KHE_SRS_EXPR_OR) e, srs);
	break;

      case KHE_SRS_EXPR_AND_TAG:

	KheSrsExprAndDelete((KHE_SRS_EXPR_AND) e, srs);
	break;

      case KHE_SRS_EXPR_INT_SUM_TAG:

	KheSrsExprIntSumDelete((KHE_SRS_EXPR_INT_SUM) e, srs);
	break;

      /* *** currently unused
      case KHE_SRS_EXPR_INT_SEQ_TAG:

	KheSrsExprIntSeqDelete((KHE_SRS_EXPR_INT_SEQ) e, srs);
	break;
      *** */

      case KHE_SRS_EXPR_FLOAT_SUM_TAG:

	KheSrsExprFloatSumDelete((KHE_SRS_EXPR_FLOAT_SUM) e, srs);
	break;

      case KHE_SRS_EXPR_INT_DEV_TAG:

	KheSrsExprIntDevDelete((KHE_SRS_EXPR_INT_DEV) e, srs);
	break;

      case KHE_SRS_EXPR_FLOAT_DEV_TAG:

	KheSrsExprFloatDevDelete((KHE_SRS_EXPR_FLOAT_DEV) e, srs);
	break;

      case KHE_SRS_EXPR_COST_TAG:

	KheSrsExprCostDelete((KHE_SRS_EXPR_COST) e, srs);
	break;

      /* ***
      case KHE_SRS_EXPR_COST_SUM_TAG:

	KheSrsExprCostSumDelete((KHE_SRS_EXPR_COST_SUM) e, srs);
	break;
      *** */

      case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

	KheSrsExprIntSumCombDelete((KHE_SRS_EXPR_INT_SUM_COMB) e, srs);
	break;

      case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

	KheSrsExprIntSeqCombDelete((KHE_SRS_EXPR_INT_SEQ_COMB) e, srs);
	break;

      default:

	HnAbort("KheSrsExprDelete internal error: unknown tag %d", e->tag);
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprDoDay(KHE_SRS_EXPR e, int day_index,                      */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,            */
/*    KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Process active expression e on day day_index.                            */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprDoDay(KHE_SRS_EXPR e, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_TASK task, KHE_TIME time,
  KHE_SRS_PARTIAL_SOLN next_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      KheSrsExprBusyTimeDoDay((KHE_SRS_EXPR_BUSY_TIME) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreeTimeDoDay((KHE_SRS_EXPR_FREE_TIME) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimeDoDay((KHE_SRS_EXPR_WORK_TIME) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_BUSY_DAY_TAG:

      KheSrsExprBusyDayDoDay((KHE_SRS_EXPR_BUSY_DAY) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_FREE_DAY_TAG:

      KheSrsExprFreeDayDoDay((KHE_SRS_EXPR_FREE_DAY) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_WORK_DAY_TAG:

      KheSrsExprWorkDayDoDay((KHE_SRS_EXPR_WORK_DAY) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrDoDay((KHE_SRS_EXPR_OR) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndDoDay((KHE_SRS_EXPR_AND) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumDoDay((KHE_SRS_EXPR_INT_SUM) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    /* *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqDoDay((KHE_SRS_EXPR_INT_SEQ) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;
    *** */

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumDoDay((KHE_SRS_EXPR_FLOAT_SUM) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevDoDay((KHE_SRS_EXPR_INT_DEV) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevDoDay((KHE_SRS_EXPR_FLOAT_DEV) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostDoDay((KHE_SRS_EXPR_COST) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    /* ***
    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumDoDay((KHE_SRS_EXPR_COST_SUM) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;
    *** */

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombDoDay((KHE_SRS_EXPR_INT_SUM_COMB) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombDoDay((KHE_SRS_EXPR_INT_SEQ_COMB) e, day_index,
	prev_ps, task, time, next_ps, srs);
      break;

    default:

      HnAbort("KheSrsExprDoDay internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprBeginDay(KHE_SRS_EXPR e, int day_index,                   */
/*    KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)          */
/*                                                                           */
/*  Inform e that day day_index is beginning.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprBeginDay(KHE_SRS_EXPR e, int day_index,
  KHE_SRS_PARTIAL_SOLN prev_ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      KheSrsExprBusyBeginDay((KHE_SRS_EXPR_BUSY_TIME) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreeBeginDay((KHE_SRS_EXPR_FREE_TIME) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimeBeginDay((KHE_SRS_EXPR_WORK_TIME) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrBeginDay((KHE_SRS_EXPR_OR) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndBeginDay((KHE_SRS_EXPR_AND) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumBeginDay((KHE_SRS_EXPR_INT_SUM) e, day_index,
	prev_ps, srs);
      break;

    ** *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqBeginDay((KHE_SRS_EXPR_INT_SEQ) e, day_index,
	prev_ps, srs);
      break;
    *** **

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumBeginDay((KHE_SRS_EXPR_FLOAT_SUM) e, day_index,
	prev_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevBeginDay((KHE_SRS_EXPR_INT_DEV) e, day_index,
	prev_ps, srs);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevBeginDay((KHE_SRS_EXPR_FLOAT_DEV) e, day_index,
	prev_ps, srs);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostBeginDay((KHE_SRS_EXPR_COST) e, day_index, prev_ps, srs);
      break;

    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumBeginDay((KHE_SRS_EXPR_COST_SUM) e, day_index,
	prev_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombBeginDay((KHE_SRS_EXPR_INT_SUM_COMB) e, day_index,
	prev_ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombBeginDay((KHE_SRS_EXPR_INT_SEQ_COMB) e, day_index,
	prev_ps, srs);
      break;

    default:

      HnAbort("KheSrsExprBeginDay internal error");
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprReceiveValueLeaf(KHE_SRS_EXPR e,                          */
/*    KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)                         */
/*                                                                           */
/*  Inform leaf e that task (possibly NULL) is running at its time.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprReceiveValueLeaf(KHE_SRS_EXPR e,
  KHE_TASK task, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      KheSrsExprBusyReceiveValueLeaf((KHE_SRS_EXPR_BUSY_TIME) e, task, srs);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreeReceiveValueLeaf((KHE_SRS_EXPR_FREE_TIME) e, task, srs);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimeReceiveValueLeaf((KHE_SRS_EXPR_WORK_TIME) e, task, srs);
      break;

    default:

      HnAbort("KheSrsExprReceiveValueLeaf internal error (tag %d)", e->tag);
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprReceiveValueInt(KHE_SRS_EXPR e, int value,                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform e that one of its children has just finalized its integer value.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprReceiveValueInt(KHE_SRS_EXPR e, int value,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrReceiveValueInt((KHE_SRS_EXPR_OR) e, value, srs);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndReceiveValueInt((KHE_SRS_EXPR_AND) e, value, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumReceiveValueInt((KHE_SRS_EXPR_INT_SUM) e, value, srs);
      break;

    ** *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqReceiveValueInt((KHE_SRS_EXPR_INT_SEQ) e, value, srs);
      break;
    *** **

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevReceiveValueInt((KHE_SRS_EXPR_INT_DEV) e, value, srs);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostReceiveValueInt((KHE_SRS_EXPR_COST) e, value, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombReceiveValueInt((KHE_SRS_EXPR_INT_SUM_COMB) e,
	value, srs);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombReceiveValueInt((KHE_SRS_EXPR_INT_SEQ_COMB) e,
	value, srs);
      break;

    default:

      HnAbort("KheSrsExprReceiveValueInt internal error (tag %d)", e->tag);
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprReceiveValueFloat(KHE_SRS_EXPR e, float value,            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform e that one of its children has just finalized its float value.    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprReceiveValueFloat(KHE_SRS_EXPR e, float value,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumReceiveValueFloat((KHE_SRS_EXPR_FLOAT_SUM) e,
	value, srs);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevReceiveValueFloat((KHE_SRS_EXPR_FLOAT_DEV) e,
	value, srs);
      break;

    default:

      HnAbort("KheSrsExprReceiveValueFloat internal error (tag %d)", e->tag);
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprReceiveValueCost(KHE_SRS_EXPR e, KHE_COST value,          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Inform e that one of its children has just finalized its cost value.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprReceiveValueCost(KHE_SRS_EXPR e, KHE_COST value,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumReceiveValueCost((KHE_SRS_EXPR_COST_SUM) e, value, srs);
      break;

    default:

      HnAbort("KheSrsExprReceiveValueCost internal error (tag %d)", e->tag);
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprEndDay(KHE_SRS_EXPR e, int day_index,                     */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Inform e that day day_index is ending.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprEndDay(KHE_SRS_EXPR e, int day_index,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TAG:

      KheSrsExprBusyEndDay((KHE_SRS_EXPR_BUSY_TIME) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreeEndDay((KHE_SRS_EXPR_FREE_TIME) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimeEndDay((KHE_SRS_EXPR_WORK_TIME) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrEndDay((KHE_SRS_EXPR_OR) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndEndDay((KHE_SRS_EXPR_AND) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumEndDay((KHE_SRS_EXPR_INT_SUM) e, day_index, ps, srs);
      break;

    ** *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqEndDay((KHE_SRS_EXPR_INT_SEQ) e, day_index, ps, srs);
      break;
    *** **

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumEndDay((KHE_SRS_EXPR_FLOAT_SUM) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevEndDay((KHE_SRS_EXPR_INT_DEV) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevEndDay((KHE_SRS_EXPR_FLOAT_DEV) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostEndDay((KHE_SRS_EXPR_COST) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumEndDay((KHE_SRS_EXPR_COST_SUM) e, day_index, ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombEndDay((KHE_SRS_EXPR_INT_SUM_COMB) e, day_index,
	ps, srs);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombEndDay((KHE_SRS_EXPR_INT_SEQ_COMB) e, day_index,
	ps, srs);
      break;

    default:

      HnAbort("KheSrsExprEndDay internal error");
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprPartialSolnDebug(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps, */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)               */
/*                                                                           */
/*  Debug print of active expression e, as it is currently in ps.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprPartialSolnDebug(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      KheSrsExprBusyTimePartialSolnDebug((KHE_SRS_EXPR_BUSY_TIME) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreePartialSolnDebug((KHE_SRS_EXPR_FREE_TIME) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimePartialSolnDebug((KHE_SRS_EXPR_WORK_TIME) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrPartialSolnDebug((KHE_SRS_EXPR_OR) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndPartialSolnDebug((KHE_SRS_EXPR_AND) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumPartialSolnDebug((KHE_SRS_EXPR_INT_SUM) e, ps,
	srs, verbosity, fp);
      break;

    ** *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqPartialSolnDebug((KHE_SRS_EXPR_INT_SEQ) e, ps,
    srs, verbosity, fp);
      break;
    *** **

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumPartialSolnDebug((KHE_SRS_EXPR_FLOAT_SUM) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevPartialSolnDebug((KHE_SRS_EXPR_INT_DEV) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevPartialSolnDebug((KHE_SRS_EXPR_FLOAT_DEV) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostPartialSolnDebug((KHE_SRS_EXPR_COST) e, ps,
	srs, verbosity, fp);
      break;

    ** ***
    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumPartialSolnDebug((KHE_SRS_EXPR_COST_SUM) e, ps,
	srs, verbosity, fp);
      break;
    *** **

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombPartialSolnDebug((KHE_SRS_EXPR_INT_SUM_COMB) e, ps,
	srs, verbosity, fp);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombPartialSolnDebug((KHE_SRS_EXPR_INT_SEQ_COMB) e, ps,
	srs, verbosity, fp);
      break;

    default:

      HnAbort("KheSrsExprPartialSolnDebug internal error");
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprDebugPartialSoln(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps, */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of e's state under ps, if active.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprDebugPartialSoln(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)
{
  KHE_SRS_EXPR child_e;  int i;  bool child_printed;
  KheSrsExprPartialSolnDebug(e, ps, srs, verbosity, fp);
  if( HaArrayCount(e->children) > 0 )
  {
    fprintf(fp, "(");
    child_printed = false;
    HaArrayForEach(e->children, child_e, i)
      if( child_e->first_day_index <= ps->day->day_index &&
	  ps->day->day_index < child_e->last_day_index )
      {
	if( child_printed )
	  fprintf(fp, ", ");
        KheSrsExprDebugPartialSoln(child_e, ps, srs, verbosity, fp);
	child_printed = true;
      }
    fprintf(fp, ")");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprDebug(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of e onto fp with the given verbosity and indent.            */
/*                                                                           */
/*  If ps != NULL, only those parts of e that have a value stored in ps's    */
/*  signature are printed, along with those values.  In that case indent     */
/*  is ignored and the whole print is on one line.                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsExprDebug(KHE_SRS_EXPR e, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  switch( e->tag )
  {
    case KHE_SRS_EXPR_BUSY_TIME_TAG:

      KheSrsExprBusyTimeDebug((KHE_SRS_EXPR_BUSY_TIME) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_FREE_TIME_TAG:

      KheSrsExprFreeTimeDebug((KHE_SRS_EXPR_FREE_TIME) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_WORK_TIME_TAG:

      KheSrsExprWorkTimeDebug((KHE_SRS_EXPR_WORK_TIME) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_BUSY_DAY_TAG:

      KheSrsExprBusyDayDebug((KHE_SRS_EXPR_BUSY_DAY) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_FREE_DAY_TAG:

      KheSrsExprFreeDayDebug((KHE_SRS_EXPR_FREE_DAY) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_WORK_DAY_TAG:

      KheSrsExprWorkDayDebug((KHE_SRS_EXPR_WORK_DAY) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_OR_TAG:

      KheSrsExprOrDebug((KHE_SRS_EXPR_OR) e, ps, srs, verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_AND_TAG:

      KheSrsExprAndDebug((KHE_SRS_EXPR_AND) e, ps, srs, verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_INT_SUM_TAG:

      KheSrsExprIntSumDebug((KHE_SRS_EXPR_INT_SUM) e, ps, srs,
	verbosity, indent, fp);
      break;

    /* *** currently unused
    case KHE_SRS_EXPR_INT_SEQ_TAG:

      KheSrsExprIntSeqDebug((KHE_SRS_EXPR_INT_SEQ) e, ps, srs, verbosity,
	indent, fp);
      break;
    *** */

    case KHE_SRS_EXPR_FLOAT_SUM_TAG:

      KheSrsExprFloatSumDebug((KHE_SRS_EXPR_FLOAT_SUM) e, ps, srs,
	verbosity, indent,fp);
      break;

    case KHE_SRS_EXPR_INT_DEV_TAG:

      KheSrsExprIntDevDebug((KHE_SRS_EXPR_INT_DEV) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_FLOAT_DEV_TAG:

      KheSrsExprFloatDevDebug((KHE_SRS_EXPR_FLOAT_DEV) e, ps, srs,
	verbosity, indent,fp);
      break;

    case KHE_SRS_EXPR_COST_TAG:

      KheSrsExprCostDebug((KHE_SRS_EXPR_COST) e, ps, srs,
	verbosity, indent, fp);
      break;

    /* ***
    case KHE_SRS_EXPR_COST_SUM_TAG:

      KheSrsExprCostSumDebug((KHE_SRS_EXPR_COST_SUM) e, ps, srs, verbosity,
	indent, fp);
      break;
    *** */

    case KHE_SRS_EXPR_INT_SUM_COMB_TAG:

      KheSrsExprIntSumCombDebug((KHE_SRS_EXPR_INT_SUM_COMB) e, ps, srs,
	verbosity, indent, fp);
      break;

    case KHE_SRS_EXPR_INT_SEQ_COMB_TAG:

      KheSrsExprIntSeqCombDebug((KHE_SRS_EXPR_INT_SEQ_COMB) e, ps, srs,
	verbosity, indent, fp);
      break;

    default:

      HnAbort("KheSrsExprDebug internal error: unknown tag %d", e->tag);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_STEP"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_STEP KheSrsStepGet(KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Get a new srs task object.  Only the times_by_day and tasks_by day       */
/*  fields are initialized.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_STEP KheSrsStepGet(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_STEP res;
  if( HaArrayCount(srs->free_steps) > 0 )
    res = HaArrayLastAndDelete(srs->free_steps);
  else
    HaMake(res, srs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_STEP KheSrsStepMake(KHE_TASK task,                               */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new srs step from task.                                           */
/*                                                                           */
/*  This code aborts if task has no assigned times.  That is safe given      */
/*  the context of the sole call to this function below.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_STEP KheSrsStepMake(KHE_TASK task,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_STEP res;  KHE_COST non_asst_cost, asst_cost;
  res = KheSrsStepGet(srs);
  res->task = task;
  res->daily_schedule = KheTaskFinderTaskDailySchedule(srs->task_finder, task);
  /* res->cost_reduction = KheTaskAssignmentCostReduction(task); */
  KheTaskNonAsstAndAsstCost(task, &non_asst_cost, &asst_cost);
  res->cost_reduction = (non_asst_cost - asst_cost);
  res->domain_count = KheResourceGroupResourceCount(KheTaskDomain(task));
  res->already_assigned = (KheTaskAsstResource(task) != NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_STEP KheSrsStepMakeFree(int day_index,                           */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new srs step which represents doing nothing on the day with       */
/*  this day_index.                                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_STEP KheSrsStepMakeFree(int day_index,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_STEP res;
  res = KheSrsStepGet(srs);
  res->task = NULL;
  res->daily_schedule = KheTaskFinderNullDailySchedule(srs->task_finder,
    KheIntervalMake(day_index, day_index));
  res->cost_reduction = 0;
  res->domain_count = 0;
  res->already_assigned = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsStepDelete(KHE_SRS_STEP step, KHE_SINGLE_RESOURCE_SOLVER srs) */
/*                                                                           */
/*  Delete step.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheSrsStepDelete(KHE_SRS_STEP step, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KheDailyScheduleDelete(step->daily_schedule);
  HaArrayAddLast(srs->free_steps, step);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheSrsStepFirstTime(KHE_SRS_STEP step)                          */
/*                                                                           */
/*  Return the first time of step.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheSrsStepFirstTime(KHE_SRS_STEP step)
{
  int index;
  index = KheIntervalFirst(KheDailyScheduleInterval(step->daily_schedule));
  return KheDailyScheduleTime(step->daily_schedule, index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheSrsStepLastTime(KHE_SRS_STEP step)                           */
/*                                                                           */
/*  Return the last time of step.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheSrsStepLastTime(KHE_SRS_STEP step)
{
  int index;
  index = KheIntervalLast(KheDailyScheduleInterval(step->daily_schedule));
  return KheDailyScheduleTime(step->daily_schedule, index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsStepBetterThan(KHE_SRS_STEP step1, KHE_SRS_STEP step2)        */
/*                                                                           */
/*  Return true if step1 is a better choice than step2, because it has a     */
/*  larger cost reduction, or equal cost reduction and a smaller domain.     */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsStepBetterThan(KHE_SRS_STEP step1, KHE_SRS_STEP step2)
{
  return step1->cost_reduction > step2->cost_reduction ||
    (step1->cost_reduction == step2->cost_reduction &&
     step1->domain_count < step2->domain_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsStepDebug(KHE_SRS_STEP step, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of step onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsStepDebug(KHE_SRS_STEP step, int verbosity,
  int indent, FILE *fp)
{
  KHE_TIME first_time, last_time;  /* int first_day_index, last_day_index; */
  KHE_TASK_FINDER tf;  /* KHE_TIME_GROUP tg1, tg2; */  KHE_FRAME frame;
  KHE_INTERVAL in;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( step->task != NULL )
  {
    first_time = KheSrsStepFirstTime(step);
    last_time = KheSrsStepLastTime(step);
    KheTaskDebug(step->task, verbosity, -1, fp);
    if( first_time != last_time )
      fprintf(fp, " (%s-%s", KheTimeId(first_time), KheTimeId(last_time));
    else
      fprintf(fp, " (%s", KheTimeId(first_time));
    fprintf(fp, ", cost %.5f, domain %d)",
      KheCostShow(step->cost_reduction), step->domain_count);
  }
  else
  {
    tf = KheDailyScheduleTaskFinder(step->daily_schedule);
    frame = KheTaskFinderFrame(tf);
    in = KheDailyScheduleInterval(step->daily_schedule);
    fprintf(fp, "Free (%s)", KheIntervalShow(in, frame));
    /* ***
    first_day_index = KheDailyScheduleFirstDayIndex(step->daily_schedule);
    last_day_index = KheDailyScheduleLastDayIndex(step->daily_schedule);
    tg1 = KheFrameTimeGroup(frame, first_day_index);
    tg2 = KheFrameTimeGroup(frame, last_day_index);
    if( tg1 != tg2 )
      fprintf(fp, "Free (%s-%s)", KheTimeGroupId(tg1), KheTimeGroupId(tg2));
    else
      fprintf(fp, "Free (%s)", KheTimeGroupId(tg1));
    *** */
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_TIME"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_TIME KheSrsTimeMake(KHE_TIME t, HA_ARENA a)                      */
/*                                                                           */
/*  Make and return a new srs time object representing t.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_TIME KheSrsTimeMake(KHE_TIME t, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_TIME res;
  HaMake(res, srs->arena);
  res->time = t;
  /* HaArrayInit(res->leaf_exprs, srs->arena); */
  HaArrayInit(res->best_steps, srs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsTimeClearSteps(KHE_SRS_TIME st,                               */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Clear the steps of st.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsTimeClearSteps(KHE_SRS_TIME st,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i;  KHE_SRS_STEP step;
  HaArrayForEach(st->best_steps, step, i)
    KheSrsStepDelete(step, srs);
  HaArrayClear(st->best_steps);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsTimeClear(KHE_SRS_TIME st, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Clear st, that is, clear out its steps.                                  */
/*                                                                           */
/*  There is no function to delete st because that is never wanted.  And     */
/*  the expressions are deleted elsewhere, so they are not deleted here.     */
/*  But the tasks are not deleted elsewhere, so they are deleted here.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsTimeClear(KHE_SRS_TIME st, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i;  KHE_SRS_STEP step;
  HaArrayForEach(st->best_steps, step, i)
    KheSrsStepDelete(step, srs);
  HaArrayClear(st->best_steps);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsTimeContainsLastTime(KHE_SRS_TIME stime,                      */
/*    KHE_TIME last_time, KHE_SRS_STEP *step, int *pos)                      */
/*                                                                           */
/*  If stime contains a step with the given last_time, return true with      */
/*  *step set to that step and *pos set to its position.  Otherwise return   */
/*  false with *step set to NULL and *pos set to -1.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsTimeContainsLastTime(KHE_SRS_TIME stime,
  KHE_TIME last_time, KHE_SRS_STEP *step, int *pos)
{
  KHE_SRS_STEP step2;  int i;
  HaArrayForEach(stime->best_steps, step2, i)
    if( KheSrsStepLastTime(step2) == last_time )
      return *step = step2, *pos = i, true;
  return *step = NULL, *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsTimeAddStep(KHE_SRS_TIME st, KHE_SRS_STEP step,               */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Add step to st.  More precisely, if it's better than what st already     */
/*  has, then add it, otherwise delete it.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSrsTimeAddStep(KHE_SRS_TIME st, KHE_SRS_STEP step,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_STEP step2;  int pos;
  if( !KheSrsTimeContainsLastTime(st, KheSrsStepLastTime(step), &step2, &pos) )
  {
    /* st has no step with the same last time as step, so add step to st */
    HaArrayAddLast(st->best_steps, step);
  }
  else if( KheSrsStepBetterThan(step, step2) )
  {
    /* step is better than the similar task step2, so replace step2 by step */
    KheSrsStepDelete(step2, srs);
    HaArrayPut(st->best_steps, pos, step);
  }
  else
  {
    /* step is worse than the similar step2, so delete step */
    KheSrsStepDelete(step, srs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsTimeDebug(KHE_SRS_TIME st, KHE_SINGLE_RESOURCE_SOLVER srs,    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of st onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsTimeDebug(KHE_SRS_TIME st, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  int i;  /* KHE_SRS_EXPR e; */  KHE_SRS_STEP step;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Time %s:\n", indent, "", KheTimeId(st->time));
    /* ***
    HaArrayForEach(st->leaf_exprs, e, i)
      KheSrsExprDebug(e, srs, verbosity, indent + 2, fp);
    *** */
    HaArrayForEach(st->best_steps, step, i)
      KheSrsStepDebug(step, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "[ Time %s (%d best_steps) ]", KheTimeId(st->time),
      HaArrayCount(st->best_steps));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_PARTIAL_SOLN"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnGet(KHE_SINGLE_RESOURCE_SOLVER srs)*/
/*                                                                           */
/*  Get a partial soln object.  Only the signature field is initialized.     */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnGet(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN res;
  if( HaArrayCount(srs->free_partial_solns) > 0 )
  {
    res = HaArrayLastAndDelete(srs->free_partial_solns);
    HaArrayClear(res->signature);
  }
  else
  {
    HaMake(res, srs->arena);
    HaArrayInit(res->signature, srs->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnMakeInit(                          */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make the partial solution that the search grows from.  It does not       */
/*  end on any day.                                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnMakeInit(
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN res;
  res = KheSrsPartialSolnGet(srs);
  res->prev_ps = NULL;
  res->step = NULL;
  res->day = NULL;
  res->best_of = 1;
  res->cost = 0;
  HaArrayAddLast(res->signature, 0);  /* number of assignments */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnMake(KHE_SRS_PARTIAL_SOLN prev_ps, */
/*    KHE_SRS_STEP step, KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Make a new partial solution with these attributes.                       */
/*                                                                           */
/*****************************************************************************/
static void KheSrsPartialSolnDebug(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp);

static KHE_SRS_PARTIAL_SOLN KheSrsPartialSolnMake(KHE_SRS_PARTIAL_SOLN prev_ps,
  KHE_SRS_STEP step, KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN res;  int asst_count, i;  KHE_TIME time;
  KHE_SRS_EXPR e;  KHE_TASK task;

  /* make the basic object */
  HnAssert(step != NULL, "KheSrsPartialSolnMake internal error (step)");
  if( DEBUG4 )
    fprintf(stderr, "[ KheSrsPartialSolnMake(prev_ps, %s, %s, srs)\n",
      step->task == NULL ? "-" : "task", KheTimeGroupId(day->time_group));
  res = KheSrsPartialSolnGet(srs);
  res->prev_ps = prev_ps;
  res->step = step;  /* in the free case, st's task will be null */
  res->day = day;
  res->best_of = 1;

  /* set the cost and signature */
  res->cost = prev_ps->cost;
  task = KheDailyScheduleTask(step->daily_schedule, day->day_index);
  time = KheDailyScheduleTime(step->daily_schedule, day->day_index);
  asst_count = HaArrayFirst(prev_ps->signature) + (task != NULL ? 1 : 0);
  HaArrayAddLast(res->signature, asst_count);
  HaArrayForEach(day->active_today, e, i)
    KheSrsExprDoDay(e, day->day_index, prev_ps, task, time, res, srs);

  /* *** old code that paid multiple visits
  HaArrayForEach(day->active_exprs, e, i)
    KheSrsExprBeginDay(e, day_index, prev_ps, srs);
  time = KheDailyScheduleTime(step->daily_schedule, day_index);
  HaArrayForEach(day->times, stime, i)
  {
    if( stime->time == time )
      HaArrayForEach(stime->leaf_exprs, e, j)
	KheSrsExprReceiveValueLeaf(e, step->task, srs);
    else
      HaArrayForEach(stime->leaf_exprs, e, j)
	KheSrsExprReceiveValueLeaf(e, NULL, srs);
  }
  HaArrayForEach(day->active_exprs, e, i)
    KheSrsExprEndDay(e, day_index, res, srs);
  *** */

  if( DEBUG4 )
  {
    KheSrsPartialSolnDebug(res, srs, 1, 2, stderr);
    fprintf(stderr, "] KheSrsPartialSolnMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnDelete(KHE_SRS_PARTIAL_SOLN ps,                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ps.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnDelete(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HaArrayAddLast(srs->free_partial_solns, ps);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPartialSolnSignatureFullHash(KHE_SRS_PARTIAL_SOLN ps)          */
/*                                                                           */
/*  Hash function for hashing the full signature of ps.                      */
/*                                                                           */
/*****************************************************************************/

static int KheSrsPartialSolnSignatureFullHash(KHE_SRS_PARTIAL_SOLN ps)
{
  int res, val, i;
  res = 0;
  HaArrayForEach(ps->signature, val, i)
    res = (res + val) * 3;
  return (res < 0 ? - res : res);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPartialSolnSignatureFullHashUntyped(void *p)                   */
/*                                                                           */
/*  Untyped version of KheSrsPartialSolnSignatureHash.                       */
/*                                                                           */
/*****************************************************************************/

static int KheSrsPartialSolnSignatureFullHashUntyped(void *p)
{
  return KheSrsPartialSolnSignatureFullHash((KHE_SRS_PARTIAL_SOLN) p);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPartialSolnSignaturePartialHash(KHE_SRS_PARTIAL_SOLN ps)       */
/*                                                                           */
/*  Hash function for hashing the partial signature of ps, that is, only     */
/*  the parts of it whose dominance tests are equality (or loose equality).  */
/*                                                                           */
/*****************************************************************************/

static int KheSrsPartialSolnSignaturePartialHash(KHE_SRS_PARTIAL_SOLN ps)
{
  int res, index, val, i;  KHE_SRS_DAY day;
  day = ps->day;
  res = 0;
  HaArrayForEach(day->eq_dom_test_indexes, index, i)
  {
    val = HaArray(ps->signature, index);
    res = (res + val) * 3;
  }
  if( res < 0 )
    res = - res;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPartialSolnSignaturePartialHashUntyped(void *p)                */
/*                                                                           */
/*  Untyped version of KheSrsPartialSolnSignaturePartialHash.                */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheSrsPartialSolnSignaturePartialHashUntyped(void *p)
{
  return KheSrsPartialSolnSignaturePartialHash((KHE_SRS_PARTIAL_SOLN) p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnSignatureFullEqual(KHE_SRS_PARTIAL_SOLN ps1,       */
/*    KHE_SRS_PARTIAL_SOLN ps2)                                              */
/*                                                                           */
/*  Return true if the signatures of ps1 and ps2 are equal.  The lengths     */
/*  of their signatures must be equal.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPartialSolnSignatureFullEqual(KHE_SRS_PARTIAL_SOLN ps1,
  KHE_SRS_PARTIAL_SOLN ps2)
{
  int i;
  HnAssert(HaArrayCount(ps1->signature) == HaArrayCount(ps2->signature),
    "KheSrsPartialSolnSignaturesEqual: signatures have different lengths");
  for( i = 0;  i < HaArrayCount(ps1->signature);  i++ )
    if( HaArray(ps1->signature, i) != HaArray(ps2->signature, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnSignatureFullEqualUntyped(void *p1, void *p2)      */
/*                                                                           */
/*  Untyped version of KheSrsPartialSolnSignaturesEqual.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPartialSolnSignatureFullEqualUntyped(void *p1, void *p2)
{
  return KheSrsPartialSolnSignatureFullEqual((KHE_SRS_PARTIAL_SOLN) p1,
    (KHE_SRS_PARTIAL_SOLN) p2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnSignaturePartialEqual(KHE_SRS_PARTIAL_SOLN ps1,    */
/*    KHE_SRS_PARTIAL_SOLN ps2)                                              */
/*                                                                           */
/*  Return true if the signatures of ps1 and ps2 are partially equal, that   */
/*  is, the entries whose dominance tests are equality (or loose equality)   */
/*  are equal.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPartialSolnSignaturePartialEqual(KHE_SRS_PARTIAL_SOLN ps1,
  KHE_SRS_PARTIAL_SOLN ps2)
{
  int i, index;  KHE_SRS_DAY day;
  HnAssert(HaArrayCount(ps1->signature) == HaArrayCount(ps2->signature),
   "KheSrsPartialSolnSignaturePartialEqual: signatures have different lengths");
  day = ps1->day;
  HaArrayForEach(day->eq_dom_test_indexes, index, i)
    if( HaArray(ps1->signature, index) != HaArray(ps2->signature, index) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnSignaturePartialEqualUntyped(void *p1, void *p2)   */
/*                                                                           */
/*  Untyped version of KheSrsPartialSolnSignaturesEqual.                     */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheSrsPartialSolnSignaturePartialEqualUntyped(void *p1, void *p2)
{
  return KheSrsPartialSolnSignaturePartialEqual((KHE_SRS_PARTIAL_SOLN) p1,
    (KHE_SRS_PARTIAL_SOLN) p2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSignatureDebug(KHE_SRS_PARTIAL_SOLN ps, FILE *fp)  */
/*                                                                           */
/*  Debug print of the sigature of ps onto fp, with no verbosity or indent.  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSignatureDebug(KHE_SRS_PARTIAL_SOLN ps, FILE *fp)
{
  int i, val;
  fprintf(fp, "[");
  HaArrayForEach(ps->signature, val, i)
  {
    if( i > 0 )
      fprintf(fp, " ");
    fprintf(fp, "%d", val);
  }
  fprintf(fp, "]");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSignatureDebugUntyped(void *p, FILE *fp)           */
/*                                                                           */
/*  Untyped version of KheSrsPartialSolnSignatureDebug.                      */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSignatureDebugUntyped(void *p, FILE *fp)
{
  KheSrsPartialSolnSignatureDebug((KHE_SRS_PARTIAL_SOLN) p, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnDominates(KHE_SRS_PARTIAL_SOLN ps1,                */
/*    KHE_SRS_PARTIAL_SOLN ps2, int start_depth)                             */
/*                                                                           */
/*  Return true if ps1 dominates ps2.  They come from the same day.          */
/*  Parameter start_depth says how far along the signature to start.         */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPartialSolnDominates(KHE_SRS_PARTIAL_SOLN ps1,
  KHE_SRS_PARTIAL_SOLN ps2, int start_depth)
{
  KHE_SRS_DOM_TEST dt;  int i, val1, val2;  KHE_SRS_DAY day;
  HnAssert(ps1->day == ps2->day,
    "KheSrsPartialSolnDominates internal error: ps1->day = %s, ps2->day = %s",
    ps1->day != NULL ? KheTimeGroupId(ps1->day->time_group) : "-",
    ps2->day != NULL ? KheTimeGroupId(ps2->day->time_group) : "-");
  if( ps2->cost < ps1->cost )
    return false;
  day = ps1->day;
  for( i = start_depth;  i < day->sig_len;  i++ )
  {
    dt = HaArray(day->dom_tests, i);
    val1 = HaArray(ps1->signature, i);
    val2 = HaArray(ps2->signature, i);
    if( !KheSrsDomTestDominates(dt, val1, val2) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnOverWrite(KHE_SRS_PARTIAL_SOLN dest_ps,            */
/*    KHE_SRS_PARTIAL_SOLN src_ps)                                           */
/*                                                                           */
/*  Overwrite src_ps onto dest_ps.  Their signatures are known to be         */
/*  equal, so those don't need to be touched.  Also the best_of field        */
/*  is not touched here; it gets updated separately.                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnOverWrite(KHE_SRS_PARTIAL_SOLN dest_ps,
  KHE_SRS_PARTIAL_SOLN src_ps)
{
  dest_ps->prev_ps = src_ps->prev_ps;
  dest_ps->step = src_ps->step;
  dest_ps->cost = src_ps->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPartialSolnCostLessThan(KHE_SRS_PARTIAL_SOLN ps1,             */
/*    KHE_SRS_PARTIAL_SOLN ps2)                                              */
/*                                                                           */
/*  Return true if ps1 is a better solution than ps2.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheSrsPartialSolnCostLessThan(KHE_SRS_PARTIAL_SOLN ps1,
  KHE_SRS_PARTIAL_SOLN ps2)
{
  return ps1->cost < ps2->cost;
    ** || (ps1->r_cost == ps2->r_cost && ps1->soln_cost < ps2->soln_cost); **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPartialSolnIncreasingAsstCountCmp(const void *p1, void *p2)    */
/*                                                                           */
/*  Comparison function for sorting an array of partial solutions by         */
/*  increasing assignment count.                                             */
/*                                                                           */
/*****************************************************************************/

static int KheSrsPartialSolnIncreasingAsstCountCmp(const void *p1,
  const void *p2)
{
  KHE_SRS_PARTIAL_SOLN ps1 = * (KHE_SRS_PARTIAL_SOLN *) p1;
  KHE_SRS_PARTIAL_SOLN ps2 = * (KHE_SRS_PARTIAL_SOLN *) p2;
  return HaArrayFirst(ps1->signature) - HaArrayFirst(ps2->signature);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnDoDebug(KHE_SRS_PARTIAL_SOLN ps,                   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of task and expression of ps.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsPartialSolnDoDebug(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_TIME_GROUP tg;
  if( ps->task != NULL )
    KheTaskDebug(ps->task, 1, -1, fp);
  else
    fprintf(fp, "-");
  tg = KheFrameTimeGroup(srs->days_frame, ps->day->day_index);
  fprintf(fp, " ending on %s (cost %.5f):\n", KheTimeGroupId(tg),
    KheCostShow(ps->cost));
  KheSrsExprDebugPartialSoln((KHE_SRS_EXPR) srs->root_expr, ps, srs,
    verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnDebugHeader(KHE_SRS_PARTIAL_SOLN ps,               */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header part of ps.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsPartialSolnDebugHeader(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, FILE *fp)
{
  KHE_TIME_GROUP tg;
  fprintf(fp, "PS(task ");
  if( ps->step->task != NULL )
    KheTaskDebug(ps->step->task, 1, -1, fp);
  else
    fprintf(fp, "-");
  if( ps->day->day_index < 0 )
    fprintf(fp, ", ending on -");
  else
  {
    tg = KheFrameTimeGroup(srs->days_frame, ps->day->day_index);
    fprintf(fp, ", ending on %s", KheTimeGroupId(tg));
  }
  fprintf(fp, ", assts %d, best of %d, cost %.5f)", HaArray(ps->signature, 0),
    ps->best_of, KheCostShow(ps->cost));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsExprCostSumDebugPartialSoln(KHE_SRS_EXPR_COST_SUM ecs,        */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug the ecs part of ps.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSrsExprCostSumDebugPartialSoln(KHE_SRS_EXPR_COST_SUM ecs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  fprintf(fp, "%*s[ COST_SUM\n", indent, "");
  HaArrayForEach(ecs->children, e, i)
    if( e->first_day_index <= ps->day->day_index &&
	ps->day->day_index < e->last_day_index )
    {
      fprintf(fp, "%*s", indent + 2, "");
      KheSrsExprDebugPartialSoln(e, ps, srs, verbosity, fp);
      fprintf(fp, "\n");
    }
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnDebugBasics(KHE_SRS_PARTIAL_SOLN ps, FILE *fp)     */
/*                                                                           */
/*  Produce an unindented one-line debug print of the basics of ps.          */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnDebugBasics(KHE_SRS_PARTIAL_SOLN ps, FILE *fp)
{
  /* step */
  fprintf(fp, "PS(");
  if( ps->step != NULL && ps->step->task != NULL )
    KheTaskDebug(ps->step->task, 1, -1, fp);
  else
    fprintf(fp, "-");

  /* day_index */
  if( ps->day == NULL )
    fprintf(fp, ", to -");
  else
    fprintf(fp, ", to %s", KheTimeGroupId(ps->day->time_group));

  /* assts, best_of, and cost */
  fprintf(fp, ", assts %d, best of %d, cost %.5f)",
    HaArray(ps->signature, 0), ps->best_of, KheCostShow(ps->cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnDebug(KHE_SRS_PARTIAL_SOLN ps,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of ps onto fp with the given verbosity and indent.           */
/*                                                                           */
/*  NB If verbosity == 1 or indent < 0, it is safe to pass NULL for srs.     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnDebug(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs, int verbosity, int indent, FILE *fp)
{
  KHE_SRS_EXPR e;  int i;
  if( indent < 0 )
    KheSrsPartialSolnDebugBasics(ps, fp);
  else if( verbosity == 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheSrsPartialSolnDebugBasics(ps, fp);
    fprintf(fp, "\n");
  }
  else
  {
    /* basics */
    fprintf(fp, "%*s[ ", indent, "");
    KheSrsPartialSolnDebugBasics(ps, fp);
    fprintf(fp, "\n");

    /* signature */
    HaArrayForEach(srs->exprs, e, i)
    {
      if( KheSrsExprFirstDebugDayIndex(e) <= ps->day->day_index &&
	  ps->day->day_index <= e->last_day_index )
      {
	fprintf(fp, "%*s", indent + 2, "");
	KheSrsExprDebug(e, ps, srs, verbosity, -1, fp);
	fprintf(fp, "\n");
      }
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  /* ***
  if( indent < 0 )
    KheSrsPartialSolnDebugHeader(ps, srs, verbosity, fp);
  else
  {
    fprintf(fp, "%*s[ Partial Soln ", indent, "");
    KheSrsPartialSolnDebugHeader(ps, srs, verbosity, fp);
    fprintf(fp, "\n");
    KheSrsExprCostSumDebugPartialSoln(srs->root_expr, ps, srs,
      verbosity, indent + 2, fp);
    if( ps->prev_ps != NULL )
    {
      fprintf(fp, "%*sprev_ps ", indent + 2, "");
      KheSrsPartialSolnDebugHeader(ps->prev_ps, srs, verbosity, fp);
      fprintf(fp, "\n");
      KheSrsExprCostSumDebugPartialSoln(srs->root_expr, ps->prev_ps,
	srs, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_PS_SET"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PS_SET KheSrsPsSetMake(int busy_days,                            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a new, empty KHE_SRS_PS_SET object for this number of busy days.  */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_PS_SET KheSrsPsSetMake(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PS_SET res;
  if( HaArrayCount(srs->free_ps_sets) > 0 )
  {
    res = HaArrayLastAndDelete(srs->free_ps_sets);
    HaArrayClear(res->partial_solns);
  }
  else
  {
    HaMake(res, srs->arena);
    HaArrayInit(res->partial_solns, srs->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsSetDelete(KHE_SRS_PS_SET ps_set,                            */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete ps_set and the partial solutions in it.                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsSetDelete(KHE_SRS_PS_SET ps_set,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN ps;  int i;
  HaArrayForEach(ps_set->partial_solns, ps, i)
    KheSrsPartialSolnDelete(ps, srs);
  HaArrayAddLast(srs->free_ps_sets, ps_set);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSrsPsSetSignaturePartialHashUntyped(void *p)                      */
/*                                                                           */
/*  Hash function for ps sets, being the partial hash of the set's first     */
/*  element.                                                                 */
/*                                                                           */
/*****************************************************************************/

static int KheSrsPsSetSignaturePartialHashUntyped(void *p)
{
  KHE_SRS_PS_SET ps_set;  KHE_SRS_PARTIAL_SOLN ps;
  ps_set = (KHE_SRS_PS_SET) p;
  ps = HaArrayFirst(ps_set->partial_solns);
  return KheSrsPartialSolnSignaturePartialHash(ps);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPsSetSignaturePartialEqualUntyped(void *p1, void *p2)         */
/*                                                                           */
/*  Equality function for ps sets, being the partial equality of the sets'   */
/*  first elements.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPsSetSignaturePartialEqualUntyped(void *p1, void *p2)
{
  KHE_SRS_PS_SET ps_set1, ps_set2;  KHE_SRS_PARTIAL_SOLN ps1, ps2;
  ps_set1 = (KHE_SRS_PS_SET) p1;
  ps_set2 = (KHE_SRS_PS_SET) p2;
  ps1 = HaArrayFirst(ps_set1->partial_solns);
  ps2 = HaArrayFirst(ps_set2->partial_solns);
  return KheSrsPartialSolnSignaturePartialEqual(ps1, ps2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsSetSignatureDebugUntyped(void *p, FILE *fp)                 */
/*                                                                           */
/*  Debug print of the signature of ps set p.                                */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsSetSignatureDebugUntyped(void *p, FILE *fp)
{
  KHE_SRS_PS_SET ps_set;  KHE_SRS_PARTIAL_SOLN ps;
  ps_set = (KHE_SRS_PS_SET) p;
  ps = HaArrayFirst(ps_set->partial_solns);
  KheSrsPartialSolnSignatureDebug(ps, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_PS_TRIE"                                              */
/*                                                                           */
/*  Abstraction function                                                     */
/*  --------------------                                                     */
/*                                                                           */
/*  A ps_trie represents a set of partial solutions.  If ps_trie == NULL,    */
/*  the set is empty.  Otherwise the set consists of ps_trie->partial_soln   */
/*  (if non-NULL) plus the sets stored in each of the children.              */
/*                                                                           */
/*  Invariant                                                                */
/*  ---------                                                                */
/*                                                                           */
/*  Exactly one of these three cases applies:                                */
/*                                                                           */
/*    (1)  ps_trie == NULL                                                   */
/*    (2)  ps_trie->partial_soln != NULL                                     */
/*    (3)  HaArrayCount(ps_trie->children) > 0                               */
/*                                                                           */
/*  and in the last case, the last child of ps_trie->children is non-NULL.   */
/*  Each partial solution appears in the trie in a place determined by       */
/*  the sequence of values in its signature, in the usual trie manner.       */
/*                                                                           */
/*  There is no requirement that two or more partial solutions be present    */
/*  in case (3).  If there are no partial solutions, case (1) always         */
/*  applies; if there are two or more partial solutions, case (3) always     */
/*  applies; but if there is exactly one partial solution, then either of    */
/*  cases (2) or (3) applies.  This is because it would be too slow and      */
/*  tedious to detect situations where a case (3) node could be simplified   */
/*  to a case (2) node.                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PS_TRIE KheSrsPsTrieMake(KHE_SRS_PARTIAL_SOLN ps,                */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Make a new trie object containing ps.  It satisfies case (2) above.      */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_PS_TRIE KheSrsPsTrieMake(KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PS_TRIE res;
  HnAssert(ps != NULL, "KheSrsPsTrieMake internal error (ps == NULL)");
  if( HaArrayCount(srs->free_ps_tries) > 0 )
  {
    res = HaArrayLastAndDelete(srs->free_ps_tries);
    HaArrayClear(res->children);
  }
  else
  {
    HaMake(res, srs->arena);
    HaArrayInit(res->children, srs->arena);
  }
  res->partial_soln = ps;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieDelete(KHE_SRS_PS_TRIE *ps_trie,                        */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Delete *ps_trie and all its descendants and partial solutions.           */
/*  Set *ps_trie to NULL.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsTrieDelete(KHE_SRS_PS_TRIE *ps_trie,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i;
  if( *ps_trie == NULL )
  {
    /* case (1), do nothing */
  }
  else if( (*ps_trie)->partial_soln != NULL )
  {
    /* case (2), delete (*ps_trie)->partial_soln and *ps_trie */
    KheSrsPartialSolnDelete((*ps_trie)->partial_soln, srs);
    HaArrayAddLast(srs->free_ps_tries, *ps_trie);
    *ps_trie = NULL;
  }
  else
  {
    /* case (3), delete children and *ps_trie */
    for( i = 0;  i < HaArrayCount((*ps_trie)->children);  i++ )
      KheSrsPsTrieDelete(&HaArray((*ps_trie)->children, i), srs);
    HaArrayAddLast(srs->free_ps_tries, *ps_trie);
    *ps_trie = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieInsertChild(KHE_SRS_PS_TRIE ps_trie,                    */
/*    KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Insert ps in its appropriate place among the children of ps_trie.        */
/*                                                                           */
/*****************************************************************************/
static void KheSrsPsTrieInsert(KHE_SRS_PS_TRIE *ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs);

static void KheSrsPsTrieInsertChild(KHE_SRS_PS_TRIE ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int index;
  index = HaArray(ps->signature, depth);
  HaArrayFill(ps_trie->children, index + 1, NULL);
  KheSrsPsTrieInsert(&HaArray(ps_trie->children, index), ps, depth + 1, srs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieInsert(KHE_SRS_PS_TRIE *ps_trie,                        */
/*    KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Insert ps into *ps_trie, possibly changing *ps_trie as a result.  The    */
/*  depth parameter indicates how far along the signature we are.            */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsTrieInsert(KHE_SRS_PS_TRIE *ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  if( *ps_trie == NULL )
  {
    /* case (1), make a new node containing just ps, -> case (2) */
    *ps_trie = KheSrsPsTrieMake(ps, srs);
  }
  else if( (*ps_trie)->partial_soln != NULL )
  {
    /* case (2), add ps and (*ps_trie)->partial_soln to children, -> case (3) */
    KheSrsPsTrieInsertChild(*ps_trie, ps, depth, srs);
    ps = (*ps_trie)->partial_soln;
    (*ps_trie)->partial_soln = NULL;
    KheSrsPsTrieInsertChild(*ps_trie, ps, depth, srs);
  }
  else
  {
    /* case (3), add ps to children, -> case (3) */
    KheSrsPsTrieInsertChild(*ps_trie, ps, depth, srs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPsTrieDominates(KHE_SRS_PS_TRIE ps_trie,                      */
/*    KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)    */
/*                                                                           */
/*  Return true if there is a partial solution in ps_trie that dominates ps. */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPsTrieDominates(KHE_SRS_PS_TRIE ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int val, from_val, to_val, i;  KHE_SRS_DOM_TEST dom_test;
  if( ps_trie == NULL )
  {
    /* case (1); no partial solutions in ps_trie, so none dominate */
    return false;
  }
  else if( ps_trie->partial_soln != NULL )
  {
    /* case (2); the only possible dominator is ps_trie->partial_soln */
    return KheSrsPartialSolnDominates(ps_trie->partial_soln, ps, depth);
  }
  else
  {
    /* case (3); find range of dominators and try each of them */
    dom_test = HaArray(ps->day->dom_tests, depth);
    val = HaArray(ps->signature, depth);
    KheSrsDomTestDominatingRange(dom_test, val, &from_val, &to_val);
    if( to_val >= HaArrayCount(ps_trie->children) )
      to_val = HaArrayCount(ps_trie->children) - 1;
    /* *** the first test may be erroneous; the second is useless
    HnAssert(0 <= from_val && from_val < HaArrayCount(ps_trie->children),
      "KheSrsPsTrieDominates error 1 (from_val %d out of range 0 .. %d)\n",
       from_val, HaArrayCount(ps_trie->children) - 1);
    HnAssert(0 <= to_val && to_val < HaArrayCount(ps_trie->children),
      "KheSrsPsTrieDominates error 1 (to_val %d out of range 0 .. %d)\n",
       to_val, HaArrayCount(ps_trie->children) - 1);
    *** */
    for( i = from_val;  i <= to_val;  i++ )
      if( KheSrsPsTrieDominates(HaArray(ps_trie->children,i), ps, depth+1, srs))
	return true;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieRemoveDominated(KHE_SRS_PS_TRIE *ps_trie,               */
/*    KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs,    */
/*    int *total_count)                                                      */
/*                                                                           */
/*  Remove those elements of *ps_trie that are dominated by ps.  This        */
/*  can change *ps_trie.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsTrieRemoveDominated(KHE_SRS_PS_TRIE *ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, int depth, KHE_SINGLE_RESOURCE_SOLVER srs,
  int *total_count)
{
  int val, from_val, to_val, i;  KHE_SRS_DOM_TEST dom_test;
  if( *ps_trie == NULL )
  {
    /* case (1); there is nothing to remove */
  }
  else if( (*ps_trie)->partial_soln != NULL )
  {
    /* case (2); if (*ps_trie)->partial_soln is dominated, remove whole trie */
    if( KheSrsPartialSolnDominates(ps, (*ps_trie)->partial_soln, depth) )
    {
      KheSrsPsTrieDelete(ps_trie, srs);
      (*total_count)--;
    }
  }
  else
  {
    /* case (3); removed dominated partial solns among the children */
    dom_test = HaArray(ps->day->dom_tests, depth);
    val = HaArray(ps->signature, depth);
    KheSrsDomTestDominatedRange(dom_test, val, &from_val, &to_val);
    if( to_val >= HaArrayCount((*ps_trie)->children) )
      to_val = HaArrayCount((*ps_trie)->children) - 1;
    for( i = from_val;  i <= to_val;  i++ )
      KheSrsPsTrieRemoveDominated(&HaArray((*ps_trie)->children, i), ps,
	depth + 1, srs, total_count);

    /* remove trailing NULL children */
    while( HaArrayCount((*ps_trie)->children) > 0 &&
	HaArrayLast((*ps_trie)->children) == NULL )
      HaArrayDeleteLast((*ps_trie)->children);

    /* if children array is empty then the whole trie is empty, so delete it */
    if( HaArrayCount((*ps_trie)->children) == 0 )
      KheSrsPsTrieDelete(ps_trie, srs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieDoStrongDominanceTest(KHE_SRS_PS_TRIE *ps_trie,         */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int *total_count)                                                      */
/*                                                                           */
/*  Carry out a strong dominance test between *ps_trie and ps.               */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsTrieDoStrongDominanceTest(KHE_SRS_PS_TRIE *ps_trie,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs, int *total_count)
{
  if( DEBUG12 )
    fprintf(stderr, "[ KheSrsPsTrieDoStrongDominanceTest(ps for %s)\n",
      KheTimeGroupId(ps->day->time_group));
  if( KheSrsPsTrieDominates(*ps_trie, ps, 0, srs) )
  {
    /* ps is dominated by some other partial soln, so delete ps */
    if( DEBUG12 )
      fprintf(stderr, "  trie deleting ps for %s\n",
	KheTimeGroupId(ps->day->time_group));
    KheSrsPartialSolnDelete(ps, srs);
  }
  else
  {
    /* remove partial solutions that ps dominates, and insert ps */
    if( DEBUG12 )
      fprintf(stderr, "  trie inserting ps for %s\n",
	KheTimeGroupId(ps->day->time_group));
    KheSrsPsTrieRemoveDominated(ps_trie, ps, 0, srs, total_count);
    KheSrsPsTrieInsert(ps_trie, ps, 0, srs);
    (*total_count)++;
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheSrsPsTrieDoStrongDominanceTest returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieExtend(KHE_SRS_PS_TRIE ps_trie,                         */
/*    KHE_SRS_DAY next_day, KHE_SINGLE_RESOURCE_SOLVER srs)                  */
/*                                                                           */
/*  Extend the solutions in ps_trie to next_day.                             */
/*                                                                           */
/*****************************************************************************/
static void KheSingleResourceSolverExtend(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_DAY day);

static void KheSrsPsTrieExtend(KHE_SRS_PS_TRIE ps_trie,
  KHE_SRS_DAY next_day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PS_TRIE child_trie;  int i;
  if( ps_trie == NULL )
  {
    /* case (1); do nothing */
  }
  else if( ps_trie->partial_soln != NULL )
  {
    /* case (2); extend ps_trie->partial_soln */
    KheSingleResourceSolverExtend(srs, ps_trie->partial_soln, next_day);
  }
  else
  {
    /* case (3); handle children */
    HaArrayForEach(ps_trie->children, child_trie, i)
      KheSrsPsTrieExtend(child_trie, next_day, srs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsTrieExtractFinalSolns(KHE_SRS_PS_TRIE ps_trie,              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Extract all the solutions from ps_trie and make them final solutions.    */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsTrieExtractFinalSolns(KHE_SRS_PS_TRIE ps_trie,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PS_TRIE child_trie;  int i;
  if( ps_trie == NULL )
  {
    /* case (1); do nothing */
  }
  else if( ps_trie->partial_soln != NULL )
  {
    /* case (2); extract ps_trie->partial_soln */
    HaArrayAddLast(srs->final_solns, ps_trie->partial_soln);
  }
  else
  {
    /* case (3); handle children */
    HaArrayForEach(ps_trie->children, child_trie, i)
      KheSrsPsTrieExtractFinalSolns(child_trie, srs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_PARTIAL_SOLN_SET"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN_SET KheSrsPartialSolnSetMake(                       */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return a new KHE_SRS_PARTIAL_SOLN_SET object.                            */
/*                                                                           */
/*****************************************************************************/

KHE_SRS_PARTIAL_SOLN_SET KheSrsPartialSolnSetMake(KHE_TIME_GROUP tg,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN_SET res;
  HaMake(res, srs->arena);
  res->time_group = tg;
  res->count = 0;
  HpTableInit(res->weak_dom_set,
    &KheSrsPartialSolnSignatureFullHashUntyped,
    &KheSrsPartialSolnSignatureFullEqualUntyped,
    &KheSrsPartialSolnSignatureDebugUntyped, srs->arena);
  HpTableInit(res->medium_dom_set,
    &KheSrsPsSetSignaturePartialHashUntyped,
    &KheSrsPsSetSignaturePartialEqualUntyped,
    &KheSrsPsSetSignatureDebugUntyped, srs->arena);
  HaArrayInit(res->strong_dom_set, srs->arena);
  res->trie_dom_set = NULL;  /* represents the empty set */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSetClear(KHE_SRS_PARTIAL_SOLN_SET pss,             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Remove and delete the partial solutions and ps_sets held in pss.         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSetClear(KHE_SRS_PARTIAL_SOLN_SET pss,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int pos;  KHE_SRS_PARTIAL_SOLN ps;  KHE_SRS_PS_SET ps_set;
  HpTableForEachValue(pss->weak_dom_set, ps, pos)
    KheSrsPartialSolnDelete(ps, srs);
  HpTableClear(pss->weak_dom_set);
  HpTableForEachValue(pss->medium_dom_set, ps_set, pos)
    KheSrsPsSetDelete(ps_set, srs);
  HpTableClear(pss->medium_dom_set);
  HaArrayForEach(pss->strong_dom_set, ps_set, pos)
    KheSrsPsSetDelete(ps_set, srs);
  HaArrayClear(pss->strong_dom_set);
  KheSrsPsTrieDelete(&pss->trie_dom_set, srs);
  HnAssert(pss->trie_dom_set == NULL,
    "KheSrsPartialSolnSetClear internal error");
  pss->count = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPsSetDoStrongDominanceTest(KHE_SRS_PS_SET ps_set,             */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,               */
/*    int *total_count)                                                      */
/*                                                                           */
/*  Carry out a strong dominance test between ps_set and ps.                 */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPsSetDoStrongDominanceTest(KHE_SRS_PS_SET ps_set,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SINGLE_RESOURCE_SOLVER srs,
  int *total_count)
{
  KHE_SRS_PARTIAL_SOLN other_ps;  int i;

  /* if ps is dominated by anything else, delete ps and return */
  HaArrayForEach(ps_set->partial_solns, other_ps, i)
    if( KheSrsPartialSolnDominates(other_ps, ps, 0) )
    {
      KheSrsPartialSolnDelete(ps, srs);
      return;
    }

  /* remove other partial solutions that ps dominates */
  HaArrayForEach(ps_set->partial_solns, other_ps, i)
    if( KheSrsPartialSolnDominates(ps, other_ps, 0) )
    {
      KheSrsPartialSolnDelete(other_ps, srs);
      HaArrayDeleteAndPlug(ps_set->partial_solns, i);
      (*total_count)--;
      i--;
    }

  /* add ps to ps_set */
  HaArrayAddLast(ps_set->partial_solns, ps);
  (*total_count)++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSetAddPartialSoln(KHE_SRS_PARTIAL_SOLN_SET pss,    */
/*  KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)*/
/*                                                                           */
/*  Add ps to pss.                                                           */
/*                                                                           */
/*  Strong dominance                                                         */
/*  ----------------                                                         */
/*                                                                           */
/*  For each partial solution other_pos in pss, if ps is dominated by        */
/*  other_ps, then delete ps and return.                                     */
/*                                                                           */
/*  Otherwise, delete each other_ps that ps dominates, then add ps.          */
/*                                                                           */
/*  Weak dominance                                                           */
/*  --------------                                                           */
/*                                                                           */
/*  If pss does not already contain a partial solution with the same         */
/*  signature as ps, just add ps to pss and return.                          */
/*                                                                           */
/*  If pss already contains a partial solution other_ps with the same        */
/*  signature as ps, keep whichever one of ps and other_ps has the smallest  */
/*  cost, by overwriting other_ps's fields with ps's fields, if required.    */
/*                                                                           */
/*  If ps and other_ps have equal cost, keep one chosen at random, with      */
/*  weights which ensure that if there are K partial solutions with the      */
/*  same signature and the same cost, then each has probability 1/K of       */
/*  being the one kept.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSetAddPartialSoln(KHE_SRS_PARTIAL_SOLN_SET pss,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN other_ps;  int asst_count;
  KHE_SRS_PS_SET ps_set, other_ps_set;
  switch( srs->dom_kind )
  {
    case KHE_SRS_DOM_WEAK:

      /* do equivalence testing */
      if( HpTableAddUnique(pss->weak_dom_set, (void *) ps, ps, other_ps) )
	pss->count++;
      else
      {
	/* other_ps will be the best of one more partial solution than before */
	other_ps->best_of++;

	/* ensure other_ps is the better of other_ps and ps, by overwriting */
	if( ps->cost < other_ps->cost || (ps->cost == other_ps->cost &&
	      srs->diversifier % other_ps->best_of == 0) )
	{
	  /* ps is better than other_ps, so overwrite other_ps with ps */
	  KheSrsPartialSolnOverWrite(other_ps, ps);
	}

	/* free ps */
	KheSrsPartialSolnDelete(ps, srs);
      }
      break;

    case KHE_SRS_DOM_MEDIUM:

      /* make a ps_set holding just ps and try adding it to medium_dom_set */
      ps_set = KheSrsPsSetMake(srs);
      HaArrayAddLast(ps_set->partial_solns, ps);
      if( HpTableAddUnique(pss->medium_dom_set, (void *) ps_set, ps_set,
	    other_ps_set) )
      {
	/* ps_set added, so that's all that's needed */
	pss->count++;
      }
      else
      {
	/* remove ps_set and do strong dominance of ps with other_ps_set */
	HaArrayClear(ps_set->partial_solns);
	KheSrsPsSetDelete(ps_set, srs);
        KheSrsPsSetDoStrongDominanceTest(other_ps_set, ps, srs, &pss->count);
      }
      break;

    case KHE_SRS_DOM_STRONG:

      /* find or make ps_set, the KHE_SRS_PS_SET which should hold ps */
      asst_count = HaArrayFirst(ps->signature);
      while( HaArrayCount(pss->strong_dom_set) <= asst_count )
      {
	ps_set = KheSrsPsSetMake(srs);
	HaArrayAddLast(pss->strong_dom_set, ps_set);
      }
      ps_set = HaArray(pss->strong_dom_set, asst_count);

      /* carry out strong dominance test between ps_set and ps */
      KheSrsPsSetDoStrongDominanceTest(ps_set, ps, srs, &pss->count);
      break;

    case KHE_SRS_DOM_TRIE:

      KheSrsPsTrieDoStrongDominanceTest(&pss->trie_dom_set, ps, srs,
	&pss->count);
      break;

    default:

      HnAbort("KheSrsPartialSolnSetAddPartialSoln internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSetExtend(KHE_SRS_PARTIAL_SOLN_SET pss,            */
/*    KHE_SRS_DAY next_day, KHE_SINGLE_RESOURCE_SOLVER srs)                  */
/*                                                                           */
/*  Extend pss by adding tasks from next_day.                                */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSetExtend(KHE_SRS_PARTIAL_SOLN_SET pss,
  KHE_SRS_DAY next_day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN ps;  int i, j;  KHE_SRS_PS_SET ps_set;
  switch( srs->dom_kind )
  {
    case KHE_SRS_DOM_WEAK:

      if( DEBUG9 )
      {
	float probes;
	probes = HpTableProbeLength(pss->weak_dom_set);
	fprintf(stderr, "  %s hash table: count %d, size %d, probes %.2f\n",
	  KheTimeGroupId(pss->time_group), pss->count,
	  HpTableSize(pss->weak_dom_set), probes);
	if( probes > 14.0 )
	  HpTableDebug(pss->weak_dom_set, 2, stderr);
      }
      HpTableForEachValue(pss->weak_dom_set, ps, i)
	KheSingleResourceSolverExtend(srs, ps, next_day);
      break;

    case KHE_SRS_DOM_MEDIUM:

      HpTableForEachValue(pss->medium_dom_set, ps_set, i)
	HaArrayForEach(ps_set->partial_solns, ps, j)
	  KheSingleResourceSolverExtend(srs, ps, next_day);
      break;

    case KHE_SRS_DOM_STRONG:

      HaArrayForEach(pss->strong_dom_set, ps_set, i)
	HaArrayForEach(ps_set->partial_solns, ps, j)
	  KheSingleResourceSolverExtend(srs, ps, next_day);
      break;

    case KHE_SRS_DOM_TRIE:

      KheSrsPsTrieExtend(pss->trie_dom_set, next_day, srs);
      break;

    default:

      HnAbort("KheSrsPartialSolnSetExtend internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsPartialSolnSetExtractFinalSolns(KHE_SRS_PARTIAL_SOLN_SET pss, */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Extract final solutions from pss.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheSrsPartialSolnSetExtractFinalSolns(KHE_SRS_PARTIAL_SOLN_SET pss,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_PARTIAL_SOLN ps;  int i, j;  KHE_SRS_PS_SET ps_set;
  switch( srs->dom_kind )
  {
    case KHE_SRS_DOM_WEAK:

      HpTableForEachValue(pss->weak_dom_set, ps, i)
	HaArrayAddLast(srs->final_solns, ps);
      break;

    case KHE_SRS_DOM_MEDIUM:

      HpTableForEachValue(pss->medium_dom_set, ps_set, i)
	HaArrayForEach(ps_set->partial_solns, ps, j)
	  HaArrayAddLast(srs->final_solns, ps);
      break;

    case KHE_SRS_DOM_STRONG:

      HaArrayForEach(pss->strong_dom_set, ps_set, i)
	HaArrayForEach(ps_set->partial_solns, ps, j)
	  HaArrayAddLast(srs->final_solns, ps);
      break;

    case KHE_SRS_DOM_TRIE:

      KheSrsPsTrieExtractFinalSolns(pss->trie_dom_set, srs);
      break;

    default:

      HnAbort("KheSrsPartialSolnSetExtractFinalSolns internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_DAY"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_DAY KheSrsDayMake(int day_index, KHE_SINGLE_RESOURCE_SOLVER srs) */
/*                                                                           */
/*  Make a new day object for the day with this day index.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_DAY KheSrsDayMake(int day_index, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_DAY res;  KHE_TIME t;  int i;  KHE_SRS_TIME stime;
  HaMake(res, srs->arena);
  res->day_index = day_index;
  res->time_group = KheFrameTimeGroup(srs->days_frame, day_index);
  HaArrayInit(res->times, srs->arena);
  for( i = 0;  i < KheTimeGroupTimeCount(res->time_group);  i++ )
  {
    t = KheTimeGroupTime(res->time_group, i);
    stime = KheSrsTimeMake(t, srs);
    HaArrayAddLast(res->times, stime);
  }
  /* res->partial_solns_count = 0; */
  res->sig_len = 1;  /* asst_count is always at start of signature */
  HaArrayInit(res->dom_tests, srs->arena);
  HaArrayInit(res->eq_dom_test_indexes, srs->arena);
  KheSrsDayAddDomTest(res, KheSrsDomTestMake(KHE_SRS_DOM_EQ));
  /* HaArrayInit(res->leaf_exprs, srs->arena); */
  HaArrayInit(res->active_today, srs->arena);
  HaArrayInit(res->leaf_today, srs->arena);
  /* HaArrayInit(res->active_exprs, srs->arena); */
  res->free_step = KheSrsStepMakeFree(day_index, srs);
  res->partial_soln_set = KheSrsPartialSolnSetMake(res->time_group, srs);
  /* ***
  HpTableInit(res->partial_solns_table, &KheSrsPartialSolnSignatureHashUntyped,
    &KheSrsPartialSolnSignaturesEqualUntyped,
    &KheSrsPartialSolnSignatureDebugUntyped, srs->arena);
  HaArrayInit(res->partial_solns_array, srs->arena);
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayClearSteps(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)*/
/*                                                                           */
/*  Clear out all the steps running on day, except the free step.            */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayClearSteps(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i;  KHE_SRS_TIME t;
  HaArrayForEach(day->times, t, i)
    KheSrsTimeClearSteps(t, srs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayClear(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)     */
/*                                                                           */
/*  Clear day, ready for a fresh solve.                                      */
/*                                                                           */
/*  Implementation note.  The expressions in leaf_today will be deleted      */
/*  separately, as part of KheSingleResourceSolverClear.                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayClear(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  /* int pos;  KHE_SRS_PARTIAL_SOLN ps; */
  KheSrsDayClearSteps(day, srs);
  /* day->partial_solns_count = 0; */
  day->sig_len = 1;
  HaArrayClear(day->dom_tests);
  HaArrayClear(day->eq_dom_test_indexes);
  KheSrsDayAddDomTest(day, KheSrsDomTestMake(KHE_SRS_DOM_EQ));
  HaArrayClear(day->active_today);
  HaArrayClear(day->leaf_today);
  KheSrsPartialSolnSetClear(day->partial_soln_set, srs);
  /* ***
  HpTableForEachValue(day->partial_solns_table, ps, pos)
    KheSrsPartialSolnDelete(ps, srs);
  HpTableClear(day->partial_solns_table);
  HaArrayForEach(day->partial_solns_array, ps, pos)
    KheSrsPartialSolnDelete(ps, srs);
  HaArrayClear(day->partial_solns_array);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsDayAddSteps(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs,  */
/*    KHE_SRS_STEP *preassigned_step)                                        */
/*                                                                           */
/*  Add suitable steps to day.                                               */
/*                                                                           */
/*  If true is returned, it means that a preassigned task was found; a step  */
/*  for it will be the only one added.  It is returned in *preassigned_step. */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsDayAddSteps(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_STEP *preassigned_step)
{
  int i, j, k;  KHE_TIME t;  KHE_TASK task;  KHE_SRS_TIME stime;
  KHE_SRS_STEP step;  KHE_MEET meet;
  HaArrayForEach(day->times, stime, i)
  {
    /* for each time of day */
    t = stime->time;
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(srs->etm, t);  j++ )
    {
      /* for each meet running at time */
      meet = KheEventTimetableMonitorTimeMeet(srs->etm, stime->time, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	/* for the proper root task of each task of meet */
	task = KheTaskProperRoot(KheMeetTask(meet, k));
	if( KheTaskAsstResource(task) == srs->resource )
	{
	  /* task is already assigned, so it is the only legal choice */
	  KheSrsDayClearSteps(day, srs);
	  step = KheSrsStepMake(task, srs);
	  HaArrayAddLast(stime->best_steps, step);
	  *preassigned_step = step;
	  return true;
	}
	else if( KheTaskAssignResourceCheck(task, srs->resource) )
	{
	  step = KheSrsStepMake(task, srs);
	  if( KheSrsStepFirstTime(step) == t )
	  {
	    /* add step to stime, possibly replacing an existing step */
	    KheSrsTimeAddStep(stime, step, srs);
	  }
	  else
	  {
	    /* step starts at the wrong time, so ignore it */
	    KheSrsStepDelete(step, srs);
	  }
	}
      }
    }
  }
  *preassigned_step = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayAddDomTest(KHE_SRS_DAY day, KHE_SRS_DOM_TEST dom_test)     */
/*                                                                           */
/*  Add dom_test to day.  This may change the eq_dom_test_indexes array as   */
/*  well as the dom_tests array.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayAddDomTest(KHE_SRS_DAY day, KHE_SRS_DOM_TEST dom_test)
{
  if( dom_test.type == KHE_SRS_DOM_EQ || dom_test.type == KHE_SRS_DOM_EQ_LOOSE )
    HaArrayAddLast(day->eq_dom_test_indexes, HaArrayCount(day->dom_tests));
  HaArrayAddLast(day->dom_tests, dom_test);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayDomTestsDebug(KHE_SRS_DAY day, int verbosity,              */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the dom tests of day.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayDomTestsDebug(KHE_SRS_DAY day, int verbosity,
  int indent, FILE *fp)
{
  int dom_test_freq[KHE_SRS_DOM_EQ_LOOSE + 1];  int i;
  KHE_SRS_DOM_TEST dom_test;  char buff[100];

  /* initialize dom_test_freq */
  for( i = KHE_SRS_DOM_UNUSED;  i <= KHE_SRS_DOM_EQ_LOOSE;  i++ )
    dom_test_freq[i] = 0;

  /* accumulate dom_test_freq */
  HaArrayForEach(day->dom_tests, dom_test, i)
    dom_test_freq[dom_test.type]++;

  /* display frequencies */
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ Day %s dom tests: ", KheTimeGroupId(day->time_group));
  for( i = 0;  i <= KHE_SRS_DOM_EQ_LOOSE;  i++ )
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%d %s", dom_test_freq[i],
      KheSrsDomTestShow(KheSrsDomTestMake(i), buff));
  }
  fprintf(fp, " ]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayDebug(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs,     */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of day onto fp.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayDebug(KHE_SRS_DAY day, KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  int i, pos;  KHE_SRS_TIME stime;  KHE_SRS_DOM_TEST dom_test;  char buff[100];
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Day ", indent, "");
    KheTimeGroupDebug(day->time_group, 1, -1, stderr);
    fprintf(fp, " (times %d, sig_len %d, active_today %d)\n",
      HaArrayCount(day->times), day->sig_len, HaArrayCount(day->active_today));
    HaArrayForEach(day->times, stime, i)
      KheSrsTimeDebug(stime, srs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s  dom_tests [", indent, "");
    HaArrayForEach(day->dom_tests, dom_test, i)
      fprintf(fp, "%s%s%s", i > 0 ? ", " : "",
      HaArrayContains(day->eq_dom_test_indexes, i, &pos) ? "*" : "",
      KheSrsDomTestShow(dom_test, buff));
    fprintf(fp, "]\n");
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheTimeGroupDebug(day->time_group, 1, -1, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SRS_FULL_SOLN"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_FULL_ITEM KheSrsFullItemMake(KHE_SRS_STEP step,                  */
/*    bool previously_assigned)                                              */
/*                                                                           */
/*  Make a new full item object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_SRS_FULL_ITEM KheSrsFullItemMake(KHE_SRS_STEP step,
  bool previously_assigned)
{
  KHE_SRS_FULL_ITEM res;
  res.step = step;
  res.previously_assigned = previously_assigned;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_FULL_SOLN KheSrsFullSolnMake(KHE_SINGLE_RESOURCE_SOLVER srs)     */
/*                                                                           */
/*  Make a new, empty full soln object.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_FULL_SOLN KheSrsFullSolnMake(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_FULL_SOLN res;
  HaMake(res, srs->arena);
  res->asst_count = 0;
  HaArrayInit(res->steps, srs->arena);
  res->resource = NULL;
  res->adopted = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsFullSolnLoad(KHE_SRS_FULL_SOLN fs, KHE_SRS_PARTIAL_SOLN ps)   */
/*                                                                           */
/*  Load ps into fs.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheSrsFullSolnLoad(KHE_SRS_FULL_SOLN fs, KHE_SRS_PARTIAL_SOLN ps,
  KHE_RESOURCE r)
{
  /* KHE_SRS_FULL_ITEM item; */
  HaArrayClear(fs->steps);
  fs->resource = r;
  fs->asst_count = HaArrayFirst(ps->signature);
  while( ps->step != NULL )
  {
    /* ***
    item = KheSrsFullItemMake(ps->step,
      ps->step->task != NULL && KheTaskAsstResource(ps->step->task) != NULL);
    HaArrayAddLast(fs->items, item);
    *** */
    HaArrayAddLast(fs->steps, ps->step);
    ps = ps->prev_ps;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsFullSolnAdopt(KHE_SRS_FULL_SOLN fs)                           */
/*                                                                           */
/*  Adopt fs.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheSrsFullSolnAdopt(KHE_SRS_FULL_SOLN fs)
{
  /* KHE_SRS_FULL_ITEM item; */  int i;  KHE_SRS_STEP step;
  HnAssert(!fs->adopted, "KheSrsFullSolnAdopt: already adopted");
  HaArrayForEach(fs->steps, step, i)
    if( step->task != NULL && !step->already_assigned )
    {
      if( !KheTaskAssignResource(step->task, fs->resource) )
      {
	KheTaskDebug(step->task, 1, 0, stderr);
	HnAbort("KheSrsFullSolnAdopt error (cannot assign step %d to %s)",
	  i, KheResourceId(fs->resource));
      }
      if( DEBUG6 )
      {
	fprintf(stderr, "  KheSrsFullSolnAdopt assigning %s to ",
	  KheResourceId(fs->resource));
	KheTaskDebug(step->task, 1, 0, stderr);
      }
    }
  fs->adopted = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsFullSolnUnAdopt(KHE_SRS_FULL_SOLN fs)                         */
/*                                                                           */
/*  Undo the matching call to KheSrsFullSolnAdopt.                           */
/*                                                                           */
/*****************************************************************************/

static void KheSrsFullSolnUnAdopt(KHE_SRS_FULL_SOLN fs)
{
  /* KHE_SRS_FULL_ITEM item; */  int i;  KHE_SRS_STEP step;
  HnAssert(fs->adopted, "KheSrsFullSolnUnAdopt: not adopted");
  HaArrayForEach(fs->steps, step, i)
    if( step->task != NULL && !step->already_assigned )
    {
      if( !KheTaskUnAssignResource(step->task) )
	HnAbort("KheSrsFullSolnAdopt internal error");
    }
  fs->adopted = false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_PARTIAL_SOLN KheFullSolnReRun(KHE_SRS_FULL_SOLN fs,              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Rerun fs, returning the resulting partial solution.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_SRS_PARTIAL_SOLN KheFullSolnReRun(KHE_SRS_FULL_SOLN fs,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i, di /* , first_day_index, last_day_index */;  KHE_INTERVAL in;
  KHE_SRS_PARTIAL_SOLN ps;  KHE_TIME_GROUP tg;  KHE_SRS_STEP step;
  HnAssert(!fs->adopted, "KheFullSolnReRun: already adopted");
  srs->rerun = true;
  ps = KheSrsPartialSolnMakeInit(srs);
  HaArrayForEachReverse(fs->steps, step, i)
  {
    in = KheDailyScheduleInterval(step->daily_schedule);
    /* ***
    first_day_index = KheDailyScheduleFirstDayIndex(step->daily_schedule);
    last_day_index = KheDailyScheduleLastDayIndex(step->daily_schedule);
    *** */
    for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
    {
      tg = KheFrameTimeGroup(srs->days_frame, di);
      fprintf(stderr, "  starting day %s\n", KheTimeGroupId(tg));
      ps = KheSrsPartialSolnMake(ps, step, HaArray(srs->days, di), srs);
    }
  }
  srs->rerun = false;
  return ps;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsFullSolnDebug(KHE_SRS_FULL_SOLN fs, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of fs onto fp.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheSrsFullSolnDebug(KHE_SRS_FULL_SOLN fs, int verbosity,
  int indent, FILE *fp)
{
  /* KHE_SRS_FULL_ITEM item; */  int i;  KHE_SRS_STEP step;
  fprintf(fp, "%*s[ Full Soln (%d assts%s) for %s:\n", indent, "",
    fs->asst_count, fs->adopted ? ", adopted" : "",KheResourceId(fs->resource));
  HaArrayForEachReverse(fs->steps, step, i)
    KheSrsStepDebug(step, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitor handling"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddAvoidClashesMonitor(                      */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_AVOID_CLASHES_MONITOR m)           */
/*                                                                           */
/*  Add avoid clashes monitor m to srs.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddAvoidClashesMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_AVOID_CLASHES_MONITOR m)
{
  /* ignoring this; there will be no clashes */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddAvoidUnavailableTimesMonitor(             */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m) */
/*                                                                           */
/*  Add avoid unavailable times monitor m to srs.                            */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddAvoidUnavailableTimesMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;  KHE_SRS_EXPR_BUSY_TIME eb;
  int i;  KHE_TIME t;  KHE_SRS_EXPR_COST ec;  KHE_COST_FUNCTION cost_fn;
  KHE_TIME_GROUP tg;  KHE_SRS_EXPR_INT_SUM eis;  KHE_COST combined_weight;
  KHE_SRS_DOM_TEST unused_dom_test, le_dom_test;
  c = KheAvoidUnavailableTimesMonitorConstraint(m);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(c);
  unused_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) m);
  if( combined_weight > 0 && KheTimeGroupTimeCount(tg) > 0 )
  {
    cost_fn = KheMonitorCostFunction((KHE_MONITOR) m);
    if( cost_fn == KHE_LINEAR_COST_FUNCTION || KheTimeGroupTimeCount(tg) == 1 )
    {
      /* a separate expression tree for each time */
      for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
      {
	t = KheTimeGroupTime(tg, i);
	ec = KheSrsExprCostMake((KHE_MONITOR) m, cost_fn, combined_weight, srs);
	HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) ec);
	/* ***
	KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) ec);
	*** */
	eb = KheSrsExprBusyTimeMake(t, unused_dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) ec, (KHE_SRS_EXPR) eb);
      }
    }
    else
    {
      /* a single expression tree (max limit of 0, in effect) */
      ec = KheSrsExprCostMake((KHE_MONITOR) m, cost_fn, combined_weight, srs);
      HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) ec);
      /* KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) ec);*/
      le_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_LE);
      eis = KheSrsExprIntSumMake(le_dom_test, srs);
      KheSrsExprAddChild((KHE_SRS_EXPR) ec, (KHE_SRS_EXPR) eis);
      for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
      {
	t = KheTimeGroupTime(tg, i);
	eb = KheSrsExprBusyTimeMake(t, unused_dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) eis, (KHE_SRS_EXPR) eb);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddLimitIdleTimesMonitor(                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_IDLE_TIMES_MONITOR m)        */
/*                                                                           */
/*  Add limit idle times monitor m to srs.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddLimitIdleTimesMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  /* not included in the solve, and that fact is documented */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupContainsWholeDay(KHE_TIME_GROUP tg, int start_index,    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP *day_tg)                */
/*                                                                           */
/*  If tg contains a complete day's worth of times, starting from            */
/*  start_index, then return true and set *day_tg to the day time group,     */
/*  otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupContainsWholeDay(KHE_TIME_GROUP tg, int start_index,
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP *day_tg)
{
  KHE_TIME tg_t, day_t;  int i;  KHE_TIME_GROUP d_tg;
  tg_t = KheTimeGroupTime(tg, start_index);
  d_tg = KheFrameTimeTimeGroup(srs->days_frame, tg_t);
  for( i = 0;  i < KheTimeGroupTimeCount(d_tg);  i++ )
  {
    day_t = KheTimeGroupTime(d_tg, i);
    if( start_index + i >= KheTimeGroupTimeCount(tg) )
      return *day_tg = NULL, false;
    tg_t = KheTimeGroupTime(tg, start_index + i);
    if( day_t != tg_t )
      return *day_tg = NULL, false;
  }
  return *day_tg = d_tg, true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMakeOrNodeAndChildren(KHE_SRS_EXPR prnt, KHE_TIME_GROUP tg,      */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make an OR node with one BUSY child for each time of tg, and add the     */
/*  OR node as a child of prnt.  If tg has exactly one time, optimize by     */
/*  adding just the BUSY node for that time as a child of prnt.              */
/*                                                                           */
/*  Either way, add dom_test to the root of these vertices.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMakeOrNodeAndChildren(KHE_SRS_EXPR prnt, KHE_TIME_GROUP tg,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_OR eo;  KHE_SRS_EXPR_BUSY_TIME ebt;  int i;  KHE_TIME t;
  KHE_SRS_DOM_TEST unused_dom_test;  KHE_SRS_EXPR_BUSY_DAY ebd;
  KHE_TIME_GROUP day_tg;
  if( KheTimeGroupTimeCount(tg) == 1 )
  {
    /* just a single BUSY_TIME node */
    t = KheTimeGroupTime(tg, 0);
    ebt = KheSrsExprBusyTimeMake(t, dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) ebt);
  }
  else if( KheTimeGroupContainsWholeDay(tg, 0, srs, &day_tg) &&
    KheTimeGroupTimeCount(tg) == KheTimeGroupTimeCount(day_tg) )
  {
    /* just a single BUSY_DAY node */
    ebd = KheSrsExprBusyDayMake(day_tg, dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) ebd);
  }
  else
  {
    /* an OR node with BUSY_DAY and BUSY_TIME children */
    eo = KheSrsExprOrMake(dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) eo);
    unused_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      if( KheTimeGroupContainsWholeDay(tg, i, srs, &day_tg) )
      {
	/* add a BUSY_DAY node */
	ebd = KheSrsExprBusyDayMake(day_tg, dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) eo, (KHE_SRS_EXPR) ebd);
	i += KheTimeGroupTimeCount(day_tg) - 1;
      }
      else
      {
	/* add a BUSY_TIME node */
	t = KheTimeGroupTime(tg, i);
	ebt = KheSrsExprBusyTimeMake(t, unused_dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) eo, (KHE_SRS_EXPR) ebt);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMakeAndNodeAndChildren(KHE_SRS_EXPR prnt, KHE_TIME_GROUP tg,     */
/*    KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)             */
/*                                                                           */
/*  Make an AND node with one FREE child for each time of tg, and add the    */
/*  AND node as a child of prnt.  If tg has exactly one time, optimize by    */
/*  adding just the FREE node for that time as a child of prnt.              */
/*                                                                           */
/*  Either way, add dom_test to the root of these vertices.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMakeAndNodeAndChildren(KHE_SRS_EXPR prnt, KHE_TIME_GROUP tg,
  KHE_SRS_DOM_TEST dom_test, KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_SRS_EXPR_AND ea;  KHE_SRS_EXPR_FREE_TIME eft;  int i;  KHE_TIME t;
  KHE_SRS_DOM_TEST unused_dom_test;  KHE_SRS_EXPR_FREE_DAY efd;
  KHE_TIME_GROUP day_tg;
  if( KheTimeGroupTimeCount(tg) == 1 )
  {
    /* just a single FREE node */
    t = KheTimeGroupTime(tg, 0);
    eft = KheSrsExprFreeTimeMake(t, dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) eft);
  }
  else if( KheTimeGroupContainsWholeDay(tg, 0, srs, &day_tg) &&
    KheTimeGroupTimeCount(tg) == KheTimeGroupTimeCount(day_tg) )
  {
    /* just a single FREE_DAY node */
    efd = KheSrsExprFreeDayMake(day_tg, dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) efd);
  }
  else
  {
    /* an AND node with FREE_DAY and FREE_TIME children */
    ea = KheSrsExprAndMake(dom_test, srs);
    KheSrsExprAddChild(prnt, (KHE_SRS_EXPR) ea);
    unused_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      if( KheTimeGroupContainsWholeDay(tg, i, srs, &day_tg) )
      {
	/* add a FREE_DAY node */
	efd = KheSrsExprFreeDayMake(day_tg, dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) ea, (KHE_SRS_EXPR) efd);
	i += KheTimeGroupTimeCount(day_tg) - 1;
      }
      else
      {
	/* add a FREE_TIME node */
	t = KheTimeGroupTime(tg, i);
	eft = KheSrsExprFreeTimeMake(t, unused_dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) ea, (KHE_SRS_EXPR) eft);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitsToTests(int min_limit, int max_limit, int max_max_limit,   */
/*    bool allow_zero, KHE_SRS_DOM_TEST *alpha_dom_test,                     */
/*    KHE_SRS_DOM_TEST *beta_dom_test)                                       */
/*                                                                           */
/*  Convert limits (min_limit, max_limit, allow_zero) into dominance tests.  */
/*  if min_limit <= 0 there is no min limit; if max_limit >= max_max_limit   */
/*  there is no maximum limit.                                               */
/*                                                                           */
/*  The values returned in *alpha_dom_test and *beta_dom_test are referred   */
/*  to as alpha and beta in the User's Guide.                                */
/*                                                                           */
/*  Note.  If there is a trivial allow_zero flag (if min_limit <= 1), an     */
/*  adjustment will have already been made by now to remove it.  So this     */
/*  call may assume that allow_zero implies min_limit > 1.                   */
/*                                                                           */
/*****************************************************************************/

static void KheLimitsToTests(int min_limit, int max_limit, int max_max_limit,
  bool allow_zero, KHE_SRS_DOM_TEST *alpha_dom_test,
  KHE_SRS_DOM_TEST *beta_dom_test)
{
  if( allow_zero /* && min_limit > 1 */ )
  {
    /* fall back on equality when there is a non-trivial allow_zero flag */
    *alpha_dom_test = *beta_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_EQ);
  }
  else if( max_limit >= max_max_limit )
  {
    if( min_limit <= 0 )
    {
      /* no max_limit, and no min_limit (should never happen) */
      HnAbort("KheLimitsToTests: no max limit, no min limit");
      *alpha_dom_test = *beta_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
    }
    else
    {
      /* no max_limit, and min_limit */
      *alpha_dom_test = KheSrsDomTestMakeLoose(KHE_SRS_DOM_GE_LOOSE, min_limit);
      *beta_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_GE);
    }
  }
  else
  {
    if( min_limit <= 0 )
    {
      /* max_limit, and no min_limit */
      *alpha_dom_test = *beta_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_LE);
    }
    else
    {
      /* max_limit, and min_limit */
      *alpha_dom_test = KheSrsDomTestMakeLoose(KHE_SRS_DOM_EQ_LOOSE, min_limit);
      *beta_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_EQ);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsClusterMonitorContainsWholePositiveDay(                       */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int start_index,                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP *day_tg)                */
/*                                                                           */
/*  If m contains a sequence of singleton time groups starting at day_index  */
/*  that have postive polarity and together cover a complete day, return     */
/*  true and set *day_tg to the day time group;  otherwise return false.     */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsClusterMonitorContainsWholePositiveDay(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int start_index,
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_TIME_GROUP *day_tg)
{
  KHE_TIME day_t;  int i, tg_count;  KHE_TIME_GROUP m_tg, d_tg;
  KHE_POLARITY po;  KHE_TIME t;
  tg_count = KheClusterBusyTimesMonitorTimeGroupCount(m);
  m_tg = KheClusterBusyTimesMonitorTimeGroup(m, start_index, &po);
  if( po == KHE_POSITIVE && KheTimeGroupTimeCount(m_tg) == 1 )
  {
    t = KheTimeGroupTime(m_tg, 0);
    d_tg = KheFrameTimeTimeGroup(srs->days_frame, t);
    for( i = 0;  i < KheTimeGroupTimeCount(d_tg);  i++ )
    {
      day_t = KheTimeGroupTime(d_tg, i);
      if( start_index + i >= tg_count )
	return *day_tg = NULL, false;
      m_tg = KheClusterBusyTimesMonitorTimeGroup(m, start_index + i, &po);
      if( po != KHE_POSITIVE || KheTimeGroupTimeCount(m_tg) != 1 ||
	  KheTimeGroupTime(m_tg, 0) != day_t )
	return *day_tg = NULL, false;
    }
    return *day_tg = d_tg, true;
  }
  else
    return *day_tg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddClusterBusyTimesMonitor(                  */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_CLUSTER_BUSY_TIMES_MONITOR m)      */
/*                                                                           */
/*  Add cluster busy times monitor m to srs.                                 */
/*                                                                           */
/*  Implementation note.  If m contains a sequence of consecutive            */
/*  singleton time groups with positive polarity that add up to a            */
/*  complete day, then one BUSY_DAY node is generated for them instead       */
/*  of one BUSY_TIME node for each.  This is correct the sum reported        */
/*  in either case is 1 if the day is busy and 0 otherwise.  It cannot       */
/*  be more than 1 because the resource takes at most one task each day.     */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddClusterBusyTimesMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg, day_tg;
  KHE_COST_FUNCTION cost_fn;  KHE_COST combined_weight;  bool allow_zero;
  int i, min_limit, max_limit, history_before, history_after, tg_count;
  KHE_SRS_EXPR_INT_SUM_COMB eisc;  KHE_SRS_EXPR_BUSY_DAY ebd;
  KHE_POLARITY po;  KHE_SRS_DOM_TEST alpha_dom_test, beta_dom_test;

  /* boilerplate, and return if constraint does nothing */
  c = KheClusterBusyTimesMonitorConstraint(m);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) m);
  cost_fn = KheMonitorCostFunction((KHE_MONITOR) m);
  min_limit = KheClusterBusyTimesConstraintMinimum(c);
  max_limit = KheClusterBusyTimesConstraintMaximum(c);
  allow_zero = KheClusterBusyTimesConstraintAllowZero(c);
  history_before = KheClusterBusyTimesMonitorHistory(m);
  history_after = KheClusterBusyTimesMonitorHistoryAfter(m);
  if( allow_zero && min_limit <= 1 )
    min_limit = 0, allow_zero = false;
  tg_count = KheClusterBusyTimesConstraintTimeGroupCount(c);
  if( (min_limit <= 0 && max_limit >= tg_count) || combined_weight == 0 )
    return;

  /* work out the various dom_tests */
  KheLimitsToTests(min_limit, max_limit, tg_count, allow_zero,
    &alpha_dom_test, &beta_dom_test);

  /* one int_cost_comb node at top of tree */
  eisc = KheSrsExprIntSumCombMake((KHE_MONITOR) m, cost_fn, combined_weight,
    min_limit, max_limit, allow_zero, history_before, history_after,
    alpha_dom_test, srs);
  HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) eisc);
  /* KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) eisc); */

  /* one OR or AND node for each time group */
  /* or a BUSY_DAY or FREE_DAY node if all singles of one day present */
  for( i = 0;  i < tg_count;  i++ )
  {
    if( KheSrsClusterMonitorContainsWholePositiveDay(m, i, srs, &day_tg) )
    {
      /* add a BUSY_DAY node */
      ebd = KheSrsExprBusyDayMake(day_tg, beta_dom_test, srs);
      KheSrsExprAddChild((KHE_SRS_EXPR) eisc, (KHE_SRS_EXPR) ebd);
      i += KheTimeGroupTimeCount(day_tg) - 1;
    }
    else
    {
      tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
      if( po == KHE_POSITIVE )
      {
	/* make an OR node with one BUSY node for each time of tg */
	KheMakeOrNodeAndChildren((KHE_SRS_EXPR) eisc, tg, beta_dom_test, srs);
      }
      else
      {
	/* make an AND node with one FREE node for each time of tg */
	KheMakeAndNodeAndChildren((KHE_SRS_EXPR) eisc, tg, beta_dom_test, srs);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddLimitBusyTimesMonitor(                    */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_BUSY_TIMES_MONITOR m)        */
/*                                                                           */
/*  Add limit busy times monitor m to srs.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddLimitBusyTimesMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg, day_tg;
  KHE_COST_FUNCTION cost_fn;  KHE_COST combined_weight;
  int i, j, junk, min_limit, max_limit;  bool allow_zero;
  KHE_SRS_EXPR_INT_SUM_COMB eisc;  KHE_TIME t;
  KHE_SRS_EXPR_INT_SUM eis, eis2;  KHE_SRS_EXPR_COST ec;
  KHE_SRS_EXPR_INT_DEV eid;
  KHE_SRS_EXPR_BUSY_TIME ebt;
  KHE_SRS_DOM_TEST unused_dom_test, le_dom_test, alpha_dom_test, beta_dom_test;

  /* boilerplate, and return if constraint does nothing */
  c = KheLimitBusyTimesMonitorConstraint(m);
  cost_fn = KheMonitorCostFunction((KHE_MONITOR) m);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) m);
  min_limit = KheLimitBusyTimesConstraintMinimum(c);
  max_limit = KheLimitBusyTimesConstraintMaximum(c);
  allow_zero = KheLimitBusyTimesConstraintAllowZero(c);
  if( allow_zero && min_limit <= 1 )
    min_limit = 0, allow_zero = false;
  unused_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  le_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_LE);
  if( combined_weight == 0 )
    return;

  if( cost_fn == KHE_LINEAR_COST_FUNCTION ||
      KheLimitBusyTimesMonitorTimeGroupCount(m) == 1 )
  {
    /* one INT_SUM_COMB for each time group */
    /* unless the time group is one day, min_limit = 0, and max_limit = 1 */
    for( i = 0;  i < KheLimitBusyTimesMonitorTimeGroupCount(m);  i++ )
    {
      tg = KheLimitBusyTimesMonitorTimeGroup(m, i, &junk);
      if( min_limit == 0 && max_limit == 1 &&
	  KheTimeGroupContainsWholeDay(tg, 0, srs, &day_tg) &&
	  KheTimeGroupTimeCount(tg) == KheTimeGroupTimeCount(day_tg) )
      {
	/* constraint specifies at most one task per day; trivial, so omit */
      }
      else if( min_limit > 0 || max_limit < KheTimeGroupTimeCount(tg) )
      {
	KheLimitsToTests(min_limit, max_limit, KheTimeGroupTimeCount(tg),
	  allow_zero, &alpha_dom_test, &beta_dom_test);
	eisc = KheSrsExprIntSumCombMake((KHE_MONITOR) m, cost_fn,
	  combined_weight, min_limit, max_limit, allow_zero, 0, 0,
	  alpha_dom_test, srs);
	HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) eisc);
	/* ***
	KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) eisc);
	*** */
	for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	{
	  t = KheTimeGroupTime(tg, j);
	  ebt = KheSrsExprBusyTimeMake(t, unused_dom_test, srs);
	  KheSrsExprAddChild((KHE_SRS_EXPR) eisc, (KHE_SRS_EXPR) ebt);
	}
      }
    }
  }
  else
  {
    /* one COST and INT_SUM at root of tree */
    ec = KheSrsExprCostMake((KHE_MONITOR) m, cost_fn, combined_weight, srs);
    HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) ec);
    /* KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) ec); */
    eis = KheSrsExprIntSumMake(le_dom_test, srs);
    KheSrsExprAddChild((KHE_SRS_EXPR) ec, (KHE_SRS_EXPR) eis);

    /* one INT_DEV and INT_SUM for each time group */
    for( i = 0;  i < KheLimitBusyTimesMonitorTimeGroupCount(m);  i++ )
    {
      tg = KheLimitBusyTimesMonitorTimeGroup(m, i, &junk);
      eid = KheSrsExprIntDevMake(min_limit, max_limit, allow_zero, srs);
      KheSrsExprAddChild((KHE_SRS_EXPR) eis, (KHE_SRS_EXPR) eid);
      KheLimitsToTests(min_limit, max_limit, KheTimeGroupTimeCount(tg),
	allow_zero, &alpha_dom_test, &beta_dom_test);
      eis2 = KheSrsExprIntSumMake(alpha_dom_test, srs);
      KheSrsExprAddChild((KHE_SRS_EXPR) eid, (KHE_SRS_EXPR) eis2);
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
      {
	t = KheTimeGroupTime(tg, j);
	ebt = KheSrsExprBusyTimeMake(t, unused_dom_test, srs);
	KheSrsExprAddChild((KHE_SRS_EXPR) eis2, (KHE_SRS_EXPR) ebt);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddLimitWorkloadMonitor(                     */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_WORKLOAD_MONITOR m)          */
/*                                                                           */
/*  Add limit workload monitor m to srs.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddLimitWorkloadMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_LIMIT_WORKLOAD_CONSTRAINT c;
  KHE_COST_FUNCTION cost_fn;  KHE_COST combined_weight;
  int i, j, min_limit, max_limit;  bool allow_zero;  float junk;
  KHE_TIME_GROUP tg;  KHE_TIME t;  KHE_SRS_EXPR_WORK_TIME ew;
  KHE_SRS_EXPR_INT_SUM eis;  KHE_SRS_EXPR_COST ec;
  KHE_SRS_EXPR_FLOAT_SUM efs;  KHE_SRS_EXPR_FLOAT_DEV efd;
  KHE_SRS_DOM_TEST unused_dom_test, le_dom_test, alpha_dom_test, beta_dom_test;

  /* boilerplate, and return if constraint does nothing */
  c = KheLimitWorkloadMonitorConstraint(m);
  cost_fn = KheMonitorCostFunction((KHE_MONITOR) m);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) m);
  min_limit = KheLimitWorkloadConstraintMinimum(c);
  max_limit = KheLimitWorkloadConstraintMaximum(c);
  allow_zero = KheLimitWorkloadConstraintAllowZero(c);
  if( allow_zero && min_limit <= 1 )
    min_limit = 0, allow_zero = false;
  unused_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_UNUSED);
  le_dom_test = KheSrsDomTestMake(KHE_SRS_DOM_LE);
  if( combined_weight == 0 )
    return;

  /* one COST and INT_SUM at the root */
  ec = KheSrsExprCostMake((KHE_MONITOR) m, cost_fn, combined_weight, srs);
  HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) ec);
  /* KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) ec); */
  eis = KheSrsExprIntSumMake(le_dom_test, srs);
  KheSrsExprAddChild((KHE_SRS_EXPR) ec, (KHE_SRS_EXPR) eis);

  /* one FLOAT_DEV and FLOAT_SUM for each time group */
  KheLimitsToTests(min_limit, max_limit, INT_MAX, allow_zero,
    &alpha_dom_test, &beta_dom_test);
  for( i = 0;  i < KheLimitWorkloadMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheLimitWorkloadMonitorTimeGroup(m, i, &junk);
    efd = KheSrsExprFloatDevMake(min_limit, max_limit, allow_zero, srs);
    KheSrsExprAddChild((KHE_SRS_EXPR) eis, (KHE_SRS_EXPR) efd);
    efs = KheSrsExprFloatSumMake(alpha_dom_test, srs);
    KheSrsExprAddChild((KHE_SRS_EXPR) efd, (KHE_SRS_EXPR) efs);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      ew = KheSrsExprWorkTimeMake(t, unused_dom_test, srs);
      KheSrsExprAddChild((KHE_SRS_EXPR) efs, (KHE_SRS_EXPR) ew);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddLimitActiveIntervalsMonitor(              */
/*    KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)  */
/*                                                                           */
/*  Add limit active intervals monitor m to srs.                             */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddLimitActiveIntervalsMonitor(
  KHE_SINGLE_RESOURCE_SOLVER srs, KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  KHE_COST_FUNCTION cost_fn;  KHE_COST combined_weight;
  int i, min_limit, max_limit, history_before, history_after, tg_count;
  KHE_SRS_EXPR_INT_SEQ_COMB eisc;
  KHE_SRS_DOM_TEST alpha_dom_test, beta_dom_test;

  /* boilerplate, and return if constraint does nothing */
  c = KheLimitActiveIntervalsMonitorConstraint(m);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) m);
  cost_fn = KheMonitorCostFunction((KHE_MONITOR) m);
  min_limit = KheLimitActiveIntervalsConstraintMinimum(c);
  max_limit = KheLimitActiveIntervalsConstraintMaximum(c);
  history_before = KheLimitActiveIntervalsMonitorHistory(m);
  history_after = KheLimitActiveIntervalsMonitorHistoryAfter(m);
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  if( (min_limit <= 1 && max_limit >= tg_count) || combined_weight == 0 )
    return;

  /* work out the various dom_tests */
  KheLimitsToTests(min_limit, max_limit, tg_count, false,
    &alpha_dom_test, &beta_dom_test);

  /* one int_seq_comb node at top of tree */
  eisc = KheSrsExprIntSeqCombMake((KHE_MONITOR) m, cost_fn, combined_weight,
    min_limit, max_limit, history_before, history_after, alpha_dom_test, srs);
  HaArrayAddLast(srs->exprs, (KHE_SRS_EXPR) eisc);
  /* KheSrsExprAddChild((KHE_SRS_EXPR) srs->root_expr, (KHE_SRS_EXPR) eisc); */

  /* one OR or AND node for each time group */
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(m, i, &po);
    if( po == KHE_POSITIVE )
    {
      /* make an OR node with one BUSY node for each time of tg */
      KheMakeOrNodeAndChildren((KHE_SRS_EXPR) eisc, tg, beta_dom_test, srs);
    }
    else
    {
      /* make an AND node with one FREE node for each time of tg */
      KheMakeAndNodeAndChildren((KHE_SRS_EXPR) eisc, tg, beta_dom_test, srs);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddMonitor(KHE_SINGLE_RESOURCE_SOLVER srs,   */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Add m to srs.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverAddMonitor(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_MONITOR m)
{
  switch( KheMonitorTag(m) )
  {
    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheSingleResourceSolverAddAvoidClashesMonitor(srs,
        (KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheSingleResourceSolverAddAvoidUnavailableTimesMonitor(srs,
        (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheSingleResourceSolverAddLimitIdleTimesMonitor(srs,
        (KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheSingleResourceSolverAddClusterBusyTimesMonitor(srs,
        (KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheSingleResourceSolverAddLimitBusyTimesMonitor(srs,
        (KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheSingleResourceSolverAddLimitWorkloadMonitor(srs,
        (KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheSingleResourceSolverAddLimitActiveIntervalsMonitor(srs,
        (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    default:

      HnAbort("KheSingleResourceSolverAddMonitor internal error (tag %d)",
        KheMonitorTag(m));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SINGLE_RESOURCE_SOLVER KheSingleResourceSolverMake(KHE_SOLN soln,    */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Make and return a new single resource solver with these attributes.      */
/*                                                                           */
/*****************************************************************************/

KHE_SINGLE_RESOURCE_SOLVER KheSingleResourceSolverMake(KHE_SOLN soln,
  KHE_OPTIONS options)
{
  KHE_SINGLE_RESOURCE_SOLVER res;  HA_ARENA a;
  int i;  KHE_FRAME days_frame;  KHE_EVENT_TIMETABLE_MONITOR etm;

  /* get the common frame and event timetable monitor; return NULL if can't */
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  if( days_frame == NULL || etm == NULL )
    return NULL;

  /* make the basic object and initialize the boilerplate fields */
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->options = options;
  res->days_frame = days_frame;
  res->etm = etm;
  res->task_finder = KheTaskFinderMake(soln, options, a);

  /* initialize the fields that vary with the resource and solve */
  /* HaArrayInit(res->cost_limits, a); */
  res->cluster_solver = KheClusterMinimumSolverMake(a);
  res->diversifier = 0;
  res->resource = NULL;
  res->dom_kind = KHE_SRS_DOM_STRONG;
  res->min_assts = res->max_assts = 0;
  res->cost_limit = KheCost(0, 0);
  HaArrayInit(res->exprs, a);
  /* res->root_expr = KheSrsExprCostSumMake(res); */
  /* res->min_cost_reduction = KheCost(0, 0); */
  HaArrayInit(res->days, a);
  for( i = 0;  i < KheFrameTimeGroupCount(days_frame);  i++ )
    HaArrayAddLast(res->days, KheSrsDayMake(i, res));
  HaArrayInit(res->final_solns, a);
  res->full_soln = KheSrsFullSolnMake(res);
  res->rerun = false;
  /* ***
  res->adopted_index = -1;
  HaArrayInit(res->adopted_tasks, a);
  *** */

  /* initialize the free lists */
  HaArrayInit(res->free_expr_busy_time, a);
  HaArrayInit(res->free_expr_free_time, a);
  HaArrayInit(res->free_expr_work_time, a);
  HaArrayInit(res->free_expr_busy_day, a);
  HaArrayInit(res->free_expr_free_day, a);
  HaArrayInit(res->free_expr_work_day, a);
  HaArrayInit(res->free_expr_or, a);
  HaArrayInit(res->free_expr_and, a);
  HaArrayInit(res->free_expr_int_sum, a);
  /* HaArrayInit(res->free_expr_int_seq, a); */
  HaArrayInit(res->free_expr_float_sum, a);
  HaArrayInit(res->free_expr_int_dev, a);
  HaArrayInit(res->free_expr_float_dev, a);
  HaArrayInit(res->free_expr_cost, a);
  /* HaArrayInit(res->free_expr_cost_sum, a); */
  HaArrayInit(res->free_expr_int_sum_comb, a);
  HaArrayInit(res->free_expr_int_seq_comb, a);
  HaArrayInit(res->free_steps, a);
  HaArrayInit(res->free_partial_solns, a);
  HaArrayInit(res->free_ps_sets, a);
  HaArrayInit(res->free_ps_tries, a);

#if TESTING
  res->timer = KheTimerMake("single", KHE_NO_TIME, a);
  HaArrayInit(res->running_time_per_day, a);
#endif

  /* all done */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverDelete(KHE_SINGLE_RESOURCE_SOLVER srs)       */
/*                                                                           */
/*  Delete srs, reclaiming its memory.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverDelete(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KheSolnArenaEnd(srs->soln, srs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsDayAddPartialSoln(KHE_SRS_DAY day, KHE_SRS_PARTIAL_SOLN ps,   */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Add ps to day, or do nothing if ps has a hard constraint violation or    */
/*  contains too few or too many assignments.  Otherwise what to do depends  */
/*  on whether we are using strong dominance or weak dominance.              */
/*                                                                           */
/*****************************************************************************/

static void KheSrsDayAddPartialSoln(KHE_SRS_DAY day, KHE_SRS_PARTIAL_SOLN ps,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  /* KHE_SRS_PARTIAL_SOLN other_ps; */  int asst_count /* , i */;
  asst_count = HaArrayFirst(ps->signature);
  if( asst_count + HaArrayCount(srs->days) - day->day_index < srs->min_assts ||
      asst_count > srs->max_assts || ps->cost > srs->cost_limit )
  {
    /* do nothing except free ps; it is unacceptable */
    KheSrsPartialSolnDelete(ps, srs);
  }
  else
    KheSrsPartialSolnSetAddPartialSoln(day->partial_soln_set, ps, day, srs);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoExtend(KHE_SINGLE_RESOURCE_SOLVER srs,                            */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_STEP step)                            */
/*                                                                           */
/*  Extend ps by adding step to it.  Add the resulting new partial solution  */
/*  to the day object representing step's last day.                          */
/*                                                                           */
/*****************************************************************************/

static void DoExtend(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_STEP step)
{
  KHE_SRS_PARTIAL_SOLN curr_ps, last_ps;  KHE_INTERVAL in;  int di;
  /* , first_day_index, last_day_index;  KHE_SRS_DAY last_day; */
  /* KHE_TIME time;  KHE_SRS_TIME stime;  KHE_TASK task; */

  /* make one partial solution for each day of step, ending at last_ps */
  last_ps = ps;
  in = KheDailyScheduleInterval(step->daily_schedule);
  /* ***
  first_day_index = KheDailyScheduleFirstDayIndex(step->daily_schedule);
  last_day_index = KheDailyScheduleLastDayIndex(step->daily_schedule);
  *** */
  for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
    last_ps = KheSrsPartialSolnMake(last_ps, step, HaArray(srs->days, di), srs);
  HnAssert(last_ps != ps, "DoExtend internal error");

  /* delete all partial solutions between ps and last_ps exclusive */
  /* NB field curr_ps->prev is usable after calling KhePartialSolnDelete */
  for( curr_ps = last_ps->prev_ps;  curr_ps != ps;  curr_ps = curr_ps->prev_ps )
    KheSrsPartialSolnDelete(curr_ps, srs);
  last_ps->prev_ps = ps;

  /* add last_ps to last_day, and increment the diversifier */
  /* last_day = HaArray(srs->days, last_day_index); */
  KheSrsDayAddPartialSoln(last_ps->day, last_ps, srs);
  srs->diversifier++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverExtend(KHE_SINGLE_RESOURCE_SOLVER srs,       */
/*    KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_DAY day)                              */
/*                                                                           */
/*  Extend the dynamic programming search by searching out of ps.  Each      */
/*  new task starts on day.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverExtend(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_SRS_PARTIAL_SOLN ps, KHE_SRS_DAY day)
{
  int i, j;  KHE_SRS_STEP step;  KHE_SRS_TIME stime;

  /* extend for each task beginning on day */
  HaArrayForEach(day->times, stime, i)
    HaArrayForEach(stime->best_steps, step, j)
      DoExtend(srs, ps, step);

  /* extend by taking day off */
  DoExtend(srs, ps, day->free_step);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverClear(KHE_SINGLE_RESOURCE_SOLVER srs)        */
/*                                                                           */
/*  Clear srs, ready for a fresh solve.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSingleResourceSolverClear(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  int i;  KHE_SRS_DAY day;  KHE_SRS_EXPR e;
  srs->diversifier = 0;
  HaArrayForEach(srs->days, day, i)
    KheSrsDayClear(day, srs);
  HaArrayClear(srs->final_solns);
  HaArrayForEach(srs->exprs, e, i)
    KheSrsExprDelete(e, srs);
  HaArrayClear(srs->exprs);
  /* KheSrsExprCostSumDelete(srs->root_expr, srs); */
  /* srs->root_expr = KheSrsExprCostSumMake(srs); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorLimit(KHE_MONITOR m)                                       */
/*                                                                           */
/*  Return the maximum limit of m if there is one, or else the minimum       */
/*  limit if there is one, or else 0.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorLimit(KHE_MONITOR m)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  int val;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  switch( KheMonitorTag(m) )
  {
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      cbtc = KheClusterBusyTimesMonitorConstraint(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      val = KheClusterBusyTimesConstraintMaximum(cbtc);
      if( val < KheClusterBusyTimesConstraintTimeGroupCount(cbtc) )
	return val;
      val = KheClusterBusyTimesConstraintMinimum(cbtc);
      return val;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      lbtc = KheLimitBusyTimesMonitorConstraint(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      val = KheLimitBusyTimesConstraintMaximum(lbtc);
      if( val < INT_MAX )
	return val;
      val = KheLimitBusyTimesConstraintMinimum(lbtc);
      if( val < INT_MAX )
	return val;
      return 0;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      /* could do better here */
      return 0;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      val = KheLimitActiveIntervalsMonitorMaximum(laim);
      if( val < KheLimitActiveIntervalsMonitorTimeGroupCount(laim) )
	return val;
      val = KheLimitActiveIntervalsMonitorMinimum(laim);
      if( val < INT_MAX )
	return val;
      return 0;

    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    default:

      return 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorLargestLimitFirstCmp(const void *p1, const void *p2)       */
/*                                                                           */
/*  Comparison function for sorting monitors so that those with the          */
/*  largest limits come first.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorLargestLimitFirstCmp(const void *p1, const void *p2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) p1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) p2;
  return KheMonitorLimit(m2) - KheMonitorLimit(m1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsInitializeForSolving(KHE_SINGLE_RESOURCE_SOLVER srs,          */
/*    KHE_RESOURCE r, KHE_SRS_DOM_KIND dom_kind, int min_assts,              */
/*    int max_assts, KHE_COST cost_limit)                                    */
/*                                                                           */
/*  Initialize srs for a solve with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static void KheSrsInitializeForSolving(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_RESOURCE r, KHE_SRS_DOM_KIND dom_kind, int min_assts,
  int max_assts, KHE_COST cost_limit)
{
  int di, i /* , j */;  KHE_MONITOR m;  KHE_SRS_DAY day;
  /* KHE_SRS_TIME stime; */  KHE_SRS_STEP preassigned_step;  KHE_SRS_EXPR e;

  /* if there was already a resource, clear out old stuff */
  if( srs->resource != NULL )
    KheSingleResourceSolverClear(srs);

  /* add the resource, and the expression tree representing the monitors */
  srs->resource = r;
  srs->dom_kind = dom_kind;
  srs->min_assts = min_assts;
  srs->max_assts = max_assts;
  srs->cost_limit = cost_limit;
  KheSolnResourceMonitorSort(srs->soln, r, &KheMonitorLargestLimitFirstCmp);
  if( DEBUG13 )
    fprintf(stderr, "  [ monitor limits:\n");
  for( i = 0;  i < KheSolnResourceMonitorCount(srs->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(srs->soln, r, i);
    if( DEBUG13 )
    {
      fprintf(stderr, "    limit %d: ", KheMonitorLimit(m));
      KheMonitorDebug(m, 2, 0, stderr);
    }
    KheSingleResourceSolverAddMonitor(srs, m);
  }
  if( DEBUG13 )
    fprintf(stderr, "  ]\n");

  /* sort out the day info for the expression tree */
  HaArrayForEach(srs->exprs, e, i)
  {
    KheSrsExprSetDayRange(e, /* -1, */ srs);
    KheSrsExprSetSigIndexes(e, srs);
    KheSrsExprAddExpressionsToDays(e, srs);
  }
  /* ***
  KheSrsExprSetDayRange((KHE_SRS_EXPR) srs->root_expr, 0, srs);
  KheSrsExprSetSigIndexes((KHE_SRS_EXPR) srs->root_expr, srs);
  KheSrsExprAddExpressionsToDays((KHE_SRS_EXPR) srs->root_expr, srs);
  *** */

  /* add suitable tasks for each day, skipping days with preassigned tasks */
  HaArrayForEach(srs->days, day, di)
    if( KheSrsDayAddSteps(day, srs, &preassigned_step) )
      di = KheIntervalLast(KheDailyScheduleInterval(
	preassigned_step->daily_schedule));
           /* continue from after here */

  /* work out the minimum cost reduction over all selected tasks */
  /* *** still to do
  srs->min_cost_reduction = KheCost(INT_MAX, INT_MAX);
  HaArrayForEach(srs->days, day, di)
    HaArrayForEach(day->times, stime, i)
      HaArrayForEach(stime->best_steps, step, j)
        if( !step->already_assigned &&
	    step->cost_reduction < srs->min_cost_reduction )
	  srs->min_cost_reduction = step->cost_reduction;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverSolve(KHE_SINGLE_RESOURCE_SOLVER srs,        */
/*    KHE_RESOURCE r, KHE_SRS_DOM_KIND dom_kind, int min_assts,              */
/*    int max_assts, KHE_COST cost_limit)                                    */
/*                                                                           */
/*  Solve for r.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverSolve(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_RESOURCE r, KHE_SRS_DOM_KIND dom_kind, int min_assts,
  int max_assts, KHE_COST cost_limit)
{
  int di, i /* , pos, asst_count */;  KHE_SRS_DAY day, prev_day;
  KHE_SRS_PARTIAL_SOLN ps;  /* KHE_MONITOR m; */
  /* KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_CLUSTER_MINIMUM_GROUP g; */

  if( DEBUG1 )
    fprintf(stderr, "[ KheSingleResourceSolverSolve(srs, %s)\n",
      KheResourceId(r));

  /* initialize srs for solving r */
  KheSrsInitializeForSolving(srs, r, dom_kind, min_assts, max_assts,
    cost_limit);

  /* adjust r's cluster monitors */
  if( DEBUG14 )
    fprintf(stderr, "  [ KheSingleResourceSolverSolve cluster minimum solve\n");
  KheClusterMinimumSolverSolve(srs->cluster_solver, srs->soln,
    srs->options, KheResourceResourceType(r));
  KheClusterMinimumSolverSetBegin(srs->cluster_solver);
  KheClusterMinimumSolverSetMulti(srs->cluster_solver,
    KheResourceSingletonResourceGroup(r));
  /* ***
  for( i = 0;  i < KheSolnResourceMonitorCount(srs->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(srs->soln, r, i);
    if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      if( KheClusterMinimumSolverMonitorGroup(srs->cluster_solver, cbtm, &g) &&
	  KheClusterMinimumGroupDemand(g) >= KheClusterMinimumGroupSupply(g) )
      {
	if( DEBUG14 )
	{
	  fprintf(stderr, "    monitor %s min limit %d -> %d based on group:\n",
	    KheMonitorId(m), KheClusterBusyTimesMonitorMinimum(cbtm),
	    KheClusterBusyTimesMonitorMaximum(cbtm));
	  KheClusterMinimumGroupDebug(g, 2, 4, stderr);
	}
	KheClusterMinimumSolverSet(srs->cluster_solver, cbtm,
	  KheClusterBusyTimesMonitorMaximum(cbtm));
      }
    }
  }
  *** */
  if( DEBUG14 )
    fprintf(stderr, "  ] end cluster minimum solve\n");

  if( DEBUG5 )
    KheSingleResourceSolverDebug(srs, 1, 2, stderr);
  if( DEBUG10 )
    HaArrayForEach(srs->days, day, i)
      KheSrsDayDomTestsDebug(day, 1, 2, stderr);

  /* build up the partial solutions for each day */
  prev_day = NULL;
  HaArrayForEach(srs->days, day, di)
  {
    if( prev_day == NULL )
    {
      /* virtual prev_day contains a single partial solution (an empty one) */
      if( DEBUG2 )
	fprintf(stderr, "  extending from day 0 (1 partial soln)\n");
      ps = KheSrsPartialSolnMakeInit(srs);
      KheSingleResourceSolverExtend(srs, ps, day);
    }
    else
    {
      /* extend each partial solution from prev_day with tasks from day */
#if TESTING
      if( DEBUG7 )
	fprintf(stderr, "  running_time_per_day = %d, days = %d, di = %d\n",
	  HaArrayCount(srs->running_time_per_day), HaArrayCount(srs->days), di);
      HaArray(srs->running_time_per_day, di) = KheTimerElapsedTime(srs->timer);
#endif
      if( DEBUG2 )
	fprintf(stderr, "  extending from day %s (%d partial solns)\n",
	  KheTimeGroupId(prev_day->time_group),
	  prev_day->partial_soln_set->count);
      KheSrsPartialSolnSetExtend(prev_day->partial_soln_set, day, srs);
    }
    prev_day = day;
  }

  /* extract final solutions whose assignment count is in the desired range */
  day = HaArrayLast(srs->days);
  KheSrsPartialSolnSetExtractFinalSolns(day->partial_soln_set, srs);
  HaArraySort(srs->final_solns, &KheSrsPartialSolnIncreasingAsstCountCmp);

  /* undo the adjustment of r's cluster monitors */
  KheClusterMinimumSolverSetEnd(srs->cluster_solver, true);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final solutions:\n");
    HaArrayForEach(srs->final_solns, ps, i)
      KheSrsPartialSolnDebug(ps, srs, 1, 2, stderr);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheSingleResourceSolverSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAddCostCutoff(KHE_SINGLE_RESOURCE_SOLVER srs,*/
/*    KHE_COST cost_limit)                                                   */
/*                                                                           */
/*  Add cost_limit to the list of cost cutoffs.                              */
/*                                                                           */
/*****************************************************************************/

/* *** decided against this
void KheSingleResourceSolverAddCostCutoff(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_COST cost_limit)
{
  HaArrayAddLast(srs->cost_limits, cost_limit);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSingleResourceSolverTimetableCount(KHE_SINGLE_RESOURCE_SOLVER srs)*/
/*                                                                           */
/*  Return the number of timetables found.                                   */
/*                                                                           */
/*****************************************************************************/

int KheSingleResourceSolverTimetableCount(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverTimetableCount called out of order");
  return HaArrayCount(srs->final_solns);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverTimetable(KHE_SINGLE_RESOURCE_SOLVER srs,    */
/*    int i, int *asst_count, KHE_COST *r_cost, KHE_COST *soln_cost)         */
/*                                                                           */
/*  Report on the i'th timetable found by the solver.                        */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverTimetable(KHE_SINGLE_RESOURCE_SOLVER srs,
  int i, int *asst_count, KHE_COST *cost)
{
  /* make sure there is an i'th solution */
  KHE_SRS_PARTIAL_SOLN ps;
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverTimetable called out of order");
  HnAssert(0 <= i && i < HaArrayCount(srs->final_solns),
    "KheSingleResourceSolverTimetable:  i (%d) out of range (0 .. %d)",
    i, HaArrayCount(srs->final_solns) - 1);

  /* return the relevant fields of the i'th solution */
  ps = HaArray(srs->final_solns, i);
  *asst_count = HaArrayFirst(ps->signature);
  *cost = ps->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsSolnDebug(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_RESOURCE r,     */
/*    KHE_COST *cost)                                                        */
/*                                                                           */
/*  Debug print of r's timetable and costs.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSrsSolnDebug(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_RESOURCE r,
  KHE_COST *cost)
{
  int i;  KHE_COST m_cost;  KHE_MONITOR m;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(srs->soln, r);
  KheResourceTimetableMonitorPrintTimetable(rtm, srs->days_frame, 6, 2, stderr);
  *cost = KheCost(0, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(srs->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(srs->soln, r, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	m_cost = KheMonitorCost(m);
	if( m_cost > 0 )
	{
	  fprintf(stderr, "  true cost of %s is %.5f\n", KheMonitorId(m),
	    KheCostShow(m_cost));
	  *cost += m_cost;
	}
	break;

      default:

	/* ignore monitors of other types */
	break;
    }
  }
  fprintf(stderr, "  total cost is %.5f\n", KheCostShow(*cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverAdopt(KHE_SINGLE_RESOURCE_SOLVER srs, int i) */
/*                                                                           */
/*  Change srs's solution to include the i'th timetable.                     */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverAdopt(KHE_SINGLE_RESOURCE_SOLVER srs, int i)
{
  /* make sure there is an i'th solution */
  KHE_SRS_PARTIAL_SOLN final_ps, rerun_ps;  KHE_COST cost;
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverAdopt called out of order");
  HnAssert(0 <= i && i < HaArrayCount(srs->final_solns),
    "KheSingleResourceSolverAdopt:  i (%d) out of range (0 .. %d)",
    i, HaArrayCount(srs->final_solns) - 1);

  /* load the i'th solution, and adopt it */
  final_ps = HaArray(srs->final_solns, i);
  KheSrsFullSolnLoad(srs->full_soln, final_ps, srs->resource);
  KheSrsFullSolnAdopt(srs->full_soln);

  /* assign the tasks of the i'th solution */
  /* ***
  srs->adopted_index = i;
  HaArrayClear(srs->adopted_tasks);
  final_ps = HaArray(srs->final_solns, i);
  for( ps = final_ps;  ps != NULL;  ps = ps->prev_ps )
    if( ps->task != NULL && KheTaskAsstResource(ps->task) == NULL )
    {
      ** assign srs->resource to ps->task **
      if( !KheTaskAssignResource(ps->task, srs->resource) )
	HnAbort("KheSingleResourceSolverAdopt internal error");
      HaArrayAddLast(srs->adopted_tasks, ps->task);
    }
  *** */

  /* check that the resulting resource costs agree with the final soln */
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheSingleResourceSolverAdopt(srs, %d)\n", i);
    KheSrsSolnDebug(srs, srs->resource, &cost);
    if( cost != final_ps->cost )
    {
      fprintf(stderr, "KheSingleResourceSolverAdopt: cost discrepancy "
	"(true %.5f != single %.5f):\n", KheCostShow(cost),
	KheCostShow(final_ps->cost));
      KheSrsFullSolnDebug(srs->full_soln, 1, 2, stderr);
      KheSrsFullSolnUnAdopt(srs->full_soln);
      rerun_ps = KheFullSolnReRun(srs->full_soln, srs);
      fprintf(stderr, "  rerun_ps cost is %.5f\n", KheCostShow(rerun_ps->cost));
      HnAbort("aborting now (cost discrepancy)");
    }
    fprintf(stderr, "  %s assts %d, cost %.5f\n", KheResourceId(srs->resource),
      HaArrayFirst(final_ps->signature), KheCostShow(cost));
    fprintf(stderr, "] KheSingleResourceSolverAdopt returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverUnAdopt(KHE_SINGLE_RESOURCE_SOLVER srs)      */
/*                                                                           */
/*  Unadopt the most recently adopted full solution.                         */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverUnAdopt(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KheSrsFullSolnUnAdopt(srs->full_soln);
  /* ***
  KHE_TASK task;  int j;

  ** make sure the call is legal **
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverUnAdopt called out of order");
  HnAssert(0 <= i && i < HaArrayCount(srs->final_solns),
    "KheSingleResourceSolverUnAdopt:  i (%d) out of range (0 .. %d)",
    i, HaArrayCount(srs->final_solns) - 1);
  HnAssert(srs->adopted_index != -1, "KheSingleResourceSolverUnAdopt:  "
    "no previous matching call to KheSingleResourceSolverAdopt");
  HnAssert(i == srs->adopted_index, "KheSingleResourceSolverUnAdopt:  "
    "i (%d) not most recently adopted (%d)", i, srs->adopted_index);

  ** unassign the tasks that were assigned by KheSingleResourceSolverAdopt **
  HaArrayForEach(srs->adopted_tasks, task, j)
    if( !KheTaskUnAssignResource(task) )
      HnAbort("KheSingleResourceSolverUnAdopt internal error");

  ** reset adopted fields **
  HaArrayClear(srs->adopted_tasks);
  srs->adopted_index = -1;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartialSolnAsstCount(KHE_SRS_PARTIAL_SOLN ps)                     */
/*                                                                           */
/*  Return the assignment count of ps.  (First element of the signature.)    */
/*                                                                           */
/*****************************************************************************/

static int KhePartialSolnAsstCount(KHE_SRS_PARTIAL_SOLN ps)
{
  return HaArrayFirst(ps->signature);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KhePartialSolnAdjustedCost(KHE_SRS_PARTIAL_SOLN ps, KHE_COST c) */
/*                                                                           */
/*  Return the adjusted cost of ps.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KhePartialSolnAdjustedCost(KHE_SRS_PARTIAL_SOLN ps, KHE_COST c)
{
  return ps->cost - KhePartialSolnAsstCount(ps) * c;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSingleResourceSolverBest(KHE_SINGLE_RESOURCE_SOLVER srs,          */
/*    KHE_COST cost_reduction)                                               */
/*                                                                           */
/*  Return the index of the final solution which has a mminimal value of     */
/*                                                                           */
/*    r_cost - asst_count * cost_reduction                                   */
/*                                                                           */
/*  Break ties in favour of final solutions with larger asst_count values.   */
/*                                                                           */
/*****************************************************************************/

int KheSingleResourceSolverBest(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_COST cost_reduction)
{
  int best_i, i, asst_count, best_asst_count;  KHE_SRS_PARTIAL_SOLN ps, best_ps;
  KHE_COST adjusted_cost, best_adjusted_cost;
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverBest called out of order");
  HnAssert(HaArrayCount(srs->final_solns) > 0,
    "KheSingleResourceSolverBest:  there are no solutions");
  best_ps = NULL;
  best_i = -1;
  best_adjusted_cost = 0;
  best_asst_count = 0;
  HaArrayForEach(srs->final_solns, ps, i)
  {
    asst_count = KhePartialSolnAsstCount(ps);
    adjusted_cost = KhePartialSolnAdjustedCost(ps, cost_reduction);
    if( best_ps == NULL || adjusted_cost < best_adjusted_cost ||
	(adjusted_cost == best_adjusted_cost && asst_count > best_asst_count) )
    {
      best_ps = ps;
      best_i = i;
      best_adjusted_cost = adjusted_cost;
      best_asst_count = asst_count;
    }
  }
  return best_i;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSingleResourceSolverMinCostReduction(                        */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Return the minimum, over all tasks t that could have been assigned in    */
/*  the most recent solve, of the cost of not assigning t.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_COST KheSingleResourceSolverMinCostReduction(
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  HnAssert(srs->resource != NULL,
    "KheSingleResourceSolverMinCostReduction called out of order");
  HnAbort("KheSingleResourceSolverMinCostReduction still to do");
  return srs->min_cost_reduction;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverDebug(KHE_SINGLE_RESOURCE_SOLVER srs,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug printf of srs onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverDebug(KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp)
{
  KHE_SRS_DAY day;  int i;  /* KHE_SRS_EXPR e; */
  HnAssert(indent >= 0, "KheSingleResourceSolverDebug: indent < 0");
  fprintf(fp, "%*s[ SingleResourceSolver(%s)\n", indent, "",
    srs->resource == NULL ? "@" : KheResourceId(srs->resource));
  HaArrayForEach(srs->days, day, i)
    KheSrsDayDebug(day, srs, verbosity, indent + 2, fp);
  /* ***
  HaArrayForEach(srs->exprs, e, i)
    KheSrsExprDebug(e, NULL, srs, verbosity, indent + 2, fp);
  *** */
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "manual testing"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SRS_FULL_SOLN KheSrsManualSolnBegin(KHE_SINGLE_RESOURCE_SOLVER srs,  */
/*    char *resource_id)                                                     */
/*                                                                           */
/*  Begin loading a manual solution for this resource.                       */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static KHE_SRS_FULL_SOLN KheSrsManualSolnBegin(KHE_SINGLE_RESOURCE_SOLVER srs,
  char *resource_id)
{
  KHE_INSTANCE ins;  KHE_RESOURCE r;  KHE_SRS_FULL_SOLN res;
  ins = KheSolnInstance(srs->soln);
  if( !KheInstanceRetrieveResource(ins, resource_id, &r) )
    return NULL;
  res = KheSrsFullSolnMake(srs);
  res->resource = r;
  return res;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsManualSolnAddStep(KHE_SRS_FULL_SOLN fs, char *time_id,        */
/*    KHE_SINGLE_RESOURCE_SOLVER srs)                                        */
/*                                                                           */
/*  Add the best step for time_id to fs.                                     */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void KheSrsManualSolnAddStep(KHE_SRS_FULL_SOLN fs, char *time_id,
  KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_INSTANCE ins;  KHE_TIME time;  int day_index;  KHE_SRS_DAY day;
  KHE_SRS_TIME t;  int i;  KHE_SRS_STEP best_step;  KHE_INTERVAL in;
  ins = KheSolnInstance(srs->soln);
  if( !KheInstanceRetrieveTime(ins, time_id, &time) )
    HnAbort("KheSrsManualSolnAddStep:  unknown time %s", time_id);
  day_index = KheFrameTimeIndex(srs->days_frame, time);
  day = HaArray(srs->days, day_index);
  HaArrayForEach(day->times, t, i)
    if( t->time == time && HaArrayCount(t->best_steps) > 0 )
    {
      best_step = HaArrayFirst(t->best_steps);
      in = KheDailyScheduleInterval(best_step->daily_schedule);
      /* ***
      if( KheDailyScheduleFirstDayIndex(best_step->daily_schedule) ==
	  KheDailyScheduleLastDayIndex(best_step->daily_schedule) )
      *** */
      if( KheIntervalFirst(in) == KheIntervalLast(in) )
      {
	HaArrayAddLast(fs->steps, best_step);
	return;
      }
    }
  HnAbort("KheSrsManualSolnAddStep:  no best step at time %s", time_id);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsManualSolnTest1(KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Carry out a specific manual test, if the instance is INRC2-4-030-1-6291. */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void KheSrsManualSolnTest1(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_INSTANCE ins;  KHE_SRS_FULL_SOLN fs;  KHE_COST cost;
  ins = KheSolnInstance(srs->soln);
  if( strcmp(KheInstanceId(ins), "INRC2-4-030-1-6291") == 0 )
  {
    fprintf(stderr, "[ KheSrsManualSolnTest1(%s, %s)\n",
      KheInstanceId(ins), "HN_0");
    fs = KheSrsManualSolnBegin(srs, "HN_0");
    KheSrsInitializeForSolving(srs, fs->resource, false, 0, INT_MAX,
      KheCost(1, 0));
    KheSrsManualSolnAddStep(fs, "1Wed1", srs);
    KheSrsManualSolnAddStep(fs, "1Thu1", srs);
    KheSrsManualSolnAddStep(fs, "1Fri1", srs);

    KheSrsManualSolnAddStep(fs, "2Mon2", srs);
    KheSrsManualSolnAddStep(fs, "2Tue2", srs);
    KheSrsManualSolnAddStep(fs, "2Wed2", srs);
    KheSrsManualSolnAddStep(fs, "2Thu2", srs);
    KheSrsManualSolnAddStep(fs, "2Fri2", srs);

    KheSrsManualSolnAddStep(fs, "3Mon1", srs);
    KheSrsManualSolnAddStep(fs, "3Tue1", srs);
    KheSrsManualSolnAddStep(fs, "3Wed1", srs);
    KheSrsManualSolnAddStep(fs, "3Thu3", srs);
    KheSrsManualSolnAddStep(fs, "3Fri3", srs);

    KheSrsManualSolnAddStep(fs, "4Mon2", srs);
    KheSrsManualSolnAddStep(fs, "4Tue2", srs);
    KheSrsManualSolnAddStep(fs, "4Wed2", srs);
    KheSrsManualSolnAddStep(fs, "4Thu2", srs);
    KheSrsManualSolnAddStep(fs, "4Fri2", srs);
    KheSrsFullSolnAdopt(fs);
    KheSrsSolnDebug(srs, fs->resource, &cost);
    KheSrsFullSolnUnAdopt(fs);
    fprintf(stderr, "] KheSrsManualSolnTest1 returning\n");
  }
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsManualSolnTest2(KHE_SINGLE_RESOURCE_SOLVER srs)               */
/*                                                                           */
/*  Carry out a specific manual test, if the instance is INRC2-4-030-1-6291. */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void KheSrsManualSolnTest2(KHE_SINGLE_RESOURCE_SOLVER srs)
{
  KHE_INSTANCE ins;  KHE_SRS_FULL_SOLN fs;  KHE_COST cost;
  ins = KheSolnInstance(srs->soln);
  if( strcmp(KheInstanceId(ins), "INRC2-4-030-1-6291") == 0 )
  {
    fprintf(stderr, "[ KheSrsManualSolnTest2(%s, %s)\n",
      KheInstanceId(ins), "HN_0");
    fs = KheSrsManualSolnBegin(srs, "HN_0");
    KheSrsInitializeForSolving(srs, fs->resource, false, 0, INT_MAX,
      KheCost(1, 0));
    KheSrsManualSolnAddStep(fs, "1Wed1", srs);
    KheSrsManualSolnAddStep(fs, "1Thu1", srs);
    KheSrsManualSolnAddStep(fs, "1Fri1", srs);

    KheSrsManualSolnAddStep(fs, "2Mon2", srs);
    KheSrsManualSolnAddStep(fs, "2Tue2", srs);
    KheSrsManualSolnAddStep(fs, "2Wed2", srs);
    KheSrsManualSolnAddStep(fs, "2Thu2", srs);
    KheSrsManualSolnAddStep(fs, "2Fri2", srs);

    KheSrsManualSolnAddStep(fs, "3Mon1", srs);
    KheSrsManualSolnAddStep(fs, "3Tue1", srs);
    KheSrsManualSolnAddStep(fs, "3Wed1", srs);
    KheSrsManualSolnAddStep(fs, "3Thu1", srs);
    KheSrsManualSolnAddStep(fs, "3Fri1", srs);

    KheSrsManualSolnAddStep(fs, "4Mon2", srs);
    KheSrsManualSolnAddStep(fs, "4Tue2", srs);
    KheSrsManualSolnAddStep(fs, "4Wed2", srs);
    KheSrsManualSolnAddStep(fs, "4Thu2", srs);
    KheSrsManualSolnAddStep(fs, "4Fri2", srs);
    KheSrsFullSolnAdopt(fs);
    KheSrsSolnDebug(srs, fs->resource, &cost);
    KheSrsFullSolnUnAdopt(fs);
    fprintf(stderr, "] KheSrsManualSolnTest2 returning\n");
  }
}
#endif




/*****************************************************************************/
/*                                                                           */
/*  Submodule "testing"                                                      */
/*                                                                           */
/*****************************************************************************/

#if TESTING
/* ***
typedef enum {
  KHE_SRS_TEST_WEAK,
  KHE_SRS_TEST_STRONG,
  KHE_SRS_TEST_WEAK_AND_STRONG
} KHE_SRS_TEST_TYPE;
*** */

typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;
#endif


/*****************************************************************************/
/*                                                                           */
/*  char *KheSrsTestTypeShow(KHE_SRS_TEST_TYPE type)                         */
/*                                                                           */
/*  Show a test type.                                                        */
/*                                                                           */
/*****************************************************************************/

#if TESTING
/* ***
static char *KheSrsTestTypeShow(KHE_SRS_TEST_TYPE type)
{
  switch( type )
  {
    case KHE_SRS_TEST_WEAK:		return "w";
    case KHE_SRS_TEST_STRONG:		return "s";
    case KHE_SRS_TEST_WEAK_AND_STRONG:	return "ws";
    default:				return "?";
  }
}
*** */
#endif


/*****************************************************************************/
/*                                                                           */
/*  void TestName(KHE_INSTANCE ins, KHE_RESOURCE r, KHE_SRS_TEST_TYPE type,  */
/*    int min_assts, int max_assts, ARRAY_KHE_COST *cost_limits,char *suffix,*/
/*    char buff[200])                                                        */
/*  void TestName(KHE_INSTANCE ins, KHE_RESOURCE r, KHE_SRS_TEST_TYPE type,  */
/*    int min_assts, int max_assts, char *suffix, char buff[200])            */
/*                                                                           */
/*  Set buff to the name of a test with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void TestName(KHE_INSTANCE ins, KHE_RESOURCE r,
  /* KHE_SRS_TEST_TYPE type, */
  int min_assts, int max_assts, ARRAY_KHE_COST *cost_limits, char *suffix,
  char buff[200])
{
  int len, i, hard_cost_limit, soft_cost_limit;  KHE_COST cost_limit;

  /* add standard prefix to buff */
  len = snprintf(buff, 200, "%s-%s", KheInstanceId(ins),
	KheResourceId(r) /* , KheSrsTestTypeShow(type) */);

  /* add min_assts, or nothing if zero */
  if( min_assts == 0 )
    len += snprintf(&buff[len], 200 - len, "-");
  else
    len += snprintf(&buff[len], 200 - len, "-%d", min_assts);

  /* add max_assts, or nothing if INT_MAX */
  if( max_assts == INT_MAX )
    len += snprintf(&buff[len], 200 - len, "-");
  else
    len += snprintf(&buff[len], 200 - len, "-%d", max_assts);

  /* add cost_limits, or nothing if empty */
  if( HaArrayCount(*cost_limits) != 1 ||
      HaArrayFirst(*cost_limits) != KheCost(0, INT_MAX) )
  {
    HaArrayForEach(*cost_limits, cost_limit, i)
    {
      if( cost_limit == KheCost(INT_MAX, INT_MAX) )
	len += snprintf(&buff[len], 200 - len, "-inf");
      else
      {
        hard_cost_limit = KheHardCost(cost_limit);
	soft_cost_limit = KheSoftCost(cost_limit);
	if( hard_cost_limit == 0 )
	{
	  if( soft_cost_limit == INT_MAX )
	    len += snprintf(&buff[len], 200 - len, "-max");
	  else
	    len += snprintf(&buff[len], 200 - len, "-%d", soft_cost_limit);
	}
	else
          len += snprintf(&buff[len], 200 - len, "-%d,%d",
	    hard_cost_limit, soft_cost_limit);
      }
    }
  }

  /* add suffix */
  if( strcmp(suffix, "") != 0 )
    len += snprintf(&buff[len], 200 - len, "-%s", suffix);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  KHE_DATASET_POINTS_TYPE KheDomKindToPointsType(KHE_SRS_DOM_KIND dom_kind)*/
/*                                                                           */
/*  Return the dataset points type corresponding to dom_kind.                */
/*                                                                           */
/*****************************************************************************/

static KHE_DATASET_POINTS_TYPE KheDomKindToPointsType(KHE_SRS_DOM_KIND dom_kind)
{
  switch( dom_kind )
  {
    case KHE_SRS_DOM_WEAK:	return KHE_DATASET_POINTS_SQUARE;
    case KHE_SRS_DOM_MEDIUM:	return KHE_DATASET_POINTS_CIRCLE;
    case KHE_SRS_DOM_STRONG:	return KHE_DATASET_POINTS_PLUS;
    case KHE_SRS_DOM_TRIE:	return KHE_DATASET_POINTS_CROSS;

    default:

      HnAbort("KheDomKindToPointsType: unknown dom_kind (%d)", dom_kind);
      return KHE_DATASET_POINTS_CROSS;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DATASET_PAIRS_TYPE KheDomKindToPairsType(KHE_SRS_DOM_KIND dom_kind)  */
/*                                                                           */
/*  Return the dataset pairs type corresponding to dom_kind.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_DATASET_PAIRS_TYPE KheDomKindToPairsType(KHE_SRS_DOM_KIND dom_kind)
{
  switch( dom_kind )
  {
    case KHE_SRS_DOM_WEAK:	return KHE_DATASET_PAIRS_DOTTED;
    case KHE_SRS_DOM_MEDIUM:	return KHE_DATASET_PAIRS_DASHED;
    case KHE_SRS_DOM_STRONG:	return KHE_DATASET_PAIRS_SOLID;
    case KHE_SRS_DOM_TRIE:	return KHE_DATASET_PAIRS_DOT_DASHED;

    default:

      HnAbort("KheDomKindToPairsType: unknown dom_kind (%d)", dom_kind);
      return KHE_DATASET_PAIRS_SOLID;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheDomKindLabel(KHE_SRS_DOM_KIND dom_kind)                         */
/*                                                                           */
/*  Return a suitable label for dom_kind.                                    */
/*                                                                           */
/*****************************************************************************/

static char *KheDomKindLabel(KHE_SRS_DOM_KIND dom_kind)
{
  switch( dom_kind )
  {
    case KHE_SRS_DOM_WEAK:	return "Weak";
    case KHE_SRS_DOM_MEDIUM:	return "Medium";
    case KHE_SRS_DOM_STRONG:	return "Strong";
    case KHE_SRS_DOM_TRIE:	return "Trie";

    default:

      HnAbort("KheDomKindLabel: unknown dom_kind (%d)", dom_kind);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSrsPrintPoint(KHE_SINGLE_RESOURCE_SOLVER srs, int i)             */
/*                                                                           */
/*  Return true if we want to print a point with index i.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheSrsPrintPoint(KHE_SINGLE_RESOURCE_SOLVER srs, int i)
{
  return HaArrayCount(srs->days) <= 28 || i % 2 == 0 ||
    i == HaArrayCount(srs->days) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSrsSolveTest(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_RESOURCE r,     */
/*    KHE_SRS_DOM_KIND dom_kind, int min_assts, int max_assts,               */
/*    ARRAY_KHE_COST *cost_limits, KHE_GRAPH time_graph,                     */
/*    KHE_GRAPH size_graph, KHE_GRAPH cost_graph)                            */
/*                                                                           */
/*  Carry out a test with these attributes, and add a dataset to time_graph, */
/*  size_graph, and cost_graph recording the results.                        */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void KheSrsSolveTest(KHE_SINGLE_RESOURCE_SOLVER srs, KHE_RESOURCE r,
  KHE_SRS_DOM_KIND dom_kind, int min_assts, int max_assts,
  ARRAY_KHE_COST *cost_limits, KHE_GRAPH time_graph,
  KHE_GRAPH size_graph, KHE_GRAPH cost_graph)
{
  int i, best_i, asst_count;  float val;  KHE_DATASET kd;  KHE_SRS_DAY day;
  KHE_COST r_cost, cost_limit, marginal_cost;  /* KHE_BALANCE_SOLVER bs; */
  KHE_DATASET_POINTS_TYPE points_type; KHE_DATASET_PAIRS_TYPE pairs_type;
  char *label;  KHE_SRS_PARTIAL_SOLN ps;  char buff[200];
  if( DEBUG11 )
  {
    TestName(KheSolnInstance(srs->soln), r, min_assts, max_assts, cost_limits,
      KheDomKindLabel(dom_kind), buff);
    fprintf(stderr, "[ KheSrsSolveTest(%s)\n", buff);
  }

  /* solve, measuring running time */
  HaArrayClear(srs->running_time_per_day);
  HaArrayFill(srs->running_time_per_day, HaArrayCount(srs->days), 0.0);
  KheTimerResetStartTime(srs->timer);
  HaArrayForEach(*cost_limits, cost_limit, i)
  {
    KheSingleResourceSolverSolve(srs, r, dom_kind, min_assts, max_assts,
      cost_limit);
    if( KheSingleResourceSolverTimetableCount(srs) > 0 )
      break;
  }

  /* verify costs */
  for( i = 0;  i < KheSingleResourceSolverTimetableCount(srs);  i++ )
  {
    KheSingleResourceSolverAdopt(srs, i);
    KheSingleResourceSolverUnAdopt(srs);
  }

  if( DEBUG11 )
  {
    int demand, supply;  KHE_COST task_cost, resource_cost;
    /* ***
    bs = KheBalanceSolverMake(srs->soln, KheResourceResourceType(r),
      srs->days_frame, srs->arena);
    KheBalanceSolverDebug(bs, 2, 2, stderr);
    marginal_cost = KheBalanceSolverMarginalCost(bs);
    *** */
    KheResourceDemandExceedsSupply(srs->soln,
      KheResourceResourceType(srs->resource), &demand, &supply,
      &task_cost, &resource_cost);
    marginal_cost = task_cost;  /* anyway this is what it used to be */
    fprintf(stderr, "  marginal cost = %.5f\n", KheCostShow(marginal_cost));
    if( KheSingleResourceSolverTimetableCount(srs) > 0 )
    {
      best_i = KheSingleResourceSolverBest(srs, marginal_cost);
      fprintf(stderr, "  best of best has index %d\n", best_i);
    }
    HaArrayForEach(srs->final_solns, ps, i)
      fprintf(stderr, "  best soln (%d assts) has cost %.5f%s\n",
	HaArrayFirst(ps->signature), KheCostShow(ps->cost),
	i == best_i ? " (overall best)" : "");
  }

  /* sort out the points type and pairs type */
  points_type = KheDomKindToPointsType(dom_kind);
  pairs_type = KHE_DATASET_PAIRS_DASHED;
  label = KheDomKindLabel(dom_kind);

  /* add one data set to the running time graph */
  kd = KheDataSetAdd(time_graph, points_type, pairs_type, label);
  HaArrayForEach(srs->running_time_per_day, val, i)
    if( KheSrsPrintPoint(srs, i) )
      KhePointAdd(kd, (float) i + 1, val);

  /* add one data set to the table size graph */
  kd = KheDataSetAdd(size_graph, points_type, pairs_type, label);
  KhePointAdd(kd, 0.0, 0.0);
  HaArrayForEach(srs->days, day, i)
    if( KheSrsPrintPoint(srs, i) )
      KhePointAdd(kd, (float) i + 1, day->partial_soln_set->count);

  /* add one data set to the cost graph */
  kd = KheDataSetAdd(cost_graph, points_type, pairs_type, label);
  for( i = 0;  i < KheSingleResourceSolverTimetableCount(srs);  i++ )
  {
    KheSingleResourceSolverTimetable(srs, i, &asst_count, &r_cost);
    KhePointAdd(kd, (float) asst_count, (float) KheSoftCost(r_cost));
  }

  if( DEBUG11 )
    fprintf(stderr, "] KheSrsSolveTest returning\n");
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverDoTest(KHE_SINGLE_RESOURCE_SOLVER srs,       */
/*    KHE_RESOURCE r, KHE_SRS_TEST_TYPE test_type, int min_assts,            */
/*    int max_assts, ARRAY_KHE_COST *cost_limits)                            */
/*                                                                           */
/*  Do a test with these values for min_assts, max_assts, and cost_limits.   */
/*                                                                           */
/*****************************************************************************/

#if TESTING
static void KheSingleResourceSolverDoTest(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_RESOURCE r, /* KHE_SRS_TEST_TYPE test_type, */ int min_assts,
  int max_assts, ARRAY_KHE_COST *cost_limits)
{
  char time_file_name[200], size_file_name[200], cost_file_name[200];
  KHE_FILE time_file, size_file, cost_file;
  KHE_GRAPH time_graph, size_graph, cost_graph;

  /* initialize the time file and graph */
  TestName(KheSolnInstance(srs->soln), r, /* test_type, */ min_assts,
    max_assts, cost_limits, "time", time_file_name);
  time_file = KheFileBegin(time_file_name, KHE_FILE_LOUT_STANDALONE);
  time_graph = KheGraphBegin(time_file);
  KheGraphSetBelowCaption(time_graph, "Days");
  KheGraphSetLeftCaptionAndGap(time_graph, "Secs", "1.0c");

  /* initialize the table size file and graph */
  TestName(KheSolnInstance(srs->soln), r, /* test_type, */ min_assts,
    max_assts, cost_limits, "size", size_file_name);
  size_file = KheFileBegin(size_file_name, KHE_FILE_LOUT_STANDALONE);
  size_graph = KheGraphBegin(size_file);
  KheGraphSetBelowCaption(size_graph, "Days");
  KheGraphSetLeftCaptionAndGap(size_graph, "Size", "1.0c");

  /* initialize the cost file and graph */
  TestName(KheSolnInstance(srs->soln), r, /* test_type, */ min_assts,
    max_assts, cost_limits, "cost", cost_file_name);
  cost_file = KheFileBegin(cost_file_name, KHE_FILE_LOUT_STANDALONE);
  cost_graph = KheGraphBegin(cost_file);
  KheGraphSetBelowCaption(cost_graph, "Assignments");
  KheGraphSetLeftCaptionAndGap(cost_graph, "Cost", "1.0c");

  /* test with strong, medium, and weak dominance */
  KheSrsSolveTest(srs, r, KHE_SRS_DOM_WEAK, min_assts, max_assts,
    cost_limits, time_graph, size_graph, cost_graph);
  KheSrsSolveTest(srs, r, KHE_SRS_DOM_MEDIUM, min_assts, max_assts,
    cost_limits, time_graph, size_graph, cost_graph);
  KheSrsSolveTest(srs, r, KHE_SRS_DOM_STRONG, min_assts, max_assts,
    cost_limits, time_graph, size_graph, cost_graph);
  KheSrsSolveTest(srs, r, KHE_SRS_DOM_TRIE, min_assts, max_assts,
    cost_limits, time_graph, size_graph, cost_graph);

  /* tidy up graphs and stats files */
  KheGraphEnd(time_graph);
  KheFileEnd(time_file);
  KheGraphEnd(size_graph);
  KheFileEnd(size_file);
  KheGraphEnd(cost_graph);
  KheFileEnd(cost_file);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheMinConstraintWeight(KHE_SOLN soln, KHE_RESOURCE r,               */
/*    KHE_COST *min_weight)                                                  */
/*                                                                           */
/*  If r has at least one constraint whose cost will be optimized and        */
/*  whose weight is non-negative, return the minimum of those weights.       */
/*                                                                           */
/*****************************************************************************/

static bool KheMinConstraintWeight(KHE_SOLN soln, KHE_RESOURCE r,
  KHE_COST *min_weight)
{
  int i;  KHE_CONSTRAINT c;  KHE_COST weight;
  *min_weight = KheCost(INT_MAX, INT_MAX);
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    switch( KheConstraintTag(c) )
    {
      case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

	weight = KheConstraintCombinedWeight(c);
	if( weight > 0 && weight < *min_weight )
	  *min_weight = weight;
	break;

      default:

	/* ignore other constraints */
	break;
    }
  }
  return *min_weight < KheCost(INT_MAX, INT_MAX);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSingleResourceSolverTest(KHE_SOLN soln, KHE_OPTIONS options,     */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Test single resource solving on r.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSingleResourceSolverTest(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE r)
{
#if TESTING
  KHE_SINGLE_RESOURCE_SOLVER srs;  int max_busy_times, min_times, max_times;
  ARRAY_KHE_COST cost_limits;  KHE_COST min_weight;

  /* make a solver */
  srs = KheSingleResourceSolverMake(soln, options);
  HaArrayInit(cost_limits, srs->arena);
  HaArrayAddLast(cost_limits, KheCost(0, INT_MAX));

  /* do the test */
  KheSingleResourceSolverDoTest(srs, r, /* KHE_SRS_TEST_WEAK_AND_STRONG, */
    0, INT_MAX, &cost_limits);
  if( KheResourceMaxBusyTimes(srs->soln, r, &max_busy_times) )
  {
    /* unlimited cost, limited busy days range */
    min_times = max_busy_times - max_busy_times / 5;
    max_times = max_busy_times + max_busy_times / 5;
    KheSingleResourceSolverDoTest(srs, r, min_times, max_times, &cost_limits);

    /* [w, nw, unlimited] cost, limited busy days range */
    if( KheMinConstraintWeight(soln, r, &min_weight) )
    {
      HaArrayClear(cost_limits);
      HaArrayAddLast(cost_limits, min_weight);
      HaArrayAddLast(cost_limits, min_weight * HaArrayCount(srs->days));
      HaArrayAddLast(cost_limits, KheCost(0, INT_MAX));
      KheSingleResourceSolverDoTest(srs, r, min_times, max_times, &cost_limits);
    }
  }

  /* manual tests and return */
  if( DEBUG8 )
  {
    KheSrsManualSolnTest1(srs);
    KheSrsManualSolnTest2(srs);
  }
  KheSingleResourceSolverDelete(srs);
#else
  fprintf(stderr, "to test, set TESTING to 1 in khe_sr_single_resource.c"\n);
#endif
}

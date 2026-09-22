
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
/*  FILE:         khe_se_ejector.c                                           */
/*  DESCRIPTION:  Ejector objects                                            */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define MAX_GROUP_AUGMENT 30

#define DEBUG1 0	/* KheEjectorSolveOneDefect */
#define DEBUG2 0	/* phases and new successes */
#define DEBUG3 0	/* cost limit setup */
#define DEBUG4 0	/* KheEjectorAugment */
#define DEBUG5 0	/* KheEjectorSolveEnd */
#define DEBUG7 0	/* histograms */
#define DEBUG8 0	/* drops */
#define DEBUG9 0	/* check no clashes if successful */
#define DEBUG10 0	/* fresh visits */
#define DEBUG11 0	/* successes */
#define DEBUG12 0	/* KheEjectorMakeBegin */
#define DEBUG13 0	/* KheEjectorSolveOneMajorSchedule */
#define DEBUG14 0	/* KheVertexMerge */
#define DEBUG15 0
#define DEBUG16 0
#define DEBUG17 1
#define DEBUG18(d, soln) (true && KheMonitorTag(d)==KHE_GROUP_MONITOR_TAG && \
  KheGroupMonitorSubTag((KHE_GROUP_MONITOR)(d)) == KHE_SUBTAG_ORDINARY_DEMAND \
  && 1116 <= KheHardCost(KheSolnCost(soln))				      \
  && KheHardCost(KheSolnCost(soln)) <= 1117)

#define DEBUG19(d) (true && KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG &&  \
  KheGroupMonitorSubTag((KHE_GROUP_MONITOR)(d)) == KHE_SUBTAG_ORDINARY_DEMAND)

#define KHE_EJECTOR_WITH_STATS 1


/*****************************************************************************/
/*                                                                           */
/*  Type declarations (construction)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MINOR_SCHEDULE                                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_minor_schedule_rec {
  int				max_length;	/* max length                 */
  bool				may_revisit;	/* allow revisiting          */
} *KHE_EJECTOR_MINOR_SCHEDULE;

typedef HA_ARRAY(KHE_EJECTOR_MINOR_SCHEDULE) ARRAY_KHE_EJECTOR_MINOR_SCHEDULE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MAJOR_SCHEDULE                                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_major_schedule_rec {
  ARRAY_KHE_EJECTOR_MINOR_SCHEDULE	minor_schedules;
} *KHE_EJECTOR_MAJOR_SCHEDULE;

typedef HA_ARRAY(KHE_EJECTOR_MAJOR_SCHEDULE) ARRAY_KHE_EJECTOR_MAJOR_SCHEDULE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_FN - one augment function, stored with its type              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_augment_fn_rec {
  KHE_EJECTOR_AUGMENT_FN	fn;
  int				type;
} KHE_AUGMENT_FN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_REPAIR_INFO - constant information about one repair type             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_repair_info_rec {
  int				repair_type;		/* integer value     */
  char				*repair_label;		/* label             */
} *KHE_REPAIR_INFO;

typedef HA_ARRAY(KHE_REPAIR_INFO) ARRAY_KHE_REPAIR_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_INFO - constant information about one augment type           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_augment_info_rec {
  int				augment_type;		/* integer value     */
  char				*augment_label;		/* label             */
} *KHE_AUGMENT_INFO;

typedef HA_ARRAY(KHE_AUGMENT_INFO) ARRAY_KHE_AUGMENT_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Type declarations (statistics)                                           */
/*                                                                           */
/*****************************************************************************/

#if KHE_EJECTOR_WITH_STATS
/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_IMPROVEMENT_STATS - statistics about one improvement         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_improvement_stats_rec {
  int				repair_count;		/* no. of repairs    */
  float				time;			/* time to here      */
  KHE_COST			cost;			/* cost after        */
  int				defects;		/* defects           */
} KHE_EJECTOR_IMPROVEMENT_STATS;

typedef HA_ARRAY(KHE_EJECTOR_IMPROVEMENT_STATS)
  ARRAY_KHE_EJECTOR_IMPROVEMENT_STATS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_REPAIR_STATS - statistics about repairs of one augment_type          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_pair_rec {
  int				total;
  int				successful;
} KHE_PAIR;

typedef HA_ARRAY(KHE_PAIR) ARRAY_KHE_PAIR;

typedef struct khe_ejector_repair_stats_rec {
  KHE_PAIR			overall;
  ARRAY_KHE_PAIR		by_type;
} KHE_REPAIR_STATS;

typedef HA_ARRAY(KHE_REPAIR_STATS) ARRAY_KHE_REPAIR_STATS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_STATS - statistics about the ejector generally               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_stats_rec {

  /* statistics about the current call to KheEjectorSolve */
  ARRAY_KHE_EJECTOR_IMPROVEMENT_STATS	improvement_stats;
  KHE_TIMER			timer;
  KHE_COST			init_cost;
  int				init_defects;

  /* statistics about the full history */
  HA_ARRAY_INT			repair_count_histo;
  HA_ARRAY_INT			augment_count_histo;
  ARRAY_KHE_REPAIR_STATS	repair_stats;
} KHE_EJECTOR_STATS;
#endif


/*****************************************************************************/
/*                                                                           */
/*  Type declarations (solving)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_BEAM_ELT - one element of an ejection beam                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_beam_elt_rec {
  KHE_MONITOR			monitor;		/* the monitor       */
  KHE_COST			target;			/* its target        */
  KHE_COST			gap;			/* cost minus target */
} KHE_BEAM_ELT;

typedef HA_ARRAY(KHE_BEAM_ELT) ARRAY_KHE_BEAM_ELT;


/*****************************************************************************/
/*                                                                           */
/*   KHE_VERTEX - one vertex of the search space                             */
/*                                                                           */
/*   This type also contains fields representing the current outgoing        */
/*   edge from the vertex, when there is a current outgoing edge.            */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_vertex_rec {
  ARRAY_KHE_BEAM_ELT	beam;			/* the ejection beam here    */
  KHE_MARK		mark;			/* marks soln at start       */
  /* KHE_TRACE		trace; */		/* for tracing each repair   */
  int			augment_type;		/* augment type              */
  /* bool		success; */		/* if successful repair      */
} *KHE_VERTEX;

typedef HA_ARRAY(KHE_VERTEX) ARRAY_KHE_VERTEX;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_STATE - the state of an ejector                              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_EJECTOR_MAKE_SETTING,		/* setting up to make                */
  KHE_EJECTOR_SOLVE_IDLE,		/* idle, ready to solve              */
  KHE_EJECTOR_SOLVE_SETTING,		/* setting up to solve               */
  KHE_EJECTOR_SOLVE_RUN,		/* running                           */
  KHE_EJECTOR_SOLVE_RUN_REPAIR		/* running, expecting RepairEnd      */
} KHE_EJECTOR_STATE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST_LIMIT - a limit on the cost of a given monitor                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_cost_limit_rec {
  KHE_MONITOR			monitor;		/* the monitor       */
  KHE_COST			cost;			/* its cost limit    */
  bool				reducing;		/* reducing limit    */
} KHE_COST_LIMIT;

typedef HA_ARRAY(KHE_COST_LIMIT) ARRAY_KHE_COST_LIMIT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR                                                              */
/*                                                                           */
/*  Note on length.  The length of an augment is the number of calls on      */
/*  the chain it ends, including itself.  So the length of an augment        */
/*  called immediately from the main loop is 1, the length of an augment     */
/*  below it is 2, and so on.                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_ejector_rec
{
  /* free lists */
  ARRAY_KHE_EJECTOR_MAJOR_SCHEDULE major_schedule_free_list;
  ARRAY_KHE_EJECTOR_MINOR_SCHEDULE minor_schedule_free_list;

  /* defined after setting up the ejector */
  HA_ARENA			arena;			/* arena             */
  KHE_EJECTOR_STATE		state;			/* ejector state     */
  ARRAY_KHE_AUGMENT_INFO	augment_info_array;	/* augment info      */
  ARRAY_KHE_REPAIR_INFO		repair_info_array;	/* repair info       */
  ARRAY_KHE_EJECTOR_MAJOR_SCHEDULE major_schedules;	/* major schedules   */
  /* KHE_EJECTOR_MAJOR_SCHEDULE	new_major_schedule; */	/* when constructing */
  KHE_AUGMENT_FN		nongroup_augment[KHE_MONITOR_TAG_COUNT];
  KHE_AUGMENT_FN		group_augment[MAX_GROUP_AUGMENT];
  /* ARRAY_KHE_VERTEX		free_vertices; */	/* free list         */
#if KHE_EJECTOR_WITH_STATS
  KHE_EJECTOR_STATS		stats;			/* statistics        */
#endif

  /* options, defined after setting up for solving */
  int				max_augments;		/* max no of augments*/
  int				max_repairs;		/* max no of repairs */
  /* bool			no_promote_defects; */	/* !promote defects  */
  int				limit_defects;		/* limit defects     */
  bool				fresh_visits;		/* fresh visits      */
  int				max_beam;		/* max beam width    */
  KHE_MONITOR			debug_monitor;		/* debug this monitor*/
  KHE_MONITOR			whynot_monitor;		/* debug this monitor*/
  bool				whynot_running;		/* whynot running now*/

  /* also defined after setting up for solving */
  KHE_SOLN			soln;			/* solution          */
  KHE_GROUP_MONITOR		start_gm;		/* main loop gm      */
  KHE_GROUP_MONITOR		continue_gm;		/* augment fn        */
  KHE_TRACE			continue_gm_trace;	/* traces repairs    */
  KHE_OPTIONS			options;		/* solve options     */
  char				*schedule;		/* schedule string  */
  KHE_AUGMENT_OPTIONS		augment_options;	/* augment options   */
  /* ***
  KHE_FRAME			frame;			** frame             **
  KHE_EVENT_TIMETABLE_MONITOR	event_timetable_monitor;  ** etm             **
  KHE_TASK_FINDER		task_finder;		** frame             **
  *** */
  ARRAY_KHE_COST_LIMIT		cost_limits;		/* cost limits       */
  ARRAY_KHE_VERTEX		vertex_stack;		/* augment stack     */
  int				augment_count;		/* no. of augments   */

  /* defined only temporarily during solving (which is what "curr_" means) */
  KHE_EJECTOR_MAJOR_SCHEDULE	curr_major_schedule;	/* current schedule  */
  KHE_EJECTOR_MINOR_SCHEDULE	curr_minor_schedule;	/* current schedule  */
  KHE_COST			curr_target_cost;	/* cost to beat      */
  int				curr_debug_count;	/* debug if > 0      */
  int				curr_length;		/* no of vertices    */
  int				curr_augment_count;	/* current augments  */
  int				curr_repair_count;	/* current repairs   */
  /* int			curr_successful_visit; */ /* visit num       */
  ARRAY_KHE_MONITOR		curr_defects;		/* current defects   */
  bool				curr_time_limit_reached; /*time limit reached*/
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EJECTOR_MINOR_SCHEDULE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MINOR_SCHEDULE KheEjectorMinorScheduleMake(int max_length,   */
/*    bool may_revisit, HA_ARENA a)                                          */
/*                                                                           */
/*  Make a new minor schedule with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_EJECTOR_MINOR_SCHEDULE KheEjectorMinorScheduleMake(int max_length,
  bool may_revisit, KHE_EJECTOR ej)
{
  KHE_EJECTOR_MINOR_SCHEDULE res;
  HnAssert(max_length > 0,
    "KheEjectorMinorScheduleMake: max_length (%d) out of range", max_length);
  if( HaArrayCount(ej->minor_schedule_free_list) > 0 )
    res = HaArrayLastAndDelete(ej->minor_schedule_free_list);
  else
    HaMake(res, ej->arena);
  res->max_length = max_length;
  res->may_revisit = may_revisit;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMinorScheduleDelete(KHE_EJECTOR_MINOR_SCHEDULE mns,       */
/*    KHE_EJECTOR ej)                                                        */
/*                                                                           */
/*  Delete mns.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** unused
static void KheEjectorMinorScheduleDelete(KHE_EJECTOR_MINOR_SCHEDULE mns,
  KHE_EJECTOR ej)
{
  HaArrayAddLast(ej->minor_schedule_free_list, mns);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheEjectorReadMinorSchedule(KHE_EJECTOR ej, char *p)               */
/*                                                                           */
/*  Read one minor schedule, starting at *p, and install it.  Return a       */
/*  pointer to the first unread character.                                   */
/*                                                                           */
/*****************************************************************************/

static char *KheEjectorReadMinorSchedule(KHE_EJECTOR ej,
  KHE_EJECTOR_MAJOR_SCHEDULE mjs, char *p)
{
  int max_length;  bool may_revisit;  KHE_EJECTOR_MINOR_SCHEDULE ms;

  /* read max_length */
  switch( *p )
  {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':

      sscanf(p, "%d", &max_length);
      do
	p++;
      while( *p >= '0' && *p <= '9' );
      break;

    case 'u':

      max_length = INT_MAX;
      p++;
      break;

    default:

      HnAbort("KheEjectorSetSchedulesFromString: '%c' in minor schedule", *p);
      max_length = 0;  /* keep compiler happy */
      break;
  }

  /* read may_revisit */
  switch( *p )
  {
    case '+':
      may_revisit = true;
      p++;
      break;

    case '-':
      may_revisit = false;
      p++;
      break;

    default:
      HnAbort("KheEjectorSetSchedulesFromString: minor schedule ends with '%c'",
        *p);
      may_revisit = false;  /* keep compiler happy */
      break;
  }

  /* install the minor schedule and return */
  ms = KheEjectorMinorScheduleMake(max_length, may_revisit, ej);
  HaArrayAddLast(mjs->minor_schedules, ms);
  return p;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMinorScheduleDebug(KHE_EJECTOR_MINOR_SCHEDULE mns,        */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mns onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorMinorScheduleDebug(KHE_EJECTOR_MINOR_SCHEDULE mns,
  int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  if( mns->max_length == INT_MAX )
    fprintf(fp, "u");
  else
    fprintf(fp, "%d", mns->max_length);
  fprintf(fp, "%c", mns->may_revisit ? '+' : '-');
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EJECTOR_MAJOR_SCHEDULE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MAJOR_SCHEDULE KheEjectorMajorScheduleMake(HA_ARENA a)       */
/*                                                                           */
/*  Make a new, empty major schedule.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_EJECTOR_MAJOR_SCHEDULE KheEjectorMajorScheduleMake(KHE_EJECTOR ej)
{
  KHE_EJECTOR_MAJOR_SCHEDULE res;
  if( HaArrayCount(ej->major_schedule_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ej->major_schedule_free_list);
    HaArrayClear(res->minor_schedules);
  }
  else
  {
    HaMake(res, ej->arena);
    HaArrayInit(res->minor_schedules, ej->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMajorScheduleDelete(KHE_EJECTOR_MAJOR_SCHEDULE mjs,       */
/*    KHE_EJECTOR ej)                                                        */
/*                                                                           */
/*  Delete mjs, including deleting its minor schedules.                      */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorMajorScheduleDelete(KHE_EJECTOR_MAJOR_SCHEDULE mjs,
  KHE_EJECTOR ej)
{
  int i;
  HaArrayAppend(ej->minor_schedule_free_list, mjs->minor_schedules, i);
  HaArrayAddLast(ej->major_schedule_free_list, mjs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMajorScheduleDebug(KHE_EJECTOR_MAJOR_SCHEDULE mjs,        */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mjs onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorMajorScheduleDebug(KHE_EJECTOR_MAJOR_SCHEDULE mjs,
  int indent, FILE *fp)
{
  int j;  KHE_EJECTOR_MINOR_SCHEDULE mns;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(mjs->minor_schedules, mns, j)
  {
    if( j > 0 )
      fprintf(fp, ",");
    KheEjectorMinorScheduleDebug(mns, -1, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEjectorReadMajorSchedule(KHE_EJECTOR ej, char *p)               */
/*                                                                           */
/*  Read one major schedule, starting at *p, and install it.  Return a       */
/*  pointer to the first unread character.                                   */
/*                                                                           */
/*****************************************************************************/

static char *KheEjectorReadMajorSchedule(KHE_EJECTOR ej, char *p)
{
  KHE_EJECTOR_MAJOR_SCHEDULE mjs;
  mjs = KheEjectorMajorScheduleMake(ej);
  HaArrayAddLast(ej->major_schedules, mjs);
  p = KheEjectorReadMinorSchedule(ej, mjs, p);
  while( *p == ',' )
  {
    p++;
    p = KheEjectorReadMinorSchedule(ej, mjs, p);
  }
  return p;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorSetSchedulesFromString(KHE_EJECTOR ej,                    */
/*    char *ejector_schedule_string)                                         */
/*                                                                           */
/*  Set ej's schedule using the instructions in ejector_schedule_string.    */
/*  But first, clear out any old schedule.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorSetSchedulesFromString(KHE_EJECTOR ej,
  char *ejector_schedule_string)
{
  char *p;  KHE_EJECTOR_MAJOR_SCHEDULE mjs;  int i;

  /* clear out any old schedule */
  HaArrayForEach(ej->major_schedules, mjs, i)
    KheEjectorMajorScheduleDelete(mjs, ej);
  HaArrayClear(ej->major_schedules);

  /* read and install the new schedule */
  p = ejector_schedule_string;
  p = KheEjectorReadMajorSchedule(ej, p);
  while( *p == ';' )
  {
    p++;
    p = KheEjectorReadMajorSchedule(ej, p);
  }
  HnAssert(*p == '\0', "KheEjectorSetSchedulesFromString: trailing character");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorSchedulesDebug(KHE_EJECTOR ej, int indent, FILE *fp)      */
/*                                                                           */
/*  Debug print of ejector schedule.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorSchedulesDebug(KHE_EJECTOR ej, int indent, FILE *fp)
{
  KHE_EJECTOR_MAJOR_SCHEDULE mjs;  int i;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(ej->major_schedules, mjs, i)
  {
    if( i > 0 )
      fprintf(fp, ";");
    KheEjectorMajorScheduleDebug(mjs, -1, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "vertices" (private)                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VERTEX KheVertexMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Make a new vertex.  All the attributes are basically undefined.          */
/*                                                                           */
/*****************************************************************************/

static KHE_VERTEX KheVertexMake(HA_ARENA a)
{
  KHE_VERTEX res;
  /* *** no free list needed now
  if( HaArrayCount(ej->free_vertices) > 0 )
  {
    res = HaArrayLastAndDelete(ej->free_vertices);
    HaArrayClear(res->beam);
  }
  else
  {
    HaMake(res, ej->arena);
    HaArrayInit(res->beam, ej->arena);
  }
  *** */
  HaMake(res, a);
  HaArrayInit(res->beam, a);
  res->mark = NULL;
  res->augment_type = 0;
  /* res->success = false; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVertexDelete(KHE_VERTEX v, KHE_EJECTOR ej)                       */
/*                                                                           */
/*  Delete v (put it on ej's free list).                                     */
/*                                                                           */
/*****************************************************************************/

/* ***  no longer using a free list
static void KheVertexDelete(KHE_VERTEX v, KHE_EJECTOR ej)
{
  ** KheTraceDelete(v->trace); **
  HaArrayAddLast(ej->free_vertices, v);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheVertexInit(KHE_VERTEX v, KHE_MONITOR m)                          */
/*                                                                           */
/*  Initialize v's beam to hold just m.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheVertexInit(KHE_VERTEX v, KHE_MONITOR m)
{
  KHE_BEAM_ELT be;  KHE_COST lower_bound, cost;
  HaArrayClear(v->beam);
  lower_bound = KheMonitorLowerBound(m);
  cost = KheMonitorCost(m);
  if( cost > lower_bound )
  {
    be.monitor = m;
    be.target = lower_bound;
    be.gap = cost - lower_bound;
    HaArrayAddLast(v->beam, be);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorFirstVertex(KHE_EJECTOR ej, KHE_MONITOR m,                */
/*    KHE_VERTEX *first_v)                                                   */
/*                                                                           */
/*  If m makes a suitable basis for a first vertex, set *first_v to that     */
/*  vertex and return true.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorFirstVertex(KHE_EJECTOR ej, KHE_MONITOR m,
  KHE_VERTEX *first_v)
{
  *first_v = HaArrayFirst(ej->vertex_stack);
  return KheVertexInit(*first_v, m);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheVertexContainsMonitor(KHE_VERTEX v, KHE_MONITOR m)               */
/*                                                                           */
/*  Return true if v contains m.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheVertexContainsMonitor(KHE_VERTEX v, KHE_MONITOR m)
{
  KHE_BEAM_ELT be;  int i;
  HaArrayForEachReverse(v->beam, be, i)
    if( be.monitor == m )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheVertexMerge(KHE_VERTEX pred_v, KHE_TRACE trace,                  */
/*    KHE_COST min_gap, int max_elements, KHE_VERTEX succ_v)                 */
/*                                                                           */
/*  Merge pred_v's beam and trace into a new beam, stored in succ_v.         */
/*  The new beam B must satisfy these conditions:                            */
/*                                                                           */
/*    * No monitor appears twice in B                                        */
/*                                                                           */
/*    * All B's monitors are unvisited                                       */
/*                                                                           */
/*    * sum[m in B](cost(m) - targ(m)) > min_gap                             */
/*                                                                           */
/*    * 1 <= |B| <= max_elements                                             */
/*                                                                           */
/*  Since min_gap >= 0, the third condition implies |B| >= 1.  If no beam    */
/*  satisfying these conditions can be found, KheVertexMerge returns false.  */
/*                                                                           */
/*****************************************************************************/
static void KheVertexDebug(KHE_VERTEX v, int verbosity, int indent, FILE *fp);

static bool KheVertexMerge(KHE_EJECTOR ej, KHE_VERTEX pred_v, KHE_TRACE trace,
  KHE_COST min_gap, int max_elements, KHE_VERTEX succ_v)
{
  KHE_BEAM_ELT pred_be, succ_be, be;  int i, j;  KHE_MONITOR m;
  KHE_COST total_gap;

  /* transfer pred_v's beam's elements to succ_v's beam */
  /* if( DEBUG14 && KheEjectorCurrDebug(ej) ) */
  if( DEBUG14 )
  {
    fprintf(stderr, "%*s  [ KheVertexMerge; old beam:\n",
      KheEjectorCurrDebugIndent(ej), "");
    KheVertexDebug(pred_v, 2, KheEjectorCurrDebugIndent(ej) + 4, stderr);
  }
  HnAssert(min_gap >= 0, "KheVertexMerge internal error (min_gap %.5f)",
    KheCostShow(min_gap));
  HaArrayClear(succ_v->beam);
  total_gap = 0;
  HaArrayForEach(pred_v->beam, pred_be, i)
  {
    /* ignore this element if it is already visited */
    if( KheMonitorVisited(pred_be.monitor, 0) )
      continue;

    /* build succ_be */
    succ_be.monitor = pred_be.monitor;
    succ_be.target = pred_be.target;
    succ_be.gap = KheMonitorCost(succ_be.monitor) - succ_be.target;

    /* insert succ_be into child beam in sorted order */
    HaArrayForEachReverse(succ_v->beam, be, j)
      if( be.gap >= succ_be.gap )
	break;
    HaArrayAdd(succ_v->beam, j + 1, succ_be);
    total_gap += succ_be.gap;
  }
  /* if( DEBUG14 && KheEjectorCurrDebug(ej) ) */
  if( DEBUG14 )
  {
    fprintf(stderr, "%*s    new beam after old beam copy (total_gap = %.5f):\n",
      KheEjectorCurrDebugIndent(ej), "", KheCostShow(total_gap));
    KheVertexDebug(succ_v, 2, KheEjectorCurrDebugIndent(ej) + 4, stderr);
  }

  /* transfer trace's elements to succ_v's beam */
  for( i = 0;  i < KheTraceMonitorCount(trace);  i++ )
  {
    /* find i'th monitor but ignore it if it is visited or already present */
    m = KheTraceMonitor(trace, i);
    if( KheMonitorVisited(m, 0) || KheVertexContainsMonitor(succ_v, m) )
      continue;

    /* build succ_be but ignore it if its cost has not increased */
    succ_be.monitor = m;
    succ_be.target = KheTraceMonitorInitCost(trace, i);
    succ_be.gap = KheMonitorCost(m) - succ_be.target;
    if( succ_be.gap <= 0 )
      continue;

    /* insert succ_be into child beam in sorted order */
    HaArrayForEachReverse(succ_v->beam, be, j)
      if( be.gap >= succ_be.gap )
	break;
    HaArrayAdd(succ_v->beam, j + 1, succ_be);
    total_gap += succ_be.gap;

    /* if beam is too long, remove the last (smallest) element */
    if( HaArrayCount(succ_v->beam) > max_elements )
    {
      be = HaArrayLastAndDelete(succ_v->beam);
      total_gap -= be.gap;
    }
  }
  /* if( DEBUG14 && KheEjectorCurrDebug(ej) ) */
  if( DEBUG14 )
  {
    fprintf(stderr, "%*s    new beam after trace copy (total_gap = %.5f):\n",
      KheEjectorCurrDebugIndent(ej), "", KheCostShow(total_gap));
    KheVertexDebug(succ_v, 2, KheEjectorCurrDebugIndent(ej) + 4, stderr);
  }

  /* remove final elements while doing so does not violate min_gap */
  while( HaArrayCount(succ_v->beam) > 0 )
  {
    be = HaArrayLast(succ_v->beam);
    if( total_gap - be.gap <= min_gap )
      break;
    HaArrayDeleteLast(succ_v->beam);
    total_gap -= be.gap;
  }

  /* return true if total gap is at least min_gap */
  /* if( DEBUG14 && KheEjectorCurrDebug(ej) ) */
  if( DEBUG14 )
  {
    fprintf(stderr, "%*s    new beam after reduction (total_gap = %.5f):\n",
      KheEjectorCurrDebugIndent(ej), "", KheCostShow(total_gap));
    KheVertexDebug(succ_v, 2, KheEjectorCurrDebugIndent(ej) + 4, stderr);
    fprintf(stderr, "%*s  ] KheVertexMerge returning %s (%.5f > %.5f)\n",
      KheEjectorCurrDebugIndent(ej), "", bool_show(total_gap > min_gap),
      KheCostShow(total_gap), KheCostShow(min_gap));
  }
  if( KheEjectorCurrDebug(ej) && total_gap <= min_gap )
  {
    fprintf(stderr, "%*s  total_gap (%.5f) <= min_gap (%.5f)\n",
      KheEjectorCurrDebugIndent(ej), "", KheCostShow(total_gap),
      KheCostShow(min_gap));
    KheVertexDebug(succ_v, 1, KheEjectorCurrDebugIndent(ej), stderr);
  }
  return HaArrayCount(succ_v->beam) > 0 && total_gap > min_gap;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorNextVertex(KHE_EJECTOR ej, KHE_VERTEX *succ_v)            */
/*                                                                           */
/*  Decide whether ej's path can extend by one vertex.  If it can,           */
/*  set *succ_v to this vertex and return true, but do not increase          */
/*  the path length.  Otherwise return false.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorNextVertex(KHE_EJECTOR ej, KHE_VERTEX *succ_v)
{
  KHE_VERTEX pred_v;  KHE_COST min_gap;

  /* make sure there is another vertex, if needed */
  if( ej->curr_length >= HaArrayCount(ej->vertex_stack) )
    HaArrayAddLast(ej->vertex_stack, KheVertexMake(ej->arena));

  /* get pred_v and succ_v */
  pred_v = HaArray(ej->vertex_stack, ej->curr_length - 1);
  *succ_v = HaArray(ej->vertex_stack, ej->curr_length);
  min_gap = KheSolnCost(ej->soln) - ej->curr_target_cost;
  return min_gap > 0 && KheVertexMerge(ej, pred_v, ej->continue_gm_trace,
    min_gap, ej->max_beam, *succ_v);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVertexDebug(KHE_VERTEX v, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of v's beam with the given verbosity and indent.             */
/*                                                                           */
/*****************************************************************************/

static void KheVertexDebug(KHE_VERTEX v, int verbosity, int indent, FILE *fp)
{
  KHE_BEAM_ELT be;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Beam (%d elements)\n", indent, "",
      HaArrayCount(v->beam));
    HaArrayForEach(v->beam, be, i)
    {
      fprintf(fp, "%*s  targ %.5f, gap %.5f:\n", indent, "",
	KheCostShow(be.target), KheCostShow(be.gap));
      KheMonitorDebug(be.monitor, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "[ Beam (%d elements) ]", HaArrayCount(v->beam));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejector construction"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR KheEjectorMakeBegin(void)                                    */
/*                                                                           */
/*  Start off the construction of a new ejector.                             */
/*                                                                           */
/*****************************************************************************/
static void KheEjectorInitAllStats(KHE_EJECTOR ej);

KHE_EJECTOR KheEjectorMakeBegin( /* char *schedule, */ HA_ARENA a)
{
  KHE_EJECTOR res;  int i;  KHE_AUGMENT_FN null_fn;

  /* free lists */
  if( DEBUG12 )
    fprintf(stderr, "[ KheEjectorMakeBegin(a)\n");
    /* fprintf(stderr, "[ KheEjectorMakeBegin(\"%s\", a)\n", schedule); */
  HaMake(res, a);
  HaArrayInit(res->major_schedule_free_list, a);
  HaArrayInit(res->minor_schedule_free_list, a);

  /* defined after setting up the ejector */
  res->arena = a;
  res->schedule = NULL;
  /* res->schedule = HnStringCopy(schedule, a); */
  res->state = KHE_EJECTOR_MAKE_SETTING;
  HaArrayInit(res->augment_info_array, a);
  HaArrayInit(res->repair_info_array, a);
  HaArrayInit(res->major_schedules, a);
  /* res->new_major_schedule = NULL; */
  if( DEBUG12 )
    KheEjectorSchedulesDebug(res, 2, stderr);
  null_fn.fn = NULL;
  null_fn.type = 0;
  for( i = 0;  i < KHE_MONITOR_TAG_COUNT;  i++ )
    res->nongroup_augment[i] = null_fn;
  for( i = 0;  i < MAX_GROUP_AUGMENT;  i++ )
    res->group_augment[i] = null_fn;
  /* HaArrayInit(res->free_vertices, a); */
  KheEjectorInitAllStats(res);

  /* options, defined after setting up for solving */
  res->max_augments = INT_MAX;
  res->max_repairs = INT_MAX;
  /* res->no_promote_defects = false; */
  res->limit_defects = INT_MAX - 1;
  res->fresh_visits = false;
  res->max_beam = 1;
  res->debug_monitor = NULL;
  res->whynot_monitor = NULL;
  res->whynot_running = false;

  /* also defined after setting up for solving */
  res->soln = NULL;
  res->start_gm = NULL;
  res->continue_gm = NULL;
  res->continue_gm_trace = NULL;
  res->options = NULL;
  res->augment_options = NULL;
  /* ***
  res->frame = NULL;
  res->event_timetable_monitor = NULL;
  res->task_finder = NULL;
  *** */
  HaArrayInit(res->cost_limits, a);
  HaArrayInit(res->vertex_stack, a);
  HaArrayAddLast(res->vertex_stack, KheVertexMake(a));
  res->augment_count = 0;

  /* defined only temporarily during solving */
  res->curr_target_cost = 0;
  res->curr_debug_count = 0;
  res->curr_augment_count = 0;
  res->curr_repair_count = 0;
  /* res->curr_successful_visit = 0; */
  HaArrayInit(res->curr_defects, a);
  res->curr_major_schedule = NULL;
  res->curr_minor_schedule = NULL;
  if( DEBUG12 )
    fprintf(stderr, "] KheEjectorMakeBegin returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMakeEnd(KHE_EJECTOR ej)                                   */
/*                                                                           */
/*  Finish off the construction of the new ejector.                          */
/*                                                                           */
/*****************************************************************************/

void KheEjectorMakeEnd(KHE_EJECTOR ej)
{
  HnAssert(ej->state == KHE_EJECTOR_MAKE_SETTING,
    "KheEjectorMakeEnd called out of order");
  ej->state = KHE_EJECTOR_SOLVE_IDLE;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEjectorSchedules(KHE_EJECTOR ej)                                */
/*                                                                           */
/*  Return the schedule string of ej.                                       */
/*                                                                           */
/*****************************************************************************/

char *KheEjectorSchedules(KHE_EJECTOR ej)
{
  return ej->schedule;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheEjectorArena(KHE_EJECTOR ej)                                 */
/*                                                                           */
/*  Return ej's arena.                                                       */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheEjectorArena(KHE_EJECTOR ej)
{
  return ej->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddAugment(KHE_EJECTOR ej, KHE_MONITOR_TAG tag,           */
/*    KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type)                   */
/*                                                                           */
/*  Set the augment function for non-group monitors with this tag.           */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddAugment(KHE_EJECTOR ej, KHE_MONITOR_TAG tag,
  KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type)
{
  HnAssert(ej->state==KHE_EJECTOR_MAKE_SETTING,
    "KheEjectorAddAugment called out of order");
  HnAssert(tag >= 0 && tag < KHE_MONITOR_TAG_COUNT,
    "KheEjectorAddAugment: tag (%d) out of range", tag);
  HnAssert(tag != KHE_GROUP_MONITOR_TAG,
    "KheEjectorAddAugment: tag is KHE_GROUP_MONITOR_TAG");
  ej->nongroup_augment[tag].fn = augment_fn;
  ej->nongroup_augment[tag].type = augment_type;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddGroupAugment(KHE_EJECTOR ej, int sub_tag,              */
/*    KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type)                   */
/*                                                                           */
/*  Set the augment function for group monitors with this sub-tag.           */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddGroupAugment(KHE_EJECTOR ej, int sub_tag,
  KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type)
{
  HnAssert(ej->state==KHE_EJECTOR_MAKE_SETTING,
    "KheEjectorAddGroupAugment called out of order");
  HnAssert(sub_tag >= 0 && sub_tag < MAX_GROUP_AUGMENT,
    "KheEjectorAddGroupAugment: sub_tag (%d) out of range", sub_tag);
  ej->group_augment[sub_tag].fn = augment_fn;
  ej->group_augment[sub_tag].type = augment_type;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejector solving (main part)"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorSolve(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,         */
/*    KHE_GROUP_MONITOR continue_gm, KHE_OPTIONS options)                    */
/*                                                                           */
/*  Run a solve using ej.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheEjectorSolve(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,
  KHE_GROUP_MONITOR continue_gm, char *schedule, KHE_AUGMENT_OPTIONS ao,
  KHE_OPTIONS options)
{
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_IDLE,
    "KheEjectorSolve called out of order");
  KheEjectorSolveBegin(ej, start_gm, continue_gm, schedule, ao, options);
  return KheEjectorSolveEnd(ej);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorSolveBegin(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,    */
/*    KHE_GROUP_MONITOR continue_gm, KHE_OPTIONS options)                    */
/*                                                                           */
/*  Begin setting up for a solve.                                            */
/*                                                                           */
/*****************************************************************************/

void KheEjectorSolveBegin(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,
  KHE_GROUP_MONITOR continue_gm, char *schedule, KHE_AUGMENT_OPTIONS ao,
  KHE_OPTIONS options)
{
  KHE_SOLN soln;  char *opt, *monitor_id;
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_IDLE,
    "KheEjectorSolveBegin called out of order");
  soln = KheMonitorSoln((KHE_MONITOR) start_gm);
  HnAssert(soln == KheMonitorSoln((KHE_MONITOR) continue_gm),
    "KheEjectorSolveBegin: start_gm and continue_gm monitor unequal solutions");

  /* options, defined after setting up for solving */
  if( schedule == NULL )
    ej->schedule = KheOptionsGet(options, "es_schedule", "1+;u-");
  else
    ej->schedule = HnStringCopy(schedule, ej->arena);
  ej->max_augments = KheOptionsGetInt(options, "es_max_augments", 200);
  ej->max_repairs = KheOptionsGetInt(options, "es_max_repairs", INT_MAX);
  /* ***
  ej->no_promote_defects =
    KheOptionsGetBool(options, "es_no_promote_defects", false);
  *** */
  opt = KheOptionsGet(options, "es_limit_defects", "unlimited");
  ej->limit_defects = (strcmp(opt, "unlimited") == 0 ? INT_MAX - 1 :
    KheOptionsGetInt(options, "es_limit_defects", INT_MAX - 1));
  ej->fresh_visits = KheOptionsGetBool(options, "es_fresh_visits", false);
  ej->max_beam =  KheOptionsGetInt(options, "es_max_beam", 1);
  ej->debug_monitor =
    (KHE_MONITOR) KheOptionsGetObject(options, "gs_debug_monitor", NULL);
  monitor_id = KheOptionsGet(options, "es_whynot_monitor_id", NULL);
  if( monitor_id == NULL )
    ej->whynot_monitor = NULL;
  else if( !KheSolnRetrieveMonitor(soln, monitor_id, &ej->whynot_monitor) )
    HnAbort("KheEjectorSolveBegin: cannot find es_whynot_monitor_id \"%s\"\n",
      monitor_id);
  ej->whynot_running = false;

  /* also defined after setting up for solving */
  ej->soln = soln;
  ej->start_gm = start_gm;
  ej->continue_gm = continue_gm;
  ej->continue_gm_trace = KheTraceMake(continue_gm);
  ej->options = options;
  ej->augment_options = ao;
  /* ej->schedule = HnStringCopy(schedule, ej->arena); */
  KheEjectorSetSchedulesFromString(ej, ej->schedule);
  /* ***
  ej->frame = KheOptionsFrame(options, "gs_common_frame", soln);
  ej->event_timetable_monitor = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  ej->task_finder = KheTaskFi nderMake(soln, options, ej->arena);
  *** */
  HaArrayClear(ej->cost_limits);
  /* HaArrayClear(ej->vertex_stack); NO! */
  ej->augment_count = 0;

  /* change state, ready for further setup for solving */
  ej->state = KHE_EJECTOR_SOLVE_SETTING;
  /* ***
  if( DEBUG9 )
    KheFrameAssertNoClashes(ej->frame);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDecreasingCostCmp(const void *t1, const void *t2)          */
/*  int KheMonitorDecreasingCostDiversifyCmp(const void *t1, const void *t2) */
/*                                                                           */
/*  Comparison functions for sorting an array of defects, first by           */
/*  increasing visit number, then by decreasing cost, then by index number.  */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorDecreasingCostDiversifyCmp(const void *t1,
  const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  int cmp;
  cmp = KheCostCmp(KheMonitorCost(m2), KheMonitorCost(m1));
  if( cmp != 0 )
    return cmp;
  else if( KheSolnDiversifierChoose(KheMonitorSoln(m1), 2) == 0 )
    return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
  else
    return KheMonitorSolnIndex(m2) - KheMonitorSolnIndex(m1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorCopyAndSortDefects(KHE_EJECTOR ej)                        */
/*                                                                           */
/*  Copy and sort the defects of ej's start monitor.  Also drop some from    */
/*  the end of the sorted list, if there are too many.                       */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorCopyAndSortDefects(KHE_EJECTOR ej)
{
  KHE_MONITOR d;  int i, drop;

  /* copy the defects into ej->curr_defects */
  HaArrayClear(ej->curr_defects);
  for( i = 0;  i < KheGroupMonitorDefectCount(ej->start_gm);  i++ )
  {
    d = KheGroupMonitorDefect(ej->start_gm, i);
    HaArrayAddLast(ej->curr_defects, d);
  }

  /* sort ej->curr_defects */
  HaArraySort(ej->curr_defects, KheMonitorDecreasingCostDiversifyCmp);

  /* ensure that there are at most ejector_limit_defects defects on list */
  drop = HaArrayCount(ej->curr_defects) - ej->limit_defects;
  if( drop > 0 )
  {
    if( DEBUG8 )
    {
      fprintf(stderr,
	"  KheEjectorCopyAndSortDefects: cost %.5f, %d defects, dropping %d:\n",
	KheCostShow(KheSolnCost(ej->soln)),
	HaArrayCount(ej->curr_defects), drop);
      for( i = 0;  i < drop;  i++ )
      {
	d = HaArray(ej->curr_defects, HaArrayCount(ej->curr_defects) - drop+i);
	KheMonitorDebug(d, 1, 4, stderr);
      }
    }
    HaArrayDeleteLastSlice(ej->curr_defects, drop);
  }
  else if( DEBUG8 )
    fprintf(stderr, "  KheEjectorCopyAndSortDefects: cost %.5f, %d defects\n",
      KheCostShow(KheSolnCost(ej->soln)),
      HaArrayCount(ej->curr_defects));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorDebugDefects(KHE_GROUP_MONITOR gm, char *label,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the defects of gm.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorDebugDefects(KHE_GROUP_MONITOR gm, char *label,
  int indent, FILE *fp)
{
  int i;  KHE_MONITOR d;
  for( i = 0;  i < KheGroupMonitorDefectCount(gm);  i++ )
  {
    d = KheGroupMonitorDefect(gm, i);
    fprintf(stderr, "%*s%s ", indent, "", label);
    KheMonitorDebug(d, 2, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorDebugDefects2(KHE_EJECTOR ej, int indent, FILE *fp)       */
/*                                                                           */
/*  Debug print of the defects of ej.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorDebugDefects2(KHE_EJECTOR ej, int indent, FILE *fp)
{
  int i;  KHE_MONITOR d;
  HaArrayForEach(ej->curr_defects, d, i)
    KheMonitorDebug(d, 1, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorPotential(KHE_MONITOR m)                              */
/*                                                                           */
/*  Return the potential of m:  its cost minus its lower bound.              */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_COST KheMonitorPotential(KHE_MONITOR m)
{
  return KheMonitorCost(m) - KheMonitorLowerBound(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorTimeLimitReached(KHE_EJECTOR ej)                          */
/*                                                                           */
/*  Return true if ej's time limit is reached.                               */
/*                                                                           */
/*  If this is a whynot run, there is no time limit.  Once the time          */
/*  limit is reached, that fact is stored in ej->curr_time_limit_reached,    */
/*  to avoid calling KheOptionsTimeLimitReached multiple times after the     */
/*  time limit is reached.                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorTimeLimitReached(KHE_EJECTOR ej)
{
  /* no time limit on a whynot */
  if( ej->whynot_running )
    return false;

  /* if time limit was found to be reached previously, return true */
  if( ej->curr_time_limit_reached )
    return true;

  /* check timers and remember the result */
  ej->curr_time_limit_reached = KheOptionsTimeLimitReached(ej->options);
  return ej->curr_time_limit_reached;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDebugDemandMonitors(KHE_SOLN soln, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of all demand monitors.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSolnDebugDemandMonitors(KHE_SOLN soln, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_MONITOR m;
  fprintf(fp, "%*s[ all demand monitors:\n", indent, "");
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    if( KheMonitorTag(m) == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
        KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
      KheMonitorDebug(m, verbosity, indent + 2, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorSolveOneDefect(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment from d, trying each minor schedule in turn.  This function is    */
/*  called from the main loop, so it has to initialize for finding one       */
/*  chain as well as start the chain.                                        */
/*                                                                           */
/*  Implementation note.  It is possible for the cost of d to change to      */
/*  0 even when the algorithm does not succeed, when d is a resource         */
/*  demand monitor.  Hence the test at the start of each iteration.          */
/*                                                                           */
/*****************************************************************************/
static bool KheEjectorAugment(KHE_EJECTOR ej, KHE_VERTEX pred_v);

static bool KheEjectorSolveOneDefect(KHE_EJECTOR ej, KHE_MONITOR d, bool debug)
{
  KHE_COST init_cost;  int i, msc_count;  KHE_EJECTOR_MAJOR_SCHEDULE ejs;
  KHE_VERTEX first_v;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEjectorSolveOneDefect(ej, -)\n");
    KheMonitorDebug(d, 2, 2, stderr);
  }

  /* run each minor schedule */
  ejs = ej->curr_major_schedule;
  msc_count = HaArrayCount(ejs->minor_schedules);
  for( i = 0;  i < msc_count && !KheEjectorTimeLimitReached(ej) &&
      KheEjectorFirstVertex(ej, d, &first_v);  i++ )
  {
    ej->curr_minor_schedule = HaArray(ejs->minor_schedules, i);
    ej->curr_target_cost = KheSolnCost(ej->soln);
    ej->curr_debug_count = (debug ? 1 : 0);
    /* ej->curr_debug_count = (d == ej->debug_monitor ? 1 : 0); */
    ej->curr_length = 0;
    ej->curr_augment_count = 0;
    ej->curr_repair_count = 0;
    KheSolnNewGlobalVisit(ej->soln);
    init_cost = KheSolnCost(ej->soln);
    if( KheEjectorAugment(ej, first_v) )
    {
      if( DEBUG1 )
	fprintf(stderr, "] KheEjectorSolveOneDefect returning true\n");
      return true;
    }
    if( DEBUG17 && KheSolnCost(ej->soln) != init_cost )
    {
      fprintf(stderr, "  KheEjectorSolveOneDefect failing at vertex:\n");
      KheVertexDebug(first_v, 3, 2, stderr);
      KheSolnDebugDemandMonitors(ej->soln, 2, 2, stderr);
    }
    HnAssert(KheSolnCost(ej->soln) == init_cost,
      "KheEjectorSolveOneDefect internal error 3: %.5f != %.5f",
      KheCostShow(KheSolnCost(ej->soln)), KheCostShow(init_cost));
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheEjectorSolveOneDefect returning false\n");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorSolveOneMajorSchedule(KHE_EJECTOR ej,                     */
/*    KHE_EJECTOR_MAJOR_SCHEDULE mjs)                                        */
/*                                                                           */
/*  Solve for major schedule mjs; return true if progress was made.          */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorSolveOneMajorSchedule(KHE_EJECTOR ej,
  KHE_EJECTOR_MAJOR_SCHEDULE mjs)
{
  int i, j, curr_successful_visit, phase_count;  KHE_COST prev_cost;
  bool progressing, success;  KHE_MONITOR d;  KHE_COST_LIMIT lim;
  if( DEBUG13 )
  {
    fprintf(stderr, "[ KheEjectorSolveOneMajorSchedule(ej, ");
    KheEjectorMajorScheduleDebug(mjs, -1, stderr);
    fprintf(stderr, ")\n");
  }
  ej->curr_major_schedule = mjs;
  curr_successful_visit = KheSolnGlobalVisitNum(ej->soln);
  success = false;
  phase_count = 0;
  do
  {
    progressing = false;
    phase_count++;
    KheEjectorCopyAndSortDefects(ej);
    if( DEBUG2 )
    {
      fprintf(stderr, "  ejector starting phase %d of \"", phase_count);
      KheEjectorMajorScheduleDebug(mjs, -1, stderr);
      fprintf(stderr, "\" (%d defects):\n", HaArrayCount(ej->curr_defects));
      KheEjectorDebugDefects2(ej, 2, stderr);
    }
    prev_cost = KheSolnCost(ej->soln);
    HaArrayForEach(ej->curr_defects, d, i)
    {
      if( (DEBUG11 && KheMonitorCost(d) >= KheCost(1, 0))
	  || DEBUG13 || d == ej->debug_monitor )
      {
	fprintf(stderr, "  SolveOneMajorSchedule(ej, \"");
        KheEjectorMajorScheduleDebug(mjs, -1, stderr);
	fprintf(stderr, "\") at ");
	KheMonitorDebug(d, 1, 0, stderr);
      }

      /* quit early if time limit reached */
      if( KheEjectorTimeLimitReached(ej) )
	break;

      /* handle d in one of several ways */
      if( KheMonitorCost(d) <= KheMonitorLowerBound(d) )
      {
	/* no potential for improvement, so ignore d */
	if( DEBUG2 || d == ej->debug_monitor )
	{
	  fprintf(stderr, "  drop ");
	  KheMonitorDebug(d, 1, 0, stderr);
	}
      }
      else if( KheMonitorVisitNum(d) > curr_successful_visit )
      {
	/* d has been tried more recently than the last success, so no hope */
	if( DEBUG2 || d == ej->debug_monitor )
	{
	  fprintf(stderr, "  skip (%d > %d) ", KheMonitorVisitNum(d),
	    curr_successful_visit);
	  KheMonitorDebug(d, 1, 0, stderr);
	}
      }
      else if( KheEjectorSolveOneDefect(ej, d, d == ej->debug_monitor) )
      {
	/* augment from d has been tried, and it succeeded */
	if( DEBUG11 )
	  fprintf(stderr, "  ejector success, cost %.5f -> %.5f\n",
	    KheCostShow(prev_cost), KheCostShow(KheSolnCost(ej->soln)));
	if( DEBUG2 || d == ej->debug_monitor )
	{
	  if( KheMonitorVisitNum(d) > curr_successful_visit )
	    fprintf(stderr, "  xsuc ");
	  else
	    fprintf(stderr, "  succ ");
	  KheMonitorDebug(d, 1, 0, stderr);
	}

	/* make sure success means reduced cost */
	HnAssert(prev_cost > KheSolnCost(ej->soln),
	  "KheEjectorSolveOneMajorSchedule: cost increased (%.5f -> %.5f)\n",
	  KheCostShow(prev_cost), KheCostShow(KheSolnCost(ej->soln)));
	prev_cost = KheSolnCost(ej->soln);

	/* update visit num */
	KheMonitorSetVisitNum(d, KheSolnGlobalVisitNum(ej->soln));

	/* update reducing cost limits */
	HaArrayForEach(ej->cost_limits, lim, j)
	  if( lim.reducing )
	  {
	    lim.cost = KheMonitorCost(lim.monitor);
	    HaArrayPut(ej->cost_limits, j, lim);
	  }

	/* record success and update visit number of last success */
	success = true;
	progressing = true;
	HnAssert(KheSolnGlobalVisitNum(ej->soln) > curr_successful_visit,
	  "KheEjectorSolveOneMajorSchedule internal error 2");
	curr_successful_visit = KheSolnGlobalVisitNum(ej->soln);
      }
      else
      {
	/* augment from d has been tried, and it failed */
	if( DEBUG2 || d == ej->debug_monitor )
	{
	  fprintf(stderr, "  fail ");
	  KheMonitorDebug(d, 1, 0, stderr);
	}

	/* update visit num */
	KheMonitorSetVisitNum(d, KheSolnGlobalVisitNum(ej->soln));
      }
    }
  } while( progressing && !KheEjectorTimeLimitReached(ej) );
  if( DEBUG13 )
    fprintf(stderr, "] KheEjectorSolveOneMajorSchedule returning %s\n",
      bool_show(success));
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorSolveEnd(KHE_EJECTOR ej)                                  */
/*                                                                           */
/*  End setting up for a solve, including doing the actual solving.          */
/*                                                                           */
/*****************************************************************************/
static void KheEjectorResetSolveStats(KHE_EJECTOR ej);

bool KheEjectorSolveEnd(KHE_EJECTOR ej)
{
  int i;  KHE_COST initial_cost, final_cost, cost;  /* KHE_VERTEX init_v; */
  KHE_EJECTOR_MAJOR_SCHEDULE mjs;  KHE_MONITOR d;

  /* set up for solving */
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_SETTING,
    "KheEjectorSolveEnd called out of order");
  ej->state = KHE_EJECTOR_SOLVE_RUN;
  initial_cost = KheSolnCost(ej->soln);
  /* ***
  HaArrayClear(ej->vertex_stack);
  init_v = KheVertexMake(ej);
  HaArrayAddLast(ej->vertex_stack, init_v);
  *** */
  KheEjectorResetSolveStats(ej);
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheEjectorSolveEnd(initial cost %.5f)\n",
      KheCostShow(initial_cost));
    KheEjectorDebugDefects(ej->start_gm, "init", 2, stderr);
  }

  /* do the whole thing once for each major schedule */
  ej->curr_time_limit_reached = false;
  HaArrayForEach(ej->major_schedules, mjs, i)
    KheEjectorSolveOneMajorSchedule(ej, mjs);

  /* do a whynot run, if applicable */
  if( ej->whynot_monitor != NULL )
  {
    d = ej->whynot_monitor;
    ej->whynot_running = true;
    if( HaArrayCount(ej->major_schedules) == 0 )
      fprintf(stderr, "  KheEjectorSolveEnd: no %s whynot run "
	"(no major schedules)\n", KheMonitorId(d));
    else if( KheMonitorCost(d) <= KheMonitorLowerBound(d) )
      fprintf(stderr, "  KheEjectorSolveEnd: no %s whynot run "
	"(cost %.5f <= lower bound %.5f)\n", KheMonitorId(ej->whynot_monitor),
	KheCostShow(KheMonitorCost(d)), KheCostShow(KheMonitorLowerBound(d)));
    else
    {
      fprintf(stderr, "[ KheEjectorSolveEnd: %s whynot run:\n",
	KheMonitorId(d));
      do
      {
	cost = KheSolnCost(ej->soln);
	KheEjectorSolveOneDefect(ej, d, true);
      } while( KheSolnCost(ej->soln) < cost );
      fprintf(stderr, "] end whynot run\n");
    }
    ej->whynot_running = false;
  }

  /* clean up and exit */
  if( DEBUG5 )
  {
    /* KheEjectorDebugDefects(ej->start_gm, "final", 2, stderr); */
    fprintf(stderr, "] KheEjectorSolveEnd(final cost %.5f, diversifier %d, "
      "augment_count %d)\n", KheCostShow(KheSolnCost(ej->soln)),
      KheSolnDiversifier(ej->soln), ej->augment_count);
  }
  final_cost = KheSolnCost(ej->soln);
  HnAssert(final_cost <= initial_cost,
    "KheEjectorSolve: final cost (%.5f) exceeds initial cost (%.5f)\n",
     KheCostShow(final_cost), KheCostShow(initial_cost));
  ej->state = KHE_EJECTOR_SOLVE_IDLE;
  ej->soln = NULL;
  ej->start_gm = NULL;
  KheTraceDelete(ej->continue_gm_trace);
  ej->continue_gm = NULL;
  ej->continue_gm_trace = NULL;
  ej->options = NULL;
  ej->augment_options = NULL;
  /* ***
  ej->frame = NULL;
  ej->event_timetable_monitor = NULL;
  ej->task_finder = NULL;
  *** */
  HaArrayClear(ej->cost_limits);
  /* ***
  HnAssert(HaArrayCount(ej->vertex_stack) == 1,
    "KheEjectorSolveEnd internal error");
  ej->curr_length--;
  *** */
  /* KheVertexDelete(HaArrayLastAndDelete(ej->vertex_stack), ej); */
  if( DEBUG16 )
    fprintf(stderr, "  ej->augment_count = %d\n", ej->augment_count);
  ej->augment_count = 0;
  HaArrayClear(ej->curr_defects);
  return final_cost < initial_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejector solving - cost limits"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorDoAddMonitorCostLimit(KHE_EJECTOR ej,                     */
/*    KHE_MONITOR m, KHE_COST cost_limit, bool reducing)                     */
/*                                                                           */
/*  Add a cost limit to ej.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorDoAddMonitorCostLimit(KHE_EJECTOR ej,
  KHE_MONITOR m, KHE_COST cost_limit, bool reducing)
{
  KHE_COST_LIMIT lim;
  lim.monitor = m;
  lim.cost = cost_limit;
  lim.reducing = reducing;
  HaArrayAddLast(ej->cost_limits, lim);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddMonitorCostLimit(KHE_EJECTOR ej, KHE_MONITOR m,        */
/*    KHE_COST cost_limit)                                                   */
/*                                                                           */
/*  Add a cost limit to ej.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddMonitorCostLimit(KHE_EJECTOR ej, KHE_MONITOR m,
  KHE_COST cost_limit)
{
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheEjectorAddMonitorCostLimit(ej, m, %.5f)\n",
      KheCostShow(cost_limit));
    if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
      KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) m, 2, 2, stderr);
    else
      KheMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "]\n");
  }
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_SETTING,
    "KheEjectorAddMonitorCostLimit called out of order");
  KheEjectorDoAddMonitorCostLimit(ej, m, cost_limit, false);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddMonitorCostLimitReducing(KHE_EJECTOR ej,               */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Add a reducing cost limit to ej.                                         */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddMonitorCostLimitReducing(KHE_EJECTOR ej,
  KHE_MONITOR m)
{
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheEjectorAddMonitorCostLimitReducing(ej, m (%.5f))\n",
      KheCostShow(KheMonitorCost(m)));
    if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
      KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) m, 2, 2, stderr);
    else
      KheMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "]\n");
  }
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_SETTING,
    "KheEjectorAddMonitorCostLimitReducing called out of order");
  KheEjectorDoAddMonitorCostLimit(ej, m, KheMonitorCost(m), true);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorMonitorCostLimitCount(KHE_EJECTOR ej)                      */
/*                                                                           */
/*  Return the number of cost limits in ej.                                  */
/*                                                                           */
/*****************************************************************************/

int KheEjectorMonitorCostLimitCount(KHE_EJECTOR ej)
{
  return HaArrayCount(ej->cost_limits);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorMonitorCostLimit(KHE_EJECTOR ej, int i,                   */
/*    KHE_MONITOR *m, KHE_COST *cost_limit, bool *reducing)                  */
/*                                                                           */
/*  Return the i'th cost limit of ej.                                        */
/*                                                                           */
/*****************************************************************************/

void KheEjectorMonitorCostLimit(KHE_EJECTOR ej, int i,
  KHE_MONITOR *m, KHE_COST *cost_limit, bool *reducing)
{
  KHE_COST_LIMIT lim;
  lim = HaArray(ej->cost_limits, i);
  *m = lim.monitor;
  *cost_limit = lim.cost;
  *reducing = lim.reducing;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejector solving - queries"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheEjectorStartGroupMonitor(KHE_EJECTOR ej)            */
/*                                                                           */
/*  Return the group monitor whose defects ej's main loop repairs.           */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheEjectorStartGroupMonitor(KHE_EJECTOR ej)
{
  return ej->start_gm;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheEjectorContinueGroupMonitor(KHE_EJECTOR ej)         */
/*                                                                           */
/*  Return the group monitor whose defects ej's augment functions repair.    */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheEjectorContinueGroupMonitor(KHE_EJECTOR ej)
{
  return ej->continue_gm;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS KheEjectorOptions(KHE_EJECTOR ej)                            */
/*                                                                           */
/*  Return the options object passed to KheEjectorSolve.                     */
/*                                                                           */
/*****************************************************************************/

KHE_OPTIONS KheEjectorOptions(KHE_EJECTOR ej)
{
  return ej->options;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheEjectorFrame(KHE_EJECTOR ej)                                */
/*                                                                           */
/*  Return ej's frame.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer stored in ejector
KHE_FRAME KheEjectorFrame(KHE_EJECTOR ej)
{
  return ej->frame;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheEjectorEventTimetableMonitor(             */
/*    KHE_EJECTOR ej)                                                        */
/*                                                                           */
/*  Return ej's event timetable monitor, or NULL if none.                    */
/*                                                                           */
/*****************************************************************************/

/* *** no longer stored in ejector
KHE_EVENT_TIMETABLE_MONITOR KheEjectorEventTimetableMonitor(KHE_EJECTOR ej)
{
  return ej->event_timetable_monitor;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_FINDER KheEjectorTaskFinder(KHE_EJECTOR ej)                     */
/*                                                                           */
/*  Return ej's task finder.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** no longer stored in ejector
KHE_TASK_FINDER KheEjectorTaskFinder(KHE_EJECTOR ej)
{
  return ej->task_finder;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheEjectorSoln(KHE_EJECTOR ej)                                  */
/*                                                                           */
/*  Return start_gm's (and also continue_gm)'s solution.                     */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheEjectorSoln(KHE_EJECTOR ej)
{
  return ej->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEjectorTargetCost(KHE_EJECTOR ej)                            */
/*                                                                           */
/*  Return the target cost of ej.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEjectorTargetCost(KHE_EJECTOR ej)
{
  return ej->curr_target_cost;
  /* ***
  KHE_VERTEX ea;
  HnAssert(HaArrayCount(ej->vertex_stack) > 0,
    "KheEjectorTargetCost: not currently augmenting");
  ea = HaArrayLast(ej->vertex_stack);
  return ea->target_cost;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MAJOR_SCHEDULE KheEjectorCurrMajorSchedule(KHE_EJECTOR ej)   */
/*                                                                           */
/*  Return the major schedule currently in use.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_EJECTOR_MAJOR_SCHEDULE KheEjectorCurrMajorSchedule(KHE_EJECTOR ej)
{
  return ej->curr_major_schedule;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR_MINOR_SCHEDULE KheEjectorCurrMinorSchedule(KHE_EJECTOR ej)   */
/*                                                                           */
/*  Return the minor schedule currently in use.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_EJECTOR_MINOR_SCHEDULE KheEjectorCurrMinorSchedule(KHE_EJECTOR ej)
{
  return ej->curr_minor_schedule;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorCurrMayRevisit(KHE_EJECTOR ej)                            */
/*                                                                           */
/*  Return true if the current minor schedule allows revisiting.             */
/*                                                                           */
/*****************************************************************************/

bool KheEjectorCurrMayRevisit(KHE_EJECTOR ej)
{
  return ej->curr_minor_schedule->may_revisit;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorCurrLength(KHE_EJECTOR ej)                                  */
/*                                                                           */
/*  Return the length of the current chain.                                   */
/*                                                                           */
/*****************************************************************************/

int KheEjectorCurrLength(KHE_EJECTOR ej)
{
  return ej->curr_length;
  /* return HaArrayCount(ej->vertex_stack); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorCurrAugmentCount(KHE_EJECTOR ej)                           */
/*                                                                           */
/*  Return the number of augments on ej since the beginning of this solve.   */
/*                                                                           */
/*****************************************************************************/

int KheEjectorCurrAugmentCount(KHE_EJECTOR ej)
{
  return ej->augment_count;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorCurrDebug(KHE_EJECTOR ej)                                 */
/*                                                                           */
/*  Return true if ej is current debugging.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheEjectorCurrDebug(KHE_EJECTOR ej)
{
  return ej->curr_debug_count > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorCurrDebugIndent(KHE_EJECTOR ej)                            */
/*                                                                           */
/*  Return the current debug indent.  This is just twice the current length. */
/*                                                                           */
/*****************************************************************************/

int KheEjectorCurrDebugIndent(KHE_EJECTOR ej)
{
  return 2 * KheEjectorCurrLength(ej);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejector solving - augment and repair"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorAugment(KHE_EJECTOR ej, KHE_VERTEX pred_v)                */
/*                                                                           */
/*  Carry out one augment starting from pred_v, which contains the beam      */
/*  for reaching the current state of soln.                                  */
/*                                                                           */
/*  Vertex pred_v is not officially on the path at the start of this         */
/*  function.  However, all that is needed to make it officially on the      */
/*  path is ej->curr_length++.                                               */
/*                                                                           */
/*  Return true if the augment is successful.  Success means reducing the    */
/*  solution cost to below ej->curr_target_cost, without violating any       */
/*  monitor limits.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorAugment(KHE_EJECTOR ej, KHE_VERTEX pred_v)
{
  KHE_EJECTOR_AUGMENT_FN augment_fn;  int sub_tag, augment_type;
  bool success, debug /* , debug18_happened */;  KHE_MONITOR d;
  KHE_TRACE debug_trace;  KHE_COST debug_init_cost;

  /* update debugging */
  debug = (ej->debug_monitor != NULL &&
    KheVertexContainsMonitor(pred_v, ej->debug_monitor));
  if( debug )
    ej->curr_debug_count++;

  /* debug entry */
  if( DEBUG4 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr,
      "%*s[ KheEjectorAugment(ej, pred_v, -) soln %.5f length %d%s\n",
      KheEjectorCurrDebugIndent(ej), "",
      KheCostShow(KheSolnCost(ej->soln)), KheEjectorCurrLength(ej),
      KheEjectorCurrLength(ej) == ej->curr_minor_schedule->max_length ?
      " (limit)" : "");
    KheVertexDebug(pred_v, 2, KheEjectorCurrDebugIndent(ej) + 2, stderr);
  }

  /* quit now if the time limit is reached */
  if( KheEjectorTimeLimitReached(ej) )
  {
    if( DEBUG4 || KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s] KheEjectorAugment returning false (time limit)\n",
	KheEjectorCurrDebugIndent(ej), "");
    if( debug )
      ej->curr_debug_count--;
    return false;
  }

  /* select an arbitrary defect d and visit it */
  HnAssert(HaArrayCount(pred_v->beam) > 0,
    "KheEjectorAugment: pred_v->beam is empty");
  d = HaArrayFirst(pred_v->beam).monitor;
  KheMonitorVisit(d);
  /* ***
  if( d == ej->debug_monitor )
    fprintf(stderr, "%*s  KheMonitorVisitNum(%s) = %d\n",
      KheEjectorCurrDebugIndent(ej), "", KheMonitorId(d),
      KheMonitorVisitNum(d));
  *** */

  /* find the user's augment function for d; quit now if none */
  if( KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG )
  {
    sub_tag = KheGroupMonitorSubTag((KHE_GROUP_MONITOR) d);
    if( DEBUG15 )
      fprintf(stderr, "  augment fn for group monitor subtag %d\n", sub_tag);
    HnAssert(sub_tag >= 0 && sub_tag < MAX_GROUP_AUGMENT,
      "KheEjectorAugment: sub_tag (%d) out of range", sub_tag);
    augment_fn = ej->group_augment[sub_tag].fn;
    augment_type = ej->group_augment[sub_tag].type;
  }
  else
  {
    if( DEBUG15 )
      fprintf(stderr, "  augment fn for monitor tag %d\n", KheMonitorTag(d));
    augment_fn = ej->nongroup_augment[KheMonitorTag(d)].fn;
    augment_type = ej->nongroup_augment[KheMonitorTag(d)].type;
  }
  if( augment_fn == NULL )
  {
    if( DEBUG4 || KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s] KheEjectorAugment returning false (augment_fn)\n",
	KheEjectorCurrDebugIndent(ej), "");
    if( debug )
      ej->curr_debug_count--;
    return false;
  }

  /* add pred_v to the path and initialize its fields (beam is already done) */
  ej->curr_length++;
  /* debug18_happened = false; */
  debug_trace = NULL;
  debug_init_cost = 0;
  if( DEBUG19(d) )
  {
    /* ***
    fprintf(stderr, "[ about to augment ");
    KheMonitorDebug(d, 2, 0, stderr);
    KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) ej->soln, 2, 2, stderr);
    debug18_happened = true;
    *** */
    debug_trace = KheTraceMake((KHE_GROUP_MONITOR) ej->soln);
    KheTraceBegin(debug_trace);
    debug_init_cost = KheSolnCost(ej->soln);
  }
  pred_v->mark = KheMarkBegin(ej->soln);
  pred_v->augment_type = augment_type;
  /* pred_v->success = false; */

  /* call the user's augment function */
  ej->augment_count++;
  ej->curr_augment_count++;
  success = augment_fn(ej, ej->augment_options, d);
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_RUN,
    "KheEjectorRepairBegin called without matching KheEjectorRepairEnd");

  /* finalize the fields of pred_v, saving some, then pop pred_v from path */
  /* success = pred_v->success; */
  KheMarkEnd(pred_v->mark, !success);
  if( DEBUG19(d) )
  {
    if( !success && debug_init_cost != KheSolnCost(ej->soln) )
    {
      fprintf(stderr, "KheEjectorAugment problem(cost init %.5f, final %.5f)\n",
	KheCostShow(debug_init_cost), KheCostShow(KheSolnCost(ej->soln)));
      KheMonitorDebug(d, 2, 2, stderr);
      KheTraceDebug(debug_trace, 3, 2, stderr);
    }
    KheTraceEnd(debug_trace);
    KheTraceDelete(debug_trace);
    /* ***
    KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) ej->soln, 2, 2, stderr);
    *** */
  }
  ej->curr_length--;

  /* unvisit d if that's what is wanted */
  if( KheEjectorCurrMayRevisit(ej) )
    KheMonitorUnVisit(d);

  /* debug print and exit */
  if( DEBUG4 || KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheEjectorAugment returning %s\n",
      KheEjectorCurrDebugIndent(ej), "", bool_show(success));
  if( debug )
    ej->curr_debug_count--;
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorRepairBegin(KHE_EJECTOR ej)                               */
/*                                                                           */
/*  Begin a repair.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheEjectorRepairBegin(KHE_EJECTOR ej)
{
  /* check and change state */
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_RUN,
    "KheEjectorRepairBegin called out of order");
  ej->state = KHE_EJECTOR_SOLVE_RUN_REPAIR;

  /* fresh visit number if requested and length is 1 */
  if( ej->fresh_visits && KheEjectorCurrLength(ej) == 1 )
  {
    /* HnAbort("KheEjectorRepairBegin currently forbidding fresh visits"); */
    KheSolnNewGlobalVisit(ej->soln);
    if( DEBUG10 )
      fprintf(stderr, "%*sfresh visit\n", KheEjectorCurrDebugIndent(ej), "");
  }

  /* start tracing */
  KheTraceBegin(ej->continue_gm_trace);

  /* start debugging, if required */
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ ", KheEjectorCurrDebugIndent(ej), "");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAugmentSuccess(KHE_VERTEX ea, KHE_COST target_cost)              */
/*                                                                           */
/*  Return true if ea is successful.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheAugmentSuccess(KHE_EJECTOR ej)
{
  int i;  KHE_COST_LIMIT lim;
  if( KheSolnCost(ej->soln) >= ej->curr_target_cost )
    return false;
  HaArrayForEach(ej->cost_limits, lim, i)
    if( KheMonitorCost(lim.monitor) > lim.cost )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorNotAtLimit(KHE_EJECTOR ej)                                */
/*                                                                           */
/*  Return true if the current chain is not at the maximum length.           */
/*                                                                           */
/*****************************************************************************/

static bool KheEjectorNotAtLimit(KHE_EJECTOR ej)
{
  bool res;
  res = ej->curr_length < ej->curr_minor_schedule->max_length &&
    ej->curr_augment_count < ej->max_augments &&
    ej->curr_repair_count < ej->max_repairs;
  if( KheEjectorCurrDebug(ej) )
  {
    if( ej->curr_length >= ej->curr_minor_schedule->max_length )
      fprintf(stderr, "%*s  at length limit %d\n",
	KheEjectorCurrDebugIndent(ej), "", ej->curr_minor_schedule->max_length);
    if( ej->curr_augment_count >= ej->max_augments )
      fprintf(stderr, "%*s  at augment limit %d\n",
	KheEjectorCurrDebugIndent(ej), "", ej->max_augments);
    if( ej->curr_repair_count >= ej->max_repairs )
      fprintf(stderr, "%*s  at repair limit %d\n",
	KheEjectorCurrDebugIndent(ej), "", ej->max_repairs);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAugmentSucceed(KHE_EJECTOR ej, KHE_VERTEX pred_v,int repair_type)*/
/*                                                                           */
/*  Do what has to be done after a successful augment, then return true.     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheEjectorStatsOneRepair(KHE_EJECTOR ej, int augment_type,
  int repair_type, bool success);

static bool KheAugmentSucceed(KHE_EJECTOR ej, KHE_VERTEX pred_v,
  int repair_type)
** , void (*on_success_fn)(void *on_success_val), void *on_success_val) **
{
  ** int i;  KHE_MONITOR m; */
  /* ***
  if( on_success_fn != NULL )
    on_success_fn(on_success_val);
  *** **

  ** promote defects **
  ** *** withdrawn
  if( !ej->no_promote_defects && ej->start_gm != ej->continue_gm )
  {
    for( i = 0;  i < KheTraceMonitorCount(pred_v->trace);  i++ )
    {
      m = KheTraceMonitor(pred_v->trace, i);
      if( KheMonitorCost(m) > KheTraceMonitorInitCost(pred_v->trace, i) &&
	  !KheGroupMonitorHasChildMonitor(ej->start_gm, m) )
	KheGroupMonitorAddChildMonitor(ej->start_gm, m);
    }
  }
  *** **

  KheEjectorStatsOneRepair(ej, pred_v->augment_type, repair_type, true);
  ** pred_v->success = true; **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorRepairEndLong(KHE_EJECTOR ej, int repair_type,            */
/*    bool success,                                                          */
/*    void (*on_success_fn)(void *on_success_val), void *on_success_val)     */
/*                                                                           */
/*  End a repair, return true if augment function should end early.          */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheEjectorRepairEndLong(KHE_EJECTOR ej, int repair_type, bool success,
  void (*on_success_fn)(void *on_success_val), void *on_success_val)
{
  KHE_VERTEX pred_v, succ_v;  KHE_MONITOR d;  int i;  KHE_COST init_cost;

  ** check and change state **
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_RUN_REPAIR,
    "KheEjectorRepairEnd called out of order");
  ej->state = KHE_EJECTOR_SOLVE_RUN;

  ** end tracing **
  KheTraceEnd(ej->continue_gm_trace);

  ** stop debugging, if required **
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "\n");

  ** boilerplate **
  if( DEBUG9 && success )
    KheFrameAssertNoClashes(ej->frame);
  ej->curr_repair_count++;
  pred_v = HaArray(ej->vertex_stack, ej->curr_length - 1);

  ** if repair was unsuccessful, forget the whole thing **
  if( !success )
  {
    KheMarkUndo(pred_v->mark);
    if( DEBUG6 )
      fprintf(stderr, "%*srepair %d not success\n",
	KheEjectorCurrDebugIndent(ej), "", repair_type);
    return false;
  }

  ** debug print of open defects **
  if( KheEjectorCurrDebug(ej) )
  {
    for( i = 0;  i < KheTraceMonitorCount(ej->continue_gm_trace);  i++ )
    {
      d = KheTraceMonitor(ej->continue_gm_trace, i);
      init_cost = KheTraceMonitorInitCost(ej->continue_gm_trace, i);
      if( KheMonitorCost(d) > init_cost )
      {
	fprintf(stderr, "%*s  new defect %.5f -> %.5f: ",
	  KheEjectorCurrDebugIndent(ej), "", KheCostShow(init_cost),
	  KheCostShow(KheMonitorCost(d)));
	KheMonitorDebug(d, 1, 0, stderr);
      }
    }
  }

  ** if successful, return true **
  if( KheAugmentSuccess(ej) )
  {
    if( DEBUG6 )
      fprintf(stderr, "%*srepair %d immediate success\n",
	KheEjectorCurrDebugIndent(ej), "", repair_type);
    if( KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s  success: immediate\n",
	KheEjectorCurrDebugIndent(ej), "");
    KheEjectorStatsAddImprovement(ej);
    return KheAugmentSucceed(ej, pred_v, repair_type, on_success_fn,
      on_success_val);
  }

  ** if not at limit and can extend the path, try that **
  if( KheEjectorNotAtLimit(ej) && KheEjectorNextVertex(ej, &succ_v) )
  {
    if( KheEjectorAugment(ej, succ_v) )
      return KheAugmentSucceed(ej, pred_v, repair_type, on_success_fn,
	on_success_val);
  }

  ** no luck **
  KheEjectorStatsOneRepair(ej, pred_v->augment_type, repair_type, false);
  KheMarkUndo(pred_v->mark);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorRepairEnd(KHE_EJECTOR ej, int repair_type, bool success)  */
/*                                                                           */
/*  Abbreviated form of KheEjectorRepairEndLong.                             */
/*                                                                           */
/*****************************************************************************/
static void KheEjectorStatsAddImprovement(KHE_EJECTOR ej);
static void KheEjectorStatsOneRepair(KHE_EJECTOR ej, int augment_type,
  int repair_type, bool success);

bool KheEjectorRepairEnd(KHE_EJECTOR ej, int repair_type, bool success)
{
  KHE_VERTEX pred_v, succ_v;  KHE_MONITOR d;  int i;  KHE_COST init_cost;

  /* check and change state */
  HnAssert(ej->state == KHE_EJECTOR_SOLVE_RUN_REPAIR,
    "KheEjectorRepairEnd called out of order");
  ej->state = KHE_EJECTOR_SOLVE_RUN;

  /* end tracing */
  KheTraceEnd(ej->continue_gm_trace);

  /* stop debugging, if required */
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "\n");

  /* boilerplate */
  /* ***
  if( DEBUG9 && success )
    KheFrameAssertNoClashes(ej->frame);
  *** */
  ej->curr_repair_count++;
  pred_v = HaArray(ej->vertex_stack, ej->curr_length - 1);

  /* if repair was unsuccessful, forget the whole thing */
  if( !success )
  {
    KheMarkUndo(pred_v->mark);
    if( KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
    return false;
  }

  /* debug print of open defects */
  if( KheEjectorCurrDebug(ej) )
  {
    for( i = 0;  i < KheTraceMonitorCount(ej->continue_gm_trace);  i++ )
    {
      d = KheTraceMonitor(ej->continue_gm_trace, i);
      init_cost = KheTraceMonitorInitCost(ej->continue_gm_trace, i);
      if( KheMonitorCost(d) > init_cost )
      {
	fprintf(stderr, "%*s  new defect %.5f -> %.5f: ",
	  KheEjectorCurrDebugIndent(ej), "", KheCostShow(init_cost),
	  KheCostShow(KheMonitorCost(d)));
	KheMonitorDebug(d, 1, 0, stderr);
      }
    }
  }

  /* if successful, return true */
  if( KheAugmentSuccess(ej) )
  {
    if( KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s  success: immediate\n",
	KheEjectorCurrDebugIndent(ej), "");
    KheEjectorStatsAddImprovement(ej);
    KheEjectorStatsOneRepair(ej, pred_v->augment_type, repair_type, true);
    if( KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
    return true;
  }

  /* if not at limit and can extend the path, try that */
  if( KheEjectorNotAtLimit(ej) && KheEjectorNextVertex(ej, &succ_v) )
  {
    if( KheEjectorAugment(ej, succ_v) )
    {
      KheEjectorStatsOneRepair(ej, pred_v->augment_type, repair_type, true);
      if( KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
      return true;
    }
  }

  /* no luck */
  KheEjectorStatsOneRepair(ej, pred_v->augment_type, repair_type, false);
  KheMarkUndo(pred_v->mark);
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "measuring performance (general)"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorInitAllStats(KHE_EJECTOR ej)                              */
/*                                                                           */
/*  Initialize all the statistics of ej.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorInitAllStats(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  HaArrayInit(ej->stats.improvement_stats, ej->arena);
  ej->stats.timer = KheTimerMake("ejector", KHE_NO_TIME, ej->arena);
  ej->stats.init_cost = 0;
  ej->stats.init_defects = 0;
  if( DEBUG7 )
    fprintf(stderr, "  %p histo init\n", (void *) ej);
  HaArrayInit(ej->stats.repair_count_histo, ej->arena);
  HaArrayInit(ej->stats.augment_count_histo, ej->arena);
  HaArrayInit(ej->stats.repair_stats, ej->arena);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorResetSolveStats(KHE_EJECTOR ej)                           */
/*                                                                           */
/*  Reset those statistics of ej concerned with an individual solve.         */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorResetSolveStats(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  HaArrayClear(ej->stats.improvement_stats);
  KheTimerResetStartTime(ej->stats.timer);
  ej->stats.init_cost = KheSolnCost(ej->soln);
  ej->stats.init_defects = KheGroupMonitorDefectCount(ej->start_gm);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorStatsOneRepair(KHE_EJECTOR ej, int augment_type,          */
/*    int repair_type, bool success)                                         */
/*                                                                           */
/*  This function should be called each time a repair is made (each time     */
/*  KheEjectorRepairEnd is called), with success == true if the repair       */
/*  causes the enclosing call to KheEjectorAugment to return true.           */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorStatsOneRepair(KHE_EJECTOR ej, int augment_type,
  int repair_type, bool success)
{
#if KHE_EJECTOR_WITH_STATS
  KHE_REPAIR_STATS repair_stats, *rs;  KHE_PAIR pair, *ps;

  /* make sure there is an entry in repairs for augment_type */
  while( HaArrayCount(ej->stats.repair_stats) <= augment_type )
  {
    repair_stats.overall.total = 0;
    repair_stats.overall.successful = 0;
    HaArrayInit(repair_stats.by_type, ej->arena);
    HaArrayAddLast(ej->stats.repair_stats, repair_stats);
  }

  /* update the repair record for augment_type */
  rs = &HaArray(ej->stats.repair_stats, augment_type);
  rs->overall.total++;
  if( success )
    rs->overall.successful++;

  /* make sure there is an entry in rs for repair_type */
  while( HaArrayCount(rs->by_type) <= repair_type )
  {
    pair.total = 0;
    pair.successful = 0;
    HaArrayAddLast(rs->by_type, pair);
  }

  /* update the repair record for repair_type */
  ps = &HaArray(rs->by_type, repair_type);
  ps->total++;
  if( success )
    ps->successful++;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorStatsAddImprovement(KHE_EJECTOR ej)                       */
/*                                                                           */
/*  This function should be called each time an overall improvement is       */
/*  found.                                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheEjectorStatsAddImprovement(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  KHE_EJECTOR_IMPROVEMENT_STATS is;

  /* store sequence info about this chain */
  is.repair_count = ej->curr_length;
  HnAssert(is.repair_count >= 1,
    "KheEjectorStatsAddImprovement internal error 1");
  is.time = KheTimerElapsedTime(ej->stats.timer);
  is.cost = KheSolnCost(ej->soln);
  is.defects = KheGroupMonitorDefectCount(ej->start_gm);
  HaArrayAddLast(ej->stats.improvement_stats, is);

  /* update repair_count_histo */
  while( HaArrayCount(ej->stats.repair_count_histo) < is.repair_count )
    HaArrayAddLast(ej->stats.repair_count_histo, 0);
  if( DEBUG7 )
    fprintf(stderr, "  %p histo[%d]++\n", (void *) ej, is.repair_count - 1);
  ++HaArray(ej->stats.repair_count_histo, is.repair_count - 1);

  /* update augment_count_histo */
  while( HaArrayCount(ej->stats.augment_count_histo) < ej->curr_augment_count )
    HaArrayAddLast(ej->stats.augment_count_histo, 0);
  ++HaArray(ej->stats.augment_count_histo, ej->curr_augment_count - 1);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementCount(KHE_EJECTOR ej)                           */
/*                                                                           */
/*  Return the number of improvements.                                       */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementCount(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArrayCount(ej->stats.improvement_stats);
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementNumberOfRepairs(KHE_EJECTOR ej, int i)          */
/*                                                                           */
/*  Return the number of repairs of the i'th improvement.                    */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementNumberOfRepairs(KHE_EJECTOR ej, int i)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArray(ej->stats.improvement_stats, i).repair_count;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  float KheEjectorImprovementTime(KHE_EJECTOR ej, int i)                   */
/*                                                                           */
/*  Return the elapsed time between when KheEjectorSolve began and when the  */
/*  i'th improvement was applied.                                            */
/*                                                                           */
/*****************************************************************************/

float KheEjectorImprovementTime(KHE_EJECTOR ej, int i)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArray(ej->stats.improvement_stats, i).time;
#else
  return 0.0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEjectorImprovementCost(KHE_EJECTOR ej, int i)                */
/*                                                                           */
/*  Return the solution cost after the i'th improvement.                     */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEjectorImprovementCost(KHE_EJECTOR ej, int i)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArray(ej->stats.improvement_stats, i).cost;
#else
  return KheCost(0, 0);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementDefects(KHE_EJECTOR ej, int i)                  */
/*                                                                           */
/*  Return the number of main loop defects after the i'th improvement.       */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementDefects(KHE_EJECTOR ej, int i)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArray(ej->stats.improvement_stats, i).defects;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEjectorInitCost(KHE_EJECTOR ej)                              */
/*                                                                           */
/*  Return the initial cost.                                                 */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEjectorInitCost(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  return ej->stats.init_cost;
#else
  return KheCost(0, 0);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorInitDefects(KHE_EJECTOR ej)                                */
/*                                                                           */
/*  Return the initial number of defects.                                    */
/*                                                                           */
/*****************************************************************************/

int KheEjectorInitDefects(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  return ej->stats.init_defects;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementRepairHistoMax(KHE_EJECTOR ej)                  */
/*                                                                           */
/*  Return the maximum number of repairs in any improvement, or 0 if         */
/*  there have been no improvements.                                         */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementRepairHistoMax(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  if( DEBUG7 )
    fprintf(stderr, "  %p histo max %d\n", (void *) ej,
      HaArrayCount(ej->stats.repair_count_histo));
  return HaArrayCount(ej->stats.repair_count_histo);
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementRepairHistoFrequency(KHE_EJECTOR ej,            */
/*    int repair_count)                                                      */
/*                                                                           */
/*  Return the number of improvements with the given number of repairs.      */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementRepairHistoFrequency(KHE_EJECTOR ej, int repair_count)
{
#if KHE_EJECTOR_WITH_STATS
  HnAssert(repair_count >= 1,
    "KheEjectorImprovementRepairHistoFrequency: repair_count < 1");
  HnAssert(repair_count <= HaArrayCount(ej->stats.repair_count_histo),
    "KheEjectorImprovementRepairHistoFrequency: "
    "repair_count > KheEjectorImprovementRepairHistoMax(ej)");
  return HaArray(ej->stats.repair_count_histo, repair_count - 1);
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementRepairHistoTotal(KHE_EJECTOR ej)                */
/*                                                                           */
/*  Return the total number of improvements.                                 */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementRepairHistoTotal(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  int no_of_chains, i, num;
  no_of_chains = 0;
  HaArrayForEach(ej->stats.repair_count_histo, num, i)
    no_of_chains += num;
  return no_of_chains;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  float KheEjectorImprovementRepairHistoAverage(KHE_EJECTOR ej)            */
/*                                                                           */
/*  Return the average number of repairs of successful improvements.         */
/*                                                                           */
/*****************************************************************************/

float KheEjectorImprovementRepairHistoAverage(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  int no_of_chains, total_length_of_chains, i, num;
  no_of_chains = 0; total_length_of_chains = 0;
  HaArrayForEach(ej->stats.repair_count_histo, num, i)
  {
    no_of_chains += num;
    total_length_of_chains += num * (i + 1);
  }
  HnAssert(no_of_chains > 0, "KheEjectorChainHistoAverage: no chains");
  return (float) total_length_of_chains / (float) no_of_chains;
#else
  return 0.0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementAugmentHistoMax(KHE_EJECTOR ej)                 */
/*                                                                           */
/*  Return the maximum number of augments in any improvement, or 0 if        */
/*  there have been no improvements.                                         */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementAugmentHistoMax(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArrayCount(ej->stats.augment_count_histo);
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementAugmentHistoFrequency(KHE_EJECTOR ej,           */
/*    int repair_count)                                                      */
/*                                                                           */
/*  Return the number of improvements with the given number of augments.     */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementAugmentHistoFrequency(KHE_EJECTOR ej,
  int augment_count)
{
#if KHE_EJECTOR_WITH_STATS
  HnAssert(augment_count >= 1,
    "KheEjectorImprovementAugmentHistoFrequency: repair_count < 1");
  HnAssert(augment_count <= HaArrayCount(ej->stats.augment_count_histo),
    "KheEjectorImprovementAugmentHistoFrequency: "
    "augment_count > KheEjectorImprovementAugmentHistoMax(ej)");
  return HaArray(ej->stats.augment_count_histo, augment_count - 1);
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorImprovementAugmentHistoTotal(KHE_EJECTOR ej)               */
/*                                                                           */
/*  Return the total number of improvements.                                 */
/*                                                                           */
/*****************************************************************************/

int KheEjectorImprovementAugmentHistoTotal(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  int no_of_chains, i, num;
  no_of_chains = 0;
  HaArrayForEach(ej->stats.augment_count_histo, num, i)
    no_of_chains += num;
  return no_of_chains;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  float KheEjectorImprovementAugmentHistoAverage(KHE_EJECTOR ej)           */
/*                                                                           */
/*  Return the average number of augments of successful improvements.        */
/*                                                                           */
/*****************************************************************************/

float KheEjectorImprovementAugmentHistoAverage(KHE_EJECTOR ej)
{
#if KHE_EJECTOR_WITH_STATS
  int no_of_chains, total_length_of_chains, i, num;
  no_of_chains = 0; total_length_of_chains = 0;
  HaArrayForEach(ej->stats.augment_count_histo, num, i)
  {
    no_of_chains += num;
    total_length_of_chains += num * (i + 1);
  }
  HnAssert(no_of_chains > 0, "KheEjectorChainAugmentHistoAverage: no chains");
  return (float) total_length_of_chains / (float) no_of_chains;
#else
  return 0.0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorTotalRepairs(KHE_EJECTOR ej, int augment_type)             */
/*                                                                           */
/*  Return the total number of augment_type repairs.                         */
/*                                                                           */
/*****************************************************************************/

int KheEjectorTotalRepairs(KHE_EJECTOR ej, int augment_type)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArrayCount(ej->stats.repair_stats) <= augment_type ? 0 :
    HaArray(ej->stats.repair_stats, augment_type).overall.total;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorSuccessfulRepairs(KHE_EJECTOR ej, int augment_type)        */
/*                                                                           */
/*  Return the number of successful augment_type repairs.                    */
/*                                                                           */
/*****************************************************************************/

int KheEjectorSuccessfulRepairs(KHE_EJECTOR ej, int augment_type)
{
#if KHE_EJECTOR_WITH_STATS
  return HaArrayCount(ej->stats.repair_stats) <= augment_type ? 0 :
    HaArray(ej->stats.repair_stats, augment_type).overall.successful;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorTotalRepairsByType(KHE_EJECTOR ej, int augment_type,       */
/*    int repair_type)                                                       */
/*                                                                           */
/*  Return the number of (augment_type, repair_type) pairs.                  */
/*                                                                           */
/*****************************************************************************/

int KheEjectorTotalRepairsByType(KHE_EJECTOR ej, int augment_type,
  int repair_type)
{
#if KHE_EJECTOR_WITH_STATS
  KHE_REPAIR_STATS rs;
  if( HaArrayCount(ej->stats.repair_stats) <= augment_type )
    return 0;
  rs = HaArray(ej->stats.repair_stats, augment_type);
  if( HaArrayCount(rs.by_type) <= repair_type )
    return 0;
  return HaArray(rs.by_type, repair_type).total;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorSuccessfulRepairsByType(KHE_EJECTOR ej, int augment_type,  */
/*    int repair_type)                                                       */
/*                                                                           */
/*  Return the number of successful (augment_type, repair_type) pairs.       */
/*                                                                           */
/*****************************************************************************/

int KheEjectorSuccessfulRepairsByType(KHE_EJECTOR ej, int augment_type,
  int repair_type)
{
#if KHE_EJECTOR_WITH_STATS
  KHE_REPAIR_STATS rs;
  if( HaArrayCount(ej->stats.repair_stats) <= augment_type )
    return 0;
  rs = HaArray(ej->stats.repair_stats, augment_type);
  if( HaArrayCount(rs.by_type) <= repair_type )
    return 0;
  return HaArray(rs.by_type, repair_type).successful;
#else
  return 0;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "measuring performance (augment info and augment types)"       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_INFO KheAugmentInfoMake(int augment_type,                    */
/*    char *augment_label)                                                   */
/*                                                                           */
/*  Make a new augment_info object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_AUGMENT_INFO KheAugmentInfoMake(int augment_type,
  char *augment_label, HA_ARENA a)
{
  KHE_AUGMENT_INFO res;
  HnAssert(augment_label != NULL,
    "KheAugmentInfoMake: augment_label == NULL");
  HaMake(res, a);
  res->augment_type = augment_type;
  res->augment_label = HnStringCopy(augment_label, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAugmentInfoFind(ARRAY_KHE_AUGMENT_INFO *aai,                     */
/*    int augment_type, KHE_AUGMENT_INFO *ai)                                */
/*                                                                           */
/*  If *aai contains a repair info object with the given repair_type,        */
/*  return true and set *ai to that object.  Otherwise return false.         */
/*                                                                           */
/*****************************************************************************/

static bool KheAugmentInfoFind(ARRAY_KHE_AUGMENT_INFO *aai,
  int augment_type, KHE_AUGMENT_INFO *ai)
{
  KHE_AUGMENT_INFO ai2;  int i;
  HaArrayForEach(*aai, ai2, i)
    if( ai2->augment_type == augment_type )
    {
      *ai = ai2;
      return true;
    }
  *ai = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddAugmentType(KHE_EJECTOR ej, int augment_type,          */
/*    char *augment_label)                                                   */
/*                                                                           */
/*  Declare an augment type.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddAugmentType(KHE_EJECTOR ej, int augment_type,
  char *augment_label)
{
  KHE_AUGMENT_INFO res;
  HnAssert(ej->state == KHE_EJECTOR_MAKE_SETTING,
    "KheEjectorAddAugmentType called out of order");
  HnAssert( !KheAugmentInfoFind(&ej->augment_info_array, augment_type,
    &res), "KheEjectorAddAugmentType: augment_type already declared");
  res = KheAugmentInfoMake(augment_type, augment_label, ej->arena);
  HaArrayAddLast(ej->augment_info_array, res);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorAugmentTypeCount(KHE_EJECTOR ej)                           */
/*                                                                           */
/*  Return the number of declared augment types.                             */
/*                                                                           */
/*****************************************************************************/

int KheEjectorAugmentTypeCount(KHE_EJECTOR ej)
{
  return HaArrayCount(ej->augment_info_array);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorAugmentType(KHE_EJECTOR ej, int i)                         */
/*                                                                           */
/*  Return the i'th declared augment type.                                   */
/*                                                                           */
/*****************************************************************************/

int KheEjectorAugmentType(KHE_EJECTOR ej, int i)
{
  KHE_AUGMENT_INFO ai;
  ai = HaArray(ej->augment_info_array, i);
  return ai->augment_type;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEjectorAugmentTypeLabel(KHE_EJECTOR ej, int augment_type)       */
/*                                                                           */
/*  Return the label of augment_type.                                        */
/*                                                                           */
/*****************************************************************************/

char *KheEjectorAugmentTypeLabel(KHE_EJECTOR ej, int augment_type)
{
  KHE_AUGMENT_INFO ai;
  if( !KheAugmentInfoFind(&ej->augment_info_array, augment_type, &ai) )
    HnAbort("KheAugmentTypeLabel: augment_type not declared");
  return ai->augment_label;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "measuring performance (repair info and repair types)"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_REPAIR_INFO KheRepairInfoMake(int repair_type, char *repair_label)   */
/*                                                                           */
/*  Make a new repair_info object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_REPAIR_INFO KheRepairInfoMake(int repair_type, char *repair_label,
  HA_ARENA a)
{
  KHE_REPAIR_INFO res;
  HnAssert(repair_label != NULL,
    "KheRepairInfoMake: repair_label == NULL");
  HaMake(res, a);
  res->repair_type = repair_type;
  res->repair_label = HnStringCopy(repair_label, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRepairInfoFind(ARRAY_KHE_REPAIR_INFO *ari, int repair_type,      */
/*    KHE_REPAIR_INFO *ri)                                                   */
/*                                                                           */
/*  If *ari contains a repair info object with the given repair_type,        */
/*  return true and set *ri to that object.  Otherwise return false.         */
/*                                                                           */
/*****************************************************************************/

static bool KheRepairInfoFind(ARRAY_KHE_REPAIR_INFO *ari, int repair_type,
  KHE_REPAIR_INFO *ri)
{
  KHE_REPAIR_INFO ri2;  int i;
  HaArrayForEach(*ari, ri2, i)
    if( ri2->repair_type == repair_type )
    {
      *ri = ri2;
      return true;
    }
  *ri = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEjectorAddRepairType(KHE_EJECTOR ej, int repair_type,            */
/*    char *repair_label)                                                    */
/*                                                                           */
/*  Declare a repair type.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheEjectorAddRepairType(KHE_EJECTOR ej, int repair_type,
  char *repair_label)
{
  KHE_REPAIR_INFO res;
  HnAssert(ej->state == KHE_EJECTOR_MAKE_SETTING,
    "KheEjectorAddRepairType called out of order");
  HnAssert( !KheRepairInfoFind(&ej->repair_info_array, repair_type, &res),
    "KheEjectorAddRepairType: repair_type already declared");
  res = KheRepairInfoMake(repair_type, repair_label, ej->arena);
  HaArrayAddLast(ej->repair_info_array, res);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorRepairTypeCount(KHE_EJECTOR ej)                            */
/*                                                                           */
/*  Return the number of declared repair types.                              */
/*                                                                           */
/*****************************************************************************/

int KheEjectorRepairTypeCount(KHE_EJECTOR ej)
{
  return HaArrayCount(ej->repair_info_array);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEjectorRepairType(KHE_EJECTOR ej, int i)                          */
/*                                                                           */
/*  Return the i'th declared repair type.                                    */
/*                                                                           */
/*****************************************************************************/

int KheEjectorRepairType(KHE_EJECTOR ej, int i)
{
  KHE_REPAIR_INFO ri;
  ri = HaArray(ej->repair_info_array, i);
  return ri->repair_type;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEjectorRepairTypeLabel(KHE_EJECTOR ej, int repair_type)         */
/*                                                                           */
/*  Return the label of repair_type.                                         */
/*                                                                           */
/*****************************************************************************/

char *KheEjectorRepairTypeLabel(KHE_EJECTOR ej, int repair_type)
{
  KHE_REPAIR_INFO ri;
  if( !KheRepairInfoFind(&ej->repair_info_array, repair_type, &ri) )
    HnAbort("KheEjectorRepairTypeLabel: repair_type not declared");
  return ri->repair_label;
}

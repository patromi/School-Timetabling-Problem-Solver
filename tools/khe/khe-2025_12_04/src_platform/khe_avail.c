
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
/*  FILE:         khe_avail.c                                                */
/*  DESCRIPTION:  Resource availability                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0	/* high-level stuff */
#define DEBUG2 0
#define DEBUG4 0
#define DEBUG7 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASST_TIME_INFO - preassignment information for one time           */
/*                                                                           */
/*****************************************************************************/

/* *** not doing it this way now
typedef struct khe_preasst_time_info_rec {
  KHE_TIME			time;			** the time          **
  bool				all_preassigned;	** all tasks preass  **
  ARRAY_KHE_RESOURCE		resources;		** resources         **
} *KHE_PREASST_TIME_INFO;

typedef HA_ARRAY(KHE_PREASST_TIME_INFO) ARRAY_KHE_PREASST_TIME_INFO;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASST_INFO - preassignment information for one resource type       */
/*                                                                           */
/*****************************************************************************/

/* *** not doing it this way now
typedef struct khe_preasst_info_rec {
  ** KHE_RESOURCE_TYPE		resource_type; **	** the resource type **
  ARRAY_KHE_PREASST_TIME_INFO	time_info;		** indexed by time   **
} *KHE_PREASST_INFO;

typedef HA_ARRAY(KHE_PREASST_INFO) ARRAY_KHE_PREASST_INFO;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET - one set of event resources                                   */
/*                                                                           */
/*  Implementation note.  We very cleverly do not store the actual event     */
/*  resources; instead we just store the information that we need about      */
/*  them, which is their minimum and maximum workload per time.              */
/*                                                                           */
/*  If the set is empty these values are undefined.  We use the test         */
/*  min_workload_per_time == FLT_MAX to detect that case.                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_erset_rec {
  float			min_workload_per_time;
  float			max_workload_per_time;
} *KHE_ERSET;

typedef HA_ARRAY(KHE_ERSET) ARRAY_KHE_ERSET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET_TIME - sets of event resources for a preassigned time          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_erset_time_rec {
  KHE_ERSET			pu_set;			/* Spu(t)            */
  ARRAY_KHE_ERSET		pp_set;			/* Spp(t, r)         */
} *KHE_ERSET_TIME;

typedef HA_ARRAY(KHE_ERSET_TIME) ARRAY_KHE_ERSET_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET_ALL - sets of event resources                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_erset_all_rec {
  KHE_ERSET			uu_set;			/* Suu               */
  ARRAY_KHE_ERSET_TIME   	t_set;			/* Spu(t), Spp(t, r) */
  ARRAY_KHE_ERSET		up_set;			/* Sup(r)            */
} *KHE_ERSET_ALL;

typedef HA_ARRAY(KHE_ERSET_ALL) ARRAY_KHE_ERSET_ALL;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT - a limit, either int or float                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_limit_rec {
  bool				is_float;
  union {
    int				int_val;
    float			float_val;
  } u;
} KHE_LIMIT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_NODE                                                           */
/*                                                                           */
/*  One node in the graph for which we need an independent set.              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_avail_node_rec {
  KHE_AVAIL_NODE_TYPE		type;			/* node type         */
  KHE_LIMIT			limit;
  KHE_TIME_SET			time_set;		/* the times         */
  KHE_MONITOR			monitor;		/* provenance        */
} *KHE_AVAIL_NODE;

typedef HA_ARRAY(KHE_AVAIL_NODE) ARRAY_KHE_AVAIL_NODE;
typedef HP_TABLE(KHE_AVAIL_NODE) TABLE_KHE_AVAIL_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SET - an independent set of avail nodes                        */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_AVAIL_SET_TIMES,
  KHE_AVAIL_SET_WORKLOAD,
} KHE_AVAIL_SET_TYPE;

typedef struct khe_avail_set_rec {
  KHE_AVAIL_SET_TYPE		type;
  KHE_LIMIT			limit;
  /* int			total_times; */
  ARRAY_KHE_AVAIL_NODE		nodes;
} *KHE_AVAIL_SET;

typedef HA_ARRAY(KHE_AVAIL_SET) ARRAY_KHE_AVAIL_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LOAD_SOLVER - solver for finding load (busy times or workload)       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_load_solver_rec {
  TABLE_KHE_AVAIL_NODE		node_table;	/* avail nodes for resource  */
  ARRAY_KHE_AVAIL_NODE		node_array;	/* avail nodes for resource  */
  KHE_AVAIL_SET			best_avail_set; /* best load avail set*/
} *KHE_LOAD_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SOLVER - an avail solver                                       */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TIME_SET) ARRAY_KHE_TIME_SET;

struct khe_avail_solver_rec {

  /* free lists */
  ARRAY_KHE_TIME_SET	time_set_free_list;		/* free list */
  ARRAY_KHE_ERSET	erset_free_list;		/* free list */
  ARRAY_KHE_ERSET_TIME	erset_time_free_list;		/* free list */
  ARRAY_KHE_ERSET_ALL	erset_all_free_list;		/* free list */
  ARRAY_KHE_AVAIL_NODE	avail_node_free_list;		/* free list */
  ARRAY_KHE_AVAIL_SET	avail_set_free_list;		/* free list */

  /* constant fields */
  HA_ARENA		arena;		    /* memory used by solver */
  KHE_SOLN		soln;		    /* soln being worked on  */
  KHE_INSTANCE		instance;	    /* soln's instance       */

  /* fields dependent on curr_rt, or even curr_r */
  KHE_RESOURCE_TYPE	curr_rt;		/* current resource type */
  KHE_RESOURCE		curr_r;			/* current resource      */
  KHE_ERSET_ALL		erset_all;		/* for curr_rt           */
  HA_ARRAY_BOOL		zero_times;		/* indexed by time       */
  KHE_LOAD_SOLVER	busy_times_solver[2];	/* for busy times        */
  KHE_LOAD_SOLVER	workload_solver[2];	/* for busy times        */
  KHE_LOAD_SOLVER	best_busy_times_solver;	/* best for busy times   */
  KHE_LOAD_SOLVER	best_workload_solver;	/* best for workload     */

  /* scratch fields */
  HA_ARRAY_INT		tmp_cluster_limits; /* cluster constraints   */
  ARRAY_KHE_TIME_SET	tmp_cluster_time_sets; /* cluster constraints*/
  HA_ARRAY_FLOAT	tmp_min_workloads;  /* workload constraints  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TIME_SET"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheAvailSolverMakeTimeSet(KHE_AVAIL_SOLVER as)              */
/*                                                                           */
/*  Get a new empty time set from as's free list, or make one.               */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_SET KheAvailSolverMakeTimeSet(KHE_AVAIL_SOLVER as)
{
  KHE_TIME_SET res;
  if( HaArrayCount(as->time_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->time_set_free_list);
    KheTimeSetClear(res);
  }
  else
    res = KheTimeSetMake(as->instance, as->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverFreeTimeSet(KHE_AVAIL_SOLVER as, KHE_TIME_SET ts)     */
/*                                                                           */
/*  Free ts.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverFreeTimeSet(KHE_AVAIL_SOLVER as, KHE_TIME_SET ts)
{
  HaArrayAddLast(as->time_set_free_list, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheAvailSolverCopyTimeSet(KHE_AVAIL_SOLVER as,              */
/*    KHE_TIME_SET ts)                                                       */
/*                                                                           */
/*  Return a copy of ts.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_SET KheAvailSolverCopyTimeSet(KHE_AVAIL_SOLVER as,
  KHE_TIME_SET ts)
{
  KHE_TIME_SET res;
  res = KheAvailSolverMakeTimeSet(as);
  KheTimeSetCopyElements(res, ts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PREASST_TIME_INFO"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASST_TIME_INFO KhePreasstTimeInfoMake(KHE_AVAIL_SOLVER as,        */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Make a new, initially empty preasst time info object.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_PREASST_TIME_INFO KhePreasstTimeInfoMake(KHE_AVAIL_SOLVER as,
  KHE_TIME t)
{
  KHE_PREASST_TIME_INFO res;
  if( HaArrayCount(as->preasst_time_info_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->preasst_time_info_free_list);
    HaArrayClear(res->resources);
  }
  else
  {
    HaMake(res, as->arena);
    HaArrayInit(res->resources, as->arena);
  }
  res->time = t;
  res->all_preassigned = true;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstTimeInfoFree(KHE_PREASST_TIME_INFO ti,                    */
/*    KHE_AVAIL_SOLVER as)                                                   */
/*                                                                           */
/*  Free ti, recycling its memory through as's free lists.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstTimeInfoFree(KHE_PREASST_TIME_INFO ti,
  KHE_AVAIL_SOLVER as)
{
  HaArrayAddLast(as->preasst_time_info_free_list, ti);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstTimeInfoAddResource(KHE_PREASST_TIME_INFO ti,             */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Add preassigned resource r to ti; or if r is NULL, it means that not     */
/*  all event resources running at ti have preassigned resources.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstTimeInfoAddResource(KHE_PREASST_TIME_INFO ti,
  KHE_RESOURCE r)
{
  if( r == NULL )
    ti->all_preassigned = false;
  else
    HaArrayAddLast(ti->resources, r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstTimeInfoDebug(KHE_PREASST_TIME_INFO ti,                   */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ti onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstTimeInfoDebug(KHE_PREASST_TIME_INFO ti,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE r;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[%s %sall preassigned: ", KheTimeId(ti->time),
    ti->all_preassigned ? "" : "not ");
  HaArrayForEach(ti->resources, r, i)
    fprintf(fp, "%s%s", i == 0 ? "" : ", ", KheResourceId(r));
  fprintf(fp, "]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PREASST_INFO"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASST_INFO KhePreasstInfoMake(KHE_AVAIL_SOLVER as)                 */
/*                                                                           */
/*  Make a new, empty preasst info object.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_PREASST_INFO KhePreasstInfoMake(KHE_AVAIL_SOLVER as)
{
  KHE_PREASST_INFO res;
  if( HaArrayCount(as->preasst_info_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->preasst_info_free_list);
    HaArrayClear(res->time_info);
  }
  else
  {
    HaMake(res, as->arena);
    HaArrayInit(res->time_info, as->arena);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstInfoFree(KHE_PREASST_INFO pi, KHE_AVAIL_SOLVER as)        */
/*                                                                           */
/*  Free pi, including freeing its preasst time info objects.                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstInfoFree(KHE_PREASST_INFO pi, KHE_AVAIL_SOLVER as)
{
  int i;
  HaArrayAppend(as->preasst_time_info_free_list, pi->time_info, i);
  HaArrayAddLast(as->preasst_info_free_list, pi);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstInfoAddEvent(KHE_PREASST_INFO pi, KHE_EVENT e)            */
/*                                                                           */
/*  Examine e and update pi depending on what is found.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstInfoAddEvent(KHE_PREASST_INFO pi, KHE_EVENT e,
  KHE_RESOURCE_TYPE rt)
{
  KHE_TIME start_t, t;  KHE_PREASST_TIME_INFO ti;  int i, j;
  KHE_EVENT_RESOURCE er;
  start_t = KheEventPreassignedTime(e);
  HnAssert(start_t != NULL, "KhePreasstInfoAddEvent internal error");
  for( i = 0;  i < KheEventDuration(e);  i++ )
  {
    t = KheTimeNeighbour(start_t, i);
    ti = HaArray(pi->time_info, KheTimeIndex(t));
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      if( KheEventResourceResourceType(er) == rt )
        KhePreasstTimeInfoAddResource(ti,
	  KheEventResourcePreassignedResource(er));
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstInfoCompressTimeInfo(KHE_PREASST_INFO pi,                 */
/*    KHE_AVAIL_SOLVER as)                                                   */
/*                                                                           */
/*  Delete and free all unpreassigned time info records.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstInfoCompressTimeInfo(KHE_PREASST_INFO pi,
  KHE_AVAIL_SOLVER as)
{
  int j, k, dropped;  KHE_PREASST_TIME_INFO ti;
  ** j is where to put the next all preassigned ti; k is the next to try **
  j = dropped = 0;
  HaArrayForEach(pi->time_info, ti, k)
  {
    if( !ti->all_preassigned )
    {
      if( DEBUG3 )
      {
	fprintf(stderr, "  compression at %d freeing ", k);
	KhePreasstTimeInfoDebug(ti, 2, 0, stderr);
      }
      KhePreasstTimeInfoFree(ti, as), dropped++;
    }
    else
    {
      if( DEBUG3 )
      {
	fprintf(stderr, "  compression at %d moving to %d ", k, j);
	KhePreasstTimeInfoDebug(ti, 2, 0, stderr);
      }
      HaArrayPut(pi->time_info, j++, ti);
    }
  }
  HaArrayDeleteLastSlice(pi->time_info, dropped);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASST_INFO KhePreasstInfoBuild(KHE_AVAIL_SOLVER as)                 */
/*                                                                           */
/*  Make and set up a preasst_info object for resource type as->curr_rt.     */
/*                                                                           */
/*****************************************************************************/
/* ***
static void KhePreasstInfoDebug(KHE_PREASST_INFO pi, int verbosity, int indent,
  FILE *fp);

static KHE_PREASST_INFO KhePreasstInfoBuild(KHE_AVAIL_SOLVER as)
{
  KHE_PREASST_INFO res;  KHE_INSTANCE ins;  KHE_PREASST_TIME_INFO ti;
  KHE_TIME t;  int i;
  if( DEBUG3 )
    fprintf(stderr, "[ KhePreasstInfoBuild(as, %s)\n",
      KheResourceTypeId(as->curr_rt));

  ** return NULL if not all events have preassigned times **
  if( !KheInstanceAllEventsHavePreassignedTimes(as->instance) )
    return NULL;

  ** make the object and its time info components, one for each time **
  res = KhePreasstInfoMake(as);
  ins = KheResourceTypeInstance(as->curr_rt);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
    ti = KhePreasstTimeInfoMake(as, t);
    HaArrayAddLast(res->time_info, ti);
  }

  ** set the all_preassigned and resources fields of the time info objects **
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
    KhePreasstInfoAddEvent(res, KheInstanceEvent(ins, i), as->curr_rt);

  ** compress the time info objects **
  if( DEBUG3 )
  {
    fprintf(stderr, "  KhePreasstInfoBuild before compressing:\n");
    KhePreasstInfoDebug(res, 2, 4, stderr);
  }
  KhePreasstInfoCompressTimeInfo(res, as);

  ** uniqueify the resources arrays of the time info fields **
  HaArrayForEach(res->time_info, ti, i)
    HaArraySortUnique(ti->resources, &KheResourceCmp);
  if( DEBUG3 )
  {
    fprintf(stderr, "  KhePreasstInfoBuild at end\n");
    KhePreasstInfoDebug(res, 2, 4, stderr);
    fprintf(stderr, "] KhePreasstInfoBuild returning\n");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreasstInfoDebug(KHE_PREASST_INFO pi, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of pi onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePreasstInfoDebug(KHE_PREASST_INFO pi, int verbosity,
  int indent, FILE *fp)
{
  KHE_PREASST_TIME_INFO ti;  int i;
  fprintf(fp, "%*s[ PreasstInfo\n", indent, "");
  ** ***
  fprintf(fp, "%*s[ PreasstInfo(%s)\n", indent, "",
    KheResourceTypeId(pi->resource_type));
  *** **
  HaArrayForEach(pi->time_info, ti, i)
    KhePreasstTimeInfoDebug(ti, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ERSET"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET KheErSetMake(KHE_AVAIL_SOLVER as)                              */
/*                                                                           */
/*  Make a new event resource set, initially empty.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_ERSET KheErSetMake(KHE_AVAIL_SOLVER as)
{
  KHE_ERSET res;
  if( HaArrayCount(as->erset_free_list) > 0 )
    res = HaArrayLastAndDelete(as->erset_free_list);
  else
    HaMake(res, as->arena);
  res->min_workload_per_time = FLT_MAX;
  res->max_workload_per_time = 0.0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetFree(KHE_ERSET erset, KHE_AVAIL_SOLVER as)                  */
/*                                                                           */
/*  Free erset by returning it to the free list in as.                       */
/*                                                                           */
/*****************************************************************************/

static void KheErSetFree(KHE_ERSET erset, KHE_AVAIL_SOLVER as)
{
  HaArrayAddLast(as->erset_free_list, erset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetAddEventResource(KHE_ERSET erset, KHE_EVENT_RESOURCE er)    */
/*                                                                           */
/*  Add er to erset.  We don't store the event resource, we just update      */
/*  the min_workload_per_time and max_workload_per_time fields.              */
/*                                                                           */
/*****************************************************************************/

static void KheErSetAddEventResource(KHE_ERSET erset, KHE_EVENT_RESOURCE er)
{
  float workload_per_time;
  workload_per_time = KheEventResourceWorkloadPerTime(er);
  if( workload_per_time < erset->min_workload_per_time )
    erset->min_workload_per_time = workload_per_time;
  if( workload_per_time > erset->max_workload_per_time )
    erset->max_workload_per_time = workload_per_time;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheErSetIsEmpty(KHE_ERSET erset)                                    */
/*                                                                           */
/*  Return true if erset is empty.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheErSetIsEmpty(KHE_ERSET erset)
{
  return erset->min_workload_per_time == FLT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetUnion(KHE_ERSET dst_erset, KHE_ERSET src_erset)             */
/*                                                                           */
/*  Replace dst_erset by its union with src_erset.                           */
/*                                                                           */
/*****************************************************************************/

static void KheErSetUnion(KHE_ERSET dst_erset, KHE_ERSET src_erset)
{
  if( src_erset->min_workload_per_time < dst_erset->min_workload_per_time )
    dst_erset->min_workload_per_time = src_erset->min_workload_per_time;
  if( src_erset->max_workload_per_time > dst_erset->max_workload_per_time )
    dst_erset->max_workload_per_time = src_erset->max_workload_per_time;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheFloatShow(float val, char buff[50])                             */
/*                                                                           */
/*  Show val, using buff for scratch memory.                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheFloatShow(float val, char buff[50])
{
  if( val == FLT_MAX )
    sprintf(buff, "%s", "inf");
  else
    sprintf(buff, "%.1f", val);
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetDebug(KHE_ERSET erset, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of erset onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheErSetDebug(KHE_ERSET erset, int verbosity, int indent, FILE *fp)
{
  char buff1[50], buff2[50];
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s:%s", KheFloatShow(erset->min_workload_per_time, buff1),
    KheFloatShow(erset->max_workload_per_time, buff2));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ERSET_TIME"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET_TIME KheErSetTimeMake(KHE_AVAIL_SOLVER as)                     */
/*                                                                           */
/*  Make a new erset time object whose sets are initially empty.             */
/*                                                                           */
/*****************************************************************************/

static KHE_ERSET_TIME KheErSetTimeMake(KHE_AVAIL_SOLVER as)
{
  KHE_ERSET_TIME res;  int i, count;

  /* make the basic object */
  if( HaArrayCount(as->erset_time_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->erset_time_free_list);
    HaArrayClear(res->pp_set);
  }
  else
  {
    HaMake(res, as->arena);
    HaArrayInit(res->pp_set, as->arena);
  }

  /* add pu_set */
  res->pu_set = KheErSetMake(as);

  /* add one erset to pp_set for each resource of as->curr_rt */
  count = KheResourceTypeResourceCount(as->curr_rt);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->pp_set, KheErSetMake(as));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetTimeFree(KHE_ERSET_TIME et, KHE_AVAIL_SOLVER as)            */
/*                                                                           */
/*  Free et by returning it to the free list in as.  This includes freeing   */
/*  its erset objects.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheErSetTimeFree(KHE_ERSET_TIME et, KHE_AVAIL_SOLVER as)
{
  int i;
  KheErSetFree(et->pu_set, as);
  HaArrayAppend(as->erset_free_list, et->pp_set, i);
  HaArrayAddLast(as->erset_time_free_list, et);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWorkloadTimeContainsResource(KHE_WORKLOAD_TIME wt,               */
/*    KHE_RESOURCE r, KHE_WORKLOAD_RESOURCE *wr)                             */
/*                                                                           */
/*  If wt contains a workload resource object for r, return true with *wr    */
/*  set to that object.  Otherwise return false with *wr set to NULL.        */
/*                                                                           */
/*****************************************************************************/

/* *** not doing it this way now
static bool KheWorkloadTimeContainsResource(KHE_WORKLOAD_TIME wt,
  KHE_RESOURCE r, KHE_WORKLOAD_RESOURCE *wr)
{
  KHE_WORKLOAD_RESOURCE wr2;  int i;
  HaArrayForEach(wt->workload_per_resource, wr2, i)
    if( wr2->resource == r )
      return *wr = wr2, true;
  return *wr = NULL, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetTimeDebug(KHE_ERSET_TIME et, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of et onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheErSetTimeDebug(KHE_ERSET_TIME et, int verbosity,
  int indent, FILE *fp)
{
  KHE_ERSET erset;  int i;
  fprintf(fp, "%*s[ ", indent, "");
  KheErSetDebug(et->pu_set, verbosity, 0, fp);
  HaArrayForEach(et->pp_set, erset, i)
  {
    fprintf(fp, "%s", i == 0 ? "; " : ", ");
    KheErSetDebug(erset, verbosity, -1, fp);
  }
  fprintf(fp, " ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ERSET_ALL"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET_ALL KheErSetAllMake(KHE_AVAIL_SOLVER as)                       */
/*                                                                           */
/*  Make a new erset_all object with empty event resource sets.              */
/*                                                                           */
/*****************************************************************************/

static KHE_ERSET_ALL KheErSetAllMake(KHE_AVAIL_SOLVER as)
{
  KHE_ERSET_ALL res;  int tcount, rcount, i;

  /* make the basic object */
  if( HaArrayCount(as->erset_all_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->erset_all_free_list);
    HaArrayClear(res->t_set);
    HaArrayClear(res->up_set);
  }
  else
  {
    HaMake(res, as->arena);
    HaArrayInit(res->t_set, as->arena);
    HaArrayInit(res->up_set, as->arena);
  }

  /* initialize the fields */
  res->uu_set = KheErSetMake(as);
  tcount = KheInstanceTimeCount(as->instance);
  for( i = 0;  i < tcount;  i++ )
    HaArrayAddLast(res->t_set, KheErSetTimeMake(as));
  rcount = KheResourceTypeResourceCount(as->curr_rt);
  for( i = 0;  i < rcount;  i++ )
    HaArrayAddLast(res->up_set, KheErSetMake(as));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetAllFree(KHE_ERSET_ALL ea, KHE_AVAIL_SOLVER as)              */
/*                                                                           */
/*  Free ea by returning it to the free list in as.  This includes           */
/*  freeing all its component objects.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheErSetAllFree(KHE_ERSET_ALL ea, KHE_AVAIL_SOLVER as)
{
  KHE_ERSET_TIME et;  KHE_ERSET erset;  int i;
  KheErSetFree(ea->uu_set, as);
  HaArrayForEach(ea->t_set, et, i)
    KheErSetTimeFree(et, as);
  HaArrayForEach(ea->up_set, erset, i)
    KheErSetFree(erset, as);
  HaArrayAddLast(as->erset_all_free_list, ea);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ERSET_ALL KheErSetAllBuild(KHE_AVAIL_SOLVER as)                      */
/*                                                                           */
/*  Build an erset_all object for as->curr_rt.                               */
/*                                                                           */
/*****************************************************************************/
static void KheErSetAllDebug(KHE_ERSET_ALL ea, int verbosity,
  int indent, FILE *fp);

static KHE_ERSET_ALL KheErSetAllBuild(KHE_AVAIL_SOLVER as)
{
  KHE_ERSET_ALL res;  int i, j, k;  KHE_RESOURCE r;  KHE_TIME t;  KHE_EVENT e;
  KHE_EVENT_RESOURCE er;  KHE_ERSET erset;  KHE_ERSET_TIME et;

  /* initialize a new erset_all object */
  res = KheErSetAllMake(as);

  /* add every event resource to the structure */
  for( i = 0;  i < KheInstanceEventCount(as->instance);  i++ )
  {
    e = KheInstanceEvent(as->instance, i);
    t = KheEventPreassignedTime(e);
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      if( KheEventResourceResourceType(er) == as->curr_rt )
      {
	/* update Suu, Sup(r), Spu(t), or Spp(t, r) to take account of er */
	r = KheEventResourcePreassignedResource(er);
	if( t == NULL )
	{
	  if( r == NULL )
	  {
	    /* Suu: no preassigned time, no preassigned resource */
	    KheErSetAddEventResource(res->uu_set, er);
	  }
	  else
	  {
	    /* Sup(r): no preassigned time, preassigned resource r */
	    erset = HaArray(res->up_set, KheResourceResourceTypeIndex(r));
	    KheErSetAddEventResource(erset, er);
	  }
	}
	else
	{
	  for( k = 0;  k < KheEventDuration(e);  k++ )
	  {
	    et = HaArray(res->t_set, KheTimeIndex(t) + k);
	    if( r == NULL )
	    {
	      /* Spu(t): preassigned time t + k, no preassigned resource */
	      KheErSetAddEventResource(et->pu_set, er);
	    }
	    else
	    {
	      /* Spp(t, r): preassigned time t + k, preassigned resource r */
	      erset = HaArray(et->pp_set, KheResourceResourceTypeIndex(r));
	      KheErSetAddEventResource(erset, er);
	    }
	  }
	}
      }
    }
  }
  if( DEBUG4 )
    KheErSetAllDebug(res, 2, 2, stderr);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  ERSET_TYPE KheErSetAllRetrieveErSet(KHE_ERSET_ALL ea,                    */
/*    KHE_TIME t, KHE_RESOURCE r, KHE_AVAIL_SOLVER as,                       */
/*    float *min_workload_per_time, float *max_workload_per_time)            */
/*                                                                           */
/*  Assuming that ea has been built, return the type of event resource set   */
/*  for time t and resource r in ea:                                         */
/*                                                                           */
/*    ERSET_IS_PREASSIGNED     r is preassigned at time t                    */
/*    ERSET_IS_EMPTY           r cannot be busy at time t                    */
/*    ERSET_IS_NORMAL          neither of the above                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  ERSET_IS_PREASSIGNED,
  ERSET_IS_EMPTY,
  ERSET_IS_NORMAL
} ERSET_TYPE;

static ERSET_TYPE KheErSetAllRetrieveErSet(KHE_ERSET_ALL ea,
  KHE_TIME t, KHE_RESOURCE r, KHE_AVAIL_SOLVER as,
  float *min_workload_per_time, float *max_workload_per_time)
{
  KHE_ERSET_TIME et;  KHE_ERSET erset;  ERSET_TYPE res;

  /* first, see if there is a non-empty Spp */
  et = HaArray(ea->t_set, KheTimeIndex(t));
  erset = HaArray(et->pp_set, KheResourceResourceTypeIndex(r));
  if( !KheErSetIsEmpty(erset) )
  {
    *min_workload_per_time = erset->min_workload_per_time;
    *max_workload_per_time = erset->max_workload_per_time;
    return ERSET_IS_PREASSIGNED;
  }

  /* otherwise, return Suu union Sup(r) union Spu(t) */
  erset = KheErSetMake(as);
  KheErSetUnion(erset, ea->uu_set);
  KheErSetUnion(erset, HaArray(ea->up_set, KheResourceResourceTypeIndex(r)));
  KheErSetUnion(erset, et->pu_set);
  *min_workload_per_time = erset->min_workload_per_time;
  *max_workload_per_time = erset->max_workload_per_time;
  res = (KheErSetIsEmpty(erset) ? ERSET_IS_EMPTY : ERSET_IS_NORMAL);
  KheErSetFree(erset, as);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheErSetAllDebug(KHE_ERSET_ALL ea, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ea onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheErSetAllDebug(KHE_ERSET_ALL ea, int verbosity,
  int indent, FILE *fp)
{
  KHE_ERSET_TIME et;  KHE_ERSET erset;  int i;
  fprintf(fp, "%*s[ Er SetAll Suu = ", indent, "");
  KheErSetDebug(ea->uu_set, verbosity, 0, fp);
  HaArrayForEach(ea->t_set, et, i)
    KheErSetTimeDebug(et, verbosity, indent + 2, fp);
  HaArrayForEach(ea->up_set, erset, i)
    KheErSetDebug(erset, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_LIMIT"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT KheLimitMakeInt(int val)                                       */
/*                                                                           */
/*  Make a limit object holding an integer limit.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_LIMIT KheLimitMakeInt(int val)
{
  KHE_LIMIT res;
  res.is_float = false;
  res.u.int_val = val;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT KheLimitMakeFloat(float val)                                   */
/*                                                                           */
/*  Make a limit object holding a floating-point limit.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_LIMIT KheLimitMakeFloat(float val)
{
  KHE_LIMIT res;
  res.is_float = true;
  res.u.float_val = val;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitAccumulate(KHE_LIMIT *dst_limit, KHE_LIMIT src_limit)       */
/*                                                                           */
/*  Add scr_limit into dst_limit.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheLimitAccumulate(KHE_LIMIT *dst_limit, KHE_LIMIT src_limit)
{
  HnAssert(src_limit.is_float == dst_limit->is_float,
    "KheLimitAccumulate internal error");
  if( src_limit.is_float )
  {
    if( dst_limit->u.float_val == FLT_MAX || src_limit.u.float_val == FLT_MAX )
      dst_limit->u.float_val = FLT_MAX;
    else
      dst_limit->u.float_val += src_limit.u.float_val;
  }
  else
  {
    if( dst_limit->u.int_val == INT_MAX || src_limit.u.int_val == INT_MAX )
      dst_limit->u.int_val = INT_MAX;
    else
      dst_limit->u.int_val += src_limit.u.int_val;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitLessThan(KHE_LIMIT limit1, KHE_LIMIT limit2)                */
/*                                                                           */
/*  Return true if limit1 < limit2.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitLessThan(KHE_LIMIT limit1, KHE_LIMIT limit2)
{
  HnAssert(limit1.is_float == limit2.is_float,
    "KheLimitLessThan internal error");
  if( limit1.is_float )
    return limit1.u.float_val < limit2.u.float_val;
  else
    return limit1.u.int_val < limit2.u.int_val;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEqualsZero(KHE_LIMIT limit)                                 */
/*                                                                           */
/*  Return true if limit equals zero.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitEqualsZero(KHE_LIMIT limit)
{
  if( limit.is_float )
    return limit.u.float_val == 0.0;
  else
    return limit.u.int_val == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTimeSetMaxWorkload(KHE_TIME_SET ts, KHE_AVAIL_SOLVER as)        */
/*                                                                           */
/*  Return the maximum workload that as->curr_r could incur during ts.       */
/*  This code assumes that empty ersets have maximum workload 0.0.           */
/*                                                                           */
/*****************************************************************************/

static float KheTimeSetMaxWorkload(KHE_TIME_SET ts, KHE_AVAIL_SOLVER as)
{
  int i;  KHE_TIME t;  float min_workload_per_time, max_workload_per_time, res;
  res = 0.0;
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
  {
    t = KheTimeSetTime(ts, i);
    KheErSetAllRetrieveErSet(as->erset_all, t, as->curr_r, as,
      &min_workload_per_time, &max_workload_per_time);
    res += max_workload_per_time;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIsNonTrivial(KHE_LIMIT limit, KHE_TIME_SET ts,              */
/*    KHE_AVAIL_SOLVER as)                                                   */
/*                                                                           */
/*  Return true if limit is non-trivial.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitIsNonTrivial(KHE_LIMIT limit, KHE_TIME_SET ts,
  KHE_AVAIL_SOLVER as)
{
  if( limit.is_float )
  {
    /* find the maximum workload and compare */
    return limit.u.float_val < KheTimeSetMaxWorkload(ts, as);
  }
  else
  {
    /* find the maximum busy times (just |ts|) and compare */
    return limit.u.int_val < KheTimeSetTimeCount(ts);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitShow(KHE_LIMIT limit, char buff[50])                       */
/*                                                                           */
/*  Show limit, using buff to hold the result.                               */
/*                                                                           */
/*****************************************************************************/

static char *KheLimitShow(KHE_LIMIT limit, char buff[50])
{
  if( limit.is_float )
    snprintf(buff, 50, "%.1f", limit.u.float_val);
  else
    snprintf(buff, 50, "%d", limit.u.int_val);
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_AVAIL_NODE"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_NODE KheAvailNodeMake(KHE_AVAIL_NODE_TYPE type,                */
/*    KHE_LIMIT limit, KHE_TIME_SET ts, KHE_MONITOR m, KHE_AVAIL_SOLVER as)  */
/*                                                                           */
/*  Make a new avail node with these attributes.                             */
/*                                                                           */
/*  No checks for suitability are made here.  This function is called        */
/*  from only one place, KheAvailSolverMakeAndAddAvailNode, and it is the    */
/*  job of that function to make those checks.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_AVAIL_NODE KheAvailNodeMake(KHE_AVAIL_NODE_TYPE type,
  KHE_LIMIT limit, KHE_TIME_SET ts, KHE_MONITOR m, KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_NODE res;
  if( HaArrayCount(as->avail_node_free_list) > 0 )
    res = HaArrayLastAndDelete(as->avail_node_free_list);
  else
    HaMake(res, as->arena);
  res->type = type;
  res->limit = limit;
  res->time_set = ts;
  res->monitor = m;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailNodeFree(KHE_AVAIL_NODE an, KHE_AVAIL_SOLVER as)            */
/*                                                                           */
/*  Free an.  Also free its time set.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheAvailNodeFree(KHE_AVAIL_NODE an, KHE_AVAIL_SOLVER as)
{
  KheAvailSolverFreeTimeSet(as, an->time_set);
  HaArrayAddLast(as->avail_node_free_list, an);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailNodeCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting avail nodes by decreasing number of      */
/*  times, with ties broken by KheTimeSetTypedCmp.                           */
/*                                                                           */
/*****************************************************************************/

static int KheAvailNodeCmp(const void *t1, const void *t2)
{
  KHE_AVAIL_NODE an1 = * (KHE_AVAIL_NODE *) t1;
  KHE_AVAIL_NODE an2 = * (KHE_AVAIL_NODE *) t2;
  int an1_value, an2_value;
  if( an1->limit.is_float || an2->limit.is_float )
  {
    an1_value = KheTimeSetTimeCount(an1->time_set) /* - an1->int_limit */;
    an2_value = KheTimeSetTimeCount(an2->time_set) /* - an2->int_limit */;
  }
  else
  {
    an1_value = KheTimeSetTimeCount(an1->time_set) - an1->limit.u.int_val;
    an2_value = KheTimeSetTimeCount(an2->time_set) - an2->limit.u.int_val;
  }
  if( an1_value != an2_value )
    return an2_value - an1_value;
  else
    return KheTimeSetTypedCmp(an1->time_set, an2->time_set);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvailNodeTypeShow(KHE_AVAIL_NODE_TYPE type)                     */
/*                                                                           */
/*  Return a short phrase in static memory describing type.                  */
/*                                                                           */
/*****************************************************************************/

char *KheAvailNodeTypeShow(KHE_AVAIL_NODE_TYPE type)
{
  switch( type )
  {
    case KHE_AVAIL_NODE_UNASSIGNABLE_TIME:

      return "Unassignable times";

    case KHE_AVAIL_NODE_UNAVAILABLE_TIME:

      return "Unavailable times";

    case KHE_AVAIL_NODE_LIMIT_BUSY_ZERO:

      return "Limit busy times constraint with zero max limit";

    case KHE_AVAIL_NODE_CLUSTER_BUSY_ZERO:

      return "Cluster busy times constraint with zero max limit";

    case KHE_AVAIL_NODE_LIMIT_BUSY:

      return "Limit busy times constraint with non-zero max limit";

    case KHE_AVAIL_NODE_CLUSTER_BUSY:

      return "Cluster busy times constraint with non-zero max limit";

    case KHE_AVAIL_NODE_CLUSTER_BUSY_MIN:

      return "Cluster busy times constraint with min limit";

    case KHE_AVAIL_NODE_WORKLOAD:

      return "Limit workload constraint";

    case KHE_AVAIL_NODE_LEFTOVER:

      return "Leftover times";

    default:

      HnAbort("KheAvailNodeTypeShow: invalid type (%d)", type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvailNodeTypeDebug(KHE_AVAIL_NODE_TYPE type)                    */
/*                                                                           */
/*  Return type as a string.                                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheAvailNodeTypeDebug(KHE_AVAIL_NODE_TYPE type)
{
  switch( type )
  {
    case KHE_AVAIL_NODE_UNASSIGNABLE_TIME:

      return "UNASSIGNABLE_TIME";

    case KHE_AVAIL_NODE_UNAVAILABLE_TIME:

      return "UNAVAILABLE_TIME";

    case KHE_AVAIL_NODE_LIMIT_BUSY_ZERO:

      return "LIMIT_BUSY_ZERO";

    case KHE_AVAIL_NODE_CLUSTER_BUSY_ZERO:

      return "CLUSTER_BUSY_ZERO";

    case KHE_AVAIL_NODE_LIMIT_BUSY:

      return "LIMIT_BUSY";

    case KHE_AVAIL_NODE_CLUSTER_BUSY:

      return "CLUSTER_BUSY";

    case KHE_AVAIL_NODE_CLUSTER_BUSY_MIN:

      return "CLUSTER_BUSY_MIN";

    case KHE_AVAIL_NODE_WORKLOAD:

      return "WORKLOAD";

    case KHE_AVAIL_NODE_LEFTOVER:

      return "LEFTOVER";

    default:

      HnAbort("KheAvailNodeTypeDebug: invalid type (%d)", type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailNodeDebug(KHE_AVAIL_NODE an, int verbosity, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of an onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheAvailNodeDebug(KHE_AVAIL_NODE an, int verbosity,
  int indent, FILE *fp)
{
  char buff[50];
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "AvailNode(%s %s ", KheAvailNodeTypeDebug(an->type),
      KheLimitShow(an->limit, buff));
  KheTimeSetDebug(an->time_set, verbosity, -1, fp);
  if( an->monitor != NULL )
    fprintf(fp, ", %s", KheMonitorId(an->monitor));
  fprintf(fp, ")\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "AVAIL_SET" - independent sets of avail nodes                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SET KheAvailSetMake(KHE_AVAIL_SET_TYPE type,                   */
/*    KHE_AVAIL_SOLVER as)                                                   */
/*                                                                           */
/*  Make a new, empty set of avail nodes.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_AVAIL_SET KheAvailSetMake(KHE_AVAIL_SET_TYPE type,
  KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_SET res;
  if( HaArrayCount(as->avail_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(as->avail_set_free_list);
    HaArrayClear(res->nodes);
  }
  else
  {
    HaMake(res, as->arena);
    HaArrayInit(res->nodes, as->arena);
  }
  res->type = type;
  if( type == KHE_AVAIL_SET_TIMES )
    res->limit = KheLimitMakeInt(0);
  else
    res->limit = KheLimitMakeFloat(0.0);
  /* res->total_times = 0; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSetFree(KHE_AVAIL_SET avail_set, KHE_AVAIL_SOLVER as)       */
/*                                                                           */
/*  Free avail_set.  We don't free its nodes, except that if the last        */
/*  node is a leftover node, we free that.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSetFree(KHE_AVAIL_SET avail_set, KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_NODE last_an;

  /* free the last avail node, if it's a leftover node */
  if( HaArrayCount(avail_set->nodes) > 0 )
  {
    last_an = HaArrayLast(avail_set->nodes);
    if( last_an->type == KHE_AVAIL_NODE_LEFTOVER )
      KheAvailNodeFree(last_an, as);
  }

  /* free the avail set object */
  HaArrayAddLast(as->avail_set_free_list, avail_set);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailSetFinalLimit(KHE_AVAIL_SET avail_set, KHE_AVAIL_SOLVER as)  */
/*                                                                           */
/*  Return the final limit of avail_set.                                     */
/*                                                                           */
/*  For clarity we have separated the two cases, but they are the same,      */
/*  because KheInstanceTimeCount(as->instance) - avail_set->total_times      */
/*  is zero in workload nodes whose final limit matters.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_LIMIT KheAvailSetFinalLimit(KHE_AVAIL_SET avail_set,
  KHE_AVAIL_SOLVER as)
{
  switch( avail_set->type )
  {
    case KHE_AVAIL_SET_TIMES:

      return avail_set->limit.u.int_val +
	KheInstanceTimeCount(as->instance) - avail_set->total_times;

    case KHE_AVAIL_SET_WORKLOAD:

      return avail_set->limit.u.int_val;

    default:

      HnAbort("AvailSetFinalLimit internal error");
      return 0;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSetIsCandidate(KHE_AVAIL_SET avail_set, KHE_AVAIL_SOLVER as)*/
/*                                                                           */
/*  Return true if avail_set is a candidate.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** all avail sets are candidates now
static bool KheAvailSetIsCandidate(KHE_AVAIL_SET avail_set, KHE_AVAIL_SOLVER as)
{
  switch( avail_set->type )
  {
    case KHE_AVAIL_SET_TIMES:

      ** the set's limit must be non-trivial **
      return avail_set->limit.u.int_val < avail_set->total_times;

    case KHE_AVAIL_SET_WORKLOAD:

      ** the set must cover the whole cycle **
      return avail_set->total_times == KheInstanceTimeCount(as->instance);

    default:

      HnAbort("AvailSetIsCandidate internal error");
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSetAcceptsNode(KHE_AVAIL_SET as, KHE_AVAIL_NODE an)         */
/*                                                                           */
/*  Return true if as accepts an (if an is independent of as's nodes).       */
/*                                                                           */
/*****************************************************************************/

static bool KheAvailSetAcceptsNode(KHE_AVAIL_SET as, KHE_AVAIL_NODE an)
{
  int i;  KHE_AVAIL_NODE an2;
  HaArrayForEach(as->nodes, an2, i)
    if( !KheTimeSetDisjoint(an->time_set, an2->time_set) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSetAddNode(KHE_AVAIL_SET as, KHE_AVAIL_NODE an)             */
/*                                                                           */
/*  Add an to as.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSetAddNode(KHE_AVAIL_SET as, KHE_AVAIL_NODE an)
{
  KheLimitAccumulate(&as->limit, an->limit);
  /* as->total_times += KheTimeSetTimeCount(an->time_set); */
  HaArrayAddLast(as->nodes, an);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSetContainsTime(KHE_AVAIL_SET avail_set, KHE_TIME t)        */
/*                                                                           */
/*  Return true if avail_set contains t.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheAvailSetContainsTime(KHE_AVAIL_SET avail_set, KHE_TIME t)
{
  KHE_AVAIL_NODE an;  int i;
  HaArrayForEach(avail_set->nodes, an, i)
    if( KheTimeSetContainsTime(an->time_set, t) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_NODE KheAvailSetMakeLeftoverNode(KHE_AVAIL_SET avail_set,      */
/*    KHE_AVAIL_SOLVER as)                                                   */
/*                                                                           */
/*  Return a leftover node for avail_set, or NULL if not wanted.             */
/*                                                                           */
/*****************************************************************************/

static KHE_AVAIL_NODE KheAvailSetMakeLeftoverNode(KHE_AVAIL_SET avail_set,
  KHE_AVAIL_SOLVER as)
{
  KHE_LIMIT limit;  KHE_TIME_SET leftover_ts;  KHE_TIME t;  int i;

  /* build leftover_ts, containing the leftover times */
  leftover_ts = KheAvailSolverMakeTimeSet(as);
  for( i = 0;  i < KheInstanceTimeCount(as->instance);  i++ )
  {
    t = KheInstanceTime(as->instance, i);
    if( !KheAvailSetContainsTime(avail_set, t) )
      KheTimeSetAddTime(leftover_ts, t);
  }

  /* if leftover_ts is empty we don't want a leftover node */
  if( KheTimeSetTimeCount(leftover_ts) == 0 )
  {
    KheAvailSolverFreeTimeSet(as, leftover_ts);
    return NULL;
  }

  /* make the limit */
  if( avail_set->type == KHE_AVAIL_SET_TIMES )
  {
    /* limit is just the size of leftover_ts */
    limit = KheLimitMakeInt(KheTimeSetTimeCount(leftover_ts));
  }
  else
  {
    /* limit is the total max workload per time of leftover_ts */
    limit = KheLimitMakeFloat(KheTimeSetMaxWorkload(leftover_ts, as));
  }

  /* make and return the node */
  return KheAvailNodeMake(KHE_AVAIL_NODE_LEFTOVER, limit, leftover_ts,NULL,as);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSetContainsNode(KHE_AVAIL_SET avail_set,                    */
/*    KHE_AVAIL_NODE_TYPE type)                                              */
/*                                                                           */
/*  Return true if avail_set contains a node of the given type.              */
/*                                                                           */
/*****************************************************************************/

static bool KheAvailSetContainsNode(KHE_AVAIL_SET avail_set,
  KHE_AVAIL_NODE_TYPE type)
{
  KHE_AVAIL_NODE an;  int i;
  HaArrayForEach(avail_set->nodes, an, i)
    if( an->type == type )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSetDebug(KHE_AVAIL_SET as, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of as onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSetDebug(KHE_AVAIL_SET as, int verbosity,
  int indent, FILE *fp)
{
  KHE_AVAIL_NODE an;  int i;  char buff[50];
  fprintf(fp,"%*s[ AvailSet%s (total_limit %s)\n",
    indent, "", as->type == KHE_AVAIL_SET_TIMES ? "Times" : "Workload",
    KheLimitShow(as->limit, buff));
  HaArrayForEach(as->nodes, an, i)
    KheAvailNodeDebug(an, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_LOAD_SOLVER"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LOAD_SOLVER KheLoadSolverMake(KHE_AVAIL_SOLVER as)                   */
/*                                                                           */
/*  Make a load solver object.  NB there is no free list for this type.      */
/*                                                                           */
/*****************************************************************************/

static KHE_LOAD_SOLVER KheLoadSolverMake(KHE_AVAIL_SOLVER as)
{
  KHE_LOAD_SOLVER res;
  HaMake(res, as->arena);
  HpTableInit(res->node_table, (HP_HASH_FN) &KheTimeSetHash,
    (HP_EQUAL_FN) &KheTimeSetEqual, NULL, as->arena);
  HaArrayInit(res->node_array, as->arena);
  res->best_avail_set = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLoadSolverClear(KHE_LOAD_SOLVER ls)                              */
/*                                                                           */
/*  Clear ls, ready for a fresh solve, including freeing its nodes.          */
/*                                                                           */
/*****************************************************************************/

static void KheLoadSolverClear(KHE_LOAD_SOLVER ls, KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_NODE an;  int i;

  /* free ls's avail nodes and avail set if any */
  if( ls->best_avail_set != NULL )
    KheAvailSetFree(ls->best_avail_set, as);
  HaArrayForEach(ls->node_array, an, i)
    KheAvailNodeFree(an, as);

  /* clear ls's fields */
  HpTableClear(ls->node_table);
  HaArrayClear(ls->node_array);
  ls->best_avail_set = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLoadSolverMakeAndAddAvailNode(KHE_LOAD_SOLVER ls,                */
/*    KHE_AVAIL_NODE_TYPE type, KHE_LIMIT limit, KHE_TIME_SET ts,            */
/*    KHE_MONITOR m, KHE_AVAIL_SOLVER as)                                    */
/*                                                                           */
/*  Make and add an avail node with these attributes, if non-trivial.        */
/*  Leftover nodes are added separately, not via this function.              */
/*                                                                           */
/*****************************************************************************/
static void KheUpdateZeroTimeNodes(KHE_AVAIL_SOLVER as, KHE_AVAIL_NODE an);

static void KheLoadSolverMakeAndAddAvailNode(KHE_LOAD_SOLVER ls,
  KHE_AVAIL_NODE_TYPE type, KHE_LIMIT limit, bool force,
  KHE_TIME_SET ts, KHE_MONITOR m, KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_NODE an;  int pos;  char buff[50];
  if( DEBUG7 )
    fprintf(stderr, "[ KheLoadSolverMakeAndAddAvailNode(ls, %s, %s, %d times, "
      "%s)\n", KheAvailNodeTypeShow(type), KheLimitShow(limit, buff),
      KheTimeSetTimeCount(ts), m == NULL ? "-" : KheMonitorId(m));
  if( !force && !KheLimitIsNonTrivial(limit, ts, as) )
  {
    /* ignore this call because it adds nothing useful */
    if( DEBUG7 )
      fprintf(stderr, "  ignoring, not useful\n");
    KheAvailSolverFreeTimeSet(as, ts);
  }
  else if( !HpTableRetrieve(ls->node_table, (void *) ts, an, pos) )
  {
    /* ts is new, make a node and add it to the table and array */
    an = KheAvailNodeMake(type, limit, ts, m, as);
    HpTableAdd(ls->node_table, (void *) ts, an);
    HaArrayAddLast(ls->node_array, an);
    KheUpdateZeroTimeNodes(as, an);
    if( DEBUG7 )
      fprintf(stderr, "  new node, adding to as\n");
  }
  else if( KheLimitLessThan(limit, an->limit) )
  {
    /* ts is already present but this call is stronger, so replace */
    an->type = type;
    an->limit = limit;
    an->monitor = m;
    KheAvailSolverFreeTimeSet(as, ts);
    KheUpdateZeroTimeNodes(as, an);
    if( DEBUG7 )
      fprintf(stderr, "  old node but improved limit\n");
  }
  else
  {
    /* ignore this call because something at least as good is already there */
    KheAvailSolverFreeTimeSet(as, ts);
    if( DEBUG7 )
      fprintf(stderr, "  ignoring, exising node has better limit %s\n",
	KheLimitShow(an->limit, buff));
  }
  if( DEBUG7 )
    fprintf(stderr, "] KheLoadSolverMakeAndAddAvailNode returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLoadSolverFindBestAvailSet(KHE_LOAD_SOLVER ls,                   */
/*    KHE_AVAIL_SET_TYPE type, KHE_AVAIL_SOLVER as)                          */
/*  KHE_AVAIL_SET KheAvailSolverFindBestAvailSet(KHE_AVAIL_SOLVER as,        */
/*    KHE_AVAIL_SET_TYPE type)                                               */
/*                                                                           */
/*  Find the best avail set we can from the nodes in as->node_array.         */
/*                                                                           */
/*****************************************************************************/
static void KheLoadSolverDebug(KHE_LOAD_SOLVER ls, int verbosity,
  int indent, FILE *fp);

static void KheLoadSolverFindBestAvailSet(KHE_LOAD_SOLVER ls,
  KHE_AVAIL_SET_TYPE type, KHE_AVAIL_SOLVER as)
{
  KHE_AVAIL_SET avail_set, res;  int count, start_pos, i, since_new_best;
  KHE_AVAIL_NODE an, leftover_an;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheLoadSolverFindBestAvailSet(%s, -)\n",
      KheResourceId(as->curr_r));
    KheLoadSolverDebug(ls, 2, 2, stderr);
  }

  /* sort the nodes */
  HaArraySort(ls->node_array, &KheAvailNodeCmp);

  /* find one independent set starting at each position and keep the best */
  res = NULL;
  since_new_best = 0;
  count = HaArrayCount(ls->node_array);
  for( start_pos = 0;  start_pos < count;  start_pos++ )
  {
    /* give up if too long since a new best appeared */
    if( since_new_best >= 20 )
      break;

    /* build an avail set starting from start_an */
    avail_set = KheAvailSetMake(type, as);
    for( i = 0;  i < count;  i++ )
    {
      an = HaArray(ls->node_array, (start_pos + i) % count);
      if( KheAvailSetAcceptsNode(avail_set, an) )
	KheAvailSetAddNode(avail_set, an);
    }
    leftover_an = KheAvailSetMakeLeftoverNode(avail_set, as);
    if( leftover_an != NULL )
      KheAvailSetAddNode(avail_set, leftover_an);

    /* compare avail_set with res and keep the best */
    if( res == NULL )
    {
      /* avail_set is first best */
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (first best):\n", i);
	KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      res = avail_set;
      since_new_best = 0;
    }
    else if( KheLimitLessThan(avail_set->limit, res->limit) )
    {
      /* avail_set is new best; delete old best */
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (new best):\n", i);
	KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      KheAvailSetFree(res, as);
      res = avail_set;
      since_new_best = 0;
    }
    else
    {
      /* avail_set is not new best; delete it */
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (not new best):\n", i);
	KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      KheAvailSetFree(avail_set, as);
      since_new_best++;
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] AvailSolverFindBestAvailSet returning %s\n",
      res == NULL ? "NULL" : "non-NULL");
  ls->best_avail_set = res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLoadSolverBestContainsLimitWorkloadNode(KHE_LOAD_SOLVER ls)      */
/*                                                                           */
/*  Return true if ls->best_avail_set contains a limit workload node.        */
/*                                                                           */
/*****************************************************************************/

static bool KheLoadSolverBestContainsLimitWorkloadNode(KHE_LOAD_SOLVER ls)
{
  if( ls->best_avail_set == NULL )
    return false;
  return KheAvailSetContainsNode(ls->best_avail_set, KHE_AVAIL_NODE_WORKLOAD);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLoadSolverClearBest(KHE_LOAD_SOLVER ls, KHE_AVAIL_SOLVER as)     */
/*                                                                           */
/*  Ensure ls->best_avail_set is NULL.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheLoadSolverClearBest(KHE_LOAD_SOLVER ls, KHE_AVAIL_SOLVER as)
{
  if( ls->best_avail_set != NULL )
  {
    KheAvailSetFree(ls->best_avail_set, as);
    ls->best_avail_set = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLoadSolverDebug(KHE_LOAD_SOLVER ls, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug printf of ls onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheLoadSolverDebug(KHE_LOAD_SOLVER ls, int verbosity,
  int indent, FILE *fp)
{
  KHE_AVAIL_NODE an;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ LoadSolver\n", indent, "");
    HaArrayForEach(ls->node_array, an, i)
      KheAvailNodeDebug(an, verbosity, indent + 2, fp);
    if( ls->best_avail_set != NULL )
      KheAvailSetDebug(ls->best_avail_set, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "LoadSolver");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_AVAIL_SOLVER"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SOLVER KheAvailSolverMake(KHE_SOLN soln, HA_ARENA a)           */
/*                                                                           */
/*  Make an avail solver for r in soln.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_AVAIL_SOLVER KheAvailSolverMake(KHE_SOLN soln, HA_ARENA a)
{
  KHE_AVAIL_SOLVER res;
  HaMake(res, a);

  /* free lists - must do these first */
  HaArrayInit(res->time_set_free_list, a);
  /* HaArrayInit(res->preasst_time_info_free_list, a); */
  /* HaArrayInit(res->preasst_info_free_list, a); */
  HaArrayInit(res->erset_free_list, a);
  HaArrayInit(res->erset_time_free_list, a);
  HaArrayInit(res->erset_all_free_list, a);
  HaArrayInit(res->avail_node_free_list, a);
  HaArrayInit(res->avail_set_free_list, a);

  /* constant fields */
  res->arena = a;
  res->soln = soln;
  res->instance = KheSolnInstance(soln);
  /* HaArrayInit(res->preasst_info, a); */
  /* ***
  if( KheInstanceAllEventsHavePreassignedTimes(res->ins) )
  {
    for( i = 0;  i < KheInstanceResourceTypeCount(res->ins);  i++ )
    {
      rt = KheInstanceResourceType(res->ins, i);
      HaArrayAddLast(res->preasst_info, KhePreasstInfoBuild(res, rt));
    }
  }
  HaArrayInit(res->workload_array, a);
  HaArrayFill(res->workload_array, KheInstanceResourceTypeCount(res->ins),NULL);
  *** */

  /* fields dependent on curr_rt, or event curr_r */
  res->curr_rt = NULL;
  res->curr_r = NULL;
  /* res->preasst_info = NULL; */
  res->erset_all = NULL;
  /* HaArrayInit(res->limit_and_cluster_monitors, a); */
  /* HaArrayInit(res->workload_monitors, a); */
  HaArrayInit(res->zero_times, a);
  HaArrayFill(res->zero_times, KheInstanceTimeCount(res->instance), false);
  res->busy_times_solver[0] = KheLoadSolverMake(res);
  res->busy_times_solver[1] = KheLoadSolverMake(res);
  res->workload_solver[0] = KheLoadSolverMake(res);
  res->workload_solver[1] = KheLoadSolverMake(res);
  res->best_busy_times_solver = NULL;
  res->best_workload_solver = NULL;
  /* ***
  HpTableInit(res->node_table, (HP_HASH_FN) &KheTimeSetHash,
    (HP_EQUAL_FN) &KheTimeSetEqual, NULL, a);
  HaArrayInit(res->node_array, a);
  res->best_busy_times = NULL;
  res->best_workload = NULL;
  *** */

  /* scratch fields */
  HaArrayInit(res->tmp_cluster_limits, a);
  HaArrayInit(res->tmp_cluster_time_sets, a);
  HaArrayInit(res->tmp_min_workloads, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUpdateZeroTimeNodes(KHE_AVAIL_SOLVER as, KHE_AVAIL_NODE an)      */
/*                                                                           */
/*  If an is a zero time node, update the zero time nodes array.             */
/*                                                                           */
/*****************************************************************************/

static void KheUpdateZeroTimeNodes(KHE_AVAIL_SOLVER as, KHE_AVAIL_NODE an)
{
  KHE_TIME t;  int i;
  if( KheLimitEqualsZero(an->limit) )
  {
    for( i = 0;  i < KheTimeSetTimeCount(an->time_set);  i++ )
    {
      t = KheTimeSetTime(an->time_set, i);
      HaArrayPut(as->zero_times, KheTimeIndex(t), true);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSolverTimeIsZeroTime(KHE_AVAIL_SOLVER as, KHE_TIME t)       */
/*                                                                           */
/*  Return true if t is known to be a zero time node.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheAvailSolverTimeIsZeroTime(KHE_AVAIL_SOLVER as, KHE_TIME t)
{
  return HaArray(as->zero_times, KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverDebug(KHE_AVAIL_SOLVER as, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of avail solver as.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverDebug(KHE_AVAIL_SOLVER as, int verbosity,
  int indent, FILE *fp)
{
  int i;  bool zero_time;
  fprintf(fp, "%*s[ AvailSolver(%s: curr_rt %s, curr_r %s)\n", indent, "",
    KheInstanceId(as->instance),
    as->curr_rt == NULL ? "-" : KheResourceTypeId(as->curr_rt),
    as->curr_r == NULL ? "-" : KheResourceId(as->curr_r));
  HaArrayForEach(as->zero_times, zero_time, i)
    if( zero_time )
      fprintf(fp, "%*s  zero time: %s\n", indent, "",
	KheTimeId(KheInstanceTime(as->instance, i)));
  if( as->best_busy_times_solver != NULL )
  {
    fprintf(fp, "%*sbusy_times_solver:\n", indent, "");
    KheLoadSolverDebug(as->best_busy_times_solver, verbosity, indent + 2, fp);
  }
  if( as->best_workload_solver != NULL )
  {
    fprintf(fp, "%*sworkload_solver:\n", indent, "");
    KheLoadSolverDebug(as->best_workload_solver, verbosity, indent + 2, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "set resource" (does all the real work)                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverTimeSetAddTimeGroup(KHE_AVAIL_SOLVER as,              */
/*    KHE_TIME_GROUP tg, KHE_TIME_SET ts)                                    */
/*                                                                           */
/*  Add the times of tg to ts, excluding zero times.                         */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverTimeSetAddTimeGroup(KHE_AVAIL_SOLVER as,
  KHE_TIME_GROUP tg, KHE_TIME_SET ts)
{
  KHE_TIME t;  int i;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    if( !KheAvailSolverTimeIsZeroTime(as, t) )
      KheTimeSetAddTime(ts, t);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheAvailSolverTimeSetFromTimeGroup(KHE_AVAIL_SOLVER as,     */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Return a new time set containing the times of tg, excluding zero times.  */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_SET KheAvailSolverTimeSetFromTimeGroup(KHE_AVAIL_SOLVER as,
  KHE_TIME_GROUP tg)
{
  KHE_TIME_SET res;
  res = KheAvailSolverMakeTimeSet(as);
  KheAvailSolverTimeSetAddTimeGroup(as, tg, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheClusterMonitorTimeSet(KHE_CLUSTER_BUSY_TIMES_MONITOR m,  */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a time set containing m's times.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_TIME_SET KheClusterMonitorTimeSet(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  KHE_AVAIL_SOLVER as)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;  KHE_TIME_SET res;
  res = KheAvailSolverMakeTimeSet(as);
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
    KheAvailSolverTimeSetAddTimeGroup(as, tg, res);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAvailNodeBuild(KHE_AVAIL_SOLVER as,                   */
/*    KHE_AVAIL_NODE_TYPE type, int int_limit, float float_limit,            */
/*    KHE_TIME_SET ts, KHE_MONITOR m)                                        */
/*                                                                           */
/*  Make and add an avail node with these attributes, unless it would add    */
/*  nothing useful or there is already something as good or better.          */
/*                                                                           */
/*  Care is needed with ts:  it is undefined after this operation, because   */
/*  if a new node is not created, it is freed.                               */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLoadSolver
static void KheAvailSolverAvailNodeBuild(KHE_AVAIL_SOLVER as,
  KHE_AVAIL_NODE_TYPE type, int int_limit, float float_limit,
  KHE_TIME_SET ts, KHE_MONITOR m)
{
  KHE_AVAIL_NODE an;  int pos;
  if( DEBUG7 )
    fprintf(stderr, "[ KheAvailSolverAvailNodeBuild(%s, %d, %d times, "
      "%s)\n", KheAvailNodeTypeShow(type), int_limit, KheTimeSetTimeCount(ts),
      KheMonitorId(m));
  HnAssert(int_limit >= 0, "KheAvailSolverAvailNodeBuild internal error");
  if( int_limit >= KheTimeSetTimeCount(ts) )
  {
    ** ignore this call because it adds nothing useful **
    if( DEBUG7 )
      fprintf(stderr, "  ignoring, not useful\n");
    KheAvailSolverFreeTimeSet(as, ts);
  }
  else if( !HpTableRetrieve(as->node_table, (void *) ts, an, pos) )
  {
    ** ts is new, make a node and add it to the table and array **
    an = KheAvailNodeMake(type, int_limit, float_limit, ts, m, as);
    HpTableAdd(as->node_table, (void *) ts, an);
    HaArrayAddLast(as->node_array, an);
    KheUpdateZeroTimeNodes(as, an);
    if( DEBUG7 )
      fprintf(stderr, "  new node, adding to as\n");
  }
  else if( int_limit < an->int_limit )
  {
    ** ts is already present but this call is stronger, so replace **
    an->type = type;
    an->int_limit = int_limit;
    an->float_limit = float_limit;
    an->monitor = m;
    KheAvailSolverFreeTimeSet(as, ts);
    KheUpdateZeroTimeNodes(as, an);
    if( DEBUG7 )
      fprintf(stderr, "  old node but improved limit\n");
  }
  else
  {
    ** ignore this call because something at least as good is already there **
    KheAvailSolverFreeTimeSet(as, ts);
    if( DEBUG7 )
      fprintf(stderr, "  ignoring, exising node has better limit %d\n",
	an->int_limit);
  }
  if( DEBUG7 )
    fprintf(stderr, "] KheAvailSolverAvailNodeBuild returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverMakeAndAddAvailNodeAndSubsets(KHE_AVAIL_SOLVER as,    */
/*    KHE_AVAIL_NODE_TYPE type, int limit, KHE_TIME_SET ts, KHE_MONITOR m)   */
/*                                                                           */
/*  Add an avail node with these attributes, and also add one avail node     */
/*  for each time set which is a subset of ts with one less element, and     */
/*  the same other attributes.  But only add the subsets if they are         */
/*  useful, that is, if they contain at least one time and limit places      */
/*  a non-trivial constraint on their number.                                */
/*                                                                           */
/*  Care is needed with ts:  it is undefined after this operation.           */
/*                                                                           */
/*****************************************************************************/

/* *** not doing subsets any more
static void KheAvailSolverMakeAndAddAvailNodeAndSubsets(KHE_AVAIL_SOLVER as,
  KHE_AVAIL_NODE_TYPE type, int limit, KHE_TIME_SET ts, KHE_MONITOR m)
{
  int omit, i, count;  KHE_AVAIL_NODE an;

  ** add a node containing ts; this makes ts undefined **
  an = KheAvailSolverAvailNodeBuild(as, type, limit, ts, m);

  ** build and add subsets, where useful **
  if( an != NULL )
  {
    count = KheTimeSetTimeCount(an->time_set);
    if( count >= 2 && limit < count - 1 )
    {
      for( omit = 0;  omit < KheTimeSetTimeCount(an->time_set);  omit++ )
      {
	** build and add a time set, omitting the element at omit **
	ts = KheAvailSolv erMakeTimeSet(as);
	for( i = 0;  i < count;  i++ )
	  if( i != omit )
	    KheTimeSetAddTime(ts, KheTimeSetTime(an->time_set, i));
	KheAvailSolverAvailNodeBuild(as, type, limit, ts, m);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SET KheAvailSolverFindBestAvailSet(KHE_AVAIL_SOLVER as,        */
/*    KHE_AVAIL_SET_TYPE type)                                               */
/*                                                                           */
/*  Find the best avail set we can from the nodes in as->node_array.         */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLoadSolverFindBestAvailSet
static KHE_AVAIL_SET KheAvailSolverFindBestAvailSet(KHE_AVAIL_SOLVER as,
  KHE_AVAIL_SET_TYPE type)
{
  KHE_AVAIL_SET avail_set, res;  int i, j, since_new_best;
  KHE_AVAIL_NODE init_an, an;
  if( DEBUG2 )
    fprintf(stderr, "[ AvailSolverFindBestAvailSet(%s, -)\n",
      KheResourceId(as->curr_r));

  ** sort the nodes **
  HaArraySort(as->node_array, &KheAvailNodeCmp);

  ** find one independent set starting at each position and keep the best **
  res = NULL;
  since_new_best = 0;
  HaArrayForEach(as->node_array, init_an, i)
  {
    ** give up if too long since a new best appeared **
    if( since_new_best >= 20 )
      break;

    ** build an avail set starting from init_an **
    avail_set = KheAvailSetMake(type, as);
    KheAvailSetAddNode(avail_set, init_an);
    for( j = i + 1;  j < HaArrayCount(as->node_array);  j++ )
    {
      an = HaArray(as->node_array, j);
      if( KheAvailSetAcceptsNode(avail_set, an) )
	KheAvailSetAddNode(avail_set, an);
    }

    ** compare avail_set (if it is a candidate) with res and keep the best **
    if( !KheAvailSetIsCandidate(avail_set, as) )
    {
      ** avail_set is not a candidate; delete it **
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (not candidate):\n", i);
	if( DEBUG6 )
	  KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      KheAvailSetFree(avail_set, false, as);
      since_new_best++;
    }
    else if( res == NULL )
    {
      ** avail_set is first best **
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (first best):\n", i);
	if( DEBUG6 )
	  KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      res = avail_set;
      since_new_best = 0;
    }
    else if( KheAvailSetFinalLimit(avail_set, as) <
	     KheAvailSetFinalLimit(res, as) )
    {
      ** avail_set is new best; delete old best **
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (new best):\n", i);
	if( DEBUG6 )
	  KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      KheAvailSetFree(res, false, as);
      res = avail_set;
      since_new_best = 0;
    }
    else
    {
      ** avail_set is not new best; delete it **
      if( DEBUG2 )
      {
	fprintf(stderr, "  avail set %d (not new best):\n", i);
	if( DEBUG6 )
	  KheAvailSetDebug(avail_set, 2, 2, stderr);
      }
      KheAvailSetFree(avail_set, false, as);
      since_new_best++;
    }
  }
  if( res != NULL )
  {
    ** add leftover node **
    KheAvailSetAddLeftoverNode(res, as);
  }
  if( DEBUG2 )
    fprintf(stderr, "] AvailSolverFindBestAvailSet returning\n");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddZeroTimeNodes(KHE_AVAIL_SOLVER as,                 */
/*    KHE_AVAIL_NODE_TYPE type, KHE_TIME_GROUP tg, KHE_MONITOR m)            */
/*                                                                           */
/*  For each time t of tg not already known to be a zero time, add a zero    */
/*  time node with attributes type, 0, t, and m.                             */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddZeroTimeNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_AVAIL_NODE_TYPE type, KHE_TIME_GROUP tg, KHE_MONITOR m)
{
  int i;  KHE_TIME t;  KHE_TIME_SET ts;
  ts = KheAvailSolverMakeTimeSet(as);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    if( !KheAvailSolverTimeIsZeroTime(as, t) )
      KheTimeSetAddTime(ts, t);
  }
  if( KheTimeSetEmpty(ts) )
    KheAvailSolverFreeTimeSet(as, ts);
  else
  {
    KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver[index], type,
      KheLimitMakeInt(0), false, ts, m, as);
    KheLoadSolverMakeAndAddAvailNode(as->workload_solver[index], type,
      KheLimitMakeFloat(0.0), false, KheAvailSolverCopyTimeSet(as, ts), m, as);
  }
  /* ***
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    if( !KheAvailSolverTimeIsZeroTime(as, t) )
    {
      ts = KheAvailSolverMakeTimeSet(as);
      KheTimeSetAddTime(ts, t);
      KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver, type,
	KheLimitMakeInt(0), ts, m, as);
      KheLoadSolverMakeAndAddAvailNode(as->workload_solver, type,
	KheLimitMakeFloat(0.0), KheAvailSolverCopyTimeSet(as, ts), m, as);
    }
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddUnassignableTimeNodes(KHE_AVAIL_SOLVER as)         */
/*                                                                           */
/*  Add all nodes of type KHE_AVAIL_NODE_UNASSIGNABLE_TIME to as.            */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddUnassignableTimeNodes(KHE_AVAIL_SOLVER as,
  int index)
{
  int i;  KHE_TIME t;  float min_workload_per_time, max_workload_per_time;
  KHE_TIME_SET ts;
  ts = KheAvailSolverMakeTimeSet(as);
  for( i = 0;  i < KheInstanceTimeCount(as->instance);  i++ )
  {
    t = KheInstanceTime(as->instance, i);
    if( !KheAvailSolverTimeIsZeroTime(as, t) &&
	KheErSetAllRetrieveErSet(as->erset_all, t, as->curr_r, as,
	  &min_workload_per_time, &max_workload_per_time) == ERSET_IS_EMPTY )
      KheTimeSetAddTime(ts, t);
      /* ***
      KheAvailSolverAddZeroTimeNodes(as, KHE_AVAIL_NODE_UNASSIGNABLE_TIME,
	KheTimeSingletonTimeGroup(t), NULL);
      *** */
  }
  if( KheTimeSetEmpty(ts) )
    KheAvailSolverFreeTimeSet(as, ts);
  else
  {
    KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver[index],
      KHE_AVAIL_NODE_UNASSIGNABLE_TIME, KheLimitMakeInt(0), true, ts, NULL, as);
    KheLoadSolverMakeAndAddAvailNode(as->workload_solver[index],
      KHE_AVAIL_NODE_UNASSIGNABLE_TIME, KheLimitMakeFloat(0.0), true,
      KheAvailSolverCopyTimeSet(as, ts), NULL, as);
  }

  /* *** old method
  int i, pos;  KHE_PREASST_INFO pi;  KHE_PREASST_TIME_INFO ti;
  if( as->preasst_info != NULL )
  {
    pi = as->preasst_info;
    ** pi = HaArray(as->preasst_info, KheResourceTypeIndex(as->curr_rt)); **
    HaArrayForEach(pi->time_info, ti, i)
    {
      HnAssert(ti->all_preassigned,
	"KheAvailSolverAddUnassignableTimeNodes internal error");
      if( !HaArrayContains(ti->resources, as->curr_r, &pos) )
      {
	** all the slots at ti->time are preassigned, and as->curr_r is not **
	** assigned any of them, so ti->time is unassignable for as->curr_r **
	KheAvailSolverAddZeroTimeNodes(as, KHE_AVAIL_NODE_UNASSIGNABLE_TIME,
	  KheTimeSingletonTimeGroup(ti->time), NULL);
      }
    }
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddUnavailableTimeNodes(KHE_AVAIL_SOLVER as,          */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Add nodes derived from m of type KHE_AVAIL_NODE_UNAVAILABLE_TIME to as.  */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddUnavailableTimeNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg;
  c = KheAvoidUnavailableTimesMonitorConstraint(m);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(c);
  KheAvailSolverAddZeroTimeNodes(as, index, KHE_AVAIL_NODE_UNAVAILABLE_TIME,
    tg, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddLimitBusyZeroNodes(KHE_AVAIL_SOLVER as,            */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Add nodes derived from m of type KHE_AVAIL_NODE_LIMIT_BUSY_ZERO to as.   */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddLimitBusyZeroNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c;  int i, offset;  KHE_TIME_GROUP tg;
  c = KheLimitBusyTimesMonitorConstraint(m);
  offset = KheLimitBusyTimesMonitorOffset(m);
  for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    /* max limit is 0, so all times in all time groups are unavailable */
    tg = KheLimitBusyTimesConstraintTimeGroup(c, i, offset);
    KheAvailSolverAddZeroTimeNodes(as, index, KHE_AVAIL_NODE_LIMIT_BUSY_ZERO,
      tg, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddClusterBusyZeroNodes(KHE_AVAIL_SOLVER as,          */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Add nodes derived from m of type KHE_AVAIL_NODE_CLUSTER_BUSY_ZERO to as. */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddClusterBusyZeroNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
    if( po == KHE_POSITIVE )
    {
      /* max limit is 0, so all times in positive time groups are unavailable */
      KheAvailSolverAddZeroTimeNodes(as, index,
	KHE_AVAIL_NODE_CLUSTER_BUSY_ZERO, tg, (KHE_MONITOR) m);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddLimitBusyNodes(KHE_AVAIL_SOLVER as,                */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Add nodes derived from m of type KHE_AVAIL_NODE_LIMIT_BUSY to as.        */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddLimitBusyNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c;  int i, offset, limit;  KHE_TIME_GROUP tg;
  c = KheLimitBusyTimesMonitorConstraint(m);
  limit = KheLimitBusyTimesConstraintMaximum(c);
  offset = KheLimitBusyTimesMonitorOffset(m);
  for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    /* limit is non-zero, so the whole time group is limited */
    tg = KheLimitBusyTimesConstraintTimeGroup(c, i, offset);
    KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver[index],
      KHE_AVAIL_NODE_LIMIT_BUSY, KheLimitMakeInt(limit), false,
      KheAvailSolverTimeSetFromTimeGroup(as, tg), (KHE_MONITOR) m, as);
    /* ***
    KheAvailSolverAvailNodeBuild(as, KHE_AVAIL_NODE_LIMIT_BUSY, limit,
      FLT_MAX, KheAvailSolverTimeSetFromTimeGroup(as, tg), (KHE_MONITOR) m);
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntDecreasingCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Comparison function for sorting an array of integers by decreasing order.*/
/*                                                                           */
/*****************************************************************************/

static int KheIntDecreasingCmp(const void *t1, const void *t2)
{
  int i1 = * (int *) t1;
  int i2 = * (int *) t2;
  return i2 - i1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddClusterBusyNodes(KHE_AVAIL_SOLVER as,              */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_POLARITY po)                     */
/*                                                                           */
/*  If po is KHE_POSITIVE, try to add a KHE_AVAIL_NODE_CLUSTER_BUSY avail    */
/*  node based on the positive time groups and maximum limit of m.           */
/*                                                                           */
/*  If po is KHE_NEGATIVE, try to add a KHE_AVAIL_NODE_CLUSTER_BUSY_MIN      */
/*  avail node based on the negative time groups and minimum limit of m.     */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddClusterBusyNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_POLARITY po)
{
  int i, pos, limit, max_lim, total_limit, total_times, history, history_before;
  KHE_TIME_GROUP tg;  KHE_POLARITY po2; KHE_TIME_SET ts, total_ts;
  KHE_AVAIL_NODE an;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;

  /* find a limit and time set for each time group of polarity po */
  HaArrayClear(as->tmp_cluster_limits);
  HaArrayClear(as->tmp_cluster_time_sets);
  total_times = 0;
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po2);
    if( po2 == po )
    {
      ts = KheAvailSolverTimeSetFromTimeGroup(as, tg);
      if( HpTableRetrieve(as->busy_times_solver[index]->node_table,
	    (void *) ts, an, pos) )
      {
	/* use the limit from the table */
	HnAssert(!an->limit.is_float,
	  "KheAvailSolverAddClusterBusyNodes internal error");
	limit = an->limit.u.int_val;
      }
      else
      {
	/* the limit is just the time set size */
	limit = KheTimeSetTimeCount(ts);
      }
      HaArrayAddLast(as->tmp_cluster_limits, limit);
      HaArrayAddLast(as->tmp_cluster_time_sets, ts);
      total_times += KheTimeSetTimeCount(ts);
    }
  }

  /* sort the limits and sum the largest max_lim */
  c = KheClusterBusyTimesMonitorConstraint(m);
  history = KheClusterBusyTimesConstraintHistory(c, as->curr_r);
  history_before = KheClusterBusyTimesConstraintHistoryBefore(c);
  if( po == KHE_POSITIVE )
    max_lim = KheClusterBusyTimesConstraintMaximum(c) - history;
  else
    max_lim = KheClusterBusyTimesConstraintTimeGroupCount(c) -
      KheClusterBusyTimesConstraintMinimum(c) - (history_before - history);
  if( max_lim < 0 )
    max_lim = 0;  /* done to agree with doc but makes no difference in fact */
  HaArraySort(as->tmp_cluster_limits, &KheIntDecreasingCmp);
  total_limit = 0;
  for( i = 0;  i < max_lim && i < HaArrayCount(as->tmp_cluster_limits);  i++ )
    total_limit += HaArray(as->tmp_cluster_limits, i);
  if( total_limit < total_times )
  {
    /* make a new node whose times are the stored ones */
    total_ts = KheAvailSolverMakeTimeSet(as);
    HaArrayForEach(as->tmp_cluster_time_sets, ts, i)
      KheTimeSetUnion(total_ts, ts);
    KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver[index],
      KHE_AVAIL_NODE_CLUSTER_BUSY, KheLimitMakeInt(total_limit),
      false, total_ts, (KHE_MONITOR) m, as);
    /* ***
    KheAvailSolverAvailNodeBuild(as, KHE_AVAIL_NODE_CLUSTER_BUSY,
      total_limit, FLT_MAX, total_ts, (KHE_MONITOR) m);
    *** */
  }

  /* free the time sets */
  HaArrayForEach(as->tmp_cluster_time_sets, ts, i)
    KheAvailSolverFreeTimeSet(as, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFloatCmp(const void *t1, const void *t2)                          */
/*                                                                           */
/*  Comparison function for sorting an array of floats into increasing order.*/
/*                                                                           */
/*****************************************************************************/

static int KheFloatCmp(const void *t1, const void *t2)
{
  float f1 = * (float *) t1;
  float f2 = * (float *) t2;
  float diff = f1 - f2;
  return diff < 0 ? -1 : diff > 0 ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddLimitWorkloadNodeForTimeGroup(KHE_AVAIL_SOLVER as, */
/*    KHE_TIME_GROUP tg, float max_lim, KHE_LIMIT_WORKLOAD_MONITOR m)        */
/*                                                                           */
/*  Add an avail node expressing (in the form of a limit on the number of    */
/*  busy times) the workload limit of at most max_lim on tg.                 */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddLimitWorkloadNodeForTimeGroup(KHE_AVAIL_SOLVER as,
  int index, KHE_TIME_GROUP tg, float max_lim, KHE_LIMIT_WORKLOAD_MONITOR m)
{
  float min_workload_per_time, max_workload_per_time, total;
  KHE_TIME t;  int i;  KHE_TIME_SET ts;  char buff[50];

  /* set ts to the times, and tmp_min_workloads to their min workloads */
  if( DEBUG4 )
    fprintf(stderr,
      "[ KheAvailSolverAddLimitWorkloadNodeForTimeGroup(as, %s, %s)\n",
      KheTimeGroupId(tg), KheFloatShow(max_lim, buff));
  ts = KheAvailSolverMakeTimeSet(as);
  HaArrayClear(as->tmp_min_workloads);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    if( KheAvailSolverTimeIsZeroTime(as, t) )
    {
      if( DEBUG4 )
	fprintf(stderr, "  %s: zero time\n", KheTimeId(t));
    }
    else
    {
      switch( KheErSetAllRetrieveErSet(as->erset_all, t, as->curr_r, as,
	&min_workload_per_time, &max_workload_per_time) )
      {
	case ERSET_IS_PREASSIGNED:

	  /* r is preassigned; subtract min_workload from max_lim, ignore t */
	  max_lim -= min_workload_per_time;
	  if( DEBUG4 )
	    fprintf(stderr, "  %s: preassigned time, max_lim -= %s\n",
	      KheTimeId(t), KheFloatShow(min_workload_per_time, buff));
	  break;

	case ERSET_IS_EMPTY:

	  /* r cannot be busy at t; ignore t */
	  if( DEBUG4 )
	    fprintf(stderr, "  %s: not busy time\n", KheTimeId(t));
	  break;

	case ERSET_IS_NORMAL:

	  /* include t and L(t, r), NB L(t, r) < infinity */
	  KheTimeSetAddTime(ts, t);
	  HaArrayAddLast(as->tmp_min_workloads, min_workload_per_time);
	  if( DEBUG4 )
	    fprintf(stderr, "  %s: adding time and %s\n",
	      KheTimeId(t), KheFloatShow(min_workload_per_time, buff));
	  break;

	default:

	  HnAbort("KheAvailSolverAddLimitWorkloadNodeForTimeGroup "
	    "internal error");
	  break;
      }
    }
  }

  /* sort tmp_min_workloads and find the largest k st sum(0 .. k) <= max_lim */
  HaArraySort(as->tmp_min_workloads, &KheFloatCmp);
  if( DEBUG4 )
  {
    fprintf(stderr, "  sorted workloads: ");
    HaArrayForEach(as->tmp_min_workloads, min_workload_per_time, i)
      fprintf(stderr, "%s%s", i == 0 ? "" : i % 8 == 0 ? ",\n  " : ", ",
	KheFloatShow(min_workload_per_time, buff));
    fprintf(stderr, "\n");
  }
  total = 0.0;
  HaArrayForEach(as->tmp_min_workloads, min_workload_per_time, i)
  {
    /* total is the sum of workloads 0 .. i - 1 */
    if( total + min_workload_per_time > max_lim )
      break;
    total += min_workload_per_time;
  }
  if( DEBUG4 )
    fprintf(stderr, "  stopping at %d (total workload %s)\n", i,
      KheFloatShow(total, buff));
  KheLoadSolverMakeAndAddAvailNode(as->busy_times_solver[index],
    KHE_AVAIL_NODE_WORKLOAD, KheLimitMakeInt(i), false, ts, (KHE_MONITOR)m, as);
  /* ***
  KheAvailSolverAvailNodeBuild(as, KHE_AVAIL_NODE_WORKLOAD, i, FLT_MAX, ts,
    (KHE_MONITOR) m);
  *** */
  if( DEBUG4 )
    fprintf(stderr,"] KheAvailSolverAddLimitWorkloadNodeForTimeGroup "
      "returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddLimitWorkloadNodes(KHE_AVAIL_SOLVER as,            */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Add nodes based on m of type KHE_AVAIL_NODE_WORKLOAD to as.              */
/*  This is for both limits on busy times and limits on workload.            */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverAddLimitWorkloadNodes(KHE_AVAIL_SOLVER as,
  int index, KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_LIMIT_WORKLOAD_CONSTRAINT c;  int i, offset, max_lim; KHE_TIME_GROUP tg;
  if( DEBUG4 )
    fprintf(stderr, "[ KheAvailSolverAddLimitWorkloadNodes(as, %s)\n",
      KheMonitorId((KHE_MONITOR) m));
  c = KheLimitWorkloadMonitorConstraint(m);
  offset = KheLimitWorkloadMonitorOffset(m);
  max_lim = KheLimitWorkloadConstraintMaximum(c);
  for( i = 0;  i < KheLimitWorkloadConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitWorkloadConstraintTimeGroup(c, i, offset);
    KheAvailSolverAddLimitWorkloadNodeForTimeGroup(as, index, tg,
      (float) max_lim, m);
    KheLoadSolverMakeAndAddAvailNode(as->workload_solver[index],
      KHE_AVAIL_NODE_WORKLOAD, KheLimitMakeFloat((float) max_lim), false,
      KheAvailSolverTimeSetFromTimeGroup(as, tg), (KHE_MONITOR) m, as);
  }
  if( DEBUG4 )
    fprintf(stderr, "] KheAvailSolverAddLimitWorkloadNodes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverAddWorkloadNodes(KHE_AVAIL_SOLVER as,                 */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Add nodes based on m of type KHE_AVAIL_NODE_WORKLOAD to as.              */
/*  This is for the calculation of limits on workload.                       */
/*                                                                           */
/*****************************************************************************/

/* *** folded into KheAvailSolverAddLimitWorkloadNodes
static void KheAvailSolverAddWorkloadNodes(KHE_AVAIL_SOLVER as,
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  int i, max_lim, offset;  KHE_TIME_GROUP tg;
  KHE_LIMIT_WORKLOAD_CONSTRAINT c;
  c = KheLimitWorkloadMonitorConstraint(m);
  max_lim = KheLimitWorkloadConstraintMaximum(c);
  offset = KheLimitWorkloadMonitorOffset(m);
  for( i = 0;  i < KheLimitWorkloadConstraintTimeGroupCount(c);  i++ )
  {
    ** make and add in an avail node for tg **
    tg = KheLimitWorkloadConstraintTimeGroup(c, i, offset);
    KheLoadSolverMakeAndAddAvailNode(as->workload_solver,
      KHE_AVAIL_NODE_WORKLOAD, KheLimitMakeFloat((float) max_lim),
      KheAvailSolverTimeSetFromTimeGroup(as, tg), (KHE_MONITOR) m, as);
    ** ***
    KheAvailSolverAvailNodeBuild(as, KHE_AVAIL_NODE_WORKLOAD, INT_MAX,
      (float) max_lim, KheAvailSolverTimeSetFromTimeGroup(as, tg),
      (KHE_MONITOR) m);
    *** **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverSolve(KHE_AVAIL_SOLVER as)                            */
/*                                                                           */
/*  Solve for curr_r.                                                        */
/*                                                                           */
/*  Implementation note.  This function makes three passes over the          */
/*  resource monitors of as->curr_r:                                         */
/*                                                                           */
/*  Pass 1:  zero-limit monitors                                             */
/*  Pass 2:  non-zero limit monitors                                         */
/*  Pass 3:  non-zero limit monitors again.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverSolve(KHE_AVAIL_SOLVER as, int index,
  bool include_unavailable)
/* static KHE_AVAIL_SET KheAvailSolverFindBestBusyTimes(KHE_AVAIL_SOLVER as) */
{
  KHE_CONSTRAINT c;  KHE_MONITOR m;  int i, pass;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheAvailSolverSolve(as)\n");
    KheAvailSolverDebug(as, 1, 2, stderr);
  }

  /* clear out retained information about any previous resource */
  KheLoadSolverClear(as->busy_times_solver[index], as);
  KheLoadSolverClear(as->workload_solver[index], as);
  HaArrayClear(as->zero_times);
  HaArrayFill(as->zero_times, KheInstanceTimeCount(as->instance), false);

  /* add nodes for unassignable times */
  if( DEBUG1 )
    fprintf(stderr, "  calling KheAvailSolverAddUnassignableTimeNodes:\n");
  KheAvailSolverAddUnassignableTimeNodes(as, index);

  for( pass = 0;  pass < 3;  pass++ )
  {
    if( DEBUG1 )
      fprintf(stderr, "  pass %d:\n", pass);
    for( i = 0;  i < KheSolnResourceMonitorCount(as->soln, as->curr_r);  i++ )
    {
      m = KheSolnResourceMonitor(as->soln, as->curr_r, i);
      c = KheMonitorConstraint(m);
      if( c != NULL && KheConstraintWeight(c) > 0 )
	switch( KheMonitorTag(m) )
	{
	  case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	    /* handle on pass 0, or not at all */
	    if( include_unavailable && pass == 0 )
	      KheAvailSolverAddUnavailableTimeNodes(as, index,
		(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
	    break;

	  case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	    /* handle on pass 0 if limit is 0, otherwise on passes 1 and 2 */
	    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
	    cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	    if( pass == 0 )
	    {
	      if( KheClusterBusyTimesConstraintMaximum(cbtc) == 0 )
		KheAvailSolverAddClusterBusyZeroNodes(as, index, cbtm);
	    }
	    else /* pass == 1 or pass == 2 */
	    {
	      if( KheClusterBusyTimesConstraintMaximum(cbtc) > 0 &&
		  KheClusterBusyTimesConstraintTimeGroupsDisjoint(cbtc) )
	      {
		KheAvailSolverAddClusterBusyNodes(as, index, cbtm,KHE_POSITIVE);
		KheAvailSolverAddClusterBusyNodes(as, index, cbtm,KHE_NEGATIVE);
	      }
	    }
	    break;

	  case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	    /* handle on pass 0 if limit is 0, otherwise on passes 1 and 2 */
	    lbtc = (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
	    lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	    if( pass == 0 )
	    {
	      if( KheLimitBusyTimesConstraintMaximum(lbtc) == 0 )
		KheAvailSolverAddLimitBusyZeroNodes(as, index, lbtm);
	    }
	    else /* pass == 1 or pass == 2 */
	    {
	      if( KheLimitBusyTimesConstraintMaximum(lbtc) > 0 )
		KheAvailSolverAddLimitBusyNodes(as, index, lbtm);
	    }
	    break;

	  case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	    /* handle on passes 1 and 2 only */
	    if( pass >= 1 )
	    {
	      lwm = (KHE_LIMIT_WORKLOAD_MONITOR) m;
	      KheAvailSolverAddLimitWorkloadNodes(as, index, lwm);
	    }
	    break;

	  default:

	    /* ignore all other monitors */
	    break;
	}
    }
  }

  /* find the best avail sets and return */
  if( DEBUG1 )
    fprintf(stderr, "  avail finding best avail set:\n");
  KheLoadSolverFindBestAvailSet(as->busy_times_solver[index],
    KHE_AVAIL_SET_TIMES, as);
  KheLoadSolverFindBestAvailSet(as->workload_solver[index],
    KHE_AVAIL_SET_WORKLOAD, as);
  if( !KheLoadSolverBestContainsLimitWorkloadNode(as->workload_solver[index]) )
    KheLoadSolverClearBest(as->workload_solver[index], as);
  if( DEBUG1 )
    fprintf(stderr, "] AvailSolverSolve returning\n");
}


/* *** old version
static void KheAvailSolverSolve(KHE_AVAIL_SOLVER as)
** static KHE_AVAIL_SET KheAvailSolverFindBestBusyTimes(KHE_AVAIL_SOLVER as) **
{
  KHE_CONSTRAINT c;  KHE_MONITOR m;  int i, pass;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ AvailSolverFindBestBusyTimes(as)\n");
    KheAvailSolverDebug(as, 1, 2, stderr);
  }

  ** add nodes for unassignable times **
  if( DEBUG1 )
    fprintf(stderr, "  calling KheAvailSolverAddUnassignableTimeNodes:\n");
  KheAvailSolverAddUnassignableTimeNodes(as);

  ** for each monitor, either add its nodes immediately or save it for later **
  if( DEBUG1 )
    fprintf(stderr, "  avail adding zero nodes:\n");
  HaArrayClear(as->limit_and_cluster_monitors);
  HaArrayClear(as->workload_monitors);
  for( i = 0;  i < KheSolnResourceMonitorCount(as->soln, as->curr_r);  i++ )
  {
    m = KheSolnResourceMonitor(as->soln, as->curr_r, i);
    c = KheMonitor Constraint(m);
    if( c != NULL && KheConstraintWeight(c) > 0 )
      switch( KheMonitorTag(m) )
      {
	case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	  ** handle immediately **
	  KheAvailSolverAddUnavailableTimeNodes(as,
	    (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
	  break;

	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	  ** handle immediately if limit is 0, save for later otherwise **
	  cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  if( KheClusterBusyTimesConstraintMaximum(cbtc) == 0 )
	    KheAvailSolverAddClusterBusyZeroNodes(as, cbtm);
	  else if( KheClusterBusyTimesConstraintTimeGroupsDisjoint(cbtc) )
	    HaArrayAddLast(as->limit_and_cluster_monitors, m);
	  break;

	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	  ** handle immediately if limit is 0, save for later otherwise **
	  lbtc = (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
	  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	  if( KheLimitBusyTimesConstraintMaximum(lbtc) == 0 )
	    KheAvailSolverAddLimitBusyZeroNodes(as, lbtm);
	  else
	    HaArrayAddLast(as->limit_and_cluster_monitors, m);
	  break;

	case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	  ** save for later **
	  lwm = (KHE_LIMIT_WORKLOAD_MONITOR) m;
	  HaArrayAddLast(as->workload_monitors, lwm);
	  break;

	default:

	  ** ignore all other monitors **
	  break;
      }
  }

  ** try each monitor saved above twice:  one can open the way to another **
  if( DEBUG1 )
    fprintf(stderr, "  avail adding non-zero nodes:\n");
  for( attempts = 1;  attempts <= 2;  attempts++ )
  {
    ** limit and cluster busy times monitors **
    HaArrayForEach(as->limit_and_cluster_monitors, m, i)
      if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
      {
	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	KheAvailSolverAddLimitBusyNodes(as, lbtm);
      }
      else
      {
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
        KheAvailSolverAddClusterBusyNodes(as, cbtm, KHE_POSITIVE);
        KheAvailSolverAddClusterBusyNodes(as, cbtm, KHE_NEGATIVE);
      }

    ** limit workload monitors **
    HaArrayForEach(as->workload_monitors, lwm, i)
      KheAvailSolverAddLimitWorkloadNodes(as, lwm);
  }


  ** find the best avail sets and return **
  if( DEBUG1 )
    fprintf(stderr, "  avail finding best avail set:\n");
  KheLoadSolverFindBestAvailSet(as->busy_times_solver, KHE_AVAIL_SET_TIMES, as);
  KheLoadSolverFindBestAvailSet(as->workload_solver, KHE_AVAIL_SET_WORKLOAD,as);
  if( DEBUG1 )
    fprintf(stderr, "] AvailSolverSolve returning\n");
  ** ***
  res = KheAvailSolverFindBestAvailSet(as, KHE_AVAIL_SET_TIMES);
  if( DEBUG1 )
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "AvailSolverFindBestBusyTimes returning:\n");
      if( res != NULL )
	KheAvailSetDebug(res, 1, 2, stderr);
      KheAvailSolverDebug(as, 1, 2, stderr);
    }
    fprintf(stderr, "] AvailSolverFindBestBusyTimes\n");
  }
  return res;
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SET AvailSolverFindBestBusyTimes(KHE_AVAIL_SOLVER as)          */
/*                                                                           */
/*  Return an avail set for the maximum number of busy times.                */
/*                                                                           */
/*****************************************************************************/

/* *** old version
static KHE_AVAIL_SET AvailSolverFindBestBusyTimes(KHE_AVAIL_SOLVER as)
{
  int i, j, max_lim, offset;
  KHE_TIME_SET ts;  KHE_TIME_GROUP tg;  KHE_MONITOR m;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;

  ** handle all time groups from limit busy times monitors **
  for( i = 0;  i < KheSolnResourceMonitorCount(as->soln, as->resource);  i++ )
  {
    m = KheSolnResourceMonitor(as->soln, as->resource, i);
    if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
    {
      lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
      lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
      if( KheConstraintWeight((KHE_CONSTRAINT) lbtc) > 0 )
      {
	max_lim = KheLimitBusyTimesConstraintMaximum(lbtc);
	offset = KheLimitBusyTimesMonitorOffset(lbtm);
	for( j = 0;  j < KheLimitBusyTimesConstraintTimeGroupCount(lbtc);  j++ )
	{
	  ** make an avail node for tg and add it to the list **
	  tg = KheLimitBusyTimesConstraintTimeGroup(lbtc, j, offset);
	  ts = KheTimeSetFromTimeGroup(tg, as);
	  KheAvailSolverMakeAndAddAvailNodeAndSubsets(as,
	    KHE_AVAIL_NODE_LIMIT_BUSY, max_lim, ts, m);
	}
      }
    }
  }

  ** sort and uniqueify the limit 1 nodes we have so far **
  ** AvailSolverSortMaxOneTimeSets(as); **

  ** handle all cluster busy times monitors **
  for( i = 0;  i < KheSolnResourceMonitorCount(as->soln, as->resource);  i++ )
  {
    m = KheSolnResourceMonitor(as->soln, as->resource, i);
    if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
      if( KheConstraintWeight((KHE_CONSTRAINT) cbtc) > 0 &&
          KheClusterBusyTimesConstraintTimeGroupsDisjoint(cbtc) &&
	  (KheClusterBusyTimesConstraintAllPositive(cbtc) ||
	   KheClusterBusyTimesConstraintAllNegative(cbtc)) &&
	  KheClusterMonitorHasMaxOneTimeGroups(as, cbtm) )
      {
	if( KheClusterBusyTimesConstraintAllPositive(cbtc) )
	{
	  ** all positive, limit is just the max limit **
	  max_lim = KheClusterBusyTimesConstraintMaximum(cbtc);
	}
	else
	{
	  ** all negative, limit is time groups minus the min limit **
	  max_lim = KheClusterBusyTimesMonitorTimeGroupCount(cbtm) -
            KheClusterBusyTimesConstraintMinimum(cbtc);
	}

	** make an avail node for cbtm and add it to the list **
	ts = KheClusterMonitorTimeSet(cbtm, as);
	KheAvailSolverMakeAndAddAvailNode(as, KHE_AVAIL_NODE_CLUSTER_BUSY,
          max_lim, ts, m);
      }
    }
  }

  ** 1 ** AvailSolverAddSingleTimeNodes(as);
  ** 2 ** KheAvailSolverAddUnassignableTimeNodes(as);
  AvailSolverAddNodesForMonitorsSimple(as);
  AvailSolverAddNodesForMonitorsComplex(as);
  ** 7 ** KheAvailSolverAddClusterBusyNodes(as);
  ** 8 ** AvailSolverAddClusterBusyMinNodes(as);
  return AvailSolverFindBestAvailSet(as, KHE_AVAIL_SET_TIMES);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SET KheAvailSolverFindBestWorkload(KHE_AVAIL_SOLVER as)        */
/*                                                                           */
/*  Return an avail set for the maximum workload as found by as.             */
/*                                                                           */
/*  Implementation note.  At the time this function is called, all           */
/*  relevant limit workload monitors will be in as->workload_monitors.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_AVAIL_SET KheAvailSolverFindBestWorkload(KHE_AVAIL_SOLVER as)
{
  int i;  KHE_LIMIT_WORKLOAD_MONITOR lwm;  KHE_AVAIL_SET res;
  if( DEBUG1 )
    fprintf(stderr, "[ AvailSolverFindBestWorkload(as)\n");
  HaArrayForEach(as->workload_monitors, lwm, i)
    KheAvailSolverAddWorkloadNodes(as, lwm);
  res = KheAvailSolverFindBestAvailSet(as, KHE_AVAIL_SET_WORKLOAD);
  if( DEBUG1 )
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "  AvailSolverFindBestWorkload returning:\n");
      if( res != NULL )
	KheAvailSetDebug(res, 1, 2, stderr);
      KheAvailSolverDebug(as, 1, 2, stderr);
    }
    fprintf(stderr, "] AvailSolverFindBestWorkload\n");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverFreeNodes(KHE_AVAIL_SOLVER as, KHE_AVAIL_SET keep_set)*/
/*                                                                           */
/*  Free the nodes of as, except keep any in keep_set.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheAvailSolverFreeNodes(KHE_AVAIL_SOLVER as, KHE_AVAIL_SET keep_set)
{
  KHE_AVAIL_NODE an;  int i, pos;
  HpTableClear(as->node_table);
  HaArrayForEach(as->node_array, an, i)
    if( keep_set == NULL || !HaArrayContains(keep_set->nodes, an, &pos) )
      KheAvailNodeFree(an, as);
  HaArrayClear(as->node_array);
  HaArrayClear(as->zero_times);
  HaArrayFill(as->zero_times, KheInstanceTimeCount(as->instance), false);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverSetResource(KHE_AVAIL_SOLVER as, KHE_RESOURCE r)      */
/*                                                                           */
/*  Set as to work on resource r.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheAvailSolverSetResource(KHE_AVAIL_SOLVER as, KHE_RESOURCE r)
{
  KHE_AVAIL_SET avail_set_0, avail_set_1;
  if( DEBUG1 )
    fprintf(stderr, "[ KheAvailSolverSetResource(as, %s%s)\n",
      r == NULL ? "-" : KheResourceId(r),
      r == as->curr_r ? " unchanged" : "");
  HnAssert(r != NULL, "KheAvailSolverSetResource:  r is NULL");
  HnAssert(KheResourceInstance(r) == as->instance,
    "KheAvailSolverSetResource:  r is from the wrong instance");
  if( r != as->curr_r )
  {
    /* clear out retained information about any previous resource */
    /* *** moved to KheAvailSolverSolve
    KheLoadSolverClear(as->busy_times_solver, as);
    KheLoadSolverClear(as->workload_solver, as);
    HaArrayClear(as->zero_times);
    HaArrayFill(as->zero_times, KheInstanceTimeCount(as->instance), false);
    *** */
    /* ***
    if( as->best_busy_times != NULL )
    {
      KheAvailSetFree(as->best_busy_times, true, as);
      as->best_busy_times = NULL;
    }
    if( as->best_workload != NULL )
    {
      KheAvailSetFree(as->best_workload, true, as);
      as->best_workload = NULL;
    }
    *** */

    /* build up the wanted information about rt */
    if( KheResourceResourceType(r) != as->curr_rt )
    {
      /* preasst info for as->curr_rt */
      /* *** obsolete
      if( as->preasst_info != NULL )
	KhePreasstInfoFree(as->preasst_info, as);
      as->preasst_info = KhePreasstInfoBuild(as);
      *** */

      /* ersets for as->curr_rt */
      as->curr_rt = KheResourceResourceType(r);
      if( as->erset_all != NULL )
	KheErSetAllFree(as->erset_all, as);
      as->erset_all = KheErSetAllBuild(as);
    }

    /* build up the wanted information about r */
    as->curr_r = r;
    KheAvailSolverSolve(as, 0, true);
    KheAvailSolverSolve(as, 1, false);
    avail_set_0 = as->busy_times_solver[0]->best_avail_set;
    avail_set_1 = as->busy_times_solver[1]->best_avail_set;
    /* ***
    HnAssert(avail_set_0 != NULL,
      "KheAvailSolverSetResource internal error 1");
    HnAssert(avail_set_1 != NULL,
      "KheAvailSolverSetResource internal error 2");
    *** */
    if( avail_set_0 == NULL )
    {
      as->best_busy_times_solver = as->busy_times_solver[1];
      as->best_workload_solver = as->workload_solver[1];
    }
    else if( avail_set_1 == NULL )
    {
      as->best_busy_times_solver = as->busy_times_solver[0];
      as->best_workload_solver = as->workload_solver[0];
    }
    else if( avail_set_0->limit.u.int_val < avail_set_1->limit.u.int_val )
    {
      as->best_busy_times_solver = as->busy_times_solver[0];
      as->best_workload_solver = as->workload_solver[0];
    }
    else
    {
      as->best_busy_times_solver = as->busy_times_solver[1];
      as->best_workload_solver = as->workload_solver[1];
    }
    /* ***
    as->best_busy_times = KheAvailSolverFindBestBusyTimes(as);
    KheAvailSolverFreeNodes(as, as->best_busy_times);
    as->best_workload = KheAvailSolverFindBestWorkload(as);
    KheAvailSolverFreeNodes(as, as->best_workload);
    *** */
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheAvailSolverSetResource returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public queries"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSolverMaxBusyTimes(KHE_AVAIL_SOLVER as, int *res)           */
/*                                                                           */
/*  If the set resource has a max busy times limit, return true and set      */
/*  *res to that limit.  Otherwise return false with *res set to INT_MAX.    */
/*                                                                           */
/*****************************************************************************/

bool KheAvailSolverMaxBusyTimes(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int *res)
{
  KHE_AVAIL_SET best_avail_set;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_busy_times_solver->best_avail_set;
  if( best_avail_set != NULL )
    return *res = best_avail_set->limit.u.int_val, true;
  else
    return *res = INT_MAX, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvailSolverMaxWorkload(KHE_AVAIL_SOLVER as, float *res)          */
/*                                                                           */
/*  If the set resource has a max workload limit, return true and set        */
/*  *res to that limit.  Otherwise return false with *res set to FLT_MAX.    */
/*                                                                           */
/*****************************************************************************/

bool KheAvailSolverMaxWorkload(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, float *res)
{
  KHE_AVAIL_SET best_avail_set;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_workload_solver->best_avail_set;
  if( best_avail_set != NULL )
    return *res = best_avail_set->limit.u.float_val, true;
  else
    return *res = FLT_MAX, false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailSolverMaxBusyTimesAvailNodeCount(KHE_AVAIL_SOLVER as)        */
/*                                                                           */
/*  Return the number of avail nodes in the independent set that defines     */
/*  the maximum busy times of the set resource.                              */
/*                                                                           */
/*****************************************************************************/

int KheAvailSolverMaxBusyTimesAvailNodeCount(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r)
{
  KHE_AVAIL_SET best_avail_set;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_busy_times_solver->best_avail_set;
  return best_avail_set == NULL ? 0 : HaArrayCount(best_avail_set->nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverMaxBusyTimesAvailNode(KHE_AVAIL_SOLVER as, int i,     */
/*    KHE_AVAIL_NODE_TYPE *type, int *limit, KHE_TIME_SET *ts,KHE_MONITOR *m)*/
/*                                                                           */
/*  Return the fields of the i'th avail node of the independent set that     */
/*  defines the maximum busy times of the set resource.                      */
/*                                                                           */
/*****************************************************************************/

void KheAvailSolverMaxBusyTimesAvailNode(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int i, KHE_AVAIL_NODE_TYPE *type, int *limit,
  KHE_TIME_SET *ts, KHE_MONITOR *m)
{
  KHE_AVAIL_SET best_avail_set;  KHE_AVAIL_NODE an;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_busy_times_solver->best_avail_set;
  HnAssert(best_avail_set != NULL,
    "KheAvailSolverMaxBusyTimesAvailNode: no max busy times");
  an = HaArray(best_avail_set->nodes, i);
  *type = an->type;
  *limit = an->limit.u.int_val;
  *ts = an->time_set;
  *m = an->monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailSolverMaxWorkloadAvailNodeCount(KHE_AVAIL_SOLVER as)         */
/*                                                                           */
/*  Return the number of avail nodes in the independent set that defines     */
/*  the maximum workload of the set resource.                                */
/*                                                                           */
/*****************************************************************************/

int KheAvailSolverMaxWorkloadAvailNodeCount(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r)
{
  KHE_AVAIL_SET best_avail_set;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_workload_solver->best_avail_set;
  return best_avail_set == NULL ? 0 : HaArrayCount(best_avail_set->nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailSolverMaxWorkloadAvailNode(KHE_AVAIL_SOLVER as, int i,      */
/*    KHE_AVAIL_NODE_TYPE *type, int *limit, KHE_TIME_SET *ts,KHE_MONITOR *m)*/
/*                                                                           */
/*  Return the fields of the i'th avail node of the independent set that     */
/*  defines the maximum workload of the set resource.                        */
/*                                                                           */
/*****************************************************************************/

void KheAvailSolverMaxWorkloadAvailNode(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int i, KHE_AVAIL_NODE_TYPE *type, float *limit,
  KHE_TIME_SET *ts, KHE_MONITOR *m)
{
  KHE_AVAIL_SET best_avail_set;  KHE_AVAIL_NODE an;
  KheAvailSolverSetResource(as, r);
  best_avail_set = as->best_workload_solver->best_avail_set;
  HnAssert(best_avail_set != NULL,
    "KheAvailSolverMaxWorkloadAvailNode: no max workload");
  an = HaArray(best_avail_set->nodes, i);
  *type = an->type;
  *limit = an->limit.u.float_val;
  *ts = an->time_set;
  *m = an->monitor;
}

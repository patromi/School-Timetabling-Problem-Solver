
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
/*  FILE:         khe_sr_reassign.c                                          */
/*  DESCRIPTION:  Resource reassignment repair                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_mmatch.h"
#include <limits.h>

#define bool_show(val)  ((val) ? "true" : "false")

#define DEBUG1 0		/* public functions			    */
#define DEBUG2 0		/* individual solves and what led to them   */
#define DEBUG3 0		/* ResourceInfoEnsureGroupsUpToDate	    */
#define DEBUG4 0		/* GroupMake and GroupDelete		    */
#define	DEBUG5 0		/* solve details			    */
#define DEBUG6 0		/* brief display of improvements	    */
#define DEBUG8 0		/* SolveDefective			    */
#define DEBUG8_ID "HN_2"	/* SolveDefective			    */
#define DEBUG9 0		/*			   		    */
#define DEBUG10 0		/* Matching solver	   		    */
#define DEBUG11 0		/* Matching solver	   		    */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL - an interval (private)                                     */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_solvers.h
typedef struct khe_interval_rec {
  int first;
  int last;
} KHE_INTERVAL;
*** */

typedef HA_ARRAY(KHE_INTERVAL) ARRAY_KHE_INTERVAL;


/*****************************************************************************/
/*                                                                           */
/*  REASSIGN_SELECT_TYPE - types of selection                                */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  REASSIGN_SELECT_NONE,
  REASSIGN_SELECT_ADJACENT,
  REASSIGN_SELECT_DEFECTIVE,
  REASSIGN_SELECT_ALL,
  REASSIGN_SELECT_CONSTRAINT
} REASSIGN_SELECT_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  RESOURCE_DEFECTIVE - whether resource timetable is defective             */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  RESOURCE_DEFECTIVE_UNKNOWN,
  RESOURCE_DEFECTIVE_NO,
  RESOURCE_DEFECTIVE_YES
} RESOURCE_DEFECTIVE;


/*****************************************************************************/
/*                                                                           */
/*  REASSIGN_ATTRIBUTES - attributes that affect grouping                    */
/*                                                                           */
/*****************************************************************************/

typedef struct reassign_attributes_rec {
  KHE_INTERVAL		interval;
  KHE_REASSIGN_GROUPING	grouping;
  bool			allow_partial;
} REASSIGN_ATTRIBUTES;


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP - one group of tasks, with their interval and domain           */
/*  RESOURCE_INFO - information about one resource                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_group_rec *KHE_GROUP;
typedef HA_ARRAY(KHE_GROUP) ARRAY_KHE_GROUP;

typedef struct resource_info_rec *RESOURCE_INFO;
typedef HA_ARRAY(RESOURCE_INFO) ARRAY_RESOURCE_INFO;

struct khe_group_rec {
  KHE_TASK_SET		task_set;		/* the group's tasks         */
  bool			unassignable;		/* tasks don't need asst     */
  KHE_INTERVAL		interval;		/* interval                  */
  ARRAY_RESOURCE_INFO	domain;			/* legal resources           */
  KHE_MMATCH_NODE	mmatch_node;		/* node when matching        */
};

struct resource_info_rec {
  KHE_RESOURCE		resource;		/* the resource              */
  REASSIGN_ATTRIBUTES	group_attributes;	/* its groups' attributes    */
  ARRAY_KHE_GROUP	groups;			/* its groups                */
  RESOURCE_DEFECTIVE	defective;		/* if resource is defective  */
  int			curr_last_index;	/* solve last assigned day   */
  KHE_MMATCH_NODE	mmatch_node;		/* node when matching        */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_REASSIGN_SOLVER - a reassign solver                                  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) ARRAY_CLUSTER;
typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

struct khe_reassign_solver_rec {

  /* used throughout the lifetime of the solver */
  HA_ARENA		arena;
  KHE_SOLN		soln;
  KHE_RESOURCE_TYPE	resource_type;
  KHE_TASK_FINDER	task_finder;
  bool			resource_invariant;
  ARRAY_RESOURCE_INFO	dormant_resources;
  ARRAY_RESOURCE_INFO	resources;
  ARRAY_KHE_GROUP	free_groups;
  KHE_MMATCH		mmatch;

  /* used by KheReassignRepair */
  ARRAY_CLUSTER		select_constraints;
  ARRAY_KHE_RESOURCE	violating_resources;
  ARRAY_KHE_RESOURCE	slack_resources;
  ARRAY_KHE_INTERVAL	intervals;

  /* used by individual solves (KheReassignSolverDoSolve) */
  ARRAY_KHE_GROUP	movable_groups;
  ARRAY_KHE_GROUP	immovable_groups;
  KHE_REASSIGN_METHOD	method;
  int			max_assignments;
  KHE_MARK		init_mark;
  int			init_defect_count;
  bool			debug_on;
  int			curr_assignments;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "intervals"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheInterval(int first, int last)                            */
/*                                                                           */
/*  Return a new interval with these attributes.                             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static KHE_INTERVAL KheInterval(int first, int last)
{
  KHE_INTERVAL res;
  res.first = first;
  res.last = last;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEmpty(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return true if in is empty.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalEmpty(KHE_INTERVAL in)
{
  return in.first > in.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)                */
/*                                                                           */
/*  Return true when in1 and in2 are equal.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  if( KheIntervalEmpty(in1) )
    return KheIntervalEmpty(in2);
  else
    return in1.first == in2.first && in1.last == in2.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)             */
/*                                                                           */
/*  Return true if in1 and in2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) || KheIntervalEmpty(in2) ||
    in1.last < in2.first || in2.last < in1.first;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Comparison function for sorting an array of intervals.                   */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static int KheIntervalTypedCmp(KHE_INTERVAL i1, KHE_INTERVAL i2)
{
  if( i1.first != i2.first )
    return i1.first - i2.first;
  else
    return i1.last - i2.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Comparison function for sorting an array of intervals.                   */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static int KheIntervalCmp(const void *t1, const void *t2)
{
  KHE_INTERVAL i1 = * (KHE_INTERVAL *) t1;
  KHE_INTERVAL i2 = * (KHE_INTERVAL *) t2;
  return KheIntervalTypedCmp(i1, i2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheIntervalShow(KHE_INTERVAL in, KHE_FRAME frame)                  */
/*                                                                           */
/*  Show in in static memory.                                                */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static char *KheIntervalShow(KHE_INTERVAL in, KHE_FRAME frame)
{
  static char buff[500];
  if( KheIntervalEmpty(in) )
    sprintf(buff, "-");
  else if( frame != NULL )
    sprintf(buff, "%s-%s",
      KheTimeGroupId(KheFrameTimeGroup(frame, in.first)),
      KheTimeGroupId(KheFrameTimeGroup(frame, in.last)));
  else
    sprintf(buff, "%d-%d", in.first, in.last);
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_REASSIGN_GROUPING ReassignGroupingGet(KHE_OPTIONS options, char *key)*/
/*                                                                           */
/*  Return an enum value representing the rs_reassign_grouping option.       */
/*                                                                           */
/*****************************************************************************/

static KHE_REASSIGN_GROUPING ReassignGroupingGet(KHE_OPTIONS options, char *key)
{
  char *val;
  val = KheOptionsGet(options, key, "minimal");
  if( strcmp(val, "minimal") == 0 )
    return KHE_REASSIGN_MINIMAL;
  else if( strcmp(val, "runs") == 0 )
    return KHE_REASSIGN_RUNS;
  else if( strcmp(val, "maximal") == 0 )
    return KHE_REASSIGN_MAXIMAL;
  else
  {
    HnAbort("KheReassignRepair: unknown %s value \"%s\"", key, val);
    return KHE_REASSIGN_MINIMAL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *ReassignGroupingShow(KHE_REASSIGN_GROUPING rg)                     */
/*                                                                           */
/*  Show rg.                                                                 */
/*                                                                           */
/*****************************************************************************/

static char *ReassignGroupingShow(KHE_REASSIGN_GROUPING rg)
{
  switch( rg )
  {
    case KHE_REASSIGN_MINIMAL:	return "minimal";
    case KHE_REASSIGN_RUNS:	return "runs";
    case KHE_REASSIGN_MAXIMAL:	return "maximal";
    default:			return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "method"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_REASSIGN_METHOD ReassignMethodGet(KHE_OPTIONS options, char *key)    */
/*                                                                           */
/*  Return an enum value representing the rs_reassign_method option.         */
/*                                                                           */
/*****************************************************************************/

static KHE_REASSIGN_METHOD ReassignMethodGet(KHE_OPTIONS options, char *key)
{
  char *val;
  val = KheOptionsGet(options, key, "exhaustive");
  if( strcmp(val, "exhaustive") == 0 )
    return KHE_REASSIGN_EXHAUSTIVE;
  else if( strcmp(val, "matching") == 0 )
    return KHE_REASSIGN_MATCHING;
  else
  {
    HnAbort("KheReassignRepair: unknown %s value \"%s\"", key, val);
    return KHE_REASSIGN_EXHAUSTIVE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *ReassignMethodShow(KHE_REASSIGN_METHOD method)                     */
/*                                                                           */
/*  Show method.                                                             */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently unused
static char *ReassignMethodShow(KHE_REASSIGN_METHOD method)
{
  switch( method )
  {
    case KHE_REASSIGN_EXHAUSTIVE:	return "exhaustive";
    case KHE_REASSIGN_MATCHING:		return "matching";
    default:				return "?";
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reassign select type"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  REASSIGN_SELECT_TYPE ReassignSelectTypeGet(KHE_OPTIONS options,          */
/*    char *key, char **str)                                                 */
/*                                                                           */
/*  Return an enum value representing the rs_reassign_select option.         */
/*                                                                           */
/*****************************************************************************/

static REASSIGN_SELECT_TYPE ReassignSelectTypeGet(KHE_OPTIONS options,
  char *key, char **str)
{
  char *val;
  val = KheOptionsGet(options, key, "none");
  if( strcmp(val, "none") == 0 )
    return *str = NULL, REASSIGN_SELECT_NONE;
  else if( strcmp(val, "adjacent") == 0 )
    return *str = NULL, REASSIGN_SELECT_ADJACENT;
  else if( strcmp(val, "defective") == 0 )
    return *str = NULL, REASSIGN_SELECT_DEFECTIVE;
  else if( strcmp(val, "all") == 0 )
    return *str = NULL, REASSIGN_SELECT_ALL;
  else if( strstr(val, "constraint:") == val )
    return *str = &val[strlen("constraint:")], REASSIGN_SELECT_CONSTRAINT;
  else
  {
    HnAbort("KheReassignRepair: unknown %s value \"%s\"", key, val);
    return *str = NULL, REASSIGN_SELECT_ALL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *ReassignSelectTypeShow(REASSIGN_SELECT_TYPE rs, char *str)         */
/*                                                                           */
/*  Display a reassign_select value.                                         */
/*                                                                           */
/*****************************************************************************/

static char *ReassignSelectTypeShow(REASSIGN_SELECT_TYPE rs, char *str)
{
  static char buff[200];
  switch( rs )
  {
    case REASSIGN_SELECT_DEFECTIVE:

      return "defective";

    case REASSIGN_SELECT_NONE:

      return "none";

    case REASSIGN_SELECT_ADJACENT:

      return "adjacent";

    case REASSIGN_SELECT_ALL:

      return "all";

    case REASSIGN_SELECT_CONSTRAINT:

      snprintf(buff, 200, "constraint:%s", str);
      return buff;

    default:

      return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource defective"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *ResourceDefectiveShow(RESOURCE_DEFECTIVE defective)                */
/*                                                                           */
/*  Display a resource's defectiveness.                                      */
/*                                                                           */
/*****************************************************************************/

static char *ResourceDefectiveShow(RESOURCE_DEFECTIVE defective)
{
  switch( defective )
  {
    case RESOURCE_DEFECTIVE_UNKNOWN:	return "unknown";
    case RESOURCE_DEFECTIVE_NO:		return "no";
    case RESOURCE_DEFECTIVE_YES:	return "yes";
    default:				return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reassign attributes"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  REASSIGN_ATTRIBUTES ReassignAttributesMake(int first_index,              */
/*    int last_index, KHE_REASSIGN_GROUPING grouping, bool allow_partial)   */
/*                                                                           */
/*  Make a reassign attributes object.  These are the attributes that        */
/*  affect which groups are needed for some resource when solving.           */
/*                                                                           */
/*****************************************************************************/

static REASSIGN_ATTRIBUTES ReassignAttributesMake(KHE_INTERVAL in,
  KHE_REASSIGN_GROUPING grouping, bool allow_partial)
{
  REASSIGN_ATTRIBUTES res;
  res.interval = in;
  res.grouping = grouping;
  res.allow_partial = allow_partial;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ReassignAttributesEqual(REASSIGN_ATTRIBUTES ra1,                    */
/*    REASSIGN_ATTRIBUTES ra2)                                               */
/*                                                                           */
/*  Return true if ra1 and ra2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

static bool ReassignAttributesEqual(REASSIGN_ATTRIBUTES ra1,
  REASSIGN_ATTRIBUTES ra2)
{
  return KheIntervalEqual(ra1.interval, ra2.interval) &&
    ra1.grouping == ra2.grouping &&
    ra1.allow_partial == ra2.allow_partial;
}


/*****************************************************************************/
/*                                                                           */
/*  char *ReassignAttributesShow(REASSIGN_ATTRIBUTES ra,                     */
/*    KHE_REASSIGN_SOLVER rs)                                                */
/*                                                                           */
/*  Return a static string display of ra.                                    */
/*                                                                           */
/*****************************************************************************/

static char *ReassignAttributesShow(REASSIGN_ATTRIBUTES ra,
  KHE_REASSIGN_SOLVER rs)
{
  static char buff[200];  KHE_FRAME frame;
  frame = rs == NULL ? NULL : KheTaskFinderFrame(rs->task_finder);
  snprintf(buff, 200, "%s:%s:%s", KheIntervalShow(ra.interval, frame),
    ReassignGroupingShow(ra.grouping), bool_show(ra.allow_partial));
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "groups (of tasks)"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP KheGroupMake(KHE_RESOURCE_GROUP_SOLVER rs)                     */
/*                                                                           */
/*  Make or get a group object, whose task set and domain are initially      */
/*  empty, and whose first_index and last_index fields are undefined.        */
/*                                                                           */
/*****************************************************************************/

static KHE_GROUP KheGroupMake(KHE_REASSIGN_SOLVER rs)
{
  KHE_GROUP res;
  if( HaArrayCount(rs->free_groups) > 0 )
  {
    /* get the group object from the free list */
    res = HaArrayLastAndDelete(rs->free_groups);
    KheTaskSetClear(res->task_set);
    HaArrayClear(res->domain);
    if( DEBUG4 )
      fprintf(stderr, "KheGroupMake ret. %p from free list\n", (void *) res);
  }
  else
  {
    /* make a new group object */
    HaMake(res, rs->arena);
    res->task_set = KheTaskSetMake(rs->soln);
    HaArrayInit(res->domain, rs->arena);
    if( DEBUG4 )
      fprintf(stderr, "KheGroupMake returning %p from arena\n", (void *) res);
  }
  res->mmatch_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupDelete(KHE_GROUP group, KHE_REASSIGN_SOLVER rs)             */
/*                                                                           */
/*  Delete group; place it on rs's free list.  Its task set and domain go    */
/*  with it.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheGroupDelete(KHE_GROUP group, KHE_REASSIGN_SOLVER rs)
{
  if( DEBUG4 )
    fprintf(stderr, "KheGroupDelete deleting %p\n", (void *) group);
  HaArrayAddLast(rs->free_groups, group);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupMoveCheck(KHE_GROUP group, KHE_RESOURCE r)                  */
/*                                                                           */
/*  Check moving group to r, returning true if it would succeed.             */
/*                                                                           */
/*  If the group is already assigned r, do nothing but count this as         */
/*  successful too.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupMoveCheck(KHE_GROUP group, KHE_RESOURCE r)
{
  KHE_TASK first_task;
  first_task = KheTaskSetFirst(group->task_set);
  if( KheTaskAsstResource(first_task) == r )
    return true;
  else
    return KheTaskSetMoveResourceCheck(group->task_set, r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupMove(KHE_GROUP group, KHE_RESOURCE r)                       */
/*                                                                           */
/*  Move group to r, returning true if successful.                           */
/*                                                                           */
/*  If the group is already assigned r, do nothing but count this as         */
/*  successful too.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupMove(KHE_GROUP group, KHE_RESOURCE r)
{
  KHE_TASK first_task;
  first_task = KheTaskSetFirst(group->task_set);
  if( KheTaskAsstResource(first_task) == r )
    return true;
  else
    return KheTaskSetMoveResource(group->task_set, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupCmp(const void *t1, const void *t2)                          */
/*                                                                           */
/*  Comparison function for sorting an array of groups into increasing order.*/
/*                                                                           */
/*****************************************************************************/

static int KheGroupCmp(const void *t1, const void *t2)
{
  KHE_GROUP group1 = * (KHE_GROUP *) t1;
  KHE_GROUP group2 = * (KHE_GROUP *) t2;
  return KheIntervalTypedCmp(group1->interval, group2->interval);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupDisjoint(KHE_GROUP group1, KHE_GROUP group2)                */
/*                                                                           */
/*  Return true if group1 and group2 are disjoint in time.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupDisjoint(KHE_GROUP group1, KHE_GROUP group2)
{
  return KheIntervalDisjoint(group1->interval, group2->interval);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupOverlap(KHE_GROUP group1, KHE_GROUP group2)                 */
/*                                                                           */
/*  Return true if group1 and group2 are not disjoint in time.               */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupOverlap(KHE_GROUP group1, KHE_GROUP group2)
{
  return !KheGroupDisjoint(group1, group2);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceShow(KHE_RESOURCE r)                                    */
/*                                                                           */
/*  Return the Id of r, or "@" if r is NULL.                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheResourceShow(KHE_RESOURCE r)
{
  if( r == NULL )
    return "@";
  else if( KheResourceId(r) == NULL )
    return "?";
  else
    return KheResourceId(r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupDebug(KHE_GROUP group, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of group onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheGroupDebug(KHE_GROUP group, int verbosity, int indent, FILE *fp)
{
  RESOURCE_INFO ri, rj;  int i, j, ri_index, rj_index;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( DEBUG4 )
    fprintf(fp, "%p", (void *) group);
  fprintf(fp, "(%02d-%02d)%s", group->interval.first, group->interval.last,
    group->unassignable ? "unassignable" : "");
  KheTaskSetDebug(group->task_set, verbosity, -1, fp);
  if( verbosity >= 2 && HaArrayCount(group->domain) > 0 )
  {
    fprintf(fp, "[");
    for( i = 0;  i < HaArrayCount(group->domain);  i = j )
    {
      /* find consecutive resources from i to j-1 */
      ri = HaArray(group->domain, i);
      if( ri->resource == NULL )
	ri_index = INT_MAX;
      else
	ri_index = KheResourceResourceTypeIndex(ri->resource);
      for( j = i + 1;  j < HaArrayCount(group->domain);  j++ )
      {
	rj = HaArray(group->domain, j);
	if( rj->resource == NULL )
	  rj_index = INT_MAX;
	else
	  rj_index = KheResourceResourceTypeIndex(rj->resource);
	if( rj_index - ri_index != j - i )
	  break;
      }

      /* print sequence from i to j-1 */
      if( i > 0 )
	fprintf(fp, ", ");
      if( j == i + 1 )
	fprintf(fp, "%s", KheResourceShow(ri->resource));
      else
      {
	rj = HaArray(group->domain, j-1);
	fprintf(fp, "%s .. %s", KheResourceShow(ri->resource),
	  KheResourceShow(rj->resource));
      }
    }
    fprintf(fp, "]");
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource info"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  RESOURCE_INFO ResourceInfoMake(KHE_REASSIGN_SOLVER rs, KHE_RESOURCE r)   */
/*                                                                           */
/*  Make a resource info object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static RESOURCE_INFO ResourceInfoMake(KHE_REASSIGN_SOLVER rs, KHE_RESOURCE r)
{
  RESOURCE_INFO res;
  HaMake(res, rs->arena);
  res->resource = r;
  res->group_attributes = ReassignAttributesMake(KheIntervalMake(-1, -1),
    KHE_REASSIGN_MINIMAL, false);
  HaArrayInit(res->groups, rs->arena);
  res->defective = RESOURCE_DEFECTIVE_UNKNOWN;
  res->curr_last_index = -1;
  res->mmatch_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceInfoSetGroupsOutOfDate(RESOURCE_INFO ri)                    */
/*                                                                           */
/*  Mark ri's groups as being out of date.                                   */
/*                                                                           */
/*****************************************************************************/

static void ResourceInfoSetGroupsOutOfDate(RESOURCE_INFO ri)
{
  ri->group_attributes = ReassignAttributesMake(KheIntervalMake(-1, -1),
    KHE_REASSIGN_MINIMAL, false);
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceInfoEnsureGroupsUpToDate(RESOURCE_INFO ri,                  */
/*    KHE_REASSIGN_SOLVER rs, REASSIGN_ATTRIBUTES ra, int indent)            */
/*                                                                           */
/*  Ensure that ri's groups are up to date for solving with attributes ra.   */
/*  If the old attributes are equal to the new ones, it is assumed that ri   */
/*  is already up to date.                                                   */
/*                                                                           */
/*****************************************************************************/
static void ResourceInfoDebug(RESOURCE_INFO ri, KHE_REASSIGN_SOLVER rs,
  int verbosity, int indent, FILE *fp);

static void ResourceInfoEnsureGroupsUpToDate(RESOURCE_INFO ri,
  KHE_REASSIGN_SOLVER rs, REASSIGN_ATTRIBUTES ra, int indent)
{
  int i;  KHE_GROUP group, tmp_group;  KHE_TASK task;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ ResourceInfoEnsureGroupsUpToDate(ri, rs, %s) %s\n",
      indent, "", ReassignAttributesShow(ra, rs),
      ReassignAttributesEqual(ri->group_attributes, ra) ?
      "unchanged" : "changed");

  if( !ReassignAttributesEqual(ri->group_attributes, ra) )
  {
    /* reset ri's attributes */
    ri->group_attributes = ra;

    /* clear out any old groups */
    HaArrayForEach(ri->groups, group, i)
      KheGroupDelete(group, rs);
    HaArrayClear(ri->groups);

    /* add the new groups */
    switch( ra.grouping )
    {
      case KHE_REASSIGN_MINIMAL:

	/* one group for each task in the interval */
	tmp_group = KheGroupMake(rs);
	KheFindTasksInInterval(rs->task_finder, ra.interval,
	  rs->resource_type, ri->resource, false,
	  ra.allow_partial, tmp_group->task_set, &tmp_group->interval);
	for( i = 0;  i < KheTaskSetTaskCount(tmp_group->task_set);  i++ )
	{
	  task = KheTaskSetTask(tmp_group->task_set, i);
	  group = KheGroupMake(rs);
	  KheTaskSetAddTask(group->task_set, task);
	  group->interval = KheTaskFinderTaskInterval(rs->task_finder, task);
	  group->unassignable = ri->resource != NULL &&
	    !KheTaskSetNeedsAssignment(group->task_set);
	  HaArrayAddLast(ri->groups, group);
	}
	KheGroupDelete(tmp_group, rs);
	break;

      case KHE_REASSIGN_RUNS:

	/* one group for each run of tasks overlapping the interval */
	group = KheGroupMake(rs);
	while( KheFindFirstRunInInterval(rs->task_finder, ra.interval,
	  rs->resource_type, ri->resource, false, ra.allow_partial,
	  true, group->task_set, &group->interval) )
	{
	  HnAssert(group->interval.first >= 0,
	    "ResourceInfoEnsureGroupsUpToDate internal error 1");
	  HnAssert(group->interval.last >= 0,
	    "ResourceInfoEnsureGroupsUpToDate internal error 2");
	  group->unassignable = ri->resource != NULL &&
	    !KheTaskSetNeedsAssignment(group->task_set);
	  HaArrayAddLast(ri->groups, group);
	  ra.interval.first = group->interval.last + 1;
	  group = KheGroupMake(rs);
	}
	KheGroupDelete(group, rs);
	break;

      case KHE_REASSIGN_MAXIMAL:

	/* a single group containing all the tasks in the interval */
	group = KheGroupMake(rs);
	KheFindTasksInInterval(rs->task_finder, ra.interval,
	  rs->resource_type, ri->resource, false, ra.allow_partial,
	  group->task_set, &group->interval);
	group->unassignable = ri->resource != NULL &&
	  !KheTaskSetNeedsAssignment(group->task_set);
	HaArrayAddLast(ri->groups, group);
	break;

      default:

        HnAbort("ResourceInfoEnsureGroupsUpToDate internal error");
    }
  }
  if( DEBUG3 )
  {
    ResourceInfoDebug(ri, rs, 2, indent + 2, stderr);
    fprintf(stderr, "%*s] ResourceInfoEnsureGroupsUpToDate returning\n",
      indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceInfoSetDefectiveOutOfDate(RESOURCE_INFO ri)                 */
/*                                                                           */
/*  Mark ri's defectiveness as unknown.                                      */
/*                                                                           */
/*****************************************************************************/

static void ResourceInfoSetDefectiveOutOfDate(RESOURCE_INFO ri)
{
  ri->defective = RESOURCE_DEFECTIVE_UNKNOWN;
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceInfoEnsureDefectiveUpToDate(RESOURCE_INFO ri,               */
/*    KHE_REASSIGN_SOLVER rs)                                                */
/*                                                                           */
/*  Ensure ri's defective field is up to date.                               */
/*                                                                           */
/*****************************************************************************/

static void ResourceInfoEnsureDefectiveUpToDate(RESOURCE_INFO ri,
  KHE_REASSIGN_SOLVER rs)
{
  if( ri->defective == RESOURCE_DEFECTIVE_UNKNOWN )
  {
    if( ri->resource == NULL )
      ri->defective = RESOURCE_DEFECTIVE_NO;
    else if( KheSolnResourceCost(rs->soln, ri->resource) > 0 )
      ri->defective = RESOURCE_DEFECTIVE_YES;
    else
      ri->defective = RESOURCE_DEFECTIVE_NO;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceInfoDebug(RESOURCE_INFO ri, KHE_REASSIGN_SOLVER rs,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ri onto fp with the given verbosity and indent.           */
/*  The groups are only printed if indent >= 0.                              */
/*                                                                           */
/*****************************************************************************/

static void ResourceInfoDebug(RESOURCE_INFO ri, KHE_REASSIGN_SOLVER rs,
  int verbosity, int indent, FILE *fp)
{
  KHE_GROUP group;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( ri->group_attributes.interval.first >= 0 )
  {
    fprintf(fp, "[%s defective=%s %s:", KheResourceShow(ri->resource),
      ResourceDefectiveShow(ri->defective),
      ReassignAttributesShow(ri->group_attributes, rs));
    if( indent >= 0 )
    {
      fprintf(fp, "\n");
      HaArrayForEach(ri->groups, group, i)
	KheGroupDebug(group, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]", indent, "");
    }
    else
      fprintf(fp, " %d groups]", HaArrayCount(ri->groups));
  }
  else
    fprintf(fp, "[%s not up to date]", KheResourceShow(ri->resource));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheReassignSolverSolve"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnIsNewBest(KHE_REASSIGN_SOLVER rs)                            */
/*                                                                           */
/*  Return true if the current solution is a new best.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheSolnIsNewBest(KHE_REASSIGN_SOLVER rs)
{
  if( KheMarkPathCount(rs->init_mark) == 0 )
    return true;
  return KheSolnCost(rs->soln) < KhePathSolnCost(KheMarkPath(rs->init_mark,0))
    && (!rs->resource_invariant ||
      KheSolnMatchingDefectCount(rs->soln) <= rs->init_defect_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverContinueSolve(KHE_REASSIGN_SOLVER rs,              */
/*    int curr_index)                                                        */
/*                                                                           */
/*  Solve from curr_index onwards.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverContinueSolve(KHE_REASSIGN_SOLVER rs,
  int curr_index)
{
  KHE_GROUP group;  RESOURCE_INFO ri;  int i, save_curr_last_index;
  KHE_MARK mark;
  if( DEBUG5 && rs->debug_on )
    fprintf(stderr, "%*s[ ContinueSolve(rs, %d)\n", curr_index * 2, "",
      curr_index);
  if( rs->curr_assignments < rs->max_assignments )
  {
    if( curr_index >= HaArrayCount(rs->movable_groups) )
    {
      /* time to stop; if new best, save path */
      if( DEBUG5 && rs->debug_on )
	fprintf(stderr, "%*s  off end: %s\n", curr_index * 2, "",
	   KheSolnIsNewBest(rs) ? "new best" : "no luck");
      if( KheSolnIsNewBest(rs) )
	KheMarkAddBestPath(rs->init_mark, 1);
      rs->curr_assignments++;
    }
    else
    {
      /* try each possible assignment of the group at curr_index */
      group = HaArray(rs->movable_groups, curr_index);
      if( DEBUG5 && rs->debug_on )
      {
	fprintf(stderr, "%*s  [ assigning ", curr_index * 2, "");
	KheGroupDebug(group, 1, -1, stderr);
	fprintf(stderr, ":\n");
      }
      HaArrayForEach(group->domain, ri, i)
	if( ri->curr_last_index < group->interval.first )
	{
	  /* ri->resource is in the domain and assignment would not clash */
	  mark = KheMarkBegin(rs->soln);
	  if( KheGroupMove(group, ri->resource) )
	  {
	    save_curr_last_index = ri->curr_last_index;
            ri->curr_last_index = group->interval.last;
            KheReassignSolverContinueSolve(rs, curr_index + 1);
            ri->curr_last_index = save_curr_last_index;
	  }
	  KheMarkEnd(mark, true);
	}
      if( group->unassignable )
      {
	  /* try unassignment */
	  mark = KheMarkBegin(rs->soln);
	  if( KheGroupMove(group, NULL) )
            KheReassignSolverContinueSolve(rs, curr_index + 1);
	  KheMarkEnd(mark, true);
      }
      if( DEBUG5 && rs->debug_on )
	fprintf(stderr, "%*s  ]\n", curr_index * 2, "");
    }
  }
  else
  {
    if( DEBUG5 && rs->debug_on )
      fprintf(stderr, "%*s  limit %d reached\n", curr_index * 2, "",
	rs->max_assignments);
  }
  if( DEBUG5 && rs->debug_on )
    fprintf(stderr, "%*s]\n", curr_index * 2, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverPrepareGroups(KHE_REASSIGN_SOLVER rs,              */
/*    REASSIGN_ATTRIBUTES ra, int indent)                                    */
/*                                                                           */
/*  Prepare the groups for solving.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverPrepareGroups(KHE_REASSIGN_SOLVER rs,
  REASSIGN_ATTRIBUTES ra, int indent)
{
  int i, j, k, pos;
  RESOURCE_INFO ri, ri2, imm_group_ri;  KHE_GROUP group, imm_group;

  /* ensure that the resource info is up to date */
  HaArrayForEach(rs->resources, ri, i)
    ResourceInfoEnsureGroupsUpToDate(ri, rs, ra, indent + 2);

  /* set the groups' domains; set rs->movable_groups and rs->immovable_groups */
  if( DEBUG9 )
    fprintf(stderr, "  setting domains, movable groups, immovable groups\n");
  HaArrayClear(rs->movable_groups);
  HaArrayClear(rs->immovable_groups);
  HaArrayForEach(rs->resources, ri2, i)
    HaArrayForEach(ri2->groups, group, j)
    {
      HaArrayClear(group->domain);
      HaArrayForEach(rs->resources, ri, k)
	if( KheGroupMoveCheck(group, ri->resource) )
	  HaArrayAddLast(group->domain, ri);
      if( HaArrayCount(group->domain) <= 1 )
      {
	/* immovable group */
	HnAssert(HaArrayCount(group->domain) == 1 &&
	  HaArrayFirst(group->domain) == ri2,
	  "KheReassignSolverSolve internal error 1");
	HaArrayAddLast(rs->immovable_groups, group);
      }
      else
      {
	/* movable group */
	HaArrayAddLast(rs->movable_groups, group);
      }
    }

  /* sort the movable and immovable groups */
  if( DEBUG9 )
    fprintf(stderr, "  sorting groups\n");
  HaArraySort(rs->movable_groups, &KheGroupCmp);
  HaArraySort(rs->immovable_groups, &KheGroupCmp);

  /* reduce the domains of movable groups that overlap immovable groups */
  if( DEBUG9 )
    fprintf(stderr, "  reducing domains\n");
  i = 0;
  HaArrayForEach(rs->immovable_groups, imm_group, j)
  {
    /************************************************************************/
    /*                                                                      */
    /*  invariant: movable[0 .. i-1] strictly precede immovable[j-1..$]     */
    /*                                                                      */
    /************************************************************************/
    if( i > HaArrayCount(rs->movable_groups) )
      break;
    group = HaArray(rs->movable_groups, i);
    if( group->interval.last < imm_group->interval.first )
      i++;
    else
    {
      imm_group_ri = HaArrayFirst(imm_group->domain);
      for( k = i;  k < HaArrayCount(rs->movable_groups);  k++ )
      {
	group = HaArray(rs->movable_groups, k);
	if( group->interval.first >
	    imm_group_ri->group_attributes.interval.last )
	  break;
	if( KheGroupOverlap(imm_group, group) )
	{
	  if( HaArrayContains(group->domain, imm_group_ri, &pos) )
	    HaArrayDeleteAndPlug(group->domain, pos);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Debug functions for matching.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverBackDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_REASSIGN_SOLVER rs;
  rs = (KHE_REASSIGN_SOLVER) value;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(stderr, "ReassignSolver(%s of %s)",
    KheResourceTypeId(rs->resource_type),
    KheInstanceId(KheSolnInstance(rs->soln)));
  if( indent >= 0 )
    fprintf(fp, "\n");
}

static void KheReassignSolverDemandBackDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  KheGroupDebug((KHE_GROUP) value, verbosity, indent, fp);
}

static void KheReassignSolverSupplyBackDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  ResourceInfoDebug((RESOURCE_INFO) value, NULL, verbosity, indent, fp);
}

static void KheReassignSolverCostDebug(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%.5f", KheCostShow(KheCost(c1, c2)));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolverDoSolve(KHE_REASSIGN_SOLVER rs,                    */
/*    REASSIGN_ATTRIBUTES ra, int indent)                                    */
/*                                                                           */
/*  Carry out KheReassignSolverSolve, with an indent parameter for use       */
/*  during debugging.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignSolverDoSolve(KHE_REASSIGN_SOLVER rs,
  REASSIGN_ATTRIBUTES ra, int indent)
{
  int i, j;  KHE_COST init_cost, cost;  bool res;  RESOURCE_INFO ri;
  int mult, c1, c2, c3;  KHE_MMATCH_NODE sn;  KHE_GROUP group;
  if( DEBUG2 )
  {
    fprintf(stderr, "%*s[ KheReassignSolverDoSolve(rs, %s) [",
      indent, "", ReassignAttributesShow(ra, rs));
    HaArrayForEach(rs->resources, ri, i)
      fprintf(stderr, "%s%s", i > 0 ? ", " : "", KheResourceShow(ri->resource));
    fprintf(stderr, "]\n");
  }

  /* prepare the groups */
  KheReassignSolverPrepareGroups(rs, ra, indent);

  /* debug print of resources ready for solve */
  if( DEBUG2 )
    HaArrayForEach(rs->resources, ri, i)
      ResourceInfoDebug(ri, rs, 2, indent + 2, stderr);

  /* set up for and perform the actual solve */
  if( DEBUG9 )
    fprintf(stderr, "  starting actual solve\n");
  init_cost = KheSolnCost(rs->soln);
  switch( rs->method )
  {
    case KHE_REASSIGN_EXHAUSTIVE:

      HaArrayForEach(rs->resources, ri, i)
	ri->curr_last_index = -1;
      rs->init_mark = KheMarkBegin(rs->soln);
      if( rs->resource_invariant )
	rs->init_defect_count = KheSolnMatchingDefectCount(rs->soln);
      else
	rs->init_defect_count = -1;
      rs->debug_on = false;
      rs->curr_assignments = 0;
      KheReassignSolverContinueSolve(rs, 0);

      if( KheMarkPathCount(rs->init_mark) > 0 &&
	  KhePathSolnCost(KheMarkPath(rs->init_mark, 0)) < init_cost )
      {
	/* success; redo best path and mark the resources as out of date */
	if( DEBUG9 )
	  fprintf(stderr, "  wrapup (success)\n");
	HnAssert(KheMarkPathCount(rs->init_mark) == 1,
	  "KheReassignSolverSolve internal error 1");
	KhePathRedo(KheMarkPath(rs->init_mark, 0));
	KheMarkEnd(rs->init_mark, false);
	res = true;
      }
      else
      {
	KheMarkEnd(rs->init_mark, true);
	res = false;
      }
      break;

    case KHE_REASSIGN_MATCHING:

      /* initialize the match object */
      if( rs->mmatch == NULL )
	rs->mmatch = KheMMatchMake((void *) rs, &KheReassignSolverBackDebug,
	  &KheReassignSolverDemandBackDebug, &KheReassignSolverSupplyBackDebug,
	  &KheReassignSolverCostDebug, rs->arena);
      KheMMatchClear(rs->mmatch);

      /* set one demand node per group; also unassign the groups */
      HaArrayForEach(rs->movable_groups, group, i)
      {
	group->mmatch_node = KheMMatchDemandNodeMake(rs->mmatch, 1,
	  (void *) group);
        if( !KheGroupMove(group, NULL) )
	  HnAbort("KheReassignSolverSolve internal error 2");
      }

      /* set one supply node per resource */
      HaArrayForEach(rs->resources, ri, j)
	ri->mmatch_node = KheMMatchSupplyNodeMake(rs->mmatch, 1, (void *) ri);

      /* add the edges */
      HaArrayForEach(rs->movable_groups, group, i)
	HaArrayForEach(group->domain, ri, j)
	  if( KheGroupMove(group, ri->resource) )
	  {
	    cost = KheSolnCost(rs->soln);
	    KheMMatchAddEdge(group->mmatch_node, ri->mmatch_node, 1,
	      KheHardCost(cost), KheSoftCost(cost), 0);
	    KheGroupMove(group, NULL);
	  }

      /* solve and accept result */
      KheMMatchSolve(rs->mmatch);
      if( DEBUG11 )
	KheMMatchDebug(rs->mmatch, 2, indent + 2, stderr);
      HaArrayForEach(rs->movable_groups, group, i)
      {
	KheMMatchResultEdge(group->mmatch_node, &sn, &mult, &c1, &c2, &c3);
	ri = (RESOURCE_INFO) KheMMatchSupplyNodeBack(sn);
	if( DEBUG11 )
	{
	  fprintf(stderr, "%*sresult edge from ", indent + 2, "");
	  KheGroupDebug(group, 1, -1, stderr);
	  fprintf(stderr, " to ");
	  ResourceInfoDebug(ri, rs, 1, -1, stderr);
	  fprintf(stderr, "\n");
	}
        if( !KheGroupMove(group, ri->resource) )
	  HnAbort("KheReassignSolverSolve internal error 3");
      }

      /* success if cost has reduced */
      if( DEBUG10 )
	fprintf(stderr, "%*smatching result is %s (%.5f -> %.5f)\n",
	  indent + 2, "", KheSolnCost(rs->soln) < init_cost ? "better" :
	  KheSolnCost(rs->soln) == init_cost ? "the same" : "worse",
	  KheCostShow(init_cost), KheCostShow(KheSolnCost(rs->soln)));
      res = (KheSolnCost(rs->soln) < init_cost);
      break;

    default:

      HnAbort("KheReassignSolverDoSolve internal error");
      res = false;  /* keep compiler happy */
      break;
  }

  /* wrapup depending on res */
  if( res )
  {
    /* success */
    HaArrayForEach(rs->resources, ri, i)
    {
      ResourceInfoSetGroupsOutOfDate(ri);
      ResourceInfoSetDefectiveOutOfDate(ri);
    }
    if( DEBUG2 )
      fprintf(stderr, "%*s] KheReassignSolverDoSolve returning true "
	"(%.5f -> %.5f)\n", indent, "", KheCostShow(init_cost),
	KheCostShow(KheSolnCost(rs->soln)));
    if( DEBUG6 )
    {
      fprintf(stderr, "  KheReassignSolverSolve(rs, %s) [",
	ReassignAttributesShow(ra, rs));
      HaArrayForEach(rs->resources, ri, i)
	fprintf(stderr, "%s%s", i > 0 ? ", " : "",
	  KheResourceShow(ri->resource));
      fprintf(stderr, "] returning true (%.5f -> %.5f)\n",
	KheCostShow(init_cost), KheCostShow(KheSolnCost(rs->soln)));
    }
    return true;
  }
  else
  {
    /* failure */
    if( rs->method == KHE_REASSIGN_MATCHING )
      HaArrayForEach(rs->resources, ri, i)
      {
	ResourceInfoSetGroupsOutOfDate(ri);
	ResourceInfoSetDefectiveOutOfDate(ri);
      }
    if( DEBUG9 )
      fprintf(stderr, "  wrapup (failure)\n");
    if( DEBUG2 )
      fprintf(stderr, "%*s] KheReassignSolverDoSolve returning false\n",
	indent, "");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolverSolve(KHE_REASSIGN_SOLVER rs, int first_index,     */
/*    int last_index, KHE_REASSIGN_GROUPING grouping, bool allow_partial,    */
/*    KHE_REASSIGN_METHOD method, int max_assignments)                       */
/*                                                                           */
/*  Solve, and return true if any assignments changed.  At the start, make   */
/*  sure the resources are up to date, and at the end, mark them out of      */
/*  date if any assignments changed.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheReassignSolverSolve(KHE_REASSIGN_SOLVER rs, int first_index,
  int last_index, KHE_REASSIGN_GROUPING grouping, bool allow_partial,
  KHE_REASSIGN_METHOD method, int max_assignments)
{
  bool res;  REASSIGN_ATTRIBUTES ra;
  if( DEBUG2 )
    fprintf(stderr, "[ KheReassignSolverSolve(rs, %d, %d, %s, %s)\n",
      first_index, last_index, ReassignGroupingShow(grouping),
      bool_show(allow_partial));
  ra = ReassignAttributesMake(KheIntervalMake(first_index, last_index),
    grouping, allow_partial);
  rs->method = method;
  rs->max_assignments = max_assignments;
  res = KheReassignSolverDoSolve(rs, ra, 2);
  if( DEBUG2 )
    fprintf(stderr, "] KheReassignSolverSolve returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_REASSIGN_SOLVER KheReassignSolverMake(KHE_SOLN soln,                 */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)                             */
/*                                                                           */
/*  Make a new reassign solver with these attributes, using memory from an   */
/*  arena taken from soln.                                                   */
/*                                                                           */
/*****************************************************************************/

KHE_REASSIGN_SOLVER KheReassignSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  KHE_REASSIGN_SOLVER res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);

  /* used throughout the lifetime of the solver */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->task_finder = KheTaskFinderMake(soln, options, a);
  res->resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  HaArrayInit(res->dormant_resources, a);
  HaArrayInit(res->resources, a);
  HaArrayInit(res->free_groups, a);
  res->mmatch = NULL;

  /* used by KheReassignRepair */
  HaArrayInit(res->select_constraints, a);
  HaArrayInit(res->violating_resources, a);
  HaArrayInit(res->slack_resources, a);
  HaArrayInit(res->intervals, a);

  /* used by individual solves (KheReassignSolverDoSolve) */
  HaArrayInit(res->movable_groups, a);
  HaArrayInit(res->immovable_groups, a);
  res->method = KHE_REASSIGN_EXHAUSTIVE;
  res->max_assignments = -1;
  res->init_mark = NULL;
  res->init_defect_count = -1;
  res->debug_on = false;
  res->curr_assignments = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverDelete(KHE_REASSIGN_SOLVER rs)                     */
/*                                                                           */
/*  Delete rs.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheReassignSolverDelete(KHE_REASSIGN_SOLVER rs)
{
  KheSolnArenaEnd(rs->soln, rs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverContainsResource(KHE_REASSIGN_SOLVER rs,                   */
/*    KHE_RESOURCE r, int *pos)                                              */
/*                                                                           */
/*  If rs contains r, return true with *pos set to its position, else        */
/*  return false.  Here r may be NULL.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheSolverContainsResource(KHE_REASSIGN_SOLVER rs,
  KHE_RESOURCE r, int *pos)
{
  RESOURCE_INFO ri;  int i;
  for( i = HaArrayCount(rs->resources) - 1;  i >= 0;  i-- )
  {
    ri = HaArray(rs->resources, i);
    if( ri->resource == r )
      return *pos = i, true;
  }
  return *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolverAddResource(KHE_REASSIGN_SOLVER rs, KHE_RESOURCE r)*/
/*                                                                           */
/*  Add r to rs and return true, or do nothing and return false if r is      */
/*  already present.  Here r may be NULL.                                    */
/*                                                                           */
/*  It is an invariant that every resource in rs->resources has an           */
/*  up-to-date defectiveness, so if this function adds a resource it also    */
/*  brings its defectiveness up to date.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheReassignSolverAddResource(KHE_REASSIGN_SOLVER rs, KHE_RESOURCE r)
{
  int pos, index;  RESOURCE_INFO ri;

  /* if already present, do nothing and return false */
  if( KheSolverContainsResource(rs, r, &pos) )
    return false;

  /* get ri from dormant resources if present, else make it */
  if( r == NULL )
    index = KheResourceTypeResourceCount(rs->resource_type);
  else
  {
    HnAssert(KheResourceResourceType(r) == rs->resource_type,
      "KheReassignSolverAddResource: r has wrong resource type for rs");
    index = KheResourceResourceTypeIndex(r);
  }
  HaArrayFill(rs->dormant_resources, index + 1, NULL);
  ri = HaArray(rs->dormant_resources, index);
  if( ri == NULL )
  {
    ri = ResourceInfoMake(rs, r);
    HaArrayPut(rs->dormant_resources, index, ri);
  }

  /* ensure ri's defectiveness is up to date */
  ResourceInfoEnsureDefectiveUpToDate(ri, rs);

  /* add ri to resources and return true */
  HaArrayAddLast(rs->resources, ri);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolverDeleteResource(KHE_REASSIGN_SOLVER rs,             */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Delete r from rs and return true, or do nothing and return false if r    */
/*  not present.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheReassignSolverDeleteResource(KHE_REASSIGN_SOLVER rs, KHE_RESOURCE r)
{
  int pos;

  /* if not already present, do nothing and return false */
  HnAssert(r == NULL || KheResourceResourceType(r) == rs->resource_type,
    "KheReassignSolverDeleteResource: r has wrong resource type for rs");
  if( !KheSolverContainsResource(rs, r, &pos) )
    return false;

  /* delete from resources array (order does not matter) and return true */
  HaArrayDeleteAndPlug(rs->resources, pos);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverClearResources(KHE_REASSIGN_SOLVER rs)             */
/*                                                                           */
/*  Clear all the resources from rs.                                         */
/*                                                                           */
/*****************************************************************************/

void KheReassignSolverClearResources(KHE_REASSIGN_SOLVER rs)
{
  HaArrayClear(rs->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolveDefective(KHE_REASSIGN_SOLVER rs,                   */
/*    int first_avail, int resource_count, int min_defective,                */
/*    REASSIGN_ATTRIBUTES ra, bool reassign_null, int indent)                */
/*                                                                           */
/*  Solve for all sets of resources that satisfy these conditions:           */
/*                                                                           */
/*    * The resources are all those currently in the solver plus             */
/*      resource_count extra resources.                                      */
/*                                                                           */
/*    * The extra resources all come from rt[first_avail ..].                */
/*                                                                           */
/*    * There must be at least min_defective defective resources among       */
/*      the extra resources.                                                 */
/*                                                                           */
/*  Return true if the solution is improved.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignSolveDefective(KHE_REASSIGN_SOLVER rs,
  int first_avail, int resource_count, int min_defective,
  REASSIGN_ATTRIBUTES ra, bool reassign_null, int indent)
{
  KHE_RESOURCE r;  RESOURCE_INFO ri;  bool res;  int i, count;
  if( DEBUG2 )
    fprintf(stderr, "%*s[ SolveDefective(rs, fa %d, rc %d, md %d, %s)\n",
      indent, "", first_avail, resource_count, min_defective,
      ReassignAttributesShow(ra, rs));

  if( min_defective > resource_count )
  {
    /* no prospect of finding enough defective extra resources */
    res = false;
  }
  else if( resource_count == 0 )
  {
    /* non-NULL resources are all loaded; add NULL resource and solve */
    if( reassign_null )
      KheReassignSolverAddResource(rs, NULL);
    res = KheReassignSolverDoSolve(rs, ra, indent + 2);
    if( reassign_null )
      KheReassignSolverDeleteResource(rs, NULL);
  }
  else
  {
    /* choose one resource from first_avail or later, then recurse;  if */
    /* i > count - resource_count, not enough resources to choose from  */
    count = KheResourceTypeResourceCount(rs->resource_type);
    res = false;
    for( i = first_avail;  i <= count - resource_count;  i++ )
    {
      r = KheResourceTypeResource(rs->resource_type, i);
      KheReassignSolverAddResource(rs, r);
      ri = HaArrayLast(rs->resources);
      if( KheReassignSolveDefective(rs, i + 1, resource_count - 1,
	  min_defective - (ri->defective == RESOURCE_DEFECTIVE_YES ? 1 : 0),
	  ra, reassign_null, indent + 2) )
	res = true;
      KheReassignSolverDeleteResource(rs, r);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "%*s] SolveDefective returning %s\n", indent, "",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolveAdjacent(KHE_REASSIGN_SOLVER rs,                    */
/*    REASSIGN_ATTRIBUTES ra, int reassign_resources, bool reassign_null,    */
/*    int indent)                                                            */
/*                                                                           */
/*  Solve adjacent sets of resources and return true if any succeeded.       */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignSolveAdjacent(KHE_REASSIGN_SOLVER rs,
  REASSIGN_ATTRIBUTES ra, int reassign_resources, bool reassign_null,
  int indent)
{
  int i, j, last_start;  bool res;  KHE_RESOURCE r;
  if( DEBUG2 )
    fprintf(stderr, "%*s[ SolveAdjacent(rs, %s)\n", indent, "",
      ReassignAttributesShow(ra, rs));
  res = false;
  last_start = KheResourceTypeResourceCount(rs->resource_type) -
    reassign_resources;
  for( i = 0;  i <= last_start;  i++ )
  {
    /* add rs->reassign_resources resources to rs */
    for( j = 0;  j < reassign_resources;  j++ )
    {
      r = KheResourceTypeResource(rs->resource_type, i + j);
      KheReassignSolverAddResource(rs, r);
    }

    /* add NULL resource if required, and solve */
    if( reassign_null )
      KheReassignSolverAddResource(rs, NULL);
    if( KheReassignSolverDoSolve(rs, ra, indent + 2) )
      res = true;

    /* clear the resources */
    KheReassignSolverClearResources(rs);
  }
  if( DEBUG2 )
    fprintf(stderr, "%*s] SolveAdjacent returning %s\n", indent, "",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool DoConstraint(KHE_REASSIGN_SOLVER rs, int slack_count,               */
/*    int slack_first_index, REASSIGN_ATTRIBUTES ra, int indent)             */
/*                                                                           */
/*  Add slack_count slack resources in all possible ways and solve.          */
/*                                                                           */
/*****************************************************************************/

static bool DoConstraint(KHE_REASSIGN_SOLVER rs, int slack_count,
  int slack_first_index, REASSIGN_ATTRIBUTES ra, int indent)
{
  int i;  KHE_RESOURCE r;  bool res;
  if( slack_count <= 0 )
  {
    /* solve */
    res = KheReassignSolverDoSolve(rs, ra, indent);
  }
  else
  {
    /* add one slack resource and recurse */
    res = false;
    for( i = slack_first_index;  i < HaArrayCount(rs->slack_resources);  i++ )
    {
      r = HaArray(rs->slack_resources, i);
      KheReassignSolverAddResource(rs, r);
      if( DoConstraint(rs, slack_count - 1, i + 1, ra, indent) )
	res = true;
      KheReassignSolverDeleteResource(rs, r);
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolveConstraint(KHE_REASSIGN_SOLVER rs,                  */
/*    REASSIGN_ATTRIBUTES ra, int reassign_resources, bool reassign_null,    */
/*    int indent)                                                            */
/*                                                                           */
/*  Solve for resources which violate rs->constraints.                       */
/*                                                                           */
/*  Implementation note 1.  Array rs->constraints is set by this point.      */
/*                                                                           */
/*  Implementation note 2.  Could save some time by keeping an array of      */
/*  all relevant monitors in r's resource info object.  But getting it       */
/*  right would be a bit convoluted, so at present we are not bothering.     */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignSolveConstraint(KHE_REASSIGN_SOLVER rs,
  REASSIGN_ATTRIBUTES ra, int reassign_resources, bool reassign_null,
  int indent)
{
  int i, j, k, pos;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_RESOURCE r;  KHE_MONITOR m;  bool res;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;

  if( DEBUG8 )
    fprintf(stderr,"%*s[ SolveConstraint(soln, %s)\n", indent, "",
      ReassignAttributesShow(ra, rs));

  /* build arrays of violating and slack resources of type rt */
  HaArrayClear(rs->violating_resources);
  HaArrayClear(rs->slack_resources);
  for( j = 0;  j < KheResourceTypeResourceCount(rs->resource_type);  j++ )
  {
    r = KheResourceTypeResource(rs->resource_type, j);
    for( k = 0;  k < KheSolnResourceMonitorCount(rs->soln, r);  k++ )
    {
      m = KheSolnResourceMonitor(rs->soln, r, k);
      if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
      {
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	if( HaArrayContains(rs->select_constraints, cbtc, &pos) )
	{
	  if( KheMonitorCost(m) > 0 )
	  {
	    HaArrayAddLast(rs->violating_resources, r);
	    if( DEBUG8 )
	      fprintf(stderr, "  violating resource %s\n", KheResourceId(r));
	    break;  /* next resource */
	  }
	  else if( !KheClusterBusyTimesMonitorAtMaxLimitCount(cbtm) )
	  {
	    HaArrayAddLast(rs->slack_resources, r);
	    if( DEBUG8 )
	      fprintf(stderr, "  slack resource %s\n", KheResourceId(r));
	    break;  /* next resource */
	  }
	}
      }
    }
  }

  /* solve for each violating resource and pair (etc.) of slack resources */
  res = false;
  if( reassign_null )
    KheReassignSolverAddResource(rs, NULL);
  HaArrayForEach(rs->violating_resources, r, i)
  {
    KheReassignSolverAddResource(rs, r);
    if( DoConstraint(rs, reassign_resources - 1, 0, ra, indent + 2) )
      res = true;
    KheReassignSolverDeleteResource(rs, r);
  }
  if( reassign_null )
    KheReassignSolverDeleteResource(rs, NULL);
  if( DEBUG8 )
    fprintf(stderr, "%*s] SolveConstraint ret. %s\n", indent, "",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignSolveForResources(KHE_REASSIGN_SOLVER rs,                */
/*    int reassign_resources, REASSIGN_SELECT_TYPE reassign_select,          */
/*    bool reassign_null, KHE_REASSIGN_GROUPING reassign_grouping,           */
/*    bool reassign_allow_partial, int indent)                               */
/*                                                                           */
/*  Solve with these parameters.  Return true if any solves succeeded.       */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignSolveForResources(KHE_REASSIGN_SOLVER rs,
  int reassign_resources, REASSIGN_SELECT_TYPE reassign_select,
  bool reassign_null, KHE_REASSIGN_GROUPING reassign_grouping,
  bool reassign_allow_partial, int indent)
{
  bool res;  int i;  REASSIGN_ATTRIBUTES ra;  KHE_INTERVAL in;
  if( DEBUG2 )
    fprintf(stderr, "%*s[ SolveForResources(rs, %d)\n", indent, "",
      reassign_resources);
      /* ***
      ReassignSelectTypeShow(rs->reassign_select, rs->select_constraints_str));
      *** */
  res = false;
  /* ***
  final_index = KheTaskFinderLastIndex(rs->task_finder);
  ra = ReassignAttributesMake(rs->reassign_start,
    rs->reassign_start + rs->reassign_parts - 1, rs->reassign_grouping,
    rs->reassign_allow_partial);
  while( ra.last_index <= final_index )
  *** */
  ra = ReassignAttributesMake(KheIntervalMake(-1, -1), reassign_grouping,
    reassign_allow_partial);
  HaArrayForEach(rs->intervals, in, i)
  {
    ra.interval = in;
    switch( reassign_select )
    {
      case REASSIGN_SELECT_ADJACENT:

	if( KheReassignSolveAdjacent(rs, ra, reassign_resources,
	      reassign_null, indent + 2) )
	  res = true;
	break;

      case REASSIGN_SELECT_DEFECTIVE:

	if( KheReassignSolveDefective(rs, 0, reassign_resources,
	      1, ra, reassign_null, indent + 2) )
	  res = true;
	break;

      case REASSIGN_SELECT_ALL:

	if(KheResourceTypeResourceCount(rs->resource_type)==reassign_resources)
	{
	  if( KheReassignSolveAdjacent(rs, ra, reassign_resources,
		reassign_null, indent + 2) )
	    res = true;
	}
	else
	{
	  if( KheReassignSolveDefective(rs, 0, reassign_resources,
		0, ra, reassign_null, indent + 2) )
	    res = true;
	}
	break;

      case REASSIGN_SELECT_CONSTRAINT:

	if( KheReassignSolveConstraint(rs, ra, reassign_resources,
	      reassign_null, indent + 2) )
	  res = true;
	break;

      case REASSIGN_SELECT_NONE:
      default:

	HnAbort("KheReassignSolveForResources internal error");
	break;
    }
    /* ***
    ra.first_index += rs->reassign_increment;
    ra.last_index += rs->reassign_increment;
    *** */
  }
  if( DEBUG2 )
    fprintf(stderr, "%*s] SolveForResources returning %s\n", indent, "",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverDebug(KHE_REASSIGN_SOLVER rs,                      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheReassignSolverDebug(KHE_REASSIGN_SOLVER rs,
  int verbosity, int indent, FILE *fp)
{
  int i;  RESOURCE_INFO ri;  KHE_INTERVAL in;  KHE_FRAME frame;  KHE_GROUP g;

  /* header line */
  fprintf(fp, "%*s[ ReassignSolver\n", indent, "");

  /* resources */
  if( HaArrayCount(rs->resources) > 0 )
  {
    HaArrayForEach(rs->resources, ri, i)
    {
      if( i == 0 )
	fprintf(fp, "%*s", indent + 2, "");
      else if( i % 8 == 0 )
	fprintf(fp, ",\n%*s", indent + 2, "");
      else
	fprintf(fp, ", ");
      fprintf(fp, "%s", KheResourceShow(ri->resource));
    }
    fprintf(fp, "\n");
  }

  /* intervals */
  frame = KheTaskFinderFrame(rs->task_finder);
  if( HaArrayCount(rs->intervals) > 0 )
  {
    HaArrayForEach(rs->intervals, in, i)
    {
      if( i == 0 )
	fprintf(fp, "%*s", indent + 2, "");
      else if( i % 8 == 0 )
	fprintf(fp, ",\n%*s", indent + 2, "");
      else
	fprintf(fp, ", ");
      fprintf(fp, "%s", KheIntervalShow(in, frame));
    }
    fprintf(fp, "\n");
  }

  /* movable groups */
  HaArrayForEach(rs->movable_groups, g, i)
    KheGroupDebug(g, 2, indent + 2, fp);

  /* wrapup */
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverSetSelectConstraints(KHE_REASSIGN_SOLVER rs)       */
/*                                                                           */
/*  Set rs->select_constraints based on str.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverSetSelectConstraints(KHE_REASSIGN_SOLVER rs,
  char *str)
{
  KHE_INSTANCE ins;  int i;  KHE_CONSTRAINT c;
  ins = KheSolnInstance(rs->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG &&
	strstr(KheConstraintName(c), str) != NULL )
    {
      HaArrayAddLast(rs->select_constraints,
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);
      if( DEBUG8 )
	KheConstraintDebug(c, 2, 2, stderr);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverSetConstraintIntervals(KHE_REASSIGN_SOLVER rs,     */
/*    char *str)                                                             */
/*                                                                           */
/*  Set rs->intervals based on str.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverSetConstraintIntervals(KHE_REASSIGN_SOLVER rs,
  char *str)
{
  KHE_INSTANCE ins;  int i, j, k, count, offset /* , fi, li */;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_TIME_GROUP tg;
  KHE_POLARITY po;  KHE_CONSTRAINT c;  KHE_INTERVAL in;
  HaArrayClear(rs->intervals);
  ins = KheSolnInstance(rs->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG &&
	strstr(KheConstraintName(c), str) != NULL )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( DEBUG8 )
	KheConstraintDebug(c, 2, 2, stderr);
      count = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc);
      for( j = 0;  j < count;  j++ )
      {
	offset = KheClusterBusyTimesConstraintAppliesToOffset(cbtc, j);
	for( k = 0; k < KheClusterBusyTimesConstraintTimeGroupCount(cbtc); k++ )
	{
	  tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, k, offset, &po);
	  in = KheTaskFinderTimeGroupInterval(rs->task_finder, tg);
	  HaArrayAddLast(rs->intervals, in);
	}
      }
    }
  }
  HaArraySortUnique(rs->intervals, &KheIntervalCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheReassignSolverSetPartsIntervals(KHE_REASSIGN_SOLVER rs)          */
/*                                                                           */
/*  Set the intervals of rs based on parts options.                          */
/*                                                                           */
/*****************************************************************************/

static void KheReassignSolverSetPartsIntervals(KHE_REASSIGN_SOLVER rs,
  int parts, int start, int inc)
{
  int fi, li, final;
  final = KheTaskFinderLastIndex(rs->task_finder);
  HaArrayClear(rs->intervals);
  for( fi = start, li = start + parts - 1;  li <= final;  fi += inc, li += inc )
    HaArrayAddLast(rs->intervals, KheIntervalMake(fi, li));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignDoRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,            */
/*    KHE_OPTIONS options, char *fn_name, char *rs_reassign_resources,       */
/*    char *rs_reassign_select, char *rs_reassign_null,                      */
/*    char *rs_reassign_parts, char *rs_reassign_start,                      */
/*    char *rs_reassign_increment, char *rs_reassign_grouping,               */
/*    char *rs_reassign_allow_partial, char *rs_reassign_method,             */
/*    char *rs_reassign_max_assignments)                                     */
/*                                                                           */
/*  Call KheReassignSolverSolve repeatedly on the resources of type rt,      */
/*  depending on options.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheReassignDoRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, char *fn_name, char *rs_reassign_resources,
  char *rs_reassign_select, char *rs_reassign_null,
  char *rs_reassign_parts, char *rs_reassign_start,
  char *rs_reassign_increment, char *rs_reassign_grouping,
  char *rs_reassign_allow_partial, char *rs_reassign_method,
  char *rs_reassign_max_assignments)
{
  KHE_REASSIGN_SOLVER rs;  KHE_INSTANCE ins;
  char *select_constraints_str, *str;
  int reassign_resources, parts, start, inc;
  bool reassign_null, reassign_allow_partial;
  REASSIGN_SELECT_TYPE reassign_select;  KHE_COST init_cost;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_RESOURCE r;
  KHE_REASSIGN_GROUPING reassign_grouping;

  /* get a few options; exit early if not debugging and nothing to do */
  str = KheOptionsGet(options, rs_reassign_resources, "2");
  if( strcmp(str, "all") == 0 )
    reassign_resources = KheResourceTypeResourceCount(rt);
  else
    reassign_resources = KheOptionsGetInt(options, rs_reassign_resources, 2);
  reassign_select = ReassignSelectTypeGet(options, rs_reassign_select,
    &select_constraints_str);
  reassign_null = KheOptionsGetBool(options, rs_reassign_null, false);
  if( !DEBUG1 )
  {
    if( reassign_resources + (reassign_null ? 1 : 0) < 2 ||
	reassign_select == REASSIGN_SELECT_NONE ||
        KheOptionsTimeLimitReached(options) )
      return false;
  }

  /* make a solver and initialize its options */
  rs = KheReassignSolverMake(soln, rt, options);
  /* ***
  rs->reassign_resources = reassign_resources;
  rs->reassign_select = reassign_select;
  rs->reassign_null = reassign_null;
  *** */
  reassign_grouping = ReassignGroupingGet(options, rs_reassign_grouping);
  reassign_allow_partial = KheOptionsGetBool(options,
    rs_reassign_allow_partial, false);
  rs->method = ReassignMethodGet(options, rs_reassign_method);
  rs->max_assignments = KheOptionsGetInt(options,
    rs_reassign_max_assignments, 1000000);

  /* initialize select_constraints, if required */
  if( reassign_select == REASSIGN_SELECT_CONSTRAINT )
    KheReassignSolverSetSelectConstraints(rs, select_constraints_str);

  /* initialize intervals */
  str = KheOptionsGet(options, rs_reassign_parts, "14");
  if( strstr(str, "constraint:") == str )
  {
    /* parts has the form constraint:xxx */
    KheReassignSolverSetConstraintIntervals(rs, &str[strlen("constraint:")]);
  }
  else
  {
    /* parts must be an integer */
    parts = KheOptionsGetInt(options, rs_reassign_parts, 14);
    start = KheOptionsGetInt(options, rs_reassign_start, 0);
    inc = KheOptionsGetInt(options, rs_reassign_increment, parts);
    KheReassignSolverSetPartsIntervals(rs, parts, start, inc);
  }

  /* debug print of call with options */
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
  {
    fprintf(stderr, "[ %s(select %d %s, partial %s)\n", fn_name,
      reassign_resources,
      ReassignSelectTypeShow(reassign_select, select_constraints_str),
      bool_show(reassign_allow_partial));
    KheReassignSolverDebug(rs, 2, 2, stderr);
  }
  /* ***
      fn_name, KheInstanceId(ins), reassign_resources,
      ReassignSelectTypeShow(reassign_select, select_constraints_str),
      rs->reassign_null ? " plus NULL" : "",
      parts, start, inc,
      ReassignGroupingShow(reassign_grouping),
      bool_show(reassign_allow_partial),
      ReassignMethodShow(reassign_method));
  *** */
  if( DEBUG1 && DEBUG8 && KheInstanceRetrieveResource(ins, DEBUG8_ID, &r) )
  {
    rtm = KheResourceTimetableMonitor(soln, r);
    KheResourceTimetableMonitorPrintTimetable(rtm, NULL, 10, 2, stderr);
  }

  /* quit now if rs_reassign_resources (possibly plus NULL) is less than 2 */
  if( DEBUG1 && reassign_resources + (reassign_null ? 1 : 0) < 2 )
  {
    fprintf(stderr, "] %s returning false (too few resources)\n", fn_name);
    return false;
  }

  /* quit now if rs_reassign_select is "none" */
  if( DEBUG1 && reassign_select == REASSIGN_SELECT_NONE )
  {
    fprintf(stderr, "] %s returning false (rs_reassign_select is \"none\")\n",
      fn_name);
    return false;
  }

  /* quit now if the time limit has been reached */
  if( DEBUG1 && KheOptionsTimeLimitReached(options) )
  {
    fprintf(stderr, "] %s returning false (time limit)\n", fn_name);
    return false;
  }

  /* solve for rt, debug result and return */
  init_cost = KheSolnCost(soln);
  if( KheReassignSolveForResources(rs, reassign_resources, reassign_select,
	reassign_null, reassign_grouping, reassign_allow_partial, 2) )
  {
    KheReassignSolverDelete(rs);
    if( DEBUG1 )
      fprintf(stderr, "] %s returning true (%.5f -> %.5f)\n", fn_name,
	KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
    return true;
  }
  else
  {
    KheReassignSolverDelete(rs);
    if( DEBUG1 )
      fprintf(stderr, "] %s returning false\n", fn_name);
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassign1Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Call KheReassignSolverSolve repeatedly on the resources of type rt,      */
/*  depending on options.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheReassign1Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  return KheReassignDoRepair(soln, rt, options, "KheReassignRepair",
    "rs_reassign_resources", "rs_reassign_select", "rs_reassign_null",
    "rs_reassign_parts", "rs_reassign_start", "rs_reassign_increment",
    "rs_reassign_grouping", "rs_reassign_allow_partial",
    "rs_reassign_method", "rs_reassign_max_assignments");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassign2Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Like KheReassignRepair except consulting a different set of options.     */
/*                                                                           */
/*****************************************************************************/

bool KheReassign2Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  return KheReassignDoRepair(soln, rt, options, "KheReassign2Repair",
    "rs_reassign2_resources", "rs_reassign2_select", "rs_reassign2_null",
    "rs_reassign2_parts", "rs_reassign2_start", "rs_reassign2_increment",
    "rs_reassign2_grouping", "rs_reassign2_allow_partial",
    "rs_reassign2_method", "rs_reassign2_max_assignments");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassign3Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Like KheReassignRepair except consulting a different set of options.     */
/*                                                                           */
/*****************************************************************************/

bool KheReassign3Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  return KheReassignDoRepair(soln, rt, options, "KheReassign3Repair",
    "rs_reassign3_resources", "rs_reassign3_select", "rs_reassign3_null",
    "rs_reassign3_parts", "rs_reassign3_start", "rs_reassign3_increment",
    "rs_reassign3_grouping", "rs_reassign3_allow_partial",
    "rs_reassign3_method", "rs_reassign3_max_assignments");
}

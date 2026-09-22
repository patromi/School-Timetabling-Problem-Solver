
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
/*  FILE:         khe_sr_comb_solver.c                                       */
/*  DESCRIPTION:  Combinatorial grouping solver                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0	/* recursive search */
#define DEBUG4 0	/* KheCombSolverAddClassInfo */
#define DEBUG5 0	/* adding requirements */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELEMENT_INFO                                                         */
/*                                                                           */
/*  Information about one element (one time group or class), including how   */
/*  many times it has been covered.                                          */
/*                                                                           */
/*  Array first_cover_class_info contains those classes which cover this     */
/*  element but do not cover any previous element.  The solver will try      */
/*  to use these classes to cover this element.                              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_class_info_rec *KHE_CLASS_INFO;
typedef HA_ARRAY(KHE_CLASS_INFO) ARRAY_KHE_CLASS_INFO;

typedef struct khe_element_info_rec {

  /* constant during one solve */
  KHE_TIME_GROUP		time_group;	/* if element is a time group*/
  KHE_TASKER_CLASS		class;		/* if element is a class     */
  KHE_TIME_SET			time_set;
  ARRAY_KHE_CLASS_INFO		first_cover_class_info;
  KHE_COMB_SOLVER_COVER_TYPE	cover_type;

  /* varying during one solve */
  int				cover_count;
} *KHE_ELEMENT_INFO;

typedef HA_ARRAY(KHE_ELEMENT_INFO) ARRAY_KHE_ELEMENT_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASS_INFO                                                           */
/*                                                                           */
/*  Information about one class, including the elements it covers.           */
/*                                                                           */
/*****************************************************************************/

struct khe_class_info_rec {

  /* constant during one solve */
  KHE_TASKER_CLASS		class;
  ARRAY_KHE_ELEMENT_INFO	element_info;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_ALGORITHM - solve algorithm                                     */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_COMB_SOLVE_MIN = KHE_COMB_SOLVER_COST_MIN,
  KHE_COMB_SOLVE_ZERO = KHE_COMB_SOLVER_COST_ZERO,
  KHE_COMB_SOLVE_SOLE_ZERO = KHE_COMB_SOLVER_COST_SOLE_ZERO,
  KHE_COMB_SINGLES
} KHE_COMB_ALGORITHM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_SOLVER - solver for combinatorial grouping                      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASKER_CLASS) ARRAY_KHE_TASKER_CLASS;

struct khe_comb_solver_rec {

  /* constant for the lifetime of the solver */
  HA_ARENA			arena;
  KHE_TASKER			tasker;
  KHE_SOLN			soln;
  KHE_FRAME			days_frame;

  /* requirements, may vary freely but always defined */
  ARRAY_KHE_ELEMENT_INFO	element_info;
  ARRAY_KHE_ELEMENT_INFO	free_element_info;
  KHE_PROFILE_TIME_GROUP	profile_tg;
  KHE_RESOURCE_GROUP		profile_domain;
  bool				profile_max_len_on;
  bool				no_singles_on;

  /* set at the start of each solve, then constant throughout the solve */
  bool				class_info_ready;
  ARRAY_KHE_CLASS_INFO		class_info;
  ARRAY_KHE_CLASS_INFO		free_class_info;
  int				first_time_index;
  int				last_time_index;
  int				first_frame_index;
  int				last_frame_index;

  /* common to all algorithms */
  KHE_COMB_ALGORITHM		alg;
  bool				stopping;

  /* specific to KheCombSolverSolve */
  /* KHE_COMB_SOLVER_COST_TYPE	cost_type; */
  char				*debug_str;
  KHE_GROUP_MONITOR		group_monitor;
  KHE_RESOURCE			group_monitor_resource;
  /* KHE_TRACE			trace; */
  ARRAY_KHE_TASKER_CLASS	best_grouping;
  KHE_COST			best_cost;
  int				zero_costs;

  /* specific to KheCombSolverSingleTasks */
  int				single_tasks;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cover types and cost types"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *CoverTypeShow(KHE_COMB_SOLVER_COVER_TYPE cover_type)               */
/*                                                                           */
/*  Show a cover type.                                                       */
/*                                                                           */
/*****************************************************************************/

static char *CoverTypeShow(KHE_COMB_SOLVER_COVER_TYPE cover_type)
{
  switch( cover_type )
  {
    case KHE_COMB_SOLVER_COVER_YES:	return "cover_yes";
    case KHE_COMB_SOLVER_COVER_NO:	return "cover_no";
    case KHE_COMB_SOLVER_COVER_FREE:	return "cover_free";
    case KHE_COMB_SOLVER_COVER_PREV:	return "cover_prev";

    default:

      HnAbort("CoverTypeShow: unexpected cover type (%d)", cover_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *CostTypeShow(KHE_COMB_SOLVER_COST_TYPE cost_type)                  */
/*                                                                           */
/*  Show a cost type.                                                        */
/*                                                                           */
/*****************************************************************************/

static char *CostTypeShow(KHE_COMB_SOLVER_COST_TYPE cost_type)
{
  switch( cost_type )
  {
    case KHE_COMB_SOLVER_COST_MIN:	 return "cost_min";
    case KHE_COMB_SOLVER_COST_ZERO:	 return "cost_zero";
    case KHE_COMB_SOLVER_COST_SOLE_ZERO: return "cost_sole_zero";

    default:

      HnAbort("CostTypeShow: unexpected cost type (%d)", cost_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "element info"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELEMENT_INFO KheElementInfoMake(KHE_COMB_SOLVER cs,                  */
/*    KHE_TIME_GROUP tg, KHE_TASKER_CLASS c,                                 */
/*    KHE_COMB_SOLVER_COVER_TYPE cover)                                      */
/*                                                                           */
/*  Make an element info object.                                             */
/*                                                                           */
/*****************************************************************************/

static KHE_ELEMENT_INFO KheElementInfoMake(KHE_COMB_SOLVER cs,
  KHE_TIME_GROUP tg, KHE_TASKER_CLASS c, KHE_COMB_SOLVER_COVER_TYPE cover)
{
  KHE_ELEMENT_INFO res;
  if( HaArrayCount(cs->free_element_info) > 0 )
  {
    res = HaArrayLastAndDelete(cs->free_element_info);
    KheTimeSetClear(res->time_set);
    HaArrayClear(res->first_cover_class_info);
  }
  else
  {
    HaMake(res, cs->arena);
    res->time_set = KheTimeSetMake(KheSolnInstance(cs->soln), cs->arena);
    HaArrayInit(res->first_cover_class_info, cs->arena);
  }

  /* constant during one solve */
  res->time_group = tg;
  res->class = c;
  if( tg != NULL )
    KheTimeSetAddTimeGroup(res->time_set, tg);
  res->cover_type = cover;

  /* varying during one solve */
  res->cover_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElementInfoDebug(KHE_ELEMENT_INFO ei, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ei onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheClassInfoDebug(KHE_CLASS_INFO ci, int verbosity,
  int indent, FILE *fp);

static void KheElementInfoDebug(KHE_ELEMENT_INFO ei, int verbosity,
  int indent, FILE *fp)
{
  KHE_CLASS_INFO ci;  int i;
  fprintf(fp, "%*s[ ElementInfo(%s, %s, %d covers)\n", indent, "",
    ei->time_group != NULL ? KheTimeGroupId(ei->time_group) : "class",
    CoverTypeShow(ei->cover_type), ei->cover_count);
  HaArrayForEach(ei->first_cover_class_info, ci, i)
    KheClassInfoDebug(ci, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "class info"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASS_INFO KheClassInfoMake(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)  */
/*                                                                           */
/*  Make a class info object.                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_CLASS_INFO KheClassInfoMake(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)
{
  KHE_CLASS_INFO res;
  if( HaArrayCount(cs->free_class_info) > 0 )
  {
    res = HaArrayLastAndDelete(cs->free_class_info);
    HaArrayClear(res->element_info);
  }
  else
  {
    HaMake(res, cs->arena);
    HaArrayInit(res->element_info, cs->arena);
  }
  res->class = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClassInfoDebug(KHE_CLASS_INFO ci, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ci onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheClassInfoDebug(KHE_CLASS_INFO ci, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s[ ClassInfo (%d elements)\n", indent, "",
    HaArrayCount(ci->element_info));
  KheTaskerClassDebug(ci->class, 1, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "making class info, ready for solving"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClassCovers(KHE_TASKER_CLASS c, KHE_ELEMENT_INFO ei)             */
/*                                                                           */
/*  Return true if c covers ei.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheClassCovers(KHE_TASKER_CLASS c, KHE_ELEMENT_INFO ei)
{
  if( ei->time_group != NULL )
    return !KheTimeSetDisjoint(ei->time_set, KheTaskerClassTimeSet(c));
  else
    return ei->class == c;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClassIsWanted(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)            */
/*                                                                           */
/*  Return true if c is wanted, because it has not already been chosen, it   */
/*  does not cover any No elt, and (if requested) it is not too long.        */
/*                                                                           */
/*****************************************************************************/

static bool KheClassIsWanted(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)
{
  KHE_ELEMENT_INFO ei;  int i;  KHE_CLASS_INFO ci;

  /* if c has already been chosen, we don't want it again */
  HaArrayForEach(cs->class_info, ci, i)
    if( ci->class == c )
      return false;

  /* if domain is wanted but c doesn't have it, we don't want c */
  /* ***
  if( domain != NULL && !KheResourceGroupEqual(domain,KheTaskerClassDomain(c)) )
    return false;
  *** */

  /* if c covers any No time group, we don't want it */
  HaArrayForEach(cs->element_info, ei, i)
    if( ei->cover_type == KHE_COMB_SOLVER_COVER_NO && KheClassCovers(c, ei) )
      return false;

  /* if profile_max_len_on, check that requirement */
  if( cs->profile_max_len_on &&
      KheTaskerClassProfileTimeCount(c) > KheTaskerProfileMaxLen(cs->tasker) )
    return false;

  /* otherwise all good and we want c */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMakeAndAddClassInfo(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)      */
/*                                                                           */
/*  We've decided that c is wanted by the solve.  So make a class info       */
/*  object ci for it and add the elements covered by c to ci.  Also          */
/*  add ci to the first element it covers.                                   */
/*                                                                           */
/*  Only do this if c actually covers something.  But it will.               */
/*                                                                           */
/*****************************************************************************/

static void KheMakeAndAddClassInfo(KHE_COMB_SOLVER cs, KHE_TASKER_CLASS c)
{
  KHE_CLASS_INFO ci;  KHE_ELEMENT_INFO ei;  int i;
  ci = NULL;
  HaArrayForEach(cs->element_info, ei, i)
    if( ei->cover_type != KHE_COMB_SOLVER_COVER_NO && KheClassCovers(c, ei) )
    {
      if( ci == NULL )
      {
	ci = KheClassInfoMake(cs, c);
	HaArrayAddLast(cs->class_info, ci);
	HaArrayAddLast(ei->first_cover_class_info, ci);
      }
      HaArrayAddLast(ci->element_info, ei);
    }
  HnAssert(ci != NULL, "KheMakeAndAddClassInfo internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverMakeClassInfoReady(KHE_COMB_SOLVER cs)                 */
/*                                                                           */
/*  Make class info, ready for solving.  This finds the classes that lie in  */
/*  the search space, and sets cs->first_time_index and cs->last_time_index. */
/*                                                                           */
/*****************************************************************************/

static void KheCombSolverMakeClassInfoReady(KHE_COMB_SOLVER cs)
{
  KHE_ELEMENT_INFO ei;  int i, j, k, tg_count, index;  KHE_INSTANCE ins;
  KHE_TASKER_TIME t;  KHE_TASKER_CLASS c;  KHE_TIME time;

  if( !cs->class_info_ready )
  {
    /* initialize class_info for this solve, first removing any old classes */
    HaArrayAppend(cs->free_class_info, cs->class_info, i);
    HaArrayClear(cs->class_info);
    HaArrayForEach(cs->element_info, ei, i)
      if( ei->cover_type != KHE_COMB_SOLVER_COVER_NO )
      {
	if( ei->time_group != NULL )
	{
	  /* element is a time group */
	  for( j = 0;  j < KheTimeGroupTimeCount(ei->time_group);  j++ )
	  {
	    time = KheTimeGroupTime(ei->time_group, j);
	    t = KheTaskerTime(cs->tasker, KheTimeIndex(time));
	    for( k = 0;  k < KheTaskerTimeClassCount(t);  k++ )
	    {
	      c = KheTaskerTimeClass(t, k);
	      if( KheClassIsWanted(cs, c) )
		KheMakeAndAddClassInfo(cs, c);
	    }
	  }
	}
	else
	{
	  /* element is a class */
	  if( KheClassIsWanted(cs, ei->class) )
	    KheMakeAndAddClassInfo(cs, ei->class);
	}
      }

    /* initialize first_time_index and last_time_index */
    cs->first_time_index = INT_MAX;
    cs->last_time_index = -1;
    HaArrayForEach(cs->element_info, ei, i)
      if( ei->time_group != NULL )
      {
	tg_count = KheTimeGroupTimeCount(ei->time_group);
	if( tg_count > 0 )
	{
	  index = KheTimeIndex(KheTimeGroupTime(ei->time_group, 0));
	  if( index < cs->first_time_index )
	    cs->first_time_index = index;
	  index = KheTimeIndex(KheTimeGroupTime(ei->time_group, tg_count - 1));
	  if( index > cs->last_time_index )
	    cs->last_time_index = index;
	}
      }

    /* initialize first_frame_index and last_frame_index */
    ins = KheSolnInstance(cs->soln);
    time = KheInstanceTime(ins, cs->first_time_index);
    cs->first_frame_index = KheFrameTimeIndex(cs->days_frame, time);
    time = KheInstanceTime(ins, cs->last_time_index);
    cs->last_frame_index = KheFrameTimeIndex(cs->days_frame, time);

    /* ready now */
    cs->class_info_ready = true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and requirements"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_SOLVER KheCombSolverMake(KHE_TASKER tr, KHE_FRAME days_frame)   */
/*                                                                           */
/*  Make a new comb solver based on tr.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_COMB_SOLVER KheCombSolverMake(KHE_TASKER tr, KHE_FRAME days_frame)
{
  KHE_COMB_SOLVER res;  HA_ARENA a;

  /* constant for the lifetime of the solver */
  a = KheTaskerArena(tr);
  HaMake(res, a);
  res->arena = a;
  res->tasker = tr;
  res->soln = KheTaskerSoln(tr);
  res->days_frame = days_frame;

  /* requirements, may vary freely but always defined */
  HaArrayInit(res->element_info, a);
  HaArrayInit(res->free_element_info, a);
  res->profile_tg = NULL;
  res->profile_domain = NULL;
  res->profile_max_len_on = false;
  res->no_singles_on = false;

  /* set at the start of each solve, then constant throughout the solve */
  res->class_info_ready = false;
  HaArrayInit(res->class_info, a);
  HaArrayInit(res->free_class_info, a);
  res->first_time_index = 0;
  res->last_time_index = 0;
  res->first_frame_index = 0;
  res->last_frame_index = 0;

  /* common to all algorithms */
  res->alg = KHE_COMB_SINGLES;
  res->stopping = false;

  /* specific to KheCombSolverSolve */
  /* res->cost_type = KHE_COMB_SOLVER_COST_MIN; */
  res->debug_str = NULL;
  /* res->trace = NULL; */
  res->group_monitor = NULL;
  res->group_monitor_resource = NULL;
  HaArrayInit(res->best_grouping, a);
  res->best_cost = KheCost(INT_MAX, INT_MAX);
  res->zero_costs = 0;

  /* specific to KheCombSolverSingleTasks */
  res->single_tasks = 0;

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER KheCombSolverTasker(KHE_COMB_SOLVER cs)                       */
/*                                                                           */
/*  Return cs's tasker attribute.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER KheCombSolverTasker(KHE_COMB_SOLVER cs)
{
  return cs->tasker;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheCombSolverFrame(KHE_COMB_SOLVER cs)                         */
/*                                                                           */
/*  Return cs's frame attribute.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheCombSolverFrame(KHE_COMB_SOLVER cs)
{
  return cs->days_frame;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverClearRequirements(KHE_COMB_SOLVER cs)                  */
/*                                                                           */
/*  Clear away all requirements.                                             */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverClearRequirements(KHE_COMB_SOLVER cs)
{
  int i;
  HaArrayAppend(cs->free_element_info, cs->element_info, i);
  HaArrayClear(cs->element_info);
  cs->profile_tg = NULL;
  cs->profile_domain = NULL;
  cs->profile_max_len_on = false;
  cs->no_singles_on = false;
  cs->class_info_ready = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombSolverClearRequirements(cs) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddTimeGroupRequirement(KHE_COMB_SOLVER cs,            */
/*    KHE_TIME_GROUP tg, KHE_COMB_SOLVER_COVER_TYPE cover)                   */
/*                                                                           */
/*  Add a time group element to the solver.                                  */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddTimeGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_TIME_GROUP tg, KHE_COMB_SOLVER_COVER_TYPE cover)
{
  KHE_ELEMENT_INFO res;
  res = KheElementInfoMake(cs, tg, NULL, cover);
  HaArrayAddLast(cs->element_info, res);
  cs->class_info_ready = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombSolverAddTimeGroupRequirement(cs, %s, %s) ]\n",
      KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-",
      CoverTypeShow(cover));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteTimeGroupRequirement(KHE_COMB_SOLVER cs,         */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Undo the effect of a corresponding time group add.                       */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteTimeGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_TIME_GROUP tg)
{
  KHE_ELEMENT_INFO ei;  int i;
  HaArrayForEach(cs->element_info, ei, i)
    if( ei->time_group == tg )
    {
      HaArrayDeleteAndShift(cs->element_info, i);
      HaArrayAddLast(cs->free_element_info, ei);
      cs->class_info_ready = false;
      return;
    }
  HnAbort("KheCombSolverDeleteTimeGroupRequirement: tg not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddClassRequirement(KHE_COMB_SOLVER cs,                */
/*    KHE_TASKER_CLASS c, KHE_COMB_SOLVER_COVER_TYPE cover)                  */
/*                                                                           */
/*  Add a class element to the solver.                                       */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddClassRequirement(KHE_COMB_SOLVER cs,
  KHE_TASKER_CLASS c, KHE_COMB_SOLVER_COVER_TYPE cover)
{
  KHE_ELEMENT_INFO res;
  res = KheElementInfoMake(cs, NULL, c, cover);
  HaArrayAddLast(cs->element_info, res);
  cs->class_info_ready = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombSolverAddClassRequirement(cs, c) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteClassRequirement(KHE_COMB_SOLVER cs,             */
/*    KHE_TASKER_CLASS c)                                                    */
/*                                                                           */
/*  Undo the effect of a corresponding class add.                            */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteClassRequirement(KHE_COMB_SOLVER cs,
  KHE_TASKER_CLASS c)
{
  KHE_ELEMENT_INFO ei;  int i;
  HaArrayForEach(cs->element_info, ei, i)
    if( ei->class == c )
    {
      HaArrayDeleteAndShift(cs->element_info, i);
      HaArrayAddLast(cs->free_element_info, ei);
      cs->class_info_ready = false;
      return;
    }
  HnAbort("KheCombSolverDeleteClassRequirement: c not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddProfileGroupRequirement(KHE_COMB_SOLVER cs,         */
/*    KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain)                 */
/*                                                                           */
/*  Add a requirement that all combinations tried must contain a class       */
/*  that covers ptg and (optionally) has the given domain.                   */
/*                                                                           */
/*  At most one requirement can be added in this way.                        */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not change the      */
/*  value of cs->class_info_ready.                                           */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddProfileGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain)
{
  HnAssert(ptg != NULL, "KheCombSolverAddProfileGroupRequirement: ptg is NULL");
  cs->profile_tg = ptg;
  cs->profile_domain = domain;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteProfileGroupRequirement(KHE_COMB_SOLVER cs,      */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Undo the effect of a corresponding profile group add.                    */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not change the      */
/*  value of cs->class_info_ready.                                           */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteProfileGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_PROFILE_TIME_GROUP ptg)
{
  HnAssert(ptg != NULL,
    "KheCombSolverDeleteProfileGroupRequirement: ptg is NULL");
  HnAssert(ptg == cs->profile_tg,
    "KheCombSolverDeleteProfileGroupRequirement: ptg not present");
  cs->profile_tg = NULL;
  cs->profile_domain = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddProfileMaxLenRequirement(KHE_COMB_SOLVER cs)        */
/*                                                                           */
/*  Add a profile max len requirement.                                       */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddProfileMaxLenRequirement(KHE_COMB_SOLVER cs)
{
  if( !cs->profile_max_len_on )
  {
    cs->profile_max_len_on = true;
    cs->class_info_ready = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteProfileMaxLenRequirement(KHE_COMB_SOLVER cs)     */
/*                                                                           */
/*  Delete a corresponding profile max len add.                              */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteProfileMaxLenRequirement(KHE_COMB_SOLVER cs)
{
  if( cs->profile_max_len_on )
  {
    cs->profile_max_len_on = false;
    cs->class_info_ready = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddNoSinglesRequirement(KHE_COMB_SOLVER cs)            */
/*                                                                           */
/*  Add a requirement that there be no singles.                              */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not change the      */
/*  value of cs->class_info_ready.                                           */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddNoSinglesRequirement(KHE_COMB_SOLVER cs)
{
  cs->no_singles_on = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteNoSinglesRequirement(KHE_COMB_SOLVER cs)         */
/*                                                                           */
/*  Undo the effect of a corresponding no singles add.                       */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not change the      */
/*  value of cs->class_info_ready.                                           */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteNoSinglesRequirement(KHE_COMB_SOLVER cs)
{
  cs->no_singles_on = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving - assignment cost"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorInRange(KHE_COMB_SOLVER cs, KHE_TIME first_time,          */
/*    KHE_TIME last_time)                                                    */
/*                                                                           */
/*  Return true if a monitor that monitors times from first_time to          */
/*  last_time is in the range required by cs.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMonitorInRange(KHE_COMB_SOLVER cs, KHE_TIME first_time,
  KHE_TIME last_time)
{
  return cs->first_time_index <= KheTimeIndex(first_time) &&
    KheTimeIndex(last_time) <= cs->last_time_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorSuits(KHE_MONITOR m, KHE_COMB_SOLVER cs)                  */
/*                                                                           */
/*  Return true if m suits combinatorial grouping:  if it is an avoid        */
/*  clashes monitor, or a cluster busy times or limit busy times monitor     */
/*  whose times all lie in the current interval, derived from a constraint   */
/*  that applies to all resources.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMonitorSuits(KHE_MONITOR m, KHE_COMB_SOLVER cs)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_AVOID_CLASHES_CONSTRAINT acc;  KHE_AVOID_CLASHES_MONITOR acm;
  int count;  KHE_RESOURCE_TYPE rt;  KHE_TIME first_time, last_time;
  rt = KheTaskerResourceType(cs->tasker);
  count = KheResourceTypeResourceCount(rt);
  switch( KheMonitorTag(m) )
  {
    case KHE_AVOID_CLASHES_MONITOR_TAG:

      acm = (KHE_AVOID_CLASHES_MONITOR) m;
      acc = KheAvoidClashesMonitorConstraint(acm);
      if( KheAvoidClashesConstraintResourceOfTypeCount(acc, rt) < count )
	return false;
      return true;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
      if( KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc, rt) < count )
	return false;
      KheClusterBusyTimesMonitorRange(cbtm, &first_time, &last_time);
      return KheMonitorInRange(cs, first_time, last_time);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
      lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
      if( KheLimitBusyTimesConstraintResourceOfTypeCount(lbtc, rt) < count )
	return false;
      KheLimitBusyTimesMonitorRange(lbtm, &first_time, &last_time);
      return KheMonitorInRange(cs, first_time, last_time);

    default:

      return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheCombSolverTraceCost(KHE_COMB_SOLVER cs)                      */
/*                                                                           */
/*  Return the cost of the current trace, or rather, the part of it we       */
/*  are interested in.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_COST KheCombSolverTraceCost(KHE_COMB_SOLVER cs)
{
  int i;  KHE_MONITOR m;  KHE_COST res;
  res = 0;
  for( i = 0;  i < KheTraceMonitorCount(cs->trace);  i++ )
  {
    m = KheTraceMonitor(cs->trace, i);
    if( KheMonitorSuits(m, cs) )
      res += (KheMonitorCost(m) - KheTraceMonitorInitCost(cs->trace, i));
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheCombSolverCost(KHE_COMB_SOLVER cs, KHE_RESOURCE r)           */
/*                                                                           */
/*  Return the cost of r in the current range.  This might require           */
/*  a new group monitor.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheCombSolverCost(KHE_COMB_SOLVER cs, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( cs->group_monitor_resource != r )
  {
    if( cs->group_monitor != NULL )
      KheGroupMonitorDelete(cs->group_monitor);
    cs->group_monitor = KheGroupMonitorMake(cs->soln, 11, "comb grouping");
    cs->group_monitor_resource = r;
    rtm = KheResourceTimetableMonitor(cs->soln, r);
    KheResourceTimetableMonitorAddRange(rtm, cs->first_time_index,
      cs->last_time_index, cs->group_monitor);
  }
  return KheMonitorCost((KHE_MONITOR) cs->group_monitor);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving - adding, deleting, and verifying classes"            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCombSolverAddClassInfo(KHE_COMB_SOLVER cs, KHE_CLASS_INFO ci)    */
/*                                                                           */
/*  Add ci and return true, or return false and change nothing if adding     */
/*  ci would cover a time group twice or is not acceptable to the tasker.    */
/*                                                                           */
/*****************************************************************************/

static bool KheCombSolverAddClassInfo(KHE_COMB_SOLVER cs, KHE_CLASS_INFO ci,
  int curr_index)
{
  KHE_ELEMENT_INFO ei;  int i;
  HaArrayForEach(ci->element_info, ei, i)
    if( ei->cover_count > 0 )
    {
      if( DEBUG4 )
      {
	fprintf(stderr, "%*s  add class info failed on:\n", 2*curr_index, "");
	KheElementInfoDebug(ei, 1, 2*curr_index + 2, stderr);
      }
      return false;
    }
  if( !KheTaskerGroupingAddClass(cs->tasker, ci->class) )
  {
    if( DEBUG4 )
      fprintf(stderr, "%*s  add class info failed on tasker (%d classes)\n",
	2*curr_index, "", KheTaskerGroupingClassCount(cs->tasker));
    return false;
  }
  HaArrayForEach(ci->element_info, ei, i)
    ei->cover_count++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteClassInfo(KHE_COMB_SOLVER cs, KHE_CLASS_INFO ci) */
/*                                                                           */
/*  Undo the corresponding call to KheCombSolverAddClassInfo.                */
/*                                                                           */
/*****************************************************************************/

static void KheCombSolverDeleteClassInfo(KHE_COMB_SOLVER cs, KHE_CLASS_INFO ci)
{
  KHE_ELEMENT_INFO ei;  int i;
  HaArrayForEach(ci->element_info, ei, i)
    ei->cover_count--;
  if( !KheTaskerGroupingDeleteClass(cs->tasker, ci->class) )
    HnAbort("KheCombSolverDeleteClassInfo: cannot delete class from grouping");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombSolverClassSetHasValidCover(KHE_COMB_SOLVER cs)              */
/*                                                                           */
/*  Return true if cs's current set of classes has valid cover.              */
/*                                                                           */
/*****************************************************************************/

static bool KheCombSolverClassSetHasValidCover(KHE_COMB_SOLVER cs)
{
  KHE_ELEMENT_INFO ei, prev_ei;  int i;
  prev_ei = NULL;
  HaArrayForEach(cs->element_info, ei, i)
  {
    HnAssert(ei->cover_count <= 1, "KheCombSolverClassSetIsValid: 1");
    switch( ei->cover_type )
    {
      case KHE_COMB_SOLVER_COVER_FREE:

	/* 0 or 1 is fine here */
	break;

      case KHE_COMB_SOLVER_COVER_YES:

	/* should be covered once */
	if( ei->cover_count != 1 )
	  return false;
	break;

      case KHE_COMB_SOLVER_COVER_NO:

	/* should be not covered */
	HnAssert(ei->cover_count == 0, "KheCombSolverClassSetIsValid: 2");
	break;

      case KHE_COMB_SOLVER_COVER_PREV:

	/* should be covered if and only if prev is covered */
	if( prev_ei != NULL && ei->cover_count != prev_ei->cover_count )
	  return false;
	break;

      default:

	HnAbort("KheCombSolverClassSetIsValid: invalid cover (%d)",
	  ei->cover_type);
    }
    prev_ei = ei;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving - test assignment"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAvailable(KHE_COMB_SOLVER cs, KHE_RESOURCE r)          */
/*                                                                           */
/*  Return true if r is available for all time groups in the days frame      */
/*  lying between cs->first_time_index and cs->last_time_index inclusive.    */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAvailable(KHE_COMB_SOLVER cs, KHE_RESOURCE r)
{
  int i;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(cs->soln, r);
  for( i = cs->first_frame_index;  i <= cs->last_frame_index;  i++ )
  {
    tg = KheFrameTimeGroup(cs->days_frame, i);
    if( !KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg, NULL,
	  NULL, false) )
      return false;
    if( !KheResourceTimetableMonitorAvailableForTimeGroup(rtm, tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingFindSuitableResource(KHE_COMB_SOLVER cs, KHE_RESOURCE *r)*/
/*                                                                           */
/*  Find a suitable resource to assign to the first tasks of the classes     */
/*  of cs's tasker's grouping.  If found, set *r to the resource and         */
/*  return true; otherwise, set *r to NULL and return false.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupingFindSuitableResource(KHE_COMB_SOLVER cs, KHE_RESOURCE *r)
{
  KHE_TASKER_CLASS c;  int i;  KHE_RESOURCE_GROUP domain;
  c = KheTaskerGroupingClass(cs->tasker, 0);
  *r = KheTaskerClassAsstResource(c);
  if( *r != NULL )
  {
    /* leader class is already assigned, use the assigned resource */
    return true;
  }
  else
  {
    /* leader task is not already assigned, search its domain */
    domain = KheTaskerClassDomain(c);
    for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
    {
      *r = KheResourceGroupResource(domain, i);
      if( KheResourceIsAvailable(cs, *r) )
	return true;
    }
    return *r = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupingUnassign(KHE_TASKER tr, int stop)                        */
/*                                                                           */
/*  Unassign the first task of each class of tr's grouping, up to stop.      */
/*  Only don't unassign tasks in classes that have an assigned resource.     */
/*                                                                           */
/*****************************************************************************/

static void KheGroupingUnassign(KHE_TASKER tr, int stop)
{
  int i;  KHE_TASKER_CLASS c;  KHE_TASK task;
  for( i = 0;  i < stop;  i++ )
  {
    c = KheTaskerGroupingClass(tr, i);
    if( KheTaskerClassAsstResource(c) == NULL )
    {
      task = KheTaskerClassTask(c, 0);
      KheTaskUnAssignResource(task);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingAssignResource(KHE_TASKER tr, KHE_RESOURCE r)            */
/*                                                                           */
/*  Assign r to the first task in each class of tr's grouping, unless        */
/*  there is already an assignment.  Return true if all successful.          */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupingAssignResource(KHE_TASKER tr, KHE_RESOURCE r)
{
  KHE_TASKER_CLASS c;  int i;  KHE_TASK task;
  for( i = 0;  i < KheTaskerGroupingClassCount(tr);  i++ )
  {
    c = KheTaskerGroupingClass(tr, i);
    if( KheTaskerClassAsstResource(c) == NULL )
    {
      task = KheTaskerClassTask(c, 0);
      if( !KheTaskAssignResource(task, r) )
      {
        KheGroupingUnassign(tr, i);
	return false;
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupingUnassignResource(KHE_COMB_SOLVER cs)                     */
/*                                                                           */
/*  Undo a successful KheGroupingAssignResource.                             */
/*                                                                           */
/*****************************************************************************/

static void KheGroupingUnassignResource(KHE_COMB_SOLVER cs)
{
  KheGroupingUnassign(cs->tasker, KheTaskerGroupingClassCount(cs->tasker));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsFreeForGrouping(KHE_TASKER tr, KHE_RESOURCE r)         */
/*                                                                           */
/*  Return true if r is free for assignment to the tasks of the grouping.    */
/*                                                                           */
/*  Implementation note.  KheResourceTimetableMonitorTaskAvailableInFrame    */
/*  works correctly when frame == NULL, and hence so does this function.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheResourceIsFreeForGrouping(KHE_TASKER tr, KHE_RESOURCE r)
{
  KHE_TASKER_CLASS c;  int i;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TASK task;
  rtm = KheResourceTimetableMonitor(KheTaskerSoln(tr), r);
  for( i = 0;  i < KheTaskerGroupingClassCount(tr);  i++ )
  {
    c = KheTaskerGroupingClass(tr, i);
    task = KheTaskerClassTask(c, 0);
    if( !KheResourceTimetableMonitorTaskAvailableInFrame(rtm, task,
	tr->frame, NULL) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingTestAsstBegin(KHE_COMB_SOLVER cs, KHE_RESOURCE *r)       */
/*                                                                           */
/*  Carry out a test assignment of the current grouping.  If successful,     */
/*  set *r to the resource used and return true.  Otherwise set *r to NULL   */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheGroupingTestAsstBegin(KHE_COMB_SOLVER cs, KHE_RESOURCE *r)
{
  KHE_TASKER_CLASS c;  int i;  KHE_RESOURCE_GROUP domain;
  c = KheTaskerGroupingClass(cs->tasker, 0);
  *r = KheTaskerClassAsstResource(c);
  if( *r != NULL )
  {
    if( KheGroupingAssignResource(cs->tasker, *r) )
      return true;
    else
      return *r = NULL, false;
  }
  else
  {
    domain = KheTaskerClassDomain(c);
    for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
    {
      *r = KheResourceGroupResource(domain, i);
      if( KheResourceIsAvailable(cs, *r) )
      {
	if( KheGroupingAssignResource(cs->tasker, *r) )
	  return true;
	else
	  return *r = NULL, false;
      }
    }
    return *r = NULL, false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving - visiting all combinations"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheStop(KHE_COMB_SOLVER cs)                                         */
/*                                                                           */
/*  Return true if it's time to stop.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheStop(KHE_COMB_SOLVER cs)
{
  switch( cs->cost_type )
  {
    case KHE_COMB_SOLVER_COST_MIN:
    case KHE_COMB_SOLVER_COST_ZERO:

      return cs->zero_costs > 0;

    case KHE_COMB_SOLVER_COST_SOLE_ZERO:

      return cs->zero_costs > 1;

    default:

      HnAbort("KheStop: invalid cost_type (%d)", cs->cost_type);
      return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTry(KHE_COMB_SOLVER cs, int curr_index, KHE_ELEMENT_INFO curr_ei,*/
/*    bool try_nothing, bool try_something)                                  */
/*                                                                           */
/*  Handle curr_ei at curr_index.  If try_nothing is true, try assigning     */
/*  nothing here; if try_something is true, try assigning a class here.      */
/*                                                                           */
/*****************************************************************************/
static void KheCombSolverSolveFrom(KHE_COMB_SOLVER cs, int curr_index);

static void KheTry(KHE_COMB_SOLVER cs, int curr_index, KHE_ELEMENT_INFO curr_ei,
  bool try_nothing, bool try_something)
{
  KHE_CLASS_INFO ci;  int i;

  /* try nothing if requested */
  if( try_nothing )
  {
    KheCombSolverSolveFrom(cs, curr_index + 1);
    if( cs->stopping )
      return;
  }

  /* try something if requested and it would not violate singles limit */
  if( try_something && (cs->alg != KHE_COMB_SINGLES ||
      KheTaskerGroupingClassCount(cs->tasker) == 0) )
  {
    curr_ei = HaArray(cs->element_info, curr_index);
    HaArrayForEach(curr_ei->first_cover_class_info, ci, i)
    {
      if( KheCombSolverAddClassInfo(cs, ci, curr_index) )
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  [ adding class:\n", 2*curr_index, "");
	  KheClassInfoDebug(ci, 1, 2*curr_index + 2, stderr);
	}
	KheCombSolverSolveFrom(cs, curr_index + 1);
	KheCombSolverDeleteClassInfo(cs, ci);
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  ] removing class:\n", 2*curr_index, "");
	  KheClassInfoDebug(ci, 1, 2*curr_index + 2, stderr);
	}
	if( cs->stopping )
	  return;
      }
      else
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  not adding class:\n", 2*curr_index, "");
	  KheClassInfoDebug(ci, 1, 2*curr_index + 2, stderr);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDebugSoln(KHE_COMB_SOLVER cs, KHE_COST cost,           */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug a solution we've just found with this cost onto fp.                */
/*                                                                           */
/*****************************************************************************/

static void KheCombSolverDebugSoln(KHE_COMB_SOLVER cs, KHE_COST cost,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_TASKER_CLASS c;  KHE_MONITOR m;
  if( DEBUG1 )
  {
    fprintf(fp, "%*s[ valid assignable combination (cost %.5f):\n", indent, "",
      KheCostShow(cost));
    for( i = 0;  i < KheTaskerGroupingClassCount(cs->tasker);  i++ )
    {
      c = KheTaskerGroupingClass(cs->tasker, i);
      KheTaskerClassDebug(c, 1, indent + 2, stderr);
    }
    for( i = 0;  i < KheGroupMonitorChildMonitorCount(cs->group_monitor);  i++ )
    {
      m = KheGroupMonitorChildMonitor(cs->group_monitor, i);
      KheMonitorDebug(m, 1, indent + 2, stderr);
    }
    /* ***
    for( i = 0;  i < KheTraceMonitorCount(cs->trace);  i++ )
    {
      m = KheTraceMonitor(cs->trace, i);
      fprintf(stderr, "%*s%s %.5f -> %.5f ", indent + 2, "",
	KheMonitorSuits(m, cs) ? "suits" : "does not suit",
	KheCostShow(KheTraceMonitorInitCost(cs->trace, i)),
	KheCostShow(KheMonitorCost(m)));
      KheMonitorDebug(m, 1, 0, stderr);
    }
    *** */
    if( cost == 0 )
      fprintf(stderr, "%*s  new zero cost (count changing to %d)\n",
	indent, "", cs->zero_costs + 1);
    if( cost < cs->best_cost )
      fprintf(stderr, "%*s  new best grouping\n", indent, "");
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingSatisfiesProfileRequirement(KHE_COMB_SOLVER cs)          */
/*                                                                           */
/*  Return true if the current grouping satisfies cs's profile requirement.  */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupingSatisfiesProfileRequirement(KHE_COMB_SOLVER cs)
{
  int i;  KHE_TASKER_CLASS c;
  if( cs->profile_tg == NULL )
    return true;
  for( i = 0;  i < KheTaskerGroupingClassCount(cs->tasker);  i++ )
  {
    c = KheTaskerGroupingClass(cs->tasker, i);
    if( KheTaskerClassCoversProfileTimeGroup(c, cs->profile_tg) &&
	(cs->profile_domain == NULL ||
	 KheResourceGroupEqual(cs->profile_domain, KheTaskerClassDomain(c))) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingSatisfiesSinglesRequirement(KHE_COMB_SOLVER cs)          */
/*                                                                           */
/*  Return true if the current grouping satisfies cs's singles requirement.  */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupingSatisfiesSinglesRequirement(KHE_COMB_SOLVER cs)
{
  return KheTaskerGroupingClassCount(cs->tasker) >= (cs->no_singles_on ? 2 : 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupingSaveCurrAsBest(KHE_COMB_SOLVER cs)                       */
/*                                                                           */
/*  Save the current grouping as best.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheGroupingSaveCurrAsBest(KHE_COMB_SOLVER cs)
{
  int i;  KHE_TASKER_CLASS c;
  HaArrayClear(cs->best_grouping);
  for( i = 0;  i < KheTaskerGroupingClassCount(cs->tasker);  i++ )
  {
    c = KheTaskerGroupingClass(cs->tasker, i);
    HaArrayAddLast(cs->best_grouping, c);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverSolveFrom(KHE_COMB_SOLVER cs, int curr_index)          */
/*                                                                           */
/*  Solve cs from curr_index onwards, iterating over cs->element_info.       */
/*                                                                           */
/*****************************************************************************/

static void KheCombSolverSolveFrom(KHE_COMB_SOLVER cs, int curr_index)
{
  KHE_COST cost;  KHE_TASKER_CLASS c;  KHE_RESOURCE r;
  KHE_ELEMENT_INFO curr_ei, prev_ei;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cs, %d):\n", 2*curr_index + 2, "",
      curr_index);
  if( curr_index >= HaArrayCount(cs->element_info) )
  {
    /* end of recursion; if suitable classes, try them */
    if( DEBUG3 /* || DEBUG5 */ )
    {
      fprintf(stderr, "%*send recursion: singles %s, profile %s\n",
        2*curr_index + 4, "",
	bool_show(KheGroupingSatisfiesSinglesRequirement(cs)),
        bool_show(KheGroupingSatisfiesProfileRequirement(cs)));
    }
    if( KheGroupingSatisfiesSinglesRequirement(cs) &&
        KheGroupingSatisfiesProfileRequirement(cs) )
    {
      /* verify that the classes correctly cover the elements */
      HnAssert(KheCombSolverClassSetHasValidCover(cs),
	"KheCombSolverSolveFrom internal error");

      switch( cs->alg )
      {
	case KHE_COMB_SOLVE_MIN:

	  /* make a test assignment and check its cost */
	  /* KheTraceBegin(cs->trace); */
          if( KheGroupingFindSuitableResource(cs, &r) &&
	      KheGroupingAssignResource(cs->tasker, r) )
	  {
	    cost = KheCombSolverCost(cs, r);
	    /* cost = KheCombSolverTraceCost(cs); */
	    KheCombSolverDebugSoln(cs, cost, 1, 2*curr_index + 4, stderr);
	    if( cost < cs->best_cost )
	    {
	      /* new best cost; save grouping and cost */
              KheGroupingSaveCurrAsBest(cs);
	      cs->best_cost = cost;
	    }
            KheGroupingUnassignResource(cs);
	  }
	  /* KheTraceEnd(cs->trace); */
	  break;

	case KHE_COMB_SOLVE_ZERO:

	  /* make a test assignment and check its cost */
	  /* KheTraceBegin(cs->trace); */
          if( KheGroupingFindSuitableResource(cs, &r) &&
	      KheGroupingAssignResource(cs->tasker, r) )
	  {
	    cost = KheCombSolverCost(cs, r);
	    /* cost = KheCombSolverTraceCost(cs); */
	    KheCombSolverDebugSoln(cs, cost, 1, 2*curr_index + 4, stderr);
	    if( cost < cs->best_cost )
	      cs->best_cost = cost;
	    if( cost == 0 )
	    {
	      /* new zero cost; save grouping and stop */
	      cs->zero_costs++;
              KheGroupingSaveCurrAsBest(cs);
	      cs->stopping = true;
	    }
            KheGroupingUnassignResource(cs);
	  }
	  /* KheTraceEnd(cs->trace); */
	  break;

	case KHE_COMB_SOLVE_SOLE_ZERO:

	  /* make a test assignment and check its cost */
	  /* KheTraceBegin(cs->trace); */
          if( KheGroupingFindSuitableResource(cs, &r) &&
	      KheGroupingAssignResource(cs->tasker, r) )
	  {
	    cost = KheCombSolverCost(cs, r);
	    /* cost = KheCombSolverTraceCost(cs); */
	    KheCombSolverDebugSoln(cs, cost, 1, 2*curr_index + 4, stderr);
	    if( cost < cs->best_cost )
	      cs->best_cost = cost;
	    if( cost == 0 )
	    {
	      /* new zero cost; record its presence */
	      cs->zero_costs++;
	      if( cs->zero_costs == 1 )
		KheGroupingSaveCurrAsBest(cs);
	      else
		cs->stopping = true;
	    }
            KheGroupingUnassignResource(cs);
	  }
	  /* KheTraceEnd(cs->trace); */
	  break;

	case KHE_COMB_SINGLES:

	  if( KheTaskerGroupingClassCount(cs->tasker) == 1 )
	  {
	    c = KheTaskerGroupingClass(cs->tasker, 0);
	    cs->single_tasks += KheTaskerClassTaskCount(c);
	  }
	  break;

	default:

	  HnAbort("KheCombSolverSolveFrom internal error (alg %d)", cs->alg);
	  break;
      }
    }
  }
  else
  {
    curr_ei = HaArray(cs->element_info, curr_index);
    if( DEBUG3 )
      KheElementInfoDebug(curr_ei, 2, 2*curr_index + 4, stderr);
    switch( curr_ei->cover_type )
    {
      case KHE_COMB_SOLVER_COVER_YES:

        KheTry(cs, curr_index, curr_ei, curr_ei->cover_count > 0,
	  curr_ei->cover_count == 0);
	break;

      case KHE_COMB_SOLVER_COVER_NO:

        KheTry(cs, curr_index, curr_ei, curr_ei->cover_count == 0, false);
	break;

      case KHE_COMB_SOLVER_COVER_FREE:

        KheTry(cs, curr_index, curr_ei, true, curr_ei->cover_count == 0);
	break;

      case KHE_COMB_SOLVER_COVER_PREV:

	prev_ei = HaArray(cs->element_info, curr_index - 1);
	if( prev_ei->cover_count == 0 )
	{
	  /* like KHE_COMB_SOLVER_COVER_NO for curr_ei */
	  KheTry(cs, curr_index, curr_ei, curr_ei->cover_count == 0, false);
	}
	else
	{
	  /* like KHE_COMB_SOLVER_COVER_YES for curr_ei */
	  KheTry(cs, curr_index, curr_ei, curr_ei->cover_count > 0,
	    curr_ei->cover_count == 0);
	}
	break;

      default:

	HnAbort("KheCombSolverSolveFrom: unexpected cover_type (%d)",
	  curr_ei->cover_type);
	break;
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]%s\n", 2*curr_index + 2, "",
      cs->stopping ? " stopping" : "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving - public functions"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int DoBestGrouping(KHE_COMB_SOLVER cs, int max_num)                      */
/*                                                                           */
/*  Carry out a grouping of the classes of cs->best_grouping.                */
/*                                                                           */
/*****************************************************************************/

static int DoBestGrouping(KHE_COMB_SOLVER cs, int max_num)
{
  KHE_TASKER_CLASS c;  int i;
  if( HaArrayCount(cs->best_grouping) >= 2 )
  {
    KheTaskerGroupingClear(cs->tasker);
    HaArrayForEach(cs->best_grouping, c, i)
      if( !KheTaskerGroupingAddClass(cs->tasker, c) )
	HnAbort("DoBestGrouping: cannot add best class");
    return KheTaskerGroupingBuild(cs->tasker, max_num, cs->debug_str);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombSolverSolve(KHE_COMB_SOLVER cs, int max_num,                  */
/*    KHE_COMB_SOLVER_COST_TYPE ct, char *debug_str)                         */
/*                                                                           */
/*  Carry out a solve.                                                       */
/*                                                                           */
/*****************************************************************************/

int KheCombSolverSolve(KHE_COMB_SOLVER cs, int max_num,
  KHE_COMB_SOLVER_COST_TYPE ct, char *debug_str)
{
  int i, res;  KHE_ELEMENT_INFO ei;

  /* initialize the "set at the start of each solve" fields */
  KheCombSolverMakeClassInfoReady(cs);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombSolverSolve(cs, %d, %s) solving:\n",
      max_num, CostTypeShow(ct));
    KheCombSolverDebug(cs, 2, 2, stderr);
  }

  /* initialize the "common to all algorithms" fields */
  cs->alg = (KHE_COMB_ALGORITHM) ct;
  cs->stopping = false;

  /* initialize the "specific to KheCombSolverSolve" fields */
  KheTaskerGroupingClear(cs->tasker);
  cs->debug_str = debug_str;
  /* cs->trace = KheTraceMake((KHE_GROUP_MONITOR) cs->soln); */
  cs->group_monitor = NULL;
  cs->group_monitor_resource = NULL;
  HaArrayClear(cs->best_grouping);
  cs->best_cost = KheCost(INT_MAX, INT_MAX);
  cs->zero_costs = 0;

  /* do the actual solve */
  HaArrayForEach(cs->element_info, ei, i)
    HnAssert(ei->cover_count == 0,
      "KheCombSolverSolve internal error 1 (index %d)\n", i);
  KheCombSolverSolveFrom(cs, 0);

  /* sort out the consequences */
  res = 0;
  switch( ct )
  {
    case KHE_COMB_SOLVER_COST_MIN:

      /* if there is a min, then do it */
      if( cs->best_cost < KheCost(INT_MAX, INT_MAX) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (min)\n", max_num);
	res = DoBestGrouping(cs, max_num);
      }
      break;

    case KHE_COMB_SOLVER_COST_ZERO:

      /* if there is a zero cost soln, then do it */
      if( cs->zero_costs >= 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (zero cost)\n", max_num);
	res = DoBestGrouping(cs, max_num);
      }
      break;

    case KHE_COMB_SOLVER_COST_SOLE_ZERO:

      /* if there is a single zero cost soln, then do it */
      if( cs->zero_costs == 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (sole zero cost)\n", max_num);
	res = DoBestGrouping(cs, max_num);
      }
      break;

    default:

      HnAbort("KheCombSolverSolve: unknown ct (%d)", ct);
      break;
  }

  /* tidy up and return */
  /* KheTraceDelete(cs->trace); */
  /* cs->trace = NULL; */
  if( cs->group_monitor != NULL )
  {
    KheGroupMonitorDelete(cs->group_monitor);
    cs->group_monitor = NULL;
    cs->group_monitor_resource = NULL;
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheCombSolverSolve returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombSolverSingleTasks(KHE_COMB_SOLVER cs)                         */
/*                                                                           */
/*  Return the number of individual tasks that satisfy cs's requirements.    */
/*                                                                           */
/*****************************************************************************/

int KheCombSolverSingleTasks(KHE_COMB_SOLVER cs)
{
  KHE_ELEMENT_INFO ei;  int i;

  /* initialize the "set at the start of each solve" fields */
  KheCombSolverMakeClassInfoReady(cs);

  /* initialize the "common to all algorithms" fields */
  cs->alg = KHE_COMB_SINGLES;
  cs->stopping = false;

  /* initialize the "specific to KheCombSolverSingleTasks" fields */
  cs->single_tasks = 0;

  /* do the solve */
  HaArrayForEach(cs->element_info, ei, i)
    HnAssert(ei->cover_count == 0,
      "KheCombSolverSolve internal error 1 (index %d)\n", i);
  KheCombSolverSolveFrom(cs, 0);

  /* tidy up and return */
  return cs->single_tasks;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDebug(KHE_COMB_SOLVER cs, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDebug(KHE_COMB_SOLVER cs, int verbosity,
  int indent, FILE *fp)
{
  KHE_ELEMENT_INFO ei;  int i;
  fprintf(fp, "%*s[ CombSolver(%s, type %s): ", indent, "",
    KheInstanceId(KheSolnInstance(cs->soln)),
    KheResourceTypeId(KheTaskerResourceType(cs->tasker)));
  HaArrayForEach(cs->element_info, ei, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    if( ei->time_group != NULL )
      fprintf(fp, "%s", KheTimeGroupId(ei->time_group));
    else
      fprintf(fp, "class");
  }
  fprintf(fp, "\n");
  fprintf(stderr, "%*s  frame time groups are %s .. %s\n", indent, "",
    KheTimeGroupId(KheFrameTimeGroup(cs->days_frame, cs->first_frame_index)),
    KheTimeGroupId(KheFrameTimeGroup(cs->days_frame, cs->last_frame_index)));
  HaArrayForEach(cs->element_info, ei, i)
    KheElementInfoDebug(ei, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}

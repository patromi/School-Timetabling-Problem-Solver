
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
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
/*  FILE:         space.c                                                    */
/*  MODULE:       A multi-dimensional space used for timetabling printing    */
/*                                                                           */
/*****************************************************************************/
#include "space.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - dimensions"                               */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TIME_GROUP) ARRAY_KHE_TIME_GROUP;
typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;
typedef HA_ARRAY(KHE_TASK) ARRAY_TASK;

typedef enum {
  DIM_TYPE_TIME,
  DIM_TYPE_RESOURCE,
  DIM_TYPE_RESOURCE_TYPE,
  DIM_TYPE_DUMMY,
} DIM_TYPE;

#define INHERIT_SPACE_DIM				\
  DIM_TYPE			dim_type;		\
  int				count;			\
  SPACE				encl_space;		\
  HA_ARRAY_NSTRING		labels;

typedef struct space_dim_rec {
  INHERIT_SPACE_DIM
} *SPACE_DIM;

struct space_time_dim_rec {
  INHERIT_SPACE_DIM
  ARRAY_KHE_TIME_GROUP		time_groups;
};

struct space_resource_dim_rec {
  INHERIT_SPACE_DIM
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;
  SPACE_RG_FN			major_heading_rg_fn;
  SPACE_RG_FN			avail_rg_fn;
  void				*avail_rg_impl;
  SPACE_RG_FN			after_rg_fn;
};

typedef struct space_resource_type_dim_rec {
  INHERIT_SPACE_DIM
  KHE_RESOURCE_TYPE		resource_type;
  SPACE_RG_FN			major_heading_rg_fn;
  SPACE_RG_FN			avail_rg_fn;
  void				*avail_rg_impl;
  SPACE_RG_FN			after_rg_fn;
} *SPACE_RESOURCE_TYPE_DIM;

typedef struct space_dummy_dim_rec {
  INHERIT_SPACE_DIM
} *SPACE_DUMMY_DIM;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - subspace conditions"                      */
/*                                                                           */
/*****************************************************************************/

typedef struct subspace_cond_rec {
  KHE_TIME_GROUP		time_group;
  KHE_RESOURCE_GROUP		resource_group;
  bool				runassigned;
} SUBSPACE_COND;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - subspaces"                                */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  SUBSPACE_TYPE_INTERNAL,
  SUBSPACE_TYPE_EXTERNAL,
} SUBSPACE_TYPE;

#define INHERIT_SUBSPACE				\
  SUBSPACE_TYPE			subspace_type;		\
  SPACE				encl_space;

typedef struct subspace_rec {
  INHERIT_SUBSPACE
} *SUBSPACE;

typedef HA_ARRAY(SUBSPACE) ARRAY_SUBSPACE;

typedef struct subspace_internal_rec {
  INHERIT_SUBSPACE
  int				total_durn;
  float				total_workload;
  ARRAY_SUBSPACE		subspaces;
} *SUBSPACE_INTERNAL;

typedef struct subspace_external_rec {
  INHERIT_SUBSPACE
  SUBSPACE_COND			condition;
  ARRAY_TASK			tasks;
  bool				spanned_over;
} *SUBSPACE_EXTERNAL;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - weeks"                                    */
/*                                                                           */
/*****************************************************************************/

typedef struct week_rec {
  KHE_TIME_GROUP	week_tg;
  ARRAY_KHE_TIME_GROUP	days;
} *WEEK;

typedef HA_ARRAY(WEEK) ARRAY_WEEK;

typedef struct space_error_rec {
  SPACE_ERROR_TYPE	error_type;
  KHE_TIME_GROUP	time_group1;
  KHE_TIME_GROUP	time_group2;
  KHE_TIME_GROUP	time_group3;
} *SPACE_ERROR;

typedef HA_ARRAY(SPACE_ERROR) ARRAY_SPACE_ERROR;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - space"                                    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(SPACE_DIM) ARRAY_SPACE_DIM;

typedef struct resource_info_rec {
  KHE_RESOURCE			resource;
  KHE_TIME_GROUP		hard_unavailable_tg;
  KHE_TIME_GROUP		soft_unavailable_tg;
  KHE_TIME_GROUP		hard_must_be_busy_tg;
  KHE_TIME_GROUP		soft_must_be_busy_tg;
} *RESOURCE_INFO;

typedef HA_ARRAY(RESOURCE_INFO) ARRAY_RESOURCE_INFO;

typedef struct colour_select_ref {
  SPACE_CELL_COND	cell_cond;
  char			*constraints_str;
  char			*colour;
} COLOUR_SELECT;

typedef HA_ARRAY(COLOUR_SELECT) ARRAY_COLOUR_SELECT;

typedef struct font_select_ref {
  SPACE_TASK_COND	task_cond;
  bool			show;
  HTML_FONT		font;
} FONT_SELECT;

typedef HA_ARRAY(FONT_SELECT) ARRAY_FONT_SELECT;

struct space_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  bool				allow_spanning;
  char				*dft_colour;
  bool				dft_show;
  HTML_FONT			dft_font;
  ARRAY_KHE_TIME_GROUP		days;
  ARRAY_WEEK			weeks;
  int				max_times_per_day;
  ARRAY_KHE_TIME_GROUP		days_of_week;
  HA_ARRAY_NSTRING		days_of_week_labels;
  ARRAY_SPACE_ERROR		errors;
  ARRAY_RESOURCE_INFO		resource_info;
  ARRAY_COLOUR_SELECT		colour_selects;
  ARRAY_FONT_SELECT		font_selects;
  ARRAY_SPACE_DIM		dimensions;
  SUBSPACE			root_subspace;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "dimensions"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  SPACE_TIME_DIM SpaceTimeDimMake(SPACE encl_space)                        */
/*                                                                           */
/*  Return a new, empty time dimension for encl_space.                       */
/*                                                                           */
/*****************************************************************************/

static SPACE_TIME_DIM SpaceTimeDimMake(SPACE encl_space)
{
  SPACE_TIME_DIM res;  HA_ARENA a;
  a = encl_space->arena;
  HaMake(res, a);
  res->dim_type = DIM_TYPE_TIME;
  res->count = 0;
  res->encl_space = encl_space;
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->labels, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_RESOURCE_DIM SpaceResourceDimMake(SPACE encl_space,                */
/*    SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,              */
/*    void *avail_rg_impl, SPACE_RG_FN after_rg_fn)                          */
/*                                                                           */
/*  Return a new, empty resource dimension for encl_space.                   */
/*                                                                           */
/*****************************************************************************/

static SPACE_RESOURCE_DIM SpaceResourceDimMake(SPACE encl_space,
  SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,
  void *avail_rg_impl, SPACE_RG_FN after_rg_fn)
{
  SPACE_RESOURCE_DIM res;  HA_ARENA a;
  a = encl_space->arena;
  HaMake(res, a);
  res->dim_type = DIM_TYPE_RESOURCE;
  res->count = 0;
  res->encl_space = encl_space;
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->labels, a);
  res->major_heading_rg_fn = major_heading_rg_fn;
  res->avail_rg_fn = avail_rg_fn;
  res->avail_rg_impl = avail_rg_impl;
  res->after_rg_fn = after_rg_fn;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_RESOURCE_TYPE_DIM SpaceResourceTypeDimMake(KHE_RESOURCE_TYPE rt,   */
/*    SPACE encl_space, SPACE_RG_FN major_heading_rg_fn,                     */
/*    SPACE_RG_FN avail_rg_fn, void *avail_rg_impl, SPACE_RG_FN after_rg_fn) */
/*                                                                           */
/*  Return a new, empty resource type dimension for encl_space.              */
/*                                                                           */
/*****************************************************************************/

static SPACE_RESOURCE_TYPE_DIM SpaceResourceTypeDimMake(KHE_RESOURCE_TYPE rt,
  SPACE encl_space, SPACE_RG_FN major_heading_rg_fn,
  SPACE_RG_FN avail_rg_fn, void *avail_rg_impl, SPACE_RG_FN after_rg_fn)
{
  SPACE_RESOURCE_TYPE_DIM res;  HA_ARENA a;  KHE_RESOURCE r;  int i;
  a = encl_space->arena;
  HaMake(res, a);
  res->dim_type = DIM_TYPE_RESOURCE_TYPE;
  res->encl_space = encl_space;
  res->resource_type = rt;
  HaArrayInit(res->labels, a);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    HaArrayAddLast(res->labels, KheResourceId(r));
  }
  HaArrayAddLast(res->labels, "Unassigned");
  res->major_heading_rg_fn = major_heading_rg_fn;
  res->avail_rg_fn = avail_rg_fn;
  res->avail_rg_impl = avail_rg_impl;
  res->after_rg_fn = after_rg_fn;
  res->count = KheResourceTypeResourceCount(rt) + 1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_DUMMY_DIM SpaceDummyDimMake(SPACE space)                           */
/*                                                                           */
/*  Make dummy dimension that has a single element and adds no conditions.   */
/*                                                                           */
/*****************************************************************************/

SPACE_DUMMY_DIM SpaceDummyDimMake(SPACE space)
{
  SPACE_DUMMY_DIM res;
  HaMake(res, space->arena);
  res->dim_type = DIM_TYPE_DUMMY;
  res->count = 1;
  res->encl_space = space;
  HaArrayInit(res->labels, space->arena);
  HaArrayAddLast(res->labels, "All");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SUBSPACE_COND SubSpaceCondMake(KHE_TIME_GROUP tg, KHE_RESOURCE_GROUP rg, */
/*    bool runassigned)                                                      */
/*                                                                           */
/*  Make a new subspace condition with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static SUBSPACE_COND SubSpaceCondMake(KHE_TIME_GROUP tg, KHE_RESOURCE_GROUP rg,
  bool runassigned)
{
  SUBSPACE_COND res;
  res.time_group = tg;
  res.resource_group = rg;
  res.runassigned = runassigned;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SUBSPACE_COND SpaceDimRestrictCondition(SPACE_DIM dim, int index,        */
/*    SUBSPACE_COND c)                                                       */
/*                                                                           */
/*  Given that c determines which time and resource assignments are allowed  */
/*  in the enclosing context, return a more restricted condition which is    */
/*  for the index'th position of dim.                                        */
/*                                                                           */
/*****************************************************************************/

static SUBSPACE_COND SpaceDimRestrictCondition(SPACE_DIM dim, int index,
  SUBSPACE_COND c)
{
  SPACE_TIME_DIM std;  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;
  KHE_SOLN soln;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;  KHE_TIME time;
  KHE_TIME_GROUP tg;  KHE_RESOURCE_GROUP rg;
  SUBSPACE_COND res;
  switch( dim->dim_type )
  {
    case DIM_TYPE_TIME:

      std = (SPACE_TIME_DIM) dim;
      tg = HaArray(std->time_groups, index);
      if( c.time_group == NULL )
	res.time_group = tg;
      else if( KheTimeGroupSubset(tg, c.time_group) )
	res.time_group = tg;
      else
      {
	soln = dim->encl_space->soln;
	KheSolnTimeGroupBegin(soln);
	KheSolnTimeGroupUnion(soln, c.time_group);
	KheSolnTimeGroupIntersect(soln, tg);
	res.time_group = KheSolnTimeGroupEnd(soln);
	if( KheTimeGroupTimeCount(res.time_group) == 1 )
	{
	  time = KheTimeGroupTime(res.time_group, 0);
	  res.time_group = KheTimeSingletonTimeGroup(time);
	}
      }
      res.resource_group = c.resource_group;
      res.runassigned = c.runassigned;
      break;

    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      res.time_group = c.time_group;
      rg = HaArray(srd->resource_groups, index);
      rt = KheResourceGroupResourceType(rg);
      if( c.resource_group == NULL )
	res.resource_group = rg;
      else if( KheResourceGroupResourceType(c.resource_group) != rt )
	res.resource_group = KheResourceTypeEmptyResourceGroup(rt);
      else if( KheResourceGroupSubset(rg, c.resource_group) )
	res.resource_group = rg;
      else
      {
	soln = dim->encl_space->soln;
	KheSolnResourceGroupBegin(soln, rt);
	KheSolnResourceGroupUnion(soln, c.resource_group);
	KheSolnResourceGroupIntersect(soln, rg);
	res.resource_group = KheSolnResourceGroupEnd(soln);
	if( KheResourceGroupResourceCount(res.resource_group) == 1 )
	{
	  r = KheResourceGroupResource(res.resource_group, 0);
	  res.resource_group = KheResourceSingletonResourceGroup(r);
	}
      }
      res.runassigned = false;
      break;

    case DIM_TYPE_RESOURCE_TYPE:

      /* like resource but with each resource plus unassigned at the end */
      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      res.time_group = c.time_group;
      rt = srtd->resource_type;
      if( index == KheResourceTypeResourceCount(rt) )
      {
	/* unassigned tasks only */
        res.resource_group = KheResourceTypeEmptyResourceGroup(rt);
        res.runassigned = c.runassigned;
      }
      else
      {
	/* like resource case with a singleton resource group */
	r = KheResourceTypeResource(rt, index);
	rg = KheResourceSingletonResourceGroup(r);
	if( c.resource_group == NULL )
	  res.resource_group = rg;
	else if( KheResourceGroupResourceType(c.resource_group) != rt )
	  res.resource_group = KheResourceTypeEmptyResourceGroup(rt);
	else if( KheResourceGroupContains(c.resource_group, r) )
          res.resource_group = rg;
	else
	  res.resource_group = KheResourceTypeEmptyResourceGroup(rt);
	res.runassigned = false;
      }
      break;

    case DIM_TYPE_DUMMY:

      /* dummy dimension, imposes no new conditions */
      res = c;
      break;

    default:

      HnAbort("SpaceDimRestrictCondition internal error");
      res = SubSpaceCondMake(NULL, NULL, false);  /* keep compiler happy */
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SpaceDimIndex(SPACE_DIM dim, KHE_TIME t, KHE_TASK task,             */
/*    KHE_RESOURCE r, int *index)                                            */
/*                                                                           */
/*  If task (which is assigned r at time t) can be accommodated in this dim, */
/*  set *index to its (unique) index and return true.  Else return false.    */
/*                                                                           */
/*****************************************************************************/

static bool SpaceDimIndex(SPACE_DIM dim, KHE_TIME t, KHE_TASK task,
  KHE_RESOURCE r, int *index)
{
  SPACE_TIME_DIM std;  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;
  int i, pos;  KHE_TIME_GROUP tg;  KHE_RESOURCE_GROUP rg;
  switch( dim->dim_type )
  {
    case DIM_TYPE_TIME:

      HnAssert(t != NULL, "SpaceDimIndex internal error 1");
      std = (SPACE_TIME_DIM) dim;
      *index = -1;
      HaArrayForEach(std->time_groups, tg, i)
	if( KheTimeGroupContains(tg, t, &pos) )
	{
	  *index = i;
	  break;
	}
      break;

    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      *index = -1;
      if( r != NULL )
      {
	HaArrayForEach(srd->resource_groups, rg, i)
	  if( KheResourceGroupContains(rg, r) )
	  {
	    *index = i;
	    break;
	  }
      }
      break;

    case DIM_TYPE_RESOURCE_TYPE:

      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      if( KheTaskResourceType(task) != srtd->resource_type )
	*index = -1;
      else if( r == NULL )
	*index = KheResourceTypeResourceCount(srtd->resource_type);
      else
	*index = KheResourceResourceTypeIndex(r);
      break;

    case DIM_TYPE_DUMMY:

      *index = 0;
      break;

    default:

      HnAbort("SpaceDimIndex internal error");
      *index = -1;
  }
  return (*index != -1);
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_RG_FN SpaceDimAvailFn(SPACE_DIM dim)                               */
/*                                                                           */
/*  Return the avail function of dim, or NULL if none.                       */
/*                                                                           */
/*****************************************************************************/

static SPACE_RG_FN SpaceDimAvailFn(SPACE_DIM dim, void **avail_rg_impl)
{
  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;
  switch( dim->dim_type )
  {
    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      return *avail_rg_impl = srd->avail_rg_impl, srd->avail_rg_fn;

    case DIM_TYPE_RESOURCE_TYPE:

      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      return *avail_rg_impl = srtd->avail_rg_impl, srtd->avail_rg_fn;

    default:

      return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP SpaceDimResourceGroup(SPACE_DIM dim, int index)       */
/*                                                                           */
/*  Return the resource group of dim at index.  Abort if wrong dim type.     */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP SpaceDimResourceGroup(SPACE_DIM dim, int index)
{
  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;
  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;
  switch( dim->dim_type )
  {
    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      return HaArray(srd->resource_groups, index);

    case DIM_TYPE_RESOURCE_TYPE:

      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      rt = srtd->resource_type;
      if( index == KheResourceTypeResourceCount(rt) )
	return KheResourceTypeEmptyResourceGroup(rt);
      else
      {
	r = KheResourceTypeResource(rt, index);
	return KheResourceSingletonResourceGroup(r);
      }

    default:

      HnAbort("SpaceDimResourceGroup internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource_info"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AvoidUnavailableTimesConstraintAddUnavailableTimes(                 */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_SOLN soln)               */
/*    ARRAY_KHE_TIME *hard_utimes, ARRAY_KHE_TIME *soft_utimes)              */
/*                                                                           */
/*  Add the times that c indicates to be unavailable to the time group       */
/*  currently under construction in soln.                                    */
/*                                                                           */
/*****************************************************************************/

static void AvoidUnavailableTimesConstraintAddUnavailableTimes(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  KHE_TIME_GROUP tg;
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(c);
  KheSolnTimeGroupUnion(soln, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void ClusterBusyTimesConstraintAddUnavailableTimes(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                    */
/*                                                                           */
/*  Add the times that c indicates to be unavailable to the time group       */
/*  currently under construction in soln.                                    */
/*                                                                           */
/*****************************************************************************/

static void ClusterBusyTimesConstraintAddUnavailableTimes(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j, offset;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  if( KheClusterBusyTimesConstraintMaximum(c) == 0 )
  {
    for( i = 0; i < KheClusterBusyTimesConstraintAppliesToOffsetCount(c); i++ )
    {
      offset = KheClusterBusyTimesConstraintAppliesToOffset(c, i);
      for( j = 0;  j < KheClusterBusyTimesConstraintTimeGroupCount(c);  j++ )
      {
	tg = KheClusterBusyTimesConstraintTimeGroup(c, j, offset, &po);
	if( po == KHE_POSITIVE )
	  KheSolnTimeGroupUnion(soln, tg);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LimitBusyTimesConstraintAddUnavailableTimes(                        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                      */
/*                                                                           */
/*  Add the times that c indicates to be unavailable to the time group       */
/*  currently under construction in soln.                                    */
/*                                                                           */
/*****************************************************************************/

static void LimitBusyTimesConstraintAddUnavailableTimes(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j, offset;  KHE_TIME_GROUP tg;
  if( KheLimitBusyTimesConstraintMaximum(c) == 0 )
  {
    for( i = 0; i < KheLimitBusyTimesConstraintAppliesToOffsetCount(c); i++ )
    {
      offset = KheLimitBusyTimesConstraintAppliesToOffset(c, i);
      for( j = 0;  j < KheLimitBusyTimesConstraintTimeGroupCount(c);  j++ )
      {
	tg = KheLimitBusyTimesConstraintTimeGroup(c, j, offset);
	KheSolnTimeGroupUnion(soln, tg);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LimitActiveIntervalsConstraintAddUnavailableTimes(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, KHE_SOLN soln)*/
/*                                                                           */
/*  Add the times that c indicates to be unavailable to the time group       */
/*  currently under construction in soln.                                    */
/*                                                                           */
/*****************************************************************************/

static void LimitActiveIntervalsConstraintAddUnavailableTimes(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, KHE_SOLN soln)
{
  int i, j, count, offset, history;  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* unavailable becuase at or over maximum limit of busy times */
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  history = KheLimitActiveIntervalsConstraintHistory(c, r);
  if( history >= KheLimitActiveIntervalsConstraintMaximum(c) && count > 0 )
  {
    for(i=0; i < KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c); i++)
    {
      offset = KheLimitActiveIntervalsConstraintAppliesToOffset(c, i);
      tg = KheLimitActiveIntervalsConstraintTimeGroup(c, 0, offset, &po);
      if( po == KHE_POSITIVE )
	KheSolnTimeGroupUnion(soln, tg);
    }
  }

  /* unavailable becuase under minimum limit of free times */
  if( history > 0 && history < KheLimitActiveIntervalsConstraintMinimum(c) )
  {
    count = KheLimitActiveIntervalsConstraintMinimum(c) - history;
    if( count > KheLimitActiveIntervalsConstraintTimeGroupCount(c) )
      count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
    for(i=0; i < KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c); i++)
    {
      offset = KheLimitActiveIntervalsConstraintAppliesToOffset(c, i);
      for( j = 0;  j < count;  j++ )
      {
	tg = KheLimitActiveIntervalsConstraintTimeGroup(c, 0, offset, &po);
	if( po == KHE_NEGATIVE )
	  KheSolnTimeGroupUnion(soln, tg);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LimitActiveIntervalsConstraintAddMustBeBusyTimes(                   */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, KHE_SOLN soln)*/
/*                                                                           */
/*  Add the times that c indicates must be busy to the time group            */
/*  currently under construction in soln.                                    */
/*                                                                           */
/*****************************************************************************/

static void LimitActiveIntervalsConstraintAddMustBeBusyTimes(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, KHE_SOLN soln)
{
  int i, j, offset, history, count;  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* must be busy because under minimum limit of busy times */
  history = KheLimitActiveIntervalsConstraintHistory(c, r);
  if( history > 0 && history < KheLimitActiveIntervalsConstraintMinimum(c) )
  {
    count = KheLimitActiveIntervalsConstraintMinimum(c) - history;
    if( count > KheLimitActiveIntervalsConstraintTimeGroupCount(c) )
      count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
    for(i=0; i < KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c); i++)
    {
      offset = KheLimitActiveIntervalsConstraintAppliesToOffset(c, i);
      for( j = 0;  j < count;  j++ )
      {
	tg = KheLimitActiveIntervalsConstraintTimeGroup(c, j, offset, &po);
	if( po == KHE_POSITIVE )
	  KheSolnTimeGroupUnion(soln, tg);
      }
    }
  }

  /* must be busy because at or over maximum of free times */
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  history = KheLimitActiveIntervalsConstraintHistory(c, r);
  if( history >= KheLimitActiveIntervalsConstraintMaximum(c) && count > 0 )
  {
    for(i=0; i < KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c); i++)
    {
      offset = KheLimitActiveIntervalsConstraintAppliesToOffset(c, i);
      tg = KheLimitActiveIntervalsConstraintTimeGroup(c, 0, offset, &po);
      if( po == KHE_NEGATIVE )
	KheSolnTimeGroupUnion(soln, tg);
    }
  }

}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP ResourceInfoUnavailableTimes(KHE_RESOURCE r,              */
/*    KHE_SOLN soln, bool hard)                                              */
/*                                                                           */
/*  Return a new time group containing the times that r is unavailable       */
/*  These are hard or soft unavailable times depending on hard.              */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP ResourceInfoUnavailableTimes(KHE_RESOURCE r,
  KHE_SOLN soln, bool hard)
{
  KHE_CONSTRAINT c;  int i;
  KheSolnTimeGroupBegin(soln);
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    if( (KheConstraintRequired(c) == hard) && KheConstraintWeight(c) > 0 )
      switch( KheConstraintTag(c) )
      {
	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:

	  AvoidUnavailableTimesConstraintAddUnavailableTimes(
	    (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, soln);
	  break;

	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

	  ClusterBusyTimesConstraintAddUnavailableTimes(
	    (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, soln);
	  break;

	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

	  LimitBusyTimesConstraintAddUnavailableTimes(
	    (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, soln);
	  break;

	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

          LimitActiveIntervalsConstraintAddUnavailableTimes(
            (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, r, soln);
	  break;

	default:

	  /* nothing to do in these other cases */
	  break;
      }
  }
  return KheSolnTimeGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP ResourceInfoMustBeBusyTimes(KHE_RESOURCE r,               */
/*    KHE_SOLN soln, bool hard)                                              */
/*                                                                           */
/*  Return a new time group containing the times that r must be busy.        */
/*  These are hard or soft must be busy times depending on hard.             */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP ResourceInfoMustBeBusyTimes(KHE_RESOURCE r,
  KHE_SOLN soln, bool hard)
{
  KHE_CONSTRAINT c;  int i;
  KheSolnTimeGroupBegin(soln);
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    if( (KheConstraintRequired(c) == hard) && KheConstraintWeight(c) > 0 )
      switch( KheConstraintTag(c) )
      {
	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

          LimitActiveIntervalsConstraintAddMustBeBusyTimes(
            (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, r, soln);
	  break;

	default:

	  /* nothing to do in these other cases */
	  break;
      }
  }
  return KheSolnTimeGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  RESOURCE_INFO ResourceInfoMake(KHE_RESOURCE r, KHE_SOLN soln, HA_ARENA a)*/
/*                                                                           */
/*  Make a resource info object for resource r.                              */
/*                                                                           */
/*****************************************************************************/

RESOURCE_INFO ResourceInfoMake(KHE_RESOURCE r, KHE_SOLN soln, HA_ARENA a)
{
  RESOURCE_INFO res;
  HaMake(res, a);
  res->resource = r;
  res->hard_unavailable_tg = ResourceInfoUnavailableTimes(r, soln, true);
  res->soft_unavailable_tg = ResourceInfoUnavailableTimes(r, soln, false);
  res->hard_must_be_busy_tg = ResourceInfoMustBeBusyTimes(r, soln, true);
  res->soft_must_be_busy_tg = ResourceInfoMustBeBusyTimes(r, soln, false);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "subspaces"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  SUBSPACE_INTERNAL SubSpaceInternalMake(SPACE encl_space)                 */
/*                                                                           */
/*  Make and return a new internal subspace.                                 */
/*                                                                           */
/*****************************************************************************/

static SUBSPACE_INTERNAL SubSpaceInternalMake(SPACE encl_space)
{
  SUBSPACE_INTERNAL res;  HA_ARENA a;
  a = encl_space->arena;
  HaMake(res, a);
  res->subspace_type = SUBSPACE_TYPE_INTERNAL;
  res->encl_space = encl_space;
  res->total_durn = 0;
  res->total_workload = 0.0;
  HaArrayInit(res->subspaces, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SUBSPACE_EXTERNAL SubSpaceExternalMake(SUBSPACE_COND c, SPACE encl_space)*/
/*                                                                           */
/*  Make and return a new external subspace with these attributes.           */
/*                                                                           */
/*****************************************************************************/

static SUBSPACE_EXTERNAL SubSpaceExternalMake(SUBSPACE_COND c, SPACE encl_space)
{
  SUBSPACE_EXTERNAL res;  HA_ARENA a;
  a = encl_space->arena;
  HaMake(res, a);
  res->subspace_type = SUBSPACE_TYPE_EXTERNAL;
  res->encl_space = encl_space;
  res->condition = c;
  HaArrayInit(res->tasks, a);
  res->spanned_over = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SUBSPACE SubSpaceBuild(SPACE space, int index,                           */
/*    KHE_TIME_GROUP outer_allow_tg, KHE_RESOURCE_GROUP outer_allow_rg,      */
/*    bool outer_allow_runassigned)                                          */
/*                                                                           */
/*  Build and return a subspace for everything from index onwards.  Tasks    */
/*  enter this subspace when they are assigned times from outer_allow_tg     */
/*  and resources from outer_allow_rg and outer_allow_runassigned.           */
/*                                                                           */
/*****************************************************************************/

static SUBSPACE SubSpaceBuild(SPACE space, int index, SUBSPACE_COND c)
{
  SPACE_DIM dim;  SUBSPACE_INTERNAL si;  int i;  SUBSPACE_COND inner_c;
  if( index >= HaArrayCount(space->dimensions) )
  {
    /* external subspace */
    return (SUBSPACE) SubSpaceExternalMake(c, space);
  }
  else
  {
    /* internal subspace */
    si = SubSpaceInternalMake(space);
    dim = HaArray(space->dimensions, index);
    for( i = 0;  i < dim->count;  i++ )
    {
      inner_c = SpaceDimRestrictCondition(dim, i, c);
      HaArrayAddLast(si->subspaces, SubSpaceBuild(space, index + 1, inner_c));
    }
    return (SUBSPACE) si;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SubSpaceInsertTask(SUBSPACE s, KHE_TASK task, KHE_TIME t,           */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Insert task into s at time t for resource r.                             */
/*                                                                           */
/*****************************************************************************/
/* static bool SpaceShowTask(SPACE space, KHE_TASK task, HTML_FONT *font); */

static void SubSpaceInsertTask(SUBSPACE s, KHE_TASK task, int durn,
  float workload, KHE_TIME t, KHE_RESOURCE r, SPACE space, int depth)
{
  SUBSPACE_EXTERNAL se;  SUBSPACE_INTERNAL si;  SUBSPACE ss;
  SPACE_DIM dim;  int index;
  if( DEBUG2 )
    fprintf(stderr, "[ SubSpaceInsertTask\n");
  if( s->subspace_type == SUBSPACE_TYPE_EXTERNAL )
  {
    /* external subspace, add task */
    se = (SUBSPACE_EXTERNAL) s;
    HaArrayAddLast(se->tasks, task);
  }
  else
  {
    /* internal subspace, recurse on a single child subspace */
    si = (SUBSPACE_INTERNAL) s;
    si->total_durn += durn;
    si->total_workload += workload;
    HnAssert(depth < HaArrayCount(space->dimensions),
      "SubSpaceInsertTask internal error 1");
    dim = HaArray(space->dimensions, depth);
    if( SpaceDimIndex(dim, t, task, r, &index) )
    {
      HnAssert(index < HaArrayCount(si->subspaces),
	"SubSpaceInsertTask internal error 2");
      ss = HaArray(si->subspaces, index);
      SubSpaceInsertTask(ss, task, durn, workload, t, r, space, depth + 1);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] SubSpaceInsertTask\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool SubSpaceUnavailable(SUBSPACE_EXTERNAL se, SPACE_CELL_COND cc)       */
/*                                                                           */
/*  Return true if se satisfies hard or soft unavailable cell condition cc.  */
/*                                                                           */
/*****************************************************************************/

static bool SubSpaceUnavailable(SUBSPACE_EXTERNAL se, SPACE_CELL_COND cc)
{
  RESOURCE_INFO ri;  int i, index;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;
  KHE_RESOURCE_GROUP rg;
  rg = se->condition.resource_group;
  tg = se->condition.time_group;
  if( KheResourceGroupResourceCount(rg) == 0 )
    return false;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    index = KheResourceInstanceIndex(r);
    ri = HaArray(se->encl_space->resource_info, index);
    if( ri == NULL )
    {
      ri = ResourceInfoMake(r, se->encl_space->soln, se->encl_space->arena);
      HaArrayPut(se->encl_space->resource_info, index, ri);
    }
    switch( cc )
    {
      case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD:

	if( !KheTimeGroupSubset(tg, ri->hard_unavailable_tg) )
	  return false;
	break;

      case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_SOFT:

	if( !KheTimeGroupSubset(tg, ri->soft_unavailable_tg) )
	  return false;
	break;

      case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_PART:

	if( KheTimeGroupDisjoint(tg, ri->soft_unavailable_tg) &&
	    KheTimeGroupDisjoint(tg, ri->hard_unavailable_tg) )
	  return false;
	break;

      default:

	HnAbort("SubSpaceUnavailable internal error (%d)", (int) cc);
	break;
    }
    /* ***
    tg = (cc == SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD ?
      ri->hard_unavailable_tg : ri->soft_unavailable_tg);
    if( !KheTimeGroupSubset(se->condition.time_group, tg) )
      return false;
    *** */
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SubSpaceMustBeBusy(SUBSPACE_EXTERNAL se, SPACE_CELL_COND cc)        */
/*                                                                           */
/*  Return true if se satisfies hard or soft must be busy cell condition cc. */
/*                                                                           */
/*****************************************************************************/

static bool SubSpaceMustBeBusy(SUBSPACE_EXTERNAL se, SPACE_CELL_COND cc)
{
  RESOURCE_INFO ri;  int i, index;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;
  KHE_RESOURCE_GROUP rg;
  rg = se->condition.resource_group;
  tg = se->condition.time_group;
  if( KheResourceGroupResourceCount(rg) == 0 )
    return false;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    index = KheResourceInstanceIndex(r);
    ri = HaArray(se->encl_space->resource_info, index);
    if( ri == NULL )
    {
      ri = ResourceInfoMake(r, se->encl_space->soln, se->encl_space->arena);
      HaArrayPut(se->encl_space->resource_info, index, ri);
    }
    switch( cc )
    {
      case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_HARD:

	if( !KheTimeGroupDisjoint(tg, ri->hard_must_be_busy_tg) )
	  return true;
	break;

      case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_SOFT:

	if( !KheTimeGroupDisjoint(tg, ri->soft_must_be_busy_tg) )
	  return true;
	break;

      /* *** withdrawn, not sure what it means
      case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_PART:

	if( KheTimeGroupDisjoint(tg, ri->soft_must_be_busy_tg) &&
	    KheTimeGroupDisjoint(tg, ri->hard_must_be_busy_tg) )
	  return false;
	break;
      *** */

      default:

	HnAbort("SubSpaceMustBeBusy internal error (%d)", (int) cc);
	break;
    }
    /* ***
    tg = (cc == SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD ?
      ri->hard_unavailable_tg : ri->soft_unavailable_tg);
    if( !KheTimeGroupSubset(se->condition.time_group, tg) )
      return false;
    *** */
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool TaskIsDefective(KHE_TASK task)                                      */
/*                                                                           */
/*  Return true if task is monitored by a monitor of non-zero cost.          */
/*                                                                           */
/*****************************************************************************/

static bool TaskIsDefective(KHE_TASK task)
{
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_MONITOR m;  int i;
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( KheMonitorCost(m) > 0 )
	return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CheckClusterBusyCondition(KHE_CLUSTER_BUSY_TIMES_MONITOR m,         */
/*    char *cstr, SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)              */
/*                                                                           */
/*  Return true if m satisfies the given constraint condition.               */
/*                                                                           */
/*****************************************************************************/

static bool CheckClusterBusyCondition(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)
{
  /* KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c; */
  KHE_POLARITY po;  KHE_TIME_GROUP tg2;  bool doit, allow_zero;
  int i, active_group_count, open_group_count, minimum, maximum;

  /* don't do anything that would highlight the entire cycle */
  /* ***
  c = KheClusterBusyTimesMonitorConstraint(m);
  if( KheClusterBusyTimesConstraintTimeGroupsCoverWholeCycle(c) )
    return false;
  *** */

  /* decide whether highlighting is wanted for this cell_cond */
  switch( cell_cond )
  {
    case SPACE_CELL_CONSTRAINT_VIOLATED:

      doit = (KheMonitorCost((KHE_MONITOR) m) > 0);
      break;

    case SPACE_CELL_CONSTRAINT_UNVIOLATED:

      doit = (KheMonitorCost((KHE_MONITOR) m) == 0);
      break;

    case SPACE_CELL_CONSTRAINT_SLACK:

      KheClusterBusyTimesMonitorActiveTimeGroupCount(m, &active_group_count,
	&open_group_count, &minimum, &maximum, &allow_zero);
      doit = (active_group_count < maximum);
      break;

    default:

      HnAbort("CheckClusterBusyCondition internal error: %d", cell_cond);
      doit = false;  /* keep compiler happy */
  }

  /* if we are highlighting, all time groups are in it */
  if( doit )
  {
    for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
    {
      tg2 = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
      if( !KheTimeGroupDisjoint(tg2, tg) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CheckLimitBusyCondition(KHE_LIMIT_BUSY_TIMES_MONITOR m,             */
/*    char *cstr, SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)              */
/*                                                                           */
/*  Return true if m satisfies the given constraint condition.               */
/*                                                                           */
/*****************************************************************************/

static bool CheckLimitBusyCondition(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)
{
  int i, busy_count, minimum, maximum, junk /* , count */, open_count;
  bool allow_zero;  KHE_TIME_GROUP tg2;
  /* count = KheInstanceTimeCount(KheTimeGroupInstance(tg)); */
  switch( cell_cond )
  {
    case SPACE_CELL_CONSTRAINT_VIOLATED:

      /* search through the defective time groups */
      for( i = 0;  i < KheLimitBusyTimesMonitorDefectiveTimeGroupCount(m); i++ )
      {
	KheLimitBusyTimesMonitorDefectiveTimeGroup(m, i, &tg2, &busy_count,
	  &open_count, &minimum, &maximum, &allow_zero);
	if( /* KheTimeGroupTimeCount(tg2) < count && */
	    !KheTimeGroupDisjoint(tg2, tg) )
	  return true;
      }
      return false;

    case SPACE_CELL_CONSTRAINT_UNVIOLATED:

      /* search through all time groups */
      for( i = 0;  i < KheLimitBusyTimesMonitorTimeGroupCount(m);  i++ )
      {
	tg2 = KheLimitBusyTimesMonitorTimeGroup(m, i, &junk);
	if( /* KheTimeGroupTimeCount(tg2) < count && */
	    !KheTimeGroupDisjoint(tg2, tg) )
	  return true;
      }
      return false;

    case SPACE_CELL_CONSTRAINT_SLACK:

      /* not implementing this one at present */
      return false;

    default:

      HnAbort("CheckLimitBusyCondition internal error: %d", cell_cond);
      return false;

  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool CheckLimitWorkloadCondition(KHE_LIMIT_WORKLOAD_MONITOR m,           */
/*    char *cstr, SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)              */
/*                                                                           */
/*  Return true if m satisfies the given constraint condition.               */
/*                                                                           */
/*****************************************************************************/

static bool CheckLimitWorkloadCondition(KHE_LIMIT_WORKLOAD_MONITOR m,
  SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)
{
  /* don't worry about these, because in practice they cover the whole cycle */
  /* and anyway they are covered by the "Avail" column */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CheckLimitActiveCondition(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,     */
/*    char *cstr, SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)              */
/*                                                                           */
/*  Return true if m satisfies the given constraint condition.               */
/*                                                                           */
/*****************************************************************************/

static bool CheckLimitActiveCondition(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  SPACE_CELL_COND cell_cond, KHE_TIME_GROUP tg)
{
  int i, j, history_before, first_index, last_index, history_after;
  bool too_long;  KHE_POLARITY po;  KHE_TIME_GROUP tg2;
  if( cell_cond == SPACE_CELL_CONSTRAINT_VIOLATED )
  {
    for( i=0; i < KheLimitActiveIntervalsMonitorDefectiveIntervalCount(m); i++ )
    {
      KheLimitActiveIntervalsMonitorDefectiveInterval(m, i, &history_before,
        &first_index, &last_index, &history_after, &too_long);
      for( j = first_index;  j <= last_index;  j++ )
      {
	tg2 = KheLimitActiveIntervalsMonitorTimeGroup(m, j, &po);
        if( !KheTimeGroupDisjoint(tg2, tg) )
	  return true;
      }
    }
    return false;
  }
  else
  {
    /* never show anything for unviolated or slack, because in practice */
    /* these constraints cover the whole cycle and there is no value in it */
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool CheckConstraintsCondition(KHE_SOLN soln, char *constraints_str,     */
/*    SPACE_CELL_COND cell_cond, KHE_RESOURCE_GROUP rg, KHE_TIME_GROUP tg)   */
/*                                                                           */
/*  Return true if a constraints condition holds with these attributes.      */
/*                                                                           */
/*  This function assumes that constraints_str is non-NULL and non-empty.    */
/*                                                                           */
/*****************************************************************************/

static bool CheckConstraintsCondition(KHE_SOLN soln, char *constraints_str,
  SPACE_CELL_COND cell_cond, KHE_RESOURCE_GROUP rg, KHE_TIME_GROUP tg)
{
  int i, j;  KHE_RESOURCE r;  KHE_MONITOR m;  KHE_CONSTRAINT c;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      c = KheMonitorConstraint(m);
      HnAssert(c != NULL, "CheckConstraintsCondition internal error (m = %d)",
	KheMonitorTag(m));
      if( strstr(KheConstraintName(c), constraints_str) != NULL ||
          strstr(KheConstraintId(c), constraints_str) != NULL )
	switch( KheMonitorTag(m) )
	{
	  case KHE_AVOID_CLASHES_MONITOR_TAG:
	  case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
	  case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

	    /* nothing to do here */
	    break;

	  case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	    if( CheckClusterBusyCondition((KHE_CLUSTER_BUSY_TIMES_MONITOR) m,
		cell_cond, tg) )
	      return true;
	    break;

	  case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	    if( CheckLimitBusyCondition((KHE_LIMIT_BUSY_TIMES_MONITOR) m,
		cell_cond, tg) )
	      return true;
	    break;

	  case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	    if( CheckLimitWorkloadCondition((KHE_LIMIT_WORKLOAD_MONITOR) m,
		cell_cond, tg) )
	    break;

	  case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	    if( CheckLimitActiveCondition((KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)m,
		cell_cond, tg) )
	      return true;
	    break;

	  default:

	    /* nothing to do here */
	    break;

	}
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SubSpaceCellSelected(SUBSPACE_EXTERNAL se, COLOUR_SELECT cs)        */
/*                                                                           */
/*  Return true if cs selects se.                                            */
/*                                                                           */
/*****************************************************************************/

static bool SubSpaceCellSelected(SUBSPACE_EXTERNAL se, COLOUR_SELECT cs)
{
  KHE_TASK task;  int i;  bool needs_asst, assigned;  KHE_RESOURCE junk;
  switch( cs.cell_cond )
  {
    case SPACE_CELL_TIME_GROUP_EMPTY:

      /* select when cell represents no times */
      return KheTimeGroupTimeCount(se->condition.time_group) == 0;

    case SPACE_CELL_TIME_GROUP_NON_EMPTY:

      /* select when cell represents at least one time */
      return KheTimeGroupTimeCount(se->condition.time_group) > 0;

    case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD:
    case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_SOFT:
    case SPACE_CELL_RESOURCE_GROUP_UNAVAIL_PART:

      /* select when cell represents resources at unavailable times */
      return SubSpaceUnavailable(se, cs.cell_cond);

    case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_HARD:
    case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_SOFT:
    /* case SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_PART: */

      /* select when cell represents resources at must-be-busy times */
      return SubSpaceMustBeBusy(se, cs.cell_cond);

    case SPACE_CELL_RESOURCE_UNASSIGNED:

      /* select when unassigned tasks may appear in this cell */
      return se->condition.runassigned;

    case SPACE_CELL_TASKS_NONE:

      /* select when cell contains no tasks */
      return HaArrayCount(se->tasks) == 0;

    case SPACE_CELL_TASKS_ONE_OR_MORE:

      /* select when cell contains at least one task */
      return HaArrayCount(se->tasks) >= 1;

    case SPACE_CELL_TASKS_TWO_OR_MORE:

      /* select when cell contains at least two tasks */
      return HaArrayCount(se->tasks) >= 2;

    case SPACE_CELL_HAS_DEFECTIVE_TASK:

      /* select when cell contains at least one defective task */
      HaArrayForEach(se->tasks, task, i)
	if( TaskIsDefective(task) )
	  return true;
      return false;

    case SPACE_CELL_CONSTRAINT_VIOLATED:
    case SPACE_CELL_CONSTRAINT_UNVIOLATED:
    case SPACE_CELL_CONSTRAINT_SLACK:

      /* select time groups relevant to constraints, violated or unviolated */
      if( se->condition.runassigned || cs.constraints_str == NULL ||
	  cs.constraints_str[0] == '\0' )
	return false;
      else
	return CheckConstraintsCondition(se->encl_space->soln,
	  cs.constraints_str, cs.cell_cond,
	  se->condition.resource_group, se->condition.time_group);

    case SPACE_CELL_UNREQUIRED_ASSIGNED:

      HaArrayForEach(se->tasks, task, i)
      {
	needs_asst = KheTaskNeedsAssignment(task) ||
	  KheTaskIsPreassigned(task, &junk);
	assigned = (KheTaskAsstResource(task) != NULL);
	if( !needs_asst && assigned )
	  return true;
      }
      return false;

    default:

      HnAbort("SubSpaceCellSelected internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool SubSpaceTaskSelected(KHE_TASK task, bool required, bool assigned,   */
/*    SPACE_TASK_COND tc)                                                    */
/*                                                                           */
/*  Return true if tc selects se.                                            */
/*                                                                           */
/*****************************************************************************/

static bool SubSpaceTaskSelected(KHE_TASK task, bool required, bool assigned,
  SPACE_TASK_COND tc)
{
  if( DEBUG2 )
    fprintf(stderr, "[ SubSpaceTaskSelected(%s)\n", task == NULL ? "-" : "task");
  switch( tc )
  {
    case SPACE_TASK_UNREQUIRED_UNASSIGNED:

      /* select when task is unrequired and unassigned */
      if( DEBUG2 )
	fprintf(stderr, "] SubSpaceTaskSelected\n");
      return !required && !assigned;

    case SPACE_TASK_UNREQUIRED_ASSIGNED:

      /* select when task is unrequired and assigned */
      if( DEBUG2 )
	fprintf(stderr, "] SubSpaceTaskSelected\n");
      return !required && assigned;

    case SPACE_TASK_REQUIRED_UNASSIGNED:

      /* select when task is required and unassigned */
      if( DEBUG2 )
	fprintf(stderr, "] SubSpaceTaskSelected\n");
      return required && !assigned;

    case SPACE_TASK_REQUIRED_ASSIGNED:

      /* select when task is required and assigned */
      if( DEBUG2 )
	fprintf(stderr, "] SubSpaceTaskSelected\n");
      return required && assigned;

    case SPACE_TASK_DEFECTIVE:

      /* select when task is defective */
      if( DEBUG2 )
	fprintf(stderr, "  SubSpaceTaskSelected calling TaskIsDefective\n");
      return TaskIsDefective(task);

    default:

      HnAbort("SubSpaceTaskSelected internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *SubSpaceColour(SUBSPACE_EXTERNAL se)                               */
/*                                                                           */
/*  Return the colour of se.                                                 */
/*                                                                           */
/*****************************************************************************/

static char *SubSpaceColour(SUBSPACE_EXTERNAL se)
{
  COLOUR_SELECT cs;  int i;  char *res;  KHE_TASK task;  KHE_EVENT e;
  HaArrayForEach(se->encl_space->colour_selects, cs, i)
    if( SubSpaceCellSelected(se, cs) )
    {
      res = cs.colour;
      if( res == NULL )
      {
	/* NULL means use colour of first event, or default colour if none */
	if( HaArrayCount(se->tasks) > 0 )
	{
	  task = HaArrayFirst(se->tasks);
	  e = KheMeetEvent(KheTaskMeet(task));
	  res = KheEventColor(e);
	}
	if( res == NULL )
	  res = se->encl_space->dft_colour;
      }
      return res;
    }
  return se->encl_space->dft_colour;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SpaceShowTask(SPACE space, KHE_TASK task, HTML_FONT *font)          */
/*                                                                           */
/*  If task is to be shown in space, return true and set *font to its font.  */
/*                                                                           */
/*****************************************************************************/

static bool SpaceShowTask(SPACE space, KHE_TASK task, HTML_FONT *font)
{
  FONT_SELECT fs;  int i;  bool needs_asst, assigned;  KHE_RESOURCE junk;
  if( DEBUG2 )
    fprintf(stderr, "[ SpaceShowTask\n");
  if( DEBUG2 )
    fprintf(stderr, "  SpaceShowTask calling KheTaskNeedsAssignment\n");
  needs_asst = KheTaskNeedsAssignment(task) ||
    KheTaskIsPreassigned(task, &junk);
  if( DEBUG2 )
    fprintf(stderr, "  SpaceShowTask calling KheTaskAsstResource\n");
  assigned = (KheTaskAsstResource(task) != NULL);
  HaArrayForEach(space->font_selects, fs, i)
    if( SubSpaceTaskSelected(task, needs_asst, assigned, fs.task_cond) )
    {
      if( DEBUG2 )
	fprintf(stderr, "] SpaceShowTask\n");
      return *font = fs.font, fs.show;
    }
  if( DEBUG2 )
    fprintf(stderr, "] SpaceShowTask\n");
  return *font = space->dft_font, space->dft_show;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "weeks"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  WEEK WeekMake(KHE_TIME_GROUP week_tg, HA_ARENA a)                        */
/*                                                                           */
/*  Make a new week object holding week_tg, but initially no days.           */
/*                                                                           */
/*****************************************************************************/

static WEEK WeekMake(KHE_TIME_GROUP week_tg, HA_ARENA a)
{
  WEEK res;
  HaMake(res, a);
  res->week_tg = week_tg;
  HaArrayInit(res->days, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWeekTypedCmp(WEEK week1, WEEK week2)                              */
/*                                                                           */
/*  Typed comparison function for sorting weeks.                             */
/*                                                                           */
/*****************************************************************************/

static int KheWeekTypedCmp(WEEK week1, WEEK week2)
{
  return KheTimeGroupTypedCmp(week1->week_tg, week2->week_tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWeekCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Untyped comparison function for sorting weeks.                           */
/*                                                                           */
/*****************************************************************************/

static int KheWeekCmp(const void *t1, const void *t2)
{
  WEEK week1, week2;
  week1 = * (WEEK *) t1;
  week2 = * (WEEK *) t2;
  return KheWeekTypedCmp(week1, week2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "space"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *GetDayLabel(KHE_TIME_GROUP day_tg)                                 */
/*                                                                           */
/*  Produce a suitable label for a generic day of the week, based on the     */
/*  name of day_tg.                                                          */
/*                                                                           */
/*****************************************************************************/
#define in_range(x, a, b) ((x) >= (a) && (x) <= (b))
#define is_letter(ch) (in_range(ch, 'a', 'z') || in_range(ch, 'A', 'Z'))
#define is_digit(ch) (in_range(ch, '0', '9'))

static char *GetDayLabel(KHE_TIME_GROUP day_tg, int index, HA_ARENA a)
{
  char buff[100], *str;  int len, i, j;
  str = KheTimeGroupName(day_tg);
  len = strlen(str);
  if( len < 100 )
  {
    strcpy(buff, str);
    for( i = 0;  buff[i] != '\0';  i++ )
      if( is_letter(buff[i]) )
	break;
    if( buff[i] != '\0' )
    {
      for( j = len - 1;  j >= 0;  j-- )
	if( is_letter(buff[j]) )
	  break;

      /* now buff[i .. j] begins and ends with a letter */
      buff[j+1] = '\0';
      return HnStringMake(a, "%s", &buff[i]);
    }
  }

  /* if we get to here, we haven't been able to make the name we want */
  return HnStringMake(a, "Day %d", index);
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_ERROR SpaceErrorMake(SPACE_ERROR_TYPE error_type,                  */
/*    KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_TIME_GROUP tg3, HA_ARENA a)*/
/*                                                                           */
/*  Make a new error object with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static SPACE_ERROR SpaceErrorMake(SPACE_ERROR_TYPE error_type,
  KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_TIME_GROUP tg3, HA_ARENA a)
{
  SPACE_ERROR res;
  HaMake(res, a);
  res->error_type = error_type;
  res->time_group1 = tg1;
  res->time_group2 = tg2;
  res->time_group3 = tg3;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceErrorAdd(SPACE space, SPACE_ERROR_TYPE error_type,             */
/*    KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_TIME_GROUP tg3)            */
/*                                                                           */
/*  Add an error object with these attributes to space.                      */
/*                                                                           */
/*****************************************************************************/

static void SpaceErrorAdd(SPACE space, SPACE_ERROR_TYPE error_type,
  KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_TIME_GROUP tg3)
{
  HaArrayAddLast(space->errors,
    SpaceErrorMake(error_type, tg1, tg2, tg3, space->arena));
}


/*****************************************************************************/
/*                                                                           */
/*  int SpaceErrorCount(SPACE space)                                         */
/*                                                                           */
/*  Return the number of day and week errors in space.                       */
/*                                                                           */
/*****************************************************************************/

int SpaceErrorCount(SPACE space)
{
  return HaArrayCount(space->errors);
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_ERROR_TYPE SpaceError(SPACE space, int i,                          */
/*    KHE_TIME_GROUP *tg1, KHE_TIME_GROUP *tg2, KHE_TIME_GROUP *tg3)         */
/*                                                                           */
/*  Return the i'th day or week error of space.                              */
/*                                                                           */
/*****************************************************************************/

SPACE_ERROR_TYPE SpaceError(SPACE space, int i,
  KHE_TIME_GROUP *tg1, KHE_TIME_GROUP *tg2, KHE_TIME_GROUP *tg3)
{
  SPACE_ERROR se;
  se = HaArray(space->errors, i);
  *tg1 = se->time_group1;
  *tg2 = se->time_group2;
  *tg3 = se->time_group3;
  return se->error_type;
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE SpaceMake(KHE_SOLN soln, bool allow_spanning, char *dft_colour,    */
/*    bool dft_show, HTML_FONT dft_font)                                     */
/*                                                                           */
/*  Make a new space for soln, initially with no dimensions.                 */
/*                                                                           */
/*****************************************************************************/

SPACE SpaceMake(KHE_SOLN soln, bool allow_spanning, char *dft_colour,
  bool dft_show, HTML_FONT dft_font)
{
  SPACE res;  KHE_INSTANCE ins;  WEEK week, first_week;  HA_ARENA a;
  KHE_TIME_GROUP tg, tg2, day_tg, prev_non_disjoint_week_tg;
  int i, j, count, days_total_times, weeks_total_times;

  /* make and initialize the object */
  a = KheSolnArenaBegin(soln);
  ins = KheSolnInstance(soln);
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->allow_spanning = allow_spanning;
  res->dft_colour = dft_colour;
  res->dft_show = dft_show;
  res->dft_font = dft_font;
  HaArrayInit(res->days, a);
  HaArrayInit(res->weeks, a);
  res->max_times_per_day = 0;
  HaArrayInit(res->days_of_week, a);
  HaArrayInit(res->days_of_week_labels, a);
  HaArrayInit(res->errors, a);
  HaArrayInit(res->resource_info, a);
  count = KheInstanceResourceCount(ins);
  HaArrayFill(res->resource_info, count, NULL);
  HaArrayInit(res->colour_selects, a);
  HaArrayInit(res->font_selects, a);
  HaArrayInit(res->dimensions, a);
  res->root_subspace = NULL;  /* signifies subspaces not built yet */

  /* get days, weeks, and max_times_per_day */
  days_total_times = 0;
  weeks_total_times = 0;
  for( i = 0;  i < KheInstanceTimeGroupCount(ins);  i++ )
  {
    tg = KheInstanceTimeGroup(ins, i);
    if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_DAY )
    {
      if( KheTimeGroupTimeCount(tg) > 0 )
      {
	HaArrayForEach(res->days, tg2, j)
	  if( !KheTimeGroupDisjoint(tg, tg2) )
            SpaceErrorAdd(res, SPACE_ERROR_DAYS_NOT_DISJOINT, tg, tg2, NULL);
	HaArrayAddLast(res->days, tg);
	days_total_times += KheTimeGroupTimeCount(tg);
	if( KheTimeGroupTimeCount(tg) > res->max_times_per_day )
	  res->max_times_per_day = KheTimeGroupTimeCount(tg);
      }
      else
        SpaceErrorAdd(res, SPACE_ERROR_DAY_IS_EMPTY, tg, NULL, NULL);
    }
    else if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_WEEK )
    {
      if( KheTimeGroupTimeCount(tg) > 0 )
      {
	HaArrayForEach(res->weeks, week, j)
	  if( !KheTimeGroupDisjoint(tg, week->week_tg) )
            SpaceErrorAdd(res, SPACE_ERROR_WEEKS_NOT_DISJOINT,
	      tg, week->week_tg, NULL);
	HaArrayAddLast(res->weeks, WeekMake(tg, res->arena));
	weeks_total_times += KheTimeGroupTimeCount(tg);
      }
      else
        SpaceErrorAdd(res, SPACE_ERROR_WEEK_IS_EMPTY, tg, NULL, NULL);
    }
  }
  HaArraySort(res->days, &KheTimeGroupCmp);
  if( HaArrayCount(res->days) > 0 &&
      days_total_times < KheInstanceTimeCount(ins) )
    SpaceErrorAdd(res, SPACE_ERROR_DAYS_DO_NOT_COVER_CYCLE, NULL, NULL, NULL);
  HaArraySort(res->weeks, &KheWeekCmp);
  if( HaArrayCount(res->weeks) > 0 &&
      weeks_total_times < KheInstanceTimeCount(ins) )
    SpaceErrorAdd(res, SPACE_ERROR_WEEKS_DO_NOT_COVER_CYCLE, NULL, NULL, NULL);

  if( HaArrayCount(res->days) > 0 && HaArrayCount(res->weeks) > 0 )
  {
    /* work out which days go with which weeks */
    HaArrayForEach(res->days, day_tg, i)
    {
      prev_non_disjoint_week_tg = NULL;
      HaArrayForEach(res->weeks, week, j)
	if( !KheTimeGroupDisjoint(day_tg, week->week_tg) )
	{
	  if( prev_non_disjoint_week_tg != NULL )
            SpaceErrorAdd(res, SPACE_ERROR_DAY_INTERSECTS_TWO_WEEKS,
              day_tg, prev_non_disjoint_week_tg, week->week_tg);
	  prev_non_disjoint_week_tg = week->week_tg;
	  if( KheTimeGroupSubset(day_tg, week->week_tg) )
	    HaArrayAddLast(week->days, day_tg);
	}
    }

    /* define one day of week time group for each day of the first week */
    first_week = HaArrayFirst(res->weeks);
    HaArrayForEach(first_week->days, day_tg, i)
    {
      KheSolnTimeGroupBegin(res->soln);
      HaArrayForEach(res->weeks, week, j)
	if( i < HaArrayCount(week->days) )
	{
	  tg = HaArray(week->days, i);
	  KheSolnTimeGroupUnion(res->soln, tg);
	}
      HaArrayAddLast(res->days_of_week, KheSolnTimeGroupEnd(res->soln));
      HaArrayAddLast(res->days_of_week_labels,
        GetDayLabel(day_tg, i, res->arena));
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SpaceHasDays(SPACE space)                                           */
/*                                                                           */
/*  Return true if space has Day time groups.                                */
/*                                                                           */
/*****************************************************************************/

bool SpaceHasDays(SPACE space)
{
  return HaArrayCount(space->days) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SpaceHasWeeks(SPACE space)                                          */
/*                                                                           */
/*  Return true if space has Week time groups.                               */
/*                                                                           */
/*****************************************************************************/

bool SpaceHasWeeks(SPACE space)
{
  return HaArrayCount(space->weeks) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceDelete(SPACE space)                                            */
/*                                                                           */
/*  Delete space.                                                            */
/*                                                                           */
/*****************************************************************************/

void SpaceDelete(SPACE space)
{
  KheSolnArenaEnd(space->soln, space->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_TIME_DIM SpaceAddTimeDim(SPACE space)                              */
/*                                                                           */
/*  Add a new time dimension to space and return it.                         */
/*                                                                           */
/*****************************************************************************/

SPACE_TIME_DIM SpaceAddTimeDim(SPACE space)
{
  SPACE_TIME_DIM res;
  HnAssert(space->root_subspace == NULL,
    "SpaceAddTimeDim called out of order (after tasks added)");
  res = SpaceTimeDimMake(space);
  HaArrayAddLast(space->dimensions, (SPACE_DIM) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceTimeDimAddTimeGroup(SPACE_TIME_DIM std,                        */
/*    KHE_TIME_GROUP tg, char *label)                                        */
/*                                                                           */
/*  Add a new time group with the given label to the end of std.             */
/*                                                                           */
/*****************************************************************************/

void SpaceTimeDimAddTimeGroup(SPACE_TIME_DIM std,
  KHE_TIME_GROUP tg, char *label)
{
  HnAssert(std->encl_space->root_subspace == NULL,
    "SpaceTimeDimAddTimeGroup called out of order (after tasks added)");
  HaArrayAddLast(std->time_groups, tg);
  HaArrayAddLast(std->labels, label);
  std->count++;
}


/*****************************************************************************/
/*                                                                           */
/*  SPACE_RESOURCE_DIM SpaceAddResourceDim(SPACE space,                      */
/*    SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,              */
/*    void *avail_rg_impl, SPACE_RG_FN after_rg_fn)                          */
/*                                                                           */
/*  Add a new resource dimension to space and return it.                     */
/*                                                                           */
/*****************************************************************************/

SPACE_RESOURCE_DIM SpaceAddResourceDim(SPACE space,
  SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,
  void *avail_rg_impl, SPACE_RG_FN after_rg_fn)
{
  SPACE_RESOURCE_DIM res;
  HnAssert(space->root_subspace == NULL,
    "SpaceAddResourceDim called out of order (after tasks added)");
  res = SpaceResourceDimMake(space, major_heading_rg_fn, avail_rg_fn,
    avail_rg_impl, after_rg_fn);
  HaArrayAddLast(space->dimensions, (SPACE_DIM) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceResourceDimAddResourceGroup(SPACE_RESOURCE_DIM srd,            */
/*    KHE_RESOURCE_GROUP rg, char *label)                                    */
/*                                                                           */
/*  Add a new resource group with the given label to the end of srd.         */
/*  It is acceptable for rg to be NULL; that matches unassigned tasks.       */
/*                                                                           */
/*****************************************************************************/

void SpaceResourceDimAddResourceGroup(SPACE_RESOURCE_DIM srd,
  KHE_RESOURCE_GROUP rg, char *label)
{
  HnAssert(srd->encl_space->root_subspace == NULL,
    "SpaceResourceDimAddResourceGroup called out of order (after tasks added)");
  HaArrayAddLast(srd->resource_groups, rg);
  HaArrayAddLast(srd->labels, label);
  srd->count++;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "helper functions for frequently used dimensions"              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddResourceTypeDim(SPACE space, KHE_RESOURCE_TYPE rt,          */
/*    SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,              */
/*    void *avail_rg_impl, SPACE_RG_FN after_rg_fn)                          */
/*                                                                           */
/*  Add a resource dimension with one resource for each element of rt.       */
/*                                                                           */
/*****************************************************************************/

void SpaceAddResourceTypeDim(SPACE space, KHE_RESOURCE_TYPE rt,
  SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,
  void *avail_rg_impl, SPACE_RG_FN after_rg_fn)
{
  SPACE_RESOURCE_TYPE_DIM res;

  /* abort if call out of order */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddResourceTypeDim called out of order (after tasks added)");

  /* add a resource type dimension */
  res = SpaceResourceTypeDimMake(rt, space, major_heading_rg_fn,
    avail_rg_fn, avail_rg_impl, after_rg_fn);
  HaArrayAddLast(space->dimensions, (SPACE_DIM) res);
  /* res->count = KheResourceTypeResourceCount(rt); */
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddTimesOfCycleDim(SPACE space)                                */
/*                                                                           */
/*  Add a time dimension with one time group for each time of the cycle.     */
/*                                                                           */
/*****************************************************************************/

void SpaceAddTimesOfCycleDim(SPACE space)
{
  SPACE_TIME_DIM std;  KHE_INSTANCE ins;  KHE_TIME t;  int i;

  /* abort if call out of order */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddTimesOfCycleDim called out of order (after tasks added)");

  /* add a time dimension with one time group for each time of the cycle */
  std = SpaceAddTimeDim(space);
  ins = KheSolnInstance(space->soln);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
    SpaceTimeDimAddTimeGroup(std, KheTimeSingletonTimeGroup(t), KheTimeId(t));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddTimesOfDayDim(SPACE space)                                  */
/*                                                                           */
/*  Add a time dimension with one time group for each time of day.  Abort    */
/*  if the instance has no Day time groups.                                  */
/*                                                                           */
/*****************************************************************************/

void SpaceAddTimesOfDayDim(SPACE space)
{
  SPACE_TIME_DIM std;  int i, j;  KHE_TIME_GROUP tg;  char *label;

  /* abort if call out of order or there are no day time groups */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddTimesOfDayDim called out of order (after tasks added)");
  HnAssert(SpaceHasDays(space), "SpaceAddTimesOfDayDim: space has no days");

  /* add a time dimension with one time group for each time of day */
  std = SpaceAddTimeDim(space);
  for( i = 0;  i < space->max_times_per_day;  i++ )
  {
    /* build a time group containing the i'th time on each day */
    KheSolnTimeGroupBegin(space->soln);
    HaArrayForEach(space->days, tg, j)
      if( i < KheTimeGroupTimeCount(tg) )
	KheSolnTimeGroupAddTime(space->soln, KheTimeGroupTime(tg, i));
    tg = KheSolnTimeGroupEnd(space->soln);
    label = HnStringMake(space->arena, "Time %d", i + 1);
    SpaceTimeDimAddTimeGroup(std, tg, label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddDaysOfWeekDim(SPACE space)                                  */
/*                                                                           */
/*  Add a time dimension with one time group for each day of the week.       */
/*  Abort if the instance has no Day time groups, or no Week time groups.    */
/*                                                                           */
/*****************************************************************************/

void SpaceAddDaysOfWeekDim(SPACE space)
{
  SPACE_TIME_DIM std;  int i;  KHE_TIME_GROUP tg;

  /* abort if call out of order or there are no day time groups */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddDaysOfWeekDim called out of order (after tasks added)");
  HnAssert(SpaceHasDays(space), "SpaceAddDaysOfWeekDim: space has no days");
  HnAssert(SpaceHasWeeks(space), "SpaceAddDaysOfWeekDim: space has no weeks");

  /* have weeks; add a time dimension holding days of week */
  std = SpaceAddTimeDim(space);
  HaArrayForEach(space->days_of_week, tg, i)
    SpaceTimeDimAddTimeGroup(std, tg, HaArray(space->days_of_week_labels, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddDaysOfCycleDim(SPACE space)                                 */
/*                                                                           */
/*  Add a time dimension with one time group for each day of the cycle.      */
/*  Abort if the instance has no Day time groups.                            */
/*                                                                           */
/*****************************************************************************/

void SpaceAddDaysOfCycleDim(SPACE space)
{
  SPACE_TIME_DIM std;  int i;  KHE_TIME_GROUP tg;

  /* abort if call out of order or there are no day time groups */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddDaysOfCycleDim called out of order (after tasks added)");
  HnAssert(SpaceHasDays(space), "SpaceAddDaysOfCycleDim: space has no days");

  /* add a time dimension with one time group for each day of the cycle */
  std = SpaceAddTimeDim(space);
  HaArrayForEach(space->days, tg, i)
    SpaceTimeDimAddTimeGroup(std, tg, KheTimeGroupId(tg));
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddWeeksOfCycleDim(SPACE space)                                */
/*                                                                           */
/*  Add a time dimension with one time group for each week of the cycle.     */
/*  Abort if the instance has no Week time groups.                           */
/*                                                                           */
/*****************************************************************************/

void SpaceAddWeeksOfCycleDim(SPACE space)
{
  SPACE_TIME_DIM std;  int i;  WEEK week;

  /* abort if call out of order or there are no Week time groups */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddWeeksOfCycleDim called out of order (after tasks added)");
  HnAssert(SpaceHasWeeks(space), "SpaceAddDaysOfCycleDim: space has no weeks");

  /* add a time dimension with one time group for each week of the cycle */
  std = SpaceAddTimeDim(space);
  HaArrayForEach(space->weeks, week, i)
    SpaceTimeDimAddTimeGroup(std, week->week_tg, KheTimeGroupId(week->week_tg));
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddDummyDim(SPACE space)                                       */
/*                                                                           */
/*  Add a dummy dimension.  This has one element and imposes no conditions.  */
/*                                                                           */
/*****************************************************************************/

void SpaceAddDummyDim(SPACE space)
{
  SPACE_DUMMY_DIM res;

  /* abort if call out of order */
  HnAssert(space->root_subspace == NULL,
    "SpaceAddWeeksOfCycleDim called out of order (after tasks added)");

  /* add a dummy dimension */
  res = SpaceDummyDimMake(space);
  HaArrayAddLast(space->dimensions, (SPACE_DIM) res);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tasks"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddTask(SPACE space, KHE_TASK task)                            */
/*                                                                           */
/*  Add task to space.                                                       */
/*                                                                           */
/*****************************************************************************/

void SpaceAddTask(SPACE space, KHE_TASK task)
{
  KHE_MEET meet;  int durn, i, duration;  KHE_TIME t, nt;  KHE_RESOURCE r;
  HTML_FONT font;  float workload;
  if( DEBUG2 )
    fprintf(stderr, "[ SpaceAddTask\n");

  /* build the subspaces if not already done */
  if( space->root_subspace == NULL )
    space->root_subspace = SubSpaceBuild(space, 0,
      SubSpaceCondMake(NULL, NULL, true));
  if( DEBUG2 )
    fprintf(stderr, "  SpaceAddTask after SubSpaceBuild\n");

  if( SpaceShowTask(space, task, &font) )
  {
    /* if task has an assigned time, insert it at each of its times */
    if( DEBUG2 )
      fprintf(stderr, "  SpaceAddTask calling KheTaskMeet \n");
    meet = KheTaskMeet(task);
    if( meet != NULL )
    {
      t = KheMeetAsstTime(meet);
      if( t != NULL )
      {
	duration = durn = KheMeetDuration(meet);
	if( DEBUG2 )
	  fprintf(stderr, "  SpaceAddTask calling KheTaskWorkload\n");
	workload = KheTaskWorkload(task);
	r = KheTaskAsstResource(task);
	for( i = 0;  i < durn;  i++ )
	{
	  nt = KheTimeNeighbour(t, i);
	  SubSpaceInsertTask(space->root_subspace, task, duration, workload,
	    nt, r, space, 0);
	  duration = 0, workload = 0.0;  /* to avoid double counting */
	}
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] SpaceAddTask\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceAddAllTasks(SPACE space)                                       */
/*                                                                           */
/*  Add all tasks to space.                                                  */
/*                                                                           */
/*****************************************************************************/

void SpaceAddAllTasks(SPACE space)
{
  KHE_TASK task;  int i;
  for( i = 0;  i < KheSolnTaskCount(space->soln);  i++ )
  {
    task = KheSolnTask(space->soln, i);
    SpaceAddTask(space, task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "background colour and font functions"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COLOUR_SELECT ColourSelectMake(SPACE_CELL_COND cc,                       */
/*    char *constraints_str, char *colour)                                   */
/*                                                                           */
/*  Make a colour select object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static COLOUR_SELECT ColourSelectMake(SPACE_CELL_COND cc,
  char *constraints_str, char *colour)
{
  COLOUR_SELECT res;
  res.cell_cond = cc;
  res.constraints_str = constraints_str;
  res.colour = colour;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceCellFormat(SPACE space, SPACE_CELL_COND cc, char *colour)      */
/*                                                                           */
/*  Select this background colour for all cells satisfying cc.  When         */
/*  multiple calls to this function are made on space, the first one         */
/*  whose condition is satisfied determines the colour.                      */
/*                                                                           */
/*****************************************************************************/

void SpaceCellFormat(SPACE space, SPACE_CELL_COND cc, char *colour)
{
  HaArrayAddLast(space->colour_selects, ColourSelectMake(cc, NULL, colour));
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceCellFormat2(SPACE space, SPACE_CELL_COND cc,                   */
/*    char *constraints_str, char *colour)                                   */
/*                                                                           */
/*  Like SpaceCellFormat only with more attributes.                          */
/*                                                                           */
/*****************************************************************************/

void SpaceCellFormat2(SPACE space, SPACE_CELL_COND cc,
  char *constraints_str, char *colour)
{
  HaArrayAddLast(space->colour_selects,
    ColourSelectMake(cc, constraints_str, colour));
}


/*****************************************************************************/
/*                                                                           */
/*  FONT_SELECT FontSelectMake(SPACE_TASK_COND tc, bool show, HTML_FONT font)*/
/*                                                                           */
/*  Make a font select object with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

static FONT_SELECT FontSelectMake(SPACE_TASK_COND tc, bool show, HTML_FONT font)
{
  FONT_SELECT res;
  res.task_cond = tc;
  res.show = show;
  res.font = font;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceTaskFormat(SPACE space, SPACE_TASK_COND tc, bool show,         */
/*    HTML_FONT font)                                                        */
/*                                                                           */
/*  Select this format for all tasks satisfying tc.                          */
/*                                                                           */
/*****************************************************************************/

void SpaceTaskFormat(SPACE space, SPACE_TASK_COND tc, bool show, HTML_FONT font)
{
  HaArrayAddLast(space->font_selects, FontSelectMake(tc, show, font));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "display"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SubSpaceExternalDisplay(SUBSPACE_EXTERNAL se, HTML html)            */
/*                                                                           */
/*  Display se onto html.                                                    */
/*                                                                           */
/*****************************************************************************/

static void SubSpaceExternalDisplay(SUBSPACE_EXTERNAL se, int height,
  int width, char *col_label, int *total_italic, HTML html)
{
  HTML_FONT font;  KHE_TASK task;  int i;  char buff[200];  KHE_EVENT e;
  char *start_name, *tg_name;

  /* begin table entry */
  HTMLTableEntrySpanSpanBegin(html, width, height, SubSpaceColour(se));

  /* print the table entry contents, or just a space if none */
  if( HaArrayCount(se->tasks) == 0 )
    HTMLHSpace(html, 2);
  else
  {
    HaArrayForEach(se->tasks, task, i)
    {
      e = KheMeetEvent(KheTaskMeet(task));
      if( !SpaceShowTask(se->encl_space, task, &font) )
	HnAbort("SubSpaceExternalDisplay internal error");
      if( i > 0 )
	HTMLNewLine(html);
      if( font != HTML_ROMAN )
	HTMLFontBegin(html, font);
      if( font == HTML_ITALIC )
        (*total_italic)++;
      start_name = KheEventName(e);
      tg_name = KheTimeGroupName(se->condition.time_group);
      if( tg_name != NULL && strstr(start_name, tg_name) == start_name )
      {
	start_name += strlen(tg_name);
	if( *start_name == ':' )
	  start_name++;
      }
      snprintf(buff, 2 + width * 4, "%s", start_name);
      HTMLText(html, "%s", buff);
      if( font != HTML_ROMAN )
	HTMLFontEnd(html, font);
    }
  }

  /* end table entry */
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  bool SubSpaceExternalEqual(SUBSPACE_EXTERNAL se1, SUBSPACE_EXTERNAL se2) */
/*                                                                           */
/*  Return true if se1 and se2 have the same tasks and background colour.    */
/*  If either has no tasks, they are considered to be unequal, since we      */
/*  don't want empty cells to span.                                          */
/*                                                                           */
/*****************************************************************************/

static bool SubSpaceExternalEqual(SUBSPACE_EXTERNAL se1, SUBSPACE_EXTERNAL se2)
{
  int i;  KHE_TASK task1, task2;  KHE_MEET meet1, meet2;

  /* if either of the cells is empty, there is no spanning */
  if( HaArrayCount(se1->tasks) == 0 )
    return false;

  /* check that the tasks are the same */
  if( HaArrayCount(se1->tasks) != HaArrayCount(se2->tasks) )
    return false;
  HaArrayForEach(se1->tasks, task1, i)
  {
    task2 = HaArray(se2->tasks, i);
    meet1 = KheTaskMeet(task1);
    meet2 = KheTaskMeet(task2);
    if( meet1 != NULL && meet2 != NULL &&
	KheMeetEvent(meet1) != NULL && KheMeetEvent(meet2) != NULL )
    {
      if( KheMeetEvent(meet1) != KheMeetEvent(meet2) )
	return false;
    }
    else
    {
      if( task2 != task1 )
	return false;
    }
  }

  /* check that the colours are the same */
  return strcmp(SubSpaceColour(se1), SubSpaceColour(se2)) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int SubSpaceSpanWidth(SUBSPACE_INTERNAL si, int start_col)               */
/*                                                                           */
/*  Return the number of external subspaces, starting from start_col, that   */
/*  are equal to the subspace at start_col and are not already spanned over. */
/*  The result is at least 1, because start_col is not spanned over.         */
/*                                                                           */
/*****************************************************************************/

static int SubSpaceSpanWidth(SUBSPACE_INTERNAL si, int start_col)
{
  SUBSPACE_EXTERNAL se1, se2;  int i;
  se1 = (SUBSPACE_EXTERNAL) HaArray(si->subspaces, start_col);
  HnAssert(!se1->spanned_over, "SubSpaceSpanWidth internal error");
  for( i = start_col + 1;  i < HaArrayCount(si->subspaces);  i++ )
  {
    se2 = (SUBSPACE_EXTERNAL) HaArray(si->subspaces, i);
    if( se2->spanned_over || !SubSpaceExternalEqual(se1, se2) )
      break;
  }
  return i - start_col;
}


/*****************************************************************************/
/*                                                                           */
/*  int SubSpaceSpanHeight(SUBSPACE_INTERNAL si, int start_row,              */
/*    int start_col, int width)                                              */
/*                                                                           */
/*  Return the height of a rectangle of equal spaces starting at             */
/*  (start_row, start_col) with the given width (at least 1);                */
/*                                                                           */
/*****************************************************************************/

static int SubSpaceSpanHeight(SUBSPACE_INTERNAL si, int start_row,
  int start_col, int width)
{
  SUBSPACE_INTERNAL si1, si2;  SUBSPACE_EXTERNAL se1, se2;  int i;
  si1 = (SUBSPACE_INTERNAL) HaArray(si->subspaces, start_row);
  se1 = (SUBSPACE_EXTERNAL) HaArray(si1->subspaces, start_col);
  for( i = start_row + 1;  i < HaArrayCount(si->subspaces);  i++ )
  {
    si2 = (SUBSPACE_INTERNAL) HaArray(si->subspaces, i);
    se2 = (SUBSPACE_EXTERNAL) HaArray(si2->subspaces, start_col);
    if( !SubSpaceExternalEqual(se1, se2) )
      break;
    if( SubSpaceSpanWidth(si2, start_col) < width )
      break;
  }
  return i - start_row;
}


/*****************************************************************************/
/*                                                                           */
/*  void SubSpaceFindSpanning(SUBSPACE_INTERNAL si, bool allow_spanning,     */
/*    int start_row, int start_col, int *height, int *width)                 */
/*                                                                           */
/*  Set *height and *width for spanning; make both 1 if not allowed.         */
/*                                                                           */
/*****************************************************************************/

static void SubSpaceFindSpanning(SUBSPACE_INTERNAL si, bool allow_spanning,
  int start_row, int start_col, int *height, int *width)
{
  SUBSPACE_INTERNAL si1;

  if( allow_spanning )
  {
    /* find the width */
    si1 = (SUBSPACE_INTERNAL) HaArray(si->subspaces, start_row);
    *width = SubSpaceSpanWidth(si1, start_col);

    /* find the height */
    *height = SubSpaceSpanHeight(si, start_row, start_col, *width);
  }
  else
    *width = *height = 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void SubSpaceMarkSpannedOver(SUBSPACE_INTERNAL si, int start_row,        */
/*    int start_col, int height, int width)                                  */
/*                                                                           */
/*  Mark this rectangle of si as spanned over.                               */
/*                                                                           */
/*****************************************************************************/

static void SubSpaceMarkSpannedOver(SUBSPACE_INTERNAL si, int start_row,
  int start_col, int height, int width)
{
  SUBSPACE_INTERNAL ss_row;  SUBSPACE_EXTERNAL ss_col;  int row, col;
  for( row = 0;  row < height;  row++ )
  {
    ss_row = (SUBSPACE_INTERNAL) HaArray(si->subspaces, start_row + row);
    for( col = 0;  col < width;  col++ )
    {
      ss_col = (SUBSPACE_EXTERNAL) HaArray(ss_row->subspaces, start_col + col);
      HnAssert(!ss_col->spanned_over, "SubSpaceMarkSpannedOver(start_row %d, "
	"start_col %d, height %d, width %d) internal error",
	start_row, start_col, height, width);
      ss_col->spanned_over = true;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool DimMajorHeadingCallBack(SPACE_DIM dim, int index, HTML html)        */
/*                                                                           */
/*  Call back to generate a major heading for the index'th element of dim.   */
/*  Return true if an actual call was made.                                  */
/*                                                                           */
/*****************************************************************************/

static bool DimMajorHeadingCallBack(SPACE_DIM dim, int index, HTML html)
{
  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;  KHE_RESOURCE r;
  switch( dim->dim_type )
  {
    case DIM_TYPE_TIME:

      /* no callback here at present */
      break;

    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      if( srd->major_heading_rg_fn != NULL )
      {
	srd->major_heading_rg_fn(HaArray(srd->resource_groups, index),
	  srd->encl_space->soln, 0, 0.0, NULL, html, srd->encl_space->arena);
	return true;
      }
      break;

    case DIM_TYPE_RESOURCE_TYPE:

      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      if( srtd->major_heading_rg_fn != NULL &&
	  index < KheResourceTypeResourceCount(srtd->resource_type) )
      {
	r = KheResourceTypeResource(srtd->resource_type, index);
	srtd->major_heading_rg_fn(KheResourceSingletonResourceGroup(r),
	  srtd->encl_space->soln, 0, 0.0, NULL, html, srtd->encl_space->arena);
	return true;
      }
      break;

    case DIM_TYPE_DUMMY:

      /* no callback here */
      break;

    default:

      HnAbort("DimMajorHeadingCallBack internal error");
      break;
  }
  return false;  /* no actual call was made */
}


/*****************************************************************************/
/*                                                                           */
/*  bool DimAfterCallBack(SPACE_DIM dim, int index, HTML html)               */
/*                                                                           */
/*  Call back after the index'th element of dim.                             */
/*                                                                           */
/*  Return true if an actual call was made.                                  */
/*                                                                           */
/*****************************************************************************/

static bool DimAfterCallBack(SPACE_DIM dim, int index, HTML html)
{
  SPACE_RESOURCE_DIM srd;  SPACE_RESOURCE_TYPE_DIM srtd;  KHE_RESOURCE r;
  switch( dim->dim_type )
  {
    case DIM_TYPE_TIME:

      /* no callback here at present */
      break;

    case DIM_TYPE_RESOURCE:

      srd = (SPACE_RESOURCE_DIM) dim;
      if( srd->after_rg_fn != NULL )
      {
	srd->after_rg_fn(HaArray(srd->resource_groups, index),
	  srd->encl_space->soln, 0, 0.0, NULL, html, srd->encl_space->arena);
	return true;
      }
      break;

    case DIM_TYPE_RESOURCE_TYPE:

      srtd = (SPACE_RESOURCE_TYPE_DIM) dim;
      if( srtd->after_rg_fn != NULL &&
	  index < KheResourceTypeResourceCount(srtd->resource_type) )
      {
	r = KheResourceTypeResource(srtd->resource_type, index);
	srtd->after_rg_fn(KheResourceSingletonResourceGroup(r),
	  srtd->encl_space->soln, 0, 0.0, NULL, html, srtd->encl_space->arena);
	return true;
      }
      break;

    case DIM_TYPE_DUMMY:

      /* no callback here */
      break;

    default:

      HnAbort("DimAfterCallBack internal error");
      break;
  }
  return false;  /* no actual call was made */
}


/*****************************************************************************/
/*                                                                           */
/*  void SubSpaceInternalDisplay(SUBSPACE_INTERNAL si,                       */
/*    bool allow_spanning, int depth, HTML html)                             */
/*                                                                           */
/*  Display si from depth onwards.  It is known to have at least dim 2.      */
/*                                                                           */
/*****************************************************************************/

static void SubSpaceInternalDisplay(SUBSPACE_INTERNAL si,
  bool allow_spanning, int depth, int *total_italic, HTML html)
{
  int i, j, height, width;  SUBSPACE_INTERNAL row_ss;  SUBSPACE ss;
  SPACE_DIM dim, row_dim, col_dim;  SUBSPACE_EXTERNAL col_ss;  char *label;
  SPACE_RG_FN avail_rg_fn;  void *avail_rg_impl;  KHE_RESOURCE_GROUP rg;

  if( depth == HaArrayCount(si->encl_space->dimensions) - 2 )
  {
    /* display a single (two-dimensional) table */
    row_dim = HaArray(si->encl_space->dimensions, depth);
    avail_rg_fn = SpaceDimAvailFn(row_dim, &avail_rg_impl);
    col_dim = HaArray(si->encl_space->dimensions, depth + 1);

    /* begin table */
    HTMLParagraphBegin(html);
    HTMLTableBeginAttributed(html, 3, 0, 1, LightGreen);

    /* header row */
    HTMLTableRowVAlignBegin(html, "top");
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    if( avail_rg_fn != NULL )
    {
      HTMLTableEntryCentredBegin(html);
      HTMLTextBold(html, "Avail");
      HTMLTableEntryEnd(html);
    }
    HaArrayForEach(col_dim->labels, label, i)
    {
      HTMLTableEntryCentredBegin(html);
      HTMLTextBold(html, label);
      HTMLTableEntryEnd(html);
    }
    HTMLTableRowEnd(html);

    /* ordinary rows */
    HaArrayForEach(row_dim->labels, label, i)
    {
      /* begin row */
      row_ss = (SUBSPACE_INTERNAL) HaArray(si->subspaces, i);
      HTMLTableRowVAlignBegin(html, "top");

      /* header column */
      HTMLTableEntryTextBold(html, label);

      /* avail column */
      if( avail_rg_fn != NULL )
      {
	rg = SpaceDimResourceGroup(row_dim, i);
	HTMLTableEntryCentredBegin(html);
	avail_rg_fn(rg, si->encl_space->soln, row_ss->total_durn,
	  row_ss->total_workload, avail_rg_impl, html, si->encl_space->arena);
	HTMLTableEntryEnd(html);
      }

      /* ordinary columns */
      HaArrayForEach(row_ss->subspaces, ss, j)
      {
	col_ss = (SUBSPACE_EXTERNAL) ss;
	if( !col_ss->spanned_over )
	{
          SubSpaceFindSpanning(si, allow_spanning, i, j, &height, &width);
          SubSpaceMarkSpannedOver(si, i, j, height, width);
	  SubSpaceExternalDisplay(col_ss, height, width,
	    HaArray(col_dim->labels, j), total_italic, html);
	}
      }

      /* end row */
      HTMLTableRowEnd(html);
    }

    /* end table */
    HTMLTableEnd(html);
    HTMLParagraphEnd(html);
  }
  else
  {
    /* display the depth'th dimension and recurse */
    dim = HaArray(si->encl_space->dimensions, depth);
    HaArrayForEach(si->subspaces, ss, i)
    {
      /* call back to generate the heading, or just the label if not */
      if( !DimMajorHeadingCallBack(dim, i, html) )
      {
	HTMLParagraphBegin(html);
	HTMLTextBold(html, "%s", HaArray(dim->labels, i));
	HTMLParagraphEnd(html);
      }

      /* display the subspace and call back afterwards */
      SubSpaceInternalDisplay((SUBSPACE_INTERNAL) ss, allow_spanning,
	depth + 1, total_italic, html);
      DimAfterCallBack(dim, i, html);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SpaceDisplay(SPACE space, HTML html)                                */
/*                                                                           */
/*  Display space onto html.                                                 */
/*                                                                           */
/*****************************************************************************/

void SpaceDisplay(SPACE space, int *total_italic, HTML html)
{
  /* build the subspaces if not already done */
  if( space->root_subspace == NULL )
    space->root_subspace = SubSpaceBuild(space, 0,
      SubSpaceCondMake(NULL, NULL, true));

  /* do the display */
  *total_italic = 0;
  if( HaArrayCount(space->dimensions) < 2 )
  {
    HTMLParagraphBegin(html);
    HTMLTextBold(html, "HSEval has been asked to display a table with");
    HTMLTextBold(html, "fewer than 2 dimensions, which it cannot do.");
    HTMLParagraphEnd(html);
  }
  else
    SubSpaceInternalDisplay((SUBSPACE_INTERNAL) space->root_subspace,
      space->allow_spanning, 0, total_italic, html);
}

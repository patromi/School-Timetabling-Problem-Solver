
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
/*  FILE:         khe_frame.c                                                */
/*  DESCRIPTION:  Frames                                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#include <limits.h>
#include <float.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_frame_elt_rec {
  KHE_TIME_GROUP	time_group;
  KHE_POLARITY		polarity;
} KHE_FRAME_ELT;

typedef HA_ARRAY(KHE_FRAME_ELT) ARRAY_KHE_FRAME_ELT;
*** */

/* ***
struct khe_frame_make_rec {
  KHE_SOLN		soln;			** enclosing soln            **
  ARRAY_KHE_TIME_GROUP	time_groups;		** time groups               **
  ** ARRAY_KHE_FRAME_ELT elements; **		** time groups/polarities    **
  HA_ARRAY_INT		time_indexes;		** indexed by time           **
  int			min_time_index;
  bool			immutable;		** creation is complete      **
};
*** */

struct khe_frame_rec {
  KHE_INSTANCE		ins;			/* instance */
  ARRAY_KHE_TIME_GROUP	time_groups;		/* time groups               */
  HA_ARRAY_INT		time_indexes;		/* indexed by time           */
  int			min_time_index;
  bool			immutable;		/* creation is complete      */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME_WORKLOAD                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
struct khe_frame_workload_rec {
  KHE_SOLN		soln;
  KHE_RESOURCE_TYPE	resource_type;
  HA_ARRAY_FLOAT	min_workloads;
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrame(KHE_FRAME_MAKE priv, int start_index, int stop_index) */
/*                                                                           */
/*  Return a new frame object with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_FRAME KheFrame(KHE_FRAME_MAKE priv, int start_index, int stop_index)
{
  KHE_FRAME res;
  res.priv = priv;
  res.start_index = start_index;
  res.stop_index = stop_index;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameIsSlice(KHE_FRAME frame)                                    */
/*                                                                           */
/*  Return true if frame is in fact a slice.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheFrameIsSlice(KHE_FRAME frame)
{
  return frame.start_index != 0 ||
    frame.stop_index != HaArrayCount(frame.priv->time_groups);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameMakeBegin(KHE_INSTANCE ins, HA_ARENA a)                */
/*                                                                           */
/*  Begin the construction of a frame.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeBegin(KHE_INSTANCE ins, HA_ARENA a)
{
  KHE_FRAME res;
  HaMake(res, a);
  res->ins = ins;
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->time_indexes, a);
  res->min_time_index = 0;
  res->immutable = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheFrameInstance(KHE_FRAME frame)                           */
/*                                                                           */
/*  Return the instance that frame is for,                                   */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheFrameInstance(KHE_FRAME frame)
{
  return frame->ins;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameAddTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg)            */
/*                                                                           */
/*  Add tg to the frame under construction.                                  */
/*                                                                           */
/*****************************************************************************/

void KheFrameAddTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg)
{
  HnAssert(!frame->immutable,
    "KheFrameAddTimeGroup called after KheFrameMakeEnd");
  HaArrayAddLast(frame->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupFirstTimeCmp(const void *t1, const void *t2)              */
/*                                                                           */
/*  Comparison function for sorting frame elements by increasing first time. */
/*                                                                           */
/*****************************************************************************/

static int KheTimeGroupFirstTimeCmp(const void *t1, const void *t2)
{
  KHE_TIME_GROUP tg1 = * (KHE_TIME_GROUP *) t1;
  KHE_TIME_GROUP tg2 = * (KHE_TIME_GROUP *) t2;
  int index1, index2;
  index1 = (KheTimeGroupTimeCount(tg1) == 0 ? -1 :
    KheTimeIndex(KheTimeGroupTime(tg1, 0)));
  index2 = (KheTimeGroupTimeCount(tg2) == 0 ? -1 :
    KheTimeIndex(KheTimeGroupTime(tg2, 0)));
  return index1 - index2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMinAndMaxTimeIndexes(KHE_FRAME frame,                            */
/*    int *min_index, int *max_index)                                        */
/*                                                                           */
/*  If fm contains a time group containing at least one time, return true    */
/*  and set *min_index and *max_index to the indexes of the smallest and     */
/*  largest times.  Otherwise return false.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMinAndMaxTimeIndexes(KHE_FRAME frame,
  int *min_index, int *max_index)
{
  int i;  KHE_TIME min_t, max_t;  KHE_TIME_GROUP tg;
  *min_index = *max_index = -1;
  HaArrayForEach(frame->time_groups, tg, i)
    if( KheTimeGroupTimeCount(tg) > 0 )
    {
      min_t = KheTimeGroupTime(tg, 0);
      max_t = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
      if( *min_index == -1 || KheTimeIndex(min_t) < *min_index )
	*min_index = KheTimeIndex(min_t);
      if( *max_index == -1 || KheTimeIndex(max_t) > *max_index )
	*max_index = KheTimeIndex(max_t);
    }
  return *min_index != -1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameMakeEnd(KHE_FRAME frame, bool sort_time_groups)             */
/*                                                                           */
/*  End the construction of frame.                                           */
/*                                                                           */
/*****************************************************************************/

void KheFrameMakeEnd(KHE_FRAME frame, bool sort_time_groups)
{
  int i, j, min_index, max_index;  KHE_TIME_GROUP tg;  KHE_TIME t;
  HnAssert(!frame->immutable, "KheFrameMakeEnd called twice on the same fm");
  frame->immutable = true;
  if( sort_time_groups )
    HaArraySort(frame->time_groups, &KheTimeGroupFirstTimeCmp);
  if( KheMinAndMaxTimeIndexes(frame, &min_index, &max_index) )
  {
    HaArrayFill(frame->time_indexes, max_index - min_index + 1, -1);
    frame->min_time_index = min_index;
    HaArrayForEach(frame->time_groups, tg, i)
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
      {
	t = KheTimeGroupTime(tg, j);
	HaArrayPut(frame->time_indexes, KheTimeIndex(t) - min_index, i);
      }
  }
  else
    frame->min_time_index = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameTimeGroupCount(KHE_FRAME frame)                              */
/*                                                                           */
/*  Return the number of time groups in frame.                               */
/*                                                                           */
/*****************************************************************************/

int KheFrameTimeGroupCount(KHE_FRAME frame)
{
  return HaArrayCount(frame->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheFrameTimeGroup(KHE_FRAME frame, int i)                 */
/*                                                                           */
/*  Return the i'th time group of frame.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheFrameTimeGroup(KHE_FRAME frame, int i)
{
  HnAssert(0 <= i && i < HaArrayCount(frame->time_groups),
    "KheFrameTimeGroup:  i (%d) out of range (0 .. %d)",
    i, HaArrayCount(frame->time_groups) - 1);
  return HaArray(frame->time_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameIsDisjoint(KHE_FRAME frame, int *problem_index1,            */
/*    int *problem_index2)                                                   */
/*                                                                           */
/*  If frame is disjoint, return true with *problem_index1 and               */
/*  *problem_index2 set to -1.  Otherwise return false with                  */
/*  *problem_index1 and *problem_index2 set to the indexes of two            */
/*  non-disjoint time groups.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheFrameIsDisjoint(KHE_FRAME frame, int *problem_index1,
  int *problem_index2)
{
  KHE_TIME_GROUP tg1, tg2;  int i1, i2;
  for( i2 = 0;  i2 < KheFrameTimeGroupCount(frame);  i2++ )
  {
    tg2 = KheFrameTimeGroup(frame, i2);
    for( i1 = 0;  i1 < i2;  i1++ )
    {
      tg1 = KheFrameTimeGroup(frame, i1);
      if( !KheTimeGroupDisjoint(tg1, tg2) )
      {
	*problem_index1 = i1;
	*problem_index2 = i2;
	return false;
      }
    }
  }
  *problem_index1 = -1;
  *problem_index2 = -1;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameIsComplete(KHE_FRAME frame, KHE_TIME *problem_time)         */
/*                                                                           */
/*  If frame is complete, return true with *problem_time set to NULL.        */
/*  If frame is not complete, return false with *problem_time set to a       */
/*  time not covered by frame.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheFrameIsComplete(KHE_FRAME frame, KHE_TIME *problem_time)
{
  int i;  KHE_TIME t;
  for( i = 0;  i < KheInstanceTimeCount(frame->ins);  i++ )
  {
    t = KheInstanceTime(frame->ins, i);
    if( KheFrameTimeIndex(frame, t) == -1 )
    {
      *problem_time = t;
      return false;
    }
  }
  *problem_time = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameTimeIndex(KHE_FRAME frame, KHE_TIME t)                       */
/*                                                                           */
/*  Return the index of the time group of frame that contains time t.        */
/*                                                                           */
/*****************************************************************************/

int KheFrameTimeIndex(KHE_FRAME frame, KHE_TIME t)
{
  int t_index;
  t_index = KheTimeIndex(t) - frame->min_time_index;
  if( t_index < 0 || t_index >= HaArrayCount(frame->time_indexes) )
    return -1;
  return HaArray(frame->time_indexes, t_index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheFrameTimeTimeGroup(KHE_FRAME frame, KHE_TIME t)        */
/*                                                                           */
/*  Return the time group of frame containing t, or abort if none.           */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheFrameTimeTimeGroup(KHE_FRAME frame, KHE_TIME t)
{
  int index = KheFrameTimeIndex(frame, t);
  HnAssert(index != -1, "KheFrameTimeTimeGroup:  time %s not in frame",
    KheTimeId(t));
  return KheFrameTimeGroup(frame, index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameTimeOffset(KHE_FRAME frame, KHE_TIME t)                      */
/*                                                                           */
/*  Return t's offset in its time group in frame.                            */
/*                                                                           */
/*****************************************************************************/

int KheFrameTimeOffset(KHE_FRAME frame, KHE_TIME t)
{
  KHE_TIME_GROUP tg;
  tg = KheFrameTimeTimeGroup(frame, t);
  return KheTimeIndex(t) - KheTimeIndex(KheTimeGroupTime(tg, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheFrameMakeCommon and its helper functions"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameEqual(KHE_FRAME frame1, KHE_FRAME frame2)                   */
/*                                                                           */
/*  Return true if frame1 and frame2 are equal.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheFrameEqual(KHE_FRAME frame1, KHE_FRAME frame2)
{
  KHE_TIME_GROUP tg1, tg2;  int i;
  if( KheFrameTimeGroupCount(frame1) != KheFrameTimeGroupCount(frame2) )
    return false;
  for( i = 0;  i < KheFrameTimeGroupCount(frame1);  i++ )
  {
    tg1 = KheFrameTimeGroup(frame1, i);
    tg2 = KheFrameTimeGroup(frame2, i);
    if( !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheDoFrame(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc, HA_ARENA a)   */
/*                                                                           */
/*  If the time groups of lbtc partition the cycle, return the frame for it, */
/*  otherwise return a null frame.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_FRAME KheDoFrame(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc, HA_ARENA a)
{
  int i, offset, problem_index1, problem_index2;  KHE_TIME problem_time;
  KHE_TIME_GROUP tg;  KHE_FRAME res;  KHE_INSTANCE ins;
  if( DEBUG3 )
    fprintf(stderr, "[ KheDoFrame(...)\n");
  if( DEBUG9 )
    fprintf(stderr, "  calling KheFrameMakeBegin from KheDoFrame\n");
  ins = KheConstraintInstance((KHE_CONSTRAINT) lbtc);
  res = KheFrameMakeBegin(ins, a);
  if( KheLimitBusyTimesConstraintAppliesToTimeGroup(lbtc) == NULL )
  {
    /* no applies to time group, so use all the time groups */
    for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(lbtc);  i++ )
    {
      tg = KheLimitBusyTimesConstraintTimeGroup(lbtc, i, 0);
      KheFrameAddTimeGroup(res, tg);
    }
  }
  else if( KheLimitBusyTimesConstraintTimeGroupCount(lbtc) >= 1 )
  {
    /* applies to time group, so use the first time group, repeated */
    for( i = 0; i < KheLimitBusyTimesConstraintAppliesToOffsetCount(lbtc); i++ )
    {
      offset = KheLimitBusyTimesConstraintAppliesToOffset(lbtc, i);
      tg = KheLimitBusyTimesConstraintTimeGroup(lbtc, 0, offset);
      if( DEBUG3 )
	fprintf(stderr, "  adding time group %s\n", KheTimeGroupId(tg));
      KheFrameAddTimeGroup(res, tg);
    }
  }
  KheFrameMakeEnd(res, true);
  if( !KheFrameIsDisjoint(res, &problem_index1, &problem_index2) )
  {
    if( DEBUG3 )
      fprintf(stderr, "] KheDoFrame ret false (3)\n");
    return NULL;
  }
  if( !KheFrameIsComplete(res, &problem_time) )
  {
    if( DEBUG3 )
      fprintf(stderr, "] KheDoFrame ret false (4)\n");
    return NULL;
  }
  if( DEBUG3 )
  {
    fprintf(stderr, "] KheDoFrame ret true: ");
    KheFrameDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheGetFrame(KHE_RESOURCE_TYPE rt)                              */
/*                                                                           */
/*  Make and return a common frame for the resources of rt, or               */
/*  return a null frame if that is not possible.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_FRAME KheGetFrame(KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_RESOURCE r;  KHE_CONSTRAINT c;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
  KHE_FRAME res;
  int i;
  if( DEBUG2 )
    fprintf(stderr, "[ KheGetFrame(%s)\n", KheResourceTypeId(rt));
  r = KheResourceTypeResource(rt, 0);
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    if( KheConstraintTag(c) == KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG &&
        KheConstraintRequired(c) && KheConstraintWeight(c) > 0 )
    {
      lbtc = (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
      if( KheLimitBusyTimesConstraintResourceOfTypeCount(lbtc, rt) >=
	  KheResourceTypeResourceCount(rt) )
      {
	res = KheDoFrame(lbtc, a);
	if( res != NULL )
	{
	  if( DEBUG2 )
	  {
	    fprintf(stderr, "] KheGetFrame ret ");
	    KheFrameDebug(res, 1, 0, stderr);
	  }
	  return res;
	}
      }
    }
  }
  res = NULL;
  if( DEBUG2 )
  {
    fprintf(stderr, "] KheGetFrame ret ");
    KheFrameDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameMakeCommon(KHE_INSTANCE ins, HA_ARENA a)               */
/*                                                                           */
/*  Make a common frame for the resources of ins.                            */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeCommon(KHE_INSTANCE ins, HA_ARENA a)
{
  KHE_FRAME res, res2;  int i;  KHE_RESOURCE_TYPE rt;
  if( DEBUG1 )
    fprintf(stderr, "[ KheFrameMakeCommon(%s)\n", KheInstanceId(ins));
  res = NULL;
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    if( KheResourceTypeResourceCount(rt) > 0 )
    {
      res2 = KheGetFrame(rt, a);
      if( DEBUG1 )
      {
	fprintf(stderr, "  %s: ", KheResourceTypeId(rt));
	KheFrameDebug(res2, 1, 0, stderr);
      }
      if( res2 == NULL )
      {
	if( DEBUG1 )
	  fprintf(stderr, "] KheFrameMakeCommon returning NULL (%s)\n",
	    KheResourceTypeId(rt));
	return NULL;
      }
      else if( res == NULL )
	res = res2;
      else if( !KheFrameEqual(res, res2) )
      {
	if( DEBUG1 )
	  fprintf(stderr, "] KheFrameMakeCommon ret. NULL (incompatible %s)\n",
	    KheResourceTypeId(rt));
	return NULL;
      }
    }
  }
  if( res == NULL )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheFrameMakeCommon ret. NULL (no candidates)\n");
    return NULL;
  }
  else
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "] KheFrameMakeCommon returning ");
      KheFrameDebug(res, 1, 0, stderr);
    }
    return res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other public functions"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameMakeSingletons(KHE_INSTANCE ins, HA_ARENA a)           */
/*                                                                           */
/*  Make a frame of singletons.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeSingletons(KHE_INSTANCE ins, HA_ARENA a)
{
  KHE_FRAME res;  int i;  KHE_TIME t;
  if( DEBUG9 )
    fprintf(stderr,"  calling KheFrameMakeBegin from KheFrameMakeSingletons\n");
  res = KheFrameMakeBegin(ins, a);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
    KheFrameAddTimeGroup(res, KheTimeSingletonTimeGroup(t));
  }
  KheFrameMakeEnd(res, false);
  if( DEBUG4 )
  {
    fprintf(stderr, "KheFrameMakeSingletons ");
    KheFrameDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIncreasingFloatCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Comparison function for sorting floats into increasing order.            */
/*                                                                           */
/*****************************************************************************/

int KheIncreasingFloatCmp(const void *t1, const void *t2)
{
  float cmp;
  float f1 = * (float *) t1;
  float f2 = * (float *) t2;
  cmp = f1 - f2;
  if( cmp < 0 )
    return -1;
  else if( cmp > 0 )
    return 1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameIntersectsTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg)     */
/*                                                                           */
/*  Return true if any of the time groups of frame have a non-empty          */
/*  intersection with tg.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheFrameIntersectsTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg)
{
  int i;  KHE_TIME_GROUP tg2;
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg2 = KheFrameTimeGroup(frame, i);
    if( !KheTimeGroupDisjoint(tg2, tg) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameDebug(KHE_FRAME frame, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of frame onto fp with the given verbosity and indent.        */
/*  Here frame may be NULL.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheFrameDebug(KHE_FRAME frame, int verbosity, int indent, FILE *fp)
{
  int i, j, index;  KHE_TIME ti, tj;  KHE_TIME_GROUP tg;  KHE_INSTANCE ins;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  /* fprintf(fp, "%d:%d", frame.start_index, frame.stop_index); */
  if( frame == NULL )
    fprintf(fp, "[NULL]");
  else if( KheFrameTimeGroupCount(frame) == 0 )
    fprintf(fp, "[EMPTY]");
  else if( verbosity == 1 )
  {
    for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
    {
      tg = KheFrameTimeGroup(frame, i);
      if( KheTimeGroupTimeCount(tg) == 0 )
	fprintf(fp, "[]");
      else if( KheTimeGroupTimeCount(tg) == 1 )
      {
	ti = KheTimeGroupTime(tg, 0);
	fprintf(fp, "[%s]", KheTimeId(ti));
      }
      else if( KheTimeGroupIsCompact(tg) && KheTimeGroupId(tg) != NULL )
	fprintf(fp, "[%s]", KheTimeGroupId(tg));
      else
      {
	ti = KheTimeGroupTime(tg, 0);
	tj = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
	fprintf(fp, "[%s-%s]", KheTimeId(ti), KheTimeId(tj));
      }
    }
  }
  else
  {
    tg = KheFrameTimeGroup(frame, 0);
    ins = KheTimeGroupInstance(tg);
    for( i = 0;  i < HaArrayCount(frame->time_indexes);  i = j )
    {
      for( j = i + 1;  j < HaArrayCount(frame->time_indexes);  j++ )
	if( HaArray(frame->time_indexes, i) !=
	    HaArray(frame->time_indexes, j) )
	  break;
      if( j - 1 == i )
      {
	ti = KheInstanceTime(ins, i);
	fprintf(fp, "[%s]", KheTimeId(ti));
      }
      else
      {
	ti = KheInstanceTime(ins, i);
	tj = KheInstanceTime(ins, j - 1);
	index = HaArray(frame->time_indexes, i);
	if( index == -1 )
          fprintf(fp, "[-]");
	else
	{
	  tg = KheFrameTimeGroup(frame, index);
	  fprintf(fp, "[%s-%s]", KheTimeId(ti), KheTimeId(tj));
	  /* ***
	  if( po == KHE_POSITIVE )
	  else
	    fprintf(fp, "[*%s-%s*]", KheTimeId(ti), KheTimeId(tj));
	  *** */
	}
      }
    }
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameResourceHasClashes(KHE_FRAME frame, KHE_RESOURCE r,         */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Return true if frame has clashes for r in soln.                          */
/*                                                                           */
/*****************************************************************************/

bool KheFrameResourceHasClashes(KHE_FRAME frame, KHE_RESOURCE r, KHE_SOLN soln)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i, j, k;
  KHE_TASK first_task, task;  KHE_TIME t;
  rtm = KheResourceTimetableMonitor(soln, r);
  HaArrayForEach(frame->time_groups, tg, i)
  {
    first_task = NULL;
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	if( first_task == NULL )
	  first_task = task;
	else
	  return true;
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameResourceAssertNoClashes(KHE_FRAME frame, KHE_RESOURCE r,    */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Verify that r has no clashes in frame.  If not, abort.                   */
/*                                                                           */
/*****************************************************************************/

void KheFrameResourceAssertNoClashes(KHE_FRAME frame, KHE_RESOURCE r,
  KHE_SOLN soln)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i, j, k;
  KHE_TASK first_task, task;  KHE_TIME t;
  rtm = KheResourceTimetableMonitor(soln, r);
  HaArrayForEach(frame->time_groups, tg, i)
  {
    first_task = NULL;
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	if( first_task == NULL )
	  first_task = task;
	else
	{
	  fprintf(stderr, "In %s, clash for %s (",
	    KheInstanceId(KheResourceInstance(r)), KheResourceId(r));
	  KheTaskDebug(KheTaskProperRoot(first_task), 2, -1, stderr);
	  fprintf(stderr, " and ");
	  KheTaskDebug(KheTaskProperRoot(task), 2, -1, stderr);
	  fprintf(stderr, ") in time group ");
	  KheTimeGroupDebug(tg, 2, -1, stderr);
	  fprintf(stderr, ":\n");
	  KheResourceTimetableMonitorPrintTimetable(rtm, frame, 10, 0, stderr);
	  HnAbort("timetable clash, aborting now");
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameAssertNoClashes(KHE_FRAME frame, KHE_SOLN soln)             */
/*                                                                           */
/*  Verify that there are no clashes in soln, aborting if not.               */
/*                                                                           */
/*****************************************************************************/

void KheFrameAssertNoClashes(KHE_FRAME frame, KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int i;  KHE_RESOURCE r;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheFrameResourceAssertNoClashes(frame, r, soln);
  }
}


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
/*  KHE_FRAME_MAKE  - the private part of a frame                            */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_frame_elt_rec {
  KHE_TIME_GROUP	time_group;
  KHE_POLARITY		polarity;
} KHE_FRAME_ELT;

typedef HA_ARRAY(KHE_FRAME_ELT) ARRAY_KHE_FRAME_ELT;
*** */

struct khe_frame_make_rec {
  KHE_SOLN		soln;			/* enclosing soln            */
  ARRAY_KHE_TIME_GROUP	time_groups;		/* time groups               */
  /* ARRAY_KHE_FRAME_ELT elements; */		/* time groups/polarities    */
  HA_ARRAY_INT		time_indexes;		/* indexed by time           */
  int			min_time_index;
  bool			immutable;		/* creation is complete      */
};

struct khe_frame_rec {
  KHE_SOLN		soln;			/* enclosing soln            */
  ARRAY_KHE_TIME_GROUP	time_groups;		/* time groups               */
  /* ARRAY_KHE_FRAME_ELT elements; */		/* time groups/polarities    */
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
/*  KHE_FRAME_MAKE KheFrameMakeBegin(KHE_SOLN soln)                          */
/*                                                                           */
/*  Begin the construction of a frame.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME_MAKE KheFrameMakeBegin(KHE_SOLN soln)
{
  KHE_FRAME_MAKE res;  HA_ARENA a;
  res = KheSolnGetFrameMakeFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->time_groups, a);
    HaArrayInit(res->time_indexes, a);
  }
  else
  {
    HaArrayClear(res->time_groups);
    HaArrayClear(res->time_indexes);
  }
  /* MMakeCount(priv, 51); */
  res->min_time_index = 0;
  res->soln = soln;
  res->immutable = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameMakeAddTimeGroup(KHE_FRAME_MAKE fm, KHE_TIME_GROUP tg,      */
/*    KHE_POLARITY po)                                                       */
/*                                                                           */
/*  Add tg with polarity po to the frame under construction.                 */
/*                                                                           */
/*****************************************************************************/

void KheFrameMakeAddTimeGroup(KHE_FRAME_MAKE fm, KHE_TIME_GROUP tg)
  /* KHE_POLARITY po) */
{
  /* KHE_FRAME_ELT elt; */
  HnAssert(!fm->immutable,
    "KheFrameMakeAddTimeGroup called after KheFrameMakeEnd");
  HaArrayAddLast(fm->time_groups, tg);
  /* ***
  elt.time_group = tg;
  elt.polarity = po;
  HaArrayAddLast(fm->elements, elt);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameSetTimeGroupIndexes(KHE_FRAME frame, KHE_TIME_GROUP tg,     */
/*    int tg_index)                                                          */
/*                                                                           */
/*  Within frame, set the time group indexes of the times of tg to tg_index. */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheFrameSetTimeGroupIndexes(KHE_FRAME_MAKE priv,
  KHE_TIME_GROUP tg, int tg_index)
{
  int i, tg_count, index;  KHE_TIME t;
  tg_count = KheTimeGroupTimeCount(tg);
  if( tg_count > 0 )
  {
    t = KheTimeGroupTime(tg, tg_count - 1);
    index = KheTimeIndex(t);
    HaArrayFill(priv->time_indexes, index + 1, -1);
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      index = KheTimeIndex(t);
      HaArrayPut(priv->time_indexes, index, tg_index);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupFirstTimeCmp(const void *t1, const void *t2)             */
/*                                                                           */
/*  Comparison function for sorting time groups by increasing first time.    */
/*                                                                           */
/*****************************************************************************/

/* ***
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
*** */


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
  /* ***
  KHE_FRAME_ELT fe1 = * (KHE_FRAME_ELT *) t1;
  KHE_FRAME_ELT fe2 = * (KHE_FRAME_ELT *) t2;
  int index1, index2;
  index1 = (KheTimeGroupTimeCount(fe1.time_group) == 0 ? -1 :
    KheTimeIndex(KheTimeGroupTime(fe1.time_group, 0)));
  index2 = (KheTimeGroupTimeCount(fe2.time_group) == 0 ? -1 :
    KheTimeIndex(KheTimeGroupTime(fe2.time_group, 0)));
  *** */
  return index1 - index2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMinAndMaxTimeIndexes(KHE_FRAME_MAKE fm,                          */
/*    int *min_index, int *max_index)                                        */
/*                                                                           */
/*  If fm contains a time group containing at least one time, return true    */
/*  and set *min_index and *max_index to the indexes of the smallest and     */
/*  largest times.  Otherwise return false.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMinAndMaxTimeIndexes(KHE_FRAME_MAKE fm,
  int *min_index, int *max_index)
{
  int i;  /* KHE_FRAME_ELT fe; */  KHE_TIME min_t, max_t;  KHE_TIME_GROUP tg;
  *min_index = *max_index = -1;
  /* ***
  HaArrayForEach(fm->elements, fe, i)
    if( KheTimeGroupTimeCount(fe.time_group) > 0 )
    {
      min_t = KheTimeGroupTime(fe.time_group, 0);
      max_t = KheTimeGroupTime(fe.time_group,
	KheTimeGroupTimeCount(fe.time_group) - 1);
      if( *min_index == -1 || KheTimeIndex(min_t) < *min_index )
	*min_index = KheTimeIndex(min_t);
      if( *max_index == -1 || KheTimeIndex(max_t) > *max_index )
	*max_index = KheTimeIndex(max_t);
    }
  *** */
  HaArrayForEach(fm->time_groups, tg, i)
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
/*  KHE_FRAME KheFrameMakeEnd(KHE_FRAME_MAKE fm, bool sort_time_groups)      */
/*                                                                           */
/*  End the construction of a frame, and return it.                          */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeEnd(KHE_FRAME_MAKE fm, bool sort_time_groups)
{
  int i, j, min_index, max_index;  KHE_TIME_GROUP tg;  KHE_TIME t;
  HnAssert(!fm->immutable, "KheFrameMakeEnd called twice on the same fm");
  fm->immutable = true;
  if( sort_time_groups )
    HaArraySort(fm->time_groups, &KheTimeGroupFirstTimeCmp);
  if( KheMinAndMaxTimeIndexes(fm, &min_index, &max_index) )
  {
    HaArrayFill(fm->time_indexes, max_index - min_index + 1, -1);
    fm->min_time_index = min_index;
    HaArrayForEach(fm->time_groups, tg, i)
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
      {
	t = KheTimeGroupTime(tg, j);
	HaArrayPut(fm->time_indexes, KheTimeIndex(t) - min_index, i);
      }
  }
  else
    fm->min_time_index = 0;
  return (KHE_FRAME) fm;
  /* return KheFrame(fm, 0, HaArrayCount(fm->time_groups)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameDelete(KHE_FRAME frame)                                     */
/*                                                                           */
/*  Delete frame, that is, delete its private part.                          */
/*                                                                           */
/*****************************************************************************/

void KheFrameDelete(KHE_FRAME frame)
{
  /* HnAssert(!KheFrameIsSlice(frame), "KheFrameDelete: frame is a slice"); */
  KheSolnAddFrameMakeToFreeList(frame->soln, (KHE_FRAME_MAKE) frame);
  /* ***
  MArrayFree(frame.priv->elements);
  MArrayFree(frame.priv->time_indexes);
  MFree(frame.priv);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheFrameSoln(KHE_FRAME frame)                                   */
/*                                                                           */
/*  Return the soln enclosing frame.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheFrameSoln(KHE_FRAME frame)
{
  return frame->soln;
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
  /* return frame.stop_index - frame.start_index; */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheFrameTimeGroup(KHE_FRAME frame, int i,                 */
/*    KHE_POLARITY *po)                                                      */
/*                                                                           */
/*  Return the i'th time group of frame, and set *po to its polarity.        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheFrameTimeGroup(KHE_FRAME frame, int i /*, KHE_POLARITY *po */)
{
  HnAssert(0 <= i && i < HaArrayCount(frame->time_groups),
    "KheFrameTimeGroup:  i (%d) out of range (0 .. %d)",
    i, HaArrayCount(frame->time_groups) - 1);
  return HaArray(frame->time_groups, i);
  /* ***
  return HaArray(frame->time_groups, i + frame.start_index);
  KHE_FRAME_ELT fe;
  fe = HaArray(frame.priv->elements, i + frame.start_index);
  *po = fe.polarity;
  return fe.time_group;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameMakeNull(void)                                         */
/*                                                                           */
/*  Make a NULL frame.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameMakeNull(void)
{
  return KheFrame(NULL, 0, -1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameIsNull(KHE_FRAME frame)                                     */
/*                                                                           */
/*  Return true if frame is NULL.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheFrameIsNull(KHE_FRAME frame)
{
  return frame.priv == NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameSlice(KHE_FRAME frame, int start_index, int stop_index)*/
/*                                                                           */
/*  Return a slice of frame.                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameSlice(KHE_FRAME frame, int start_index, int stop_index)
{
  HnAssert(0 <= frame.start_index + start_index,
    "KheFrameSlice: start_index too small");
  HnAssert(start_index <= stop_index,"KheFrameSlice: start_index > stop_index");
  HnAssert(frame.start_index + stop_index <= HaArrayCount(frame.priv->time_groups),
    "KheFrameSlice: stop_index too large");
  return KheFrame(frame.priv, frame.start_index + start_index,
    frame.start_index + stop_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameInsideLeftSlice(KHE_FRAME frame, int len)              */
/*                                                                           */
/*  Return the slice of frame at the left end of frame with the given len,   */
/*  or less if necessary.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameInsideLeftSlice(KHE_FRAME frame, int len)
{
  int avail_len = KheFrameTimeGroupCount(frame);
  if( len > avail_len )
    len = avail_len;
  return KheFrame(frame.priv, frame.start_index, frame.start_index + len);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameInsideRightSlice(KHE_FRAME frame, int len)             */
/*                                                                           */
/*  Return the slice of frame at the right end of frame with the given len,  */
/*  or less if necessary.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameInsideRightSlice(KHE_FRAME frame, int len)
{
  int avail_len = KheFrameTimeGroupCount(frame);
  if( len > avail_len )
    len = avail_len;
  return KheFrame(frame.priv, frame.stop_index - len, frame.stop_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameOutsideLeftSlice(KHE_FRAME frame, int len)             */
/*                                                                           */
/*  Return a frame adjacent to frame on the left.  Its length will be len,   */
/*  or less (possibly empty) if necessary.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameOutsideLeftSlice(KHE_FRAME frame, int len)
{
  int avail_len = frame.start_index;
  if( len > avail_len )
    len = avail_len;
  return KheFrame(frame.priv, frame.start_index - len, frame.start_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameOutsideRightSlice(KHE_FRAME frame, int len)            */
/*                                                                           */
/*  Return a frame adjacent to frame on the right.  Its length will be len,  */
/*  or less (possibly empty) if necessary.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheFrameOutsideRightSlice(KHE_FRAME frame, int len)
{
  int avail_len = HaArrayCount(frame.priv->time_groups) - frame.stop_index;
  if( len > avail_len )
    len = avail_len;
  return KheFrame(frame.priv, frame.stop_index, frame.stop_index + len);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameFullSlice(KHE_FRAME frame)                             */
/*                                                                           */
/*  Return the full slice that frame was sliced from.                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
KHE_FRAME KheFrameFullSlice(KHE_FRAME frame)
{
  return KheFrame(frame.priv, 0, HaArrayCount(frame.priv->time_groups));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameStartIndex(KHE_FRAME frame)                                  */
/*                                                                           */
/*  Return the start index of frame.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
int KheFrameStartIndex(KHE_FRAME frame)
{
  return frame.start_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameStopIndex(KHE_FRAME frame)                                   */
/*                                                                           */
/*  Return the stop index of frame.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
int KheFrameStopIndex(KHE_FRAME frame)
{
  return frame.stop_index == -1 ? HaArrayCount(frame.priv->time_groups) :
    frame.stop_index;
}
*** */


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
  int i;  KHE_INSTANCE ins;  KHE_TIME t;
  ins = KheSolnInstance(frame->soln);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
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
  /* ***
  return (tg_index >= frame.start_index && tg_index < frame.stop_index) ?
    tg_index - frame.start_index : -1;
  *** */
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
/*  bool KheResourceGroupsDisjoint(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc,     */
/*    KHE_RESOURCE_TYPE rt, int *count)                                      */
/*                                                                           */
/*  If the resource groups of lbtc of type rt are all disjoint, return       */
/*  true and set *count to their total number of resources.  Otherwise       */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheResourceGroupsDisjoint(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc,
  KHE_RESOURCE_TYPE rt, int *count)
{
  int i, j;  KHE_RESOURCE_GROUP rg, rg2;
  *count = 0;
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceGroupCount(lbtc);  i++ )
  {
    rg = KheLimitBusyTimesConstraintResourceGroup(lbtc, i);
    if( KheResourceGroupResourceType(rg) == rt )
    {
      ** check disjoint with all previous resource groups of type rt **
      for( j = 0;  j < i;  j++ )
      {
	rg2 = KheLimitBusyTimesConstraintResourceGroup(lbtc, j);
	if( KheResourceGroupResourceType(rg2) == rt &&
	    !KheResourceGroupDisjoint(rg, rg2) )
	  return false;
      }

      ** rg is OK, add to *count **
      *count += KheResourceGroupResourceCount(rg);
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourcesLowerBound(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc)         */
/*                                                                           */
/*  Return a lower bound on the number of resources which are points of      */
/*  application of lbtc.  This code assumes that individual resources        */
/*  are distinct.                                                            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLimitBusyTimesConstraintResourceOfTypeCount(lbtc, rt)
static int KheResourcesLowerBound(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc,
  KHE_RESOURCE_TYPE rt)
{
  int res, i, count;  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;

  ** first estimate is the number of individual resources **
  res = 0;
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceCount(lbtc);  i++ )
  {
    r = KheLimitBusyTimesConstraintResource(lbtc, i);
    if( KheResourceResourceType(r) == rt )
      res++;
  }

  if( KheResourceGroupsDisjoint(lbtc, rt, &count) )
  {
    ** the resource groups are disjoint, so their total size will do **
    if( count > res )
      res = count;
  }
  else
  {
    ** otherwise the sizes of suitable resource groups **
    for( i = 0;  i < KheLimitBusyTimesConstraintResourceGroupCount(lbtc);  i++ )
    {
      rg = KheLimitBusyTimesConstraintResourceGroup(lbtc, i);
      if( KheResourceGroupResourceType(rg) == rt &&
	  KheResourceGroupResourceCount(rg) > res )
	res = KheResourceGroupResourceCount(rg);
    }
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheDoFrame(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc)               */
/*                                                                           */
/*  If the time groups of lbtc partition the cycle, return the frame for it, */
/*  otherwise return a null frame.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_FRAME KheDoFrame(KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc, KHE_SOLN soln)
{
  int i, offset, problem_index1, problem_index2;  KHE_TIME problem_time;
  KHE_TIME_GROUP tg;  KHE_FRAME res;  KHE_FRAME_MAKE fm;
  if( DEBUG3 )
    fprintf(stderr, "[ KheDoFrame(...)\n");
  if( DEBUG9 )
    fprintf(stderr, "  calling KheFrameMakeBegin from KheDoFrame\n");
  fm = KheFrameMakeBegin(soln);
  if( KheLimitBusyTimesConstraintAppliesToTimeGroup(lbtc) == NULL )
  {
    /* no applies to time group, so use all the time groups */
    for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(lbtc);  i++ )
    {
      tg = KheLimitBusyTimesConstraintTimeGroup(lbtc, i, 0);
      KheFrameMakeAddTimeGroup(fm, tg);
      /* ***
      if( !KheFrameIsDisjoint(res) )
      {
	KheFrameDelete(res);
	if( DEBUG3 )
	  fprintf(stderr, "] KheDoFrame ret false (1)\n");
	return KheFrameMakeNull();
      }
      *** */
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
      KheFrameMakeAddTimeGroup(fm, tg);
      /* ***
      if( !KheFrameIsDisjoint(res) )
      {
	KheFrameDelete(res);
	if( DEBUG3 )
	  fprintf(stderr, "] KheDoFrame ret false (2)\n");
	return KheFrameMakeNull();
      }
      *** */
    }
  }
  res = KheFrameMakeEnd(fm, true);
  if( !KheFrameIsDisjoint(res, &problem_index1, &problem_index2) )
  {
    KheFrameDelete(res);
    if( DEBUG3 )
      fprintf(stderr, "] KheDoFrame ret false (3)\n");
    return NULL;
    /* return KheFrameMakeNull(); */
  }
  if( !KheFrameIsComplete(res, &problem_time) )
  {
    KheFrameDelete(res);
    if( DEBUG3 )
      fprintf(stderr, "] KheDoFrame ret false (4)\n");
    return NULL;
    /* return KheFrameMakeNull(); */
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

KHE_FRAME KheGetFrame(KHE_RESOURCE_TYPE rt, KHE_SOLN soln)
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
	res = KheDoFrame(lbtc, soln);
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
/*  KHE_FRAME KheFrameMakeCommon(KHE_INSTANCE ins)                           */
/*                                                                           */
/*  Make a common frame for the resources of ins.                            */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeCommon(KHE_SOLN soln)
{
  KHE_FRAME res, res2;  int i;  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheFrameMakeCommon(%s)\n", KheInstanceId(ins));
  res = NULL;
  /* res = KheFrameMakeNull(); */
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    if( KheResourceTypeResourceCount(rt) > 0 )
    {
      res2 = KheGetFrame(rt, soln);
      if( DEBUG1 )
      {
	fprintf(stderr, "  %s: ", KheResourceTypeId(rt));
	KheFrameDebug(res2, 1, 0, stderr);
      }
      if( res2 == NULL )
      {
	if( res != NULL )
	{
	  KheFrameDelete(res);
	  res = NULL;
	}
	break;
      }
      else if( res == NULL )
	res = res2;
      else if( KheFrameEqual(res, res2) )
        KheFrameDelete(res2);
      else
      {
        KheFrameDelete(res);
        KheFrameDelete(res2);
	res = NULL;
	break;
      }
    }
  }
  if( DEBUG1 )
  {
    fprintf(stderr, "] KheFrameMakeCommon returning ");
    KheFrameDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other public functions"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameMakeSingletons(KHE_SOLN soln)                          */
/*                                                                           */
/*  Make a frame of singletons.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheFrameMakeSingletons(KHE_SOLN soln)
{
  KHE_FRAME res;  int i;  KHE_TIME t;  KHE_FRAME_MAKE fm;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  if( DEBUG9 )
    fprintf(stderr,"  calling KheFrameMakeBegin from KheFrameMakeSingletons\n");
  fm = KheFrameMakeBegin(soln);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
    KheFrameMakeAddTimeGroup(fm, KheTimeSingletonTimeGroup(t));
  }
  res = KheFrameMakeEnd(fm, false);
  if( DEBUG4 )
  {
    fprintf(stderr, "KheFrameMakeSingletons ");
    KheFrameDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheFrameOption(KHE_OPTIONS options, char *key,                 */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Get from options (or make and add to options) a common frame.            */
/*                                                                           */
/*****************************************************************************/

/* *** have to move this to solvers now
KHE_FRAME KheFrameOption(KHE_OPTIONS options, char *key, KHE_SOLN soln)
{
  KHE_FRAME *value, res;
  value = (KHE_FRAME *) KheOptionsGetObject(options, key, NULL);
  if( value != NULL )
    res = *value;
  else
  {
    res = KheFrameMakeCommon(soln);
    if( KheFrameIsNull(res) )
      res = KheFrameMakeSingletons(soln);
    MMakeCount(value, 52);
    *value = res;
    KheOptionsSetObject(options, key, (void *) value);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheMeetMinWorkload(PerTimeKHE_MEET meet, KHE_RESOURCE_TYPE rt)     */
/*                                                                           */
/*  Return the minimum, over all tasks of meet of type rt, of the            */
/*  workload per time of the task, or FLT_MAX if none.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static float KheMeetMinWorkloadPerTime(KHE_MEET meet, KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_TASK task;  float workload, min_workload;
  min_workload = FLT_MAX;
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    if( KheTaskResourceType(task) == rt )
    {
      workload = KheTaskWorkloadPerTime(task);
      if( workload < min_workload )
	min_workload = workload;
    }
  }
  return min_workload;
}
*** */


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
/*  KHE_FRAME_WORKLOAD KheFrameWo rkloadMake(KHE_FRAME frame,                */
/*    KHE_RESOURCE_TYPE rt, KHE_EVENT_TIMETABLE_MONITOR etm)                 */
/*                                                                           */
/*  Make a frame workload object for frame.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete, replaced by functions in khe_avail.c
KHE_FRAME_WORKLOAD KheFrameWorkl oadMake(KHE_FRAME frame,
  KHE_RESOURCE_TYPE rt, KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_FRAME_WORKLOAD res;  KHE_TIME t;  int i, j, k;  KHE_TIME_GROUP tg;
  KHE_MEET meet;  float workload, min_workload, total;  KHE_SOLN soln;
  HA_ARENA a;

  if( DEBUG5 )
    fprintf(stderr, "[ KheFrameWor kloadMake(frame, %s, options)\n",
      KheResourceTypeId(rt));
  HnAssert(etm != NULL, "KheFrameWork loadMake: no event timetable monitor");
  soln = KheFrameSoln(frame);
  res = KheSolnGetFrameWorkloadFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->min_workloads, a);
  }
  else
  {
    HaArrayClear(res->min_workloads);
  }
  res->soln = soln;
  res->resource_type = rt;
  HaArrayAddLast(res->min_workloads, 0.0);  ** sentinel at left end **

  ** find the minimum workload for each time group **
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    ** find the minimum workload for tg **
    tg = KheFrameTimeGroup(frame, i);
    min_workload = FLT_MAX;
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheEventTimetableMonitorTimeMeetCount(etm, t);  k++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(etm, t, k);
	workload = KheMeetMinWorkloadPerTime(meet, rt);
	if( workload < min_workload )
	  min_workload = workload;
      }
    }

    ** if there is a minimum workload, add it to min_workloads **
    if( min_workload < FLT_MAX )
      HaArrayAddLast(res->min_workloads, min_workload);
  }

  ** sort the minimum workloads into increasing order, and accumulate them **
  HaArraySort(res->min_workloads, &KheIncreasingFloatCmp);
  total = 0.0;
  HaArrayForEach(res->min_workloads, workload, i)
  {
    total += workload;
    HaArrayPut(res->min_workloads, i, total);
  }
  ** HaArrayAddLast(res->min_workloads, FLT_MAX);  sentinel at right end **
  if( DEBUG5 )
  {
    HaArrayForEach(res->min_workloads, workload, i)
    {
      fprintf(stderr, "%s", i == 0 ? "  " : i % 8 == 0 ? "\n  " : ", ");
      if( workload == FLT_MAX )
	fprintf(stderr, "@");
      else
	fprintf(stderr, "%.1f", workload);
    }
    fprintf(stderr, "\n] KheFrameWo rkloadMake returning\n");
  }
  return res;
}
*** */

/* *** old version that gets its etm from options
KHE_FRAME_WORKLOAD KheFrameWorkl oadMake(KHE_FRAME frame,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  KHE_FRAME_WORKLOAD res;  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_TIME t;
  int i, j, k;  KHE_TIME_GROUP tg;  KHE_POLARITY po;  KHE_MEET meet;
  float workload, min_workload, total;

  if( DEBUG5 )
    fprintf(stderr, "[ KheFrameW orkloadMake(frame, %s, options)\n",
      KheResourceTypeId(rt));
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  HnAssert(etm != NULL, "KheFrameWo rkloadMake: no event timetable monitor");
  MMakeCount(res, 53);
  res->resource_type = rt;
  MArrayInit(res->min_workloads);
  HaArrayAddLast(res->min_workloads, 0.0);  ** sentinel at left end **

  ** find the minimum workload for each time group **
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    ** find the minimum workload for tg **
    tg = KheFrameTimeGroup(frame, i, &po);
    min_workload = FLT_MAX;
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheEventTimetableMonitorTimeMeetCount(etm, t);  k++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(etm, t, k);
	workload = KheMeetMinWorkloadPerTime(meet, rt);
	if( workload < min_workload )
	  min_workload = workload;
      }
    }

    ** if there is a minimum workload, add it to min_workloads **
    if( min_workload < FLT_MAX )
      HaArrayAddLast(res->min_workloads, min_workload);
  }

  ** sort the minimum workloads into increasing order, and accumulate them **
  HaArraySort(res->min_workloads, &KheIncreasingFloatCmp);
  total = 0.0;
  HaArrayForEach(res->min_workloads, workload, i)
  {
    total += workload;
    HaArrayPut(res->min_workloads, i, total);
  }
  ** HaArrayAddLast(res->min_workloads, FLT_MAX);  sentinel at right end **
  if( DEBUG5 )
  {
    HaArrayForEach(res->min_workloads, workload, i)
    {
      fprintf(stderr, "%s", i == 0 ? "  " : i % 8 == 0 ? "\n  " : ", ");
      if( workload == FLT_MAX )
	fprintf(stderr, "@");
      else
	fprintf(stderr, "%.1f", workload);
    }
    fprintf(stderr, "\n] KheFrameWor kloadMake returning\n");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameWorkloadMaximum(KHE_FRAME_WORKLOAD fw, float max_workload)   */
/*                                                                           */
/*  Return the maximum number of busy times that a resource can have, if     */
/*  it is to have workload at most max_workload.                             */
/*                                                                           */
/*  Implementation note.  This is the index of the largest element of        */
/*  the frame workload array which is <= max_workload.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheFrameWorkloadMaximum(KHE_FRAME_WORKLOAD fw, float max_workload)
{
  int l, r, mid;  float mid_workload;
  if( DEBUG8 )
    fprintf(stderr, "[ KheFrameWorkloadMaximum(fw, %.1f)\n", max_workload);

  l = 0;
  r = HaArrayCount(fw->min_workloads);
  ** loop invariant: mw[l] <= max_workload && max_workload < mw[r] **
  ** except r may be off the end, which is considered satisfactory **
  while( l < r-1 )
  {
    mid = (l + r) / 2;  ** must be distinct from both l and r **
    mid_workload = HaArray(fw->min_workloads, mid);
    if( DEBUG8 )
      fprintf(stderr, "  [%d, %d], mid = %d, mid_workload = %.1f\n",
	l, r, mid, mid_workload);
    if( max_workload < mid_workload )
      r = mid;
    else ** max_workload >= mid_workload **
      l = mid;
  }
  HnAssert(l == r - 1,
    "KheFrameWorkloadMaximum internal error 1");
  HnAssert(HaArray(fw->min_workloads, l) <= max_workload,
    "KheFrameWorkloadMaximum internal error 2");
  HnAssert(l + 1 >= HaArrayCount(fw->min_workloads) ||
    HaArray(fw->min_workloads, l + 1) > max_workload,
    "KheFrameWorkloadMaximum internal error 3");
  if( DEBUG8 )
    fprintf(stderr, "] KheFrameWorkloadMaximum returning %d\n", l);
  return l;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFrameMatchesClusterBusyTimesMonitor(KHE_FRAME frame,             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return true if the time groups and polarities of m equal those of        */
/*  frame.                                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheFrameMatchesClusterBusyTimesMonitor(KHE_FRAME frame,
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po2;  int i;
  if( KheClusterBusyTimesMonitorTimeGroupCount(m) !=
      KheFrameTimeGroupCount(frame) )
    return false;
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg1 = KheFrameTimeGroup(frame, i);
    tg2 = KheClusterBusyTimesMonitorTimeGroup(m, i, &po2);
    if( po2 != KHE_POSITIVE || !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheFrameResourceMa xBusyTimes(KHE_FRAME frame,                       */
/*    KHE_FRAME_WORKLOAD fw, KHE_RESOURCE r)                                 */
/*                                                                           */
/*  Return the maximum number of times that r can be busy without having     */
/*  a clash in frame or violating any limit (hard or soft) on the total      */
/*  number of assignments.                                                   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete, replaced by functions in khe_avail.c
int KheFrameReso urceMaxBusyTimes(KHE_FRAME frame,
  KHE_FRAME_WORKLOAD fw, KHE_RESOURCE r)
{
  int res, val, i;  KHE_MONITOR m, res_m;  KHE_SOLN soln;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;  KHE_LIMIT_WORKLOAD_CONSTRAINT lwc;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheFrameResourceMaxTimes(-, %s)\n", KheResourceId(r));
    if( KheResourceInstanceIndex(r) == 0 )
      KheFrameDebug(frame, 2, 2, stderr);
  }
  soln = KheFrameSoln(frame);
  res = KheFrameTimeGroupCount(frame);
  res_m = NULL;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( DEBUG7 )
      KheMonitorDebug(m, 2, 2, stderr);
    switch( KheMonitorTag(m) )
    {
      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
	if( KheConstraintWeight((KHE_CONSTRAINT) lbtc) > 0 &&
	    KheLimitBusyTimesConstraintLimitsWholeCycle(lbtc) )
	{
	  val = KheLimitBusyTimesConstraintMaximum(lbtc);
	  if( val < res )
	  {
	    res = val;
	    res_m = m;
	  }
	}
	break;

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	if( KheConstraintWeight((KHE_CONSTRAINT) cbtc) > 0 )
	{
	  if( KheClusterBusyTimesConstraintTimeGroupCount(cbtc) >=
	    KheInstanceTimeCount(KheSolnInstance(soln)) )
	  {
	    ** every time in the instance is a time group in cbtc **
	    if( DEBUG7 )
	      fprintf(stderr, "    cluster weight %d, all times present\n",
		KheConstraintWeight((KHE_CONSTRAINT) cbtc));
	    val = KheClusterBusyTimesConstraintMaximum(cbtc);
	    if( val < res )
	    {
	      res = val;
	      res_m = m;
	    }
	  }
	  else if( KheFrameMatchesClusterBusyTimesMonitor(frame, cbtm) )
	  {
	    if( DEBUG7 )
	      fprintf(stderr, "    cluster weight %d, frames equal\n",
		KheConstraintWeight((KHE_CONSTRAINT) cbtc));
	    val = KheClusterBusyTimesConstraintMaximum(cbtc);
	    if( val < res )
	    {
	      res = val;
	      res_m = m;
	    }
	  }
	}
	break;

	** ***
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	if( KheClusterBusyTimesMonitorTimeGroupCount(cbtm) ==
	    KheFrameTimeGroupCount(frame) )
	{
	  ** only worth doing if sizes are equal **
	  if( DEBUG7 )
	    fprintf(stderr, "    cluster weight %d, frame equal %s\n",
	      KheConstraintWeight((KHE_CONSTRAINT) cbtc),
	      KheFrameEqual(KheClusterBusyTimesMonitorFrame(cbtm), frame) ?
	      "true" : "false");
	  if( KheConstraintWeight((KHE_CONSTRAINT) cbtc) > 0 &&
	      KheFrameEqual(KheClusterBusyTimesMonitorFrame(cbtm), frame) )
	  {
	    val = KheClusterBusyTimesConstraintMaximum(cbtc);
	    if( val < res )
	    {
	      res = val;
	      res_m = m;
	    }
	  }
	}
	break;
	*** **

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	if( fw != NULL )
	{
	  lwm = (KHE_LIMIT_WORKLOAD_MONITOR) m;
	  lwc = KheLimitWorkloadMonitorConstraint(lwm);
	  if( DEBUG7 )
	    fprintf(stderr, "    workload weight %d\n",
	      KheConstraintWeight((KHE_CONSTRAINT) lwc));
	  if( KheConstraintWeight((KHE_CONSTRAINT) lwc) > 0 &&
	      KheLimitWorkloadConstraintLimitsWholeCycle(lwc) )
	  {
	    val = KheFrameWorkloadMaximum(fw,
	      KheLimitWorkloadConstraintMaximum(lwc));
	    if( val < res )
	    {
	      res = val;
	      res_m = m;
	    }
	  }
	}
	break;

      default:

	** ignore other kinds of monitors **
	break;
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheFrameResourceMaxTimes returning %d (%s)\n", res,
      res_m == NULL ? "frame length" : KheMonitorId(res_m));
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameWorkloadDelete(KHE_FRAME_WORKLOAD fw)                       */
/*                                                                           */
/*  Delete fw.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheFrameWorkloadDelete(KHE_FRAME_WORKLOAD fw)
{
  KheSolnAddFrameWorkloadToFreeList(fw->soln, fw);
  ** ***
  MArrayFree(fw->min_workloads);
  MFree(fw);
  *** **
}
*** */


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
/*  bool KheFrameResourceHasClashes(KHE_FRAME frame, KHE_RESOURCE r)         */
/*                                                                           */
/*  Return true if frame has clashes for r.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheFrameResourceHasClashes(KHE_FRAME frame, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i, j, k;
  KHE_TASK first_task, task;  KHE_TIME t;
  rtm = KheResourceTimetableMonitor(KheFrameSoln(frame), r);
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
/*  void KheFrameResourceAssertNoClashes(KHE_FRAME frame, KHE_RESOURCE r)    */
/*                                                                           */
/*  Verify that r has no clashes in frame.  If not, abort.                   */
/*                                                                           */
/*****************************************************************************/

void KheFrameResourceAssertNoClashes(KHE_FRAME frame, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i, j, k;
  KHE_TASK first_task, task;  KHE_TIME t;
  rtm = KheResourceTimetableMonitor(KheFrameSoln(frame), r);
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
/*  void KheFrameAssertNoClashes(KHE_FRAME frame)                            */
/*                                                                           */
/*  Verify that there are no clashes, aborting if not.                       */
/*                                                                           */
/*****************************************************************************/

void KheFrameAssertNoClashes(KHE_FRAME frame)
{
  KHE_SOLN soln;  KHE_INSTANCE ins;  int i;  KHE_RESOURCE r;
  soln = KheFrameSoln(frame);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheFrameResourceAssertNoClashes(frame, r);
  }
}

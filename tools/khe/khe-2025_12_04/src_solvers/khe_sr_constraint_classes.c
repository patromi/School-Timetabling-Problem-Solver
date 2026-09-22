
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
/*  FILE:         khe_sr_constraint_classes.c                                */
/*  DESCRIPTION:  Constraint classes                                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_CONSTRAINT_CLASS_FINDER                                         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;

struct khe_constraint_class_finder_rec {
  HA_ARENA				arena;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_FRAME				days_frame;
  ARRAY_KHE_CONSTRAINT_CLASS		classes;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_CONSTRAINT_PLUS_OFFSET                                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_constraint_plus_offset_rec {
  KHE_CONSTRAINT			constraint;
  int					offset;
} KHE_CONSTRAINT_PLUS_OFFSET;

typedef HA_ARRAY(KHE_CONSTRAINT_PLUS_OFFSET) ARRAY_KHE_CONSTRAINT_PLUS_OFFSET;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_RESOURCE_INFO - info about one resource                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_info_rec {
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET	constraints;
} *KHE_RESOURCE_INFO;

typedef HA_ARRAY(KHE_RESOURCE_INFO) ARRAY_RESOURCE_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_CONSTRAINT_CLASS                                                */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CONSTRAINT) ARRAY_KHE_CONSTRAINT;

struct khe_constraint_class_rec {
  KHE_CONSTRAINT_CLASS_FINDER		ccf;
  KHE_CONSTRAINT_TAG			tag;
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET	constraints;
  int					resource_count;
  ARRAY_RESOURCE_INFO			resource_info;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_CLASS_FINDER (public functions)"               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS_FINDER KheConstraintClassFinderMake(                */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)                */
/*                                                                           */
/*  Return a new constraint class finder object with these attributes.       */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT_CLASS_FINDER KheConstraintClassFinderMake(
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)
{
  KHE_CONSTRAINT_CLASS_FINDER res;
  HaMake(res, a);
  res->arena = a;
  res->resource_type = rt;
  res->days_frame = days_frame;
  HaArrayInit(res->classes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(                  */
/*    KHE_CONSTRAINT c, int offset)                                          */
/*                                                                           */
/*  Make a new constraint plus offset object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(
  KHE_CONSTRAINT c, int offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET res;
  res.constraint = c;
  res.offset = offset;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetEquivalentClusterBusy(                       */
/*    KHE_CONSTRAINT_PLUS_OFFSET co1, KHE_CONSTRAINT_PLUS_OFFSET co2)        */
/*                                                                           */
/*  Return true if these two constraints can go into the same class,         */
/*  assuming that they both contain cluster busy times constraints.          */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetEquivalentClusterBusy(
  KHE_CONSTRAINT_PLUS_OFFSET co1, KHE_CONSTRAINT_PLUS_OFFSET co2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int i, count1, count2;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc1, cbtc2;
  cbtc1 = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co1.constraint;
  cbtc2 = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co2.constraint;
  count1 = KheClusterBusyTimesConstraintTimeGroupCount(cbtc1);
  count2 = KheClusterBusyTimesConstraintTimeGroupCount(cbtc2);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheClusterBusyTimesConstraintTimeGroup(cbtc1, i, co1.offset, &po1);
    tg2 = KheClusterBusyTimesConstraintTimeGroup(cbtc2, i, co2.offset, &po2);
    if( !KheTimeGroupEqual(tg1, tg2) || po1 != po2 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetEquivalentLimitActive(                       */
/*    KHE_CONSTRAINT_PLUS_OFFSET co1, KHE_CONSTRAINT_PLUS_OFFSET co2)        */
/*                                                                           */
/*  Return true if these two constraints can go into the same class,         */
/*  assuming that they both contain limit active intervals constraints.      */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetEquivalentLimitActive(
  KHE_CONSTRAINT_PLUS_OFFSET co1, KHE_CONSTRAINT_PLUS_OFFSET co2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int i, count1, count2;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic1, laic2;
  laic1 = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co1.constraint;
  laic2 = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co2.constraint;
  count1 = KheLimitActiveIntervalsConstraintTimeGroupCount(laic1);
  count2 = KheLimitActiveIntervalsConstraintTimeGroupCount(laic2);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheLimitActiveIntervalsConstraintTimeGroup(laic1, i,co1.offset,&po1);
    tg2 = KheLimitActiveIntervalsConstraintTimeGroup(laic2, i,co2.offset,&po2);
    if( !KheTimeGroupEqual(tg1, tg2) || po1 != po2 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAcceptsConstraint(KHE_CONSTRAINT_CLASS cc,        */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Return true if co is compatible with cc.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassAcceptsConstraint(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_PLUS_OFFSET co)
{
  KHE_CONSTRAINT_PLUS_OFFSET co2;
  if( cc->tag != KheConstraintTag(co.constraint) )
    return false;
  co2 = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheConstraintPlusOffsetEquivalentClusterBusy(co, co2);
  else
    return KheConstraintPlusOffsetEquivalentLimitActive(co, co2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassAddConstraint(KHE_CONSTRAINT_CLASS cc,            */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Add co to cc.  It's known to be compatible.                              */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassAddConstraint(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_PLUS_OFFSET co)
{
  HaArrayAddLast(cc->constraints, co);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS KheConstraintClassMake(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, KHE_CONSTRAINT_CLASS_FINDER ccf)        */
/*                                                                           */
/*  Make a new constraint class object holding co.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_CLASS KheConstraintClassMake(
  KHE_CONSTRAINT_PLUS_OFFSET co, KHE_CONSTRAINT_CLASS_FINDER ccf)
{
  KHE_CONSTRAINT_CLASS res;
  HaMake(res, ccf->arena);
  res->ccf = ccf;
  res->tag = KheConstraintTag(co.constraint);
  HaArrayInit(res->constraints, ccf->arena);
  HaArrayAddLast(res->constraints, co);
  res->resource_count = 0;
  HaArrayInit(res->resource_info, ccf->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassFinderAcceptsConstraint(                          */
/*    KHE_CONSTRAINT_CLASS_FINDER ccf, KHE_CONSTRAINT c)                     */
/*                                                                           */
/*  Return true if ccf accepts c.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassFinderAcceptsConstraint(
  KHE_CONSTRAINT_CLASS_FINDER ccf, KHE_CONSTRAINT c)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  KHE_RESOURCE_TYPE rt = ccf->resource_type;
  switch( KheConstraintTag(c) )
  {
    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      return KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc, rt) > 0;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      return KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic, rt) > 0;

    default:

      HnAbort("KheConstraintClassFinderAddConstraint: constraint has type %s",
	KheConstraintTagShow(KheConstraintTag(c)));
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassFinderAddConstraint(                              */
/*    KHE_CONSTRAINT_CLASS_FINDER ccf, KHE_CONSTRAINT c, int offset)         */
/*                                                                           */
/*  Add c plus offset to ccf.                                                */
/*                                                                           */
/*****************************************************************************/

void KheConstraintClassFinderAddConstraint(
  KHE_CONSTRAINT_CLASS_FINDER ccf, KHE_CONSTRAINT c, int offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i;  KHE_CONSTRAINT_CLASS cc;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheConstraintClassFinderAddConstraint(ccf, %s, %d)\n",
      KheConstraintId(c), offset);
    KheConstraintClassFinderDebug(ccf, 2, 2, stderr);
  }

  if( KheConstraintClassFinderAcceptsConstraint(ccf, c) )
  {
    /* make a constraint plus offset object */
    co = KheConstraintPlusOffsetMake(c, offset);

    /* try to add co to an existing constraint class */
    HaArrayForEach(ccf->classes, cc, i)
      if( KheConstraintClassAcceptsConstraint(cc, co) )
      {
	KheConstraintClassAddConstraint(cc, co);
	if( DEBUG1 )
	  fprintf(stderr, "] KheConstraintClassFinderAddConstraint returning"
	    " (added to existing class)\n");
	return;
      }

    /* no luck, so make a new constraint class for co */
    cc = KheConstraintClassMake(co, ccf);
    HaArrayAddLast(ccf->classes, cc);
    if( DEBUG1 )
      fprintf(stderr, "  added to new class\n");
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheConstraintClassFinderAddConstraint returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupIsDaySubset(KHE_TIME_GROUP tg, KHE_FRAME days_frame,    */
/*    int *day_index)                                                        */
/*                                                                           */
/*  If tg is a subset of one of the days of days_frame, set *day_index       */
/*  to the index of that day and return true.  Otherwise set *day_index      */
/*  to -1 and return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupIsDaySubset(KHE_TIME_GROUP tg, KHE_FRAME days_frame,
  bool exact_days, int *day_index)
{
  KHE_TIME time;  int index;  KHE_TIME_GROUP frame_tg;

  /* get the first time of tg, or return false if none */
  if( KheTimeGroupTimeCount(tg) == 0 )
    return *day_index = -1, false;
  time = KheTimeGroupTime(tg, 0);

  /* get the index of the day containing time, and the time group there */
  index = KheFrameTimeIndex(days_frame, time);
  frame_tg = KheFrameTimeGroup(days_frame, index);

  /* return true or false depending on whether tg is a subset of frame_tg */
  if( exact_days )
  {
    if( KheTimeGroupEqual(tg, frame_tg) )
      return *day_index = index, true;
    else
      return *day_index = -1, false;
  }
  else
  {
    if( KheTimeGroupSubset(tg, frame_tg) )
      return *day_index = index, true;
    else
      return *day_index = -1, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool DayIndexesAdjacent(int day_index1, int day_index2)                  */
/*                                                                           */
/*  Return true if day_index1 and day_index2 are adjacent.                   */
/*                                                                           */
/*****************************************************************************/

static bool DayIndexesAdjacent(int day_index1, int day_index2)
{
  int diff;
  diff = day_index1 - day_index2;
  return diff == 1 || diff == -1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetIsCompleteWeekend(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if cbtc plus offset defines a complete weekend constraint.   */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetIsCompleteWeekend(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,
  KHE_FRAME days_frame, bool exact_days)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int day_index1, day_index2;

  /* must have exactly two time groups, both positive */
  if( KheClusterBusyTimesConstraintTimeGroupCount(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllPositive(cbtc) )
    return false;

  /* must have min limit 2, max limit 2, and allow-zero flag true */
  if( KheClusterBusyTimesConstraintMinimum(cbtc) != 2 ||
      KheClusterBusyTimesConstraintMaximum(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllowZero(cbtc) )
    return false;

  /* the time groups must be subsets of adjacent days of the common frame */
  tg1 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 0, offset, &po1);
  tg2 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 1, offset, &po2);
  return KheTimeGroupIsDaySubset(tg1, days_frame, exact_days, &day_index1) &&
    KheTimeGroupIsDaySubset(tg2, days_frame, exact_days, &day_index2) &&
    DayIndexesAdjacent(day_index1, day_index2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassFinderAddCompleteWeekendsConstraints(             */
/*    KHE_CONSTRAINT_CLASS_FINDER ccf)                                       */
/*                                                                           */
/*  Add complete weekends constraints to ccf.                                */
/*                                                                           */
/*****************************************************************************/

void KheConstraintClassFinderAddCompleteWeekendsConstraints(
  KHE_CONSTRAINT_CLASS_FINDER ccf, bool exact_days)
{
  KHE_INSTANCE ins;  int i, j, offset, count;  KHE_RESOURCE_TYPE rt;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  rt = ccf->resource_type;
  ins = KheResourceTypeInstance(rt);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > 0 )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc, rt) > 0 )
      {
	count = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc);
	for( j = 0;  j < count;  j++ )
	{
	  offset = KheClusterBusyTimesConstraintAppliesToOffset(cbtc, j);
	  if( KheConstraintPlusOffsetIsCompleteWeekend(cbtc, offset,
		ccf->days_frame, exact_days) )
	  {
	    /* have complete weekends constraint plus offset, add to ws */
	    KheConstraintClassFinderAddConstraint(ccf, c, offset);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsAllSingletonsClusterBusy(                    */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc)                                */
/*                                                                           */
/*  Implement KheConstraintTimeGroupsAllSingletons for cluster busy.         */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsAllSingletonsClusterBusy(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc)
{
  int count, i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  count = KheClusterBusyTimesConstraintTimeGroupCount(cbtc);
  for( i = 0;  i < count;  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, 0, &po);
    if( KheTimeGroupTimeCount(tg) != 1 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsAllSingletonsLimitActive(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic)                            */
/*                                                                           */
/*  Implement KheConstraintTimeGroupsAllSingletons for limit active.         */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsAllSingletonsLimitActive(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic)
{
  int count, i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  for( i = 0;  i < count;  i++ )
  {
    tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, 0, &po);
    if( KheTimeGroupTimeCount(tg) != 1 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsAllSingletons(KHE_CONSTRAINT c)              */
/*                                                                           */
/*  Return true if the time groups of c are all singletons.                  */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintTimeGroupsAllSingletons(KHE_CONSTRAINT c)
{
  switch( KheConstraintTag(c) )
  {
    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsAllSingletonsClusterBusy(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsAllSingletonsLimitActive(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);

    default:

      HnAbort("KheConstraintTimeGroupsAllSingletons: constraint has type %s",
	KheConstraintTagShow(KheConstraintTag(c)));
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsEqualFrameClusterBusy(                       */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Implement KheConstraintTimeGroupsEqualFrame for cluster busy.            */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsEqualFrameClusterBusy(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,
  KHE_FRAME days_frame)
{
  int count, i;  KHE_TIME_GROUP c_tg, frame_tg;  KHE_POLARITY po;
  count = KheClusterBusyTimesConstraintTimeGroupCount(cbtc);
  if( count != KheFrameTimeGroupCount(days_frame) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, offset, &po);
    frame_tg = KheFrameTimeGroup(days_frame, i);
    if( !KheTimeGroupEqual(c_tg, frame_tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsEqualFrameLimitActive(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,                */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Implement KheConstraintTimeGroupsEqualFrame for limit active.            */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsEqualFrameLimitActive(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,
  KHE_FRAME days_frame)
{
  int count, i;  KHE_TIME_GROUP c_tg, frame_tg;  KHE_POLARITY po;
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  if( count != KheFrameTimeGroupCount(days_frame) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, offset, &po);
    frame_tg = KheFrameTimeGroup(days_frame, i);
    if( !KheTimeGroupEqual(c_tg, frame_tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsEqualFrame(KHE_CONSTRAINT c, int offset,     */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if the time groups of c are the same as the time groups      */
/*  of days_frame.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintTimeGroupsEqualFrame(KHE_CONSTRAINT c, int offset,
  KHE_FRAME days_frame)
{
  switch( KheConstraintTag(c) )
  {
    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsEqualFrameClusterBusy(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, offset, days_frame);

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsEqualFrameLimitActive(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, offset, days_frame);

    default:

      HnAbort("KheConstraintTimeGroupsEqualFrame: constraint has type %s",
	KheConstraintTagShow(KheConstraintTag(c)));
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsSubsetFrameClusterBusy(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Implement KheConstraintSubsetFrame for cluster busy.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsSubsetFrameClusterBusy(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,
  KHE_FRAME days_frame)
{
  int count, i;  KHE_TIME_GROUP c_tg, frame_tg;  KHE_POLARITY po;
  count = KheClusterBusyTimesConstraintTimeGroupCount(cbtc);
  if( count != KheFrameTimeGroupCount(days_frame) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, offset, &po);
    frame_tg = KheFrameTimeGroup(days_frame, i);
    if( !KheTimeGroupSubset(c_tg, frame_tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsSubsetFrameLimitActive(                      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,                */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Implement KheConstraintSubsetFrame for limit active.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintTimeGroupsSubsetFrameLimitActive(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,
  KHE_FRAME days_frame)
{
  int count, i;  KHE_TIME_GROUP c_tg, frame_tg;  KHE_POLARITY po;
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  if( count != KheFrameTimeGroupCount(days_frame) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, offset, &po);
    frame_tg = KheFrameTimeGroup(days_frame, i);
    if( !KheTimeGroupSubset(c_tg, frame_tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintTimeGroupsSubsetFrame(KHE_CONSTRAINT c, int offset,    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if the number of time groups of c is the same as the         */
/*  number of time groups of days_frame, and each time group of c is         */
/*  a subset of the corresponding time group of days_frame.                  */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintTimeGroupsSubsetFrame(KHE_CONSTRAINT c, int offset,
  KHE_FRAME days_frame)
{
  switch( KheConstraintTag(c) )
  {
    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsSubsetFrameClusterBusy(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, offset, days_frame);

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      return KheConstraintTimeGroupsSubsetFrameLimitActive(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, offset, days_frame);

    default:

      HnAbort("KheConstraintTimeGroupsSubsetFrame: constraint has type %s",
	KheConstraintTagShow(KheConstraintTag(c)));
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassFinderClassCount(KHE_CONSTRAINT_CLASS_FINDER ccf)  */
/*                                                                           */
/*  Return the number of classes in ccf.                                     */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassFinderClassCount(KHE_CONSTRAINT_CLASS_FINDER ccf)
{
  return HaArrayCount(ccf->classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS KheConstraintClassFinderClass(                      */
/*    KHE_CONSTRAINT_CLASS_FINDER ccf, int i)                                */
/*                                                                           */
/*  Return the i'th class of ccf.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT_CLASS KheConstraintClassFinderClass(
  KHE_CONSTRAINT_CLASS_FINDER ccf, int i)
{
  return HaArray(ccf->classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassFinderDebug(KHE_CONSTRAINT_CLASS_FINDER ccf,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ccf onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheConstraintClassFinderDebug(KHE_CONSTRAINT_CLASS_FINDER ccf,
  int verbosity, int indent, FILE *fp)
{
  KHE_CONSTRAINT_CLASS cc;  int i;
  fprintf(fp, "%*s[ ConstraintClassFinder\n", indent, "");
  HaArrayForEach(ccf->classes, cc, i)
    KheConstraintClassDebug(cc, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_CLASS (resource info)"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_INFO KheResourceInfoMake(KHE_CONSTRAINT_CLASS cc,           */
/*    KHE_RESOURCE r, HA_ARENA a)                                            */
/*                                                                           */
/*  Make a new resource info object for resource r.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_INFO KheResourceInfoMake(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r, HA_ARENA a)
{
  KHE_RESOURCE_INFO res;
  HaMake(res, a);
  HaArrayInit(res->constraints, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassResourceInfoClear(KHE_CONSTRAINT_CLASS cc)        */
/*                                                                           */
/*  Begin resource info making.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassResourceInfoClear(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE_TYPE rt)
{
  HA_ARENA a;  KHE_RESOURCE r;  KHE_RESOURCE_INFO ri;  int i;
  HnAssert(HaArrayCount(cc->resource_info) == 0,
    "KheConstraintClassResourceInfoClear internal error");
  a = cc->ccf->arena;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    ri = KheResourceInfoMake(cc, r, a);
    HaArrayAddLast(cc->resource_info, ri);
  }
  cc->resource_count = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassResourceInfoAdd(KHE_CONSTRAINT_CLASS cc,          */
/*    KHE_RESOURCE r, KHE_CONSTRAINT_PLUS_OFFSET co)                         */
/*                                                                           */
/*  Report that r is a point of application of co.                           */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassResourceInfoAdd(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r, KHE_CONSTRAINT_PLUS_OFFSET co)
{
  int index;  KHE_RESOURCE_INFO ri;
  index = KheResourceResourceTypeIndex(r);
  HnAssert(index < HaArrayCount(cc->resource_info),
    "KheConstraintClassResourceInfoAdd internal error 1");
  ri = HaArray(cc->resource_info, index);
  HnAssert(ri != NULL, "KheConstraintClassResourceInfoAdd internal error 2");
  if( HaArrayCount(ri->constraints) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "    resource_info %s (cc->resource_count %d -> %d)\n",
	KheResourceId(r), cc->resource_count, cc->resource_count + 1);
    cc->resource_count++;
  }
  HaArrayAddLast(ri->constraints, co);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassResourceGroupInfoAdd(KHE_CONSTRAINT_CLASS cc,     */
/*    KHE_RESOURCE_GROUP rg, KHE_CONSTRAINT_PLUS_OFFSET co)                  */
/*                                                                           */
/*  Report that the resources of rg are points of application of co.         */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassResourceGroupInfoAdd(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE_GROUP rg, KHE_RESOURCE_TYPE rt, KHE_CONSTRAINT_PLUS_OFFSET co)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    if( KheResourceResourceType(r) == rt )
      KheConstraintClassResourceInfoAdd(cc, r, co);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassInitResourceInfoClusterBusy(                      */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE_TYPE rt)                         */
/*                                                                           */
/*  Implement KheConstraintClassInitResourceInfo for the cluster busy case.  */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassInitResourceInfoClusterBusy(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE_TYPE rt)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, j, count;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KheConstraintClassResourceInfoClear(cc, rt);
  HaArrayForEach(cc->constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    count = KheClusterBusyTimesConstraintResourceGroupCount(cbtc);
    for( j = 0;  j < count;  j++ )
    {
      rg = KheClusterBusyTimesConstraintResourceGroup(cbtc, j);
      /* this fails sometimes if( KheResourceGroupResourceType(rg) == rt ) */
      KheConstraintClassResourceGroupInfoAdd(cc, rg, rt, co);
    }
    count = KheClusterBusyTimesConstraintResourceCount(cbtc);
    for( j = 0;  j < count;  j++ )
    {
      r = KheClusterBusyTimesConstraintResource(cbtc, j);
      if( KheResourceResourceType(r) == rt )
        KheConstraintClassResourceInfoAdd(cc, r, co);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassInitResourceInfoLimitActive(                      */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE_TYPE rt)                         */
/*                                                                           */
/*  Implement KheConstraintClassInitResourceInfo for the limit active case.  */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassInitResourceInfoLimitActive(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE_TYPE rt)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, j, count;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  KheConstraintClassResourceInfoClear(cc, rt);
  HaArrayForEach(cc->constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    count = KheLimitActiveIntervalsConstraintResourceGroupCount(laic);
    for( j = 0;  j < count;  j++ )
    {
      rg = KheLimitActiveIntervalsConstraintResourceGroup(laic, j);
      /* this fails sometimes if( KheResourceGroupResourceType(rg) == rt ) */
      KheConstraintClassResourceGroupInfoAdd(cc, rg, rt, co);
    }
    count = KheLimitActiveIntervalsConstraintResourceCount(laic);
    for( j = 0;  j < count;  j++ )
    {
      r = KheLimitActiveIntervalsConstraintResource(laic, j);
      if( KheResourceResourceType(r) == rt )
        KheConstraintClassResourceInfoAdd(cc, r, co);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassInitResourceInfo(KHE_CONSTRAINT_CLASS cc)         */
/*                                                                           */
/*  Initialize the resource info of cc.  Do nothing if already initialized.  */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassInitResourceInfo(KHE_CONSTRAINT_CLASS cc)
{
  KHE_RESOURCE_TYPE rt = cc->ccf->resource_type;
  if( HaArrayCount(cc->resource_info) == 0 )
  {
    if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
      KheConstraintClassInitResourceInfoClusterBusy(cc, rt);
    else
      KheConstraintClassInitResourceInfoLimitActive(cc, rt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_CLASS (public functions)"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassConstraintCount(KHE_CONSTRAINT_CLASS cc)           */
/*                                                                           */
/*  Return the number of constraint plus offset objects in cc.               */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassConstraintCount(KHE_CONSTRAINT_CLASS cc)
{
  return HaArrayCount(cc->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheConstraintClassConstraint(KHE_CONSTRAINT_CLASS cc,     */
/*    int i, int *offset)                                                    */
/*                                                                           */
/*  Return the i'th constraint of cc, along with its offset.                 */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheConstraintClassConstraint(KHE_CONSTRAINT_CLASS cc,
  int i, int *offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  co = HaArray(cc->constraints, i);
  return *offset = co.offset, co.constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassCoversResourceType(KHE_CONSTRAINT_CLASS cc)       */
/*                                                                           */
/*  Return true if cc covers all of ccf's resource type;                     */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassCoversResourceType(KHE_CONSTRAINT_CLASS cc)
{
  int rt_count;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheConstraintClassCoversResourceType(cc)\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
  }
  KheConstraintClassInitResourceInfo(cc);
  rt_count = KheResourceTypeResourceCount(cc->ccf->resource_type);
  if( DEBUG2 )
    fprintf(stderr, "] KheConstraintClassCoversResourceType returning %s "
      "(cc count %d, rt count %d)\n", bool_show(cc->resource_count == rt_count),
      cc->resource_count, rt_count);
  return cc->resource_count == rt_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_TAG KheConstraintClassTag(KHE_CONSTRAINT_CLASS cc)        */
/*                                                                           */
/*  Return the constraint tag of the constraints of cc.                      */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT_TAG KheConstraintClassTag(KHE_CONSTRAINT_CLASS cc)
{
  return cc->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheConstraintClassId(KHE_CONSTRAINT_CLASS cc)                      */
/*                                                                           */
/*  Return the Id of cc.                                                     */
/*                                                                           */
/*****************************************************************************/

char *KheConstraintClassId(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  co = HaArrayFirst(cc->constraints);
  return KheConstraintId(co.constraint);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassTimeGroupCount(KHE_CONSTRAINT_CLASS cc)            */
/*                                                                           */
/*  Return the number of time groups in each constraint of cc.               */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassTimeGroupCount(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintTimeGroupCount(cbtc);
  }
  else
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    return KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheConstraintClassTimeGroup(KHE_CONSTRAINT_CLASS cc,      */
/*    int i, KHE_POLARITY *po)                                               */
/*                                                                           */
/*  Return the i'th time group of cc's constraints, plus its polarity.       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheConstraintClassTimeGroup(KHE_CONSTRAINT_CLASS cc,
  int i, KHE_POLARITY *po)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintTimeGroup(cbtc, i, co.offset, po);
  }
  else
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    return KheLimitActiveIntervalsConstraintTimeGroup(laic, i, co.offset, po);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAllPositive(KHE_CONSTRAINT_CLASS cc)              */
/*                                                                           */
/*  Return true if the time groups of the constraints of cc are all          */
/*  positive.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassAllPositive(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintAllPositive(cbtc);
  }
  else
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    return KheLimitActiveIntervalsConstraintAllPositive(laic);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAllNegative(KHE_CONSTRAINT_CLASS cc)              */
/*                                                                           */
/*  Return true if the time groups of the constraints of cc are all          */
/*  negative.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassAllNegative(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintAllNegative(cbtc);
  }
  else
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    return KheLimitActiveIntervalsConstraintAllNegative(laic);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassTimeGroupsDisjoint(KHE_CONSTRAINT_CLASS cc)       */
/*                                                                           */
/*  Return true if the time groups of the constraints of cc are pairwise     */
/*  disjoint.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassTimeGroupsDisjoint(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintTimeGroupsDisjoint(cbtc);
  }
  else
  {
    HnAbort("KheConstraintClassTimeGroupsDisjoint not implemented for"
      " limit active intervals constraints");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassTimeGroupsCoverWholeCycle(KHE_CONSTRAINT_CLASS cc)*/
/*                                                                           */
/*  Return true if the time groups of each constraint of cc cover the        */
/*  whole cycle.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassTimeGroupsCoverWholeCycle(KHE_CONSTRAINT_CLASS cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  co = HaArrayFirst(cc->constraints);
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    return KheClusterBusyTimesConstraintTimeGroupsCoverWholeCycle(cbtc);
  }
  else
  {
    HnAbort("KheConstraintClassTimeGroupsCoverWholeCycle not implemented for"
      " limit active intervals constraints");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassHasUniformLimits(KHE_CONSTRAINT_CLASS cc)         */
/*                                                                           */
/*  Return true if cc has uniform limits, that is, if every resource         */
/*  affected by cc has the same minimum and maximum limits.                  */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassHasUniformLimits(KHE_CONSTRAINT_CLASS cc)
{
  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;  int i, count, minimum, maximum;

  /* true if there is one constraint and it covers every resource */
  if( HaArrayCount(cc->constraints) == 1 &&
      KheConstraintClassCoversResourceType(cc) )
    return true;

  /* do the full calculation, getting a min and max for each resource */
  rt = cc->ccf->resource_type;
  count = KheResourceTypeResourceCount(rt);
  if( count == 0 )
    return true;
  r = KheResourceTypeResource(rt, 0);
  minimum = KheConstraintClassResourceMinimum(cc, r);
  maximum = KheConstraintClassResourceMaximum(cc, r);
  for( i = 1;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheConstraintClassResourceMinimum(cc, r) != minimum )
      return false;
    if( KheConstraintClassResourceMaximum(cc, r) != maximum )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoAllowZeroClusterBusy(                                          */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the allow_zero value for these cluster busy times constraints.  */
/*                                                                           */
/*****************************************************************************/

static bool KheDoAllowZeroClusterBusy(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  int i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  HaArrayForEach(*constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    if( !KheClusterBusyTimesConstraintAllowZero(cbtc) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoAllowZero(KHE_CONSTRAINT_TAG tag,                              */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the allow_zero value for these constraints.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheDoAllowZero(KHE_CONSTRAINT_TAG tag,
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  if( tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheDoAllowZeroClusterBusy(constraints);
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAllowZero(KHE_CONSTRAINT_CLASS cc)                */
/*                                                                           */
/*  Return true if all of cc's constraints allow zero.                       */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassAllowZero(KHE_CONSTRAINT_CLASS cc)
{
  return KheDoAllowZero(cc->tag, &cc->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMinimumClusterBusy(                                             */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the minimum value for these cluster busy times constraints.     */
/*                                                                           */
/*****************************************************************************/

static int KheDoMinimumClusterBusy(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  int res, minimum, i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  res = 0;
  HaArrayForEach(*constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    minimum = KheClusterBusyTimesConstraintMinimum(cbtc);
    if( minimum > res )
      res = minimum;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMinimumLimitActive(                                             */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the minimum value for these limit active intervals constraints. */
/*                                                                           */
/*****************************************************************************/

static int KheDoMinimumLimitActive(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  int res, minimum, i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  res = 0;
  HaArrayForEach(*constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    minimum = KheLimitActiveIntervalsConstraintMinimum(laic);
    if( minimum > res )
      res = minimum;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMinimum(KHE_CONSTRAINT_TAG tag,                                 */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the minimum value for these constraints.                        */
/*                                                                           */
/*****************************************************************************/

static int KheDoMinimum(KHE_CONSTRAINT_TAG tag,
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  if( tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheDoMinimumClusterBusy(constraints);
  else
    return KheDoMinimumLimitActive(constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassMinimum(KHE_CONSTRAINT_CLASS cc)                   */
/*                                                                           */
/*  Return the smallest of the minimum values of cc's constraints.           */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassMinimum(KHE_CONSTRAINT_CLASS cc)
{
  return KheDoMinimum(cc->tag, &cc->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMaximumClusterBusy(                                             */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the maximum value for these cluster busy times constraints.     */
/*                                                                           */
/*****************************************************************************/

static int KheDoMaximumClusterBusy(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  int res, maximum, i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  res = INT_MAX;
  HaArrayForEach(*constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    maximum = KheClusterBusyTimesConstraintMaximum(cbtc);
    if( maximum < res )
      res = maximum;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMaximumCLimitActive(                                             */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the maximum value for these cluster busy times constraints.     */
/*                                                                           */
/*****************************************************************************/

static int KheDoMaximumLimitActive(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  int res, maximum, i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  res = INT_MAX;
  HaArrayForEach(*constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    maximum = KheLimitActiveIntervalsConstraintMaximum(laic);
    if( maximum < res )
      res = maximum;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoMaximum(KHE_CONSTRAINT_TAG tag,                                 */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Work out the maximum value for these constraints.                        */
/*                                                                           */
/*****************************************************************************/

static int KheDoMaximum(KHE_CONSTRAINT_TAG tag,
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  if( tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheDoMaximumClusterBusy(constraints);
  else
    return KheDoMaximumLimitActive(constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassMaximum(KHE_CONSTRAINT_CLASS cc)                   */
/*                                                                           */
/*  Return the largest of the maximum values of cc's constraints.            */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassMaximum(KHE_CONSTRAINT_CLASS cc)
{
  return KheDoMaximum(cc->tag, &cc->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDoCombinedWeight(                                            */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)                         */
/*                                                                           */
/*  Find the total combined weight of *constraints.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheDoCombinedWeight(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints)
{
  KHE_COST res;  int i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  res = 0;
  HaArrayForEach(*constraints, co, i)
    res += KheConstraintCombinedWeight(co.constraint);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassCombinedWeight(KHE_CONSTRAINT_CLASS cc)       */
/*                                                                           */
/*  Return the sum of the combined weights of cc's constraints.              */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintClassCombinedWeight(KHE_CONSTRAINT_CLASS cc)
{
  return KheDoCombinedWeight(&cc->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDoDevToCost(ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,   */
/*    int dev)                                                               */
/*                                                                           */
/*  Return the total cost of this deviation over *constraints.               */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
static KHE_COST KheDoDevToCost(ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,
  int dev)
{
  KHE_COST res;  int i;  KHE_CONSTRAINT_PLUS_OFFSET co;
  res = 0;
  HaArrayForEach(*constraints, co, i)
    res += KheConstraintDevToCost(co.constraint, dev);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassDevToCost(KHE_CONSTRAINT_CLASS cc, int dev)   */
/*                                                                           */
/*  Convert dev into a cost.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
KHE_COST KheConstraintClassDevToCost(KHE_CONSTRAINT_CLASS cc, int dev)
{
  return KheDoDevToCost(&cc->constraints, dev);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDoDeterminantToCostClusterBusy(                              */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints, int determinant)        */
/*                                                                           */
/*  Convert determinant into a cost for *constraints, which are cluster busy.*/
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheDoDeterminantToCostClusterBusy(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints, int determinant)
{
  KHE_COST res;  int i, minimum, maximum;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  bool allow_zero;
  res = 0;
  HaArrayForEach(*constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    minimum = KheClusterBusyTimesConstraintMinimum(cbtc);
    maximum = KheClusterBusyTimesConstraintMaximum(cbtc);
    allow_zero = KheClusterBusyTimesConstraintAllowZero(cbtc);
    if( allow_zero && determinant == 0 )
      res += 0;
    else if( determinant < minimum )
      res += KheConstraintDevToCost((KHE_CONSTRAINT) cbtc, minimum-determinant);
    else if( determinant > maximum )
      res += KheConstraintDevToCost((KHE_CONSTRAINT) cbtc, determinant-maximum);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDoDeterminantToCostLimitActive(                              */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,                         */
/*    int determinant, bool at_end)                                          */
/*                                                                           */
/*  Convert determinant into a cost for *constraints, which are limit        */
/*  active.  If at_end is true, we are at the end of the sequence of time    */
/*  groups, so undersized sequences have cost 0 if there is history after.   */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheDoDeterminantToCostLimitActive(
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,
  int determinant, bool at_end)
{
  KHE_COST res;  int i, minimum, maximum;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  res = 0;
  HaArrayForEach(*constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    minimum = KheLimitActiveIntervalsConstraintMinimum(laic);
    maximum = KheLimitActiveIntervalsConstraintMaximum(laic);
    if( determinant < minimum )
    {
      if( !at_end || KheLimitActiveIntervalsConstraintHistoryAfter(laic) == 0 )
	res += KheConstraintDevToCost((KHE_CONSTRAINT)laic,minimum-determinant);
    }
    else if( determinant > maximum )
      res += KheConstraintDevToCost((KHE_CONSTRAINT) laic, determinant-maximum);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDoDeterminantToCost(KHE_CONSTRAINT_TAG tag,                  */
/*    ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,                         */
/*    int determinant, bool at_end)                                          */
/*                                                                           */
/*  Return the total cost of this determinant and at_end for *constraints.   */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheDoDeterminantToCost(KHE_CONSTRAINT_TAG tag,
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET *constraints,
  int determinant, bool at_end)
{
  if( tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheDoDeterminantToCostClusterBusy(constraints, determinant);
  else
    return KheDoDeterminantToCostLimitActive(constraints, determinant, at_end);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassDeterminantToCost(KHE_CONSTRAINT_CLASS cc,    */
/*    int determinant, bool at_end)                                          */
/*                                                                           */
/*  Return the total cost of this determinant and at_end for cc.             */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintClassDeterminantToCost(KHE_CONSTRAINT_CLASS cc,
  int determinant, bool at_end)
{
  return KheDoDeterminantToCost(cc->tag, &cc->constraints, determinant,
    at_end);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_INFO KheConstraintClassResourceInfo(                        */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Get cc's resource info for r.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_INFO KheConstraintClassResourceInfo(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  int index;
  HnAssert(KheResourceResourceType(r) == cc->ccf->resource_type,
    "KheConstraintClass: resource %s does not have type %s",
    KheResourceId(r), KheResourceTypeId(cc->ccf->resource_type));
  KheConstraintClassInitResourceInfo(cc);
  index = KheResourceResourceTypeIndex(r);
  return HaArray(cc->resource_info, index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassResourceAllowZero(KHE_CONSTRAINT_CLASS cc,        */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Like KheConstraintClassAllowZero but for constraints applying to r.      */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintClassResourceAllowZero(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoAllowZero(cc->tag, &ri->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceMinimum(KHE_CONSTRAINT_CLASS cc,           */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Like KheConstraintClassMinimum but for constraints applying to r.        */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassResourceMinimum(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoMinimum(cc->tag, &ri->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceMaximum(KHE_CONSTRAINT_CLASS cc,           */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Like KheConstraintClassMaximum but for constraints applying to r.        */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassResourceMaximum(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoMaximum(cc->tag, &ri->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassResourceCombinedWeight(                       */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Like KheConstraintClassCombinedWeight but for constraints applying to r. */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintClassResourceCombinedWeight(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoCombinedWeight(&ri->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassResourceDevToCost(KHE_CONSTRAINT_CLASS cc,    */
/*    KHE_RESOURCE r, int dev)                                               */
/*                                                                           */
/*  Like KheConstraintClassDevToCost but for constraints applying to r.      */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
KHE_COST KheConstraintClassResourceDevToCost(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r, int dev)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoDevToCost(&ri->constraints, dev);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintClassResourceDeterminantToCost(                    */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r, int determinant, bool at_end) */
/*                                                                           */
/*  Like KheConstraintClassDeterminantToCost but for constraints for r.      */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintClassResourceDeterminantToCost(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r, int determinant, bool at_end)
{
  KHE_RESOURCE_INFO ri;
  ri = KheConstraintClassResourceInfo(cc, r);
  return KheDoDeterminantToCost(cc->tag, &ri->constraints,
    determinant, at_end);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceHistoryClusterBusy(                        */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Carry out KheConstraintClassHistory for the cluster busy case.           */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintClassResourceHistoryClusterBusy(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, history, res;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  res = 0;
  HaArrayForEach(cc->constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    history = KheClusterBusyTimesConstraintHistory(cbtc, r);
    if( history > res )
      res = history;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceHistoryLimitActive(                        */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Carry out KheConstraintClassHistory for the limit active case.           */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintClassResourceHistoryLimitActive(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, history, res;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  res = 0;
  HaArrayForEach(cc->constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    history = KheLimitActiveIntervalsConstraintHistory(laic, r);
    if( history > res )
      res = history;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceHistory(KHE_CONSTRAINT_CLASS cc,           */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the maximum value of the history values for r in the constraints  */
/*  of cc.                                                                   */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassResourceHistory(KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheConstraintClassResourceHistoryClusterBusy(cc, r);
  else
    return KheConstraintClassResourceHistoryLimitActive(cc, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceMaximumMinusHistoryClusterBusy(            */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Carry out KheConstraintClassMaximumMinusHistory for cluster busy case.   */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintClassResourceMaximumMinusHistoryClusterBusy(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, maximum, history, val, res;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  res = INT_MAX;
  HaArrayForEach(cc->constraints, co, i)
  {
    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) co.constraint;
    maximum = KheClusterBusyTimesConstraintMaximum(cbtc);
    history = KheClusterBusyTimesConstraintHistory(cbtc, r);
    val = max(0, maximum - history);
    if( val < res )
      res = val;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceMaximumMinusHistoryLimitActive(            */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Carry out KheConstraintClassMaximumMinusHistory for limit active case.   */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintClassResourceMaximumMinusHistoryLimitActive(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, maximum, history, val, res;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  res = INT_MAX;
  HaArrayForEach(cc->constraints, co, i)
  {
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) co.constraint;
    maximum = KheLimitActiveIntervalsConstraintMaximum(laic);
    history = KheLimitActiveIntervalsConstraintHistory(laic, r);
    val = max(0, maximum - history);
    if( DEBUG3 )
      fprintf(stderr, "  %s: max %d, history %d, val %d\n",
	KheConstraintId(co.constraint), maximum, history, val);
    if( val < res )
      res = val;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassResourceMaximumMinusHistory(                       */
/*    KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)                               */
/*                                                                           */
/*  Return the maximum value of the history values for r in the constraints  */
/*  of cc.                                                                   */
/*                                                                           */
/*****************************************************************************/

int KheConstraintClassResourceMaximumMinusHistory(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r)
{
  if( cc->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    return KheConstraintClassResourceMaximumMinusHistoryClusterBusy(cc, r);
  else
    return KheConstraintClassResourceMaximumMinusHistoryLimitActive(cc, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of co onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( co.offset > 0 )
    fprintf(fp, "%s|%d", KheConstraintId((KHE_CONSTRAINT) co.constraint),
      co.offset);
  else
    fprintf(fp, "%s", KheConstraintId((KHE_CONSTRAINT) co.constraint));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassHeaderDebug(KHE_CONSTRAINT_CLASS cc, FILE *fp)    */
/*                                                                           */
/*  Print the header part of cc's debug print onto fp.                       */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassHeaderDebug(KHE_CONSTRAINT_CLASS cc, FILE *fp)
{
  fprintf(fp, "ConstraintClass(%s, %d constraints)",
    KheConstraintClassId(cc), HaArrayCount(cc->constraints));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug printf of cc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheConstraintClassHeaderDebug(cc, fp);
    fprintf(fp, "\n");
    HaArrayForEach(cc->constraints, co, i)
      KheConstraintPlusOffsetDebug(co, 2, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheConstraintClassHeaderDebug(cc, fp);
}

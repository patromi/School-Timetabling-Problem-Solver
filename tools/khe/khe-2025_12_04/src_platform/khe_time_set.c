
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
/*  FILE:         khe_time_set.c                                             */
/*  DESCRIPTION:  Time sets                                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET                                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_time_set_rec
{
  KHE_INSTANCE			ins;			/* encl instance     */
  KHE_SET			time_indexes;		/* the time indexes  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheTimeSetMake(KHE_INSTANCE ins, HA_ARENA a)                */
/*                                                                           */
/*  Make a new, empty time set with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_SET KheTimeSetMake(KHE_INSTANCE ins, HA_ARENA a)
{
  KHE_TIME_SET res;
  HaMake(res, a);
  res->ins = ins;
  KheSetInit(res->time_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheTimeSetCopy(KHE_TIME_SET ts, HA_ARENA a)                 */
/*                                                                           */
/*  Make a fresh copy of ts in a.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_SET KheTimeSetCopy(KHE_TIME_SET ts, HA_ARENA a)
{
  KHE_TIME_SET res;
  HaMake(res, a);
  res->ins = ts->ins;
  KheSetCopy(res->time_indexes, ts->time_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetCopyElements(KHE_TIME_SET dst_ts, KHE_TIME_SET src_ts)    */
/*                                                                           */
/*  Clear dst_ts and copy the elements of src_ts into dst_ts.                */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetCopyElements(KHE_TIME_SET dst_ts, KHE_TIME_SET src_ts)
{
  dst_ts->ins = src_ts->ins;
  KheSetCopyElements(dst_ts->time_indexes, src_ts->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheTimeSetInstance(KHE_TIME_SET ts)                         */
/*                                                                           */
/*  Return the instance attribute of ts.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheTimeSetInstance(KHE_TIME_SET ts)
{
  return ts->ins;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetClear(KHE_TIME_SET ts)                                    */
/*                                                                           */
/*  Clear ts.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetClear(KHE_TIME_SET ts)
{
  KheSetClear(ts->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGetTimeIndex(KHE_TIME t)                                          */
/*                                                                           */
/*  Return the index of t, or -1 if t is NULL.                               */
/*                                                                           */
/*****************************************************************************/

static int KheGetTimeIndex(KHE_TIME t)
{
  return (t == NULL ? -1 : KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetAddTime(KHE_TIME_SET ts, KHE_TIME t)                      */
/*                                                                           */
/*  Add t to ts, or do nothing if t is already present.                      */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetAddTime(KHE_TIME_SET ts, KHE_TIME t)
{
  KheSetInsert(ts->time_indexes, KheGetTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetAddTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg)          */
/*                                                                           */
/*  Add the times of tg to ts.                                               */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetAddTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg)
{
  int i;  KHE_TIME t;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    KheTimeSetAddTime(ts, t);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetAddTaskTimes(KHE_TIME_SET ts, KHE_TASK task)              */
/*                                                                           */
/*  Add the times when task is running to ts.  This also adds the times      */
/*  of tasks assigned, directly or indirectly, to task.                      */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetAddTaskTimes(KHE_TIME_SET ts, KHE_TASK task)
{
  KHE_MEET meet;  KHE_TASK sub_task;  int i;  KHE_TIME starting_time;

  /* add the times of task */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    starting_time = KheMeetAsstTime(meet);
    if( starting_time != NULL )
    {
      for( i = 0;  i < KheMeetDuration(meet);  i++ )
      {
	HnAssert(KheTimeHasNeighbour(starting_time, i),
	  "KheTimeSetAddTaskTimes: time off end");
	KheTimeSetAddTime(ts, KheTimeNeighbour(starting_time, i));
      }
    }
  }

  /* recursively add the times of tasks assigned to task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    sub_task = KheTaskAssignedTo(task, i);
    KheTimeSetAddTaskTimes(ts, sub_task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetDeleteTime(KHE_TIME_SET ts, KHE_TIME t)                   */
/*                                                                           */
/*  Delete t from ts, if present.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetDeleteTime(KHE_TIME_SET ts, KHE_TIME t)
{
  KheSetDelete(ts->time_indexes, KheGetTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetDeleteTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg)       */
/*                                                                           */
/*  Delete tg's times from ts.                                               */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetDeleteTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg)
{
  int i;  KHE_TIME t;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    KheTimeSetDeleteTime(ts, t);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetDeleteLastTime(KHE_TIME_SET ts)                           */
/*                                                                           */
/*  Delete the last element of ts.                                           */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetDeleteLastTime(KHE_TIME_SET ts)
{
  KheSetDeleteLast(ts->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetTimeCount(KHE_TIME_SET ts)                                 */
/*                                                                           */
/*  Return the number of times in ts.                                        */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetTimeCount(KHE_TIME_SET ts)
{
  return KheSetCount(ts->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTimeSetTime(KHE_TIME_SET ts, int i)                          */
/*                                                                           */
/*  Return the i'th time of ts.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheTimeSetTime(KHE_TIME_SET ts, int i)
{
  int index;
  index = KheSetGet(ts->time_indexes, i);
  return (index == -1 ? NULL : KheInstanceTime(ts->ins, index));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetTimeIndex(KHE_TIME_SET ts, int i)                          */
/*                                                                           */
/*  Return the index of the i'th time of ts.                                 */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetTimeIndex(KHE_TIME_SET ts, int i)
{
  return KheSetGet(ts->time_indexes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetUnion(KHE_TIME_SET ts1, KHE_TIME_SET ts2)                 */
/*                                                                           */
/*  Update ts1 to contain the union of its initial value with ts2.           */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetUnion(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  KheSetUnion(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetIntersect(KHE_TIME_SET ts1, KHE_TIME_SET ts2)             */
/*                                                                           */
/*  Update ts1 to contain the intersection of its initial value with ts2.    */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetIntersect(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  KheSetIntersect(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetDifference(KHE_TIME_SET ts1, KHE_TIME_SET ts2)            */
/*                                                                           */
/*  Update ts1 to contain the difference of its initial value with ts2,      */
/*  that is, ts1 - ts2.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetDifference(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  KheSetDifference(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetUnionCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)             */
/*                                                                           */
/*  Return the cardinality of KheTimeSetUnion(ts1, ts2).                     */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetUnionCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetUnionCount(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetIntersectCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)         */
/*                                                                           */
/*  Return the cardinality of KheTimeSetIntersect(ts1, ts2).                 */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetIntersectCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetIntersectCount(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetDifferenceCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)        */
/*                                                                           */
/*  Return the cardinality of KheTimeSetDifference(ts1, ts2).                */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetDifferenceCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetDifferenceCount(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSetEmpty(KHE_TIME_SET ts)                                    */
/*                                                                           */
/*  Return true if ts is empty.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSetEmpty(KHE_TIME_SET ts)
{
  return KheSetEmpty(ts->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSetEqual(KHE_TIME_SET ts1, KHE_TIME_SET ts2)                 */
/*                                                                           */
/*  Return true if ts1 and ts2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSetEqual(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetEqual(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSetSubset(KHE_TIME_SET ts1, KHE_TIME_SET ts2)                */
/*                                                                           */
/*  Return true if ts1 is a subset of ts2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSetSubset(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetSubset(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSetDisjoint(KHE_TIME_SET ts1, KHE_TIME_SET ts2)              */
/*                                                                           */
/*  Return true if ts1 and ts2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSetDisjoint(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetDisjoint(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSetContainsTime(KHE_TIME_SET ts, KHE_TIME t)                 */
/*                                                                           */
/*  Return true if ts contains t.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSetContainsTime(KHE_TIME_SET ts, KHE_TIME t)
{
  return KheSetContains(ts->time_indexes, KheGetTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetHash(KHE_TIME_SET ts)                                      */
/*                                                                           */
/*  Hash ts.                                                                 */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetHash(KHE_TIME_SET ts)
{
  int count;
  count = KheTimeSetTimeCount(ts);
  if( count == 0 )
    return 0;
  else
    return (KheTimeSetTimeIndex(ts, 0) << 16) +
      (KheTimeSetTimeIndex(ts, count / 2) << 8) +
      KheTimeSetTimeIndex(ts, count - 1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Comparison function for sorting time sets.                               */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetCmp(const void *t1, const void *t2)
{
  KHE_TIME_SET ts1 = * (KHE_TIME_SET *) t1;
  KHE_TIME_SET ts2 = * (KHE_TIME_SET *) t2;
  return KheTimeSetTypedCmp(ts1, ts2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetTypedCmp(KHE_TIME_SET ts1, KHE_TIME_SET ts2)               */
/*                                                                           */
/*  Typed version of comparison function.                                    */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetTypedCmp(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return KheSetTypedCmp(ts1->time_indexes, ts2->time_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetCmpReverse(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Like KheTimeSetCmp, only sorts in reverse order.                         */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetCmpReverse(const void *t1, const void *t2)
{
  return - KheTimeSetCmp(t1, t2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeSetTypedCmpReverse(KHE_TIME_SET ts1, KHE_TIME_SET ts2)        */
/*                                                                           */
/*  Like KheTimeSetTypedCmp, only sorts in reverse order.                    */
/*                                                                           */
/*****************************************************************************/

int KheTimeSetTypedCmpReverse(KHE_TIME_SET ts1, KHE_TIME_SET ts2)
{
  return - KheTimeSetTypedCmp(ts1, ts2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSetDebug(KHE_TIME_SET ts, int verbosity, int indent,         */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Produce a debug print of ts on fp with the given verbosity and indent.   */
/*                                                                           */
/*****************************************************************************/

void KheTimeSetDebug(KHE_TIME_SET ts, int verbosity, int indent, FILE *fp)
{
  KHE_TIME t, t2;  int i;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  if( verbosity >= 2 || KheTimeSetTimeCount(ts) <= 2 )
  {
    /* full print */
    for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
    {
      t = KheTimeSetTime(ts, i);
      if( i > 0 )
      {
	if( indent >= 0 && i % 8 == 0 )
	  fprintf(fp, ",\n%*s", indent + 1, "");
	else
	  fprintf(fp, ", ");
      }
      fprintf(fp, "%s", t == NULL ? "NULL" : KheTimeId(t));
    }
  }
  else
  {
    /* abbreviated print; print first and last elements, with ellipsis */
    t = KheTimeSetTime(ts, 0);
    t2 = KheTimeSetTime(ts, KheTimeSetTimeCount(ts) - 1);
    fprintf(fp, "%s, ... %s", t == NULL ? "NULL" : KheTimeId(t),
      KheTimeId(t2));
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}

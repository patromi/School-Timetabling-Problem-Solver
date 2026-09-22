
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
/*  FILE:         khe_sm_interval.c                                          */
/*  DESCRIPTION:  Intervals (sequences of consecutive integers)              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_INTERVAL_COST_SUBTABLE                                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;

typedef struct khe_interval_cost_subtable_rec {
  ARRAY_KHE_COST		costs;
} *KHE_INTERVAL_COST_SUBTABLE;

typedef HA_ARRAY(KHE_INTERVAL_COST_SUBTABLE) ARRAY_KHE_INTERVAL_COST_SUBTABLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_INTERVAL_COST_TABLE                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_interval_cost_table_rec {
  HA_ARENA				arena;
  ARRAY_KHE_INTERVAL_COST_SUBTABLE	sub_free_list;
  ARRAY_KHE_INTERVAL_COST_SUBTABLE	subs;
  ARRAY_KHE_INTERVAL_COST_SUBTABLE	neg_subs;  /* when in.first < 0 */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalMake(int first, int last)                        */
/*                                                                           */
/*  Make a new interval object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheIntervalMake(int first, int last)
{
  KHE_INTERVAL res;
  HnAssert(last >= first - 1, "KheIntervalMake: last >= first - 1 failed");
  res.first = first;
  res.last = last;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalLength(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return the length of in.                                                 */
/*                                                                           */
/*****************************************************************************/

int KheIntervalLength(KHE_INTERVAL in)
{
  return in.last - in.first + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEmpty(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return true if in is empty.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalEmpty(KHE_INTERVAL in)
{
  return in.first > in.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalContains(KHE_INTERVAL in, int index)                     */
/*                                                                           */
/*  Return true if in contains index.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalContains(KHE_INTERVAL in, int index)
{
  return in.first <= index && index <= in.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)                */
/*                                                                           */
/*  Return true if in1 and in2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  if( KheIntervalEmpty(in1) )
    return KheIntervalEmpty(in2);
  else
    return in1.first == in2.first && in1.last == in2.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)               */
/*                                                                           */
/*  Return true if in1 is a subset of in2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) ||
    (in2.first <= in1.first && in1.last <= in2.last);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)             */
/*                                                                           */
/*  Return true if in1 and in2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) || KheIntervalEmpty(in2) ||
    in1.last < in2.first || in2.last < in1.first;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalUnion(KHE_INTERVAL in1, KHE_INTERVAL in2)        */
/*                                                                           */
/*  Return the union of in1 with in2.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheIntervalUnion(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  if( KheIntervalEmpty(in1) )
    return in2;
  else if( KheIntervalEmpty(in2) )
    return in1;
  else
    return KheIntervalMake(min(in1.first, in2.first),
      max(in1.last, in2.last));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalIntersection(KHE_INTERVAL in1, KHE_INTERVAL in2) */
/*                                                                           */
/*  Return the intersection of in1 with in2.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheIntervalIntersection(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  if( KheIntervalDisjoint(in1, in2) )
    return KheIntervalMake(1, 0);
  else
    return KheIntervalMake(max(in1.first, in2.first),
      min(in1.last, in2.last));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalTypedCmp(KHE_INTERVAL i1, KHE_INTERVAL i2)                */
/*                                                                           */
/*  Typed comparison function for sorting an array of intervals.             */
/*                                                                           */
/*****************************************************************************/

int KheIntervalTypedCmp(KHE_INTERVAL i1, KHE_INTERVAL i2)
{
  if( i1.first != i2.first )
    return i1.first - i2.first;
  else
    return i1.last - i2.last;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Untyped comparison function for sorting an array of intervals.           */
/*                                                                           */
/*****************************************************************************/

int KheIntervalCmp(const void *t1, const void *t2)
{
  KHE_INTERVAL i1 = * (KHE_INTERVAL *) t1;
  KHE_INTERVAL i2 = * (KHE_INTERVAL *) t2;
  return KheIntervalTypedCmp(i1, i2);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheHalfIntervalShow(int val, KHE_FRAME frame)                      */
/*                                                                           */
/*  Return half of an interval show.                                         */
/*                                                                           */
/*****************************************************************************/

static char *KheHalfIntervalShow(int val, KHE_FRAME frame)
{
  static char buff[8][45];  static int bp = 0;
  bp = (bp + 1) % 8;
  if( val < 0 )
    snprintf(buff[bp], 45, "(%d)", val);
  else if( frame != NULL && val < KheFrameTimeGroupCount(frame) )
    snprintf(buff[bp], 45, "%s", KheTimeGroupId(KheFrameTimeGroup(frame,val)));
  else
    snprintf(buff[bp], 45, "%d", val);
  return buff[bp];
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheIntervalShow(KHE_INTERVAL in, KHE_FRAME frame)                  */
/*                                                                           */
/*  Show in in static memory.                                                */
/*                                                                           */
/*****************************************************************************/

char *KheIntervalShow(KHE_INTERVAL in, KHE_FRAME frame)
{
  static char buff[8][100];  static int bp = 0;
  if( KheIntervalEmpty(in) )
    return "-";
  else if( KheIntervalLength(in) == 1 )
    return KheHalfIntervalShow(in.first, frame);
  else
  {
    bp = (bp + 1) % 8;
    snprintf(buff[bp], 100, "%s-%s", KheHalfIntervalShow(in.first, frame),
      KheHalfIntervalShow(in.last, frame));
    return buff[bp];
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "functions that return intervals of interest to solvers"       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg, KHE_FRAME frame)    */
/*                                                                           */
/*  Return the interval of days in frame covering time group tg.             */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTimeInterval(KHE_TIME t, KHE_FRAME frame)
{
  int day_index;
  day_index = KheFrameTimeIndex(frame, t);
  return KheIntervalMake(day_index, day_index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg, KHE_FRAME frame)    */
/*                                                                           */
/*  Return the interval of days in frame covering time group tg.             */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg, KHE_FRAME frame)
{
  KHE_INTERVAL res;  int tg_count;
  tg_count = KheTimeGroupTimeCount(tg);
  if( tg_count > 0 )
  {
    res.first = KheFrameTimeIndex(frame, KheTimeGroupTime(tg, 0));
    res.last  = KheFrameTimeIndex(frame, KheTimeGroupTime(tg, tg_count - 1));
  }
  else
    res = KheIntervalMake(1, 0);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoInterval(KHE_TASK task, KHE_FRAME frame, KHE_INTERVAL *in) */
/*                                                                           */
/*  Do that part of the work of KheTaskSetInterval below that pertains to    */
/*  task and its descendants.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoInterval(KHE_TASK task, KHE_FRAME frame, KHE_INTERVAL *in)
{
  KHE_MEET meet;  KHE_TIME t;  KHE_TASK child_task;  int i, durn, fi, li;

  /* do it for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      if( durn == 1 )
	fi = li = KheFrameTimeIndex(frame, t);
      else
      {
	fi = KheFrameTimeIndex(frame, t);
	li = KheFrameTimeIndex(frame, KheTimeNeighbour(t, durn - 1));
      }
      if( fi < in->first )
	in->first = fi;
      if( li > in->last )
	in->last = li;
    }
  }

  /* do it for the children of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskDoInterval(child_task, frame, in);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskInterval(KHE_TASK task, KHE_FRAME frame)             */
/*                                                                           */
/*  Return the interval of frame covered by task.                            */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTaskInterval(KHE_TASK task, KHE_FRAME frame)
{
  KHE_INTERVAL res;
  res.first = KheFrameTimeGroupCount(frame) - 1;
  res.last = 0;
  KheTaskDoInterval(task, frame, &res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskSetInterval(KHE_TASK_SET ts, KHE_FRAME frame)        */
/*                                                                           */
/*  Return the interval of frame covered by the tasks of ts.                 */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTaskSetInterval(KHE_TASK_SET ts, KHE_FRAME frame)
{
  KHE_INTERVAL res;  KHE_TASK task;  int i;
  res = KheIntervalMake(1, 0);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    res = KheIntervalUnion(res, KheTaskInterval(task, frame));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL_COST_SUBTABLE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_COST_SUBTABLE KheIntervalCostSubtableMake(                  */
/*    KHE_INTERVAL_COST_TABLE ict)                                           */
/*                                                                           */
/*  Make a new interval cost subtable object.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_COST_SUBTABLE KheIntervalCostSubtableMake(
  KHE_INTERVAL_COST_TABLE ict)
{
  KHE_INTERVAL_COST_SUBTABLE res;
  if( HaArrayCount(ict->sub_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ict->sub_free_list);
    HaArrayClear(res->costs);
  }
  else
  {
    HaMake(res, ict->arena);
    HaArrayInit(res->costs, ict->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCostSubtableFree(KHE_INTERVAL_COST_SUBTABLE sub,         */
/*    KHE_INTERVAL_COST_TABLE ict)                                           */
/*                                                                           */
/*  Free sub, or do nothing if sub is NULL.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalCostSubtableFree(KHE_INTERVAL_COST_SUBTABLE sub,
  KHE_INTERVAL_COST_TABLE ict)
{
  if( sub != NULL )
    HaArrayAddLast(ict->sub_free_list, sub);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCostSubtableDebug(KHE_INTERVAL_COST_SUBTABLE sub,        */
/*    int first_index, KHE_FRAME frame, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of the cache sub indexed by first_index.  NB first_index     */
/*  could be negative.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalCostSubtableDebug(KHE_INTERVAL_COST_SUBTABLE sub,
  int first_index, KHE_FRAME frame, int verbosity, int indent, FILE *fp)
{
  KHE_COST cost;  int len;  KHE_INTERVAL in;
  if( sub != NULL )
    HaArrayForEach(sub->costs, cost, len)
      if( cost != -1 )
      {
	in = KheIntervalMake(first_index, first_index + len - 1);
        fprintf(fp, "%*s  %s: %.5f\n", indent, "",
	  KheIntervalShow(in, frame), KheCostShow(cost));
      }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL_COST_TABLE"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_COST_TABLE KheIntervalCostTableMake(HA_ARENA a)             */
/*                                                                           */
/*  Make a new, empty cost table.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL_COST_TABLE KheIntervalCostTableMake(HA_ARENA a)
{
  KHE_INTERVAL_COST_TABLE res;
  HaMake(res, a);
  res->arena = a;
  HaArrayInit(res->sub_free_list, a);
  HaArrayInit(res->subs, a);
  HaArrayInit(res->neg_subs, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCostTableClear(KHE_INTERVAL_COST_TABLE ict)              */
/*                                                                           */
/*  Clear out ict, ready for a fresh start.                                  */
/*                                                                           */
/*****************************************************************************/

void KheIntervalCostTableClear(KHE_INTERVAL_COST_TABLE ict)
{
  KHE_INTERVAL_COST_SUBTABLE sub;  int i;

  /* clear ict->subs */
  HaArrayForEach(ict->subs, sub, i)
    KheIntervalCostSubtableFree(sub, ict);
  HaArrayClear(ict->subs);

  /* clear ict->neg_subs */
  HaArrayForEach(ict->neg_subs, sub, i)
    KheIntervalCostSubtableFree(sub, ict);
  HaArrayClear(ict->neg_subs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCostTableInsert(KHE_INTERVAL_COST_TABLE ict,             */
/*    KHE_INTERVAL in, KHE_COST cost)                                        */
/*                                                                           */
/*  Within ict, set cost to the value associated with in.  Overwrite any     */
/*  previous value.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheIntervalCostTableInsert(KHE_INTERVAL_COST_TABLE ict,
  KHE_INTERVAL in, KHE_COST cost)
{
  KHE_INTERVAL_COST_SUBTABLE sub;  int len;

  /* length and cost must be non-negative */
  len = KheIntervalLength(in);
  HnAssert(len >= 0, "KheIntervalCostTableInsert: negative interval length");
  HnAssert(cost >= 0, "KheIntervalCostTableInsert: negative cost");
  if( len == 0 )
    in = KheIntervalMake(1, 0);

  /* get sub, either existing or newly made */
  if( in.first < 0 )
  {
    in.first = - in.first;  /* does not change caller's value */
    HaArrayFill(ict->neg_subs, in.first + 1, NULL);
    sub = HaArray(ict->neg_subs, in.first);
    if( sub == NULL )
    {
      sub = KheIntervalCostSubtableMake(ict);
      HaArrayPut(ict->neg_subs, in.first, sub);
    }
  }
  else
  {
    HaArrayFill(ict->subs, in.first + 1, NULL);
    sub = HaArray(ict->subs, in.first);
    if( sub == NULL )
    {
      sub = KheIntervalCostSubtableMake(ict);
      HaArrayPut(ict->subs, in.first, sub);
    }
  }

  /* add the cost to sub */
  HaArrayFill(sub->costs, len + 1, -1 /* means NULL */);
  HaArrayPut(sub->costs, len, cost);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalCostTableRetrieve(KHE_INTERVAL_COST_TABLE ict,           */
/*    KHE_INTERVAL in, KHE_COST *cost)                                       */
/*                                                                           */
/*  If ict contains a cost for in, set *cost to that cost and return true.   */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

bool KheIntervalCostTableRetrieve(KHE_INTERVAL_COST_TABLE ict,
  KHE_INTERVAL in, KHE_COST *cost)
{
  KHE_INTERVAL_COST_SUBTABLE sub;  int len;

  /* length must be non-negative */
  len = KheIntervalLength(in);
  HnAssert(len >= 0, "KheIntervalCostTableRetrieve: negative interval length");
  if( len == 0 )
    in = KheIntervalMake(1, 0);

  /* get sub, the subtable (possibly NULL) for in; or return false */
  if( in.first < 0 )
  {
    /* use the negative of in.first as the index */
    in.first = - in.first;
    if( in.first >= HaArrayCount(ict->neg_subs) )
      return *cost = -1, false;
    sub = HaArray(ict->neg_subs, in.first);
  }
  else
  {
    if( in.first >= HaArrayCount(ict->subs) )
      return *cost = -1, false;
    sub = HaArray(ict->subs, in.first);
  }

  /* look up in in sub */
  if( sub == NULL || len >= HaArrayCount(sub->costs) )
    return *cost = -1, false;
  else
    return *cost = HaArray(sub->costs, len), *cost >= 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCostTableDebug(KHE_INTERVAL_COST_TABLE ict,              */
/*    KHE_FRAME frame, int verbosity, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of ict onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheIntervalCostTableDebug(KHE_INTERVAL_COST_TABLE ict,
  KHE_FRAME frame, int verbosity, int indent, FILE *fp)
{
  int fi;  KHE_INTERVAL_COST_SUBTABLE sub;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ IntervalCostTable\n", indent, "");
    HaArrayForEachReverse(ict->neg_subs, sub, fi)
      KheIntervalCostSubtableDebug(sub, - fi, frame, verbosity, indent, fp);
    HaArrayForEach(ict->subs, sub, fi)
      KheIntervalCostSubtableDebug(sub, fi, frame, verbosity, indent, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "IntervalCostTable");
}

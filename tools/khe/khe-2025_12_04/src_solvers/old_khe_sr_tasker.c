
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
/*  FILE:         khe_sr_tasker.c                                            */
/*  DESCRIPTION:  Tasker objects (for grouping by resource constraints)      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0	/* shows which groups are made */
#define DEBUG2	0	/* KheTaskerGroupingAddClass */
#define DEBUG3	1	/* internal error */
#define DEBUG4	0	/* TaskerMake */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type definitions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP                                                   */
/*                                                                           */
/*  A time group whose cover is needed by profile grouping.                  */
/*                                                                           */
/*  The index attribute holds this object's index in its tasker.             */
/*                                                                           */
/*  The cover attribute holds the number of tasks (not the number of         */
/*  classes) that cover this time group.  This is over all classes, not      */
/*  just classes from the current grouping.                                  */
/*                                                                           */
/*  The domain_covers attribute holds the number of tasks that cover         */
/*  this time group and have the corresponding domain.  The sum of           */
/*  these numbers equals cover.                                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;

struct khe_profile_time_group_rec {
  KHE_TASKER			tasker;
  KHE_TIME_GROUP		time_group;
  int				index;
  int				cover;
  ARRAY_KHE_RESOURCE_GROUP	domains;
  HA_ARRAY_INT			domain_covers;
};

typedef HA_ARRAY(KHE_PROFILE_TIME_GROUP) ARRAY_KHE_PROFILE_TIME_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_OVERLAP_TIME_GROUP (private)                                         */
/*                                                                           */
/*  A time group that a grouping is not permitted to cover twice.            */
/*                                                                           */
/*  The cover attribute holds the number of classes from the current         */
/*  grouping that cover this time group.  It may not exceed 1.               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_overlap_time_group_rec {
  KHE_TIME_GROUP	time_group;		/* the time group            */
  int			overlap_cover;		/* current cover             */
  int			check_overlap_cover;	/* for debugging only        */
} *KHE_OVERLAP_TIME_GROUP;

typedef HA_ARRAY(KHE_OVERLAP_TIME_GROUP) ARRAY_KHE_OVERLAP_TIME_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_CLASS                                                         */
/*                                                                           */
/*  A set of equivalent proper root tasks.                                   */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_TASKER_TIME) ARRAY_KHE_TASKER_TIME;

struct khe_tasker_class_rec {
  KHE_TASKER			tasker;		/* enclosing tasker          */
  KHE_TASK_SET			tasks;		/* the tasks                 */
  KHE_RESOURCE_GROUP		domain;		/* their common domain       */
  KHE_RESOURCE			resource;	/* their common resource     */
  KHE_TIME_SET			time_set;	/* their common time set     */
  ARRAY_KHE_TASKER_TIME		times;		/* one for each time         */
  ARRAY_KHE_TASKER_TIME		profile_times;	/* times in profile tg's     */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_TIME                                                          */
/*                                                                           */
/*  One time, as viewed by a tasker.                                         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_TASKER_CLASS) ARRAY_KHE_TASKER_CLASS;

struct khe_tasker_time_rec {
  KHE_TASKER			tasker;		/* enclosing tasker          */
  KHE_TIME			time;		/* the time                  */
  ARRAY_KHE_TASKER_CLASS	classes;	/* classes that cover time   */
  KHE_PROFILE_TIME_GROUP	profile_time_group;  /* optional profile tg  */
  KHE_OVERLAP_TIME_GROUP	overlap_time_group;  /* optional overlap tg  */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER                                                               */
/*                                                                           */
/*  A set of tasks grouped into classes and indexed by time, as needed by    */
/*  grouping by resource constraints.                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_tasker_rec {

  /* the basics */
  HA_ARENA			arena;		/* get memory from here      */
  KHE_SOLN			soln;		/* get tasks from here       */
  KHE_RESOURCE_TYPE		resource_type;	/* tasks of this type only   */
  KHE_TASK_SET			task_set;	/* that have been assigned   */
  ARRAY_KHE_TASKER_TIME		times;		/* one for each time         */
  ARRAY_KHE_TASKER_CLASS	free_classes;   /* free list of classes      */
  KHE_TIME_SET			tmp_time_set;	/* a temporary time set      */

  /* support for profile grouping */
  ARRAY_KHE_PROFILE_TIME_GROUP  profile_time_groups;
  ARRAY_KHE_PROFILE_TIME_GROUP  free_profile_time_groups;
  int				profile_max_len;

  /* grouping */
  ARRAY_KHE_TASKER_CLASS	grouping_classes;
  ARRAY_KHE_OVERLAP_TIME_GROUP	overlap_time_groups;
  ARRAY_KHE_OVERLAP_TIME_GROUP	free_overlap_time_groups;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "profile time groups"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupIncrease(KHE_PROFILE_TIME_GROUP ptg, int num,    */
/*    KHE_RESOURCE_GROUP domain)                                             */
/*                                                                           */
/*  Inform ptg that there are now num more tasks, each with the given        */
/*  domain, in its profile.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupIncrease(KHE_PROFILE_TIME_GROUP ptg, int num,
  KHE_RESOURCE_GROUP domain)
{
  int i, dcount;  KHE_RESOURCE_GROUP rg;
  ptg->cover += num;
  dcount = KheResourceGroupResourceCount(domain);
  HaArrayForEach(ptg->domains, rg, i)
  {
    if( dcount < KheResourceGroupResourceCount(rg) )
      break;
    else if( KheResourceGroupEqual(domain, rg) )
    {
      HaArrayPut(ptg->domain_covers, i, HaArray(ptg->domain_covers, i) + num);
      return;
    }
  }
  HaArrayAdd(ptg->domains, i, domain);
  HaArrayAdd(ptg->domain_covers, i, num);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDecrease(KHE_PROFILE_TIME_GROUP ptg, int num,    */
/*    KHE_RESOURCE_GROUP domain)                                             */
/*                                                                           */
/*  Undo the corresponding Add.  Don't bother to remove zero entries.        */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupDecrease(KHE_PROFILE_TIME_GROUP ptg, int num,
  KHE_RESOURCE_GROUP domain)
{
  int pos, curr;
  if( !HaArrayContains(ptg->domains, domain, &pos) )
    HnAbort("KheProfileTimeGroupDecrease:  internal error 1");
  curr = HaArray(ptg->domain_covers, pos);
  HnAssert(curr >= num, "KheProfileTimeGroupDecrease:  internal error 2");
  HaArrayPut(ptg->domain_covers, pos, curr - num);
  HnAssert(ptg->cover >= num, "KheProfileTimeGroupDecrease:  internal error 3");
  ptg->cover -= num;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDebug(KHE_PROFILE_TIME_GROUP ptg,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ptg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupDebug(KHE_PROFILE_TIME_GROUP ptg,
  int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE_GROUP domain;  int i;
  fprintf(fp, "%*s[ ProfileTimeGroup(%d: %s, cover %d)\n", indent, "",
    ptg->index, KheTimeGroupId(ptg->time_group), ptg->cover);
  HaArrayForEach(ptg->domains, domain, i)
    fprintf(fp, "%*s  %s: %d\n", indent, "", KheResourceGroupId(domain),
      HaArray(ptg->domain_covers, i));
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "classes"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_CLASS KheTaskerClassMake(KHE_TASKER tr, KHE_TASK task)        */
/*                                                                           */
/*  Make a new class suitable for holding task (but do not add task to it),  */
/*  and add it to the appropriate tasker time objects.                       */
/*                                                                           */
/*****************************************************************************/
static void KheTaskerTimeAddClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c);

static KHE_TASKER_CLASS KheTaskerClassMake(KHE_TASKER tr, KHE_TASK task)
{
  KHE_TASKER_CLASS res;  int i;  KHE_TIME time;  KHE_TASKER_TIME t;
  if( HaArrayCount(tr->free_classes) > 0 )
  {
    res = HaArrayLastAndDelete(tr->free_classes);
    KheTaskSetClear(res->tasks);
    KheTimeSetClear(res->time_set);
    HaArrayClear(res->times);
    HaArrayClear(res->profile_times);
  }
  else
  {
    HaMake(res, tr->arena);
    res->tasks = KheTaskSetMake(tr->soln);
    res->time_set = KheTimeSetMake(KheSolnInstance(tr->soln), tr->arena);
    HaArrayInit(res->times, tr->arena);
    HaArrayInit(res->profile_times, tr->arena);
  }
  res->tasker = tr;
  res->domain = KheTaskDomain(task);
  res->resource = KheTaskAsstResource(task);
  KheTimeSetAddTaskTimes(res->time_set, task);
  for( i = 0;  i < KheTimeSetTimeCount(res->time_set);  i++ )
  {
    time = KheTimeSetTime(res->time_set, i);
    t = HaArray(tr->times, KheTimeIndex(time));
    HaArrayAddLast(res->times, t);
    KheTaskerTimeAddClass(t, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassDelete(KHE_TASKER_CLASS c)                            */
/*                                                                           */
/*  Delete c from its enclosing tasker.                                      */
/*                                                                           */
/*****************************************************************************/
static void KheTaskerTimeDeleteClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c);

static void KheTaskerClassDelete(KHE_TASKER_CLASS c)
{
  int i;  KHE_TASKER_TIME t;
  HnAssert(KheTaskSetTaskCount(c->tasks) == 0,
    "KheTaskerClassDelete internal error");
  HaArrayForEach(c->times, t, i)
    KheTaskerTimeDeleteClass(t, c);
  HaArrayAddLast(c->tasker->free_classes, c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerClassTypedCmp(KHE_TASKER_CLASS c1, KHE_TASKER_CLASS c2)     */
/*                                                                           */
/*  Typed comparison function for sorting classes.                           */
/*                                                                           */
/*****************************************************************************/

static int KheTaskerClassTypedCmp(KHE_TASKER_CLASS c1, KHE_TASKER_CLASS c2)
{
  int c1_val, c2_val;
  c1_val = (c1->resource != NULL ? 0 : 1);
  c2_val = (c2->resource != NULL ? 0 : 1);
  return c1_val != c2_val ? c1_val - c2_val :
    KheResourceGroupResourceCount(c1->domain) -
    KheResourceGroupResourceCount(c2->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerClassCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting classes.                         */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheTaskerClassCmp(const void *t1, const void *t2)
{
  KHE_TASKER_CLASS c1 = * (KHE_TASKER_CLASS *) t1;
  KHE_TASKER_CLASS c2 = * (KHE_TASKER_CLASS *) t2;
  return KheTaskerClassTypedCmp(c1, c2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassAddToArray(KHE_TASKER_CLASS c,                        */
/*    ARRAY_KHE_TASKER_CLASS *classes, int *pos)                             */
/*                                                                           */
/*  Add c to *classes in its sorted position; set *pos to that position.     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassAddToArray(KHE_TASKER_CLASS c,
  ARRAY_KHE_TASKER_CLASS *classes, int *pos)
{
  KHE_TASKER_CLASS c2;
  for( *pos = 0;  *pos < HaArrayCount(*classes);  (*pos)++ )
  {
    c2 = HaArray(*classes, *pos);
    if( KheTaskerClassTypedCmp(c, c2) < 0  )
      break;
  }
  HaArrayAdd(*classes, *pos, c);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassDeleteFromArray(KHE_TASKER_CLASS c,                   */
/*    ARRAY_KHE_TASKER_CLASS *classes)                                       */
/*                                                                           */
/*  Delete c from *classes, aborting if not present.                         */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassDeleteFromArray(KHE_TASKER_CLASS c,
  ARRAY_KHE_TASKER_CLASS *classes)
{
  int pos;
  if( !HaArrayContains(*classes, c, &pos) )
    HnAbort("KheTaskerClassDeleteFromArray internal error");
  HaArrayDeleteAndShift(*classes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerClassAcceptsTask(KHE_TASKER_CLASS c, KHE_TASK task,        */
/*    KHE_TIME_SET task_time_set)                                            */
/*                                                                           */
/*  Return true if task is equivalent to the tasks already in c, and         */
/*  so could be added to c.                                                  */
/*                                                                           */
/*  Parameter task_time_set is a time set holding task's times.  It is       */
/*  not stored permanently anywhere; the caller of this function is free     */
/*  to set it to something else after this function returns.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskerClassAcceptsTask(KHE_TASKER_CLASS c, KHE_TASK task,
  KHE_TIME_SET task_time_set)
{
  return KheTaskAsstResource(task) == c->resource &&
    KheResourceGroupEqual(KheTaskDomain(task), c->domain) &&
    KheTimeSetEqual(task_time_set, c->time_set);
  /* ***
  return KheTaskAsstResource(task) == c->resource &&
    KheTaskEq uivalent(KheTaskSetTask(c->tasks, 0), task);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskDecreasingMeetIndexTypedCmp(KHE_TASK task1, KHE_TASK task2)   */
/*                                                                           */
/*  Typed comparison function for sorting tasks by decreasing meet index.    */
/*                                                                           */
/*****************************************************************************/

static int KheTaskDecreasingMeetIndexTypedCmp(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskMeetIndex(task2) - KheTaskMeetIndex(task1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskDecreasingMeetIndexCmp(const void *t1, const void *t2)        */
/*                                                                           */
/*  Untyped comparison function for sorting tasks by decreasing meet index.  */
/*                                                                           */
/*****************************************************************************/

static int KheTaskDecreasingMeetIndexCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  return KheTaskDecreasingMeetIndexTypedCmp(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassAddTask(KHE_TASKER_CLASS c, KHE_TASK task)            */
/*                                                                           */
/*  Add task to c, assuming that either the class is currently empty or      */
/*  else that KheTaskerClassAcceptsTask(c, task) holds.                      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassAddTask(KHE_TASKER_CLASS c, KHE_TASK task)
{
  KHE_TASKER_TIME t;  int i;
  KheTaskSetAddTask(c->tasks, task);
  KheTaskSetSort(c->tasks, &KheTaskDecreasingMeetIndexCmp);
  if( HaArrayCount(c->profile_times) <= c->tasker->profile_max_len )
    HaArrayForEach(c->profile_times, t, i)
      KheProfileTimeGroupIncrease(t->profile_time_group, 1, c->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerClassTaskCount(KHE_TASKER_CLASS c)                          */
/*                                                                           */
/*  Return the number of tasks in c.                                         */
/*                                                                           */
/*****************************************************************************/

int KheTaskerClassTaskCount(KHE_TASKER_CLASS c)
{
  return KheTaskSetTaskCount(c->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskerClassTask(KHE_TASKER_CLASS c, int i)                   */
/*                                                                           */
/*  Return the i'th task of c.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskerClassTask(KHE_TASKER_CLASS c, int i)
{
  return KheTaskSetTask(c->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskerClassDomain(KHE_TASKER_CLASS c)              */
/*                                                                           */
/*  Return the domain that the tasks of c share.                             */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskerClassDomain(KHE_TASKER_CLASS c)
{
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheTaskerClassAsstResource(KHE_TASKER_CLASS c)              */
/*                                                                           */
/*  Return the resource (possibly NULL) that the tasks of c are assigned to. */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheTaskerClassAsstResource(KHE_TASKER_CLASS c)
{
  return c->resource;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheTaskerClassTimeSet(KHE_TASKER_CLASS c)                   */
/*                                                                           */
/*  Return a time set containing the times that the tasks of c cover.        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_SET KheTaskerClassTimeSet(KHE_TASKER_CLASS c)
{
  return c->time_set;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskerClassLastAndDelete(KHE_TASKER_CLASS c)                 */
/*                                                                           */
/*  Delete and return the last task of c.  If this causes c to become        */
/*  empty, then delete c from the enclosing tasker.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheTaskerClassLastAndDelete(KHE_TASKER_CLASS c)
{
  int i;  KHE_TASKER_TIME t;  KHE_TASK res;
  res = KheTaskSetLastAndDelete(c->tasks);
  if( HaArrayCount(c->profile_times) <= c->tasker->profile_max_len )
    HaArrayForEach(c->profile_times, t, i)
      KheProfileTimeGroupDecrease(t->profile_time_group, 1, c->domain);
  if( KheTaskSetTaskCount(c->tasks) == 0 )
    KheTaskerClassDelete(c);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassAddProfileTime(KHE_TASKER_CLASS c, KHE_TASKER_TIME t) */
/*                                                                           */
/*  Add a profile time to c.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassAddProfileTime(KHE_TASKER_CLASS c, KHE_TASKER_TIME t)
{
  KHE_TASKER_TIME t2;  int i;

  /* update the covers of c's profile times' time groups appropriately */
  if( HaArrayCount(c->profile_times) < c->tasker->profile_max_len )
  {
    /* c counts towards covers: before yes, after yes */
    KheProfileTimeGroupIncrease(t->profile_time_group,
      KheTaskSetTaskCount(c->tasks), c->domain);
  }
  else if( HaArrayCount(c->profile_times) == c->tasker->profile_max_len )
  {
    /* c counts towards covers: before yes, after no */
    HaArrayForEach(c->profile_times, t2, i)
      KheProfileTimeGroupDecrease(t2->profile_time_group,
      KheTaskSetTaskCount(c->tasks), c->domain);
  }
  else
  {
    /* c counts towards covers: before no, after no */
  }

  /* add t to c's list of profile times */
  HaArrayAddLast(c->profile_times, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassDeleteProfileTime(KHE_TASKER_CLASS c,                 */
/*    KHE_TASKER_TIME t)                                                     */
/*                                                                           */
/*  Delete profile time t from c.                                            */
/*                                                                           */
/*  Implementation note.  When this function is called, the old value of     */
/*  t->profile_time_group is still present; it is removed later.             */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassDeleteProfileTime(KHE_TASKER_CLASS c,
  KHE_TASKER_TIME t)
{
  KHE_TASKER_TIME t2;  int i, pos;

  /* delete t from c's list of profile times */
  if( !HaArrayContains(c->profile_times, t, &pos) )
    HnAbort("KheTaskerClassDeleteProfileTime internal error");
  HaArrayDeleteAndPlug(c->profile_times, pos);

  /* update the covers of c's profile times' time groups appropriately */
  if( HaArrayCount(c->profile_times) < c->tasker->profile_max_len )
  {
    /* c counts towards covers: before yes, after yes */
    KheProfileTimeGroupDecrease(t->profile_time_group,
      KheTaskSetTaskCount(c->tasks), c->domain);
  }
  else if( HaArrayCount(c->profile_times) == c->tasker->profile_max_len )
  {
    /* c counts towards covers: before no, after yes */
    HaArrayForEach(c->profile_times, t2, i)
      KheProfileTimeGroupIncrease(t2->profile_time_group,
        KheTaskSetTaskCount(c->tasks), c->domain);
  }
  else
  {
    /* c counts towards covers: before no, after no */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerClassOverlapsGrouping(KHE_TASKER_CLASS c)                  */
/*                                                                           */
/*  Return true if c would overlap with the groups already present.          */
/*                                                                           */
/*****************************************************************************/

bool KheTaskerClassOverlapsGrouping(KHE_TASKER_CLASS c)
{
  int i;  KHE_TASKER_TIME t;
  HaArrayForEach(c->times, t, i)
    if( KheTaskerTimeOverlapsGrouping(t) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassAddOverlap(KHE_TASKER tr, KHE_TASKER_CLASS c)         */
/*                                                                           */
/*  We're adding c to the grouping, so add its overlaps.                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassAddOverlap(KHE_TASKER_CLASS c)
{
  int i;  KHE_TASKER_TIME t;
  HaArrayForEach(c->times, t, i)
    if( t->overlap_time_group != NULL )
      t->overlap_time_group->overlap_cover++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassDeleteOverlap(KHE_TASKER tr, KHE_TASKER_CLASS c)      */
/*                                                                           */
/*  We're clearing c from the grouping, so clear its overlaps.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerClassDeleteOverlap(KHE_TASKER_CLASS c)
{
  int i;  KHE_TASKER_TIME t;
  HaArrayForEach(c->times, t, i)
    if( t->overlap_time_group != NULL )
      t->overlap_time_group->overlap_cover--;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClassDebug(KHE_TASKER_CLASS c, int verbosity,              */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of c with the given indent.                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskerClassDebug(KHE_TASKER_CLASS c, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s[ TaskerClass(", indent, "");
  KheResourceGroupDebug(c->domain, 1, -1, fp);
  if( c->resource != NULL )
    fprintf(fp, ", asst %s", KheResourceId(c->resource));
  fprintf(fp, ", ");
  KheTimeSetDebug(c->time_set, 1, -1, fp);
  fprintf(fp, ", %d profile times, %d tasks): ",
    HaArrayCount(c->profile_times), KheTaskSetTaskCount(c->tasks));
  KheTaskSetDebug(c->tasks, 2, -1, fp);
  fprintf(fp, " ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_TIME KheTaskerTimeMake(KHE_TASKER tr, KHE_TIME t)             */
/*                                                                           */
/*  Make a new time object for t.   There is no KheTaskerTimeDelete.         */
/*                                                                           */
/*****************************************************************************/

static KHE_TASKER_TIME KheTaskerTimeMake(KHE_TASKER tr, KHE_TIME t)
{
  KHE_TASKER_TIME res;
  HaMake(res, tr->arena);
  res->tasker = tr;
  res->time = t;
  HaArrayInit(res->classes, tr->arena);
  res->overlap_time_group = NULL;
  res->profile_time_group = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerTimeAddClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c)        */
/*                                                                           */
/*  Add c to t, at a suitable spot for its domain and assigned resource.     */
/*                                                                           */
/*  Implementation note.  The times of classes do not change, so this        */
/*  function can only be called when c is being created.                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerTimeAddClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c)
{
  int pos;
  KheTaskerClassAddToArray(c, &t->classes, &pos);
  if( t->profile_time_group != NULL )
    KheTaskerClassAddProfileTime(c, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerTimeDeleteClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c)     */
/*                                                                           */
/*  Delete c from t.                                                         */
/*                                                                           */
/*  Implementation note.  The times of classes do not change, so this        */
/*  function can only be called when c is being deleted.                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerTimeDeleteClass(KHE_TASKER_TIME t, KHE_TASKER_CLASS c)
{
  KheTaskerClassDeleteFromArray(c, &t->classes);
  if( t->profile_time_group != NULL )
    KheTaskerClassDeleteProfileTime(c, t);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTaskerTimeTime(KHE_TASKER_TIME t)                            */
/*                                                                           */
/*  Return the time represented by t.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheTaskerTimeTime(KHE_TASKER_TIME t)
{
  return t->time;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerTimeClassCount(KHE_TASKER_TIME t)                           */
/*                                                                           */
/*  Return the number of equivalence classes in t.                           */
/*                                                                           */
/*****************************************************************************/

int KheTaskerTimeClassCount(KHE_TASKER_TIME t)
{
  return HaArrayCount(t->classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_CLASS KheTaskerTimeClass(KHE_TASKER_TIME t, int i)            */
/*                                                                           */
/*  Return the i'th class of t.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER_CLASS KheTaskerTimeClass(KHE_TASKER_TIME t, int i)
{
  return HaArray(t->classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerTimeAddProfileTimeGroup(KHE_TASKER_TIME t,                 */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Add ptg to t and its classes.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerTimeAddProfileTimeGroup(KHE_TASKER_TIME t,
  KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_TASKER_CLASS c;  int i;

  /* add ptg to t */
  if( t->profile_time_group != NULL )
    HnAbort("KheTaskerTimeAddProfileTimeGroup(%s, %s): time already in profile"
      " time group %s", KheTimeId(t->time), KheTimeGroupId(ptg->time_group),
      KheTimeGroupId(t->profile_time_group->time_group));
  t->profile_time_group = ptg;

  /* update t's classes */
  HaArrayForEach(t->classes, c, i)
    KheTaskerClassAddProfileTime(c, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerTimeDeleteProfileTimeGroup(KHE_TASKER_TIME t,              */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Remove ptg from t and its classes.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerTimeDeleteProfileTimeGroup(KHE_TASKER_TIME t,
  KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_TASKER_CLASS c;  int i;

  /* update t's classes */
  HaArrayForEach(t->classes, c, i)
    KheTaskerClassDeleteProfileTime(c, t);

  /* remove ptg from t */
  t->profile_time_group = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerTimeOverlapsGrouping(KHE_TASKER_TIME t)                    */
/*                                                                           */
/*  Return true if something running at t would overlap with the groups      */
/*  already present.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheTaskerTimeOverlapsGrouping(KHE_TASKER_TIME t)
{
  return t->overlap_time_group != NULL &&
    t->overlap_time_group->overlap_cover > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerTimeDebug(KHE_TASKER_TIME t, int verbosity, int indent,    */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of t with the given indent.                                  */
/*                                                                           */
/*****************************************************************************/
static void KheOverlapTimeGroupDebug(KHE_OVERLAP_TIME_GROUP otg,
  int verbosity, int indent, FILE *fp);
static void KheProfileTimeGroupDebug(KHE_PROFILE_TIME_GROUP ptg,
  int verbosity, int indent, FILE *fp);

void KheTaskerTimeDebug(KHE_TASKER_TIME t, int verbosity, int indent, FILE *fp)
{
  KHE_TASKER_CLASS c;  int i;
  fprintf(fp, "%*s[ TaskerTime(%s", indent, "", KheTimeId(t->time));
  if( t->overlap_time_group != NULL )
  {
    fprintf(fp, ", overlap ");
    KheOverlapTimeGroupDebug(t->overlap_time_group, 1, -1, fp);
  }
  if( t->profile_time_group != NULL )
  {
    fprintf(fp, ", profile ");
    KheProfileTimeGroupDebug(t->profile_time_group, 1, -1, fp);
  }
  fprintf(fp, ")\n");
  HaArrayForEach(t->classes, c, i)
    KheTaskerClassDebug(c, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tasker"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_CLASS KheTaskerAddTask(KHE_TASKER tr, KHE_TASK task,          */
/*    KHE_TIME_SET task_time_set)                                            */
/*                                                                           */
/*  Add task to tr and return the equivalence class it goes into, or if      */
/*  task has no assigned time, do nothing and return NULL.  Here task        */
/*  must be a proper root task.                                              */
/*                                                                           */
/*  Parameter task_time_set is a time set holding task's times.  It is       */
/*  not stored permanently anywhere; the caller of this function is free     */
/*  to set it to something else after this function returns.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_TASKER_CLASS KheTaskerAddTask(KHE_TASKER tr, KHE_TASK task,
  KHE_TIME_SET task_time_set)
{
  KHE_MEET meet;  KHE_TIME time;  int i;  KHE_TASKER_TIME t;
  KHE_TASKER_CLASS c;
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    time = KheMeetAsstTime(meet);
    if( time != NULL )
    {
      /* add task to an existing equivalence class, if that can be done */
      t = HaArray(tr->times, KheTimeIndex(time));
      HaArrayForEach(t->classes, c, i)
	if( KheTaskerClassAcceptsTask(c, task, task_time_set) )
	{
	  KheTaskerClassAddTask(c, task);
	  return c;
	}

      /* no existing equivalence class, so make one (initially empty) */
      c = KheTaskerClassMake(tr, task);
      KheTaskerClassAddTask(c, task);
      return c;
    }
  }
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSuits(KHE_TASK task, KHE_MAYBE_TYPE maybe_type)              */
/*                                                                           */
/*  Return true if task, or some task assigned directly or indirectly to     */
/*  it, is preassigned or derived from an event resource er for which        */
/*  KheEventResourceNeedsAssignment(er) == maybe_type.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSuits(KHE_TASK task, KHE_MAYBE_TYPE maybe_type)
{
  KHE_TASK child_task;  int i;  KHE_RESOURCE r;  KHE_EVENT_RESOURCE er;

  /* task suits if it is preassigned */
  if( KheTaskIsPreassigned(task, &r) )
    return true;

  /* task suits if it is derived from a maybe_type event resource */
  er = KheTaskEventResource(task);
  if( er != NULL && KheEventResourceNeedsAssignment(er) == maybe_type )
    return true;

  /* task suits if any of its children suit */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskSuits(child_task, maybe_type) )
      return true;
  }

  /* no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMaybeTypeShow(KHE_MAYBE_TYPE maybe_type)                        */
/*                                                                           */
/*  Display maybe_type.                                                      */
/*                                                                           */
/*****************************************************************************/

static char *KheMaybeTypeShow(KHE_MAYBE_TYPE maybe_type)
{
  switch( maybe_type )
  {
    case KHE_NO:	return "KHE_NO";
    case KHE_MAYBE:	return "KHE_MAYBE";
    case KHE_YES:	return "KHE_YES";
    default:		return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER KheTaskerMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,            */
/*    bool include_assigned_tasks, KHE_TASK_SET task_set, HA_ARENA a)        */
/*                                                                           */
/*  Make a tasker object with these attributes.  Add all tasks of soln of    */
/*  type rt which are proper roots (that is, either unassigned, or assigned  */
/*  directly to cycle tasks).  If include_assigned_tasks, include tasks      */
/*  that are already assigned a resource, otherwise omit them.               */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER KheTaskerMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  /* bool include_assigned_tasks, */ KHE_TASK_SET task_set, HA_ARENA a)
{
  KHE_TASKER res;  KHE_TASK task;  KHE_TIME time;
  int i, x;  KHE_TASKER_TIME t;  KHE_INSTANCE ins;
  KHE_MAYBE_TYPE maybe_type;

  /* the basics */
  ins = KheSolnInstance(soln);
  if( DEBUG4 )
    fprintf(stderr, "[ KheTaskerMake(soln of %s, %s, ts, a)\n",
      KheInstanceId(ins), KheResourceTypeId(rt));
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->task_set = task_set;
  HaArrayInit(res->times, a);
  HaArrayInit(res->free_classes, a);
  res->tmp_time_set = KheTimeSetMake(ins, a);

  /* support for profile grouping */
  HaArrayInit(res->profile_time_groups, a);
  HaArrayInit(res->free_profile_time_groups, a);
  res->profile_max_len = 0;

  /* grouping */
  HaArrayInit(res->grouping_classes, a);
  HaArrayInit(res->overlap_time_groups, a);
  HaArrayInit(res->free_overlap_time_groups, a);
  /* res->frame = NULL; */
  /* res->asst_count_check = 0; */

  /* add times */
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    time = KheInstanceTime(ins, i);
    t = KheTaskerTimeMake(res, time);
    HaArrayAddLast(res->times, t);
  }

  /* add tasks */
  x = KheInstanceConstraintOfTypeCount(ins, KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG);
  maybe_type = (x == 0 ? KHE_MAYBE : KHE_YES);
  if( DEBUG4 )
    fprintf(stderr, "  assign resource constraints: %d\n", x);
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    /* ***
    if( !KheTaskIsCycleTask(task) && KheTaskResourceType(task) == rt &&
	** (include_assigned_tasks || KheTaskAsstResource(task) == NULL ) && **
	!KheTaskAssignIsFixed(task) ** this and prev added JeffK 9/8/24 ** &&
        (KheTaskAsst(task) == NULL || KheTaskIsCycleTask(KheTaskAsst(task))) )
    *** */
    if( KheTaskIsProperRoot(task) && KheTaskResourceType(task) == rt &&
	!KheTaskAssignIsFixed(task) )
    {
      if( DEBUG4 )
      {
	KHE_EVENT_RESOURCE er;  KHE_MAYBE_TYPE mt;
	fprintf(stderr, "  KheTaskSuits(");
	KheTaskDebug(task, 2, -1, stderr);
	fprintf(stderr, ", %s) = %s", KheMaybeTypeShow(maybe_type),
	  bool_show(KheTaskSuits(task, maybe_type)));
	er = KheTaskEventResource(task);
	if( er != NULL )
	{
	  fprintf(stderr, " (");
	  KheEventResourceDebug(er, 2, -1, stderr);
	  mt = KheEventResourceNeedsAssignment(er);
	  fprintf(stderr, "  = %s)", KheMaybeTypeShow(mt));
	}
	fprintf(stderr, "\n");
      }
      if( KheTaskSuits(task, maybe_type) )
      {
	KheTimeSetClear(res->tmp_time_set);
	KheTimeSetAddTaskTimes(res->tmp_time_set, task);
	KheTaskerAddTask(res, task, res->tmp_time_set);
      }
    }
  }

  /* all done */
  if( DEBUG4 )
    fprintf(stderr, "] KheTaskerMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheTaskerSoln(KHE_TASKER tr)                                    */
/*                                                                           */
/*  Return tr's soln attribute.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheTaskerSoln(KHE_TASKER tr)
{
  return tr->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheTaskerResourceType(KHE_TASKER tr)                   */
/*                                                                           */
/*  Return tr's resource type attribute.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheTaskerResourceType(KHE_TASKER tr)
{
  return tr->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheTaskerArena(KHE_TASKER tr)                                   */
/*                                                                           */
/*  Return tr's arena attribute.                                             */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheTaskerArena(KHE_TASKER tr)
{
  return tr->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerTimeCount(KHE_TASKER tr)                                    */
/*                                                                           */
/*  Return the number of times in tr.  There is one for each time.           */
/*                                                                           */
/*****************************************************************************/

int KheTaskerTimeCount(KHE_TASKER tr)
{
  return HaArrayCount(tr->times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_TIME KheTaskerTime(KHE_TASKER tr, int i)                      */
/*                                                                           */
/*  Return the time of tr for the time with index i.                         */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER_TIME KheTaskerTime(KHE_TASKER tr, int i)
{
  return HaArray(tr->times, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "support for profile grouping"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerSetProfileMaxLen(KHE_TASKER tr, int profile_max_len)       */
/*                                                                           */
/*  Set the profile maximum length.  This can only be done when there        */
/*  are no profile time groups.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTaskerSetProfileMaxLen(KHE_TASKER tr, int profile_max_len)
{
  HnAssert(profile_max_len >= 1, "KheTaskerSetProfileMaxLen: "
    "profile_max_len (%d) out of range (min is 1)", profile_max_len);
  HnAssert(HaArrayCount(tr->profile_time_groups) == 0,
    "KheTaskerSetProfileMaxLen: forbidden when there are profile time groups");
  tr->profile_max_len = profile_max_len;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerProfileMaxLen(KHE_TASKER tr)                                */
/*                                                                           */
/*  Return the profile_max_len attribute.                                    */
/*                                                                           */
/*****************************************************************************/

int KheTaskerProfileMaxLen(KHE_TASKER tr)
{
  return tr->profile_max_len;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerProfileTimeGroupCount(KHE_TASKER tr)                        */
/*                                                                           */
/*  Return the number of profile time groups in tr.                          */
/*                                                                           */
/*****************************************************************************/

int KheTaskerProfileTimeGroupCount(KHE_TASKER tr)
{
  return HaArrayCount(tr->profile_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP KheTaskerProfileTimeGroup(KHE_TASKER tr, int i)   */
/*                                                                           */
/*  Return the i'th profile time group.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_PROFILE_TIME_GROUP KheTaskerProfileTimeGroup(KHE_TASKER tr, int i)
{
  return HaArray(tr->profile_time_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_TASKER tr,            */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Make and add to tr a new profile time group object for tg.               */
/*                                                                           */
/*****************************************************************************/

KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_TASKER tr,
  KHE_TIME_GROUP tg)
{
  KHE_PROFILE_TIME_GROUP res;  int i;  KHE_TIME time;  KHE_TASKER_TIME t;

  /* get the object from the free list or make it fresh */
  if( HaArrayCount(tr->free_profile_time_groups) > 0 )
  {
    res = HaArrayLastAndDelete(tr->free_profile_time_groups);
    HaArrayClear(res->domains);
    HaArrayClear(res->domain_covers);
  }
  else
  {
    HaMake(res, tr->arena);
    HaArrayInit(res->domains, tr->arena);
    HaArrayInit(res->domain_covers, tr->arena);
  }
  res->tasker = tr;
  res->time_group = tg;
  res->index = HaArrayCount(tr->profile_time_groups);
  res->cover = 0;

  /* add it to the set of all profile time groups */
  HaArrayAddLast(tr->profile_time_groups, res);

  /* enrol each of its times */
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    t = HaArray(tr->times, KheTimeIndex(time));
    KheTaskerTimeAddProfileTimeGroup(t, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)               */
/*                                                                           */
/*  Delete ptg.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)
{
  int i;  KHE_TIME time;  KHE_TASKER_TIME t;  KHE_TASKER tr;

  /* unenrol each of its times */
  tr = ptg->tasker;
  for( i = 0;  i < KheTimeGroupTimeCount(ptg->time_group);  i++ )
  {
    time = KheTimeGroupTime(ptg->time_group, i);
    t = HaArray(tr->times, KheTimeIndex(time));
    KheTaskerTimeDeleteProfileTimeGroup(t, ptg);
  }

  /* remove it from the set of all profile time groups */
  HaArrayDeleteAndPlugIndexed(tr->profile_time_groups, ptg->index, index);

  /* add it to the free list */
  HaArrayAddLast(tr->free_profile_time_groups, ptg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerDeleteProfileTimeGroups(KHE_TASKER tr)                     */
/*                                                                           */
/*  Delete all of tr's profile time groups.                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskerDeleteProfileTimeGroups(KHE_TASKER tr)
{
  while( HaArrayCount(tr->profile_time_groups) > 0 )
    KheProfileTimeGroupDelete(HaArrayLast(tr->profile_time_groups));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER KheProfileTimeGroupTasker(KHE_PROFILE_TIME_GROUP ptg)         */
/*                                                                           */
/*  Return ptg's tasker attribute.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER KheProfileTimeGroupTasker(KHE_PROFILE_TIME_GROUP ptg)
{
  return ptg->tasker;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheProfileTimeGroupTimeGroup(KHE_PROFILE_TIME_GROUP ptg)  */
/*                                                                           */
/*  Return ptg's time group attribute.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheProfileTimeGroupTimeGroup(KHE_PROFILE_TIME_GROUP ptg)
{
  return ptg->time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileTimeGroupCover(KHE_PROFILE_TIME_GROUP ptg)                 */
/*                                                                           */
/*  Return the number of tasks (excluding over-long tasks) that cover ptg.   */
/*                                                                           */
/*****************************************************************************/

int KheProfileTimeGroupCover(KHE_PROFILE_TIME_GROUP ptg)
{
  return ptg->cover;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileTimeGroupDomainCount(KHE_PROFILE_TIME_GROUP ptg)           */
/*                                                                           */
/*  Return the number of distinct domains in ptg.                            */
/*                                                                           */
/*****************************************************************************/

int KheProfileTimeGroupDomainCount(KHE_PROFILE_TIME_GROUP ptg)
{
  return HaArrayCount(ptg->domains);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheProfileTimeGroupDomain(KHE_PROFILE_TIME_GROUP ptg, */
/*    int i, int *cover)                                                     */
/*                                                                           */
/*  Return the i'th domain of ptg, also setting *cover to the number of      */
/*  tasks running during ptg that have this domain.                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheProfileTimeGroupDomain(KHE_PROFILE_TIME_GROUP ptg,
  int i, int *cover)
{
  *cover = HaArray(ptg->domain_covers, i);
  return HaArray(ptg->domains, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,       */
/*    KHE_RESOURCE_GROUP domain, int *cover)                                 */
/*                                                                           */
/*  If ptg contains domain, return true with *cover set to its cover.        */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,
  KHE_RESOURCE_GROUP domain, int *cover)
{
  int i;  KHE_RESOURCE_GROUP rg;
  HaArrayForEach(ptg->domains, rg, i)
    if( KheResourceGroupEqual(domain, rg) )
      return *cover = HaArray(ptg->domain_covers, i), *cover > 0;
  return *cover = 0, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerClassCoversProfileTimeGroup(KHE_TASKER_CLASS c,            */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Return true if c covers ptg.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheTaskerClassCoversProfileTimeGroup(KHE_TASKER_CLASS c,
  KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_TASKER_TIME t;  int i;
  HaArrayForEach(c->profile_times, t, i)
    if( t->profile_time_group == ptg )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerClassProfileTimeCount(KHE_TASKER_CLASS c)                   */
/*                                                                           */
/*  Return the number of profile times covered by c.                         */
/*                                                                           */
/*****************************************************************************/

int KheTaskerClassProfileTimeCount(KHE_TASKER_CLASS c)
{
  return HaArrayCount(c->profile_times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_TIME KheTaskerClassProfileTime(KHE_TASKER_CLASS c, int i)     */
/*                                                                           */
/*  Return the i'th profile time covered by c.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER_TIME KheTaskerClassProfileTime(KHE_TASKER_CLASS c, int i)
{
  return HaArray(c->profile_times, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerProfileDebug(KHE_TASKER tr, int verbosity, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug the profile.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskerProfileDebug(KHE_TASKER tr, int verbosity, int indent, FILE *fp)
{
  KHE_PROFILE_TIME_GROUP ptg;  int i, j, k;  KHE_TASKER_TIME t;  KHE_TIME time;
  KHE_TASKER_CLASS c;
  fprintf(fp, "%*s[ Tasker Profile(max_len %d):\n", indent, "",
    tr->profile_max_len);
  HaArrayForEach(tr->profile_time_groups, ptg, i)
  {
    fprintf(fp, "%*s[ TimeGroup(%s)\n", indent + 2, "",
      KheTimeGroupId(ptg->time_group));
    for( j = 0;  j < KheTimeGroupTimeCount(ptg->time_group);  j++ )
    {
      time = KheTimeGroupTime(ptg->time_group, j);
      t = HaArray(tr->times, KheTimeIndex(time));
      HaArrayForEach(t->classes, c, k)
	if( HaArrayCount(c->profile_times) <= c->tasker->profile_max_len )
	  KheTaskerClassDebug(c, 2, indent + 4, fp);
    }
    fprintf(fp, "%*s]\n", indent + 2, "");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping"                                                     */
/*                                                                           */
/*  Implementation note.  The grouping classes are stored in priority        */
/*  order, and the first class (if any) is the leader class.                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCheckOverlaps(KHE_TASKER tr)                                     */
/*                                                                           */
/*  Check that overlaps are correct.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheCheckOverlaps(KHE_TASKER tr)
{
  int i, j, k;  KHE_OVERLAP_TIME_GROUP otg;  KHE_TASKER_CLASS c;
  KHE_TASKER_TIME t;

  /* clear out the check_overlap_cover fields */
  HaArrayForEach(tr->overlap_time_groups, otg, i)
    otg->check_overlap_cover = 0;

  /* add in the overlaps */
  HaArrayForEach(tr->grouping_classes, c, i)
    HaArrayForEach(c->times, t, j)
      if( t->overlap_time_group != NULL )
      {
	if( !HaArrayContains(tr->overlap_time_groups,t->overlap_time_group,&k) )
	  HnAbort("KheCheckOverlaps internal error 1");
        t->overlap_time_group->check_overlap_cover++;
      }

  /* check the overlaps */
  HaArrayForEach(tr->overlap_time_groups, otg, i)
    HnAssert(otg->check_overlap_cover == otg->overlap_cover,
      "KheCheckOverlaps internal error 2");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerGroupingClear(KHE_TASKER tr)                               */
/*                                                                           */
/*  Clear the grouping.                                                      */
/*                                                                           */
/*****************************************************************************/
static bool KheTaskerGroupingDoDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c,
  bool check_new_leader);

void KheTaskerGroupingClear(KHE_TASKER tr)
{
  KheCheckOverlaps(tr);
  while( HaArrayCount(tr->grouping_classes) > 0 )
    KheTaskerGroupingDoDeleteClass(tr, HaArrayLast(tr->grouping_classes),false);
  KheCheckOverlaps(tr);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerClassValidLeaderAndFollower(KHE_TASKER_CLASS leader_c,     */
/*    KHE_TASKER_CLASS follower_c)                                           */
/*                                                                           */
/*  Return true if follower_c can follow leader_c.                           */
/*                                                                           */
/*  Implementation note.  The condition                                      */
/*                                                                           */
/*    follower_r != NULL && follower_r != leader_r                           */
/*                                                                           */
/*  is best understood by considering the two cases leader_r != NULL and     */
/*  leader_r == NULL.  It reduces to follower_r != NULL in the second case.  */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskerClassValidLeaderAndFollower(KHE_TASKER_CLASS leader_c,
  KHE_TASKER_CLASS follower_c)
{
  KHE_RESOURCE leader_r, follower_r;  char *fid, *lid;
  KHE_TASK leader_task, follower_task;  bool res;

  /* make sure assigned resources are compatible */
  leader_r = KheTaskerClassAsstResource(leader_c);
  follower_r = KheTaskerClassAsstResource(follower_c);
  if( follower_r != NULL && follower_r != leader_r )
  {
    fid = (KheResourceId(follower_r) == NULL ? "-" : KheResourceId(follower_r));
    lid = (leader_r == NULL ? "@" :
      KheResourceId(leader_r) == NULL ? "-" : KheResourceId(leader_r));
    if( DEBUG2 )
      fprintf(stderr,"  KheTaskerGroupingAddClass failing (foll %s, lead %s)\n",
	fid, lid);
    return false;
  }

  /* make sure tasks are assignable */
  leader_task = KheTaskerClassTask(leader_c, 0);
  follower_task = KheTaskerClassTask(follower_c, 0);
  res = KheTaskMoveCheck(follower_task, leader_task);
  if( res == false )
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  KheTaskerGroupingAddClass failing (foll ");
      KheTaskDebug(follower_task, 2, -1, stderr);
      fprintf(stderr, ", lead ");
      KheTaskDebug(leader_task, 2, -1, stderr);
      fprintf(stderr, ")\n");
    }
    return false;
  }
  else
    return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerGroupingAddClass(KHE_TASKER tr, KHE_TASKER_CLASS c)        */
/*                                                                           */
/*  Add c to the current grouping.                                           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskerGroupingAddClass(KHE_TASKER tr, KHE_TASKER_CLASS c)
{
  KHE_TASKER_CLASS c2, leader_c;  int pos, i;

  /* make sure c would not overlap */
  KheCheckOverlaps(tr);
  if( KheTaskerClassOverlapsGrouping(c) )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskerGroupingAddClass failing (overlap)\n");
    return false;
  }

  /* add c to the grouping classes */
  KheTaskerClassAddToArray(c, &tr->grouping_classes, &pos);

  if( pos == 0 )
  {
    /* c is the new leader, make sure the other classes are OK with it */
    for( i = 1;  i < HaArrayCount(tr->grouping_classes);  i++ )
    {
      c2 = HaArray(tr->grouping_classes, i);
      if( !KheTaskerClassValidLeaderAndFollower(c, c2) )
      {
	HaArrayDeleteAndShift(tr->grouping_classes, pos);
	if( DEBUG2 )
	  fprintf(stderr, "  KheTaskerGroupingAddClass failing (new leader)\n");
	return false;
      }
    }
  }
  else
  {
    /* c is not the new leader, make sure the existing leader is OK with it */
    leader_c = HaArrayFirst(tr->grouping_classes);
    if( !KheTaskerClassValidLeaderAndFollower(leader_c, c) )
    {
      HaArrayDeleteAndShift(tr->grouping_classes, pos);
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskerGroupingAddClass failing (old leader)\n");
      return false;
    }
  }

  /* all good, set overlap and return true */
  KheTaskerClassAddOverlap(c);
  KheCheckOverlaps(tr);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerGroupingDoDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c,   */
/*    bool check_new_leader)                                                 */
/*                                                                           */
/*  Internal version of KheTaskerGroupingDeleteClass.  The only difference   */
/*  is that checking that the new leader is OK is optional here.             */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskerGroupingDoDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c,
  bool check_new_leader)
{
  int i, pos;  KHE_TASKER_CLASS leader_c, c2;

  /* find c in tr->grouping_classes */
  KheCheckOverlaps(tr);
  if( !HaArrayContains(tr->grouping_classes, c, &pos) )
    HnAbort("KheTaskerGroupingDeleteClass:  c not in grouping");

  /* if c is old leader, make sure the other classes are OK with new leader */
  if( check_new_leader && pos == 0 && HaArrayCount(tr->grouping_classes) >= 3 )
  {
    leader_c = HaArray(tr->grouping_classes, 1);
    for( i = 2;  i < HaArrayCount(tr->grouping_classes);  i++ )
    {
      c2 = HaArray(tr->grouping_classes, i);
      if( !KheTaskerClassValidLeaderAndFollower(leader_c, c2) )
	return false;
    }
  }

  /* delete c and clear its overlap */
  HaArrayDeleteAndShift(tr->grouping_classes, pos);
  KheTaskerClassDeleteOverlap(c);
  KheCheckOverlaps(tr);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerGroupingDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c)     */
/*                                                                           */
/*  Delete c from the current grouping (public version that checks leader).  */
/*                                                                           */
/*****************************************************************************/

bool KheTaskerGroupingDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c)
{
  return KheTaskerGroupingDoDeleteClass(tr, c, true);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerGroupingBuild(KHE_TASKER tr, int max_num, char *debug_str)  */
/*                                                                           */
/*  End grouping, including doing the actual work.                           */
/*                                                                           */
/*****************************************************************************/

int KheTaskerGroupingBuild(KHE_TASKER tr, int max_num, char *debug_str)
{
  KHE_TASKER_CLASS c, final_c;  int i, j, count;
  KHE_TASK leader_task, task;

  /* find out how many groups we really want to make */
  KheCheckOverlaps(tr);
  HaArrayForEach(tr->grouping_classes, c, i)
  {
    count = KheTaskSetTaskCount(c->tasks);
    if( count < max_num )
      max_num = count;
  }

  /* make the groups, deleting the parts and adding the total */
  final_c = NULL;
  for( i = 0;  i < max_num;  i++ )
  {
    /* make the i'th group */
    leader_task = NULL;
    for( j = 0;  j < HaArrayCount(tr->grouping_classes);  j++ )
    {
      c = HaArray(tr->grouping_classes, j);

      /* if we are removing the last task from c, first remove c from the */
      /* grouping, only without checking leader stuff */
      if( KheTaskerClassTaskCount(c) == 1 )
      {
        KheTaskerGroupingDoDeleteClass(tr, c, false);
	j--;
      }

      /* make the first task of c the leader task or a follower task */
      if( leader_task == NULL )
	leader_task = KheTaskerClassLastAndDelete(c);
      else
      {
	task = KheTaskerClassLastAndDelete(c);
	if( !KheTaskMove(task, leader_task) )
	{
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "  about to abort, failed KheTaskMove(");
	    KheTaskDebug(task, 2, -1, stderr);
	    fprintf(stderr, ", ");
	    KheTaskDebug(leader_task, 2, -1, stderr);
	    fprintf(stderr, ")\n");
	  }
	  HnAbort("KheTaskerGroupingBuild internal error");
	}
	if( tr->task_set != NULL )
	  KheTaskSetAddTask(tr->task_set, task);
      }
    }
    if( DEBUG1 )
    {
      KHE_RESOURCE_GROUP rg;
      fprintf(stderr, "  %s made grouped task: ",
	debug_str == NULL ? "KheTaskerGroupingBuild" : debug_str);
      KheTaskDebug(leader_task, 2, -1, stderr);
      fprintf(stderr, " (");
      rg = KheTaskDomain(leader_task);
      if( rg != NULL )
	KheResourceGroupDebug(rg, 1, -1, stderr);
      else
	fprintf(stderr, "null");
      fprintf(stderr, ")\n");
    }

    /* add the newly grouped leader task to its equivalence class */
    if( final_c == NULL )
    {
      KheTimeSetClear(tr->tmp_time_set);
      KheTimeSetAddTaskTimes(tr->tmp_time_set, leader_task);
      final_c = KheTaskerAddTask(tr, leader_task, tr->tmp_time_set);
    }
    else
      KheTaskerClassAddTask(final_c, leader_task);
  }

  /* clear out the grouping array, ready for a fresh grouping */
  /* any classes that became undefined because empty are gone */
  KheTaskerGroupingClear(tr);
  return max_num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskerGroupingClassCount(KHE_TASKER tr)                           */
/*                                                                           */
/*  Return the number of classes in the current grouping.                    */
/*                                                                           */
/*****************************************************************************/

int KheTaskerGroupingClassCount(KHE_TASKER tr)
{
  return HaArrayCount(tr->grouping_classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKER_CLASS KheTaskerGroupingClass(KHE_TASKER tr, int i)            */
/*                                                                           */
/*  Return the i'th class of the current grouping.                           */
/*                                                                           */
/*****************************************************************************/

KHE_TASKER_CLASS KheTaskerGroupingClass(KHE_TASKER tr, int i)
{
  return HaArray(tr->grouping_classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "overlap time groups"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerAddOverlapTimeGroup(KHE_TASKER tr, KHE_TIME_GROUP tg)      */
/*                                                                           */
/*  Add tg as an overlap time group to tr.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTaskerAddOverlapTimeGroup(KHE_TASKER tr, KHE_TIME_GROUP tg)
{
  KHE_OVERLAP_TIME_GROUP res;  KHE_TASKER_TIME t;  int i;  KHE_TIME time;

  /* there may be no groups currently */
  HnAssert(HaArrayCount(tr->grouping_classes) == 0,
    "KheTaskerAddOverlapTimeGroup called when %d groups are present",
     HaArrayCount(tr->grouping_classes));

  /* make the overlap time group object */
  if( HaArrayCount(tr->free_overlap_time_groups) > 0 )
    res = HaArrayLastAndDelete(tr->free_overlap_time_groups);
  else
    HaMake(res, tr->arena);
  res->time_group = tg;
  res->overlap_cover = 0;
  res->check_overlap_cover = 0;

  /* add it to the set of all overlap time groups */
  HaArrayAddLast(tr->overlap_time_groups, res);

  /* enrol each of its times */
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    t = HaArray(tr->times, KheTimeIndex(time));
    if( t->overlap_time_group != NULL )
      HnAbort("KheTaskerAddOverlapTimeGroup: time %s appears in two groups, "
	" %s and %s", KheTimeId(time),
	KheTimeGroupId(t->overlap_time_group->time_group), KheTimeGroupId(tg));
    t->overlap_time_group = res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerAddOverlapFrame(KHE_TASKER tr, KHE_FRAME frame)            */
/*                                                                           */
/*  Add the time groups of frame as overlap time groups to tr.               */
/*                                                                           */
/*****************************************************************************/

void KheTaskerAddOverlapFrame(KHE_TASKER tr, KHE_FRAME frame)
{
  int i;  KHE_TIME_GROUP tg;
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    KheTaskerAddOverlapTimeGroup(tr, tg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerClearOverlapTimeGroups(KHE_TASKER tr)                      */
/*                                                                           */
/*  Clear the overlap time groups of tr.                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskerDeleteOverlapFrame(KHE_TASKER tr)
{
  KHE_OVERLAP_TIME_GROUP otg;  int i, j;  KHE_TIME time;  KHE_TASKER_TIME t;

  /* unenrol the times of the overlap time groups */
  HaArrayForEach(tr->overlap_time_groups, otg, i)
    for( j = 0;  j < KheTimeGroupTimeCount(otg->time_group);  j++ )
    {
      time = KheTimeGroupTime(otg->time_group, j);
      t = HaArray(tr->times, KheTimeIndex(time));
      t->overlap_time_group = NULL;
    }

  /* move the overlap time groups to the free list */
  HaArrayAppend(tr->free_overlap_time_groups, tr->overlap_time_groups, i);
  HaArrayClear(tr->overlap_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOverlapTimeGroupDebug(KHE_OVERLAP_TIME_GROUP otg,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of otg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheOverlapTimeGroupDebug(KHE_OVERLAP_TIME_GROUP otg,
  int verbosity, int indent, FILE *fp)
{
  fprintf(fp, "%*sOverlapTimeGroup(%s, cover %d)\n", indent, "",
    KheTimeGroupId(otg->time_group), otg->overlap_cover);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "test assignment"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerUnassign(KHE_TASKER tr, int stop)                          */
/*                                                                           */
/*  Unassign the first task of each class of tr's grouping, up to stop.      */
/*  Only don't unassign tasks in classes that have an assigned resource.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskerUnassign(KHE_TASKER tr, int stop)
{
  int i;  KHE_TASKER_CLASS c;
  for( i = 0;  i < stop;  i++ )
  {
    c = HaArray(tr->grouping_classes, i);
    if( c->resource == NULL )
    {
      KheTaskUnAssignResource(KheTaskSetTask(c->tasks, 0));
      tr->asst_count_check--;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerAssignResourceToGrouping(KHE_TASKER tr, KHE_RESOURCE r)    */
/*                                                                           */
/*  Assign r to the first task in each class of tr's grouping, unless        */
/*  there is already an assignment.  Return true if all successful.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheTaskerAssignResourceToGrouping(KHE_TASKER tr, KHE_RESOURCE r)
{
  KHE_TASKER_CLASS c;  int i;  KHE_TASK task;
  HaArrayForEach(tr->grouping_classes, c, i)
    if( c->resource == NULL )
    {
      task = KheTaskSetTask(c->tasks, 0);
      if( !KheTaskAssignResource(task, r) )
      {
        KheTaskerUnassign(tr, i);
	return false;
      }
      tr->asst_count_check++;
    }
  return true;
}
*** */


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
  rtm = KheResourceTimetableMonitor(tr->soln, r);
  HaArrayForEach(tr->grouping_classes, c, i)
  {
    task = KheTaskSetTask(c->tasks, 0);
    if( !KheResourceTimetableMonitorTaskAvailableInFrame(rtm, task, tr->frame,
	NULL) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerGroupingTestAsstBegin(KHE_TASKER tr, KHE_RESOURCE *r)      */
/*                                                                           */
/*  Carry out a test assignment of the current grouping.  If successful,     */
/*  set *r to the resource used and return true.  Otherwise set *r to NULL   */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskerGroupingTestAsstBegin(KHE_TASKER tr, KHE_RESOURCE *r)
{
  KHE_TASKER_CLASS c;  int i;

  HnAssert(tr->asst_count_check == 0,
    "KheTaskerGroupingTestAsstBegin internal error");
  c = HaArrayFirst(tr->grouping_classes);
  if( c->resource != NULL )
  {
    if( KheTaskerAssignResourceToGrouping(tr, c->resource) )
      return *r = c->resource, true;
    else
      return *r = NULL, false;
  }
  else
  {
    for( i = 0;  i < KheResourceGroupResourceCount(c->domain);  i++ )
    {
      *r = KheResourceGroupResource(c->domain, i);
      if( KheResourceIsFreeForGrouping(tr, *r) )
      {
	if( KheTaskerAssignResourceToGrouping(tr, *r) )
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
/*  void KheTaskerGroupingTestAsstEnd(KHE_TASKER tr)                         */
/*                                                                           */
/*  Undo a successful KheTaskerGroupingTestAsstBegin.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskerGroupingTestAsstEnd(KHE_TASKER tr)
{
  KheTaskerUnassign(tr, HaArrayCount(tr->grouping_classes));

  ** integrity check **
  HnAssert(tr->asst_count_check == 0,
    "KheTaskerGroupingTestAsstEnd internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerDebug(KHE_TASKER tr, int verbosity, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of tasker tr onto fp with the given verbosity and indent.    */
/*                                                                           */
/*****************************************************************************/

void KheTaskerDebug(KHE_TASKER tr, int verbosity, int indent, FILE *fp)
{
  int i;  KHE_TASKER_TIME t;  KHE_PROFILE_TIME_GROUP ptg;
  KHE_OVERLAP_TIME_GROUP otg;
  fprintf(fp, "%*s[ Tasker(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(tr->soln)),
    KheResourceTypeId(tr->resource_type));
  HaArrayForEach(tr->times, t, i)
    KheTaskerTimeDebug(t, verbosity, indent + 2, fp);
  HaArrayForEach(tr->profile_time_groups, ptg, i)
    KheProfileTimeGroupDebug(ptg, verbosity, indent + 2, fp);
  HaArrayForEach(tr->overlap_time_groups, otg, i)
    KheOverlapTimeGroupDebug(otg, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}

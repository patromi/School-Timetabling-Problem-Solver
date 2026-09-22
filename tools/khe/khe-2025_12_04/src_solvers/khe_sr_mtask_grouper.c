
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
/*  FILE:         khe_sr_mtask_grouper.c                                     */
/*  DESCRIPTION:  MTask grouper                                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1	0
#define DEBUG2	0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MTASK_GROUPER_ENTRY - one entry in an mtask grouper.            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_grouper_entry_rec {
  /* KHE_MTASK_GROUPER_ENTRY		prev; */
  KHE_MTASK				mtask;
  KHE_INTERVAL				interval;
  KHE_TASK_GROUP_DOMAIN			domain;
  /* bool				dummy_entry; */
} *KHE_MTASK_GROUPER_ENTRY;

typedef HA_ARRAY(KHE_MTASK_GROUPER_ENTRY) ARRAY_KHE_MTASK_GROUPER_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MTASK_GROUPER                                                   */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_grouper_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_TASK_GROUP_DOMAIN_FINDER	domain_finder;
  KHE_TASK_GROUPER		task_grouper;
  /* KHE_MTASK_GROUPER_ENTRY	last_entry; */
  ARRAY_KHE_MTASK_GROUPER_ENTRY	entries;
  ARRAY_KHE_MTASK_GROUPER_ENTRY	mtask_grouper_entry_free_list;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_GROUPER_ENTRY"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperEntryMake(KHE_MTASK_GROUPER mtg,  */
/*    KHE_MTASK_GROUPER_ENTRY prev, KHE_MTASK mt, KHE_INTERVAL in,           */
/*    KHE_TASK_GROUP_DOMAIN domain)                                          */
/*                                                                           */
/*  Make a new mtask grouper entry object with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperEntryMake(KHE_MTASK_GROUPER mtg,
  KHE_MTASK mt, KHE_INTERVAL in, KHE_TASK_GROUP_DOMAIN domain)
{
  KHE_MTASK_GROUPER_ENTRY res;
  HnAssert(domain != NULL, "KheMTaskGrouperEntryMake internal error");
  if( HaArrayCount(mtg->mtask_grouper_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(mtg->mtask_grouper_entry_free_list);
  else
    HaMake(res, mtg->arena);
  res->mtask = mt;
  res->interval = in;
  res->domain = domain;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_GROUPER"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUPER KheMTaskGrouperMake(KHE_SOLN soln, HA_ARENA a)         */
/*                                                                           */
/*  Make a new task grouper object.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_GROUPER KheMTaskGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a)
{
  KHE_MTASK_GROUPER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->domain_finder = tgdf;
  res->task_grouper = KheTaskGrouperMake(soln, days_frame, tgdf, a);
  HaArrayInit(res->entries, a);
  HaArrayInit(res->mtask_grouper_entry_free_list, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperClear(KHE_MTASK_GROUPER mtg)                         */
/*                                                                           */
/*  Clear out mtg, ready to start a new task group.                          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGrouperClear(KHE_MTASK_GROUPER mtg)
{
  int i;
  HaArrayAppend(mtg->mtask_grouper_entry_free_list, mtg->entries, i);
  HaArrayClear(mtg->entries);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGrouperAddMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)        */
/*                                                                           */
/*  If mt is compatible with any other mtasks that have been added, add      */
/*  it and return true.  Otherwise add nothing and return false.             */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGrouperAddMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)
{
  KHE_MTASK_GROUPER_ENTRY entry, prev;  KHE_RESOURCE_GROUP mt_domain, new_rg;
  KHE_INTERVAL mt_in, new_in;  KHE_TASK_GROUP_DOMAIN new_domain;
  bool unchecked;


  /***************************************************************************/
  /*                                                                         */
  /*  The mtask must be non-NULL                                             */
  /*                                                                         */
  /***************************************************************************/

  HnAssert(mt != NULL, "KheMTaskGrouperAddMTask internal error:  mt is NULL");

  /* get some basic facts about mt */
  mt_in = KheMTaskInterval(mt);
  mt_domain = KheMTaskDomain(mt);
  unchecked = false;

  /*************************************************************************/
  /*                                                                       */
  /*  1. The mask cannot have an empty domain.                             */
  /*                                                                       */
  /*************************************************************************/

  if( KheResourceGroupResourceCount(mt_domain) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheMTaskGrouperAddMTask returning false "
	"(mt has empty domain)\n");
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  2. The mtask cannot have a fixed assignment.                         */
  /*                                                                       */
  /*************************************************************************/

  if( KheMTaskAssignIsFixed(mt) )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddInitialTask returning false "
	"(mtask has fixed assignment)\n");
    return false;
  }


  if( HaArrayCount(mtg->entries) == 0 )
  {
    /*************************************************************************/
    /*                                                                       */
    /*  3. Interval of days must be disjoint from prev.                      */
    /*                                                                       */
    /*************************************************************************/

    /* nothing to do here when prev is NULL */


    /*************************************************************************/
    /*                                                                       */
    /*  4. The intersection of task's domain with the others is non-empty.   */
    /*                                                                       */
    /*************************************************************************/

    if( !unchecked && KheResourceGroupResourceCount(mt_domain) == 0 )
      return false;
    new_domain = KheTaskGroupDomainMake(mtg->domain_finder, NULL, mt_domain);


    /*************************************************************************/
    /*                                                                       */
    /*  All good so make the new object and return true.                     */
    /*                                                                       */
    /*************************************************************************/

    entry = KheMTaskGrouperEntryMake(mtg, mt, mt_in, new_domain);
    HaArrayAddLast(mtg->entries, entry);
    return true;
  }
  else
  {
    /*************************************************************************/
    /*                                                                       */
    /*  3. Interval of days must be disjoint from prev.                      */
    /*                                                                       */
    /*************************************************************************/

    prev = HaArrayLast(mtg->entries);
    if( !unchecked && !KheIntervalDisjoint(prev->interval, mt_in) )
      return false;
    new_in = KheIntervalUnion(prev->interval, mt_in);


    /*************************************************************************/
    /*                                                                       */
    /*  4. The intersection of task's domain with the others is non-empty.   */
    /*                                                                       */
    /*************************************************************************/

    new_domain = KheTaskGroupDomainMake(mtg->domain_finder,
      prev->domain, mt_domain);
    new_rg = KheTaskGroupDomainValue(new_domain);
    if( !unchecked && KheResourceGroupResourceCount(new_rg) == 0 )
      return false;


    /*************************************************************************/
    /*                                                                       */
    /*  All good so make the new object and return true.                     */
    /*                                                                       */
    /*************************************************************************/

    entry = KheMTaskGrouperEntryMake(mtg, mt, new_in, new_domain);
    HaArrayAddLast(mtg->entries, entry);
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperDeleteMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)     */
/*                                                                           */
/*  Delete previously added task task from mtg.  Here task must be the most  */
/*  recently added but not deleted task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGrouperDeleteMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)
{
  KHE_MTASK_GROUPER_ENTRY last;
  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperDeleteMTask internal error (no mtasks)");
  last = HaArrayLastAndDelete(mtg->entries);
  HnAssert(last->mtask == mt,
    "KheMTaskGrouperDeleteMTask internal error (mt not last added)");
  HaArrayAddLast(mtg->mtask_grouper_entry_free_list, last);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGrouperMTaskCount(KHE_MTASK_GROUPER mtg)                     */
/*                                                                           */
/*  Return the number of mtasks in mtg.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMTaskGrouperMTaskCount(KHE_MTASK_GROUPER mtg)
{
  return HaArrayCount(mtg->entries);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskGrouperMTask(KHE_MTASK_GROUPER mtg, int i)             */
/*                                                                           */
/*  Return the i'th mtask of mtg.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskGrouperMTask(KHE_MTASK_GROUPER mtg, int i)
{
  KHE_MTASK_GROUPER_ENTRY entry;
  entry = HaArray(mtg->entries, i);
  return entry->mtask;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGrouperContainsMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)   */
/*                                                                           */
/*  Return true if mtg contains mt.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGrouperContainsMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)
{
  KHE_MTASK_GROUPER_ENTRY entry;  int i;
  HaArrayForEach(mtg->entries, entry, i)
    if( entry->mtask == mt )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperCopy(KHE_MTASK_GROUPER dst_mtg,                      */
/*    KHE_MTASK_GROUPER src_mtg)                                             */
/*                                                                           */
/*  Copy the contents of src_mtg into dst_mtg.                               */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGrouperCopy(KHE_MTASK_GROUPER dst_mtg,
  KHE_MTASK_GROUPER src_mtg)
{
  KHE_MTASK_GROUPER_ENTRY entry;  int i;
  KheMTaskGrouperClear(dst_mtg);
  HaArrayForEach(src_mtg->entries, entry, i)
    KheMTaskGrouperAddMTask(dst_mtg, entry->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMTaskGrouperCost(KHE_MTASK_GROUPER mtg)                      */
/*                                                                           */
/*  Return the cost of grouping mtg's mtasks.                                */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMTaskGrouperCost(KHE_MTASK_GROUPER mtg)
{
  KHE_MTASK_GROUPER_ENTRY last;
  KHE_RESOURCE_GROUP domain;  KHE_RESOURCE r;  KHE_INTERVAL in;
  KHE_GROUP_MONITOR gm;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_MARK mark;  KHE_TASK_SET ts;  KHE_TASK task;  int i;
  KHE_MTASK_GROUPER_ENTRY entry;  KHE_COST non_asst_cost, asst_cost, res;
  KHE_FRAME days_frame;

  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperHasCost internal error (no mtasks)");

  /* find a suitable resource to assign the tasks to */
  last = HaArrayLast(mtg->entries);
  domain = KheTaskGroupDomainValue(last->domain);
  HnAssert(KheResourceGroupResourceCount(domain) > 0,
    "KheMTaskGrouperEntryCost internal error 1");
  r = KheResourceGroupResource(domain, 0);
  days_frame = KheMTaskFinderDaysFrame(KheMTaskFinder(last->mtask));

  /* find the interval of days_frame covered by these tasks */
  in = last->interval;
  in = KheIntervalMake(KheIntervalFirst(in) - 1, KheIntervalLast(in) + 1);

  /* find the cluster busy times and limit busy times monitors during in */
  /* soln = KheMTaskFinderSoln(KheMTaskFinder(last->mtask)); */
  /* soln = KheFrameSoln(days_frame); */
  gm = KheGroupMonitorMake(mtg->soln, 11, "comb grouping");
  rtm = KheResourceTimetableMonitor(mtg->soln, r);
  KheResourceTimetableMonitorAddInterval(rtm, days_frame,
    KheIntervalFirst(in), KheIntervalLast(in), false, gm);

  /* make sure r is free during in */
  mark = KheMarkBegin(mtg->soln);
  ts = KheTaskSetMake(mtg->soln);
  KheResourceTimetableMonitorAddProperRootTasksInInterval(rtm, days_frame,
    KheIntervalFirst(in), KheIntervalLast(in), true, ts);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( KheTaskAssignIsFixed(task) )
      KheTaskAssignUnFix(task);
    if( KheTaskAsstResource(task) != NULL )
      KheTaskUnAssignResource(task);
  }
  KheTaskSetDelete(ts);

  /* assign r to the mtasks of last; this could get quite messy */
  HaArrayForEachReverse(mtg->entries, entry, i)
  {
    HnAssert(KheMTaskTaskCount(entry->mtask) > 0,
      "KheMTaskGrouperEntryCost internal error 2");
    task = KheMTaskTask(entry->mtask, 0, &non_asst_cost, &asst_cost);
    if( KheTaskAssignIsFixed(task) )
      KheTaskAssignUnFix(task);
    if( !KheTaskAssignResource(task, r) )
      HnAbort("KheMTaskGrouperEntryCost internal error 3");
  }

  /* work out the cost */
  res = KheMonitorCost((KHE_MONITOR) gm);
  KheGroupMonitorDelete(gm);

  /* return to the initial state, and return the cost */
  KheMarkEnd(mark, true);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperMakeOneGroup(KHE_MTASK_GROUPER_ENTRY last,           */
/*    KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa)                            */
/*                                                                           */
/*  Make one group from the entries starting at last.  This must be          */
/*  possible.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGrouperMakeOneGroup(KHE_MTASK_GROUPER mtg,
  KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  KHE_MTASK_GROUPER_ENTRY entry;  int i;

  KheTaskGrouperClear(mtg->task_grouper);
  HaArrayForEach(mtg->entries, entry, i)
  {
    HnAssert(KheMTaskUnassignedTaskCount(entry->mtask) > 0,
      "KheMTaskGrouperMakeOneGroup internal error 1");
    task = KheMTaskTask(entry->mtask, KheMTaskAssignedTaskCount(entry->mtask),
      &non_asst_cost, &asst_cost);
    if( DEBUG1 && !KheTaskIsProperRoot(task) )
    {
      fprintf(stderr, "KheMTaskGrouperMakeOneGroup failing on task:\n");
      KheTaskDebug(task, 2, 2, stderr);
      fprintf(stderr, "  proper root is:\n");
      KheTaskDebug(KheTaskProperRoot(task), 2, 2, stderr);
    }
    if( !KheTaskGrouperAddTask(mtg->task_grouper, task) )
      HnAbort("KheMTaskGroupMakeOneGroup internal error 2");
  }
  KheMTaskFinderTaskGrouperMakeGroup(mtf, mtg->task_grouper, sa);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGrouperMakeGroups(KHE_MTASK_GROUPER mtg, int max_num,        */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make up to max_num groups; return the number actually made.              */
/*                                                                           */
/*****************************************************************************/

int KheMTaskGrouperMakeGroups(KHE_MTASK_GROUPER mtg, int max_num,
  KHE_SOLN_ADJUSTER sa)
{
  int num, res, val, i;  KHE_MTASK_FINDER mtf;
  KHE_MTASK_GROUPER_ENTRY entry;

  /* sort out the cases of no mtasks and one mtask */
  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperMakeGroups: no tasks to group");
  if( HaArrayCount(mtg->entries) == 1 )
    return 0;
  mtf = KheMTaskFinder(HaArrayFirst(mtg->entries)->mtask);

  /* set num to the number of groups to make */
  num = max_num;
  HaArrayForEachReverse(mtg->entries, entry, i)
  {
    val = KheMTaskUnassignedTaskCount(entry->mtask);
    if( val < num )
      num = val;
  }

  /* make num groups */
  for( res = 0;  res < num;  res++ )
    KheMTaskGrouperMakeOneGroup(mtg, mtf, sa);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperDebugHeader(KHE_MTASK_GROUPER mtg, FILE *fp)         */
/*                                                                           */
/*  Print ono fp the header part of the debug print of mtg.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGrouperDebugHeader(KHE_MTASK_GROUPER mtg, FILE *fp)
{
  fprintf(fp, "KheMTaskGrouper (%d mtasks)", HaArrayCount(mtg->entries));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperDebug(KHE_MTASK_GROUPER mtg,                         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of mtg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGrouperDebug(KHE_MTASK_GROUPER mtg,
  int verbosity, int indent, FILE *fp)
{
  KHE_MTASK_GROUPER_ENTRY entry;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheMTaskGrouperDebugHeader(mtg, fp);
    fprintf(fp, "\n");
    HaArrayForEach(mtg->entries, entry, i)
      KheMTaskDebug(entry->mtask, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheMTaskGrouperDebugHeader(mtg, fp);
}

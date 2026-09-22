
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

#define DEBUG1	1


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
  /* res->domain_finder = KheTaskGroupDomainFinderMake(soln, a); */
  res->task_grouper = KheTaskGrouperMake(soln, days_frame, tgdf, a);
  /* res->last_entry = NULL; */
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
void KheMTaskGrouperEntryFree(KHE_MTASK_GROUPER_ENTRY mtge,
  KHE_MTASK_GROUPER mtg);

void KheMTaskGrouperClear(KHE_MTASK_GROUPER mtg)
{
  /* ***
  KHE_MTASK_GROUPER_ENTRY mtge;
  for( mtge = mtg->last_entry;  mtge != NULL;  mtge = mtge->prev )
    KheMTaskGrouperEntryFree(mtge, mtg);  ** does not change mtge->prev **
  mtg->last_entry = NULL;
  *** */
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
static bool KheMTaskGrouperEntryMakeMTask(KHE_MTASK_GROUPER mtg,
  KHE_MTASK_GROUPER_ENTRY prev, KHE_MTASK mt, bool unchecked,
  KHE_MTASK_GROUPER_ENTRY *res);

bool KheMTaskGrouperAddMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt)
{
  /* ***
  KHE_MTASK_GROUPER_ENTRY mtge;
  if( KheMTaskGrouperEntryMakeMTask(mtg, mtg->last_entry, mt, false, &mtge) )
  {
    mtg->last_entry = mtge;
    return true;
  }
  else
  {
    ** rubbish, mtge is NULL here  KheMTaskGrouperEntryFree(mtge, mtg); **
    return false;
  }
  *** */

  KHE_MTASK_GROUPER_ENTRY entry, last;
  last = (HaArrayCount(mtg->entries) == 0 ? NULL : HaArrayLast(mtg->entries));
  entry = KheMTaskGrouperMakeEntry(mtg);
  if( KheMTaskGrouperEntryAddMTask(last, mt, mtg->domain_finder, entry) )
  {
    HaArrayAddLast(mtg->entries, entry);
    return true;
  }
  else
  {
    KheMTaskGrouperFreeEntry(mtg, entry);
    return false;
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
  /* ***
  KHE_MTASK_GROUPER_ENTRY last;
  last = mtg->last_entry;
  HnAssert(last != NULL,
    "KheMTaskGrouperDeleteMTask internal error (no mtasks)");
  HnAssert(last->mtask == mt,
    "KheMTaskGrouperDeleteMTask internal error (mtask not last added)");
  mtg->last_entry = last->prev;
  KheMTaskGrouperEntryFree(last, mtg);
  *** */

  KHE_MTASK_GROUPER_ENTRY last;
  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperDeleteMTask internal error (no tasks)");
  last = HaArrayLast(mtg->entries);
  HnAssert(last->mtask == mt,
    "KheMTaskGrouperDeleteMTask internal error (task not last added)");
  HaArrayDeleteLast(mtg->entries);
  KheMTaskGrouperFreeEntry(mtg, last);
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
static KHE_COST KheMTaskGrouperEntryCost(KHE_MTASK_GROUPER_ENTRY last,
  KHE_MTASK_GROUPER mtg);

KHE_COST KheMTaskGrouperCost(KHE_MTASK_GROUPER mtg)
{
  HnAssert(mtg->last_entry != NULL,
    "KheMTaskGrouperHasCost internal error (no mtasks)");
  return KheMTaskGrouperEntryCost(mtg->last_entry, mtg);
  /* ***
  KHE_MTASK_GROUPER_ENTRY last;
  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperHasCost internal error (no tasks)");
  last = HaArrayLast(mtg->entries);
  return KheMTaskGrouperEntryCost(last);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGrouperMakeGroups(KHE_MTASK_GROUPER mtg, int max_num,        */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make up to max_num groups; return the number actually made.              */
/*                                                                           */
/*****************************************************************************/
static int KheMTaskGrouperEntryMakeGroups(KHE_MTASK_GROUPER_ENTRY mtge,
  int max_num, KHE_MTASK_GROUPER mtg, KHE_SOLN_ADJUSTER sa);

int KheMTaskGrouperMakeGroups(KHE_MTASK_GROUPER mtg, int max_num,
  KHE_SOLN_ADJUSTER sa)
{
  HnAssert(mtg->last_entry != NULL,
    "KheMTaskGrouperMakeGroups: no mtasks to group");
  return KheMTaskGrouperEntryMakeGroups(mtg->last_entry, max_num, mtg, sa);
  /* ***
  HnAssert(HaArrayCount(mtg->entries) > 0,
    "KheMTaskGrouperMakeGroups: no tasks to group");
  return KheMTaskGrouperEntryMakeGroups(mtg, HaArrayLast(mtg->entries),
    max_num, sa);
  *** */
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
  fprintf(fp, "KheMTaskGrouper");
  /* fprintf(fp, "KheMTaskGrouper (%d mtasks)", HaArrayCount(mtg->entries)); */
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
  KHE_MTASK_GROUPER_ENTRY mtge;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheMTaskGrouperDebugHeader(mtg, fp);
    fprintf(fp, "\n");
    for( mtge = mtg->last_entry;  mtge != NULL;  mtge = mtge->prev )
      KheMTaskDebug(mtge->mtask, verbosity, indent + 2, fp);
    /* ***
    HaArrayForEach(mtg->entries, entry, i)
      KheMTaskDebug(entry->mtask, verbosity, indent + 2, fp);
    *** */
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheMTaskGrouperDebugHeader(mtg, fp);
}


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
  KHE_MTASK_GROUPER_ENTRY prev, KHE_MTASK mt, KHE_INTERVAL in,
  KHE_TASK_GROUP_DOMAIN domain)
{
  KHE_MTASK_GROUPER_ENTRY res;
  HnAssert(domain != NULL, "KheMTaskGrouperEntryMake internal error");
  if( HaArrayCount(mtg->mtask_grouper_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(mtg->mtask_grouper_entry_free_list);
  else
    HaMake(res, mtg->arena);
  res->prev = prev;
  res->mtask = mt;
  res->interval = in;
  res->domain = domain;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGrouperEntryAddMTask(KHE_MTASK_GROUPER_ENTRY prev,          */
/*    KHE_MTASK mt, KHE_MTASK_GROUPER_ENTRY next)                            */
/*                                                                           */
/*  If task is compatible with prev, make a new entry in *next (whose        */
/*  memory must have already been allocated) and return true.  Otherwise     */
/*  change nothing and return false.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGrouperEntryMakeMTask(KHE_MTASK_GROUPER mtg,
  KHE_MTASK_GROUPER_ENTRY prev, KHE_MTASK mt, bool unchecked,
  KHE_MTASK_GROUPER_ENTRY *res)
{
  KHE_RESOURCE_GROUP mt_domain, new_rg;
  /* KHE_MTASK new_leader; */  KHE_INTERVAL mt_in, new_in;
  KHE_TASK_GROUP_DOMAIN new_domain;
  /* KHE_TASK_GROUP_DOMAIN_TYPE new_type; */
  /* bool has_fixed_unassigned, new_has_fixed_unassigned; */
  /* KHE_RESOURCE assigned_resource, new_assigned_resource; */
  /* KHE_MTASK_GROUPER_ENTRY entry; */


  /***************************************************************************/
  /*                                                                         */
  /*  The mtask must be non-NULL                                             */
  /*                                                                         */
  /***************************************************************************/

  HnAssert(mt != NULL, "KheMTaskGrouperAddMTask internal error:  mt is NULL");

  /* get some basic facts about mt */
  mt_in = KheMTaskInterval(mt);
  mt_domain = KheMTaskDomain(mt);

  if( prev == NULL )
  {
    /*************************************************************************/
    /*                                                                       */
    /*  1. Interval of days must be disjoint from prev.                      */
    /*                                                                       */
    /*************************************************************************/

    /* nothing to do here when prev is NULL */


    /*************************************************************************/
    /*                                                                       */
    /*  2. The intersection of task's domain with the others is non-empty.   */
    /*                                                                       */
    /*************************************************************************/

    if( !unchecked && KheResourceGroupResourceCount(mt_domain) == 0 )
      return *res = NULL, false;
    new_domain = KheTaskGroupDomainMake(mtg->domain_finder, NULL, mt_domain);


    /*************************************************************************/
    /*                                                                       */
    /*  All good so make the new object and return true.                     */
    /*                                                                       */
    /*************************************************************************/

    *res = KheMTaskGrouperEntryMake(mtg, prev, mt, mt_in, new_domain);
    /* ***
    KheMTaskGrouperEntryInit(next, mt, **mt,** new_domain, prev, mt_in, false);
    *** */
    return true;
  }
  else
  {
    /*************************************************************************/
    /*                                                                       */
    /*  1. Interval of days must be disjoint from prev.                      */
    /*                                                                       */
    /*************************************************************************/

    if( !unchecked && !KheIntervalDisjoint(prev->interval, mt_in) )
      return *res = NULL, false;
    new_in = KheIntervalUnion(prev->interval, mt_in);


    /*************************************************************************/
    /*                                                                       */
    /*  2. The intersection of task's domain with the others is non-empty.   */
    /*                                                                       */
    /*************************************************************************/

    new_domain = KheTaskGroupDomainMake(mtg->domain_finder,
      prev->domain, mt_domain);
    new_rg = KheTaskGroupDomainValue(new_domain /* , &new_type */);
    if( !unchecked && KheResourceGroupResourceCount(new_rg) == 0 )
      return *res = NULL, false;
    /* ***
    switch( new_type )
    {
      case KHE_TASK_GROUP_DOMAIN_RG_SUBSET_PREV:

	new_leader = mt;
	break;

      case KHE_TASK_GROUP_DOMAIN_PREV_SUBSET_RG:
      case KHE_TASK_GROUP_DOMAIN_PREV_INTERSECT_RG:

	new_leader = prev->leader;
	break;

      case KHE_TASK_GROUP_DOMAIN_RG_ONLY:
      default:

	HnAbort("KheTaskGrouperEntryAddTask internal error");
	new_leader = NULL;  ** keep compiler happy **
	break;
    }
    *** */


    /*************************************************************************/
    /*                                                                       */
    /*  All good so make the new object and return true.                     */
    /*                                                                       */
    /*************************************************************************/

    *res = KheMTaskGrouperEntryMake(mtg, prev, mt, new_in, new_domain);
    /* ***
    KheMTaskGrouperEntryInit(next, mt, ** new_leader, ** new_domain, prev,
      new_in, false);
    *** */
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperEntryAddDummy(KHE_MTASK_GROUPER_ENTRY *prev,         */
/*    KHE_MTASK_GROUPER_ENTRY *next)                                         */
/*                                                                           */
/*  Add a dummy entry into the sequence.  It has the same attributes as      */
/*  prev (which must be non-NULL), but it does not add a new task and it     */
/*  is ignored when making groups.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMTaskGrouperEntryAddDummy(KHE_MTASK_GROUPER_ENTRY prev,
  KHE_MTASK_GROUPER_ENTRY next)
{
  KheMTaskGrouperEntryInit(next, prev->mtask, ** prev->leader, **
    prev->domain, prev, prev->interval, true);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperEntryFree(KHE_MTASK_GROUPER_ENTRY mtge,              */
/*    KHE_MTASK_GROUPER mtg)                                                 */
/*                                                                           */
/*  Free mtge.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGrouperEntryFree(KHE_MTASK_GROUPER_ENTRY mtge,
  KHE_MTASK_GROUPER mtg)
{
  HaArrayAddLast(mtg->mtask_grouper_entry_free_list, mtge);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGrouperEntryCopy(KHE_MTASK_GROUPER_ENTRY dst_last,          */
/*    KHE_MTASK_GROUPER_ENTRY src_last)                                      */
/*                                                                           */
/*  Copy src_last into dst_last.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMTaskGrouperEntryCopy(KHE_MTASK_GROUPER_ENTRY dst_last,
  KHE_MTASK_GROUPER_ENTRY src_last)
{
  KheMTaskGrouperEntryInit(dst_last, src_last->mtask, ** src_last->leader, **
    src_last->domain, src_last->prev, src_last->interval,
    src_last->dummy_entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperLastEntry(KHE_MTASK_GROUPER mtg)  */
/*                                                                           */
/*  Return the last entry of mtg (possibly NULL).                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperLastEntry(KHE_MTASK_GROUPER mtg)
{
  return mtg->last_entry;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMTaskGrouperEntryCost(KHE_MTASK_GROUPER_ENTRY last)          */
/*                                                                           */
/*  Return the cost of grouping last's tasks.                                */
/*                                                                           */
/*  Implementation note.  This function trashes the mtask structure.  But    */
/*  that is all right, because it puts everything back again at the end.     */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheMTaskGrouperEntryCost(KHE_MTASK_GROUPER_ENTRY last,
  KHE_MTASK_GROUPER mtg)
{
  KHE_RESOURCE_GROUP domain;  KHE_RESOURCE r;  KHE_INTERVAL in;
  /* KHE_SOLN soln; */  KHE_GROUP_MONITOR gm;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_MARK mark;  KHE_TASK_SET ts;  KHE_TASK task;  int i;
  KHE_MTASK_GROUPER_ENTRY entry;  KHE_COST non_asst_cost, asst_cost, res;
  KHE_FRAME days_frame;  /* KHE_TASK_GROUP_DOMAIN_TYPE type; */

  /* find a domain of minimum size */
  /* *** rubbish, not needed
  HnAssert(last != NULL,
    "KheMTaskGrouperEntryCost internal error:  last == NULL");
  leader_mtask = NULL;
  leader_domain = NULL;
  leader_domain_count = INT_MAX;
  for( entry = last;  entry != NULL;  entry = entry->prev )
  {
    HnAssert(entry->mtask != NULL, "KheMTaskGrouperEntryCost internal error");
    domain = KheMTaskDomain(entry->mtask);
    domain_count = KheResourceGroupResourceCount(domain);
    if( domain_count < leader_domain_count )
    {
      leader_mtask = entry->mtask;
      leader_domain = domain;
      leader_domain_count = domain_count;
    }
  }
  *** */

  /* find a suitable resource to assign the tasks to */
  /* domain = KheMTaskDomain(last->leader); */
  domain = KheTaskGroupDomainValue(last->domain /* , &type */);
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
  for( entry = last;  entry != NULL;  entry = entry->prev )
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
  KHE_MTASK_GROUPER_ENTRY last, KHE_MTASK_FINDER mtf,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  KHE_MTASK_GROUPER_ENTRY entry;

  KheTaskGrouperClear(mtg->task_grouper);
  for( entry = last;  entry != NULL;  entry = entry->prev )
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

/*  ***
static void KheMTaskGrouperMakeOneGroup(KHE_MTASK_GROUPER_ENTRY last,
  KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  KHE_MTASK_GROUPER_ENTRY entry;

  KheMTaskFinderTaskGrouperClear(mtf);
  for( entry = last;  entry != NULL;  entry = entry->prev )
  {
    HnAssert(KheMTaskUnassignedTaskCount(entry->mtask) > 0,
      "KheMTaskGrouperMakeOneGroup internal error 1");
    task = KheMTaskTask(entry->mtask, KheMTaskAssignedTaskCount(entry->mtask),
      &non_asst_cost, &asst_cost);
    if( !KheMTaskFinderTaskGrouperAddTask(mtf, task) )
      HnAbort("KheMTaskGroupMakeOneGroup internal error 2");
  }
  KheMTaskFinderTaskGrouperMakeGroup(mtf, sa);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGrouperEntryMakeGroups(KHE_MTASK_GROUPER_ENTRY mtge,         */
/*    int max_num, KHE_MTASK_GROUPER mtg, KHE_SOLN_ADJUSTER sa)              */
/*                                                                           */
/*  Make up to max_num groups.  Return the number actually made.  If         */
/*  sa != NULL, record what was done in sa so that it can be undone later.   */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskGrouperEntryMakeGroups(KHE_MTASK_GROUPER_ENTRY mtge,
  int max_num, KHE_MTASK_GROUPER mtg, KHE_SOLN_ADJUSTER sa)
{
  int num, res, val;  KHE_MTASK_FINDER mtf;
  KHE_MTASK_GROUPER_ENTRY entry;

  /* sort out the cases of no mtasks and one mtask */
  HnAssert(mtge != NULL,
    "KheMTaskGrouperEntryMakeGroups internal error:  mtge == NULL");
  if( mtge->prev == NULL )
    return 0;
  mtf = KheMTaskFinder(mtge->mtask);

  /* set num to the number of groups to make */
  num = max_num;
  for( entry = mtge;  entry != NULL;  entry = entry->prev )
  {
    val = KheMTaskUnassignedTaskCount(entry->mtask);
    if( val < num )
      num = val;
  }

  /* make num groups */
  /* ***
  if( DEBUG1 )
  {
    fprintf(stderr, "  %s: %d x ", debug_str, num);
    KheMTaskGroupDebug(mg, 2, 0, stderr);
  }
  *** */
  for( res = 0;  res < num;  res++ )
    KheMTaskGrouperMakeOneGroup(mtg, mtge, mtf, sa);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperEntryPrev(                        */
/*    KHE_MTASK_GROUPER_ENTRY mtge)                                          */
/*                                                                           */
/*  Return the prev attribute of mtge.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperEntryPrev(KHE_MTASK_GROUPER_ENTRY mtge)
{
  return mtge->prev;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskGrouperEntryMTask(KHE_MTASK_GROUPER_ENTRY mtge)        */
/*                                                                           */
/*  Return the mtask attribute of mtge.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskGrouperEntryMTask(KHE_MTASK_GROUPER_ENTRY mtge)
{
  return mtge->mtask;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskGrouperEntryInterval(KHE_MTASK_GROUPER_ENTRY mtge)  */
/*                                                                           */
/*  Return the interval attribute of mtge.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheMTaskGrouperEntryInterval(KHE_MTASK_GROUPER_ENTRY mtge)
{
  return mtge->interval;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN KheMTaskGrouperEntryDomain(                        */
/*    KHE_MTASK_GROUPER_ENTRY mtge)                                          */
/*                                                                           */
/*  Return the domain attribute of mtge.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUP_DOMAIN KheMTaskGrouperEntryDomain(KHE_MTASK_GROUPER_ENTRY mtge)
{
  return mtge->domain;
}

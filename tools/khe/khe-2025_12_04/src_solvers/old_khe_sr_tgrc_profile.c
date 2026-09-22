
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
/*  FILE:         khe_sr_tgrc_profile.c                                      */
/*  DESCRIPTION:  Task grouping by resource constraints - profile grouping   */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0  /* KheDoProfile */
#define DEBUG4 0
#define DEBUG5 0  /* KheDoProfileGroupingMonitors */
#define	DEBUG7 0  /* profile grouping */
#define	DEBUG8 0  /* profile grouping */
#define	DEBUG9 0  /* profile grouping */
#define	DEBUG11 0 /* KheProfile */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_PROFILE_DOMAIN                                                  */
/*                                                                           */
/*  One domain for one profile time group, containing the resource group     */
/*  that defines the domain, the mtasks that have that domain during that    */
/*  time group, and their cover (total number of tasks needing assignment).  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;

typedef struct khe_profile_domain_rec {
  KHE_RESOURCE_GROUP	domain;
  ARRAY_KHE_MTASK	mtasks;
  int			cover;
} *KHE_PROFILE_DOMAIN;

typedef HA_ARRAY(KHE_PROFILE_DOMAIN) ARRAY_KHE_PROFILE_DOMAIN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP                                                   */
/*                                                                           */
/*  A time group whose cover is needed by profile grouping.                  */
/*                                                                           */
/*  The three priv_* fields should not be accessed directly.  These fields   */
/*  become out of date when a group is made, and we take care to mark each   */
/*  out-of-date group and bring it up to date the first time it's needed.    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_profile_solver_rec *KHE_PROFILE_SOLVER;

typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;

typedef struct khe_profile_time_group_rec {
  KHE_PROFILE_SOLVER		profile_solver;
  KHE_TIME_GROUP		time_group;
  int				groups_made_here;
  bool				priv_up_to_date; /* true if foll. up to date */
  int				priv_cover;
  ARRAY_KHE_PROFILE_DOMAIN	priv_profile_domains;
} *KHE_PROFILE_TIME_GROUP;

typedef HA_ARRAY(KHE_PROFILE_TIME_GROUP) ARRAY_KHE_PROFILE_TIME_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_PROFILE_NODE - one node in the dynamic programming graph        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_profile_node_rec {
  KHE_MTASK_GROUP		mg;
  KHE_COST			total_cost;
  struct khe_profile_node_rec	*prev_node;
} *KHE_PROFILE_NODE;

typedef HA_ARRAY(KHE_PROFILE_NODE) ARRAY_KHE_PROFILE_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DEBUG_COLUMN - one column of the debug print                         */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_DEBUG_COLUMN_UNUSED,
  KHE_DEBUG_COLUMN_DONT_KNOW_YET,
  KHE_DEBUG_COLUMN_STARTING,
  KHE_DEBUG_COLUMN_CONTINUING
} KHE_DEBUG_COLUMN_STATE;

typedef struct khe_debug_column_rec {
  KHE_DEBUG_COLUMN_STATE	state;
  KHE_TASK			leader_task;
  KHE_TASK			task;
  /* KHE_COST			non_asst_cost; */
  /* KHE_COST			asst_cost; */
} *KHE_DEBUG_COLUMN;

typedef HA_ARRAY(KHE_DEBUG_COLUMN) ARRAY_KHE_DEBUG_COLUMN;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_PROFILE_SOLVER - solver for profile grouping                    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

struct khe_profile_solver_rec {

  /* free lists */
  ARRAY_KHE_PROFILE_DOMAIN		profile_domain_free_list;
  ARRAY_KHE_PROFILE_TIME_GROUP		profile_time_group_free_list;
  ARRAY_KHE_PROFILE_NODE		profile_node_free_list;

  /* fields in general use */
  HA_ARENA				arena;
  KHE_COMB_GROUPER			comb_grouper;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  KHE_SOLN_ADJUSTER			fix_leaders_sa;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR monitors;
  int					groups_count;

  /* fields used by each monitor in turn */
  int					min_limit;
  int					max_limit;
  bool					history_before;
  bool					history_after;
  ARRAY_KHE_PROFILE_TIME_GROUP		profile_time_groups;
  ARRAY_KHE_PROFILE_TIME_GROUP		profile_time_groups_by_time;
  ARRAY_KHE_PROFILE_NODE		dynamic_nodes;
  int					dynamic_first_index;
  int					dynamic_last_index;
  ARRAY_KHE_DEBUG_COLUMN		debug_columns;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PROFILE_DOMAIN"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_DOMAIN KheProfileDomainMake(KHE_MTASK mt,                    */
/*    KHE_PROFILE_SOLVER ps)                                                 */
/*                                                                           */
/*  Make a new profile domain object containing just mtask.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_PROFILE_DOMAIN KheProfileDomainMake(KHE_MTASK mt,
  KHE_PROFILE_SOLVER ps)
{
  /* get the basic object */
  KHE_PROFILE_DOMAIN res;
  if( HaArrayCount(ps->profile_domain_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ps->profile_domain_free_list);
    HaArrayClear(res->mtasks);
  }
  else
  {
    HaMake(res, ps->arena);
    HaArrayInit(res->mtasks, ps->arena);
  }

  /* initialize its fields and return it */
  res->domain = KheMTaskDomain(mt);
  HaArrayAddLast(res->mtasks, mt);
  res->cover = KheMTaskNeedsAssignmentTaskCount(mt);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileDomainDelete(KHE_PROFILE_DOMAIN pd, KHE_PROFILE_SOLVER ps)*/
/*                                                                           */
/*  Delete pd.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** unused because we delete a whole array of profile domains at once
static void KheProfileDomainDelete(KHE_PROFILE_DOMAIN pd, KHE_PROFILE_SOLVER ps)
{
  HaArrayAddLast(ps->profile_domain_free_list, pd);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileDomainAcceptsMTask(KHE_PROFILE_DOMAIN pd, KHE_MTASK mt)   */
/*                                                                           */
/*  If pd accepts mt, add mt to pd and return true, else return false.       */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileDomainAcceptsMTask(KHE_PROFILE_DOMAIN pd, KHE_MTASK mt)
{
  if( KheResourceGroupEqual(pd->domain, KheMTaskDomain(mt)) )
  {
    HaArrayAddLast(pd->mtasks, mt);
    pd->cover += KheMTaskNeedsAssignmentTaskCount(mt);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileDomainTypedCmp(KHE_PROFILE_DOMAIN pd1,                     */
/*    KHE_PROFILE_DOMAIN pd2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of profile domains        */
/*  by increasing domain size.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheProfileDomainTypedCmp(KHE_PROFILE_DOMAIN pd1,
  KHE_PROFILE_DOMAIN pd2)
{
  return KheResourceGroupResourceCount(pd1->domain) -
    KheResourceGroupResourceCount(pd2->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileDomainCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Untyped comparison function for sorting an array of profile domains      */
/*  by increasing domain size.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheProfileDomainCmp(const void *t1, const void *t2)
{
  KHE_PROFILE_DOMAIN pd1 = * (KHE_PROFILE_DOMAIN *) t1;
  KHE_PROFILE_DOMAIN pd2 = * (KHE_PROFILE_DOMAIN *) t2;
  return KheProfileDomainTypedCmp(pd1, pd2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PROFILE_TIME_GROUP"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupAddMTask(KHE_PROFILE_TIME_GROUP ptg,             */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Add mt to ptg.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupAddMTask(KHE_PROFILE_TIME_GROUP ptg,
  KHE_MTASK mt)
{
  KHE_PROFILE_DOMAIN pd;  int i;

  /* update the cover */
  ptg->priv_cover += KheMTaskNeedsAssignmentTaskCount(mt);

  /* add mt to an existing profile domain if there is one */
  HaArrayForEach(ptg->priv_profile_domains, pd, i)
    if( KheProfileDomainAcceptsMTask(pd, mt) )
      return;

  /* make a new profile domain and add mt to that */
  HaArrayAddLast(ptg->priv_profile_domains,
    KheProfileDomainMake(mt, ptg->profile_solver));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupSetCover(KHE_PROFILE_TIME_GROUP ptg, int max_len)*/
/*                                                                           */
/*  Set the cover, domains, and domain_covers fields of ptg.  The maximum    */
/*  task length is max_len.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupSetCover(KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_MTASK_FINDER mtf;  int i;  KHE_MTASK_SET mts;
  KHE_RESOURCE_TYPE rt;  KHE_MTASK mt;
  KHE_PROFILE_SOLVER ps;

  /* clear out old values, ready for a fresh start */
  ps = ptg->profile_solver;
  ptg->priv_cover = 0;
  HaArrayAppend(ps->profile_domain_free_list, ptg->priv_profile_domains, i);
  HaArrayClear(ptg->priv_profile_domains);

  /* get mts, the mtasks that cover ptg->time_group */
  mtf = ps->mtask_finder;
  rt = ps->resource_type;
  mts = KheMTaskFinderMTasksInTimeGroup(mtf, rt, ptg->time_group);

  /* incorporate the admissible mtasks of mts into ptg */
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskAssignIsFixed(mt) &&
	KheMTaskTotalDuration(mt) <= ps->max_limit &&
	KheMTaskNeedsAssignment(mt) )
      KheProfileTimeGroupAddMTask(ptg, mt);
  }

  /* sort the profile domains so that the smallest domains come first */
  HaArraySort(ptg->priv_profile_domains, &KheProfileDomainCmp);

  /* and now we are up to date */
  ptg->priv_up_to_date = true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileTimeGroupCover(KHE_PROFILE_TIME_GROUP ptg)                 */
/*                                                                           */
/*  Return the cover of ptg, first making sure that ptg is up to date.       */
/*                                                                           */
/*****************************************************************************/

static int KheProfileTimeGroupCover(KHE_PROFILE_TIME_GROUP ptg)
{
  if( !ptg->priv_up_to_date )
    KheProfileTimeGroupSetCover(ptg);
  return ptg->priv_cover;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileTimeGroupProfileDomainCount(KHE_PROFILE_TIME_GROUP ptg)    */
/*                                                                           */
/*  Return the number of profile domains in ptg, first making sure that      */
/*  ptg is up to date.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static int KheProfileTimeGroupProfileDomainCount(KHE_PROFILE_TIME_GROUP ptg)
{
  if( !ptg->priv_up_to_date )
    KheProfileTimeGroupSetCover(ptg);
  return HaArrayCount(ptg->priv_profile_domains);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_DOMAIN KheProfileTimeGroupProfileDomain(                     */
/*    KHE_PROFILE_TIME_GROUP ptg, int i)                                     */
/*                                                                           */
/*  Return the i'th profile domain of ptg, first making sure that ptg is     */
/*  up to date.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static KHE_PROFILE_DOMAIN KheProfileTimeGroupProfileDomain(
  KHE_PROFILE_TIME_GROUP ptg, int i)
{
  if( !ptg->priv_up_to_date )
    KheProfileTimeGroupSetCover(ptg);
  return HaArray(ptg->priv_profile_domains, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,       */
/*    KHE_RESOURCE_GROUP domain, int *pos)                                   */
/*                                                                           */
/*  If ptg contains domain, return true with *pos set to its position.       */
/*  Otherwise return false.  But first make sure that ptg is up to date.     */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,
  KHE_RESOURCE_GROUP domain, int *pos)
{
  int i;  KHE_PROFILE_DOMAIN pd;
  if( !ptg->priv_up_to_date )
    KheProfileTimeGroupSetCover(ptg);
  HaArrayForEach(ptg->priv_profile_domains, pd, i)
    if( KheResourceGroupEqual(domain, pd->domain) )
      return *pos = i, true;
  return *pos = -1, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_PROFILE_SOLVER ps,    */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Make a new profile time group object for time group tg.                  */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_PROFILE_SOLVER ps,
  KHE_TIME_GROUP tg)
{
  KHE_PROFILE_TIME_GROUP res;

  ** get the object from the free list or make it fresh **
  if( HaArrayCount(ps->profile_time_group_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ps->profile_time_group_free_list);
    HaArrayClear(res->priv_profile_domains);
  }
  else
  {
    HaMake(res, ps->arena);
    HaArrayInit(res->priv_profile_domains, ps->arena);
  }
  res->profile_solver = ps;
  res->time_group = tg;
  res->groups_made_here = 0;
  res->priv_up_to_date = false;
  res->priv_cover = 0;

  ** set the cover and return **
  ** KheProfileTimeGroupSetCover(res, ps->max_limit); **
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)               */
/*                                                                           */
/*  Delete ptg.  This assumes that it is deleted from the list separately.   */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_PROFILE_SOLVER ps;  int i;
  ps = ptg->profile_solver;
  HaArrayAppend(ps->profile_domain_free_list, ptg->priv_profile_domains, i);
  HaArrayAddLast(ps->profile_time_group_free_list, ptg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupMarkOutOfDate(KHE_PROFILE_TIME_GROUP ptg)        */
/*                                                                           */
/*  Mark ptg out of date.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileTimeGroupMarkOutOfDate(KHE_PROFILE_TIME_GROUP ptg)
{
  ptg->priv_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug print of groups"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DEBUG_COLUMN KheDebugColumnMake(KHE_TASK task, KHE_TASK leader_task, */
/*    KHE_DEBUG_COLUMN_STATE state, KHE_PROFILE_SOLVER ps)                   */
/*                                                                           */
/*  Make and return a new debug column object with these attributes.         */
/*                                                                           */
/*****************************************************************************/

static KHE_DEBUG_COLUMN KheDebugColumnMake(KHE_TASK task, KHE_TASK leader_task,
  KHE_DEBUG_COLUMN_STATE state, KHE_PROFILE_SOLVER ps)
{
  KHE_DEBUG_COLUMN res;
  HaMake(res, ps->arena);
  res->state = state;
  res->leader_task = leader_task;
  res->task = task;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDebugColumnClear(KHE_DEBUG_COLUMN dc)                            */
/*                                                                           */
/*  Clear dc back to unused.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheDebugColumnClear(KHE_DEBUG_COLUMN dc)
{
  dc->state = KHE_DEBUG_COLUMN_UNUSED;
  dc->task = NULL;
  dc->leader_task = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileSolverContainsDebugColumn(KHE_PROFILE_SOLVER ps,          */
/*    KHE_TASK leader_task, int *pos)                                        */
/*                                                                           */
/*  If ps->debug_columns contains an entry for leader_task, set *pos to      */
/*  its position and return true.  Else set *pos to -1 and return false.     */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileSolverContainsDebugColumn(KHE_PROFILE_SOLVER ps,
  KHE_TASK leader_task, int *pos)
{
  int i;  KHE_DEBUG_COLUMN dc;
  HaArrayForEach(ps->debug_columns, dc, i)
    if( dc->leader_task == leader_task )
      return *pos = i, true;
  return *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DEBUG_COLUMN KheProfileSolverUnusedDebugColumn(KHE_PROFILE_SOLVER ps)*/
/*                                                                           */
/*  Find or make an unused column and return it.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_DEBUG_COLUMN KheProfileSolverUnusedDebugColumn(KHE_PROFILE_SOLVER ps)
{
  int i;  KHE_DEBUG_COLUMN dc;
  HaArrayForEach(ps->debug_columns, dc, i)
    if( dc->state == KHE_DEBUG_COLUMN_UNUSED )
      return dc;
  dc = KheDebugColumnMake(NULL, NULL, KHE_DEBUG_COLUMN_UNUSED, ps);
  HaArrayAddLast(ps->debug_columns, dc);
  return dc;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverSetUsedToDontKnow(KHE_PROFILE_SOLVER ps)            */
/*                                                                           */
/*  Set the state of any starting or continuing column to don't know yet.    */
/*                                                                           */
/*****************************************************************************/

static void KheProfileSolverSetUsedToDontKnow(KHE_PROFILE_SOLVER ps)
{
  int i;  KHE_DEBUG_COLUMN dc;
  HaArrayForEach(ps->debug_columns, dc, i)
    if( dc->state == KHE_DEBUG_COLUMN_STARTING ||
	dc->state == KHE_DEBUG_COLUMN_CONTINUING )
      dc->state = KHE_DEBUG_COLUMN_DONT_KNOW_YET;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverSetDontKnowToUnused(KHE_PROFILE_SOLVER ps)          */
/*                                                                           */
/*  Set the don't know columns to unused.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheProfileSolverSetDontKnowToUnused(KHE_PROFILE_SOLVER ps)
{
  int i;  KHE_DEBUG_COLUMN dc;
  HaArrayForEach(ps->debug_columns, dc, i)
    if( dc->state == KHE_DEBUG_COLUMN_DONT_KNOW_YET )
      dc->state = KHE_DEBUG_COLUMN_UNUSED;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Return the descendant of task running during tg, or NULL if none.        */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TASK res, child_task;  KHE_TIME time;  int i, pos;

  /* do it for task */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    time = KheMeetAsstTime(meet);
    if( time != NULL && KheTimeGroupContains(tg, time, &pos) )
      return task;
  }

  /* do it for task's proper descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    res = KheTaskDuringTimeGroup(child_task, tg);
    if( res != NULL )
      return res;
  }

  /* no luck, return NULL */
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  char KheProfileSolverDurnChar(KHE_PROFILE_SOLVER ps,                     */
/*    KHE_DEBUG_COLUMN dc)                                                   */
/*                                                                           */
/*  Return a single character indicating how dc compares with the            */
/*  expected duration.                                                       */
/*                                                                           */
/*****************************************************************************/

static char KheProfileSolverDurnChar(KHE_PROFILE_SOLVER ps,
  KHE_DEBUG_COLUMN dc)
{
  int durn;
  durn = KheTaskTotalDuration(dc->leader_task);
  return durn < ps->min_limit ? '#' : durn > ps->max_limit ? '$' : ' ';
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverDebugGroups(KHE_PROFILE_SOLVER ps,                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the groups made for laim.                                 */
/*                                                                           */
/*****************************************************************************/
#define KHE_COL_WIDTH 14

static void KheProfileSolverDebugGroups(KHE_PROFILE_SOLVER ps,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int verbosity, int indent, FILE *fp)
{
  int i, j, k, pos, used_count, max_used_count;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  KHE_MTASK_SET mts;  KHE_MTASK mt;  KHE_TASK task;  char *id, *id2;
  KHE_DEBUG_COLUMN dc, dc1, dc2;  KHE_COST non_asst_cost, asst_cost;
  char spaces[KHE_COL_WIDTH + 1], dashes[KHE_COL_WIDTH + 1];
  char buff[KHE_COL_WIDTH + 1];
  fprintf(fp, "%*s[ Task groups for monitor %s\n", indent, "",
    KheMonitorId((KHE_MONITOR) laim));

  /* make the spaces and dashes */
  for( i = 0;  i < KHE_COL_WIDTH;  i++ )
  {
    spaces[i] = ' ';
    dashes[i] = '-';
  }
  spaces[i] = '\0';
  dashes[i] = '\0';

  /* clear out the debug columns */
  HaArrayForEach(ps->debug_columns, dc, i)
    KheDebugColumnClear(dc);

  /* ordinary print to show what's what */
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
    mts = KheMTaskFinderMTasksInTimeGroup(ps->mtask_finder,
      ps->resource_type, tg);
    fprintf(fp, "%*s  time group %s:\n", indent, "", KheTimeGroupId(tg));
    for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
    {
      mt = KheMTaskSetMTask(mts, j);
      /* ***
      if( !KheMTaskAssignIsFixed(mt) && KheMTaskNeedsAssignment(mt) )
      {
      *** */
	for( k = 0;  k < KheMTaskTaskCount(mt);  k++ )
	{
	  task = KheMTaskTask(mt, k, &non_asst_cost, &asst_cost);
	  if( /* KheTaskAsstResource(task) == NULL && */ non_asst_cost > asst_cost )
	    KheTaskDebug(task, 3, indent + 4, fp);
	}
      /* ***
      }
      *** */
    }
  }

  /* one row for each time group of laim */
  max_used_count = 0;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);

    /* mark any used columns as don't know */
    KheProfileSolverSetUsedToDontKnow(ps);

    /* work out which tasks we want to print, and update ps->debug_columns */
    mts = KheMTaskFinderMTasksInTimeGroup(ps->mtask_finder,
      ps->resource_type, tg);
    for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
    {
      mt = KheMTaskSetMTask(mts, j);
      /* ***
      if( !KheMTaskAssignIsFixed(mt) && KheMTaskNeedsAssignment(mt) )
      {
      *** */
	for( k = 0;  k < KheMTaskTaskCount(mt);  k++ )
	{
	  task = KheMTaskTask(mt, k, &non_asst_cost, &asst_cost);
	  if( /*KheTaskAsstResource(task)==NULL &&*/ non_asst_cost > asst_cost )
	  {
	    if( KheProfileSolverContainsDebugColumn(ps, task, &pos) )
	    {
	      dc = HaArray(ps->debug_columns, pos);
	      dc->task = KheTaskDuringTimeGroup(task, tg);
	      /* dc->non_asst_cost = non_asst_cost; */
	      /* dc->asst_cost = asst_cost; */
	      dc->state = KHE_DEBUG_COLUMN_CONTINUING;
	    }
	    else
	    {
	      dc = KheProfileSolverUnusedDebugColumn(ps);
	      dc->leader_task = task;
	      dc->task = KheTaskDuringTimeGroup(task, tg);
	      /* dc->non_asst_cost = non_asst_cost; */
	      /* dc->asst_cost = asst_cost; */
	      dc->state = KHE_DEBUG_COLUMN_STARTING;
	    }
	  }
	}
      /* ***
      }
      *** */
    }

    /* at this point, don't know positions are unused positions */
    KheProfileSolverSetDontKnowToUnused(ps);

    /* swap unused columns with starting columns, if possible */
    for( j = 0;  j < HaArrayCount(ps->debug_columns);  j++ )
    {
      dc1 = HaArray(ps->debug_columns, j);
      if( dc1->state == KHE_DEBUG_COLUMN_UNUSED )
      {
	for( k = j + 1;  k < HaArrayCount(ps->debug_columns);  k++ )
	{
	  dc2 = HaArray(ps->debug_columns, k);
	  if( dc2->state == KHE_DEBUG_COLUMN_STARTING )
	  {
	    HaArraySwap(ps->debug_columns, j, k, dc);
	    break;
	  }
	}
      }
    }

    /* find used_count, the number of used columns */
    HaArrayForEachReverse(ps->debug_columns, dc, j)
      if( dc->state != KHE_DEBUG_COLUMN_UNUSED )
	break;
    used_count = j + 1;
    if( used_count > max_used_count )
      max_used_count = used_count;

    /* print the separator line for the row */
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    for( j = 0;  j < max_used_count;  j++ )
    {
      dc = HaArray(ps->debug_columns, j);
      fprintf(fp, "+%s", dc->state == KHE_DEBUG_COLUMN_CONTINUING ?
	spaces : dashes);
    }
    fprintf(fp, "+\n");

    /* print the row proper */
    fprintf(stderr, "%*s%-*s", indent + 2, "", KHE_COL_WIDTH,
      KheTimeGroupId(tg));
    for( j = 0;  j < max_used_count;  j++ )
    {
      dc = HaArray(ps->debug_columns, j);
      if( (dc->state == KHE_DEBUG_COLUMN_STARTING ||
          dc->state == KHE_DEBUG_COLUMN_CONTINUING) && dc->task != NULL )
      {
	id = KheTaskId(dc->task);
	id2 = strstr(id, ":");
	id = (id2 != NULL ? id2 + 1 : id);
	fprintf(fp, "|%*s%c", KHE_COL_WIDTH - 1, id,
          KheProfileSolverDurnChar(ps, dc));
      }
      else
	fprintf(fp, "|%*s", KHE_COL_WIDTH, "");
    }
    fprintf(fp, "|\n");

    /* print the domain and cost row */
    fprintf(stderr, "%*s%-*s", indent + 2, "", KHE_COL_WIDTH, "");
    for( j = 0;  j < max_used_count;  j++ )
    {
      dc = HaArray(ps->debug_columns, j);
      if( (dc->state == KHE_DEBUG_COLUMN_STARTING ||
          dc->state == KHE_DEBUG_COLUMN_CONTINUING) && dc->task != NULL )
      {
	if( KheTaskAsstResource(dc->task) != NULL )
	{
	  /* print the assigned resource */
	  snprintf(buff, KHE_COL_WIDTH, "%s%c",
	    KheResourceId(KheTaskAsstResource(dc->task)),
	    KheProfileSolverDurnChar(ps, dc));
	}
	else
	{
	  KheTaskNonAsstAndAsstCost(dc->task, &non_asst_cost, &asst_cost);
	  if( non_asst_cost >= KheCost(1, 0) )
	  {
	    /* print domain and "H" for hard cost */
	    snprintf(buff, KHE_COL_WIDTH, "%s/H%c",
	      KheResourceGroupId(KheTaskDomain(dc->task)),
	      KheProfileSolverDurnChar(ps, dc));
	  }
	  else
	  {
	    /* print domain and soft cost */
	    snprintf(buff, KHE_COL_WIDTH, "%s/%d%c",
	      KheResourceGroupId(KheTaskDomain(dc->task)),
	      KheSoftCost(non_asst_cost),
	      KheProfileSolverDurnChar(ps, dc));
	  }
	}
      }
      else
	snprintf(buff, KHE_COL_WIDTH, "%s", "");
      fprintf(fp, "|%*s", KHE_COL_WIDTH, buff);
    }
    fprintf(fp, "|\n");
  }

  /* print the final separator line after the last row */
  fprintf(fp, "%*s%s", indent + 2, "", spaces);
  for( j = 0;  j < max_used_count;  j++ )
    fprintf(fp, "+%s", dashes);
  fprintf(fp, "+\n");

  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PROFILE_SOLVER"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_SOLVER KheProfileSolverMake(KHE_COMB_GROUPER cg,             */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make a new profile solver object.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_PROFILE_SOLVER KheProfileSolverMake(KHE_COMB_GROUPER cg,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_MTASK_FINDER mtf;  HA_ARENA a;  KHE_PROFILE_SOLVER res;
  KHE_INSTANCE ins;
  mtf = KheCombGrouperMTaskFinder(cg);
  a = KheMTaskFinderArena(mtf);
  HaMake(res, a);

  /* free lists */
  HaArrayInit(res->profile_domain_free_list, a);
  HaArrayInit(res->profile_time_group_free_list, a);
  HaArrayInit(res->profile_node_free_list, a);;

  /* fields in general use */
  res->arena = a;
  res->comb_grouper = cg;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->resource_type = KheCombGrouperResourceType(cg);
  res->soln_adjuster = sa;
  res->fix_leaders_sa = KheSolnAdjusterMake(res->soln);
  HaArrayInit(res->monitors, a);
  res->groups_count = 0;

  /* fields used by each monitor in turn, so undefined between monitors */
  res->min_limit = res->max_limit = -1;
  res->history_before = res->history_after = -1;
  HaArrayInit(res->profile_time_groups, a);
  HaArrayInit(res->profile_time_groups_by_time, a);
  ins = KheSolnInstance(res->soln);
  HaArrayFill(res->profile_time_groups_by_time,KheInstanceTimeCount(ins),NULL);
  HaArrayInit(res->dynamic_nodes, a);
  res->dynamic_first_index = res->dynamic_last_index = -1;
  HaArrayInit(res->debug_columns, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)    */
/*                                                                           */
/*  Return true if the time groups of m are all singletons.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(m, i, &po);
    if( KheTimeGroupTimeCount(tg) != 1 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorSuitsProfileGrouping(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,*/
/*    KHE_RESOURCE_TYPE rt, bool non_strict)                                 */
/*                                                                           */
/*  Return true if m suits profile grouping:  if it has at least two time    */
/*  groups, its constraint applies to all resources of type rt, and its      */
/*  time groups are all positive.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorSuitsProfileGrouping(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_RESOURCE_TYPE rt /* , bool non_strict */)
{
  int rt_count;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;

  /* m must have at least two time groups */
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(m) < 2 )
    return false;

  /* m must monitor all resources of type rt */
  rt_count = KheResourceTypeResourceCount(rt);
  c = KheLimitActiveIntervalsMonitorConstraint(m);
  if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(c, rt) < rt_count )
    return false;

  /* m's time groups must be all positive */
  if( !KheLimitActiveIntervalsConstraintAllPositive(c) )
    return false;

  /* all good */
  return true;

  /* ***
  if( non_strict )
  {
    ** its limits must be equal, but we can't enforce that here, because **
    ** we have a single monitor, not two with the same time groups **
    return true;
  }
  else
  {
    ** its time groups must be all singletons **
    return KheTimeGroupsAllSingletons(m);
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool IndexIsLegal(KHE_PROFILE_SOLVER ps, int index)                      */
/*                                                                           */
/*  Return true if index is a legal index of a profile time group.           */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static bool IndexIsLegal(KHE_PROFILE_SOLVER ps, int index)
{
  return index >= 0 && index < HaArrayCount(ps->profile_time_groups);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetYesTimeGroups(KHE_PROFILE_SOLVER ps,           */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Set a sequence of Yes time groups for profile grouping.                  */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileGroupingSetYesTimeGroups(KHE_PROFILE_SOLVER ps,
  int first_index, int last_index)
{
  KHE_PROFILE_TIME_GROUP ptg;  int index;
  for( index = first_index;  index <= last_index;  index++ )
    if( IndexIsLegal(ps, index) )
    {
      ptg = HaArray(ps->profile_time_groups, index);
      KheCombGrouperAddTimeGroupRequirement(ps->comb_grouper, ptg->time_group,
	KHE_COMB_COVER_YES);
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetNoTimeGroups(KHE_PROFILE_SOLVER ps,            */
/*    int index1, int index2)                                                */
/*                                                                           */
/*  Set the two No time groups for profile grouping.                         */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileGroupingSetNoTimeGroups(KHE_PROFILE_SOLVER ps,
  int index1, int index2)
{
  KHE_PROFILE_TIME_GROUP ptg;
  if( IndexIsLegal(ps, index1) )
  {
    ptg = HaArray(ps->profile_time_groups, index1);
    KheCombGrouperAddTimeGroupRequirement(ps->comb_grouper, ptg->time_group,
      KHE_COMB_COVER_NO);
  }
  if( IndexIsLegal(ps, index2) )
  {
    ptg = HaArray(ps->profile_time_groups, index2);
    KheCombGrouperAddTimeGroupRequirement(ps->comb_grouper, ptg->time_group,
      KHE_COMB_COVER_NO);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileDebug(KHE_PROFILE_SOLVER ps, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of current profile.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheProfileSolverDebug(KHE_PROFILE_SOLVER ps, int verbosity,
  int indent, FILE *fp)
{
  KHE_PROFILE_TIME_GROUP ptg;  int i /* , val */;
  /* KheMTaskFinderDebug(ps->mtask_finder, verbosity, indent, fp); */
  fprintf(fp, "%*s%s[", indent, "", ps->history_before ? "inf" : "0");
  HaArrayForEach(ps->profile_time_groups, ptg, i)
  {
    if( i > 0 )
      fprintf(fp, ":");
    fprintf(fp, "%d", KheProfileTimeGroupCover(ptg));

  }
  fprintf(fp, "]%s\n", ps->history_after ? "inf" : "0");
  fprintf(fp, "%*s%s<", indent, "", ps->history_before ? "inf" : "0");
  HaArrayForEach(ps->profile_time_groups, ptg, i)
  {
    if( i > 0 )
      fprintf(fp, ":");
    fprintf(fp, "%d", ptg->groups_made_here);

  }
  fprintf(fp, ">%s\n", ps->history_after ? "inf" : "0");
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheProfileTimeGroupId(KHE_PROFILE_SOLVER ps, int index)            */
/*                                                                           */
/*  Return the name of the profile time group at index, or "-" if none.      */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static char *KheProfileTimeGroupId(KHE_PROFILE_SOLVER ps, int index)
{
  KHE_PROFILE_TIME_GROUP ptg;
  if( IndexIsLegal(ps, index) )
  {
    ptg = HaArray(ps->profile_time_groups, index);
    return KheTimeGroupId(ptg->time_group);
  }
  else
    return "-";
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileSolverMakeGroups(KHE_PROFILE_SOLVER ps,                    */
/*    KHE_MTASK_GROUP mg, int max_num, bool fix_leaders, char *debug_str)    */
/*                                                                           */
/*  Make groups according to these parameters and mark the affected          */
/*  profile time groups out of date.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static int KheProfileSolverMakeGroups(KHE_PROFILE_SOLVER ps,
  KHE_MTASK_GROUP mg, int max_num, bool fix_leaders, char *debug_str)
{
  int i, j;  KHE_TIME_SET ts;  KHE_MTASK mt;  KHE_TIME time;
  KHE_PROFILE_TIME_GROUP ptg;

  ** mark the affected profile time groups out of date **
  for( i = 0;  i < KheMTaskGroupMTaskCount(mg);  i++ )
  {
    mt = KheMTaskGroupMTask(mg, i);
    ts = KheMTaskTimeSet(mt);
    for( j = 0;  j < KheTimeSetTimeCount(ts);  j++ )
    {
      time = KheTimeSetTime(ts, j);
      ptg = HaArray(ps->profile_time_groups_by_time, KheTimeIndex(time));
      if( ptg != NULL )
	KheProfileTimeGroupMarkOutOfDate(ptg);
    }
  }

  ** build the group **
  return KheMTaskGroupExecute(mg, max_num, ps->soln_adjuster,
    fix_leaders ? ps->fix_leaders_sa : NULL, debug_str);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfile(KHE_PROFILE_SOLVER ps, int first, int last,              */
/*    int no1, int no2, KHE_PROFILE_TIME_GROUP ptg,                          */
/*    KHE_PROFILE_TIME_GROUP adj_ptg, int max_num, char *debug_str)          */
/*                                                                           */
/*  Find up to max_num groups for indexes first .. last, taking account      */
/*  of task domains.  Exclude indexes no1 and no2.  Return true if any       */
/*  groups were made.                                                        */
/*                                                                           */
/*  Here ptg is the profile time group at index first (or last) that         */
/*  we use to sort out the domains.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static int min(int a, int b) { return a < b ? a : b; }

static bool KheProfile(KHE_PROFILE_SOLVER ps, int first, int last,
  int no1, int no2, KHE_PROFILE_TIME_GROUP ptg,
  KHE_PROFILE_TIME_GROUP adj_ptg, int max_num, char *debug_str)
{
  int i, adj_cover, groups, singles, cover_diff, pos;  bool res;
  KHE_PROFILE_DOMAIN pd, adj_pd;  KHE_MTASK_GROUP mg;

  if( DEBUG11 )
  {
    KHE_TIME_GROUP ptg_tg, aptg_tg;
    ptg_tg = ptg->time_group;
    aptg_tg = (adj_ptg == NULL ? NULL : adj_ptg->time_group);
    fprintf(stderr, "[ KheProfile(ps, %s .. %s, not %s, not %s, "
      "ptg %s, adj %s, max_num %d, domains %d)\n",
      KheProfileTimeGroupId(ps, first), KheProfileTimeGroupId(ps, last),
      KheProfileTimeGroupId(ps, no1), KheProfileTimeGroupId(ps, no2),
      KheTimeGroupId(ptg_tg), aptg_tg == NULL ? "-" : KheTimeGroupId(aptg_tg),
      max_num, KheProfileTimeGroupProfileDomainCount(ptg));
  }

  ** set up the time groups and other requirements, and count singles **
  KheCombGrouperClearRequirements(ps->comb_grouper);
  KheProfileGroupingSetYesTimeGroups(ps, first, last);
  KheProfileGroupingSetNoTimeGroups(ps, no1, no2);
  singles = KheCombGrouperSolveSingles(ps->comb_grouper);
  KheCombGrouperAddNoSinglesRequirement(ps->comb_grouper);
  max_num -= singles;

  ** find up to max_num groups, trying each domain of ptg in turn **
  res = false;
  mg = KheMTaskGroupMake(ps->comb_grouper);
  for(i = 0; i < KheProfileTimeGroupProfileDomainCount(ptg) && max_num > 0; i++)
  {
    ** find one domain, its cover, and its cover in adj_ptg **
    pd = KheProfileTimeGroupProfileDomain(ptg, i);
    if( adj_ptg != NULL &&
	KheProfileTimeGroupContainsDomain(adj_ptg, pd->domain, &pos) )
    {
      adj_pd = KheProfileTimeGroupProfileDomain(adj_ptg, pos);
      adj_cover = adj_pd->cover;
    }
    else
      adj_cover = 0;
    cover_diff = pd->cover - adj_cover;
    if( DEBUG11 )
    {
      fprintf(stderr, "  domain ");
      KheResourceGroupDebug(pd->domain, 1, -1, stderr);
      fprintf(stderr, ", cover %d, adj_cover %d, cover_diff %d\n",
	pd->cover, adj_cover, cover_diff);
    }

    ** do some grouping if the domain has less cover in adj_ptg **
    if( cover_diff > 0 )
    {
      ** do the grouping and adjust max_num and groups **
      KheCombGrouperAddPreferredDomainRequirement(ps->comb_grouper, pd->domain);
      if( KheCombGrouperSolve(ps->comb_grouper, KHE_COMB_VARIANT_ZERO, mg) )
        groups = KheProfileSolverMakeGroups(ps, mg, min(cover_diff, max_num),
	  false, debug_str);
      else
	groups = 0;
      KheCombGrouperDeletePreferredDomainRequirement(ps->comb_grouper);
      if( groups > 0 )
      {
	** update various records of what groups were made **
	ptg = HaArray(ps->profile_time_groups, first);
	ptg->groups_made_here += groups;
	ps->groups_count += groups;
	res = true;

	** reduce the number of groups remaining to be found **
	max_num -= groups;
	if( DEBUG3 )
	  fprintf(stderr, "  %s made %d groups for %s", debug_str, groups,
	    KheResourceGroupId(pd->domain));

	** start again, since ptg->domains may have changed **
	i = -1;
      }
    }
  }
  KheMTaskGroupDelete(mg);

  if( DEBUG11 )
    fprintf(stderr, "] KheProfile returning %s (groups_count %d)\n",
      bool_show(res), ps->groups_count);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps,                 */
/*    bool *progressing)                                                     */
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are unequal.       */
/*  Make as many groups as possible.                                         */
/*                                                                           */
/*  If any grouping actually occurred, set *progressing to true.  Otherwise  */
/*  leave *progressing unchanged.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps,
  bool *progressing)
{
  int tg_count, i, curr_cover, prev_cover, next_cover;  bool progress;
  KHE_PROFILE_TIME_GROUP curr_ptg, prev_ptg, next_ptg;

  ** boilerplate **
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheProfileTryUnequalGrouping\n");
    KheProfileSolverDebug(ps, 1, 2, stderr);
  }
  tg_count = HaArrayCount(ps->profile_time_groups);

  do
  {
    progress = false;

    ** forward pass to handle places where sequences start **
    prev_ptg = NULL;
    prev_cover = ** ps->history_before ? INT_MAX : 0 ** 0;
    for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      curr_cover = KheProfileTimeGroupCover(curr_ptg);
      if( curr_cover > prev_cover )
      {
	** curr_cover - prev_cover sequences must start at i **
	if( DEBUG5 )
	  fprintf(stderr, "  start at %d (curr %d > prev %d)\n", i,
            curr_cover, prev_cover);
	if( KheProfile(ps, i, i + ps->min_limit - 1, i - 1,
	      i + ps->max_limit, curr_ptg, prev_ptg,
	      curr_cover - prev_cover, "forwards profile grouping") )
	  *progressing = progress = true;
      }

      ** reset prev_cover for next iteration **
      prev_ptg = curr_ptg;
      prev_cover = curr_cover;
    }

    ** backward pass to handle places where sequences end **
    next_ptg = NULL;
    next_cover = ps->history_after ? INT_MAX : 0;
    for( i = tg_count - 1;  i >= ps->min_limit - 1;  i-- )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      curr_cover = KheProfileTimeGroupCover(curr_ptg);
      if( curr_cover > next_cover )
      {
	** curr_cover - next_cover sequences must end at i **
	if( DEBUG5 )
	  fprintf(stderr, "  end at %d (curr %d > next %d)\n", i,
	    curr_cover, next_cover);
	if( KheProfile(ps, i - ps->min_limit + 1, i, i - ps->max_limit,
	      i + 1, curr_ptg, next_ptg, curr_cover - next_cover,
	      "backward profile grouping") )
	  *progressing = progress = true;
      }

      ** reset next_cover for next iteration **
      next_ptg = curr_ptg;
      next_cover = curr_cover;
    }
  } while( progress );

  ** all done **
  if( DEBUG5 )
    fprintf(stderr, "] KheProfileTryUnequalGrouping returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps, bool *progressing)*/
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are equal, and     */
/*  return true if any groups were made.  Make at most one group.            */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps, bool *progressing)
{
  int val, i, min_groups, tg_count, count, curr_cover;
  KHE_PROFILE_TIME_GROUP curr_ptg;

  ** find min_groups, the minimum number of groupings **
  min_groups = INT_MAX;
  tg_count = HaArrayCount(ps->profile_time_groups);
  for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
  {
    curr_ptg = HaArray(ps->profile_time_groups, i);
    curr_cover = KheProfileTimeGroupCover(curr_ptg);
    if( curr_cover > 0 && curr_ptg->groups_made_here < min_groups )
      min_groups = val;
  }

  if( min_groups < INT_MAX )
  {
    ** for each pos with a minimum number of groups, try to group **
    count = tg_count - ps->min_limit + 1;
    for( i = 0;  i < count;  i++ )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      curr_cover = KheProfileTimeGroupCover(curr_ptg);
      if( curr_cover > 0 && curr_ptg->groups_made_here == min_groups &&
	  KheProfile(ps, i, i + ps->min_limit - 1, i - 1, i + ps->max_limit,
	    curr_ptg, NULL, 1, "non-strict profile grouping") )
      {
	*progressing = true;
	return;
      }
    }

    ** for each pos with just above a min number of groups, try to group **
    for( i = 0;  i < count;  i++ )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      curr_cover = KheProfileTimeGroupCover(curr_ptg);
      if( curr_cover > 0 && curr_ptg->groups_made_here == min_groups + 1 &&
	  KheProfile(ps, i, i + ps->min_limit - 1, i - 1, i + ps->max_limit,
	    curr_ptg, NULL, 1, "non-strict profile grouping") )
      {
	*progressing = true;
	return;
      }
    }
  }

  ** no luck **
  return;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_NODE KheProfileNodeMake(KHE_MTASK_GROUP mg,                  */
/*    KHE_COST total_cost, KHE_PROFILE_NODE prev_node, KHE_PROFILE_SOLVER ps)*/
/*                                                                           */
/*  Make and return a new profile node with these attributes.                */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static KHE_PROFILE_NODE KheProfileNodeMake(KHE_MTASK_GROUP mg,
  KHE_COST total_cost, KHE_PROFILE_NODE prev_node, KHE_PROFILE_SOLVER ps)
{
  KHE_PROFILE_NODE res;
  if( HaArrayCount(ps->profile_node_free_list) > 0 )
    res = HaArrayLastAndDelete(ps->profile_node_free_list);
  else
    HaMake(res, ps->arena);
  res->mg = mg;
  res->total_cost = total_cost;
  res->prev_node = prev_node;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileNodeDelete(KHE_PROFILE_NODE pn, KHE_PROFILE_SOLVER ps)    */
/*                                                                           */
/*  Free pn.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileNodeDelete(KHE_PROFILE_NODE pn, KHE_PROFILE_SOLVER ps)
{
  HaArrayAddLast(ps->profile_node_free_list, pn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindOldBest(KHE_PROFILE_SOLVER ps, int pos,                      */
/*    KHE_PROFILE_NODE *node, KHE_COST *total_cost)                          */
/*                                                                           */
/*  If there is a solution at position pos, set *node to that solution       */
/*  (possibly to NULL) and *total_cost to its cost and return true.          */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static bool KheFindOldBest(KHE_PROFILE_SOLVER ps, int pos,
  KHE_PROFILE_NODE *node, KHE_COST *total_cost)
{
  *node = NULL;
  pos -= ps->dynamic_first_index;
  if( pos < -1 )
  {
    ** no solutions way back here **
    return *total_cost = KheCost(INT_MAX, INT_MAX), false;
  }
  else if( pos == -1 )
  {
    ** this is what the doc calls S(0), the empty set of groupings **
    return *total_cost = KheCost(0, 0), true;
  }
  else
  {
    *node = HaArray(ps->dynamic_nodes, pos);
    if( *node == NULL )
    {
      ** in range but no group there **
      return *total_cost = KheCost(INT_MAX, INT_MAX), false;
    }
    else
    {
      ** in range and group is present **
      return *total_cost = (*node)->total_cost, true;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindNewBest(KHE_PROFILE_SOLVER ps, int first_pos, int last_pos,  */
/*    KHE_MTASK_GROUP mg, KHE_COST *mg_cost)                                 */
/*                                                                           */
/*  Find an mtask grouping spanning time groups first_pos ... last_pos.      */
/*  Given the context where this is called, these are real indexes into      */
/*  ps->profile_time_groups.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static bool KheFindNewBest(KHE_PROFILE_SOLVER ps, int first_pos, int last_pos,
  KHE_MTASK_GROUP mg, KHE_COST *mg_cost)
{
  KheCombGrouperClearRequirements(ps->comb_grouper);
  KheProfileGroupingSetYesTimeGroups(ps, first_pos, last_pos);
  KheProfileGroupingSetNoTimeGroups(ps, first_pos - 1, last_pos + 1);
  return KheCombGrouperSolve(ps->comb_grouper, KHE_COMB_VARIANT_MIN, mg)
      && KheMTaskGroupHasCost(mg, mg_cost);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTryOneDynamic(KHE_PROFILE_SOLVER ps, bool *progressing)   */
/*                                                                           */
/*  Try one round of grouping using dynamic programming.                     */
/*                                                                           */
/*  If any grouping actually occurred, set *progressing to true.  Otherwise  */
/*  leave *progressing unchanged.                                            */
/*                                                                           */
/*  Implementation note.  Node i in dynamic_nodes holds what the             */
/*  documentation calls S(i+1), the best set of groupings for the first      */
/*  i + 1 time groups.  So node 0 holds S(1) and S(0) is not represented     */
/*  explicitly as a node.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileTryOneDynamic(KHE_PROFILE_SOLVER ps, bool *progressing)
{
  KHE_MTASK_GROUP mg, best_mg, save_mg;
  int i, len, new_first_pos, new_last_pos;
  KHE_PROFILE_NODE prev_node, best_prev_node, n;
  KHE_COST prev_total_cost, total_cost, best_total_cost, mg_cost;

  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheProfileTryOneDynamic(ps, %d-%d, %s)\n",
      ps->dynamic_first_index, ps->dynamic_last_index, bool_show(*progressing));
    KheProfileSolverDebug(ps, 1, 2, stderr);
  }
  ** there should initially be no dynamic_nodes **
  HnAssert(HaArrayCount(ps->dynamic_nodes) == 0, "KheProfileTryDynamic");

  ** quit early if there is a profile time group with zero cover **
  ** ***
  HaArrayForEach(ps->profile_time_groups, ptg, i)
    if( KheProfileTimeGroupCover(ptg) == 0 )
      return;
  *** **

  ** make scratch mg and best_mg variables **
  mg = KheMTaskGroupMake(ps->comb_grouper);
  best_mg = KheMTaskGroupMake(ps->comb_grouper);

  ** find the best group ending at each position **
  for( i = ps->dynamic_first_index;  i <= ps->dynamic_last_index;  i++ )
  {
    best_total_cost = KheCost(INT_MAX, INT_MAX);
    best_prev_node = NULL;
    for( len = ps->min_limit;  len <= ps->max_limit;  len++ )
    {
      new_first_pos = i - len + 1;
      new_last_pos = i;
      if( KheFindOldBest(ps, new_first_pos - 1, &prev_node, &prev_total_cost)
	  && KheFindNewBest(ps, new_first_pos, new_last_pos, mg, &mg_cost) )
      {
	total_cost = prev_total_cost + mg_cost;
	if( total_cost < best_total_cost )
	{
	  KheMTaskGroupOverwrite(best_mg, mg);
	  best_total_cost = total_cost;
	  best_prev_node = prev_node;
	}
      }
    }
    if( best_total_cost < KheCost(INT_MAX, INT_MAX) )
    {
      save_mg = KheMTaskGroupMake(ps->comb_grouper);
      KheMTaskGroupOverwrite(save_mg, best_mg);
      n = KheProfileNodeMake(save_mg, best_total_cost, best_prev_node, ps);
    }
    else
      n = NULL;
    HaArrayAddLast(ps->dynamic_nodes, n);
    if( DEBUG4 )
    {
      KHE_PROFILE_TIME_GROUP ptg;
      ptg = HaArray(ps->profile_time_groups, i);
      fprintf(stderr, "  at %s, ", KheTimeGroupId(ptg->time_group));
      if( n == NULL )
	fprintf(stderr, "no new node\n");
      else
	fprintf(stderr, "new node with cost %.5f\n",
	  KheCostShow(best_total_cost));
    }
  }

  ** execute the groups of any solution and fix them off the profile **
  if( HaArrayLast(ps->dynamic_nodes) != NULL )
  {
    for( n = HaArrayLast(ps->dynamic_nodes);  n != NULL;  n = n->prev_node )
      KheProfileSolverMakeGroups(ps, n->mg, 1, true,"dynamic profile grouping");
    *progressing = true;
  }

  ** finished with scratch mg, best_mg, and the profile nodes **
  KheMTaskGroupDelete(mg);
  KheMTaskGroupDelete(best_mg);
  HaArrayForEach(ps->dynamic_nodes, n, i)
    if( n != NULL )
      KheProfileNodeDelete(n, ps);
  HaArrayClear(ps->dynamic_nodes);
  if( DEBUG4 )
  {
    fprintf(stderr, "  KheProfileTryOneDynamic at end:\n");
    KheProfileSolverDebug(ps, 1, 2, stderr);
    fprintf(stderr, "] KheProfileTryOneDynamic\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTryDynamic(KHE_PROFILE_SOLVER ps, bool *progressing)      */
/*                                                                           */
/*  Try multiple rounds of grouping by dynamic programming.                  */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
typedef enum {
  PROFILE_ZERO,
  PROFILE_NON_ZERO
} PROFILE_STATE;

static void KheProfileTryDynamic(KHE_PROFILE_SOLVER ps, bool *progressing)
{
  bool progress;  PROFILE_STATE state;  int i;
  KHE_PROFILE_TIME_GROUP ptg;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheProfileTryDynamic(ps, %s)\n",
      bool_show(*progressing));
    KheProfileSolverDebug(ps, 1, 2, stderr);
  }
  do
  {
    progress = false;
    state = PROFILE_ZERO;
    HaArrayForEach(ps->profile_time_groups, ptg, i)
    {
      ** ***
      if( DEBUG4 )
	fprintf(stderr, "%s%d%c", i == 0 ? "  states: " : ",",
	  KheProfileTimeGroupCover(ptg), state == PROFILE_ZERO ? 'Z' : 'N');
      *** **
      switch( state )
      {
	case PROFILE_ZERO:

	  if( KheProfileTimeGroupCover(ptg) != 0 )
	  {
	    ** start a non-zero run here **
	    ps->dynamic_first_index = i;
	    state = PROFILE_NON_ZERO;
	  }
	  break;

	case PROFILE_NON_ZERO:

	  if( KheProfileTimeGroupCover(ptg) == 0 )
	  {
	    ** non-zero run ends here **
	    ps->dynamic_last_index = i - 1;
	    KheProfileTryOneDynamic(ps, &progress);
	    state = PROFILE_ZERO;
	  }
	  break;

	default:

	  HnAbort("KheProfileTryDynamic internal error");
      }
    }
    ** ***
    if( DEBUG4 )
      fprintf(stderr, "::%c\n", state == PROFILE_ZERO ? 'Z' : 'N');
    *** **
    if( state == PROFILE_NON_ZERO )
    {
      ps->dynamic_last_index = i - 1;
      KheProfileTryOneDynamic(ps, &progress);
    }
    if( progress )
      *progressing = true;
  } while( progress );
  if( DEBUG4 )
  {
    fprintf(stderr, "  KheProfileTryDynamic at end:\n");
    KheProfileSolverDebug(ps, 1, 2, stderr);
    fprintf(stderr, "] KheProfileTryDynamic\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverAddProfileTimeGroups(KHE_PROFILE_SOLVER ps,         */
/*    int first_index)                                                       */
/*                                                                           */
/*  Add profile time groups for the monitor at index first_index.            */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileSolverAddProfileTimeGroups(KHE_PROFILE_SOLVER ps,
  int first_index)
{
  int tg_count, i, j;  KHE_POLARITY po;  KHE_TIME_GROUP tg;  KHE_TIME time;
  KHE_PROFILE_TIME_GROUP ptg;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  laim = HaArray(ps->monitors, first_index);
  tg_count = KheLimitActiveIntervalsMonitorTimeGroupCount(laim);
  for( i = 0;  i < tg_count;  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
    ptg = KheProfileTimeGroupMake(ps, tg);
    HaArrayAddLast(ps->profile_time_groups, ptg);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      time = KheTimeGroupTime(tg, j);
      HnAssert(HaArrayCount(ps->profile_time_groups_by_time) > KheTimeIndex(time),
	"KheProfileSolverAddProfileTimeGroups internal error 1");
      HaArrayPut(ps->profile_time_groups_by_time, KheTimeIndex(time), ptg);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverDeleteProfileTimeGroups(KHE_PROFILE_SOLVER ps)      */
/*                                                                           */
/*  Delete all of ps's profile time groups.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** made redundant by the second dynamic programming algorithm
static void KheProfileSolverDeleteProfileTimeGroups(KHE_PROFILE_SOLVER ps)
{
  int i;
  while( HaArrayCount(ps->profile_time_groups) > 0 )
    KheProfileTimeGroupDelete(HaArrayLastAndDelete(ps->profile_time_groups));
  for( i = 0;  i < HaArrayCount(ps->profile_time_groups_by_time);  i++ )
    HaArrayPut(ps->profile_time_groups_by_time, i, NULL);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,                */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Carry out profile grouping for monitors[first_index .. last_index].      */
/*                                                                           */
/*****************************************************************************/

static void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,
  int first_index, int last_index)
{
  int i;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  bool progressing;

  if( DEBUG8 )
  {
    laim = HaArray(ps->monitors, first_index);
    fprintf(stderr, "[ KheProfileGroupingForMonitors(ps, %s, %d-%d)\n",
      KheMonitorId((KHE_MONITOR) laim), first_index, last_index);
  }

  /* get min_limit, max_limit, history_before, and history_after */
  HnAssert(first_index <= last_index,
    "KheProfileGroupingForMonitors internal error");
  ps->min_limit = 1;
  ps->max_limit = INT_MAX;
  ps->history_before = false;
  ps->history_after = false;
  for( i = first_index;  i <= last_index;  i++ )
  {
    laim = HaArray(ps->monitors, i);
    if( KheLimitActiveIntervalsMonitorMinimum(laim) > ps->min_limit )
      ps->min_limit = KheLimitActiveIntervalsMonitorMinimum(laim);
    if( KheLimitActiveIntervalsMonitorMaximum(laim) < ps->max_limit )
      ps->max_limit = KheLimitActiveIntervalsMonitorMaximum(laim);
    if( KheLimitActiveIntervalsMonitorHistoryBefore(laim) > 0 )
      ps->history_before = true;
    if( KheLimitActiveIntervalsMonitorHistoryAfter(laim) > 0 )
      ps->history_after = true;
  }

  /* no use if min_limit is 1 */
  if( ps->min_limit <= 1 )
  {
    if( DEBUG8 )
      fprintf(stderr, "] KheProfileGroupingForMonitors returning early"
	" (ps->min_limit == %d)\n", ps->min_limit);
    return;
  }

  /* do it if all time groups are singletons, or limits are equal */
  laim = HaArray(ps->monitors, first_index);
  if( KheTimeGroupsAllSingletons(laim) && ps->min_limit >= ps->max_limit - 1 )
  {
    /* dynamic programming subsumes everything else - if it works */
    ps->groups_count += KheProfileGroupingByDynamicProgramming(
      ps->mtask_finder, laim, ps->min_limit, ps->max_limit,
      ps->history_before, ps->history_after, ps->soln_adjuster, &progressing);
    if( DEBUG2 )
      KheProfileSolverDebugGroups(ps, laim, 2, 2, stderr);

    /* *** replaced by dynamic programming
    ** add profile time groups for the monitor at first_index **
    if( DEBUG9 )
    {
      fprintf(stderr, "  profile solver at (1):\n");
      KheProfileSolverDebug(ps, 1, 2, stderr);
    }
    KheProfileSolverAddProfileTimeGroups(ps, first_index);

    ** keep grouping while groups are being found **
    do
    {
      if( DEBUG9 )
      {
	fprintf(stderr, "  profile solver at (2):\n");
	KheProfileSolverDebug(ps, 1, 2, stderr);
      }
      progressing = false;
      KheProfileTryUnequalGrouping(ps, &progressing);
      KheProfileTryDynamic(ps, &progressing);
      KheProfileTryEqualGrouping(ps, &progressing);
      ** ***
      ps->groups_count += KheProfileGroupingByDynamicProgramming(
	ps->mtask_finder, laim, ps->min_limit, ps->max_limit,
        ps->history_before, ps->history_after, &progressing);
      *** **
      if( DEBUG9 )
	KheProfileSolverDebug(ps, 1, 2, stderr);
    } while( progressing );

    ** clear this monitor's time groups away **
    KheProfileSolverDeleteProfileTimeGroups(ps);
    if( DEBUG9 )
    {
      fprintf(stderr, "  profile solver at (3):\n");
      KheProfileSolverDebug(ps, 1, 2, stderr);
    }
    if( DEBUG2 )
      KheProfileSolverDebugGroups(ps, laim, 2, 2, stderr);
  *** */
  }

  if( DEBUG8 )
    fprintf(stderr, "] KheProfileGroupingForMonitors returning "
      "(groups_count = %d)\n", ps->groups_count);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveTypedCmp(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)                              */
/*                                                                           */
/*  Typed comparison function for sorting an array of limit active           */
/*  intervals monitors so that monitors with the same time groups            */
/*  are brought together, and monitors whose time groups have fewer          */
/*  times come before monitors whose time groups have more times.  This      */
/*  function assumes (as it may) that all time groups are positive.          */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveTypedCmp(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  int i, cmp, time_count1, time_count2, tg_count1, tg_count2;

  /* if first time groups differ in size, smallest comes first */
  tg_count1 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
  tg_count2 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim2);
  if( tg_count1 > 0 && tg_count2 > 0 )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, 0, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, 0, &po2);
    time_count1 = KheTimeGroupTimeCount(tg1);
    time_count2 = KheTimeGroupTimeCount(tg2);
    if( time_count1 != time_count2 )
      return time_count1 - time_count2;
  }

  /* time groups must be equal, but no need to check polarity */
  if( tg_count1 != tg_count2 )
    return tg_count1 - tg_count2;
  for( i = 0;  i < tg_count1;  i++ )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, i, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, i, &po2);
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 ) return cmp;
  }

  /* everything that matters here is equal */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of limit active         */
/*  intervals monitors so that monitors with the same time groups            */
/*  are brought together.  This function assumes that all time groups        */
/*  are positive.                                                            */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveCmp(const void *t1, const void *t2)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1, laim2;
  laim1 = * (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR *) t1;
  laim2 = * (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR *) t2;
  return KheLimitActiveTypedCmp(laim1, laim2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileGrouping(KHE_COMB_GROUPER cg, KHE_SOLN_ADJUSTER sa)        */
/*                                                                           */
/*  Carry out profile grouping for cg.  Return the number of groups made.    */
/*                                                                           */
/*****************************************************************************/

int KheProfileGrouping(KHE_COMB_GROUPER cg, KHE_SOLN_ADJUSTER sa)
{
  KHE_MONITOR m;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laimi, laimj;
  int i, j;  KHE_RESOURCE r;  KHE_RESOURCE_TYPE rt;
  KHE_PROFILE_SOLVER ps;

  /* find suitable limit active intervals monitors */
  if( DEBUG1 )
    fprintf(stderr, "[ KheProfileGrouping(cg, sa)\n");
  ps = KheProfileSolverMake(cg, sa);
  if( DEBUG1 )
    KheProfileSolverDebug(ps, 1, 2, stderr);
  HaArrayClear(ps->monitors);
  rt = ps->resource_type;
  r = KheResourceTypeResource(rt, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(ps->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(ps->soln, r, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laimi = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      if( KheMonitorSuitsProfileGrouping(laimi, rt) )
      {
	if( DEBUG7 )
	  fprintf(stderr, "  found suitable monitor %s\n", KheMonitorId(m));
	HaArrayAddLast(ps->monitors, laimi);
      }
    }
  }
  HaArraySort(ps->monitors, &KheLimitActiveCmp);

  /* find ranges of suitable monitors and do the job for each range */
  for( i = 0;  i < HaArrayCount(ps->monitors);  i = j )
  {
    laimi = HaArray(ps->monitors, i);
    for( j = i + 1;  j < HaArrayCount(ps->monitors);  j++ )
    {
      laimj = HaArray(ps->monitors, j);
      if( KheLimitActiveTypedCmp(laimi, laimj) != 0 )
	break;
    }
    KheProfileGroupingForMonitors(ps, i, j - 1);
  }

  /* unfix any fixed leader tasks */
  KheSolnAdjusterDelete(ps->fix_leaders_sa);

  /* return the number of groups made */
  if( DEBUG1 )
    fprintf(stderr, "] KheProfileGrouping returning %d\n", ps->groups_count);
  return ps->groups_count;
}

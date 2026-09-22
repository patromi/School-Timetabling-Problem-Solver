
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
/*  FILE:         khe_sr_profile_grouper.c                                   */
/*  DESCRIPTION:  Profile grouping                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0  /* KheDoCombinatorialGroupingForInterval */
#define DEBUG3 0  /* KheDoProfile */
#define DEBUG4 0
#define DEBUG5 0  /* KheDoProfileGroupingMonitors */
#define DEBUG6 0  /* combination elimination */
#define	DEBUG7 0  /* profile grouping */
#define	DEBUG8 0  /* profile grouping */
#define	DEBUG9 0  /* special case */
#define	DEBUG10 0
#define	DEBUG11 0 /* KheProfile */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_DOMAIN                                                       */
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
/*  The cover attribute holds the number of tasks (not the number of         */
/*  mtasks) that cover this time group.  This is over all mtasks, not        */
/*  just mtasks from the current grouping.                                   */
/*                                                                           */
/*  The domain_covers attribute holds the number of tasks that cover         */
/*  this time group and have the corresponding domain.  The sum of           */
/*  these numbers equals cover.                                              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_profile_solver_rec *KHE_PROFILE_SOLVER;

typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;

typedef struct khe_profile_time_group_rec {
  KHE_PROFILE_SOLVER		profile_solver;
  KHE_TIME_GROUP		time_group;
  int				cover;
  ARRAY_KHE_PROFILE_DOMAIN	profile_domains;
  /* ***
  ARRAY_KHE_RESOURCE_GROUP	domains;
  HA_ARRAY_INT			domain_covers;
  *** */
  int				groups_made_here;
} *KHE_PROFILE_TIME_GROUP;

typedef HA_ARRAY(KHE_PROFILE_TIME_GROUP) ARRAY_KHE_PROFILE_TIME_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_SOLVER - solver for profile grouping                         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;

struct khe_profile_solver_rec {

  /* free lists */
  ARRAY_KHE_PROFILE_DOMAIN		profile_domain_free_list;
  ARRAY_KHE_PROFILE_TIME_GROUP		profile_time_group_free_list;

  /* fields in general use */
  HA_ARENA				arena;
  KHE_COMB_GROUPER			comb_grouper;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR monitors;
  int					groups_count;

  /* fields used by each monitor in turn */
  int					min_limit;
  int					max_limit;
  bool					history_before;
  bool					history_after;
  ARRAY_KHE_PROFILE_TIME_GROUP		profile_time_groups;
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
  res->cover = KheMTaskNeedsAssignmentCount(mt);
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
    pd->cover += KheMTaskNeedsAssignmentCount(mt);
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
/*  bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,       */
/*    KHE_RESOURCE_GROUP domain, int *pos)                                   */
/*                                                                           */
/*  If ptg contains domain, return true with *pos set to its position.       */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,
  KHE_RESOURCE_GROUP domain, int *pos)
{
  int i;  KHE_PROFILE_DOMAIN pd;
  HaArrayForEach(ptg->profile_domains, pd, i)
    if( KheResourceGroupEqual(domain, pd->domain) )
      return *pos = i, true;
  return *pos = -1, false;
}


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
  ptg->cover += KheMTaskNeedsAssignmentCount(mt);

  /* add mt to an existing profile domain if there is one */
  HaArrayForEach(ptg->profile_domains, pd, i)
    if( KheProfileDomainAcceptsMTask(pd, mt) )
      return;

  /* make a new profile domain and add mt to that */
  HaArrayAddLast(ptg->profile_domains,
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

static void KheProfileTimeGroupSetCover(KHE_PROFILE_TIME_GROUP ptg, int max_len)
{
  KHE_MTASK_FINDER mtf;  int i;  KHE_MTASK_SET mts;
  KHE_RESOURCE_TYPE rt;  KHE_MTASK mt;
  KHE_PROFILE_SOLVER ps;

  /* clear out old values, ready for a fresh start */
  ps = ptg->profile_solver;
  ptg->cover = 0;
  HaArrayAppend(ps->profile_domain_free_list, ptg->profile_domains, i);
  HaArrayClear(ptg->profile_domains);

  /* get mts, the mtasks that cover ptg->time_group */
  mtf = ps->mtask_finder;
  rt = ps->resource_type;
  mts = KheMTaskFinderMTasksInTimeGroup(mtf, rt, ptg->time_group);

  /* incorporate the admissible mtasks of mts into ptg */
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskAssignIsFixed(mt) && KheMTaskTotalDuration(mt) <= max_len &&
	KheMTaskNeedsAssignment(mt) )
      KheProfileTimeGroupAddMTask(ptg, mt);
  }

  /* sort the profile domains so that the smallest domains come first */
  HaArraySort(ptg->profile_domains, &KheProfileDomainCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_PROFILE_SOLVER ps,    */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Make a new profile time group object for time group tg.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_PROFILE_SOLVER ps,
  KHE_TIME_GROUP tg)
{
  KHE_PROFILE_TIME_GROUP res;

  /* get the object from the free list or make it fresh */
  if( HaArrayCount(ps->profile_time_group_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ps->profile_time_group_free_list);
    HaArrayClear(res->profile_domains);
  }
  else
  {
    HaMake(res, ps->arena);
    HaArrayInit(res->profile_domains, ps->arena);
  }
  res->profile_solver = ps;
  res->time_group = tg;
  res->cover = 0;
  res->groups_made_here = 0;

  /* set the cover and return */
  KheProfileTimeGroupSetCover(res, ps->max_limit);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)               */
/*                                                                           */
/*  Delete ptg.  This assumes that it is deleted from the list separately.   */
/*                                                                           */
/*****************************************************************************/

static void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg)
{
  KHE_PROFILE_SOLVER ps;  int i;
  ps = ptg->profile_solver;
  HaArrayAppend(ps->profile_domain_free_list, ptg->profile_domains, i);
  HaArrayAddLast(ps->profile_time_group_free_list, ptg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileSolverDeleteProfileTimeGroups(KHE_PROFILE_SOLVER ps)      */
/*                                                                           */
/*  Delete all of ps's profile time groups.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheProfileSolverDeleteProfileTimeGroups(KHE_PROFILE_SOLVER ps)
{
  while( HaArrayCount(ps->profile_time_groups) > 0 )
    KheProfileTimeGroupDelete(HaArrayLastAndDelete(ps->profile_time_groups));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupIncrease(KHE_PROFILE_TIME_GROUP ptg, int num,    */
/*    KHE_RESOURCE_GROUP domain)                                             */
/*                                                                           */
/*  Inform ptg that there are now num more tasks, each with the given        */
/*  domain, in its profile.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDecrease(KHE_PROFILE_TIME_GROUP ptg, int num,    */
/*    KHE_RESOURCE_GROUP domain)                                             */
/*                                                                           */
/*  Undo the corresponding Add.  Don't bother to remove zero entries.        */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskerClassCoversProfileTimeGroup(KHE_TASKER_CLASS c,            */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Return true if c covers ptg.                                             */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMTaskCoversProfileTimeGroup(KHE_MTASK mt,
  KHE_PROFILE_TIME_GROUP ptg)
{
  ** *** sti ll to do
  KHE_TASKER_TIME t;  int i;
  HaArrayForEach(c->profile_times, t, i)
    if( t->profile_time_group == ptg )
      return true;
  *** **
  HnAbort("KheMTaskCoversProfileTimeGroup sti ll to do");
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileTimeGroupDebug(KHE_PROFILE_TIME_GROUP ptg,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ptg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but current unused
static void KheProfileTimeGroupDebug(KHE_PROFILE_TIME_GROUP ptg,
  int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE_GROUP domain;  int i;
  fprintf(fp, "%*s[ ProfileTimeGroup(%s, cover %d)\n", indent, "",
    KheTimeGroupId(ptg->time_group), ptg->cover);
  HaArrayForEach(ptg->domains, domain, i)
    fprintf(fp, "%*s  %s: %d\n", indent, "", KheResourceGroupId(domain),
      HaArray(ptg->domain_covers, i));
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskerProfileDebug(KHE_TASKER tr, int verbosity, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug the profile.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
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
*** */


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
  mtf = KheCombGrouperMTaskFinder(cg);
  a = KheMTaskFinderArena(mtf);
  HaMake(res, a);

  /* free lists */
  HaArrayInit(res->profile_domain_free_list, a);
  HaArrayInit(res->profile_time_group_free_list, a);

  /* fields in general use */
  res->arena = a;
  res->comb_grouper = cg;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->resource_type = KheCombGrouperResourceType(cg);
  res->soln_adjuster = sa;
  HaArrayInit(res->monitors, a);
  res->groups_count = 0;

  /* fields used by each monitor in turn, so undefined between monitors */
  res->min_limit = res->max_limit = -1;
  res->history_before = res->history_after = -1;
  HaArrayInit(res->profile_time_groups, a);
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
/*  Return true if m suits profile grouping:  if its constraint applies to   */
/*  all resources of type rt, its time groups are all positive, and its      */
/*  time groups are all singletons (strict) or its limits are equal          */
/*  (non-strict).  Actually we can't enforce the second part here because    */
/*  we have not yet merged pairs of constraints.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorSuitsProfileGrouping(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_RESOURCE_TYPE rt, bool non_strict)
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

  if( non_strict )
  {
    /* its limits must be equal, but we can't enforce that here, because */
    /* we have a single monitor, not two with the same time groups */
    return true;
  }
  else
  {
    /* its time groups must be all singletons */
    return KheTimeGroupsAllSingletons(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorsCanGroupTogether(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)                              */
/*                                                                           */
/*  Return true if laim1 and laim2 can be handled together, because they     */
/*  have equal time groups, one has a maximum limit only, and the other      */
/*  has a minimum limit only.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheLimitActiveIntervalsMonitorsCanGroupTogether(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)
{
  int i;  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  bool has_min1, has_min2, has_max1, has_max2;

  ** time groups must be equal **
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(laim1) !=
      KheLimitActiveIntervalsMonitorTimeGroupCount(laim2) )
    return false;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);  i++ )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, i, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, i, &po2);
    if( !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }

  ** one must have a min limit only, the other a max limit only **
  has_min1 = KheLimitActiveIntervalsMonitorMinimum(laim1) > 0;
  has_min2 = KheLimitActiveIntervalsMonitorMinimum(laim2) > 0;
  has_max1 = KheLimitActiveIntervalsMonitorMaximum(laim1) <
    KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
  has_max2 = KheLimitActiveIntervalsMonitorMaximum(laim2) <
    KheLimitActiveIntervalsMonitorTimeGroupCount(laim2);
  if( (has_min1 && !has_max1) && (!has_min2 && has_max2) )
    return true;
  if( (!has_min1 && has_max1) && (has_min2 && !has_max2) )
    return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool IndexIsLegal(KHE_PROFILE_SOLVER ps, int index)                      */
/*                                                                           */
/*  Return true if index is a legal index of a profile time group.           */
/*                                                                           */
/*****************************************************************************/

static bool IndexIsLegal(KHE_PROFILE_SOLVER ps, int index)
{
  return index >= 0 && index < HaArrayCount(ps->profile_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetYesTimeGroups(KHE_PROFILE_SOLVER ps,           */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Set a sequence of Yes time groups for profile grouping.                  */
/*                                                                           */
/*****************************************************************************/

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


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetNoTimeGroups(KHE_PROFILE_SOLVER ps,            */
/*    int index1, int index2)                                                */
/*                                                                           */
/*  Set the two No time groups for profile grouping.                         */
/*                                                                           */
/*****************************************************************************/

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
  fprintf(fp, "%*s%s[", indent, "", ps->history_before ? "inf" : "0");
  HaArrayForEach(ps->profile_time_groups, ptg, i)
  {
    if( i > 0 )
      fprintf(fp, ":");
    fprintf(fp, "%d", ptg->cover);

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


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileSolverMTaskFn(KHE_MTASK mt, void *impl)                   */
/*                                                                           */
/*  Callback function which returns true if mt is to be included in a        */
/*  comb solve.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** the No groups will do this automatically, surely?
static bool KheProfileSolverMTaskFn(KHE_MTASK mt, void *impl)
{
  KHE_PROFILE_SOLVER ps = (KHE_PROFILE_SOLVER) impl;
  return KheMTaskTotalDuration(mt) <= ps->max_limit;
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

static int min(int a, int b) { return a < b ? a : b; }

static bool KheProfile(KHE_PROFILE_SOLVER ps, int first, int last,
  int no1, int no2, KHE_PROFILE_TIME_GROUP ptg,
  KHE_PROFILE_TIME_GROUP adj_ptg, int max_num, char *debug_str)
{
  int i, adj_cover, groups, singles, want, pos;  bool res;
  KHE_PROFILE_DOMAIN pd, adj_pd;

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
      max_num, HaArrayCount(ptg->profile_domains));
  }

  /* set up the time groups and other requirements, and count singles */
  KheCombGrouperClearRequirements(ps->comb_grouper);
  KheProfileGroupingSetYesTimeGroups(ps, first, last);
  KheProfileGroupingSetNoTimeGroups(ps, no1, no2);
  /* *** the No groups will do this automatically, surely?
  KheCombGrouperAddMTaskFnRequirement(ps->comb_grouper,
    &KheProfileSolverMTaskFn, (void *) ps);
  *** */
  singles = KheCombGrouperSolve(ps->comb_grouper, INT_MAX,
    KHE_COMB_VARIANT_SINGLES, ps->soln_adjuster, "profile singles");
  KheCombGrouperAddNoSinglesRequirement(ps->comb_grouper);
  max_num -= singles;

  /* find up to max_num groups, trying each domain of ptg in turn */
  res = false;
  for( i = 0;  i < HaArrayCount(ptg->profile_domains) && max_num > 0;  i++ )
  {
    /* find one domain, its cover, and its cover in adj_ptg */
    pd = HaArray(ptg->profile_domains, i);
    if( adj_ptg != NULL &&
	KheProfileTimeGroupContainsDomain(adj_ptg, pd->domain, &pos) )
    {
      adj_pd =  HaArray(adj_ptg->profile_domains, pos);
      adj_cover = adj_pd->cover;
    }
    else
      adj_cover = 0;
    want = pd->cover - adj_cover;
    if( DEBUG11 )
    {
      fprintf(stderr, "  domain ");
      KheResourceGroupDebug(pd->domain, 1, -1, stderr);
      fprintf(stderr, ", cover %d, adj_cover %d, want %d\n",
	pd->cover, adj_cover, want);
    }

    /* do some grouping if the domain has less cover in adj_ptg */
    if( want > 0 )
    {
      /* do the grouping and adjust max_num and groups */
      KheCombGrouperAddPreferredDomainRequirement(ps->comb_grouper, pd->domain);
      groups = KheCombGrouperSolve(ps->comb_grouper, min(want, max_num),
	KHE_COMB_VARIANT_ZERO, ps->soln_adjuster, debug_str);
      KheCombGrouperDeletePreferredDomainRequirement(ps->comb_grouper);
      if( groups > 0 )
      {
	/* update various records of what groups were made */
	ptg = HaArray(ps->profile_time_groups, first);
	ptg->groups_made_here += groups;
	ps->groups_count += groups;
	res = true;

	/* reduce the number of groups remaining to be found */
	max_num -= groups;
	if( DEBUG3 )
	  fprintf(stderr, "  %s made %d groups for %s", debug_str, groups,
	    KheResourceGroupId(pd->domain));
	/* ***
	if( DEBUG5 )
	  KheProfileSolverDebug(ps, 1, 2, stderr);
	*** */

	/* ***
	if( DEBUG8 )
          KheProfileSolverDebug(ps, 2, 2, stderr);
	*** */

	/* ptg's list of domains may have changed, so start it again */
	/* *** no need for this
	i = -1;
	*** */
      }
    }
  }

  /* update the profile time groups; some mtasks may have dropped out */
  if( res )
  {
    for( i = first;  i <= last;  i++ )
    {
      ptg = HaArray(ps->profile_time_groups, i);
      KheProfileTimeGroupSetCover(ptg, ps->max_limit);
    }
  }
  if( DEBUG11 )
    fprintf(stderr, "] KheProfile returning %s (groups_count %d)\n",
      bool_show(res), ps->groups_count);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps)                 */
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are unequal, and   */
/*  return true if any groups were made.  Make as many groups as possible.   */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps)
{
  int tg_count, i, prev_cover, next_cover;  bool res, progressing;
  KHE_PROFILE_TIME_GROUP curr_ptg, prev_ptg, next_ptg;

  /* boilerplate */
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheProfileTryUnequalGrouping\n");
    KheProfileSolverDebug(ps, 1, 2, stderr);
  }
  tg_count = HaArrayCount(ps->profile_time_groups);
  res = false;

  do
  {
    progressing = false;

    /* forward pass to handle places where sequences start */
    prev_ptg = NULL;
    prev_cover = ps->history_before ? INT_MAX : 0;
    for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      if( curr_ptg->cover > prev_cover )
      {
	/* curr_ptg->cover - prev_cover sequences must start at i */
	if( DEBUG5 )
	  fprintf(stderr, "  start at %d (curr %d > prev %d)\n", i,
	    curr_ptg->cover, prev_cover);
	if( KheProfile(ps, i, i + ps->min_limit - 1, i - 1,
	      i + ps->max_limit, curr_ptg, prev_ptg,
	      curr_ptg->cover - prev_cover, "forwards profile grouping") )
	  res = progressing = true;
      }

      /* reset prev_cover for next iteration */
      prev_ptg = curr_ptg;
      prev_cover = prev_ptg->cover;
    }

    /* backward pass to handle places where sequences end */
    next_ptg = NULL;
    next_cover = ps->history_after ? INT_MAX : 0;
    for( i = tg_count - 1;  i >= ps->min_limit - 1;  i-- )
    {
      curr_ptg = HaArray(ps->profile_time_groups, i);
      if( curr_ptg->cover > next_cover )
      {
	/* curr_ptg->cover - next_cover sequences must end at i */
	if( DEBUG5 )
	  fprintf(stderr, "  end at %d (curr %d > next %d)\n", i,
	    curr_ptg->cover, next_cover);
	if( KheProfile(ps, i - ps->min_limit + 1, i, i - ps->max_limit,
	      i + 1, curr_ptg, next_ptg, curr_ptg->cover - next_cover,
	      "backward profile grouping") )
	  res = progressing = true;
      }

      /* reset next_cover for next iteration */
      next_ptg = curr_ptg;
      next_cover = next_ptg->cover;
    }
  } while( progressing );

  /* all done */
  if( DEBUG5 )
    fprintf(stderr, "] KheProfileTryUnequalGrouping returning %s\n",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps)                   */
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are equal, and     */
/*  return true if any groups were made.  Make at most one group.            */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps)
{
  int val, i, min_groups, tg_count, count;  KHE_PROFILE_TIME_GROUP curr_ptg;

  /* find min_groups, the minimum number of groupings */
  min_groups = INT_MAX;
  tg_count = HaArrayCount(ps->profile_time_groups);
  for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
  {
    /* curr_ptg = KheTaskerProfileTimeGroup(tr, i); */
    curr_ptg = HaArray(ps->profile_time_groups, i);
    if( curr_ptg->cover > 0 && curr_ptg->groups_made_here < min_groups )
      min_groups = val;
  }

  /* for each pos with a minimum number of groups, try to group */
  count = tg_count - ps->min_limit + 1;
  for( i = 0;  i < count;  i++ )
  {
    curr_ptg = HaArray(ps->profile_time_groups, i);
    if( curr_ptg->cover > 0 && curr_ptg->groups_made_here == min_groups &&
	KheProfile(ps, i, i + ps->min_limit - 1,
	  i - 1, i + ps->max_limit, curr_ptg, NULL, 1,
	  "non-strict profile grouping") )
      return true;
  }

  /* for each pos with just above a min number of groups, try to group */
  for( i = 0;  i < count;  i++ )
  {
    curr_ptg = HaArray(ps->profile_time_groups, i);
    if( curr_ptg->cover > 0 && curr_ptg->groups_made_here == min_groups + 1 &&
	KheProfile(ps, i, i + ps->min_limit - 1,
	  i - 1, i + ps->max_limit, curr_ptg, NULL, 1,
	  "non-strict profile grouping") )
      return true;
  }

  /* no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2, bool non_strict)             */
/*                                                                           */
/*  Carry out profile grouping based on laim1 and optionally laim2.          */
/*                                                                           */
/*****************************************************************************/

static void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,
  int first_index, int last_index, bool non_strict)
{
  int tg_count, i;  KHE_POLARITY po;  KHE_TIME_GROUP tg;
  KHE_PROFILE_TIME_GROUP ptg;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;

  if( DEBUG8 )
  {
    laim = HaArray(ps->monitors, first_index);
    fprintf(stderr, "[ KheProfileGroupingForMonitors(ps, %s, %d, %d, %s)\n",
      KheMonitorId((KHE_MONITOR) laim), first_index, last_index,
      non_strict ? "non_strict" : "strict");
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

  /* do it if non-trivial min limit and either strict or limits equal */
  if( ps->min_limit >= 2 && (!non_strict || ps->min_limit == ps->max_limit) )
  {
    laim = HaArray(ps->monitors, first_index);
    tg_count = KheLimitActiveIntervalsMonitorTimeGroupCount(laim);
    for( i = 0;  i < tg_count;  i++ )
    {
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
      ptg = KheProfileTimeGroupMake(ps, tg);
      HaArrayAddLast(ps->profile_time_groups, ptg);
    }

    /* keep grouping while groups are being found */
    do
    {
      if( DEBUG8 )
	KheProfileSolverDebug(ps, 1, 2, stderr);
      KheProfileTryUnequalGrouping(ps);
      if( DEBUG8 )
	KheProfileSolverDebug(ps, 1, 2, stderr);
    } while( non_strict && KheProfileTryEqualGrouping(ps) );

    /* clear this monitor's time groups away */
    KheProfileSolverDeleteProfileTimeGroups(ps);
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
/*  are brought together.  This function assumes that all time groups        */
/*  are positive.                                                            */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveTypedCmp(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int i, cmp, count1, count2;

  /* time groups must be equal, but no need to check polarity */
  count1 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
  count2 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim2);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
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
/*  int KheProfileGrouping(KHE_COMB_GROUPER cg, bool non_strict,             */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Carry out profile grouping for cg.  If non_strict is true, carry out     */
/*  non_strict profile grouping.  Return the number of groups made.          */
/*                                                                           */
/*****************************************************************************/

int KheProfileGrouping(KHE_COMB_GROUPER cg, bool non_strict,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_MONITOR m;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laimi, laimj;
  int i, j;  KHE_RESOURCE r;  KHE_RESOURCE_TYPE rt;
  KHE_PROFILE_SOLVER ps;

  /* find suitable limit active intervals monitors */
  ps = KheProfileSolverMake(cg, sa);
  HaArrayClear(ps->monitors);
  rt = ps->resource_type;
  r = KheResourceTypeResource(rt, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(ps->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(ps->soln, r, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laimi = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      if( KheMonitorSuitsProfileGrouping(laimi, rt, non_strict) )
      {
	if( DEBUG7 )
	{
	  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;
	  c = KheLimitActiveIntervalsMonitorConstraint(laimi);
	  fprintf(stderr, "  found suitable monitor, constraint %s\n",
	    KheConstraintId((KHE_CONSTRAINT) c));
	}
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
    KheProfileGroupingForMonitors(ps, i, j - 1, non_strict);
  }
  return ps->groups_count;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheGroupByResourceConstraints"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheBCDTSpecialAddClass(KHE_TASKER tr, KHE_TIME t)                   */
/*                                                                           */
/*  Add to tr a class corresponding to t.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheBCDTSpecialAddClass(KHE_TASKER tr, KHE_TIME t)
{
  KHE_TASK ER_TIME tt;  KHE_TASKER_CLASS tc;
  tt = KheTaskerTime(tr, KheTimeIndex(t));
  HnAssert(KheTaskerTimeClassCount(tt) == 1,
    "KheBCDTSpecialAddClass(tr, %s): class count is %d\n", KheTimeId(t),
    KheTaskerTimeClassCount(tt));
  tc = KheTaskerTimeClass(tt, 0);
  if( !KheTaskerGroupingAddClass(tr, tc) )
    HnAbort("KheBCDTSpecialAddClass(tr, %s): cannot add class", KheTimeId(t));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheBCDTSpecialCaseGrouping(KHE_TASKER tr, KHE_FRAME common_frame)   */
/*                                                                           */
/*  This function groups the {Wed, Thu, Fri, Sat} and {Sun, Mon, Tue} night  */
/*  shifts.  It is used only on instance COI-BCDT-Sep-A, an experimental     */
/*  version of COI-BCDT-Sep.  It is not for production use.                  */
/*                                                                           */
/*****************************************************************************/

/* *** this works but it was only ever an exploratory thing
static void KheBCDTSpecialCaseGrouping(KHE_TASKER tr, KHE_FRAME common_frame)
{
  int i;  KHE_TIME t1, t2, t3, t4, t5, t6, t7;
  if( DEBUG9 )
    fprintf(stderr, "[ KheBCDTSpecialCaseGrouping()\n");
  for( i = 0;  i < KheFrameTimeGroupCount(common_frame) - 7;  i += 7 )
  {
    ** Wed, Thu, Fri, Sat **
    t1 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 0), 3);
    t2 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 1), 3);
    t3 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 2), 3);
    t4 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 3), 3);
    if( DEBUG9 )
      fprintf(stderr, "  {%s, %s, %s, %s}\n",
	KheTimeId(t1), KheTimeId(t2), KheTimeId(t3), KheTimeId(t4));
    KheTaskerGroupingClear(tr);
    KheBCDTSpecialAddClass(tr, t1);
    KheBCDTSpecialAddClass(tr, t2);
    KheBCDTSpecialAddClass(tr, t3);
    KheBCDTSpecialAddClass(tr, t4);
    KheTaskerGroupingBuild(tr, 2, "BCDT special");

    ** Sun, Mon, Tue **
    t5 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 4), 3);
    t6 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 5), 3);
    t7 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 6), 3);
    if( DEBUG9 )
      fprintf(stderr, "  {%s, %s, %s}\n",
	KheTimeId(t5), KheTimeId(t6), KheTimeId(t7));
    KheTaskerGroupingClear(tr);
    KheBCDTSpecialAddClass(tr, t5);
    KheBCDTSpecialAddClass(tr, t6);
    KheBCDTSpecialAddClass(tr, t7);
   profile_off KheTaskerGroupingBuild(tr, 2, "BCDT special");
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheBCDTSpecialCaseGrouping()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,         */
/*    KHE_TASK_SET ts, bool history_off, bool comb_off, bool profile_off,    */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Carry out group by resource constraints for resource type rt.  There     */
/*  is no point in doing anything if there are fewer than two resources.     */
/*                                                                           */
/*****************************************************************************/

static int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa, bool comb_off, bool profile_off,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_MTASK_FINDER mtf;  KHE_COMB_GROUPER cg;  KHE_FRAME days_frame;  int res;

  res = 0;
  if( KheResourceTypeResourceCount(rt) > 1 && !comb_off && !profile_off )
  {
    /* make an mtask finder */
    days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
    mtf = KheMTaskFinderMake(soln, rt, days_frame, true, a);

    /* make a comb grouper */
    cg = KheCombGrouperMake(mtf, rt, a);

    /* combinatorial grouping */
    if( !comb_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) comb grouping:\n",
	  KheResourceTypeId(rt));
      res += KheCombGrouping(cg, options, sa);
    }

    /* profile grouping */
    if( !profile_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) profile(false):\n",
	  KheResourceTypeId(rt));
      res += KheProfileGrouping(cg, false, sa);
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) profile(true):\n",
	  KheResourceTypeId(rt));
      res += KheProfileGrouping(cg, true, sa);
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,  */
/*    KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)                             */
/*                                                                           */
/*  Group the tasks of soln of type rt by resource constraints.  If rt is    */
/*  NULL, do this for each of the resource types of soln's instance.         */
/*                                                                           */
/*  If sa is non-NULL, record what was done in sa so that it can be undone   */
/*  later.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  bool comb_off, profile_off;  int i, groups;  KHE_INSTANCE ins;  HA_ARENA a;

  /* boilerplate */
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByResourceConstraints(%s %s, -)\n",
      KheInstanceId(ins), rt == NULL ? "all types" : KheResourceTypeId(rt));

  /* return immediately if option rs_group_by_frame_off is true */
  if( KheOptionsGetBool(options, "rs_group_by_rc_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr,"] KheGroupByResourceConstraints returning false (off)\n");
    return false;
  }

  comb_off = KheOptionsGetBool(options, "rs_group_by_rc_combinatorial_off",
    false);
  profile_off = KheOptionsGetBool(options, "rs_group_by_rc_profile_off", false);

  /* do grouping by resource constraints for each resource type */
  a = KheSolnArenaBegin(soln);
  groups = 0;
  if( rt != NULL )
    groups += KheGroupForResourceType(soln, rt, sa, comb_off, profile_off,
      options, a);
  else for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    groups += KheGroupForResourceType(soln, rt, sa, comb_off, profile_off,
      options, a);
  }
  KheSolnArenaEnd(soln, a);

  /* all done */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByResourceConstraints returning %s\n",
      bool_show(groups > 0));
  return groups > 0;
}

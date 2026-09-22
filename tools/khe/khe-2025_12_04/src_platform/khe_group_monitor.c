
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
/*  FILE:         khe_group_monitor.c                                        */
/*  DESCRIPTION:  A group monitor                                            */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR - monitors a group of other monitors.                  */
/*                                                                           */
/*****************************************************************************/

struct khe_group_monitor_rec {
  INHERIT_GROUP_MONITOR
  KHE_GROUP_MONITOR	   copy;		/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorCheck(KHE_GROUP_MONITOR gm)                          */
/*                                                                           */
/*  Check gm.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorCheck(KHE_GROUP_MONITOR gm, KHE_MONITOR change_m,
  KHE_COST old_cost, KHE_COST new_cost)
{
  KHE_COST total_cost;  int i;  KHE_MONITOR_LINK link;  KHE_MONITOR m;
  if( DEBUG4 && !KheSolnMatchingIsActive(gm->soln) &&
      KheSolnType(gm->soln) == KHE_SOLN_ORDINARY )
  {
    /* check children */
    total_cost = 0;
    HaArrayForEach(gm->child_links, link, i)
    {
      m = link->child;
      total_cost += KheMonitorCost(m);
    }
    if( total_cost != gm->cost )
    {
      if( change_m != NULL )
      {
	fprintf(stderr, "KheGroupMonitorCheck failing (m %.5f -> %.5f):\n",
	  KheCostShowWide(old_cost), KheCostShowWide(new_cost));
	KheMonitorDebug(change_m, 4, 2, stderr);
      }
      else
	fprintf(stderr, "KheGroupMonitorCheck failing:\n");
      KheGroupMonitorDebug(gm, 4, 2, stderr);
      HnAbort("KheGroupMonitorCheck(%s) internal error:"
      " (a) total_cost %.8f, gm->cost %.8f", KheMonitorId((KHE_MONITOR) gm),
      KheCostShowWide(total_cost), KheCostShowWide(gm->cost));
    }

    /* check defects */
    total_cost = 0;
    HaArrayForEach(gm->defect_links, link, i)
    {
      m = link->child;
      total_cost += KheMonitorCost(m);
    }
    if( total_cost != gm->cost )
    {
      fprintf(stderr, "KheGroupMonitorCheck failing");
      KheGroupMonitorDebug(gm, 4, 2, stderr);
      HnAbort("KheGroupMonitorCheck(%s) internal error:"
      " (b) total_cost %.8f, gm->cost %.8f", KheMonitorId((KHE_MONITOR) gm),
      KheCostShowWide(total_cost), KheCostShowWide(gm->cost));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheGroupMonitorMake(KHE_SOLN soln, int sub_tag,        */
/*    char *sub_tag_label)                                                   */
/*                                                                           */
/*  Make a new group monitor with these attributes, and no parent or         */
/*  children.                                                                */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheGroupMonitorMake(KHE_SOLN soln, int sub_tag,
  char *sub_tag_label)
{
  KHE_GROUP_MONITOR res;  /* int i; */  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupMonitorMake(soln, %d, %s)\n",
      sub_tag, sub_tag_label);
  a = KheSolnArena(soln);
  res = KheSolnGetGroupMonitorFromFreeList(soln);
  if( res == NULL )
  {
    HaMake(res, a);
    HaArrayInit(res->parent_links, a);
    HaArrayInit(res->child_links, a);
    HaArrayInit(res->defect_links, a);
    HaArrayInit(res->traces, a);
  }
  else
  {
    HaArrayClear(res->parent_links);
    HaArrayClear(res->child_links);
    HaArrayClear(res->defect_links);
    HaArrayClear(res->traces);
  }
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln, KHE_GROUP_MONITOR_TAG,
    KHE_LINEAR_COST_FUNCTION, 0);
  /* MArrayInit(res->defect_links_copy); */
  res->sub_tag = sub_tag;
  res->sub_tag_label = HnStringCopy(sub_tag_label, a);
  res->copy = NULL;
  KheGroupMonitorCheck(res, NULL, 0, 0);
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupMonitorMake returning %p\n", (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDelete(KHE_GROUP_MONITOR gm)                         */
/*                                                                           */
/*  Delete gm.  This should *not* delete its child monitors!                 */
/*                                                                           */
/*  Implementation note.  Unlike other monitor delete operations, which      */
/*  were withdrawn on the advent of the Ha arena memory allocator, this      */
/*  one continues in use because it is needed by end users.  However,        */
/*  instead of freeing memory as formerly, it now adds gm to a free list.    */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDelete(KHE_GROUP_MONITOR gm)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupMonitorDelete(%p)\n", (void *) gm);
  KheGroupMonitorCheck(gm, NULL, 0, 0);
  HnAssert(gm != (KHE_GROUP_MONITOR) gm->soln,
    "KheGroupMonitorDelete:  gm is soln");
  HnAssert(HaArrayCount(gm->traces) == 0,
    "KheGroupMonitorDelete:  gm is currently being traced");
  while( KheGroupMonitorChildMonitorCount(gm) > 0 )
    KheGroupMonitorDeleteChildMonitor(gm, KheGroupMonitorChildMonitor(gm, 0));
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) gm);
  KheSolnDeleteMonitor(gm->soln, (KHE_MONITOR) gm);
  KheSolnAddGroupMonitorToFreeList(gm->soln, gm);
  /* ***
  MArrayFree(gm->child_links);
  MArrayFree(gm->defect_links);
  MArrayFree(gm->traces);
  MFree(gm->sub_tag_label);
  MFree(gm);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupMonitorDelete returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorBypassAndDelete(KHE_GROUP_MONITOR gm)                */
/*                                                                           */
/*  Move gm's child monitors to be children of gm's parent, if any, then     */
/*  delete gm.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorBypassAndDelete(KHE_GROUP_MONITOR gm)
{
  KHE_MONITOR_LINK plk, clk;  int i, j;
  HaArrayForEach(gm->parent_links, plk, i)
    HaArrayForEach(gm->child_links, clk, j)
      KheGroupMonitorAddChildMonitor(plk->parent, clk->child);
  KheGroupMonitorDelete(gm);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheGroupMonitorCopyPhase1(KHE_GROUP_MONITOR gm,        */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of copying gm.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheGroupMonitorCopyPhase1(KHE_GROUP_MONITOR gm, HA_ARENA a)
{
  KHE_GROUP_MONITOR copy;  int i;  KHE_MONITOR_LINK link;
  if( gm->soln == (KHE_SOLN) gm )
  {
    /* gm is actually the soln object */
    return (KHE_GROUP_MONITOR) KheSolnCopyPhase1((KHE_SOLN) gm);
  }
  else
  {
    if( gm->copy == NULL )
    {
      /* KheSolnMatchingUpdate(gm->soln); */
      HnAssert(HaArrayCount(gm->traces) == 0,
        "KheGroupMonitorCopy cannot copy:  gm is currently being traced");
      HaMake(copy, a);
      gm->copy = copy;
      KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) gm, a);
      HaArrayInit(copy->child_links, a);
      HaArrayForEach(gm->child_links, link, i)
        HaArrayAddLast(copy->child_links, KheMonitorLinkCopyPhase1(link, a));
      HaArrayInit(copy->defect_links, a);
      HaArrayForEach(gm->defect_links, link, i)
        HaArrayAddLast(copy->defect_links, KheMonitorLinkCopyPhase1(link, a));
      /* MArrayInit(copy->defect_links_copy); */
      HaArrayInit(copy->traces, a);
      copy->sub_tag = gm->sub_tag;
      copy->sub_tag_label = gm->sub_tag_label;
      copy->copy = NULL;
      KheGroupMonitorCheck(gm->copy, NULL, 0, 0);
    }
    return gm->copy;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorCopyPhase2(KHE_GROUP_MONITOR gm)                     */
/*                                                                           */
/*  Carry out Phase 2 of copying gm.                                         */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorCopyPhase2(KHE_GROUP_MONITOR gm)
{
  KHE_MONITOR_LINK link;  int i;
  if( gm->soln == (KHE_SOLN) gm )
  {
    /* gm is actually the soln object */
    KheSolnCopyPhase2((KHE_SOLN) gm);
  }
  else if( gm->copy != NULL )
  {
    gm->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) gm);
    HaArrayForEach(gm->child_links, link, i)
      KheMonitorLinkCopyPhase2(link);
    HaArrayForEach(gm->defect_links, link, i)
      KheMonitorLinkCopyPhase2(link);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorSubTag(KHE_GROUP_MONITOR gm)                          */
/*                                                                           */
/*  Return the sub_tag attribute of gm.                                      */
/*                                                                           */
/*****************************************************************************/

int KheGroupMonitorSubTag(KHE_GROUP_MONITOR gm)
{
  return gm->sub_tag;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheGroupMonitorSubTagLabel(KHE_GROUP_MONITOR gm)                   */
/*                                                                           */
/*  Return the sub_tag_label attribute of gm.                                */
/*                                                                           */
/*****************************************************************************/

char *KheGroupMonitorSubTagLabel(KHE_GROUP_MONITOR gm)
{
  return gm->sub_tag_label;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAttachToSoln(KHE_GROUP_MONITOR m)                    */
/*                                                                           */
/*  Attach m to the solution.  This does nothing substantial.                */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorAttachToSoln(KHE_GROUP_MONITOR m)
{
  m->attached = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDetachFromSoln(KHE_GROUP_MONITOR m)                  */
/*                                                                           */
/*  Detach m from the solution.  This does nothing substantial.              */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDetachFromSoln(KHE_GROUP_MONITOR m)
{
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child monitors"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupMonitorCycle(KHE_GROUP_MONITOR gm, KHE_MONITOR m)           */
/*                                                                           */
/*  Return true if adding m to gm would cause a monitor cycle.               */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupMonitorCycle(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  return KheMonitorTag(m) != KHE_GROUP_MONITOR_TAG ? false :
    KheMonitorPathCount((KHE_MONITOR) gm, m) >= 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAddDefect(KHE_GROUP_MONITOR gm, KHE_MONITOR m,       */
/*    KHE_MONITOR_LINK link)                                                 */
/*                                                                           */
/*  Add m to the list of defects of gm.  Here link joins gm and m.           */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorAddDefect(KHE_GROUP_MONITOR gm, KHE_MONITOR m,
  KHE_MONITOR_LINK link)
{
  int pos;
  if( DEBUG3 )
  {
    HnAssert(link->parent_defects_index == -1,
      "KheGroupMonitorAddDefect internal error 1");
    HnAssert(!HaArrayContains(gm->defect_links, link, &pos),
      "KheGroupMonitorAddDefect internal error 2");
  }
  link->parent_defects_index = HaArrayCount(gm->defect_links);
  HaArrayAddLast(gm->defect_links, link);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDeleteDefect(KHE_GROUP_MONITOR gm, KHE_MONITOR m,    */
/*    KHE_MONITOR_LINK link)                                                 */
/*                                                                           */
/*  Delete m from the list of defects of gm.  Here link links gm and m.      */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorDeleteDefect(KHE_GROUP_MONITOR gm, KHE_MONITOR m,
  KHE_MONITOR_LINK link)
{
  int pos;  KHE_MONITOR_LINK link2;
  if( DEBUG2 )
  {
    fprintf(stderr,
      "  [ KheGroupMonitorDeleteDefect(gm %p (%d defects), m (di %d)\n",
      (void *) gm, HaArrayCount(gm->defect_links), link->parent_defects_index);
    KheMonitorDebug((KHE_MONITOR) gm, 1, 4, stderr);
    KheMonitorDebug(m, 2, 4, stderr);
  }
  /* bug here */
  HnAssert(link->parent_defects_index != -1,
    "KheGroupMonitorDeleteDefect internal error 1");
  HnAssert(HaArray(gm->defect_links, link->parent_defects_index) == link,
    "KheGroupMonitorDeleteDefect internal error 2");
  link2 = HaArrayLastAndDelete(gm->defect_links);
  if( link2 != link )
  {
    HaArrayPut(gm->defect_links, link->parent_defects_index, link2);
    link2->parent_defects_index = link->parent_defects_index;
  }
  link->parent_defects_index = -1;
  if( DEBUG3 )
  {
    HnAssert(link->parent_defects_index == -1,
      "KheGroupMonitorDeleteDefect internal error 3");
    HnAssert(!HaArrayContains(gm->defect_links, link, &pos),
      "KheGroupMonitorDeleteDefect internal error 4");
  }
  if( DEBUG2 )
    fprintf(stderr, "  ] KheGroupMonitorDeleteDefect\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAddMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)      */
/*                                                                           */
/*  Internal function which adds m to gm, assuming m is currently not the    */
/*  child of any monitor.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** folded into KheGroupMonitorAddChildMonitor now
void KheGroupMonitorAddMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  HnAssert(!KheGroupMonitorCycle(gm, m),
    "KheGroupMonitorAddChildMonitor: operation would cause a monitor cycle");
  ** *** not doing this stuff any more
  if( KheMonitorAttached(m) && !KheMonitorAttached((KHE_MONITOR) gm) )
    KheMonitorAttach((KHE_MONITOR) gm);
  *** **

  ** add m to child_monitors **
  KheMonitorSetParentMonitorAndIndex(m, gm, HaArrayCount(gm->child_monitors));
  HaArrayAddLast(gm->child_monitors, m);

  ** change gm's cost and add m to defects, if m has non-zero cost **
  if( KheMonitorCost(m) > 0 )
    KheGroupMonitorChangeCost(gm, m, 0, KheMonitorCost(m));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDeleteMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)   */
/*                                                                           */
/*  Internal function which deletes m from gm.                               */
/*                                                                           */
/*****************************************************************************/

/* folded into KheGroupMonitorDeleteChildMonitor now
void KheGroupMonitorDeleteMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  KHE_MONITOR m2;

  ** remove m from child_monitors **
  HnAssert(HaArray(gm->child_monitors, KheMonitorParentIndex(m)) == m,
    "KheGroupMonitorDeleteMonitor internal error");
  m2 = HaArrayLastAndDelete(gm->child_monitors);
  if( m2 != m )
  {
    HaArrayPut(gm->child_monitors, KheMonitorParentIndex(m), m2);
    KheMonitorSetParentMonitorAndIndex(m2, gm, KheMonitorParentIndex(m));
  }

  ** change gm's cost and remove m from defects, if m has non-zero cost **
  if( KheMonitorCost(m) > 0 )
    KheGroupMonitorChangeCost(gm, m, KheMonitorCost(m), 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorChangeLowerBound(KHE_GROUP_MONITOR gm,               */
/*    KHE_COST delta)                                                        */
/*                                                                           */
/*  Add delta to the lower bound of gm and its ancestors along all paths.    */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorChangeLowerBound(KHE_GROUP_MONITOR gm,
  KHE_COST delta)
{
  KHE_MONITOR_LINK link;  int i;
  gm->lower_bound += delta;
  HaArrayForEach(gm->parent_links, link, i)
    KheGroupMonitorChangeLowerBound(link->parent, delta);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAddChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m) */
/*                                                                           */
/*  Add m as a child to gm.                                                  */
/*                                                                           */
/*****************************************************************************/
void KheGroupMonitorChangeCostInternal(KHE_GROUP_MONITOR gm, KHE_MONITOR m,
  KHE_MONITOR_LINK link, KHE_COST old_cost, KHE_COST new_cost, bool check);

void KheGroupMonitorAddChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  KHE_MONITOR_LINK link;  HA_ARENA a;

  /* make sure m's cost is up to date (IMPORTANT - this is a nasty bug fix!) */
  KheMonitorCost(m);
  KheGroupMonitorCheck(gm, NULL, 0, 0);

  /* make sure the add would not cause a monitor cycle */
  HnAssert(!KheGroupMonitorCycle(gm, m),
    "KheGroupMonitorAddChildMonitor: operation would cause a monitor cycle");

  /* get a new link object from the soln, or make one, and initialize it */
  link = KheSolnGetMonitorLinkFromFreeList(gm->soln);
  if( link == NULL )
  {
    a = KheSolnArena(gm->soln);
    HaMake(link, a);
  }
  link->parent = gm;
  link->child = m;
  link->parent_index = HaArrayCount(gm->child_links);
  HaArrayAddLast(gm->child_links, link);
  link->parent_defects_index = -1;  /* not considered a defect yet */
  KheMonitorAddParentLink(m, link);  /* will set link->child_index */
  link->copy = NULL;

  /* update gm's lower bound */
  if( KheMonitorLowerBound(m) > 0 )
    KheGroupMonitorChangeLowerBound(gm, KheMonitorLowerBound(m));

  /* change gm's cost and add m to defects, if m has non-zero cost */
  if( KheMonitorCost(m) > 0 )
    KheGroupMonitorChangeCostInternal(gm, m, link, 0, KheMonitorCost(m), false);
  KheGroupMonitorCheck(gm, NULL, 0, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDeleteChildMonitor(KHE_GROUP_MONITOR gm,             */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Delete m from gm.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDeleteChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  KHE_MONITOR_LINK link, link2;

  /* change gm's cost and remove m from defects, if m has non-zero cost */
  KheGroupMonitorCheck(gm, NULL, 0, 0);
  if( !KheMonitorHasParentLink(m, gm, &link) )
    HnAbort("KheGroupMonitorDeleteChildMonitor: gm not parent of m");
  if( KheMonitorCost(m) > 0 )
    KheGroupMonitorChangeCostInternal(gm, m, link, KheMonitorCost(m), 0,
      false);

  /* update gm's lower bound */
  if( KheMonitorLowerBound(m) > 0 )
    KheGroupMonitorChangeLowerBound(gm, - KheMonitorLowerBound(m));

  /* remove link from m's parent links */
  KheMonitorDeleteParentLink(m, link);

  /* remove link from gm's child_links */
  link2 = HaArrayLastAndDelete(gm->child_links);
  if( link2 != link )
  {
    HaArrayPut(gm->child_links, link->parent_index, link2);
    link2->parent_index = link->parent_index;
  }
  link->parent_index = -1;

  /* free link */
  KheSolnAddMonitorLinkToFreeList(gm->soln, link);
  KheGroupMonitorCheck(gm, NULL, 0, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupMonitorHasChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m) */
/*                                                                           */
/*  Return true when m is a child of gm.                                     */
/*                                                                           */
/*  Implementation note.  It will usually be faster to search the parent     */
/*  links of m rather than the child links of gm, since monitors usually     */
/*  have very few parents.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheGroupMonitorHasChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m)
{
  KHE_MONITOR_LINK link;
  return KheMonitorHasParentLink(m, gm, &link);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorChildMonitorCount(KHE_GROUP_MONITOR gm)               */
/*                                                                           */
/*  Return the number of monitors reporting to gm.                           */
/*                                                                           */
/*****************************************************************************/

int KheGroupMonitorChildMonitorCount(KHE_GROUP_MONITOR gm)
{
  return HaArrayCount(gm->child_links);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheGroupMonitorChildMonitor(KHE_GROUP_MONITOR gm, int i)     */
/*                                                                           */
/*  Return the i'th monitor reporting to gm.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheGroupMonitorChildMonitor(KHE_GROUP_MONITOR gm, int i)
{
  KHE_MONITOR_LINK link;
  link = HaArray(gm->child_links, i);
  return link->child;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorDefectCount(KHE_GROUP_MONITOR gm)                     */
/*                                                                           */
/*  Return the number of defects (child monitors of non-zero cost) of gm.    */
/*                                                                           */
/*****************************************************************************/

int KheGroupMonitorDefectCount(KHE_GROUP_MONITOR gm)
{
  KheSolnMatchingUpdate(gm->soln);
  return HaArrayCount(gm->defect_links);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheGroupMonitorDefect(KHE_GROUP_MONITOR gm, int i)           */
/*                                                                           */
/*  Return the i'th defect (child monitor of non-zero cost) of gm.           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheGroupMonitorDefect(KHE_GROUP_MONITOR gm, int i)
{
  KHE_MONITOR_LINK link;
  KheSolnMatchingUpdate(gm->soln);
  link = HaArray(gm->defect_links, i);
  return link->child;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDefectSort(KHE_GROUP_MONITOR gm, bool diversify)     */
/*                                                                           */
/*  Sort the defects of gm into decreasing cost order.  If diversify is      */
/*  true, break ties in cost using gm's solution's diversifier.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheMonitorLinkDecreasingCostCmp(const void *t1, const void *t2)
{
  KHE_MONITOR_LINK link1 = * (KHE_MONITOR_LINK *) t1;
  KHE_MONITOR_LINK link2 = * (KHE_MONITOR_LINK *) t2;
  KHE_MONITOR m1 = link1->child;
  KHE_MONITOR m2 = link2->child;
  int cmp = KheCostCmp(KheMonitorCost(m1), KheMonitorCost(m2));
  if( cmp != 0 )
    return cmp;
  else
    return KheMonitorIndex InSoln(m1) - KheMonitorIndex InSoln(m2);
}

static int KheMonitorLinkDecreasingCostDiversifyCmp(const void *t1,
  const void *t2)
{
  KHE_MONITOR_LINK link1 = * (KHE_MONITOR_LINK *) t1;
  KHE_MONITOR_LINK link2 = * (KHE_MONITOR_LINK *) t2;
  KHE_MONITOR m1 = link1->child;
  KHE_MONITOR m2 = link2->child;
  int cmp = KheCostCmp(KheMonitorCost(m1), KheMonitorCost(m2));
  if( cmp != 0 )
    return cmp;
  else if( KheSolnDiver sifierChoose(KheMonitorSoln(m1), 2) == 0 )
    return KheMonitorIndex InSoln(m1) - KheMonitorIndex InSoln(m2);
  else
    return KheMonitorIndex InSoln(m2) - KheMonitorIndex InSoln(m1);
}

void KheGroupMonitorDefectSort(KHE_GROUP_MONITOR gm, bool diversify)
{
  int i;  KHE_MONITOR_LINK link;

  ** make sure soln's cost is up to date (IMPORTANT - nasty bug fix!) **
  KheSolnCost(gm->soln);
  MArraySort(gm->defect_links, diversify ?
    KheMonitorLinkDecreasingCostDiversifyCmp : KheMonitorLinkDecreasingCostCmp);
  HaArrayForEach(gm->defect_links, link, i)
    link->parent_defects_index = i;
  if( DEBUG3 )
    for( i = 1;  i < HaArrayCount(gm->defect_links);  i++ )
      HnAssert(HaArray(gm->defect_links,i) != HaArray(gm->defect_links,i-1),
	"KheGroupMonitorDefectSort internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorCopyDefects(KHE_GROUP_MONITOR gm)                    */
/*                                                                           */
/*  Initialize the copied defect list of gm.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheGroupMonitorCopyDefects(KHE_GROUP_MONITOR gm)
{
  int i;
  MArrayClear(gm->defect_links_copy);
  MArrayAppend(gm->defect_links_copy, gm->defect_links, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorDefectCopyCount(KHE_GROUP_MONITOR gm)                 */
/*                                                                           */
/*  Return the number of elements on gm's copied defect list.                */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheGroupMonitorDefectCopyCount(KHE_GROUP_MONITOR gm)
{
  return HaArrayCount(gm->defect_links_copy);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheGroupMonitorDefectCopy(KHE_GROUP_MONITOR gm, int i)       */
/*                                                                           */
/*  Return the i'th element of gm's copied defect list.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MONITOR KheGroupMonitorDefectCopy(KHE_GROUP_MONITOR gm, int i)
{
  KHE_MONITOR_LINK link;
  link = HaArray(gm->defect_links_copy, i);
  return link->child;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheGroupMonitorCostByType(KHE_GROUP_MONITOR gm,                 */
/*    KHE_MONITOR_TAG tag, int *defect_count)                                */
/*                                                                           */
/*  Find the cost of monitors of type tag in the subtree rooted at gm,       */
/*  and the number of defects.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheGroupMonitorCostByType(KHE_GROUP_MONITOR gm, KHE_MONITOR_TAG tag,
  int *defect_count)
{
  KHE_COST res;  int i, dc;  KHE_MONITOR_LINK link;  KHE_MONITOR m;
  KheGroupMonitorCheck(gm, NULL, 0, 0);
  res = 0;  *defect_count = 0;
  if( tag == KHE_GROUP_MONITOR_TAG )
    return res;
  KheSolnMatchingUpdate(gm->soln);
  HaArrayForEach(gm->defect_links, link, i)
  {
    m = link->child;
    if( KheMonitorTag(m) == tag )
    {
      res += KheMonitorCost(m);
      *defect_count += 1;
    }
    else if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
    {
      res += KheGroupMonitorCostByType((KHE_GROUP_MONITOR) m, tag, &dc);
      *defect_count += dc;
    }
  }
  KheGroupMonitorCheck(gm, NULL, 0, 0);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorCostByTypeDebug(KHE_GROUP_MONITOR gm,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of gm's cost onto fp, although not if gm's soln is a         */
/*  placeholder.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorCostByTypeDebug(KHE_GROUP_MONITOR gm,
  int verbosity, int indent, FILE *fp)
{
  int tag, defect_count, total_defect_count;  KHE_COST cost, total_cost;
  if(verbosity >= 1 && indent >= 0 && KheSolnType(gm->soln)==KHE_SOLN_ORDINARY)
  {
    KheGroupMonitorCheck(gm, NULL, 0, 0);
    fprintf(fp, "%*s%-31s %9s %13s\n", indent, "", gm->sub_tag_label,
      "Defects", "Cost");
    fprintf(fp, "%*s-------------------------------------------------------\n",
      indent, "");
    total_cost = 0;  total_defect_count = 0;
    for( tag = 0;  tag < KHE_MONITOR_TAG_COUNT;  tag++ )
    {
      cost = KheGroupMonitorCostByType(gm, tag, &defect_count);
      if( cost != 0 || defect_count != 0 )
	fprintf(fp, "%*s%-34s %6d %13.5f\n", indent, "", KheMonitorTagShow(tag),
	  defect_count, KheCostShow(cost));
      total_cost += cost;
      total_defect_count += defect_count;
    }
    fprintf(fp, "%*s-------------------------------------------------------\n",
      indent, "");
    fprintf(fp, "%*s%-34s %6d %13.5f\n", indent, "", "Total",
      total_defect_count, KheCostShow(total_cost));
    HnAssert(total_cost == gm->cost, "KheGroupMonitorCostByTypeDebug "
      "internal error (total_cost %.5f, gm->cost %.5f)\n",
      KheCostShow(total_cost), KheCostShow(gm->cost));
    KheGroupMonitorCheck(gm, NULL, 0, 0);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAttach(KHE_GROUP_MONITOR gm)                         */
/*                                                                           */
/*  Attach gm.   It is known to be currently detached with cost 0.           */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used with group monitors
void KheGroupMonitorAttach(KHE_GROUP_MONITOR gm)
{
  HnAssert(gm->cost == 0, "KheGroupMonitorAttach internal error");
  gm->attached = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDetach(KHE_GROUP_MONITOR gm)                         */
/*                                                                           */
/*  Detach gm.  It is known to be currently attached.                        */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used with group monitors
void KheGroupMonitorDetach(KHE_GROUP_MONITOR gm)
{
  int i;  KHE_MONITOR m;
  HaArrayForEach(gm->child_monitors, m, i)
    if( KheMonitorAttached(m) )
      KheMonitorDetach(m);
  gm->attached = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "update and tracing"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorBeginTrace(KHE_GROUP_MONITOR gm, KHE_TRACE t)        */
/*                                                                           */
/*  Begin tracing t.                                                         */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorBeginTrace(KHE_GROUP_MONITOR gm, KHE_TRACE t)
{
  HaArrayAddLast(gm->traces, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorEndTrace(KHE_GROUP_MONITOR gm, KHE_TRACE t)          */
/*                                                                           */
/*  End tracing t.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorEndTrace(KHE_GROUP_MONITOR gm, KHE_TRACE t)
{
  int pos;
  if( !HaArrayContains(gm->traces, t, &pos) )
    HnAbort("KheGroupMonitorEndTrace internal error");
  HaArrayDeleteAndShift(gm->traces, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorChangeCost(KHE_GROUP_MONITOR gm, KHE_MONITOR m,      */
/*    KHE_COST old_cost, KHE_COST new_cost)                                  */
/*                                                                           */
/*  Let gm know that its child monitor m (with the given link) has changed   */
/*  its cost from old_cost to new_cost.                                      */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorChangeCostInternal(KHE_GROUP_MONITOR gm, KHE_MONITOR m,
  KHE_MONITOR_LINK link, KHE_COST old_cost, KHE_COST new_cost, bool check)
{
  KHE_TRACE t;  int i;  KHE_COST delta_cost;  KHE_MONITOR_LINK link2;
  KHE_GROUP_MONITOR prnt;
  if( DEBUG2 )
  {
    /* important: can't call KheMonitorCost in this function! */
    fprintf(stderr,
      "[ KheGroupMonitorChangeCost(gm(%.5f), m, %.5f, %.5f)\n",
      KheCostShow((gm->cost)), KheCostShow(old_cost), KheCostShow(new_cost));
  }
  HnAssert(new_cost != old_cost, "KheGroupMonitorChangeCost internal error 1");
  if( DEBUG3 )
  {
    if( HaArray(gm->child_links, link->parent_index) != link )
    {
      fprintf(stderr, "KheGroupMonitorChangeCost %.5f -> %.5f failing, gm =\n",
        KheCostShow(old_cost), KheCostShow(new_cost));
      KheGroupMonitorDebug(gm, 3, 2, stderr);
      KheMonitorDebug(m, 3, 2, stderr);
      fprintf(stderr, "  m's parents:\n");
      for( i = 0;  i < KheMonitorParentMonitorCount(m);  i++ )
      {
	prnt = KheMonitorParentMonitor(m, i);
	KheGroupMonitorDebug(prnt, 3, 4, stderr);
      }
      fprintf(stderr, "  link->parent_index = %d\n", link->parent_index);
    }
    HnAssert(HaArray(gm->child_links, link->parent_index) == link,
      "KheGroupMonitorChangeCost internal error 2");
    if( old_cost > 0 )
    {
      HnAssert(link->parent_defects_index >= 0 &&
        HaArray(gm->defect_links, link->parent_defects_index) == link,
        "KheGroupMonitorChangeCost internal error 3");
    }
    HnAssert(new_cost >= 0, "KheGroupMonitorChangeCost internal error 4");
  }
  HaArrayForEach(gm->traces, t, i)
    KheTraceChangeCost(t, m, old_cost);
  delta_cost = new_cost - old_cost;
  gm->cost += delta_cost;
  if( old_cost == 0 )
    KheGroupMonitorAddDefect(gm, m, link);
  else if( new_cost == 0 )
    KheGroupMonitorDeleteDefect(gm, m, link);
  HaArrayForEach(gm->parent_links, link2, i)
    KheGroupMonitorChangeCost(link2->parent, (KHE_MONITOR) gm, link2,
      gm->cost - delta_cost, gm->cost);
  if( check )
    KheGroupMonitorCheck(gm, m, old_cost, new_cost);
  if( DEBUG2 )
    fprintf(stderr, "] KheGroupMonitorChangeCost returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorChangeCost(KHE_GROUP_MONITOR gm, KHE_MONITOR m,      */
/*    KHE_COST old_cost, KHE_COST new_cost)                                  */
/*                                                                           */
/*  Let gm know that its child monitor m (with the given link) has changed   */
/*  its cost from old_cost to new_cost.                                      */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorChangeCost(KHE_GROUP_MONITOR gm, KHE_MONITOR m,
  KHE_MONITOR_LINK link, KHE_COST old_cost, KHE_COST new_cost)
{
  KheGroupMonitorChangeCostInternal(gm, m, link, old_cost, new_cost, true);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorDeviationCount(KHE_GROUP_MONITOR m)                   */
/*                                                                           */
/*  Return the deviations of m (0 in this case).                             */
/*                                                                           */
/*****************************************************************************/

int KheGroupMonitorDeviationCount(KHE_GROUP_MONITOR m)
{
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupMonitorDeviation(KHE_GROUP_MONITOR m, int i)                 */
/*                                                                           */
/*  Return the i'th deviation of m.  There are none it's an error.           */
/*                                                                           */
/*****************************************************************************/

int KheGroupMonitorDeviation(KHE_GROUP_MONITOR m, int i)
{
  HnAbort("KheGroupMonitorDeviation: i out of range");
  return 0;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheGroupMonitorDeviationDescription(KHE_GROUP_MONITOR m, int i)    */
/*                                                                           */
/*  Return a description of the i'th deviation of m.  There are no           */
/*  deviations so it's an error.                                             */
/*                                                                           */
/*****************************************************************************/

char *KheGroupMonitorDeviationDescription(KHE_GROUP_MONITOR m, int i)
{
  HnAbort("KheGroupMonitorDeviationDescription: i out of range");
  return NULL;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDefectDebug(KHE_GROUP_MONITOR gm,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of gm, showing only the defective child monitors.            */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDefectDebug(KHE_GROUP_MONITOR gm,
  int verbosity, int indent, FILE *fp)
{
  int i, defects;  KHE_MONITOR m;  KHE_MONITOR_LINK link;
  if( verbosity >= 1 )
  {
    KheSolnMatchingUpdate(gm->soln);
    KheMonitorDebugWithTagBegin((KHE_MONITOR) gm,
      gm->sub_tag_label != NULL ? gm->sub_tag_label : "GroupMonitor",
      indent, fp);
    defects = HaArrayCount(gm->defect_links);
    fprintf(fp, " (sub_tag %d) %d %s", gm->sub_tag, defects,
      defects == 1 ? "defect" : "defects");
    if( indent >= 0 && verbosity >= 2 && defects > 0 )
    {
      fprintf(fp, "\n");
      HaArrayForEach(gm->defect_links, link, i)
      {
	m = link->child;
	if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
	  KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) m,
	    verbosity, indent + 2, fp);
	else
	  KheMonitorDebug(m, verbosity, indent + 2, fp);
      }
      KheMonitorDebugEnd((KHE_MONITOR) gm, false, indent, fp);
    }
    else
      KheMonitorDebugEnd((KHE_MONITOR) gm, true, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDefectTypeDebug(KHE_GROUP_MONITOR gm,                */
/*    KHE_MONITOR_TAG tag, int verbosity, int indent, FILE *fp)              */
/*                                                                           */
/*  Debug print of gm, showing only the defective child monitors with the    */
/*  given tag.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDefectTypeDebug(KHE_GROUP_MONITOR gm,
  KHE_MONITOR_TAG tag, int verbosity, int indent, FILE *fp)
{
  int i;  KHE_MONITOR m;  KHE_MONITOR_LINK link;
  if( verbosity >= 1 )
  {
    /* fprintf(fp, "%*s[ %s defects:\n", indent, "", KheMonitorTagShow(tag)); */
    HaArrayForEach(gm->defect_links, link, i)
    {
      m = link->child;
      if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
	KheGroupMonitorDefectTypeDebug((KHE_GROUP_MONITOR) m, tag,
	  verbosity, indent, fp);
      else if( KheMonitorTag(m) == tag )
	KheMonitorDebug(m, verbosity, indent, fp);
    }
    /* fprintf(fp, "%*s]\n", indent, ""); */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorDebug(KHE_GROUP_MONITOR gm,                          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheGroupMonitorDebug(KHE_GROUP_MONITOR gm, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_MONITOR_LINK link;  KHE_MONITOR m;  KHE_COST total_cost;
  if( verbosity >= 1 )
  {
    KheMonitorDebugWithTagBegin((KHE_MONITOR) gm,
      gm->sub_tag_label != NULL ? gm->sub_tag_label : "GroupMonitor",
      indent, fp);
    fprintf(fp, " (sub_tag %d) %d %s %p", gm->sub_tag,
      HaArrayCount(gm->child_links),
      HaArrayCount(gm->child_links) == 1 ? "child" : "children", (void *) gm);
    if( indent >= 0 && verbosity >= 2 && HaArrayCount(gm->child_links) > 0 )
    {
      fprintf(fp, "\n");
      total_cost = 0;
      HaArrayForEach(gm->child_links, link, i)
      {
	m = link->child;
        KheMonitorDebug(m, verbosity, indent + 2, fp);
	total_cost += KheMonitorCost(m);
      }
      KheMonitorDebugEnd((KHE_MONITOR) gm, false, indent, fp);
      HnAssert(gm->cost == total_cost,
	"KheGroupMonitorDebug: cost (%.5f) != total child cost (%.5f)",
        gm->cost, total_cost);
    }
    else
      KheMonitorDebugEnd((KHE_MONITOR) gm, true, indent, fp);
  }
}

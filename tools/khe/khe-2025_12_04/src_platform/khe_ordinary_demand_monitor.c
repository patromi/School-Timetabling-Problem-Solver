
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
/*  FILE:         khe_ordinary_demand_monitor.c                              */
/*  DESCRIPTION:  An ordinary demand monitor                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG1_INDEX 6910
#define DEBUG_ME(m)  (DEBUG1 && (m)->soln_index == DEBUG1_INDEX)

#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR                                              */
/*                                                                           */
/*****************************************************************************/

struct khe_ordinary_demand_monitor_rec {
  INHERIT_MATCHING_DEMAND_NODE
  KHE_TASK			task;
  int				offset;
  KHE_ORDINARY_DEMAND_MONITOR	copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorMake(KHE_SOLN soln,  */
/*    KHE_MATCHING_DEMAND_CHUNK dc, KHE_TASK task, int offset)               */
/*                                                                           */
/*  Make an ordinary demand monitor with these attributes.                   */
/*  Because it is unattached initially, its domain does not matter.          */
/*                                                                           */
/*****************************************************************************/

KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorMake(KHE_TASK task,
  int offset, KHE_MONITOR orig_m)
{
  KHE_ORDINARY_DEMAND_MONITOR res;  HA_ARENA a;  KHE_SOLN soln;
  soln = KheTaskSoln(task);
  res = KheSolnGetOrdinaryDemandMonitorFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->parent_links, a);
  }
  else
    HaArrayClear(res->parent_links);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_ORDINARY_DEMAND_MONITOR_TAG, KHE_LINEAR_COST_FUNCTION, KheCost(1, 0));
  res->demand_chunk = KheTaskDemandChunk(task, offset);
  res->domain = *KheSolnMatchingZeroDomain(soln);
  res->demand_asst = NULL;
  res->demand_asst_index = NO_PREV_ASST;
  res->unmatched_pos = -1;  /* initially not in unmatched list */
  res->bfs_next = NULL;
  res->bfs_parent = NULL;
  res->hall_set = NULL;
  res->task = task;
  res->offset = offset;
  res->copy = NULL;
  KheTaskAddDemandMonitor(task, res);
  if( DEBUG_ME(res) )
  {
    fprintf(stderr, "  Make: ");
    KheMonitorDebug((KHE_MONITOR) res, 1, 2, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorMake(KHE_SOLN soln,  */
/*    KHE_MATCHING_DEMAND_CHUNK dc, KHE_TASK task, int offset)               */
/*                                                                           */
/*  Make an ordinary demand monitor with these attributes.                   */
/*  Because it is unattached initially, its domain does not matter.          */
/*                                                                           */
/*****************************************************************************/

/* *** old version
KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorMake(KHE_SOLN soln,
  KHE_MATCHING_DEMAND_CHUNK dc, KHE_TASK task, int offset)
{
  KHE_ORDINARY_DEMAND_MONITOR res;  HA_ARENA a;
  ** *** silly
  HnAssert(KheMatchingImpl(KheMatchingDemandChunkMatching(dc)) == (void *) soln,
    "KheOrdinaryDemandMonitorMake internal error (%p != %p)",
    KheMatchingImpl(KheMatchingDemandChunkMatching(dc)), (void *) soln);
  *** **
  res = KheSolnGetOrdinaryDemandMonitorFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->parent_links, a);
  }
  else
    HaArrayClear(res->parent_links);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_ORDINARY_DEMAND_MONITOR_TAG);
  res->demand_chunk = dc;
  res->domain = *KheSolnMatchingZeroDomain(soln);
  res->demand_asst = NULL;
  res->demand_asst_index = NO_PREV_ASST;
  res->unmatc hed_pos = -1;  ** undefined at this point **
  res->bfs_next = NULL;
  res->bfs_parent = NULL;
  res->hall_set = NULL;
  res->task = task;
  res->offset = offset;
  res->copy = NULL;
  ** KheGroupMonitorAddMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) res); **
  KheTaskAddDemandMonitor(task, res);
  ** KheMonitorCheck((KHE_MONITOR) res); **
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheOrdinaryDemandMonitorTask(KHE_ORDINARY_DEMAND_MONITOR m)     */
/*                                                                           */
/*  Return the task that m monitors.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheOrdinaryDemandMonitorTask(KHE_ORDINARY_DEMAND_MONITOR m)
{
  return m->task;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOrdinaryDemandMonitorOffset(KHE_ORDINARY_DEMAND_MONITOR m)        */
/*                                                                           */
/*  Return the offset of m in its task.                                      */
/*                                                                           */
/*****************************************************************************/

int KheOrdinaryDemandMonitorOffset(KHE_ORDINARY_DEMAND_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorSetTaskAndOffset(                           */
/*    KHE_ORDINARY_DEMAND_MONITOR m, KHE_TASK task, int offset)              */
/*                                                                           */
/*  Set the task and offset attributes of m.                                 */
/*  This function is called only when splitting and merging solution         */
/*  resources, so the domain and matching do not change.                     */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorSetTaskAndOffset(KHE_ORDINARY_DEMAND_MONITOR m,
  KHE_TASK task, int offset)
{
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  SetTaskAndOffset(%s, %d): ", KheTaskId(task), offset);
    KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
  }
  m->task = task;
  m->offset = offset;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorCopyPhase1(          */
/*    KHE_ORDINARY_DEMAND_MONITOR m, HA_ARENA a)                             */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorCopyPhase1(
  KHE_ORDINARY_DEMAND_MONITOR m, HA_ARENA a)
{
  KHE_ORDINARY_DEMAND_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->demand_chunk = KheMatchingDemandChunkCopyPhase1(m->demand_chunk, a);
    KheSetCopy(copy->domain, m->domain, a);
    copy->demand_asst = (m->demand_asst == NULL ? NULL :
      KheMatchingSupplyNodeCopyPhase1(m->demand_asst, a));
    copy->demand_asst_index = m->demand_asst_index;
    copy->unmatched_pos = m->unmatched_pos;
    copy->bfs_next = (m->bfs_next == NULL ? NULL :
      KheMatchingDemandNodeCopyPhase1(m->bfs_next, a));
    copy->bfs_parent = (m->bfs_parent == NULL ? NULL :
      KheMatchingDemandNodeCopyPhase1(m->bfs_parent, a));
    copy->hall_set = (m->hall_set == NULL ? NULL :
      KheMatchingHallSetCopyPhase1(m->hall_set, a));
    copy->task = KheTaskCopyPhase1(m->task, a);
    copy->offset = m->offset;
    copy->copy = NULL;
    if( DEBUG_ME(m) )
    {
      fprintf(stderr, "  Copy: ");
      KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
    }
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorCopyPhase2(KHE_ORDINARY_DEMAND_MONITOR m)   */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorCopyPhase2(KHE_ORDINARY_DEMAND_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheMatchingDemandChunkCopyPhase2(m->demand_chunk);
    if( m->demand_asst != NULL )
      KheMatchingSupplyNodeCopyPhase2(m->demand_asst);
    if( m->hall_set != NULL )
      KheMatchingHallSetCopyPhase2(m->hall_set);
    KheTaskCopyPhase2(m->task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorDelete(KHE_ORDINARY_DEMAND_MONITOR m)       */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorDelete(KHE_ORDINARY_DEMAND_MONITOR m)
{
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  Delete: ");
    KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
  }
  if( m->attached )
    KheOrdinaryDemandMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheTaskDeleteDemandMonitor(m->task, m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  KheSolnAddOrdinaryDemandMonitorToFreeList(m->soln, m);
  /* MFree(m); */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0.                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorAttachToSoln(KHE_ORDINARY_DEMAND_MONITOR m) */
/*                                                                           */
/*  Attach m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorAttachToSoln(KHE_ORDINARY_DEMAND_MONITOR m)
{
  /* KheMonitorCheck((KHE_MONITOR) m); */
  KheTaskAttachDemandMonitor(m->task, m);
  /* KheMonitorCheck((KHE_MONITOR) m); */
  m->domain = *KheResourceGroupKheSet(KheTaskMatchingDomain(m->task));
  KheMatchingDemandNodeAdd((KHE_MATCHING_DEMAND_NODE) m);
  m->attached = true;
  /* KheMonitorCheck((KHE_MONITOR) m); */
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  Attach: ");
    KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/* void KheOrdinaryDemandMonitorDetachFromSoln(KHE_ORDINARY_DEMAND_MONITOR m)*/
/*                                                                           */
/*  Detach m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorDetachFromSoln(KHE_ORDINARY_DEMAND_MONITOR m)
{
  KheTaskDetachDemandMonitor(m->task, m);
  KheMatchingDemandNodeDelete((KHE_MATCHING_DEMAND_NODE) m);
  m->attached = false;
  /* KheMonitorCheck((KHE_MONITOR) m); */
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  Detach: ");
    KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorAttachCheck(KHE_ORDINARY_DEMAND_MONITOR m)  */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOrdinaryDemandMonitorAttachCheck(KHE_ORDINARY_DEMAND_MONITOR m)
{
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
    KheMonitorAttachToSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorSetDomain(KHE_ORDINARY_DEMAND_MONITOR m,    */
/*    KHE_RESOURCE_GROUP rg, KHE_MATCHING_DOMAIN_CHANGE_TYPE change_type)    */
/*                                                                           */
/*  Set the domain of m to the resource group indexes of rg.                 */
/*                                                                           */
/*  This function is called when assigning, unassigning, and changing        */
/*  the domain and matching type; but only on attached monitors.             */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorSetDomain(KHE_ORDINARY_DEMAND_MONITOR m,
  KHE_RESOURCE_GROUP rg, KHE_MATCHING_DOMAIN_CHANGE_TYPE change_type)
{
  HnAssert(m->attached, "KheOrdinaryDemandMonitorSetDomain internal error");
  KheMatchingDemandNodeSetDomain((KHE_MATCHING_DEMAND_NODE) m,
    /* KheResourceGroupResourceIndexes(rg), */
    KheResourceGroupKheSet(rg), change_type);
  /* KheMonitorCheck((KHE_MONITOR) m); */
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  SetDomain ");
    /* ***
    KheResourceGroupDebug(rg, 2, -1, stderr);
    fprintf(stderr, ": ");
    *** */
    KheMonitorDebug((KHE_MONITOR) m, 1, 0, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorSetWeight(KHE_ORDINARY_DEMAND_MONITOR m,    */
/*    KHE_COST new_weight)                                                   */
/*                                                                           */
/*  Set the weight of m to new_weight.  It is known to be attached.          */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorSetWeight(KHE_ORDINARY_DEMAND_MONITOR m,
  KHE_COST new_weight)
{
  HnAssert(m->attached, "KheOrdinaryDemandMonitorSetWeight internal error");
  if( m->demand_asst == NULL )
    KheMonitorChangeCost((KHE_MONITOR) m, new_weight);
  /* KheMonitorCheck((KHE_MONITOR) m); */
  if( DEBUG_ME(m) )
  {
    fprintf(stderr, "  Weight: ");
    KheMonitorDebug((KHE_MONITOR) m, 1, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorCheckConsistency(                           */
/*    KHE_ORDINARY_DEMAND_MONITOR m, KHE_RESOURCE_GROUP rg)                  */
/*                                                                           */
/*  Check the consistency of m's domain with rg.                             */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorCheckConsistency(
  KHE_ORDINARY_DEMAND_MONITOR m, KHE_RESOURCE_GROUP rg)
{
  KHE_SET rg_set;  KHE_TASK task;  char buff1[200], buff2[200];
  if( DEBUG2 )
  {
    rg_set = *KheResourceGroupKheSet(rg);
    if( !KheSetEqual(rg_set, m->domain) )
    {
      fprintf(stderr, "KheOrdinaryDemandMonitorCheckConsistency"
	" internal error: ");
      for( task = m->task;  task != NULL;  task = KheTaskAsst(task) )
      {
	if( task != m->task )
	  fprintf(stderr, " -> ");
	fprintf(stderr, "%s", KheTaskId(task));
      }
      HnAbort(" %s !=  m %s", KheSetShow(rg_set, buff1),
	KheSetShow(m->domain, buff2));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorChangeType(KHE_ORDINARY_DEMAND_MONITOR m,   */
/*    KHE_MATCHING_TYPE old_mt, KHE_MATCHING_TYPE new_mt)                    */
/*                                                                           */
/*  Change the type of m.  It is known to be attached.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOrdinaryDemandMonitorChangeType(KHE_ORDINARY_DEMAND_MONITOR m,
  KHE_MATCHING_TYPE old_mt, KHE_MATCHING_TYPE new_mt)
{
  KheMatchingDeman dNodeSetDomain((KHE_MATCHING_DEMAND_NODE) m,
    KheResourceGroupResourceIndexes(KheTaskDomain(m->task)),
    KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheOrdinaryDemandMonitorDeviation(KHE_ORDINARY_DEMAND_MONITOR m)     */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheOrdinaryDemandMonitorDeviation(KHE_ORDINARY_DEMAND_MONITOR m)
{
  return m->demand_asst == NULL ? 1 : 0;
  /* return m->demand_asst != NULL ? 1 : 0; swapped around 18/12/23 JeffK */
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrdinaryDemandMonitorDeviationDescription(                      */
/*    KHE_ORDINARY_DEMAND_MONITOR m)                                         */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheOrdinaryDemandMonitorDeviationDescription(
  KHE_ORDINARY_DEMAND_MONITOR m)
{
  return m->demand_asst == NULL ? "1" : "0";
  /* return m->demand_asst != NULL ? "1" : "0"; swapped around 18/12/23 JeffK */
  /* ***
  ARRAY_CHAR ac;
  MStringInit(ac);
  if( m->demand_asst != NULL )
    MStringAddString(ac, "1");
  else
    MStringAddString(ac, "0");
  return MStringVal(ac);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOrdinaryDemandMonitorDeviationCount(                              */
/*    KHE_ORDINARY_DEMAND_MONITOR m)                                         */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheOrdinaryDemandMonitorDeviationCount(KHE_ORDINARY_DEMAND_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheOrdinaryDemandMonitorDeviation(KHE_ORDINARY_DEMAND_MONITOR m,     */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheOrdinaryDemandMonitorDeviation(KHE_ORDINARY_DEMAND_MONITOR m, int i)
{
  HnAssert(i == 0, "KheOrdinaryDemandMonitorDeviation: i out of range");
  return m->demand_asst != NULL ? 1 : 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrdinaryDemandMonitorDeviationDescription(                      */
/*    KHE_ORDINARY_DEMAND_MONITOR m, int i)                                  */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheOrdinaryDemandMonitorDeviationDescription(
  KHE_ORDINARY_DEMAND_MONITOR m, int i)
{
  HnAssert(i == 0,
    "KheOrdinaryDemandMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheOrdinaryDemandMonitorDebug(KHE_ORDINARY_DEMAND_MONITOR m,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheOrdinaryDemandMonitorDebug(KHE_ORDINARY_DEMAND_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  char buff[200];
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " %p ", (void *) m);
    KheTaskDebug(m->task, 1, -1, fp);
    fprintf(fp, "+%d: ", m->offset);
    KheResourceGroupDebug(KheTaskDomain(m->task), 1, -1, fp);
    fprintf(fp, " : %s", KheSetShow(m->domain, buff));
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

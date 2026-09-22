
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
/*  FILE:         khe_sr_invariant.c                                         */
/*  DESCRIPTION:  The resource assignment invariant                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

/*****************************************************************************/
/*                                                                           */
/*  Submodule "preserving the resource assignment invariant"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicOperationBegin(KHE_MARK *mark, int *init_count,            */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Begin an atomic operation.                                               */
/*                                                                           */
/*****************************************************************************/

void KheAtomicOperationBegin(KHE_SOLN soln, KHE_MARK *mark,
  int *init_count, bool resource_invariant)
{
  *mark = KheMarkBegin(soln);
  *init_count = KheSolnMatchingDefectCount(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicOperationEnd(KHE_MARK *mark, int *init_count,              */
/*    bool resource_invariant, bool success)                                 */
/*                                                                           */
/*  End an atomic operation, returning true if successful.                   */
/*                                                                           */
/*****************************************************************************/

bool KheAtomicOperationEnd(KHE_SOLN soln, KHE_MARK *mark,
  int *init_count, bool resource_invariant, bool success)
{
  if( resource_invariant && KheSolnMatchingDefectCount(soln) > *init_count )
    success = false;
  KheMarkEnd(*mark, !success);
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInvariantTransactionBegin(KHE_TRANSACTION t, int *init_count,    */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Begin a transaction that has to not increase the matching cost if        */
/*  resource_invariant is true.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheInvariantTransactionBegin(KHE_TRANSACTION t, int *init_count,
  bool resource_invariant)
{
  KHE_SOLN soln;
  if( resource_invariant )
  {
    soln = KheTransactionSoln(t);
    *init_count = KheSolnMatchingDefectCount(soln);
    KheSolnMatchingMarkBegin(soln);
    KheTransactionBegin(t);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheInvariantTransactionEnd(KHE_TRANSACTION t,                       */
/*    int *init_count, bool resource_invariant, bool success)                */
/*                                                                           */
/*  End a transaction that has to not increase the matching cost if          */
/*  resource_invariant is true.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheInvariantTransactionEnd(KHE_TRANSACTION t,
  int *init_count, bool resource_invariant, bool success)
{
  KHE_SOLN soln;
  if( resource_invariant )
  {
    soln = KheTransactionSoln(t);
    KheTransactionEnd(t);
    if( KheSolnMatchingDefectCount(soln) > *init_count )
      success = false;
    if( !success )
      KheTransactionUndo(t);
    KheSolnMatchingMarkEnd(soln, !success);
  }
  return success;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTransactionBegin(KHE_TRANSACTION t, int *init_count,       */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Begin an atomic transaction.                                             */
/*                                                                           */
/*****************************************************************************/

/* *** old version
void KheAtomicTransactionBegin(KHE_TRANSACTION t, int *init_count,
  bool resource_invariant)
{
  KHE_SOLN soln;
  soln = KheTransactionSoln(t);
  *init_count = KheSolnMatchingDefectCount(soln);
  KheSolnMatchingMarkBegin(soln);
  KheTransactionBegin(t);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicTransactionEnd(KHE_TRANSACTION t,                          */
/*    int *init_count, bool resource_invariant, bool success)                */
/*                                                                           */
/*  End an atomic transaction, returning true if successful.                 */
/*                                                                           */
/*****************************************************************************/

/* *** old version
bool KheAtomicTransactionEnd(KHE_TRANSACTION t,
  int *init_count, bool resource_invariant, bool success)
{
  KHE_SOLN soln;
  soln = KheTransactionSoln(t);
  KheTransactionEnd(t);
  if( resource_invariant && KheSolnMatchingDefectCount(soln) > *init_count )
    success = false;
  if( !success )
    KheTransactionUndo(t);
  KheSolnMatchingMarkEnd(soln, !success);
  return success;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDisconnectAllDemandMonitors(KHE_SOLN soln, KHE_RESOURCE_TYPE rt) */
/*                                                                           */
/*  Disconnect all demand monitors (or all demand monitors of type rt)       */
/*  from all their parents.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheDisconnectAllDemandMonitors(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  KHE_RESOURCE r;  KHE_INSTANCE ins;  KHE_GROUP_MONITOR gm;
  KHE_MONITOR m;  int i, j, k;  KHE_MEET meet;  KHE_TASK task;

  /* ordinary demand nodes */
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
    {
      task = KheMeetTask(meet, j);
      if( rt == NULL || KheTaskResourceType(task) == rt )
	for( k = 0;  k < KheTaskDemandMonitorCount(task);  k++ )
	{
	  m = (KHE_MONITOR) KheTaskDemandMonitor(task, k);
	  while( KheMonitorParentMonitorCount(m) > 0 )
	  {
	    gm = KheMonitorParentMonitor(m, 0);
	    KheGroupMonitorDeleteChildMonitor(gm, m);
	  }
	}
    }
  }

  /* workload demand nodes */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    if( rt == NULL || KheResourceResourceType(r) == rt )
      for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
      {
	m = KheSolnResourceMonitor(soln, r, j);
	if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
	  while( KheMonitorParentMonitorCount(m) > 0 )
	  {
	    gm = KheMonitorParentMonitor(m, 0);
	    KheGroupMonitorDeleteChildMonitor(gm, m);
	  }
      }
  }
}

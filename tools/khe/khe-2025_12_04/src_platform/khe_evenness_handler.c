
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
/*  FILE:         khe_evenness_handler.c                                     */
/*  DESCRIPTION:  An evenness handler                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION_HANDLER - partition handler (private)                      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENNESS_MONITOR) ARRAY_KHE_EVENNESS_MONITOR;

typedef struct khe_partition_handler_rec {
  KHE_RESOURCE_GROUP		partition;		/* partition         */
  ARRAY_KHE_EVENNESS_MONITOR	monitors;		/* evenness monitors */
} *KHE_PARTITION_HANDLER;

typedef HA_ARRAY(KHE_PARTITION_HANDLER) ARRAY_KHE_PARTITION_HANDLER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_HANDLER                                                     */
/*                                                                           */
/*****************************************************************************/

struct khe_evenness_handler_rec {
  KHE_SOLN			soln;			/* encl. solution    */
  ARRAY_KHE_PARTITION_HANDLER	partition_handlers;	/* one per partition */
  int				attached_count;		/* number attached   */
  KHE_EVENNESS_HANDLER		copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "partition handlers" (private)                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION_HANDLER KhePartitionHandlerMake(KHE_SOLN soln,             */
/*    KHE_RESOURCE_GROUP partition)                                          */
/*                                                                           */
/*  Make a new partition handler and monitors.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_PARTITION_HANDLER KhePartitionHandlerMake(KHE_SOLN soln,
  KHE_RESOURCE_GROUP partition)
{
  KHE_PARTITION_HANDLER res;  KHE_INSTANCE ins;  int i;
  KHE_EVENNESS_MONITOR m;  HA_ARENA a;
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->partition = partition;
  HaArrayInit(res->monitors, a);
  ins = KheResourceGroupInstance(partition);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    m = KheEvennessMonitorMake(soln, partition, KheInstanceTime(ins, i));
    HaArrayAddLast(res->monitors, m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionHandlerCopy(KHE_PARTITION_HANDLER ph)                   */
/*                                                                           */
/*  Return a copy of ph.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_PARTITION_HANDLER KhePartitionHandlerCopy(KHE_PARTITION_HANDLER ph,
  HA_ARENA a)
{
  KHE_PARTITION_HANDLER copy;  KHE_EVENNESS_MONITOR m;  int i;
  HaMake(copy, a);
  copy->partition = ph->partition;
  HaArrayInit(copy->monitors, a);
  HaArrayForEach(ph->monitors, m, i)
    HaArrayAddLast(copy->monitors, KheEvennessMonitorCopyPhase1(m, a));
  return copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionHandlerDelete(KHE_PARTITION_HANDLER ph)                 */
/*                                                                           */
/*  Delete ph but not its monitors.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePartitionHandlerDelete(KHE_PARTITION_HANDLER ph)
{
  MArrayFree(ph->monitors);
  MFree(ph);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionHandlerDebug(KHE_PARTITION_HANDLER ph,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ph onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionHandlerDebug(KHE_PARTITION_HANDLER ph,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENNESS_MONITOR m;  int i;
  if( verbosity >= 0 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Partition Handler for ", indent, "");
    KheResourceGroupDebug(ph->partition, -1, 1, fp);
    fprintf(fp, "\n");
    HaArrayForEach(ph->monitors, m, i)
      KheEvennessMonitorDebug(m, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_HANDLER KheEvennessHandlerMake(KHE_SOLN soln)               */
/*                                                                           */
/*  Make a new evenness handler with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

KHE_EVENNESS_HANDLER KheEvennessHandlerMake(KHE_SOLN soln)
{
  KHE_EVENNESS_HANDLER res;  KHE_INSTANCE ins;  int i;  HA_ARENA a;
  ins = KheSolnInstance(soln);
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->soln = soln;
  HaArrayInit(res->partition_handlers, a);
  for( i = 0;  i < KheInstancePartitionCount(ins);  i++ )
    HaArrayAddLast(res->partition_handlers,
      KhePartitionHandlerMake(soln, KheInstancePartition(ins, i)));
  res->attached_count = 0;
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/* KHE_EVENNESS_HANDLER KheEvennessHandlerCopyPhase1(KHE_EVENNESS_HANDLER eh)*/
/*                                                                           */
/*  Carry out Phase 1 of copying eh.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENNESS_HANDLER KheEvennessHandlerCopyPhase1(KHE_EVENNESS_HANDLER eh,
  HA_ARENA a)
{
  KHE_EVENNESS_HANDLER copy;  KHE_PARTITION_HANDLER ph;  int i;
  if( eh->copy == NULL )
  {
    HaMake(copy, a);
    eh->copy = copy;
    copy->soln = KheSolnCopyPhase1(eh->soln);
    HaArrayInit(copy->partition_handlers, a);
    HaArrayForEach(eh->partition_handlers, ph, i)
      HaArrayAddLast(copy->partition_handlers, KhePartitionHandlerCopy(ph, a));
    copy->attached_count = eh->attached_count;
    copy->copy = NULL;
  }
  return eh->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerCopyPhase2(KHE_EVENNESS_HANDLER eh)               */
/*                                                                           */
/*  Carry out Phase 2 of copying eh.                                         */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerCopyPhase2(KHE_EVENNESS_HANDLER eh)
{
  if( eh->copy != NULL )
  {
    eh->copy = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerDelete(KHE_EVENNESS_HANDLER eh)                   */
/*                                                                           */
/*  Delete eh.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEvennessHandlerDelete(KHE_EVENNESS_HANDLER eh)
{
  while( HaArrayCount(eh->partition_handlers) > 0 )
    KhePartitionHandlerDelete(MArrayRemoveLast(eh->partition_handlers));
  MArrayFree(eh->partition_handlers);
  MFree(eh);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "configuration"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerAttachAllEvennessMonitors(KHE_EVENNESS_HANDLER eh)*/
/*                                                                           */
/*  Ensure that all evenness monitors are attached.                          */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerAttachAllEvennessMonitors(KHE_EVENNESS_HANDLER eh)
{
  KHE_PARTITION_HANDLER ph;  KHE_EVENNESS_MONITOR m;  int i, j;
  HaArrayForEach(eh->partition_handlers, ph, i)
    HaArrayForEach(ph->monitors, m, j)
      if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
	KheEvennessMonitorAttachToSoln(m);
  if( DEBUG1 )
  {
    fprintf(stderr, "  evenness handler after attaching all its monitors:\n");
    KheEvennessHandlerDebug(eh, 2, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerDetachAllEvennessMonitors(KHE_EVENNESS_HANDLER eh)*/
/*                                                                           */
/*  Ensure that all evenness monitors are detached.                          */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerDetachAllEvennessMonitors(KHE_EVENNESS_HANDLER eh)
{
  KHE_PARTITION_HANDLER ph;  KHE_EVENNESS_MONITOR m;  int i, j;
  if( DEBUG1 )
  {
    fprintf(stderr, "  evenness handler before detaching all its monitors:\n");
    KheEvennessHandlerDebug(eh, 2, 2, stderr);
  }
  HaArrayForEach(eh->partition_handlers, ph, i)
    HaArrayForEach(ph->monitors, m, j)
      if( KheMonitorAttachedToSoln((KHE_MONITOR) m) )
	KheEvennessMonitorDetachFromSoln(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerSetAllEvennessMonitorWeights(                     */
/*    KHE_EVENNESS_HANDLER eh, KHE_COST weight)                              */
/*                                                                           */
/*  Set the weights of all evenness monitors.                                */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerSetAllEvennessMonitorWeights(
  KHE_EVENNESS_HANDLER eh, KHE_COST weight)
{
  KHE_PARTITION_HANDLER ph;  KHE_EVENNESS_MONITOR m;  int i, j;
  HaArrayForEach(eh->partition_handlers, ph, i)
    HaArrayForEach(ph->monitors, m, j)
      KheEvennessMonitorSetWeight(m, weight);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerMonitorAttach(KHE_EVENNESS_HANDLER eh)            */
/*                                                                           */
/*  Record the fact that an evenness monitor has just attached itself.       */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerMonitorAttach(KHE_EVENNESS_HANDLER eh)
{
  eh->attached_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerMonitorDetach(KHE_EVENNESS_HANDLER eh)            */
/*                                                                           */
/*  Record the fact that an evenness monitor has just detached itself.       */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerMonitorDetach(KHE_EVENNESS_HANDLER eh)
{
  eh->attached_count--;
  HnAssert(eh->attached_count >= 0,
    "KheEvennessHandlerMonitorDetach internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerAddTask(KHE_EVENNESS_HANDLER eh,                  */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Inform eh that task has just been assigned this time index.              */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerAddTask(KHE_EVENNESS_HANDLER eh,
  KHE_TASK task, int assigned_time_index)
{
  KHE_PARTITION_HANDLER ph;  int durn, i;
  KHE_EVENNESS_MONITOR m;  KHE_RESOURCE_GROUP partition;
  partition = KheResourceGroupPartition(KheTaskDomain(task));
  if( eh->attached_count > 0 && partition != NULL )
  {
    ph = HaArray(eh->partition_handlers,
      KheResourceGroupPartitionIndex(partition));
    durn = KheTaskDuration(task);
    for( i = 0;  i < durn;  i++ )
    {
      m = HaArray(ph->monitors, assigned_time_index + i);
      KheEvennessMonitorAddTask(m);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerDeleteTask(KHE_EVENNESS_HANDLER eh,               */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Inform eh that task has just been unassigned from this time index.       */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerDeleteTask(KHE_EVENNESS_HANDLER eh,
  KHE_TASK task, int assigned_time_index)
{
  KHE_PARTITION_HANDLER ph;  int durn, i;
  KHE_EVENNESS_MONITOR m;  KHE_RESOURCE_GROUP partition;
  partition = KheResourceGroupPartition(KheTaskDomain(task));
  if( eh->attached_count > 0 && partition != NULL )
  {
    ph = HaArray(eh->partition_handlers,
      KheResourceGroupPartitionIndex(partition));
    durn = KheTaskDuration(task);
    for( i = 0;  i < durn;  i++ )
    {
      m = HaArray(ph->monitors, assigned_time_index + i);
      KheEvennessMonitorDeleteTask(m);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessHandlerDebug(KHE_EVENNESS_HANDLER eh,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of eh onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheEvennessHandlerDebug(KHE_EVENNESS_HANDLER eh,
  int verbosity, int indent, FILE *fp)
{
  KHE_PARTITION_HANDLER ph;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Evenness Handler (%d partitions, %d attached monitors)\n",
      indent, "", HaArrayCount(eh->partition_handlers), eh->attached_count);
    HaArrayForEach(eh->partition_handlers, ph, i)
      KhePartitionHandlerDebug(ph, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}

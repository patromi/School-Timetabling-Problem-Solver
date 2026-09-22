
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
/*  FILE:         khe_evenness_monitor.c                                     */
/*  DESCRIPTION:  An evenness monitor                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_MONITOR                                                     */
/*                                                                           */
/*****************************************************************************/

struct khe_evenness_monitor_rec {
  INHERIT_MONITOR(unused, limit)
  KHE_RESOURCE_GROUP		partition;		/* partition         */
  KHE_TIME			time;                   /* time              */
  int				count;                  /* count             */
  KHE_COST			weight;			/* weight            */
  KHE_EVENNESS_MONITOR		copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEvennessMonitorCost(KHE_EVENNESS_MONITOR m)                  */
/*                                                                           */
/*  Work out the cost of m, assuming m is attached.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheEvennessMonitorCost(KHE_EVENNESS_MONITOR m)
{
  return m->count > m->limit ? (m->count - m->limit) * m->weight : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_MONITOR KheEvennessMonitorMake(KHE_SOLN soln,               */
/*    KHE_RESOURCE_GROUP partition, KHE_TIME time)                           */
/*                                                                           */
/*  Make an evenness monitor with these attributes, initially unattached.    */
/*                                                                           */
/*****************************************************************************/

KHE_EVENNESS_MONITOR KheEvennessMonitorMake(KHE_SOLN soln,
  KHE_RESOURCE_GROUP partition, KHE_TIME time)
{
  KHE_EVENNESS_MONITOR res;  HA_ARENA a;
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln, KHE_EVENNESS_MONITOR_TAG,
    KHE_LINEAR_COST_FUNCTION, KheCost(1, 0));
  res->partition = partition;
  res->time = time;
  res->limit = KheResourceGroupResourceCount(partition) -
    (KheResourceGroupResourceCount(partition) / 6);
  res->count = 0;
  res->weight = KheCost(0, 1);
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheEvennessMonitorPartition(KHE_EVENNESS_MONITOR m)   */
/*                                                                           */
/*  Return the partition attribute of m.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheEvennessMonitorPartition(KHE_EVENNESS_MONITOR m)
{
  return m->partition;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheEvennessMonitorTime(KHE_EVENNESS_MONITOR m)                  */
/*                                                                           */
/*  Return the time attribute of m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheEvennessMonitorTime(KHE_EVENNESS_MONITOR m)
{
  return m->time;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEvennessMonitorCount(KHE_EVENNESS_MONITOR m)                      */
/*                                                                           */
/*  Return the count attribute of m (will be 0 if m is unattached).          */
/*                                                                           */
/*****************************************************************************/

int KheEvennessMonitorCount(KHE_EVENNESS_MONITOR m)
{
  return m->count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEvennessMonitorLimit(KHE_EVENNESS_MONITOR m)                      */
/*                                                                           */
/*  Return the limit attribute of m.                                         */
/*                                                                           */
/*****************************************************************************/

int KheEvennessMonitorLimit(KHE_EVENNESS_MONITOR m)
{
  return m->limit;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorSetLimit(KHE_EVENNESS_MONITOR m, int limit)       */
/*                                                                           */
/*  Set the limit attribute of m.                                            */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorSetLimit(KHE_EVENNESS_MONITOR m, int limit)
{
  m->limit = limit;
  if( m->attached )
    KheMonitorChangeCost((KHE_MONITOR) m, KheEvennessMonitorCost(m));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEvennessMonitorWeight(KHE_EVENNESS_MONITOR m)                */
/*                                                                           */
/*  Return the weight of m.                                                  */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEvennessMonitorWeight(KHE_EVENNESS_MONITOR m)
{
  return m->weight;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorSetWeight(KHE_EVENNESS_MONITOR m, KHE_COST weight)*/
/*                                                                           */
/*  Set the weight attribute of m.                                           */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorSetWeight(KHE_EVENNESS_MONITOR m, KHE_COST weight)
{
  m->weight = weight;
  if( m->attached )
    KheMonitorChangeCost((KHE_MONITOR) m, KheEvennessMonitorCost(m));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_MONITOR KheEvennessMonitorCopyPhase1(KHE_EVENNESS_MONITOR m,*/
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_EVENNESS_MONITOR KheEvennessMonitorCopyPhase1(KHE_EVENNESS_MONITOR m,
  HA_ARENA a)
{
  KHE_EVENNESS_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->partition = m->partition;
    copy->time = m->time;
    copy->limit = m->limit;
    copy->count = m->count;
    copy->weight = m->weight;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorCopyPhase2(KHE_EVENNESS_MONITOR m)                */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorCopyPhase2(KHE_EVENNESS_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorDelete(KHE_EVENNESS_MONITOR m)                    */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEvennessMonitorDelete(KHE_EVENNESS_MONITOR m)
{
  if( m->attached )
    KheEvennessMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorAttachToSoln(KHE_EVENNESS_MONITOR m)              */
/*                                                                           */
/*  Attach m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorAttachToSoln(KHE_EVENNESS_MONITOR m)
{
  KHE_MEET cycle_meet;  int cycle_offset;
  HnAssert(m->count == 0, "KheEvennessMonitorAttachToSoln internal error");
  cycle_meet = KheSolnTimeCycleMeet(m->soln, m->time);
  cycle_offset = KheSolnTimeCycleMeetOffset(m->soln, m->time);
  KheMeetPartitionTaskCount(cycle_meet, cycle_offset, m->partition, &m->count);
  KheEvennessHandlerMonitorAttach(KheSolnEvennessHandler(m->soln));
  m->attached = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorDetachFromSoln(KHE_EVENNESS_MONITOR m)            */
/*                                                                           */
/*  Detach m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorDetachFromSoln(KHE_EVENNESS_MONITOR m)
{
  if( m->cost != 0 )
  {
    KheMonitorChangeCost((KHE_MONITOR) m, 0);
    m->count = 0;
  }
  KheEvennessHandlerMonitorDetach(KheSolnEvennessHandler(m->soln));
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorAttachCheck(KHE_EVENNESS_MONITOR m)               */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEvennessMonitorAttachCheck(KHE_EVENNESS_MONITOR m)
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
/*  void KheEvennessMonitorAddTask(KHE_EVENNESS_MONITOR m)                   */
/*                                                                           */
/*  Monitor the addition of a task to what m is monitoring.                  */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorAddTask(KHE_EVENNESS_MONITOR m)
{
  if( m->attached )
  {
    m->count++;
    KheMonitorChangeCost((KHE_MONITOR) m, KheEvennessMonitorCost(m));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEvennessMonitorDeleteTask(KHE_EVENNESS_MONITOR m)                */
/*                                                                           */
/*  Monitor the deletion of a task from what m is monitoring.                */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorDeleteTask(KHE_EVENNESS_MONITOR m)
{
  if( m->attached )
  {
    m->count--;
    HnAssert(m->count >= 0, "KheEvennessMonitorDeleteTask internal error");
    KheMonitorChangeCost((KHE_MONITOR) m, KheEvennessMonitorCost(m));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviation"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheEvennessMonitorDeviation(KHE_EVENNESS_MONITOR m)                  */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheEvennessMonitorDeviation(KHE_EVENNESS_MONITOR m)
{
  return m->count > m->limit ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEvennessMonitorDeviationDescription(KHE_EVENNESS_MONITOR m)     */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheEvennessMonitorDeviationDescription(KHE_EVENNESS_MONITOR m)
{
  return m->count > m->limit ? "1" : "0";
  /* ***
  ARRAY_CHAR ac;
  MStringInit(ac);
  if( m->count > m->limit )
    MStringAddString(ac, "1");
  else
    MStringAddString(ac, "0");
  return MStringVal(ac);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEvennessMonitorDeviationCount(KHE_EVENNESS_MONITOR m)             */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheEvennessMonitorDeviationCount(KHE_EVENNESS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheEvennessMonitorDeviation(KHE_EVENNESS_MONITOR m, int i)           */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheEvennessMonitorDeviation(KHE_EVENNESS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheEvennessMonitorDeviation: i out of range");
  return m->count > m->limit ? 1 : 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheEvennessMonitorDeviationDescription(                            */
/*    KHE_EVENNESS_MONITOR m, int i)                                         */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheEvennessMonitorDeviationDescription(
  KHE_EVENNESS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheEvennessMonitorDeviationDescription: i out of range");
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
/*  void KheEvennessMonitorDebug(KHE_EVENNESS_MONITOR m,                     */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheEvennessMonitorDebug(KHE_EVENNESS_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  /* KHE_MEET cycle_meet;  int cycle_offset; */
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " ");
    KheResourceGroupDebug(m->partition, 1, -1, fp);
    fprintf(fp, " at %s (lim %d, count %d)",
      KheTimeId(m->time)==NULL ? "-" : KheTimeId(m->time),
      m->limit, m->count);
    /* *** correct but not using this now
    if( verbosity >= 2 && m->limit > 1 && m->count >= m->limit )
    {
      fprintf(fp, "\n");
      cycle_meet = KheSolnTimeCycleMeet(m->soln, m->time);
      cycle_offset = KheSolnTimeCycleMeetOffset(m->soln, m->time);
      KheMeetPartitionTaskDebug(cycle_meet, cycle_offset, m->partition,
	verbosity, indent + 2, fp);
      KheMonitorDebugEnd((KHE_MONITOR) m, false, indent, fp);
    }
    else
    *** */
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

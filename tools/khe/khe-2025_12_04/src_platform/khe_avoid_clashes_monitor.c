
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
/*  FILE:         khe_avoid_clashes_monitor.c                                */
/*  DESCRIPTION:  An avoid clashes monitor                                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_MONITOR                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_clashes_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  int				new_deviation;		/* total devs        */
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* monitored resource*/
  KHE_AVOID_CLASHES_CONSTRAINT	constraint;		/* constraint        */
  /* bool			separate; */		/* separate          */
  /* KHE_DEV_MONITOR		separate_dev_monitor;*/	/* separate devs     */
  KHE_AVOID_CLASHES_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorSetLowerBound(KHE_AVOID_CLASHES_MONITOR m)    */
/*                                                                           */
/*  Set the lower bound of m.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheAvoidClashesMonitorSetLowerBound(KHE_AVOID_CLASHES_MONITOR m)
{
  KHE_RESOURCE r;  int events_durn, cycle_durn;  KHE_CONSTRAINT mc;

  r = KheAvoidClashesMonitorResource(m);
  mc = (KHE_CONSTRAINT) m->constraint;
  events_durn = KheResourcePreassignedEventsDuration(r,
    KheConstraintCombinedWeight(mc));
  cycle_durn = KheInstanceTimeCount(KheResourceInstance(r));

  if( events_durn > cycle_durn )
  {
    m->lower_bound = KheMonitorDevToCost((KHE_MONITOR) m,
      events_durn - cycle_durn);
    if( DEBUG2 )
      fprintf(stderr, "  setting avoid clashes lower bound of %s to %.5f\n",
	KheResourceId(r), KheCostShow(m->lower_bound));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_MONITOR KheAvoidClashesMonitorMake(                    */
/*    KHE_RESOURCE_IN_SOLN rs, KHE_AVOID_CLASHES_CONSTRAINT c)               */
/*                                                                           */
/*  Make a new avoid clashes monitor with these attributes.                  */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_CLASHES_MONITOR KheAvoidClashesMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, KHE_AVOID_CLASHES_CONSTRAINT c)
{
  KHE_AVOID_CLASHES_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_AVOID_CLASHES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->resource_in_soln = rs;
  res->constraint = c;
  /* ***
  res->separate =
    (KheConstrai ntCostFunction((KHE_CONSTRAINT) c) != KHE_SUM_COST_FUNCTION);
  KheDevMonitorInit(&res->separate_dev_monitor);
  *** */
  res->deviation = 0;
  res->new_deviation = 0;
  res->copy = NULL;
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  KheAvoidClashesMonitorSetLowerBound(res);
  /* KheGroupMonitorAddMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) res); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_MONITOR KheAvoidClashesMonitorCopyPhase1(              */
/*    KHE_AVOID_CLASHES_MONITOR m, HA_ARENA a)                               */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_CLASHES_MONITOR KheAvoidClashesMonitorCopyPhase1(
  KHE_AVOID_CLASHES_MONITOR m, HA_ARENA a)
{
  KHE_AVOID_CLASHES_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->resource_in_soln = KheResourceInSolnCopyPhase1(m->resource_in_soln,a);
    copy->constraint = m->constraint;
    /* ***
    copy->separate = m->separate;
    KheDevMonitorCopy(&copy->separate_dev_monitor, &m->separate_dev_monitor);
    *** */
    copy->deviation = m->deviation;
    copy->new_deviation = m->new_deviation;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorCopyPhase2(KHE_AVOID_CLASHES_MONITOR m)       */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorCopyPhase2(KHE_AVOID_CLASHES_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorDelete(KHE_AVOID_CLASHES_MONITOR m)           */
/*                                                                           */
/*  Free m.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidClashesMonitorDelete(KHE_AVOID_CLASHES_MONITOR m)
{
  if( m->attached )
    KheAvoidClashesMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheResourceInSolnDele teMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  ** KheDevMonitorFree(&m->separate_dev_monitor); **
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_CONSTRAINT KheAvoidClashesMonitorConstraint(           */
/*    KHE_AVOID_CLASHES_MONITOR m)                                           */
/*                                                                           */
/*  Return the contraint that m is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_CLASHES_CONSTRAINT KheAvoidClashesMonitorConstraint(
  KHE_AVOID_CLASHES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheAvoidClashesMonitorResource(KHE_AVOID_CLASHES_MONITOR m) */
/*                                                                           */
/*  Return the resource that m is monitoring.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheAvoidClashesMonitorResource(KHE_AVOID_CLASHES_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0, because when unlinked the monitor has no    */
/*  access to a timetable and cannot observe any clashes.                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorAttachToSoln(KHE_AVOID_CLASHES_MONITOR m)     */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorAttachToSoln(KHE_AVOID_CLASHES_MONITOR m)
{
  m->attached = true;
  KheResourceInSolnAttachMonitor(m->resource_in_soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorDetachFromSoln(KHE_AVOID_CLASHES_MONITOR m)   */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorDetachFromSoln(KHE_AVOID_CLASHES_MONITOR m)
{
  KheResourceInSolnDetachMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  HnAssert(m->deviation == 0,
    "KheAvoidClashesMonitorDetachFromSoln internal error");
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorAttachCheck(KHE_AVOID_CLASHES_MONITOR m)      */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidClashesMonitorAttachCheck(KHE_AVOID_CLASHES_MONITOR m)
{
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
    KheMonitorAttachToSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesMonitorTimeRange(KHE_AVOID_CLASHES_MONITOR m,        */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidClashesMonitorTimeRange(KHE_AVOID_CLASHES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_INSTANCE ins;
  ins = KheSolnInstance(m->soln);
  if( KheInstanceTimeCount(ins) == 0 )
    return *first_time = *last_time = NULL, false;
  else
    return *first_time = KheInstanceTime(ins, 0),
      *last_time = KheInstanceTime(ins, KheInstanceTimeCount(ins) - 1), true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorSetSweepTime(KHE_AVOID_CLASHES_MONITOR m,     */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*  Adding a sweep time does not change the cost of m, assuming that         */
/*  times beyond the sweep time are unassigned.                              */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorSetSweepTime(KHE_AVOID_CLASHES_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesMonitorSweepTimeRange(KHE_AVOID_CLASHES_MONITOR m,   */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*  Adding a sweep time does not change the cost of m, assuming that         */
/*  times beyond the sweep time are unassigned.                              */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidClashesMonitorSweepTimeRange(KHE_AVOID_CLASHES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return *first_time = *last_time = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorChangeClashCount(KHE_AVOID_CLASHES_MONITOR m, */
/*    int old_clash_count, int new_clash_count)                              */
/*                                                                           */
/*  Change one of the deviation counts held by m from old_clash_count to     */
/*  new_clash_count.  These are assumed to be distinct.                      */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorChangeClashCount(KHE_AVOID_CLASHES_MONITOR m,
  int old_clash_count, int new_clash_count)
{
  /* ***
  if( m->separate )
  {
    if( old_clash_count == 0 )
      KheDevMonitorAttach(&m->separate_dev_monitor, new_clash_count);
    else if( new_clash_count == 0 )
      KheDevMonitorDetach(&m->separate_dev_monitor, old_clash_count);
    else
      KheDevMonitorUpdate(&m->separate_dev_monitor, old_clash_count,
	new_clash_count);
  }
  else
  *** */
  m->new_deviation += (new_clash_count - old_clash_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorFlush(KHE_AVOID_CLASHES_MONITOR m)            */
/*                                                                           */
/*  Flush m.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorFlush(KHE_AVOID_CLASHES_MONITOR m)
{
  if( m->new_deviation != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->new_deviation));
    m->deviation = m->new_deviation;
  }
}

/* ***
void KheAvoidClashesMonitorFlush(KHE_AVOID_CLASHES_MONITOR m)
{
  if( m->separate )
  {
    if( KheDevMonitorHasChanged(&m->separate_dev_monitor) )
    {
      KheMonitorChangeCost((KHE_MONITOR) m,
        KheConstrai ntCostMulti((KHE_CONSTRAINT) m->constraint,
	  KheDevMonitorDevs(&m->separate_dev_monitor)));
      KheDevMonitorFlush(&m->separate_dev_monitor);
    }
  }
  else
  {
    if( m->new_deviation != m->deviation )
    {
      KheMonitorChangeCost((KHE_MONITOR) m,
        KheMonitorDevToCost((KHE_MONITOR) m, m->new_deviation));
      m->deviation = m->new_deviation;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesMonitorDeviation(KHE_AVOID_CLASHES_MONITOR m)         */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesMonitorDeviation(KHE_AVOID_CLASHES_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidClashesMonitorDeviationDescription(                        */
/*    KHE_AVOID_CLASHES_MONITOR m)                                           */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidClashesMonitorDeviationDescription(KHE_AVOID_CLASHES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count, val;  KHE_INSTANCE ins;  KHE_TIME t;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  char *name;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d: ", m->deviation);
    ins = KheSolnInstance(m->soln);
    rtm = KheResourceInSolnTimetableMonitor(m->resource_in_soln);
    if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
      KheMonitorAttachToSoln((KHE_MONITOR) rtm);
    count = 0;
    for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
    {
      t = KheInstanceTime(ins, i);
      val = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
      if( val > 1 )
      {
	if( count > 0 )
          HnStringAdd(&ac, "; ");
	if( val >= 3 )
	  HnStringAdd(&ac, "%d ", val - 1);
	name = KheTimeName(t);
	HnStringAdd(&ac, name == NULL ? "?" : name);
	count++;
      }
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesMonitorDeviationCount(KHE_AVOID_CLASHES_MONITOR m)    */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAvoidClashesMonitorDeviationCount(KHE_AVOID_CLASHES_MONITOR m)
{
  KHE_TIMETABLE_MONITOR tt;  KHE_INSTANCE ins;  KHE_TIME t;  int i, res;
  ins = KheSolnInstance(m->soln);
  tt = KheResourceTimetableMonitor(m->soln,
    KheResourceInSolnResource(m->resource_in_soln));
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) tt) )
    KheMonitorAttachToSoln((KHE_MONITOR) tt);
  res = 0;
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    t = KheInstanceTime(ins, i);
    if( KheTimetableMonitorTimeMeetCount(tt, t) > 1 )
      res++;
  }
  if( DEBUG1 )
    fprintf(stderr, "  KheAvoidClashesMonitorDeviationCount(m) returning %d\n",
      res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool FindDeviation(KHE_AVOID_CLASHES_MONITOR m, int i,                   */
/*    KHE_TIME *t, int *dev)                                                 */
/*                                                                           */
/*  If m has an i'th deviation, return true with *t set to its time and      */
/*  *dev set to its deviation.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool FindDeviation(KHE_AVOID_CLASHES_MONITOR m, int i,
  KHE_TIME *t, int *dev)
{
  KHE_TIMETABLE_MONITOR tt;  KHE_INSTANCE ins;  int j, res;
  ins = KheSolnInstance(m->soln);
  tt = KheResourceTimetableMonitor(m->soln,
    KheResourceInSolnResource(m->resource_in_soln));
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) tt) )
    KheMonitorAttachToSoln((KHE_MONITOR) tt);
  res = 0;
  for( j = 0;  j < KheInstanceTimeCount(ins);  j++ )
  {
    *t = KheInstanceTime(ins, j);
    *dev = KheTimetableMonitorTimeMeetCount(tt, *t) - 1;
    if( *dev > 0 )
      res++;
    if( res == i + 1 )
    {
      if( DEBUG1 )
	fprintf(stderr, "  FindDeviation(m, %d) returning true (%s, %d)\n",
	  i, KheTimeName(*t), *dev);
      return true;
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "  FindDeviation(m, %d) returning false\n", i);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesMonitorDeviation(KHE_AVOID_CLASHES_MONITOR m, int i)  */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAvoidClashesMonitorDeviation(KHE_AVOID_CLASHES_MONITOR m, int i)
{
  KHE_TIME t;  int dev;
  if( !FindDeviation(m, i, &t, &dev) )
    MAbort("KheAvoidClashesMonitorDeviation: i out of range");
  return dev;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidClashesMonitorDeviationDescription(                        */
/*    KHE_AVOID_CLASHES_MONITOR m, int i)                                    */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheAvoidClashesMonitorDeviationDescription(
  KHE_AVOID_CLASHES_MONITOR m, int i)
{
  KHE_TIME t;  int dev;
  if( !FindDeviation(m, i, &t, &dev) )
    MAbort("KheAvoidClashesMonitorDeviationDescription: i out of range");
  return KheTimeName(t);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidClashesMonitorPointOfApplication(                          */
/*    KHE_AVOID_CLASHES_MONITOR m)                                           */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidClashesMonitorPointOfApplication(KHE_AVOID_CLASHES_MONITOR m)
{
  return KheResourceName(KheAvoidClashesMonitorResource(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidClashesMonitorId(KHE_AVOID_CLASHES_MONITOR m)              */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidClashesMonitorId(KHE_AVOID_CLASHES_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheAvoidClashesMonitorResource(m));
    a = KheSolnArena(m->soln);
    m->id = HnStringMake(a, "%s/%s", constraint_id, resource_id);
  }
  return m->id;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesMonitorDebug(KHE_AVOID_CLASHES_MONITOR m,            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesMonitorDebug(KHE_AVOID_CLASHES_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " %s", "ACLM");
    /* KheResourceInSolnDebug(m->resource_in_soln, 1, -1, fp); */
    /* ***
    fprintf(fp, "%s ", m->separate ? " sep" : "");
    if( m->separate )
      KheDevMonitorDebug(&m->separate_dev_monitor, fp);
    else
    *** */
    fprintf(fp, "%s%d", m->deviation != m->new_deviation ? "*" : "",
      m->deviation);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

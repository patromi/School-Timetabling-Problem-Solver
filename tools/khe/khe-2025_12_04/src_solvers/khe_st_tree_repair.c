
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
/*  FILE:         khe_st_tree_repair.c                                       */
/*  DESCRIPTION:  Tree search layer time repair                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define MAX_ASSIGNMENTS 10000

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_TREE_LAYER_SOLVER - one solver for this problem                      */
/*  KHE_MEET_OBJ - one meet being reassigned                                 */
/*  KHE_EXCLUSION - an exclusion                                             */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_EXCLUSION_CLASH,
  KHE_EXCLUSION_SPREAD,
  KHE_EXCLUSION_SYMMETRY
} KHE_EXCLUSION_TYPE;

typedef struct khe_tree_layer_solver_rec *KHE_TREE_LAYER_SOLVER;

typedef struct khe_meet_obj_rec *KHE_MEET_OBJ;
typedef HA_ARRAY(KHE_MEET_OBJ) ARRAY_KHE_MEET_OBJ;

typedef struct khe_exclusion_rec {
  KHE_MEET_OBJ		meet_obj;		/* the excluded meet         */
  int			time_index;		/* the excluded time index   */
  KHE_EXCLUSION_TYPE	type;			/* type of exclusion         */
} KHE_EXCLUSION;

typedef HA_ARRAY(KHE_EXCLUSION) ARRAY_KHE_EXCLUSION;

typedef struct khe_exclusion_set_rec {
  KHE_TIME		time;			/* the time being excluded   */
  ARRAY_KHE_EXCLUSION	exclusions;		/* exclusions at time        */
} *KHE_EXCLUSION_SET;

typedef HA_ARRAY(KHE_EXCLUSION_SET) ARRAY_KHE_EXCLUSION_SET;

struct khe_meet_obj_rec {
  KHE_TREE_LAYER_SOLVER		solver;		/* enclosing solver          */
  KHE_MEET			meet;		/* the meet represented      */
  HA_ARRAY_INT			domain;		/* how open each time is     */
  int				domain_size;	/* cardinality of domain     */
  /* int			curr_asst; */	/* current assignment        */
  /* int			best_asst; */	/* best assignment           */
  ARRAY_KHE_EXCLUSION_SET	exclusion_sets;	/* exclusions by time        */
  KHE_INSTANCE			instance;	/* for convenience           */
};

struct khe_tree_layer_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;		/* solution                  */
  KHE_RESOURCE			resource;	/* resource                  */
  ARRAY_KHE_MEET_OBJ		meet_objs;	/* meets to reassign         */
  int				unassignable;	/* unassignable meets        */
  int				assign_count;	/* number of assignments     */
  int				bottom_count;	/* number of bottoms         */
  KHE_MARK			root_mark;	/* at root of search tree    */
  /* KHE_TRANSACTION		init_trans; */	/* holds init assignments    */
  KHE_COST			init_cost;	/* initial cost              */
  /* KHE_COST			best_cost; */	/* best so far               */
};


/*****************************************************************************/
/*                                                                           */
/*  Exclusion sets                                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EXCLUSION_SET KheExclusionSetMake(KHE_TIME time)                     */
/*                                                                           */
/*  Make a new, empty exclusion set.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_EXCLUSION_SET KheExclusionSetMake(KHE_TIME time, HA_ARENA a)
{
  KHE_EXCLUSION_SET res;
  HaMake(res, a);
  res->time = time;
  HaArrayInit(res->exclusions, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheExclusionSetDelete(KHE_EXCLUSION_SET es)                         */
/*                                                                           */
/*  Delete es, reclaiming its memory.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheExclusionSetDelete(KHE_EXCLUSION_SET es)
{
  MArrayFree(es->exclusions);
  MFree(es);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheExclusionSetAddExclusion(KHE_EXCLUSION_SET es,                   */
/*    KHE_MEET_OBJ meet_obj, int time_index, KHE_EXCLUSION_TYPE type)        */
/*                                                                           */
/*  Add exclusion (meet_obj, time_index, etype) to es, but only if           */
/*  time_index is in the domain of meet_obj, since there is no point in      */
/*  excluding something that will not happen anyway.  Don't uniqueify yet.   */
/*                                                                           */
/*****************************************************************************/

static void KheExclusionSetAddExclusion(KHE_EXCLUSION_SET es,
  KHE_MEET_OBJ meet_obj, int time_index, KHE_EXCLUSION_TYPE type)
{
  KHE_EXCLUSION e;
  if( HaArray(meet_obj->exclusion_sets, time_index) != NULL )
  {
    e.meet_obj = meet_obj;
    e.time_index = time_index;
    e.type = type;
    HaArrayAddLast(es->exclusions, e);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheExclusionCmp(const void *p1, const void *p2)                      */
/*                                                                           */
/*  Comparison function for uniqueifying an array of exclusions.             */
/*  NB the type of exclusion is ignored.                                     */
/*                                                                           */
/*****************************************************************************/

static int KheExclusionCmp(const void *p1, const void *p2)
{
  KHE_EXCLUSION *e1 = (KHE_EXCLUSION *) p1;
  KHE_EXCLUSION *e2 = (KHE_EXCLUSION *) p2;
  if( e1->meet_obj->meet != e2->meet_obj->meet )
    return KheMeetSolnIndex(e1->meet_obj->meet) -
      KheMeetSolnIndex(e2->meet_obj->meet);
  return e1->time_index - e2->time_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheExclusionSetUniqueify(KHE_EXCLUSION_SET es)                      */
/*                                                                           */
/*  Uniqueify es.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheExclusionSetUniqueify(KHE_EXCLUSION_SET es)
{
  HaArraySortUnique(es->exclusions, &KheExclusionCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  char KheExclusionTypeShow(KHE_EXCLUSION_TYPE type)                       */
/*                                                                           */
/*  Return a one-character display of an exclusion type.                     */
/*                                                                           */
/*****************************************************************************/

static char KheExclusionTypeShow(KHE_EXCLUSION_TYPE type)
{
  switch( type )
  {
    case KHE_EXCLUSION_CLASH:		return '*';
    case KHE_EXCLUSION_SPREAD:		return '-';
    case KHE_EXCLUSION_SYMMETRY:	return '=';

    default:
      HnAbort("KheExclusionTypeShow internal error");
      return '?';  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheExclusionSetDebug(KHE_EXCLUSION_SET es, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of es onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheExclusionSetDebug(KHE_EXCLUSION_SET es, int verbosity,
  int indent, FILE *fp)
{
  KHE_EXCLUSION esi, esj, esk, esn;  int i, j, k, n;  KHE_TIME time1, time2;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ ExclusionSet(%s):\n", indent, "", KheTimeId(es->time));
    for( i = 0;  i < HaArrayCount(es->exclusions);  i = j )
    {
      esi = HaArray(es->exclusions, i);

      /* find a run of exclusions with the same meet object as esi */
      for( j = i + 1;  j < HaArrayCount(es->exclusions);  j++ )
      {
	esj = HaArray(es->exclusions, j);
	if( esi.meet_obj != esj.meet_obj )
	  break;
      }

      /* print that run of exclusions all together */
      fprintf(fp, "%*s(", indent + 2, "");
      KheMeetDebug(esi.meet_obj->meet, 1, -1, fp);
      for( k = i;  k < j;  k = n )
      {
	esk = HaArray(es->exclusions, k);

	/* find a run of exclusions with the same type and consecutive times */
	for( n = k + 1;  n < j;  n++ )
	{
	  esn = HaArray(es->exclusions, n);
	  if( esn.time_index != esk.time_index + (n-k) || esn.type != esk.type )
	    break;
	}

	/* print that run */
        fprintf(fp, ", %c", KheExclusionTypeShow(esk.type));
	esn = HaArray(es->exclusions, n - 1);
	time1 = KheInstanceTime(esk.meet_obj->instance, esk.time_index);
	time2 = KheInstanceTime(esn.meet_obj->instance, esn.time_index);
	if( time1 == time2 )
	  fprintf(fp, "%s", KheTimeId(time1));
	else
	  fprintf(fp, "%s:%s", KheTimeId(time1), KheTimeId(time2));
      }
      fprintf(fp, ")\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet objects"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_OBJ KheMeetObjMake(KHE_TREE_LAYER_SOLVER solver, KHE_MEET meet) */
/*                                                                           */
/*  Make a new meet object with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET_OBJ KheMeetObjMake(KHE_TREE_LAYER_SOLVER solver,
  KHE_MEET meet, HA_ARENA a)
{
  KHE_MEET_OBJ res;  int count;  KHE_INSTANCE ins;
  HaMake(res, a);
  res->solver = solver;
  res->meet = meet;
  HaArrayInit(res->domain, a);
  ins = KheSolnInstance(KheMeetSoln(meet));
  count = KheInstanceTimeCount(ins);
  HaArrayFill(res->domain, count, 0);
  res->domain_size = count;
  /* res->curr_asst = -1; */
  /* res->best_asst = -1; */
  HaArrayInit(res->exclusion_sets, a);
  HaArrayFill(res->exclusion_sets, count, NULL);
  /* ***
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->exclusion_sets,
      KheExclusionSetMake(KheInstanceTime(ins, i)));
  *** */
  res->instance = ins;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjDelete(KHE_MEET_OBJ mo)                                   */
/*                                                                           */
/*  Delete mo.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMeetObjDelete(KHE_MEET_OBJ mo)
{
  KHE_EXCLUSION_SET es;
  MArrayFree(mo->domain);
  while( HaArrayCount(mo->exclusion_sets) > 0 )
  {
    es = MArrayRemoveLast(mo->exclusion_sets);
    if( es != NULL )
      KheExclusionSetDelete(es);
  }
  MArrayFree(mo->exclusion_sets);
  MFree(mo);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjApplyExclusion(KHE_MEET_OBJ mo, int time_index)           */
/*                                                                           */
/*  Exlude time_index from the domain of mo.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjApplyExclusion(KHE_MEET_OBJ mo, int time_index)
{
  if( ++HaArray(mo->domain, time_index) == 1 )
  {
    /* time_index excluded for the first time, so reduce domain */
    if( --mo->domain_size == 0 )
      mo->solver->unassignable++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjUnApplyExclusion(KHE_MEET_OBJ mo, int time_index)         */
/*                                                                           */
/*  Undo the corresponding KheMeetObjApplyExclusion.                         */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjUnApplyExclusion(KHE_MEET_OBJ mo, int time_index)
{
  if( --HaArray(mo->domain, time_index) == 0 )
  {
    /* time_index excluded for the last time, so increase domain */
    if( ++mo->domain_size == 1 )
      mo->solver->unassignable--;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetObjCmp(const void *p1, const void *p2)                        */
/*                                                                           */
/*  Comparison function for sorting meets by decreasing duration, and        */
/*  also uniqueifying them.                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheMeetObjCmp(const void *p1, const void *p2)
{
  KHE_MEET_OBJ mo1 = * (KHE_MEET_OBJ *) p1;
  KHE_MEET_OBJ mo2 = * (KHE_MEET_OBJ *) p2;
  if( KheMeetDuration(mo1->meet) != KheMeetDuration(mo2->meet) )
    return KheMeetDuration(mo2->meet) - KheMeetDuration(mo1->meet);
  else
    return KheMeetSolnIndex(mo1->meet) - KheMeetSolnIndex(mo2->meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceReduceDomain(KHE_RESOURCE r, int offset, int durn,       */
/*    KHE_MEET_OBJ mo)                                                       */
/*                                                                           */
/*  Apply an exclusion to mo for each time that r is unavailable, given      */
/*  that r first appears in mo's meet at offset and is occupied for durn.    */
/*                                                                           */
/*****************************************************************************/

static void KheResourceReduceDomain(KHE_RESOURCE r, int offset, int durn,
  KHE_MEET_OBJ mo)
{
  int i, j, k, time_index, ti;  KHE_TIME_GROUP domain;
  KHE_CONSTRAINT c;  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    if( KheConstraintTag(c) == KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG )
    {
      autc = (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c;
      domain = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);
      for( j = 0;  j < KheTimeGroupTimeCount(domain);  j++ )
      {
	ti = KheTimeIndex(KheTimeGroupTime(domain, j));
	for( k = 0;  k < durn;  k++ )
	{
	  time_index = ti - (offset + k);
	  if( time_index >= 0 && time_index < HaArrayCount(mo->domain) )
	    KheMeetObjApplyExclusion(mo, time_index);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetResourcesReduceDomain(KHE_MEET meet, int offset,             */
/*    KHE_MEET_OBJ mo)                                                       */
/*                                                                           */
/*  Apply an exclusion to mo for each time that the resources assigned       */
/*  to meet, directly or indirectly, are unavailable.                        */
/*                                                                           */
/*****************************************************************************/

static void KheMeetResourcesReduceDomain(KHE_MEET meet, int offset,
  KHE_MEET_OBJ mo)
{
  int i, child_offset;  KHE_MEET child_meet;  KHE_TASK task;  KHE_RESOURCE r;

  /* handle meet's own tasks */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    r = KheTaskAsstResource(task);
    if( r != NULL )
      KheResourceReduceDomain(r, offset, KheMeetDuration(meet), mo);
  }

  /* handle meets assigned to meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    child_offset = offset + KheMeetAsstOffset(child_meet);
    KheMeetResourcesReduceDomain(child_meet, child_offset, mo);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjReduceDomain(KHE_MEET_OBJ mo)                             */
/*                                                                           */
/*  Reduce the domain of mo by removing times not in its meet's domain,      */
/*  and starting times which would cause the resources assigned to it        */
/*  to occur at unavailable times.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjReduceDomain(KHE_MEET_OBJ mo)
{
  int ti, pos;  KHE_TIME time;  KHE_TIME_GROUP domain;

  /* handle own domain */
  domain = KheMeetDomain(mo->meet);
  for( ti = 0;  ti < KheInstanceTimeCount(mo->instance);  ti++ )
  {
    time = KheInstanceTime(mo->instance, ti);
    if( !KheTimeGroupContains(domain, time, &pos) )
      KheMeetObjApplyExclusion(mo, ti);
  }

  /* handle unavailable times of assigned resources */
  KheMeetResourcesReduceDomain(mo->meet, 0, mo);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddExclusionSets(KHE_MEET_OBJ mo)                         */
/*                                                                           */
/*  Add exclusion sets to mo, but only where domains permit.                 */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddExclusionSets(KHE_MEET_OBJ mo, HA_ARENA a)
{
  int dval, ti;  KHE_TIME time;
  HaArrayForEach(mo->domain, dval, ti)
    if( dval == 0 )
    {
      time = KheInstanceTime(mo->instance, ti);
      HaArrayPut(mo->exclusion_sets, ti, KheExclusionSetMake(time, a));
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddSpread(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2,             */
/*    KHE_SPREAD_EVENTS_MONITOR sem)                                         */
/*                                                                           */
/*  Given that mo1->meet and mo2->meet share spread events monitor sem,      */
/*  add to mo1 the spread exclusions for mo2 derived from sem.               */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddSpread(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2,
  KHE_SPREAD_EVENTS_MONITOR sem)
{
  KHE_TIME_SPREAD ts;  KHE_SPREAD_EVENTS_CONSTRAINT sec;  KHE_EXCLUSION_SET es;
  KHE_LIMITED_TIME_GROUP ltg;  int i, j, k, ti1, ti2;  KHE_TIME_GROUP tg;
  sec = KheSpreadEventsMonitorConstraint(sem);
  ts = KheSpreadEventsConstraintTimeSpread(sec);
  for( i = 0;  i < KheTimeSpreadLimitedTimeGroupCount(ts);  i++ )
  {
    ltg = KheTimeSpreadLimitedTimeGroup(ts, i);
    if( KheLimitedTimeGroupMaximum(ltg) == 1 )
    {
      /* when mo1 starts at any time of ltg, exclude mo2 from starting */
      /* at any time of ltg */
      tg = KheLimitedTimeGroupTimeGroup(ltg);
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
      {
	ti1 = KheTimeIndex(KheTimeGroupTime(tg, j));
	es = HaArray(mo1->exclusion_sets, ti1);
	if( es != NULL )
	  for( k = 0;  k < KheTimeGroupTimeCount(tg);  k++ )
	  {
	    ti2 = KheTimeIndex(KheTimeGroupTime(tg, k));
	    KheExclusionSetAddExclusion(es, mo2, ti2, KHE_EXCLUSION_SPREAD);
	  }
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddSymmetry(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)           */
/*                                                                           */
/*  Add to mo1 the symmetry exclusions for mo2.                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddSymmetry(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)
{
  int ti1, ti2;  KHE_EXCLUSION_SET es;
  for( ti1 = 0;  ti1 < KheInstanceTimeCount(mo1->instance);  ti1++ )
  {
    es = HaArray(mo1->exclusion_sets, ti1);
    if( es != NULL )
      for( ti2 = 0;  ti2 < ti1;  ti2++ )
	KheExclusionSetAddExclusion(es, mo2, ti2, KHE_EXCLUSION_SYMMETRY);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddSpreadExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)   */
/*                                                                           */
/*  Add to mo1 the spread exclusions for mo2.                                */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddSpreadExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)
{
  KHE_EVENT event1, event2;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m1, m2;
  event1 = KheMeetEvent(mo1->meet);
  event2 = KheMeetEvent(mo2->meet);
  if( event1 != NULL && event2 != NULL )
  {
    soln = KheMeetSoln(mo1->meet);
    for( i = 0;  i < KheSolnEventMonitorCount(soln, event1);  i++ )
    {
      m1 = KheSolnEventMonitor(soln, event1, i);
      if( KheMonitorTag(m1) == KHE_SPREAD_EVENTS_MONITOR_TAG )
      {
	for( j = 0;  j < KheSolnEventMonitorCount(soln, event2);  j++ )
	{
	  m2 = KheSolnEventMonitor(soln, event2, j);
	  if( m2 == m1 )
	  {
	    KheMeetObjAddSpread(mo1, mo2, (KHE_SPREAD_EVENTS_MONITOR) m1);
	    if( KheMeetDuration(mo1->meet) == KheMeetDuration(mo2->meet) )
	      KheMeetObjAddSymmetry(mo1, mo2);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddClashExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)    */
/*                                                                           */
/*  Add to mo1 the clash exclusions for mo2.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddClashExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)
{
  int durn1, durn2, offset, ti, count;  KHE_EXCLUSION_SET es;

  count = HaArrayCount(mo1->exclusion_sets);
  durn1 = KheMeetDuration(mo1->meet);
  durn2 = KheMeetDuration(mo2->meet);
  HaArrayForEach(mo1->exclusion_sets, es, ti)
    if( es != NULL )
    {
      /* exclude offset 0, where mo2 starts at the same time as mo1 */
      KheExclusionSetAddExclusion(es, mo2, ti, KHE_EXCLUSION_CLASH);

      /* exclude offsets where mo2 starts after mo1, and overlaps it */
      for( offset = 1;  offset < durn1 && ti + offset < count;  offset++ )
	KheExclusionSetAddExclusion(es, mo2, ti + offset, KHE_EXCLUSION_CLASH);

      /* exclude offsets where mo2 starts before  mo1, and overlaps it */
      for( offset = 1;  offset < durn2 && ti - offset >= 0;  offset++ )
	KheExclusionSetAddExclusion(es, mo2, ti - offset, KHE_EXCLUSION_CLASH);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjAddExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)         */
/*                                                                           */
/*  Here mo1 precedes mo2 in the solve list.  Add to mo1 the exclusions      */
/*  for mo2.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjAddExclusions(KHE_MEET_OBJ mo1, KHE_MEET_OBJ mo2)
{
  KheMeetObjAddClashExclusions(mo1, mo2);
  KheMeetObjAddSpreadExclusions(mo1, mo2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjApplyExclusions(KHE_MEET_OBJ mo, int time_index)          */
/*                                                                           */
/*  Apply all exclusions arising from assigning time_index to mo.            */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjApplyExclusions(KHE_MEET_OBJ mo, int time_index)
{
  KHE_EXCLUSION_SET es;  KHE_EXCLUSION e;  int i;
  es = HaArray(mo->exclusion_sets, time_index);
  HnAssert(es != NULL, "KheMeetObjApplyExclusions internal error");
  HaArrayForEach(es->exclusions, e, i)
    KheMeetObjApplyExclusion(e.meet_obj, e.time_index);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjUnApplyExclusions(KHE_MEET_OBJ mo, int time_index)        */
/*                                                                           */
/*  Unapply all exclusions arising from assigning time_index to mo.          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjUnApplyExclusions(KHE_MEET_OBJ mo, int time_index)
{
  KHE_EXCLUSION_SET es;  KHE_EXCLUSION e;  int i;
  es = HaArray(mo->exclusion_sets, time_index);
  HnAssert(es != NULL, "KheMeetObjUnApplyExclusions internal error");
  HaArrayForEach(es->exclusions, e, i)
    KheMeetObjUnApplyExclusion(e.meet_obj, e.time_index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetObjectAssign(KHE_MEET_OBJ mo, int time_index)                */
/*                                                                           */
/*  Assign time_index to mo, returning true if successful:  if the           */
/*  assignment succeeded, there was no increase in the number of             */
/*  unmatched demand tixels, and the number of unassignable meets is         */
/*  still 0.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** expanded inline now; clearer that way
static bool KheMeetObjectAssign(KHE_MEET_OBJ mo, int time_index)
{
  KHE_TIME time;  int unassigned_tixels;

  ** fail if the meet refuses to assign **
  unassigned_tixels = KheSolnMatchingDefectCount(mo->solver->soln);
  time = KheInstanceTime(mo->instance, time_index);
  if( !KheMeetAssignTime(mo->meet, time) )
    return false;

  ** fail if the number of unassigned demand tixels has increased **
  if( KheSolnMatchingDefectCount(mo->solver->soln) > unassigned_tixels )
  {
    ** KheMeetUnAss ignTime(mo->meet); **
    return false;
  }

  ** carry out exclusions and fail if there are then any unassignable meets **
  KheMeetObjApplyExclusions(mo, time_index);
  if( mo->solver->unassignable > 0 )
  {
    KheMeetObjUnApplyExclusions(mo, time_index);
    ** KheMeetUnAss ignTime(mo->meet); **
    return false;
  }

  ** all good, return true **
  mo->solver->assign_count++;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjectUnAssign(KHE_MEET_OBJ mo, int time_index)              */
/*                                                                           */
/*  Undo a previous successful call to KheMeetObjectAssign with these        */
/*  parameters.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** expanded inline now; clearer that way
static void KheMeetObjectUnAssign(KHE_MEET_OBJ mo, int time_index)
{
  KheMeetObjUnApplyExclusions(mo, time_index);
  ** KheMeetUnAss ignTime(mo->meet); **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjDebug(KHE_MEET_OBJ mo, int verbosity,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mo onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheMeetObjDebug(KHE_MEET_OBJ mo, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_EXCLUSION_SET es;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Meet(", indent, "");
    KheMeetDebug(mo->meet, 1, -1, fp);
    fprintf(fp, ", domain_size %d)\n", mo->domain_size);
    if( verbosity >= 2 )
      HaArrayForEach(mo->exclusion_sets, es, i)
	if( es != NULL )
	  KheExclusionSetDebug(es, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solvers"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TREE_LAYER_SOLVER KheTreeLayerSolverMake(KHE_SOLN soln,              */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Make a new tree layer solver for r.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_TREE_LAYER_SOLVER KheTreeLayerSolverMake(KHE_SOLN soln,
  KHE_RESOURCE r, HA_ARENA a)
{
  KHE_TREE_LAYER_SOLVER res;  int i, junk;  KHE_MEET meet;  KHE_TASK task;
  /* HA_ARENA a; */
  /* a = HaAre naMake(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource = r;
  HaArrayInit(res->meet_objs, a);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    if( meet != NULL )
      HaArrayAddLast(res->meet_objs, KheMeetObjMake(res, meet, a));
  }
  HaArraySortUnique(res->meet_objs, &KheMeetObjCmp);
  res->unassignable = 0;
  res->assign_count = 0;
  res->bottom_count = 0;
  /* res->init_trans = KheTransactionMake(soln); */
  res->root_mark = NULL;
  res->init_cost = KheSolnCost(soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTreeLayerSolverDelete(KHE_TREE_LAYER_SOLVER tls)                 */
/*                                                                           */
/*  Delete tls;                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTreeLayerSolverDelete(KHE_TREE_LAYER_SOLVER tls)
{
  HaArenaDe lete(tls->arena);
  ** ***
  while( HaArrayCount(tls->meet_objs) > 0 )
    KheMeetObjDelete(MArrayRemoveLast(tls->meet_objs));
  ** KheTransactionDelete(tls->init_trans); **
  MFree(tls);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTreeLayerSolve(KHE_TREE_LAYER_SOLVER tls, int pos,               */
/*    int *end_index)                                                        */
/*                                                                           */
/*  Solve tls, starting from pos.   Set *end_index to the last index used.   */
/*                                                                           */
/*****************************************************************************/

static void KheTreeLayerSolve(KHE_TREE_LAYER_SOLVER tls, int pos,
  int *end_index)
{
  KHE_MEET_OBJ mo;  int i, dval, ti, shift, junk, unassigned_tixels;
  KHE_MARK mark;  KHE_TIME time;
  if( pos >= HaArrayCount(tls->meet_objs) )
  {
    /* at bottom of tree, save path if better */
    tls->bottom_count++;
    KheMarkAddBestPath(tls->root_mark, 1);
    /* ***
    if( KheSolnCost(tls->soln) < tls->best_cost )
    {
      ** new best, save assignments and cost **
      HaArrayForEach(tls->meet_objs, mo, i)
	mo->best_asst = mo->curr_asst;
      tls->best_cost = KheSolnCost(tls->soln);
      if( DEBUG2 )
	fprintf(stderr, "  KheTreeLayerSolve new best: %.5f\n",
	  KheCostShow(tls->best_cost));
    }
    *** */
  }
  else
  {
    /* try all assignments at pos, or just the first successful if limit */
    mark = KheMarkBegin(tls->soln);
    mo = HaArray(tls->meet_objs, pos);
    shift = tls->assign_count;
    for( i = 0;  i < HaArrayCount(mo->domain);  i++ )
    {
      ti = (i + shift) % HaArrayCount(mo->domain);
      dval = HaArray(mo->domain, ti);
      if( dval == 0 )
      {
        unassigned_tixels = KheSolnMatchingDefectCount(mo->solver->soln);
	time = KheInstanceTime(mo->instance, ti);
	if( KheMeetAssignTime(mo->meet, time) &&
	    KheSolnMatchingDefectCount(mo->solver->soln) > unassigned_tixels )
	{
	  tls->assign_count++;
	  KheMeetObjApplyExclusions(mo, ti);
	  if( mo->solver->unassignable == 0 )
	    KheTreeLayerSolve(tls, pos + 1, &junk);
	    /* mo->curr_asst = ti; */
	  KheMeetObjUnApplyExclusions(mo, ti);
	}
	KheMarkUndo(mark);
      }
      if( tls->assign_count > MAX_ASSIGNMENTS )
	break;
    }
    KheMarkEnd(mark, true);
    *end_index = i;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTreeLayerSolverDebug(KHE_TREE_LAYER_SOLVER tls,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tls onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheTreeLayerSolverDebug(KHE_TREE_LAYER_SOLVER tls,
  int verbosity, int indent, FILE *fp)
{
  KHE_MEET_OBJ mo;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ KheTreeLayerSolver(%s, init cost %.5f)\n", indent, "",
      KheResourceId(tls->resource), KheCostShow(tls->init_cost));
    HaArrayForEach(tls->meet_objs, mo, i)
      KheMeetObjDebug(mo, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTreeSearchLayerRepairTimes(KHE_SOLN soln, KHE_RESOURCE r)        */
/*                                                                           */
/*  Use a tree search to repair the layer of meets currently assigned r.     */
/*                                                                           */
/*****************************************************************************/

bool KheTreeSearchLayerRepairTimes(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_TREE_LAYER_SOLVER tls;  int i, j, end_index;  KHE_MEET_OBJ mo1, mo2;
  KHE_EXCLUSION_SET es;  bool success;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheTreeSearchLayerRepairTimes(soln, %s)\n",
      KheResourceId(r));

  /* make a solver */
  a = KheSolnArenaBegin(soln);
  tls = KheTreeLayerSolverMake(soln, r, a);

  /* reduce domains, using meet domains and resource unavailable times */
  HaArrayForEach(tls->meet_objs, mo1, i)
    KheMeetObjReduceDomain(mo1);

  /* add exclusion sets, but only where domains permit */
  HaArrayForEach(tls->meet_objs, mo1, i)
    KheMeetObjAddExclusionSets(mo1, tls->arena);

  /* add exclusions */
  HaArrayForEach(tls->meet_objs, mo1, i)
    for( j = i + 1;  j < HaArrayCount(tls->meet_objs);  j++ )
    {
      mo2 = HaArray(tls->meet_objs, j);
      KheMeetObjAddExclusions(mo1, mo2);
    }

  /* uniqueify exclusion sets */
  HaArrayForEach(tls->meet_objs, mo1, i)
    HaArrayForEach(mo1->exclusion_sets, es, j)
      if( es != NULL )
	KheExclusionSetUniqueify(es);

  if( DEBUG3 )
    KheTreeLayerSolverDebug(tls, 2, 2, stderr);

  /* carry out the search */
  tls->root_mark = KheMarkBegin(soln);
  KheMarkAddBestPath(tls->root_mark, 1);  /* curr soln is one option */
  HaArrayForEach(tls->meet_objs, mo1, i)
    if( KheMeetAsst(mo1->meet) != NULL )
      KheMeetUnAssign(mo1->meet);  /* part of KheTreeSearchLayerRepairTimes */
  KheTreeLayerSolve(tls, 0, &end_index);
  KheMarkUndo(tls->root_mark);
  KhePathRedo(KheMarkPath(tls->root_mark, 0));
  KheMarkEnd(tls->root_mark, false);

  /* delete the solver and return */
  HnAssert(KheSolnCost(soln) <= tls->init_cost,
    "KheTreeSearchLayerRepairTimes internal error 2");
  success = (KheSolnCost(soln) < tls->init_cost);
  if( DEBUG1 )
  {
    fprintf(stderr, "  assign %d, bottom %d, end %d\n",
	tls->assign_count, tls->bottom_count, end_index);
    if( success )
      fprintf(stderr,
	"] KheTreeSearchLayerRepairTimes returning true (%.5f -> %.5f)\n",
	KheCostShow(tls->init_cost), KheCostShow(KheSolnCost(soln)));
    else
      fprintf(stderr, "] KheTreeSearchLayerRepairTimes ret false\n");
  }
  KheSolnArenaEnd(soln, a);
  /* KheTreeLayerSolverDelete(tls); */
  return success;
}


/* *** old version, uses transactions
bool KheTreeSearchLa yerRepairTimes(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_TREE_LAYER_SOLVER tls;  int i, j, end_index;  KHE_MEET_OBJ mo1, mo2;
  KHE_EXCLUSION_SET es;  bool res;  KHE_TIME time;
  if( DEBUG1 )
    fprintf(stderr, "[ KheTreeSearc hLayerRepairTimes(soln, %s)\n",
      KheResourceId(r));

  ** make a solver **
  tls = KheTreeLayerSolverMake(soln, r);

  ** reduce domains, using meet domains and resource unavailable times **
  HaArrayForEach(tls->meet_objs, mo1, i)
    KheMeetObjReduceDomain(mo1);

  ** add exclusion sets, but only where domains permit **
  HaArrayForEach(tls->meet_objs, mo1, i)
    KheMeetObjAddExclusionSets(mo1);

  ** add exclusions **
  HaArrayForEach(tls->meet_objs, mo1, i)
    for( j = i + 1;  j < HaArrayCount(tls->meet_objs);  j++ )
    {
      mo2 = HaArray(tls->meet_objs, j);
      KheMeetObjAddExclusions(mo1, mo2);
    }

  ** uniqueify exclusion sets **
  HaArrayForEach(tls->meet_objs, mo1, i)
    HaArrayForEach(mo1->exclusion_sets, es, j)
      if( es != NULL )
	KheExclusionSetUniqueify(es);

  if( DEBUG3 )
    KheTreeLayerSolverDebug(tls, 2, 2, stderr);

  ** unassign the meets, but remember the assignments in init_trans **
  KheTransactionBegin(tls->init_trans);
  HaArrayForEach(tls->meet_objs, mo1, i)
    if( KheMeetAsst(mo1->meet) != NULL )
      KheMeetUnA ssign(mo1->meet);
  KheTransactionEnd(tls->init_trans);

  ** carry out the tree search and finalize the assignment, new or old **
  KheTreeLayerSolve(tls, 0, &end_index);
  if( tls->bes t_cost < tls->init_cost )
  {
    ** new best, redo it now **
    HaArrayForEach(tls->meet_objs, mo1, i)
    {
      time = KheInstanceTime(mo1->instance, mo1->best_asst);
      if( !KheMeetAssignTime(mo1->meet, time) )
	HnAbort("KheTreeSearchLa yerRepairTimes internal error 1");
    }
    HnAssert(KheSolnCost(soln) == tls->bes t_cost,
      "KheTreeSearchLay erRepairTimes internal error 2");
    res = true;
  }
  else
  {
    ** no new best, return to initial assignments **
    KheTransactionUndo(tls->init_trans);
    res = false;
  }

  ** delete the solver and return **
  HnAssert(KheSolnCost(soln) <= tls->init_cost,
    "KheTreeSearchLayer RepairTimes internal error 2");
  if( DEBUG1 )
  {
    fprintf(stderr, "  assign %d, bottom %d, end %d\n",
	tls->assign_count, tls->bottom_count, end_index);
    if( res )
      fprintf(stderr,
	"] KheTreeSearchLay erRepairTimes returning true (%.5f -> %.5f)\n",
	KheCostShow(tls->init_cost), KheCostShow(tls->bes t_cost));
    else
      fprintf(stderr,
	"] KheTreeSearchLay erRepairTimes ret false\n");
  }
  KheTreeLayerSolverDelete(tls);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasDefect(KHE_SOLN soln, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Return true if there is at least one defect among r's resource monitors. */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceHasDefect(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorCost(m) > KheMonitorLowerBound(m) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTreeSearchRepairTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,       */
/*    bool with_defects)                                                     */
/*                                                                           */
/*  Call KheTreeSearchLayerRepairTimes for each resource (or each resource   */
/*  of type rt, if rt is non-NULL) with or without resource defects.         */
/*                                                                           */
/*****************************************************************************/

bool KheTreeSearchRepairTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool with_defects)
{
  int i;  KHE_RESOURCE r;  KHE_INSTANCE ins;  bool res;
  res = false;
  if( rt != NULL )
  {
    for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
    {
      r = KheResourceTypeResource(rt, i);
      if( (!with_defects || KheResourceHasDefect(soln, r)) &&
	  KheTreeSearchLayerRepairTimes(soln, r) )
	res = true;
    }
  }
  else
  {
    ins = KheSolnInstance(soln);
    for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    {
      r = KheInstanceResource(ins, i);
      if( (!with_defects || KheResourceHasDefect(soln, r)) &&
	  KheTreeSearchLayerRepairTimes(soln, r) )
	res = true;
    }
  }
  return res;
}

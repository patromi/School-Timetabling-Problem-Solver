
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
/*  FILE:         khe_sr_tgrc_main.c                                         */
/*  DESCRIPTION:  Task grouping by resource constraints - main functions     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define	DEBUG3 0
#define	DEBUG4 0
#define	DEBUG10 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "combinatorial grouping"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheCombCoverTypesShow(ARRAY_KHE_COMB_COVER_TYPE *cover_types)      */
/*                                                                           */
/*  Brief display of an array of cover types.                                */
/*                                                                           */
/*****************************************************************************/

#define MAX_DISP 100

static int min(int a, int b) { return (a < b ? a : b); }

static char *KheCombCoverTypesShow(ARRAY_KHE_COMB_COVER_TYPE *cover_types)
{
  static char disp[MAX_DISP + 3];  int count, i;  char ch;
  count = min(HaArrayCount(*cover_types), MAX_DISP);
  disp[0] = '<';
  for( i = 0;  i < count;  i++ )
  {
    switch( HaArray(*cover_types, i) )
    {
      case KHE_COMB_COVER_YES:

	ch = 'Y';
	break;

      case KHE_COMB_COVER_NO:

	ch = 'N';
	break;

      case KHE_COMB_COVER_PREV:

	ch = 'P';
	break;

      case KHE_COMB_COVER_FREE:

	ch = 'F';
	break;

      default:

	ch = '?';
	break;
    }
    disp[i + 1] = ch;
  }
  disp[i + 1] = '>';
  disp[i + 2] = '\0';
  return disp;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoCombGroupingForTimeGroup(KHE_COMB_GROUPER cg,                   */
/*    KHE_TIME_GROUP tg, KHE_INTERVAL in, KHE_FRAME days_frame,              */
/*    ARRAY_KHE_COMB_COVER_TYPE *cover_types, KHE_SOLN_ADJUSTER sa)          */
/*                                                                           */
/*  Do combinatorial grouping for interval in of days_frame, for each mtask  */
/*  in tg, and return the number of groups made.                             */
/*                                                                           */
/*****************************************************************************/

static int KheDoCombGroupingForTimeGroup(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_INTERVAL in, KHE_FRAME days_frame,
  ARRAY_KHE_COMB_COVER_TYPE *cover_types, KHE_SOLN_ADJUSTER sa,
  KHE_MTASK_GROUPER scratch_mtg)
{
  KHE_RESOURCE_TYPE rt;  KHE_MTASK_SET mts;
  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;
  KHE_COMB_COVER_TYPE cover_type;
  int first_index, last_index, i, j, groups;
  KHE_TIME_GROUP tg2;

  first_index = KheIntervalFirst(in);
  last_index = KheIntervalLast(in);
  rt = KheCombGrouperResourceType(cg);
  mtf = KheCombGrouperMTaskFinder(cg);
  mts = KheMTaskFinderMTasksInTimeGroup(mtf, rt, tg);
  /* mg = KheMTaskGroupMake(cg); */
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskAssignIsFixed(mt) && KheMTaskNeedsAssignment(mt) )
    {
      if( DEBUG2 )
	fprintf(stderr, "    [ combinations using %s: %s",
	  KheTimeGroupId(tg), KheMTaskId(mt));
      KheCombGrouperClearRequirements(cg);
      KheCombGrouperAddMTaskRequirement(cg, mt, KHE_COMB_COVER_YES);
      for( j = first_index;  j <= last_index;  j++ )
      {
	tg2 = KheFrameTimeGroup(days_frame, j);
	if( j == first_index )
	  cover_type = KHE_COMB_COVER_FREE;
	else
	  cover_type = HaArray(*cover_types, j);
	KheCombGrouperAddTimeGroupRequirement(cg, tg2, cover_type);
      }
      KheMTaskGrouperClear(scratch_mtg);
      if( KheCombGrouperSolve(cg, KHE_COMB_VARIANT_SOLE_ZERO, scratch_mtg) )
	groups = KheMTaskGrouperMakeGroups(scratch_mtg, INT_MAX,
	  sa /* , "combinatorial grouping" */);
      else
	groups = 0;
      if( DEBUG2 )
	fprintf(stderr, "    ] %d groups\n", groups);
      if( groups > 0 )
	return groups;
    }
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoCombGroupingForInterval(KHE_COMB_GROUPER cg, KHE_INTERVAL in,   */
/*    KHE_FRAME days_frame, ARRAY_KHE_COMB_COVER_TYPE *cover_types,          */
/*    KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)                   */
/*                                                                           */
/*  Do combinatorial grouping for interval in of days_frame, and return      */
/*  the number of groups made.                                               */
/*                                                                           */
/*  Implementation note.  This code keeps trying for as long as it is        */
/*  finding groups in the interval, either forwards or backwards.            */
/*                                                                           */
/*****************************************************************************/

static int KheDoCombGroupingForInterval(KHE_COMB_GROUPER cg, KHE_INTERVAL in,
  KHE_FRAME days_frame, ARRAY_KHE_COMB_COVER_TYPE *cover_types,
  KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)
{
  int res, groups;  KHE_TIME_GROUP first_tg, last_tg;
  if( DEBUG4 )
    fprintf(stderr, "  [ KheDoCombGroupingForInterval(cg, %s, %s)\n",
      KheIntervalShow(in, days_frame), KheCombCoverTypesShow(cover_types));
  first_tg = KheFrameTimeGroup(days_frame, KheIntervalFirst(in));
  last_tg = KheFrameTimeGroup(days_frame, KheIntervalLast(in));
  res = 0;
  do
  {
    groups = KheDoCombGroupingForTimeGroup(cg, first_tg, in, days_frame,
      cover_types, sa, scratch_mtg);
    res += groups;
    if( groups == 0 && last_tg != first_tg )
    {
      groups = KheDoCombGroupingForTimeGroup(cg, last_tg, in, days_frame,
	cover_types, sa, scratch_mtg);
      res += groups;
    }
  } while( groups > 0 );

  if( DEBUG4 )
    fprintf(stderr, "  ] KheDoCombGroupingForInterval returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,            */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Do some combinatorial grouping.                                          */
/*                                                                           */
/*****************************************************************************/

static int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,
  KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)
{
  int max_days, len, first_index, days, total_groups;  KHE_MTASK_FINDER mtf;
  KHE_FRAME days_frame;
  KHE_EVENT_TIMETABLE_MONITOR etm;
  ARRAY_KHE_COMB_COVER_TYPE cover_types;

  /* get options */
  mtf = KheCombGrouperMTaskFinder(cg);
  max_days = KheOptionsGetInt(options, "rs_combinatorial_grouping_max_days", 3);
  days_frame = KheOptionsFrame(options, "gs_common_frame",
    KheMTaskFinderSoln(mtf));
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);

  /* eliminate combinations */
  if( DEBUG3 )
    fprintf(stderr, "  KheCombGrouping before KheElimCombSolve\n");
  HaArrayInit(cover_types, KheCombGrouperArena(cg));
  KheCombinationElimination(mtf, KheCombGrouperResourceType(cg),
    days_frame, etm, &cover_types);

  /* try each interval */
  total_groups = 0;
  days = KheFrameTimeGroupCount(days_frame);
  for( len = max_days;  len >= 2;  len-- )
  {
    if( DEBUG3 )
      fprintf(stderr, "  KheCombGrouping starting len %d\n", len);
    for( first_index = 0;  first_index <= days - len;  first_index++ )
      total_groups += KheDoCombGroupingForInterval(cg,
	KheIntervalMake(first_index, first_index + len - 1),
	days_frame, &cover_types, sa, scratch_mtg);
  }
  return total_groups;
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
   prof ile_off KheTaskerGroupingBuild(tr, 2, "BCDT special");
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheBCDTSpecialCaseGrouping()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,         */
/*    KHE_TASK_SET ts, bool history_off, bool comb_off, bool interval_off,   */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Carry out group by resource constraints for resource type rt.  There     */
/*  is no point in doing anything if there are fewer than two resources.     */
/*                                                                           */
/*****************************************************************************/

static int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa, bool comb_off, bool interval_off,
  KHE_OPTIONS options, HA_ARENA a, KHE_MTASK_GROUPER scratch_mtg)
{
  KHE_MTASK_FINDER mtf;  KHE_COMB_GROUPER cg;  KHE_FRAME days_frame;  int res;

  res = 0;
  if( KheResourceTypeResourceCount(rt) > 1 && (!comb_off || !interval_off) )
  {
    /* make an mtask finder */
    days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
    mtf = KheMTaskFinderMake(soln, rt, days_frame, true, a);

    /* combinatorial grouping */
    if( !comb_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) comb grouping:\n",
	  KheResourceTypeId(rt));
      cg = KheCombGrouperMake(mtf, rt, a);
      res += KheCombGrouping(cg, options, sa, scratch_mtg);
    }

    /* interval grouping */
    /* *** moved to khe_sr_interval_grouping.c
    if( !interval_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) interval grouping:\n",
	  KheResourceTypeId(rt));
      res += KheintervalGrouping(mtf, rt, sa);
      ** res += KhePro fileGrouping(cg, sa); **
    }
    *** */
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
/* ***
bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
*** */

int KheCombinatorialGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  int i, res;  KHE_INSTANCE ins;  HA_ARENA a;  KHE_MTASK_GROUPER scratch_mtg;

  /* boilerplate */
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByResourceConstraints(%s %s, -)\n",
      KheInstanceId(ins), rt == NULL ? "all types" : KheResourceTypeId(rt));

  /* return immediately if option rs_group_by_frame_off is true */
  /* ***
  if( KheOptionsGetBool(options, "rs_group_by_rc_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr,"] KheGroupByResourceConstraints returning false (off)\n");
    return false;
  }

  comb_off = KheOptionsGetBool(options, "rs_group_by_rc_combinatorial_off",
    false);
  interval_off=KheOptionsGetBool(options, "rs_group_by_rc_interval_off", false);
  *** */

  /* do grouping by resource constraints for each resource type */
  a = KheSolnArenaBegin(soln);
  scratch_mtg = KheMTaskGrouperMake(a);
  res = 0;
  if( rt != NULL )
    res += KheGroupForResourceType(soln, rt, sa, false, false, options, a,
      scratch_mtg);
  else for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    res += KheGroupForResourceType(soln, rt, sa, false, false, options, a,
      scratch_mtg);
  }
  KheSolnArenaEnd(soln, a);

  /* all done */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByResourceConstraints returning %d\n", res);
  return res;
}

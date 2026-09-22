
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         nrc_worker.c                                               */
/*  MODULE:       One worker (employee)                                      */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_OFF - time off for one worker                                   */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(NRC_SHIFT_SET) ARRAY_NRC_SHIFT_SET;

typedef struct nrc_time_off_or_on_rec {
  NRC_PENALTY		penalty;
  NRC_DAY_SET		days;
  NRC_SHIFT_SET		shifts;
  ARRAY_NRC_SHIFT_SET	shift_sets;
} *NRC_TIME_OFF_OR_ON;

typedef HA_ARRAY(NRC_TIME_OFF_OR_ON) ARRAY_NRC_TIME_OFF_OR_ON;


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER                                                               */
/*                                                                           */
/*  One worker (employee).                                                   */
/*                                                                           */
/*****************************************************************************/

typedef HN_TABLE(int) TABLE_INT;

struct nrc_worker_rec {
  NRC_INSTANCE			instance;
  char				*name;
  char				*converted_name;
  int				index_in_instance;
  NRC_WORKER_SET		singleton_worker_set;
  /* NRC_PENALTY		skill_penalty; */
  ARRAY_NRC_TIME_OFF_OR_ON	times_off;
  ARRAY_NRC_TIME_OFF_OR_ON	times_on;
  NRC_DAY			start_day;
  NRC_PENALTY			start_day_penalty;
  NRC_DAY			end_day;
  NRC_PENALTY			end_day_penalty;
  TABLE_INT			history_values;
  KHE_RESOURCE			resource;
};


/*****************************************************************************/
/*                                                                           */
/*  char *NrcWorkerMakeConvertedName(NRC_WORKER w, HA_ARENA a)               */
/*                                                                           */
/*  Make and return the converted name of w (the name that we will be        */
/*  known by in XESTT instances and solutions).                              */
/*                                                                           */
/*****************************************************************************/

static char *NrcWorkerMakeConvertedName(NRC_WORKER w, HA_ARENA a)
{
  if( strlen(w->name) > 8 )
    return HnStringMake(a, "%s%d",
      NrcInstanceWorkerWord(w->instance), w->index_in_instance + 1);
  else if( !is_letter(w->name[0]) )
    return HnStringMake(a, "%s%s",
      NrcInstanceWorkerWord(w->instance), w->name);
  else
    return w->name;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER NrcWorkerMake(NRC_INSTANCE ins, char *name)                   */
/*                                                                           */
/*  Make a new worker with these attributes, add it to ins, and return it.   */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER NrcWorkerMake(NRC_INSTANCE ins, char *name)
{
  NRC_WORKER res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  res->index_in_instance = NrcInstanceStaffingWorkerCount(ins);
  res->converted_name = NrcWorkerMakeConvertedName(res, a);
  res->singleton_worker_set = NULL;  /* created on demand */
  /* res->skill_penalty = NULL; */
  HaArrayInit(res->times_off, a);
  HaArrayInit(res->times_on, a);
  res->start_day = NULL;
  res->start_day_penalty = NULL;
  res->end_day = NULL;
  res->end_day_penalty = NULL;
  HnTableInit(res->history_values, a);
  res->resource = NULL;
  NrcInstanceStaffingAddWorker(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcWorkerName(NRC_WORKER w)                                        */
/*                                                                           */
/*  Return the name attribute of w.                                          */
/*                                                                           */
/*****************************************************************************/

char *NrcWorkerName(NRC_WORKER w)
{
  return w->name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcWorkerConvertedName(NRC_WORKER w)                               */
/*                                                                           */
/*  Return the converted_name attribute of w.                                */
/*                                                                           */
/*****************************************************************************/

char *NrcWorkerConvertedName(NRC_WORKER w)
{
  return w->converted_name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerIndex(NRC_WORKER w)                                         */
/*                                                                           */
/*  Return the index of w in the instance.                                   */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerIndex(NRC_WORKER w)
{
  return w->index_in_instance;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcWorkerSingletonWorkerSet(NRC_WORKER w)                 */
/*                                                                           */
/*  Return a worker set containing just w.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcWorkerSingletonWorkerSet(NRC_WORKER w)
{
  if( w->singleton_worker_set == NULL )
  {
    w->singleton_worker_set = NrcWorkerSetMake(w->instance, w->converted_name);
    NrcWorkerSetAddWorker(w->singleton_worker_set, w);
  }
  return w->singleton_worker_set;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "skill penalty"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddSkillPenalty(NRC_WORKER w, NRC_PENALTY p)               */
/*                                                                           */
/*  Add skill penalty p to w.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcWorkerAddSkillPenalty(NRC_WORKER w, NRC_PENALTY p)
{
  HnAssert(w->skill_penalty == NULL,
    "NrcWorkerAddSkillPenalty called twice for worker %s", w->name);
  w->skill_penalty = p;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcWorkerSkillPenalty(NRC_WORKER w)                          */
/*                                                                           */
/*  Return the skill penalty of w, or NULL if none.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY NrcWorkerSkillPenalty(NRC_WORKER w)
{
  return w->skill_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time off"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_OFF_OR_ON GetEntry(NRC_WORKER w, NRC_PENALTY p,                 */
/*    ARRAY_NRC_TIME_OFF_OR_ON *atoo)                                        */
/*                                                                           */
/*  Find or make an entry for (w, p).                                        */
/*                                                                           */
/*****************************************************************************/

static NRC_TIME_OFF_OR_ON GetEntry(NRC_WORKER w, NRC_PENALTY p,
  ARRAY_NRC_TIME_OFF_OR_ON *atoo)
{
  NRC_TIME_OFF_OR_ON too;  int i;  HA_ARENA a;
  a = NrcInstanceArena(w->instance);
  HaArrayForEach(*atoo, too, i)
    if( NrcPenaltyEqual(too->penalty, p) )
      return too;
  HaMake(too, a);
  too->penalty = p;
  too->days = NrcDaySetMake(w->instance, w->converted_name, w->converted_name);
  too->shifts = NrcShiftSetMake(w->instance);
  HaArrayInit(too->shift_sets, a);
  HaArrayAddLast(*atoo, too);
  return too;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddShiftOff(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p)      */
/*                                                                           */
/*  Add a request for w to have shift s off, with penalty p.                 */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddShiftOff(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_off);
  NrcShiftSetAddShift(too->shifts, s);
}


/*****************************************************************************/
/*                                                                           */
/* void NrcWorkerAddShiftSetOff(NRC_WORKER w, NRC_SHIFT_SET ss,NRC_PENALTY p)*/
/*                                                                           */
/*  Add a request for w to have ss off, with penalty p.                      */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddShiftSetOff(NRC_WORKER w, NRC_SHIFT_SET ss, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_off);
  HaArrayAddLast(too->shift_sets, ss);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddDayOff(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)          */
/*                                                                           */
/*  Add a request for w to have day d off, with penalty p.                   */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddDayOff(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_off);
  NrcDaySetAddDay(too->days, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddShiftOn(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p)       */
/*                                                                           */
/*  Add a request for w to have shift s on, with penalty p.                  */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddShiftOn(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_on);
  NrcShiftSetAddShift(too->shifts, s);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddShiftSetOn(NRC_WORKER w, NRC_SHIFT_SET ss,NRC_PENALTY p)*/
/*                                                                           */
/*  Add a request for w to have ss on, with penalty p.                       */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddShiftSetOn(NRC_WORKER w, NRC_SHIFT_SET ss, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_on);
  HaArrayAddLast(too->shift_sets, ss);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddDayOn(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)           */
/*                                                                           */
/*  Add a request for w to have day d on, with penalty p.                    */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddDayOn(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)
{
  NRC_TIME_OFF_OR_ON too;
  too = GetEntry(w, p, &w->times_on);
  NrcDaySetAddDay(too->days, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddStartDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)        */
/*                                                                           */
/*  Add start day to w.                                                      */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddStartDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)
{
  w->start_day = d;
  w->start_day_penalty = p;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddEndDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)          */
/*                                                                           */
/*  Add end day to w.                                                        */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddEndDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p)
{
  w->end_day = d;
  w->end_day_penalty = p;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "history"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerAddHistory(NRC_WORKER w, char *name, int value)            */
/*                                                                           */
/*  Add a history value.                                                     */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerAddHistory(NRC_WORKER w, char *name, int value)
{
  int junk;  /* NRC_HISTORY_VALUE rec; */  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcWorkerAddHistory(%s, \"%s\", %d)\n",
      w->name, name, value);
  a = NrcInstanceArena(w->instance);
  name = HnStringCopy(name, a);
  HnAssert(!NrcWorkerRetrieveHistory(w, name, &junk),
    "NrcWorkerAddHistoryTotal: total for %s already present", name);
  HnTableAdd(w->history_values, name, value);
  if( DEBUG1 )
    fprintf(stderr, "] NrcWorkerAddHistory\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerRetrieveHistory(NRC_WORKER w, char *name, int *value)      */
/*                                                                           */
/*  Retrieve a history value.                                                */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerRetrieveHistory(NRC_WORKER w, char *name, int *value)
{
  int pos;
  return HnTableRetrieve(w->history_values, name, *value, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerConvertToResource(NRC_WORKER w, KHE_RESOURCE_TYPE rt)      */
/*                                                                           */
/*  Convert w into a KHE resource, and store the resource within w.          */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerConvertToResource(NRC_WORKER w, KHE_RESOURCE_TYPE rt)
{
  HnAssert(w->resource == NULL, "NrcWorkerConvertToResource internal error");
  if( !KheResourceMake(rt, w->converted_name, w->converted_name, NULL,
	&w->resource) )
    HnAbort("NrcWorkerConvertToResource internal error (%s)",
      w->converted_name);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE NrcWorkerResource(NRC_WORKER w)                             */
/*                                                                           */
/*  Return the KHE resource that w was previously converted to.              */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE NrcWorkerResource(NRC_WORKER w)
{
  HnAssert(w->resource != NULL, "NrcWorkerResource internal error");
  return w->resource;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerConvertDayAndShiftOffRequests(NRC_WORKER w,                */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Convert the day and shift off requests of w to KHE constraints.          */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerConvertDayAndShiftOffRequests(NRC_WORKER w, KHE_INSTANCE ins)
{
  NRC_TIME_OFF_OR_ON too;  int i, j;  char *id, *name;  NRC_SHIFT s;
  NRC_SHIFT_SET ss; NRC_DAY d;  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;
  KHE_TIME_GROUP tg;  HA_ARENA a;
  a = NrcInstanceArena(w->instance);
  HaArrayForEach(w->times_off, too, i)
  {
    if( HaArrayCount(w->times_off) > 0 )
      id = HnStringMake(a, "TimeOff-%s-%s", w->converted_name,
	NrcPenaltyShow(too->penalty));
    else
      id = HnStringMake(a, "TimeOff-%s", w->converted_name);
    name = "Day or shift off request";
    if( !KheAvoidUnavailableTimesConstraintMake(ins, id, name,
	  NrcPenaltyConvert(too->penalty), &c) )
      HnAbort("cannot add AvoidUnavailableTimesConstraint %s", id);
    KheAvoidUnavailableTimesConstraintAddResource(c, w->resource);
    for( j = 0;  j < NrcShiftSetShiftCount(too->shifts);  j++ )
    {
      s = NrcShiftSetShift(too->shifts, j);
      KheAvoidUnavailableTimesConstraintAddTime(c, NrcShiftTime(s));
    }
    HaArrayForEach(too->shift_sets, ss, j)
    {
      tg = NrcShiftSetTimeGroup(ss, ins);
      KheAvoidUnavailableTimesConstraintAddTimeGroup(c, tg);
    }
    for( j = 0;  j < NrcDaySetDayCount(too->days);  j++ )
    {
      d = NrcDaySetDay(too->days, j);
      tg = NrcShiftSetTimeGroup(NrcDayShiftSet(d), ins);
      KheAvoidUnavailableTimesConstraintAddTimeGroup(c, tg);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerConvertDayAndShiftOnRequests(NRC_WORKER w,                 */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Convert the day and shift on requests of w to KHE constraints.           */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerConvertDayAndShiftOnRequests(NRC_WORKER w, KHE_INSTANCE ins)
{
  NRC_TIME_OFF_OR_ON too;  int i, j;  char *id, *name;  NRC_SHIFT s;
  NRC_SHIFT_SET ss;  NRC_DAY d;  KHE_TIME_GROUP tg;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c;  HA_ARENA a;
  a = NrcInstanceArena(w->instance);
  HaArrayForEach(w->times_on, too, i)
  {
    if( HaArrayCount(w->times_on) > 0 )
      id = HnStringMake(a, "TimeOn-%s-%s", w->converted_name,
	NrcPenaltyShow(too->penalty));
    else
      id = HnStringMake(a, "TimeOn-%s", w->converted_name);
    name = "Day or shift on request";
    if( !KheLimitBusyTimesConstraintMake(ins, id, name,
	  NrcPenaltyConvert(too->penalty), NULL, 1, 1000, false, &c) )
      HnAbort("cannot add LimitBusyTimesConstraint %s", id);
    KheLimitBusyTimesConstraintAddResource(c, w->resource);
    for( j = 0;  j < NrcShiftSetShiftCount(too->shifts);  j++ )
    {
      s = NrcShiftSetShift(too->shifts, j);
      tg = NrcShiftSetTimeGroup(NrcShiftSingletonShiftSet(s), ins);
      KheLimitBusyTimesConstraintAddTimeGroup(c, tg);
    }
    HaArrayForEach(too->shift_sets, ss, j)
    {
      tg = NrcShiftSetTimeGroup(ss, ins);
      KheLimitBusyTimesConstraintAddTimeGroup(c, tg);
    }
    for( j = 0;  j < NrcDaySetDayCount(too->days);  j++ )
    {
      d = NrcDaySetDay(too->days, j);
      tg = NrcShiftSetTimeGroup(NrcDayShiftSet(d), ins);
      KheLimitBusyTimesConstraintAddTimeGroup(c, tg);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*void NrcWorkerConvertStartAndEndDayRequests(NRC_WORKER w, KHE_INSTANCE ins)*/
/*                                                                           */
/*  Convert the start and end day requests for w into avoid unavailable      */
/*  times constraints.                                                       */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerConvertStartAndEndDayRequests(NRC_WORKER w, KHE_INSTANCE ins)
{
  int i, cycle_len;  char *id, *name;  NRC_DAY d;  HA_ARENA a;
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg;

  /* start day */
  a = NrcInstanceArena(w->instance);
  if( w->start_day != NULL && NrcPenaltyWeight(w->start_day_penalty) > 0 &&
      w->start_day != NrcInstanceCycleDay(w->instance, 0) )
  {
    id = HnStringMake(a, "StartDay-%s", w->converted_name);
    name = HnStringMake(a, "Start on day %s", NrcDayShortName(w->start_day));
    if( !KheAvoidUnavailableTimesConstraintMake(ins, id, name,
	NrcPenaltyConvert(w->start_day_penalty), &c) )
      HnAbort("cannot add AvoidUnavailableTimesConstraint %s", id);
    KheAvoidUnavailableTimesConstraintAddResource(c, w->resource);
    for( i = 0;  i < NrcDayIndexInCycle(w->start_day);  i++ )
    {
      d = NrcInstanceCycleDay(w->instance, i);
      tg = NrcShiftSetTimeGroup(NrcDayShiftSet(d), ins);
      KheAvoidUnavailableTimesConstraintAddTimeGroup(c, tg);
    }
  }

  /* end day */
  cycle_len = NrcInstanceCycleDayCount(w->instance);
  if( w->end_day != NULL && NrcPenaltyWeight(w->end_day_penalty) > 0 &&
      w->end_day != NrcInstanceCycleDay(w->instance, cycle_len - 1) )
  {
    id = HnStringMake(a, "EndDay-%s", w->converted_name);
    name = HnStringMake(a, "End on day %s", NrcDayShortName(w->end_day));
    if( !KheAvoidUnavailableTimesConstraintMake(ins, id, id,
	  NrcPenaltyConvert(w->end_day_penalty), &c) )
      HnAbort("cannot add AvoidUnavailableTimesConstraint %s", id);
    KheAvoidUnavailableTimesConstraintAddResource(c, w->resource);
    for( i = NrcDayIndexInCycle(w->end_day) + 1;  i < cycle_len;  i++ )
    {
      d = NrcInstanceCycleDay(w->instance, i);
      tg = NrcShiftSetTimeGroup(NrcDayShiftSet(d), ins);
      KheAvoidUnavailableTimesConstraintAddTimeGroup(c, tg);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerDebug(NRC_WORKER w, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of w onto fp with the given indent.                          */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerDebug(NRC_WORKER w, int indent, FILE *fp)
{
  NRC_TIME_OFF_OR_ON too;  int i, value, pos;  char *name;
  if( indent >= 0 )
  {
    if( strcmp(w->name, w->converted_name) != 0 )
      fprintf(fp, "%*s[ Worker %s (converted name %s):\n", indent, "",
	w->name, w->converted_name);
    else
      fprintf(fp, "%*s[ Worker %s:\n", indent, "", w->name);
    /* ***
    if( w->skill_penalty != NULL )
      fprintf(fp, "%*s  skill_penalty %s\n", indent, "",
	NrcPenaltyShow(w->skill_penalty, a));
    *** */
    if( HaArrayCount(w->times_off) > 0 )
    {
      fprintf(fp, "%*s  times_off:\n", indent, "");
      HaArrayForEach(w->times_off, too, i)
      {
	fprintf(fp, "%*s    penalty %s:\n", indent, "",
	  NrcPenaltyShow(too->penalty));
	NrcDaySetDebug(too->days, indent + 6, fp);
	NrcShiftSetDebug(too->shifts, indent + 6, fp);
      }
    }
    if( HaArrayCount(w->times_on) > 0 )
    {
      fprintf(fp, "%*s  times_on:\n", indent, "");
      HaArrayForEach(w->times_on, too, i)
      {
	fprintf(fp, "%*s    penalty %s:\n", indent, "",
	  NrcPenaltyShow(too->penalty));
	NrcDaySetDebug(too->days, indent + 6, fp);
	NrcShiftSetDebug(too->shifts, indent + 6, fp);
      }
    }
    HnTableForEach(w->history_values, name, value, pos)
      fprintf(fp, "%*s    history %s: %d\n", indent, "", name, value);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "%s", w->name);
}

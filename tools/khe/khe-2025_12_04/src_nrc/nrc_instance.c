
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XESTT CONVERTER                            */
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
/*  FILE:         nrc_instance.c                                             */
/*  DESCRIPTION:  One instance of the high school timetabling problem        */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE - one instance of the high school timetabling problem       */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_ARCHIVE) ARRAY_NRC_ARCHIVE;
typedef HA_ARRAY(NRC_DAY_SET) ARRAY_NRC_DAY_SET;
typedef HA_ARRAY(NRC_DAY_SET_SET) ARRAY_NRC_DAY_SET_SET;
typedef HA_ARRAY(NRC_SHIFT_TYPE_SET) ARRAY_NRC_SHIFT_TYPE_SET;
typedef HA_ARRAY(NRC_SHIFT) ARRAY_NRC_SHIFT;
typedef HA_ARRAY(NRC_SHIFT_SET) ARRAY_NRC_SHIFT_SET;
typedef HA_ARRAY(NRC_SHIFT_SET_SET) ARRAY_NRC_SHIFT_SET_SET;
typedef HN_TABLE(NRC_WORKER_SET) TABLE_NRC_WORKER_SET;
typedef HA_ARRAY(NRC_WORKER_SET) ARRAY_NRC_WORKER_SET;
typedef HA_ARRAY(NRC_WORKER_SET_SET) ARRAY_NRC_WORKER_SET_SET;
typedef HN_TABLE(NRC_DEMAND) TABLE_NRC_DEMAND;
typedef HA_ARRAY(NRC_DEMAND) ARRAY_NRC_DEMAND;
typedef HA_ARRAY(NRC_DEMAND_SET) ARRAY_NRC_DEMAND_SET;
typedef HA_ARRAY(NRC_PATTERN) ARRAY_NRC_PATTERN;
typedef HA_ARRAY(NRC_PATTERN_SET) ARRAY_NRC_PATTERN_SET;
typedef HA_ARRAY(NRC_DEMAND_CONSTRAINT) ARRAY_NRC_DEMAND_CONSTRAINT;
/* typedef HA_ARRAY(NRC_CONSTRAINT) ARRAY_NRC_CONSTRAINT; */

/* ***
typedef enum {
  NRC_UNIQUE_SKILL_PENALTY_UNKNOWN,
  NRC_UNIQUE_SKILL_PENALTY_FALSE,
  NRC_UNIQUE_SKILL_PENALTY_TRUE
} NRC_UNIQUE_SKILL_PENALTY;
*** */

struct nrc_instance_rec {

  /* miscellaneous */
  HA_ARENA_SET			arena_set;		/* arena             */
  HA_ARENA			arena;			/* arena             */
  ARRAY_NRC_ARCHIVE		archives;		/* archives          */
  char				*id;			/* optional id       */
  char				*meta_name;		/* metadata name     */
  char				*meta_contributor;	/* " contributor     */
  char				*meta_date;		/* " date            */
  char				*meta_country;		/* " country         */
  char				*meta_description;	/* " description     */
  char				*meta_remarks;		/* " remarks         */
  /* NRC_INSTANCE_METADATA	meta_data; */		/* instance metadata */
  /* char			*workers_word; */
  char				*worker_word;
  NRC_PENALTY			avoid_clashes_penalty;
  NRC_PENALTY			zero_penalty;
  /* NRC_UNIQUE_SKILL_PENALTY	unique_skill_penalty; */
  char				*short_day_names[7];
  char				*long_day_names[7];
  /* NRC_DEMAND			curr_demand; */		/* under construct. */
  bool				complete;		/* when complete    */

  /* arrays of stuff (it's better for initialization to have these first) */
  ARRAY_NRC_DAY_SET		day_sets;
  ARRAY_NRC_DAY_SET_SET		day_set_sets;
  ARRAY_NRC_SHIFT_TYPE_SET	shift_type_sets;
  ARRAY_NRC_SHIFT_SET		shift_sets;
  ARRAY_NRC_SHIFT_SET_SET	shift_set_sets;
  TABLE_NRC_WORKER_SET		worker_set_table;
  ARRAY_NRC_WORKER_SET		worker_sets;
  ARRAY_NRC_WORKER_SET_SET	worker_set_sets;
  TABLE_NRC_DEMAND		demand_table;
  ARRAY_NRC_DEMAND		demands;
  ARRAY_NRC_DEMAND_SET		demand_sets;
  ARRAY_NRC_PATTERN		patterns;
  ARRAY_NRC_PATTERN_SET		pattern_sets;
  ARRAY_NRC_DEMAND_CONSTRAINT	demand_constraints;
  ARRAY_NRC_CONSTRAINT		constraints;
  bool				has_workload_constraint;

  /* sets of various things, all of interest to the user */
  NRC_DAY_SET			cycle;
  NRC_DAY_SET_SET		days_of_week;
  NRC_SHIFT_TYPE_SET		shift_types;
  NRC_SHIFT_SET			shifts;
  NRC_SHIFT_SET			weekly_starting_ss;
  NRC_SHIFT_SET_SET		days_shift_set_set;
  NRC_SHIFT_SET_SET		weeks_shift_set_set;
  NRC_SHIFT_SET_SET		shifts_shift_set_set;
  NRC_WORKER_SET		staffing_ws;
  NRC_WORKER_SET		empty_ws;
  NRC_WORKER_SET_SET		contracts_wss;
  NRC_WORKER_SET_SET		skills_wss;
  int				total_workload;

  /* KHE stuff */
  KHE_INSTANCE			khe_instance;
  KHE_RESOURCE_GROUP		empty_rg;
  KHE_EVENT_GROUP		all_eg;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creating and querying instances"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcInstanceMakeBegin(char *id,                              */
/*    char *worker_word, HA_ARENA_SET as)                                    */
/*                                                                           */
/*  Make a new Nrc instance with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcInstanceMakeBegin(char *id,
  char *worker_word, HA_ARENA_SET as)
{
  NRC_INSTANCE res;  HA_ARENA a;

  /* miscellaneous */
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena_set = as;
  res->arena = a;
  HaArrayInit(res->archives, a);
  res->id = HnStringCopy(id, a);
  res->meta_name = NULL;
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_country = NULL;
  res->meta_description = NULL;
  res->meta_remarks = NULL;
  res->worker_word = HnStringCopy(worker_word, a);
  res->avoid_clashes_penalty = NrcPenalty(true, 1,NRC_COST_FUNCTION_LINEAR,res);
  res->zero_penalty = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, res);
  NrcInstanceSetDayNames(res, "Sun:Mon:Tue:Wed:Thu:Fri:Sat",
    "Sunday:Monday:Tuesday:Wednesday:Thursday:Friday:Saturday");
  res->complete = false;

  /* arrays of stuff (it's better for initialization to have these first) */
  HaArrayInit(res->day_sets, a);
  HaArrayInit(res->day_set_sets, a);
  HaArrayInit(res->shift_type_sets, a);
  HaArrayInit(res->shift_sets, a);
  HaArrayInit(res->shift_set_sets, a);
  HnTableInit(res->worker_set_table, a);
  HaArrayInit(res->worker_sets, a);
  HaArrayInit(res->worker_set_sets, a);
  HnTableInit(res->demand_table, a);
  HaArrayInit(res->demands, a);
  HaArrayInit(res->demand_sets, a);
  HaArrayInit(res->patterns, a);
  HaArrayInit(res->pattern_sets, a);
  HaArrayInit(res->demand_constraints, a);
  HaArrayInit(res->constraints, a);
  res->has_workload_constraint = false;

  /* sets of various things, all of interest to the user */
  res->cycle = NULL;
  res->days_of_week = NULL;
  res->shift_types = NrcShiftTypeSetMake(res, NULL);
  res->shifts = NrcShiftSetMake(res);
  res->weekly_starting_ss = NULL;
  res->days_shift_set_set = NULL;
  res->weeks_shift_set_set = NULL;
  res->shifts_shift_set_set = NULL;
  res->staffing_ws = NrcWorkerSetMake(res, WORKER_SET_ALL_NAME);
  res->empty_ws = NrcWorkerSetMake(res, WORKER_SET_EMPTY_NAME);
  res->contracts_wss = NrcWorkerSetSetMake(res);
  res->skills_wss = NrcWorkerSetSetMake(res);
  res->total_workload = 0;

  /* KHE stuff */
  res->khe_instance = NULL;
  res->empty_rg = NULL;
  res->all_eg = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceReplaceDemandConstraintsByDemands(NRC_INSTANCE ins)      */
/*                                                                           */
/*  Replace some demand constraints (the ones that can be so replaced)       */
/*  into demands.  This means that these constraints will ultimately be      */
/*  converted into assign resource and prefer resources constraints instead  */
/*  of limit resources constraints.                                          */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceReplaceDemandConstraintsByDemands(NRC_INSTANCE ins)
{
  NRC_SHIFT s;  int i;  NRC_DC_CONVERTER dcc;  NRC_DEMAND_CONSTRAINT dc;
  if( DEBUG4 )
    fprintf(stderr, "[ NrcInstanceReplaceDemandConstraintsByDemands\n");

  /* do the replacement */
  dcc = NrcDCConverterMake(ins);
  for( i = 0;  i < NrcShiftSetShiftCount(ins->shifts);  i++ )
  {
    s = NrcShiftSetShift(ins->shifts, i);
    NrcShiftReplaceDemandConstraintsByDemands(s, dcc);
  }

  /* delete defunct constraints */
  for( i = 0;  i < HaArrayCount(ins->demand_constraints);  i++ )
  {
    dc = HaArray(ins->demand_constraints, i);
    if( NrcDemandConstraintShiftSet(dc) == NULL )
    {
      HaArrayDeleteAndPlug(ins->demand_constraints, i);
      i--;
    }
  }

  if( DEBUG4 )
    fprintf(stderr,
      "] NrcInstanceReplaceDemandConstraintsByDemands returning\n");
}


/* *** obsolete version
static void NrcInsta nceConvertDemandConstraints(NRC_INSTANCE ins)
{
  NRC_DEMAND_CONSTRAINT dc;  int i;
  if( DEBUG4 )
    fprintf(stderr,
      "[ NrcInsta nceConvertDemandConstraints(%d constraints)\n",
      HaArrayCount(ins->demand_con straints));
  HaArrayForEach(ins->demand_co nstraints, dc, i)
    NrcDemandConstraintTryDemands(dc);
    ** *** dc will be marked defunct now instead of being removed
    if( NrcDemandConstraintTryDemands(dc) )
    {
      ** dc has been converted to demands; remove it from the list **
      HaArrayDeleteAndPlug(ins->demand_const raints, i);
      i--;
    }
    *** **
  if( DEBUG4 )
    fprintf(stderr,
      "] NrcInst anceConvertDemandConstraints returning "
      "(%d constraints)\n", HaArrayCount(ins->demand_const raints));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceMakeEnd(NRC_INSTANCE ins)                                */
/*                                                                           */
/*  End the creation of ins.                                                 */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceMakeEnd(NRC_INSTANCE ins)
{
  HnAssert(!ins->complete, "NrcInstanceMakeEnd called twice on %s", ins->id);
  NrcInstanceReplaceDemandConstraintsByDemands(ins);
  ins->complete = true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceComplete(NRC_INSTANCE ins)                               */
/*                                                                           */
/*  Return true if ins is complete.                                          */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceComplete(NRC_INSTANCE ins)
{
  return ins->complete;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA NrcInstanceArenaBegin(NRC_INSTANCE ins)                         */
/*                                                                           */
/*  Return a fresh arena.                                                    */
/*                                                                           */
/*****************************************************************************/

HA_ARENA NrcInstanceArenaBegin(NRC_INSTANCE ins)
{
  return HaArenaMake(ins->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceArenaEnd(NRC_INSTANCE ins, HA_ARENA a)                   */
/*                                                                           */
/*  Free and recycle arena a.                                                */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceArenaEnd(NRC_INSTANCE ins, HA_ARENA a)
{
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA NrcInstanceArena(NRC_INSTANCE ins)                              */
/*                                                                           */
/*  Return the arena attribute of ins.                                       */
/*                                                                           */
/*****************************************************************************/

HA_ARENA NrcInstanceArena(NRC_INSTANCE ins)
{
  return ins->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcInstanceWorkerWord(NRC_INSTANCE ins)                            */
/*                                                                           */
/*  Return the worker_word attribute of ins.                                 */
/*                                                                           */
/*****************************************************************************/

char *NrcInstanceWorkerWord(NRC_INSTANCE ins)
{
  return ins->worker_word;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcInstanceId(NRC_INSTANCE ins)                                    */
/*                                                                           */
/*  Return the Id attribute of ins.                                          */
/*                                                                           */
/*****************************************************************************/

char *NrcInstanceId(NRC_INSTANCE ins)
{
  return ins->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceSetAvoidClashesPenalty(NRC_INSTANCE ins, NRC_PENALTY p)  */
/*                                                                           */
/*  Set the avoid clashes penalty.                                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceSetAvoidClashesPenalty(NRC_INSTANCE ins, NRC_PENALTY p)
{
  ins->avoid_clashes_penalty = p;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcInstanceAvoidClashesPenalty(NRC_INSTANCE ins)             */
/*                                                                           */
/*  Return the avoid_clashes_penalty attribute of ins.                       */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY NrcInstanceAvoidClashesPenalty(NRC_INSTANCE ins)
{
  return ins->avoid_clashes_penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcInstanceZeroPenalty(NRC_INSTANCE ins)                     */
/*                                                                           */
/*  Return a zero penalty.                                                   */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY NrcInstanceZeroPenalty(NRC_INSTANCE ins)
{
  return ins->zero_penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceSetMetaData(NRC_INSTANCE ins, char *name,                */
/*    char *contributor, char *date, char *country, char *description,       */
/*    char *remarks)                                                         */
/*                                                                           */
/*  Set the metadata fields of ins.                                          */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceSetMetaData(NRC_INSTANCE ins, char *name, char *contributor,
  char *date, char *country, char *description, char *remarks)
{
  ins->meta_name = HnStringCopy(name, ins->arena);
  ins->meta_contributor = HnStringCopy(contributor, ins->arena);
  ins->meta_date = HnStringCopy(date, ins->arena);
  ins->meta_country = HnStringCopy(country, ins->arena);
  ins->meta_description = HnStringCopy(description, ins->arena);
  ins->meta_remarks = HnStringCopy(remarks, ins->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceMetaData(NRC_INSTANCE ins, char **name,                  */
/*    char **contributor, char **date, char **country, char **description,   */
/*    char **remarks)                                                        */
/*                                                                           */
/*  Retrieve the metadata fields of ins.                                     */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceMetaData(NRC_INSTANCE ins, char **name, char **contributor,
  char **date, char **country, char **description, char **remarks)
{
  *name = ins->meta_name;
  *contributor = ins->meta_contributor;
  *date = ins->meta_date;
  *country = ins->meta_country;
  *description = ins->meta_description;
  *remarks = ins->meta_remarks;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE_METADATA NrcInstanceMetaData(NRC_INSTANCE ins)              */
/*                                                                           */
/*  Return the metadata attribute of ins.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_INSTANCE_METADATA NrcInstanceMetaData(NRC_INSTANCE ins)
{
  return ins->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "archives"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive)        */
/*                                                                           */
/*  Add archive to ins, assuming it is safe to do so.                        */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive)
{
  HaArrayAddLast(ins->archives, archive);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDeleteArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive)     */
/*                                                                           */
/*  Delete archive from ins.                                                 */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceDeleteArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive)
{
  int pos;
  if( !HaArrayContains(ins->archives, archive, &pos) )
    HnAbort("NrcInstanceDeleteArchive internal error");
  HaArrayDeleteAndShift(ins->archives, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceArchiveCount(NRC_INSTANCE ins)                            */
/*                                                                           */
/*  Return the number of archives containing ins.                            */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceArchiveCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->archives);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_ARCHIVE NrcInstanceArchive(NRC_INSTANCE ins, int i)                  */
/*                                                                           */
/*  Return the i'th archive containing ins.                                  */
/*                                                                           */
/*****************************************************************************/

NRC_ARCHIVE NrcInstanceArchive(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->archives, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "day names, days, day-sets, and day-set sets"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void Separate(char *names, char *(*res)[7], int len, HA_ARENA a)         */
/*                                                                           */
/*  Separate names, make sure there are len of them, and store them in res.  */
/*                                                                           */
/*****************************************************************************/

static void Separate(char *names, char *(*res)[7], int len, HA_ARENA a)
{
  int count;  char *s, *p;
  s = HnStringCopy(names, a);
  count = 0;
  (*res)[count++] = s;
  for( p = s;  *p != '\0';  p++ )
  {
    if( *p == ':' )
    {
      *p = '\0';
      HnAssert(count < len,"NrcInstanceSetDayNames: more than %d names in %s\n",
	len, names);
      (*res)[count++] = p + 1;
    }
  }
  HnAssert(count == len, "NrcInstanceSetDayNames: fewer than %d names in %s\n",
    len, names);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceSetDayNames(NRC_INSTANCE ins, char *short_names,         */
/*    char *long_names)                                                      */
/*                                                                           */
/*  Set the short and long names of the days, and return them.               */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceSetDayNames(NRC_INSTANCE ins, char *short_names,
  char *long_names)
{
  if( DEBUG5 )
    fprintf(stderr, "[ NrcInstanceSetDayNames(ins, \"%s\", \"%s\")\n",
      short_names, long_names);
  Separate(short_names, &ins->short_day_names, 7, ins->arena);
  Separate(long_names, &ins->long_day_names, 7, ins->arena);
  if( DEBUG5 )
  {
    int i;
    for( i = 0;  i < 7;  i++ )
      fprintf(stderr, "ins->short_day_names[%d] = \"%s\"\n", i,
        ins->short_day_names[i]);
    for( i = 0;  i < 7;  i++ )
      fprintf(stderr, "ins->long_day_names[%d] = \"%s\"\n", i,
        ins->long_day_names[i]);
    fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDayNameCount(NRC_INSTANCE ins)                            */
/*                                                                           */
/*  Return the number of day names (i.e. 7).                                 */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDayNameCount(NRC_INSTANCE ins)
{
  return 7;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcInstanceShortDayName(NRC_INSTANCE ins, int i)                   */
/*                                                                           */
/*  Return the short name of the i'th day, where i == 0 means Monday.        */
/*                                                                           */
/*****************************************************************************/

char *NrcInstanceShortDayName(NRC_INSTANCE ins, int i)
{
  return ins->short_day_names[i];
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcInstanceLongDayName(NRC_INSTANCE ins, int i)                    */
/*                                                                           */
/*  Return the long name of the i'th day, where i == 0 means Monday.         */
/*                                                                           */
/*****************************************************************************/

char *NrcInstanceLongDayName(NRC_INSTANCE ins, int i)
{
  return ins->long_day_names[i];
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceWeekShiftSetSetMake(NRC_INSTANCE ins)                    */
/*                                                                           */
/*  Make a weeks shift-set set, but only if there are more than 7 days.      */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceWeekShiftSetSetMake(NRC_INSTANCE ins)
{
  int i, j;  NRC_SHIFT_SET ss;  NRC_DAY d;
  if( DEBUG8 )
    fprintf(stderr, "[ NrcInstanceWeekShiftSetSetMake(%s)\n",
      NrcInstanceId(ins));
  HnAssert(ins->weeks_shift_set_set == NULL,
    "NrcInstanceWeekShiftSetSetMake internal error");
  if( NrcDaySetDayCount(ins->cycle) > 7 )
  {
    ins->weeks_shift_set_set = NrcShiftSetSetMake(ins);
    for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i += 7 )
    {
      ss = NrcShiftSetMakeInternal(ins, KHE_TIME_GROUP_KIND_WEEK);
      for( j = 0;  j < 7;  j++ )
	if( i + j < NrcDaySetDayCount(ins->cycle) )
	{
	  d = NrcDaySetDay(ins->cycle, i + j);
	  NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(d));
	}
      NrcShiftSetSetAddShiftSet(ins->weeks_shift_set_set, ss);
      if( DEBUG8 )
	NrcShiftSetDebug(ss, 2, stderr);
    }
    if( DEBUG8 )
      fprintf(stderr, "  %d weeks\n",
	NrcShiftSetSetShiftSetCount(ins->weeks_shift_set_set));
  }
  if( DEBUG8 )
    fprintf(stderr, "] NrcInstanceWeekShiftSetSetMake\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceMakeAllShifts(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Make the shifts of ins, one day's worth at a time.                       */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceMakeAllShifts(NRC_INSTANCE ins)
{
  NRC_DAY day;  NRC_SHIFT_TYPE st;  int i, j;
  for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i++ )
  {
    day = NrcDaySetDay(ins->cycle, i);
    for( j = 0;  j < NrcShiftTypeSetShiftTypeCount(ins->shift_types);  j++ )
    {
      st = NrcShiftTypeSetShiftType(ins->shift_types, j);
      NrcShiftMake(ins, day, st);
    }
  }
}

/*****************************************************************************/
/*                                                                           */
/*  void NrcCycleMake(NRC_INSTANCE ins, int day_count, int first_day_index)  */
/*                                                                           */
/*  Add a cycle to ins, and one shift per day for each shift type.           */
/*                                                                           */
/*****************************************************************************/

void NrcCycleMake(NRC_INSTANCE ins, int day_count, int first_day_index)
{
  /* NRC_SHIFT_TYPE st;  int i, j; */
  HnAssert(ins->cycle == NULL, "NrcCycleMake: cycle already present");
  ins->cycle = NrcDaySetMakeCycle(ins, day_count, first_day_index,
    &ins->days_of_week);
  NrcInstanceMakeAllShifts(ins);
  /* ***
  for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(ins->shift_types);  i++ )
  {
    st = NrcShiftTypeSetShiftType(ins->shift_types, i);
    for( j = 0;  j < NrcDaySetDayCount(ins->cycle);  j++ )
      NrcShiftMake( ins, NrcDaySetDay(ins->cycle, j), st);
  }
  *** */
  NrcInstanceWeekShiftSetSetMake(ins);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCalendarCycleMake(NRC_INSTANCE ins,                              */
/*    char *start_ymd, char *end_ymd, char **err_str)                        */
/*                                                                           */
/*  Add a calendar cycle to ins, and one shift per day for each shift type.  */
/*                                                                           */
/*****************************************************************************/

bool NrcCalendarCycleMake(NRC_INSTANCE ins,
  char *start_ymd, char *end_ymd, char **err_str)
{
  /* NRC_SHIFT_TYPE st;  int i, j; */
  HnAssert(ins->cycle == NULL, "NrcCalendarCycleMake: cycle already present");
  ins->cycle = NrcDaySetMakeCalendarCycle(ins, start_ymd, end_ymd,
    &ins->days_of_week, err_str);
  if( ins->cycle != NULL )
  {
    NrcInstanceMakeAllShifts(ins);
    /* ***
    for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(ins->shift_types);  i++ )
    {
      st = NrcShiftTypeSetShiftType(ins->shift_types, i);
      for( j = 0;  j < NrcDaySetDayCount(ins->cycle);  j++ )
	NrcShiftMake(ins, NrcDaySetDay(ins->cycle, j), st);
    }
    *** */
    NrcInstanceWeekShiftSetSetMake(ins);
  }
  return ins->cycle != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcInstanceCycle(NRC_INSTANCE ins)                           */
/*                                                                           */
/*  Return ins's cycle, as a day-set.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcInstanceCycle(NRC_INSTANCE ins)
{
  HnAssert(ins->cycle != NULL, "NrcInstanceCycle: no cycle in ins");
  return ins->cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceCycleDayCount(NRC_INSTANCE ins)                           */
/*                                                                           */
/*  Return the number of days in ins's cycle.                                */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceCycleDayCount(NRC_INSTANCE ins)
{
  HnAssert(ins->cycle != NULL, "NrcInstanceCycleDayCount: no cycle in ins");
  return NrcDaySetDayCount(ins->cycle);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcInstanceCycleDay(NRC_INSTANCE ins, int i)                     */
/*                                                                           */
/*  Return the i'th day of ins's cycle.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcInstanceCycleDay(NRC_INSTANCE ins, int i)
{
  HnAssert(ins->cycle != NULL, "NrcInstanceCycleDay: no cycle in ins");
  return NrcDaySetDay(ins->cycle, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceCycleRetrieveDay(NRC_INSTANCE ins, char *ymd,            */
/*    NRC_DAY *d)                                                            */
/*                                                                           */
/*  Retrieve a day with the given ymd from ins's cycle.                      */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceCycleRetrieveDay(NRC_INSTANCE ins, char *ymd, NRC_DAY *d)
{
  return NrcDaySetRetrieveDay(ins->cycle, ymd, d);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET_SET NrcInstanceDaysOfWeek(NRC_INSTANCE ins)                  */
/*                                                                           */
/*  Return ins's days of the week, as a day-set set.                         */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET_SET NrcInstanceDaysOfWeek(NRC_INSTANCE ins)
{
  HnAssert(ins->cycle != NULL, "NrcInstanceDaysOfWeek: no cycle in ins");
  return ins->days_of_week;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDaysOfWeekDaySetCount(NRC_INSTANCE ins)                   */
/*                                                                           */
/*  Return the number of days of the week (always 7).                        */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDaysOfWeekDaySetCount(NRC_INSTANCE ins)
{
  HnAssert(ins->cycle != NULL,
    "NrcInstanceDaysOfWeekDaySetCount: no cycle in ins");
  return NrcDaySetSetDaySetCount(ins->days_of_week);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcInstanceDaysOfWeekDaySet(NRC_INSTANCE ins, int i)         */
/*                                                                           */
/*  Return the i'th day set of ins's days of the week.                       */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcInstanceDaysOfWeekDaySet(NRC_INSTANCE ins, int i)
{
  HnAssert(ins->cycle != NULL, "NrcInstanceDaysOfWeekDaySet: no cycle in ins");
  return NrcDaySetSetDaySet(ins->days_of_week, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceDaysOfWeekRetrieveDaySetShort(NRC_INSTANCE ins,          */
/*    char *short_name, NRC_DAY_SET *ds)                                     */
/*                                                                           */
/*  Retrieve a day set from ins's days of the week by short name.            */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceDaysOfWeekRetrieveDaySetShort(NRC_INSTANCE ins,
  char *short_name, NRC_DAY_SET *ds)
{
  HnAssert(ins->cycle != NULL,
    "NrcInstanceDaysOfWeekRetrieveDaySetShort: no cycle in ins");
  return NrcDaySetSetRetrieveDaySetShort(ins->days_of_week, short_name, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceDaysOfWeekRetrieveDaySetLong(NRC_INSTANCE ins,           */
/*    char *long_name, NRC_DAY_SET *ds)                                      */
/*                                                                           */
/*  Retrieve a day set from ins's days of the week by long name.             */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceDaysOfWeekRetrieveDaySetLong(NRC_INSTANCE ins,
  char *long_name, NRC_DAY_SET *ds)
{
  HnAssert(ins->cycle != NULL,
    "NrcInstanceDaysOfWeekRetrieveDaySetLong: no cycle in ins");
  return NrcDaySetSetRetrieveDaySetLong(ins->days_of_week, long_name, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddDaySet(NRC_INSTANCE ins, NRC_DAY_SET ds)              */
/*                                                                           */
/*  Add ds to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddDaySet(NRC_INSTANCE ins, NRC_DAY_SET ds)
{
  HaArrayAddLast(ins->day_sets, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDaySetCount(NRC_INSTANCE ins)                             */
/*                                                                           */
/*  Return the number of day sets of the instance.                           */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDaySetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->day_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcInstanceDaySet(NRC_INSTANCE ins, int i)                   */
/*                                                                           */
/*  Return the i'th day set of the instance.                                 */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcInstanceDaySet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->day_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddDaySetSet(NRC_INSTANCE ins, NRC_DAY_SET_SET dss)      */
/*                                                                           */
/*  Add dss to ins.                                                          */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddDaySetSet(NRC_INSTANCE ins, NRC_DAY_SET_SET dss)
{
  HaArrayAddLast(ins->day_set_sets, dss);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDaySetSetCount(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of day-set sets of the instance.                       */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDaySetSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->day_set_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET_SET NrcInstanceDaySetSet(NRC_INSTANCE ins, int i)            */
/*                                                                           */
/*  Return the i'th day-set set of the instance.                             */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET_SET NrcInstanceDaySetSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->day_set_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift types"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceAddShiftType(NRC_INSTANCE ins, NRC_SHIFT_TYPE st)         */
/*                                                                           */
/*  Add st to ins, and return its index, after checking that st's name       */
/*  has not already been used.  Also add one shift of the new type for       */
/*  each day, if there are days.                                             */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceAddShiftType(NRC_INSTANCE ins, NRC_SHIFT_TYPE st)
{
  int res, i;  NRC_SHIFT_TYPE st2;  /* NRC_SHIFT s;  NRC_SHIFT_SET ss; */
  if( DEBUG6 )
    fprintf(stderr, "[ NrcInstanceAddShiftType(ins, \"%s\")\n",
      NrcShiftTypeName(st));

  /* shift types must be added before the cycle is added */
  HnAssert(ins->cycle == NULL, "NrcShiftTypeMake called after cycle added");

  /* check that the shift type's name is not already in use */
  res = NrcShiftTypeSetShiftTypeCount(ins->shift_types);
  for( i = 0;  i < res;  i++ )
  {
    st2 = NrcShiftTypeSetShiftType(ins->shift_types, i);
    if( strcmp(NrcShiftTypeName(st2), NrcShiftTypeName(st)) == 0 )
      HnAbort("NrcInstanceAddShiftType: shift type name %s already used",
	NrcShiftTypeName(st));
  }

  /* add the shift type */
  NrcShiftTypeSetAddShiftType(ins->shift_types, st);

  /* add one shift for each day of the cycle, if there is a cycle */
  /* ***
  if( ins->cycle != NULL )
  {
    for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i++ )
    {
      s = NrcShift Make(ins, NrcDaySetDay(ins->cycle, i), st);
      if( ins->weeks_shift_set_set != NULL )
      {
	ss = NrcShiftSetSetShiftSet(ins->weeks_shift_set_set, i / 7);
	NrcShiftSetAddShift(ss, s);
      }
    }
  }
  *** */
  if( DEBUG6 )
    fprintf(stderr, "] NrcInstanceAddShiftType returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET NrcInstanceAllShiftTypes(NRC_INSTANCE ins)            */
/*                                                                           */
/*  Return the set of all shift types.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE_SET NrcInstanceAllShiftTypes(NRC_INSTANCE ins)
{
  return ins->shift_types;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceShiftTypeCount(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of shift types in ins.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceShiftTypeCount(NRC_INSTANCE ins)
{
  return NrcShiftTypeSetShiftTypeCount(ins->shift_types);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE NrcInstanceShiftType(NRC_INSTANCE ins, int i)             */
/*                                                                           */
/*  Return the i'th shift type of ins.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE NrcInstanceShiftType(NRC_INSTANCE ins, int i)
{
  return NrcShiftTypeSetShiftType(ins->shift_types, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrieveShiftType(NRC_INSTANCE ins, char *name,          */
/*    NRC_SHIFT_TYPE *st)                                                    */
/*                                                                           */
/*  Retrieve a shift type with the given name from ins.                      */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrieveShiftType(NRC_INSTANCE ins, char *name,
  NRC_SHIFT_TYPE *st)
{
  NRC_SHIFT_TYPE st2;  int i;
  for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(ins->shift_types);  i++ )
  {
    st2 = NrcShiftTypeSetShiftType(ins->shift_types, i);
    if( strcmp(name, NrcShiftTypeName(st2)) == 0 )
    {
      *st = st2;
      return true;
    }
  }
  *st = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrieveShiftTypeByLabel(NRC_INSTANCE ins, char *label,  */
/*    NRC_SHIFT_TYPE *st)                                                    */
/*                                                                           */
/*  Retrieve a shift type with the given label from ins.                     */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrieveShiftTypeByLabel(NRC_INSTANCE ins, char *label,
  NRC_SHIFT_TYPE *st)
{
  NRC_SHIFT_TYPE st2;  int i;  char *st_label;
  for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(ins->shift_types);  i++ )
  {
    st2 = NrcShiftTypeSetShiftType(ins->shift_types, i);
    st_label = NrcShiftTypeLabel(st2);
    if( st_label != NULL && strcmp(label, st_label) == 0 )
    {
      *st = st2;
      return true;
    }
  }
  *st = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift-type sets"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddShiftTypeSet(NRC_INSTANCE ins, NRC_SHIFT_TYPE_SET sts)*/
/*                                                                           */
/*  Add sts to the list of shift-type sets of ins.                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddShiftTypeSet(NRC_INSTANCE ins, NRC_SHIFT_TYPE_SET sts)
{
  HaArrayAddLast(ins->shift_type_sets, sts);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceShiftTypeSetCount(NRC_INSTANCE ins)                       */
/*                                                                           */
/*  Return the number of shift-type sets of ins.                             */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceShiftTypeSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->shift_type_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET NrcInstanceShiftTypeSet(NRC_INSTANCE ins, int i)      */
/*                                                                           */
/*  Return the i'th shift-type set of ins.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE_SET NrcInstanceShiftTypeSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->shift_type_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrieveShiftType(NRC_INSTANCE ins, char *name,          */
/*    NRC_SHIFT_TYPE *st)                                                    */
/*                                                                           */
/*  Retrieve a shift type set with the given name from ins.                  */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrieveShiftTypeSet(NRC_INSTANCE ins, char *name,
  NRC_SHIFT_TYPE_SET *sts)
{
  NRC_SHIFT_TYPE_SET sts2;  int i;
  for( i = 0;  i < HaArrayCount(ins->shift_type_sets);  i++ )
  {
    sts2 = HaArray(ins->shift_type_sets, i);
    if( NrcShiftTypeSetName(sts2) != NULL &&
	strcmp(name, NrcShiftTypeSetName(sts2)) == 0 )
    {
      *sts = sts2;
      return true;
    }
  }
  *sts = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shifts"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcInstanceAllShifts(NRC_INSTANCE ins)                     */
/*                                                                           */
/*  Return the shifts of ins as a shift-set.                                 */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcInstanceAllShifts(NRC_INSTANCE ins)
{
  return ins->shifts;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceAddShift(NRC_INSTANCE ins, NRC_SHIFT s)                   */
/*                                                                           */
/*  Add s to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceAddShift(NRC_INSTANCE ins, NRC_SHIFT s)
{
  int res;
  res = NrcShiftSetShiftCount(ins->shifts);
  NrcShiftSetAddShift(ins->shifts, s);
  ins->total_workload += NrcShiftWorkload(s);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceShiftCount(NRC_INSTANCE ins)                              */
/*                                                                           */
/*  Return the number of shifts in ins.                                      */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceShiftCount(NRC_INSTANCE ins)
{
  return NrcShiftSetShiftCount(ins->shifts);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT NrcInstanceShift(NRC_INSTANCE ins, int i)                      */
/*                                                                           */
/*  Return the i'th shift of ins.                                            */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT NrcInstanceShift(NRC_INSTANCE ins, int i)
{
  return NrcShiftSetShift(ins->shifts, i);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcInstanceDailyStartingShiftSet(NRC_INSTANCE ins)         */
/*                                                                           */
/*  Return a shift-set containing, for each day of the cycle, the first      */
/*  shift of that day.                                                       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcInstanceDailyStartingShiftSet(NRC_INSTANCE ins)
{
  return NrcDaySetStartingShiftSet(ins->cycle);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcInstanceWeeklyStartingShiftSet(NRC_INSTANCE ins)        */
/*                                                                           */
/*  Return a shift-set containing, for each week, the first shift of the     */
/*  first day of the week.                                                   */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcInstanceWeeklyStartingShiftSet(NRC_INSTANCE ins)
{
  int i;  NRC_SHIFT_SET ss;
  if( ins->weekly_starting_ss == NULL )
  {
    ins->weekly_starting_ss = NrcShiftSetMake(ins);
    for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i += 7 )
    {
      ss = NrcDayShiftSet(NrcDaySetDay(ins->cycle, i));
      HnAssert(NrcShiftSetShiftCount(ss) > 0,
	"NrcInstanceWeeklyStartingShiftSet: empty shift set for day");
      NrcShiftSetAddShift(ins->weekly_starting_ss, NrcShiftSetShift(ss, 0));
    }
  }
  return ins->weekly_starting_ss;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift-sets"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddShiftSet(NRC_INSTANCE ins, NRC_SHIFT_SET ss)          */
/*                                                                           */
/*  Add ss to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddShiftSet(NRC_INSTANCE ins, NRC_SHIFT_SET ss)
{
  HaArrayAddLast(ins->shift_sets, ss);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceShiftSetCount(NRC_INSTANCE ins)                           */
/*                                                                           */
/*  Return the number of shift-sets in ins.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceShiftSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->shift_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcInstanceShiftSet(NRC_INSTANCE ins, int i)               */
/*                                                                           */
/*  Return the i'th shift-set of ins.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcInstanceShiftSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->shift_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift-set sets"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddShiftSetSet(NRC_INSTANCE ins, NRC_SHIFT_SET_SET sss)  */
/*                                                                           */
/*  Add sss to ins.                                                          */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddShiftSetSet(NRC_INSTANCE ins, NRC_SHIFT_SET_SET sss)
{
  HaArrayAddLast(ins->shift_set_sets, sss);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceShiftSetSetCount(NRC_INSTANCE ins)                        */
/*                                                                           */
/*  Return the number of shift-set sets in ins.                              */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceShiftSetSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->shift_set_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcInstanceShiftSetSet(NRC_INSTANCE ins, int i)        */
/*                                                                           */
/*  Return the i'th shift-set set of ins.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcInstanceShiftSetSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->shift_set_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcInstanceDaysShiftSetSet(NRC_INSTANCE ins)           */
/*                                                                           */
/*  Return a shift-set set containing one shift-set for each day of the      */
/*  cycle.  The shift-set contains the shifts of its day.                    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcInstanceDaysShiftSetSet(NRC_INSTANCE ins)
{
  int i;  NRC_DAY d;
  if( ins->days_shift_set_set == NULL )
  {
    ins->days_shift_set_set = NrcShiftSetSetMake(ins);
    for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i++ )
    {
      d = NrcDaySetDay(ins->cycle, i);
      NrcShiftSetSetAddShiftSet(ins->days_shift_set_set, NrcDayShiftSet(d));
    }
  }
  return ins->days_shift_set_set;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcInstanceShiftsShiftSetSet(NRC_INSTANCE ins)         */
/*                                                                           */
/*  Return a shift-set set containing one shift-set for each shift of the    */
/*  cycle.  The shift-set contains just that shift.                          */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcInstanceShiftsShiftSetSet(NRC_INSTANCE ins)
{
  int i, j;  NRC_DAY d;  NRC_SHIFT_SET dss;  NRC_SHIFT s;
  if( ins->shifts_shift_set_set == NULL )
  {
    ins->shifts_shift_set_set = NrcShiftSetSetMake(ins);
    for( i = 0;  i < NrcDaySetDayCount(ins->cycle);  i++ )
    {
      d = NrcDaySetDay(ins->cycle, i);
      dss = NrcDayShiftSet(d);
      for( j = 0;  j < NrcShiftSetShiftCount(dss);  j++ )
      {
	s = NrcShiftSetShift(dss, j);
	NrcShiftSetSetAddShiftSet(ins->shifts_shift_set_set,
	  NrcShiftSingletonShiftSet(s));
      }
    }
  }
  return ins->shifts_shift_set_set;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "staffing"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceAddWorker(NRC_INSTANCE ins, NRC_WORKER w)                 */
/*                                                                           */
/*  Add w to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceStaffingAddWorker(NRC_INSTANCE ins, NRC_WORKER w)
{
  NrcWorkerSetAddWorker(ins->staffing_ws, w);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcInstanceStaffing(NRC_INSTANCE ins)                     */
/*                                                                           */
/*  Return the staffing worker set of ins.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcInstanceStaffing(NRC_INSTANCE ins)
{
   return ins->staffing_ws;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceStaffingWorkerCount(NRC_INSTANCE ins)                     */
/*                                                                           */
/*  Return the number of workers in ins's staffing worker set.               */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceStaffingWorkerCount(NRC_INSTANCE ins)
{
  return NrcWorkerSetWorkerCount(ins->staffing_ws);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER NrcInstanceStaffingWorker(NRC_INSTANCE ins, int i)            */
/*                                                                           */
/*  Return the i'th worker of ins's staffing worker set.                     */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER NrcInstanceStaffingWorker(NRC_INSTANCE ins, int i)
{
  return NrcWorkerSetWorker(ins->staffing_ws, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceStaffingRetrieveWorker(NRC_INSTANCE ins, char *name,     */
/*    NRC_WORKER *w)                                                         */
/*                                                                           */
/*  Retrieve the worker with the given name from ins's staffing.             */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceStaffingRetrieveWorker(NRC_INSTANCE ins, char *name,
  NRC_WORKER *w)
{
  return NrcWorkerSetRetrieveWorker(ins->staffing_ws, name, w);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcOptionalPenaltyEqual(NRC_PENALTY p1, NRC_PENALTY p2)             */
/*                                                                           */
/*  Return true if these two optional penalties are equal.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcOptionalPenaltyEqual(NRC_PENALTY p1, NRC_PENALTY p2)
{
  return p1 == NULL ? (p2 == NULL) : (p2 != NULL && NrcPenaltyEqual(p1, p2));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceSetUniqueSkillPenalty(NRC_INSTANCE ins)                  */
/*                                                                           */
/*  Set ins->unique_skill_penalty to NRC_UNIQUE_SKILL_PENALTY_FALSE or       */
/*  NRC_UNIQUE_SKILL_PENALTY_TRUE as appropriate.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceSetUniqueSkillPenalty(NRC_INSTANCE ins)
{
  NRC_PENALTY unique_p, p;  int i;
  if( NrcWorkerSetWorkerCount(ins->staffing_ws) > 0 )
  {
    unique_p = NrcWorkerSkillPenalty(NrcInstanceStaffingWorker(ins, 0));
    for( i = 1;  i < NrcWorkerSetWorkerCount(ins->staffing_ws);  i++ )
    {
      p = NrcWorkerSkillPenalty(NrcInstanceStaffingWorker(ins, i));
      if( !NrcOptionalPenaltyEqual(unique_p, p) )
      {
        ins->unique_skill_penalty = NRC_UNIQUE_SKILL_PENALTY_FALSE;
	return;
      }
    }
  }
  ins->unique_skill_penalty = NRC_UNIQUE_SKILL_PENALTY_TRUE;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceUniqueSkillPenalty(NRC_INSTANCE ins, NRC_PENALTY *p)     */
/*                                                                           */
/*  If all the workers have the same skill penalty (possibly NULL), set *p   */
/*  to that penalty and return true.  Otherwise set *p to NULL and return    */
/*  false.                                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool NrcInstanceUniqueSkillPenalty(NRC_INSTANCE ins, NRC_PENALTY *p)
{
  if( ins->unique_skill_penalty == NRC_UNIQUE_SKILL_PENALTY_UNKNOWN )
    NrcInstanceSetUniqueSkillPenalty(ins);
  if( ins->unique_skill_penalty == NRC_UNIQUE_SKILL_PENALTY_FALSE )
  {
    *p = NULL;
    return false;
  }
  else
  {
    if( NrcWorkerSetWorkerCount(ins->staffing_ws) == 0 )
      *p = NULL;
    else
      *p = NrcWorkerSkillPenalty(NrcInstanceStaffingWorker(ins, 0));
    return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "empty worker-set"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcInstanceEmptyWorkerSet(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Return the empty worker-set.  Make sure that it really is empty.         */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcInstanceEmptyWorkerSet(NRC_INSTANCE ins)
{
  HnAssert(NrcWorkerSetWorkerCount(ins->empty_ws) == 0,
    "NrcInstanceEmptyWorkerSet: the empty worker-set is not empty");
  return ins->empty_ws;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "worker-sets"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddWorkerSet(NRC_INSTANCE ins, NRC_WORKER_SET ws)        */
/*                                                                           */
/*  Add ws to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddWorkerSet(NRC_INSTANCE ins, NRC_WORKER_SET ws)
{
  int pos;
  if( HnTableContains(ins->worker_set_table, NrcWorkerSetName(ws), pos) )
    HnAbort("NrcWorkerSetMake: a worker-set with name %s already exists",
      NrcWorkerSetName(ws));
  HnTableAdd(ins->worker_set_table, NrcWorkerSetName(ws), ws);
  HaArrayAddLast(ins->worker_sets, ws);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceWorkerSetCount(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of worker-sets in ins.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceWorkerSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->worker_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcInstanceWorkerSet(NRC_INSTANCE ins, int i)             */
/*                                                                           */
/*  Return the i'th worker-set of ins.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcInstanceWorkerSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->worker_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrieveWorkerSet(NRC_INSTANCE ins, char *name,          */
/*    NRC_WORKER_SET *ws)                                                    */
/*                                                                           */
/*  Retrieve the worker-set with the given name from ins.                    */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrieveWorkerSet(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *ws)
{
  int pos;
  return HnTableRetrieve(ins->worker_set_table, name, *ws, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "worker-set sets"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddWorkerSetSet(NRC_INSTANCE ins, NRC_WORKER_SET_SET wss)*/
/*                                                                           */
/*  Add wss to ins.                                                          */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddWorkerSetSet(NRC_INSTANCE ins, NRC_WORKER_SET_SET wss)
{
  HaArrayAddLast(ins->worker_set_sets, wss);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceWorkerSetSetCount(NRC_INSTANCE ins)                       */
/*                                                                           */
/*  Return the number of worker-set sets in ins.                             */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceWorkerSetSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->worker_set_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_SET NrcInstanceWorkerSetSet(NRC_INSTANCE ins, int i)      */
/*                                                                           */
/*  Return the i'th worker-set set of ins.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_SET NrcInstanceWorkerSetSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->worker_set_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "contracts"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceContractsAddContract(NRC_INSTANCE ins,                   */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add a contract worker-set to ins's contracts.                            */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceContractsAddContract(NRC_INSTANCE ins,
  NRC_WORKER_SET contract_ws)
{
  NrcWorkerSetSetAddWorkerSet(ins->contracts_wss, contract_ws);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_SET NrcInstanceContracts(NRC_INSTANCE ins)                */
/*                                                                           */
/*  Return ins's contracts.                                                  */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_SET NrcInstanceContracts(NRC_INSTANCE ins)
{
  return ins->contracts_wss;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceContractsContractCount(NRC_INSTANCE ins)                  */
/*                                                                           */
/*  Return the number of contracts in ins's contracts.                       */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceContractsContractCount(NRC_INSTANCE ins)
{
  return NrcWorkerSetSetWorkerSetCount(ins->contracts_wss);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcInstanceContractsContract(NRC_INSTANCE ins, int i)     */
/*                                                                           */
/*  Return the i'th contract of ins's contracts.                             */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcInstanceContractsContract(NRC_INSTANCE ins, int i)
{
  return NrcWorkerSetSetWorkerSet(ins->contracts_wss, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceContractsRetrieveContract(NRC_INSTANCE ins, char *name,  */
/*    NRC_WORKER_SET *contract_ws)                                           */
/*                                                                           */
/*  Retrieve the contract worker-set with this name.                         */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceContractsRetrieveContract(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *contract_ws)
{
  return NrcWorkerSetSetRetrieveWorkerSet(ins->contracts_wss,name,contract_ws);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "skills"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceSkillsAddSkill(NRC_INSTANCE ins, NRC_WORKER_SET skill_ws)*/
/*                                                                           */
/*  Add a skill worker-set to ins's skills.                                  */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceSkillsAddSkill(NRC_INSTANCE ins, NRC_WORKER_SET skill_ws)
{
  NrcWorkerSetSetAddWorkerSet(ins->skills_wss, skill_ws);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_SET NrcInstanceSkills(NRC_INSTANCE ins)                   */
/*                                                                           */
/*  Return the skill worker-set set.                                         */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_SET NrcInstanceSkills(NRC_INSTANCE ins)
{
  return ins->skills_wss;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceSkillsSkillCount(NRC_INSTANCE ins)                        */
/*                                                                           */
/*  Return the number of skills.                                             */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceSkillsSkillCount(NRC_INSTANCE ins)
{
  return NrcWorkerSetSetWorkerSetCount(ins->skills_wss);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcInstanceSkillsSkill(NRC_INSTANCE ins, int i)           */
/*                                                                           */
/*  Return the i'th skill worker-set.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcInstanceSkillsSkill(NRC_INSTANCE ins, int i)
{
  return NrcWorkerSetSetWorkerSet(ins->skills_wss, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceSkillsRetrieveSkill(NRC_INSTANCE ins, char *name,        */
/*    NRC_WORKER_SET *skill_ws)                                              */
/*                                                                           */
/*  Retrieve the skill worker-set with this name.                            */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceSkillsRetrieveSkill(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *skill_ws)
{
  return NrcWorkerSetSetRetrieveWorkerSet(ins->skills_wss, name, skill_ws);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demands" (construction)                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandBegin(NRC_INSTANCE ins)                            */
/*                                                                           */
/*  Begin construction of a new demand object.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandBegin(NRC_INSTANCE ins)
{
  HnAssert(ins->curr_demand == NULL,
    "NrcInstanceDemandBegin called out of order");
  ins->curr_demand = NrcDemandMake(ins);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeNonAssignment(NRC_INSTANCE ins,            */
/*    NRC_PENALTY_TYPE ptype, NRC_PENALTY p)                                 */
/*                                                                           */
/*  Add this penalizer to the demand object currently under construction.    */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeNonAssignment(NRC_INSTANCE ins,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeNonAssignment called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcInstanceDemandPenalizeNonAssignment: penalty cost function not linear");
  NrcDemandAddPenalizer(ins->curr_demand, NRC_PENALIZER_NON_ASSIGNMENT,
    NULL, ptype, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeWorkerSet(NRC_INSTANCE ins,                */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Add this penalizer to the demand object currently under construction.    */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeWorkerSet called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcInstanceDemandPenalizeWorkerSet: penalty cost function not linear");
  NrcDemandAddPenalizer(ins->curr_demand, NRC_PENALIZER_WORKER_SET,
    ws, ptype, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeNotWorkerSet(NRC_INSTANCE ins,             */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Add this penalizer to the demand object currently under construction.    */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeNotWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeNotWorkerSet called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcInstanceDemandPenalizeNotWorkerSet: penalty cost function not linear");
  NrcDemandAddPenalizer(ins->curr_demand, NRC_PENALIZER_NOT_WORKER_SET,
    ws, ptype, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPrefer(NRC_INSTANCE ins,                           */
/*    NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)          */
/*                                                                           */
/*  Add this preference to the demand object currently under construction.   */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPrefer(NRC_INSTANCE ins,
  NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPrefer called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcInstanceDemandPrefer: penalty cost function is not linear");
  NrcDemandAddPreference(ins->curr_demand, prefer_ws, prefer_nonasst, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeWorkerSet(NRC_INSTANCE ins,                */
/*    NRC_WORKER_SET ws, NRC_PENALTY p)                                      */
/*                                                                           */
/*  Tell the demand currently under construction to penalize assigning       */
/*  a worker from ws.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeWorkerSet called out of order");
  if( NrcWorkerSetEqual(ws, NrcInstanceStaffing(ins)) )
    NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_ASSIGNMENT, NULL, p);
  else
    NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_WORKER_SET, ws, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeNotWorkerSet(NRC_INSTANCE ins,             */
/*    NRC_WORKER_SET ws, NRC_PENALTY p)                                      */
/*                                                                           */
/*  Tell the demand currently under construction to penalize assigning       */
/*  a worker not from ws.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeNotWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeNotWorkerSet called out of order");
  if( NrcWorkerSetEqual(ws, NrcInstanceStaffing(ins)) )
    NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_NON_ASSIGNMENT,NULL,p);
  else
    NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_NOT_WORKER_SET, ws, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeAssignment(NRC_INSTANCE ins, NRC_PENALTY p)*/
/*                                                                           */
/*  Tell the demand currently under construction to penalize assignment.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeAssignment(NRC_INSTANCE ins, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeAssignment called out of order");
  NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_ASSIGNMENT, NULL, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDemandPenalizeNonAssignment(NRC_INSTANCE ins,            */
/*    NRC_PENALTY p)                                                         */
/*                                                                           */
/*  Tell the demand currently under construction to penalize non-assignment. */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcInstanceDemandPenalizeNonAssignment(NRC_INSTANCE ins, NRC_PENALTY p)
{
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandPenalizeNonAssignment called out of order");
  NrcDemandAddPenalize(ins->curr_demand, NRC_PENALIZER_NON_ASSIGNMENT, NULL, p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcInstanceDemandEnd(NRC_INSTANCE ins)                        */
/*                                                                           */
/*  Finish off the current demand.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_DEMAND NrcInstanceDemandEnd(NRC_INSTANCE ins)
{
  NRC_DEMAND res;  char *name;
  HnAssert(ins->curr_demand != NULL,
    "NrcInstanceDemandEnd called out of order");
  name = NrcDemandNa me(ins->curr_demand);
  if( !NrcInstanceRetrieveDemand(ins, name, &res) )
  {
    res = ins->curr_demand;
    NrcInstanceAddDemand(ins, ins->curr_demand);
  }
  ins->curr_demand = NULL;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demands"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrieveDemand(NRC_INSTANCE ins, char *name,             */
/*    NRC_DEMAND *d)                                                         */
/*                                                                           */
/*  Retrieve a demand with the given name from ins.                          */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrieveDemand(NRC_INSTANCE ins, char *name, NRC_DEMAND *d)
{
  int pos;
  return HnTableRetrieve(ins->demand_table, name, *d, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddDemand(NRC_INSTANCE ins, char *name, NRC_DEMAND d)    */
/*                                                                           */
/*  Add d to ins under the given name.                                       */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddDemand(NRC_INSTANCE ins, char *name, NRC_DEMAND d)
{
  HaArrayAddLast(ins->demands, d);
  HnTableAdd(ins->demand_table, name, d);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDemandCount(NRC_INSTANCE ins)                             */
/*                                                                           */
/*  Return the number of demands of ins.                                     */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDemandCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->demands);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcInstanceDemand(NRC_INSTANCE ins, int i)                    */
/*                                                                           */
/*  Return the i'th demand of ins.                                           */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND NrcInstanceDemand(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->demands, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand-sets"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddDemandSet(NRC_INSTANCE ins, NRC_DEMAND_SET ds)        */
/*                                                                           */
/*  Add ds to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddDemandSet(NRC_INSTANCE ins, NRC_DEMAND_SET cs)
{
  HaArrayAddLast(ins->demand_sets, cs);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDemandSetCount(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of demand-sets of ins.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDemandSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->demand_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_SET NrcInstanceDemandSet(NRC_INSTANCE ins, int i)             */
/*                                                                           */
/*  Return the i'th demand-set of ins.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND_SET NrcInstanceDemandSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->demand_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "patterns"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceAddPattern(NRC_INSTANCE ins, NRC_PATTERN p)               */
/*                                                                           */
/*  Add p to ins, returning p's index in ins.                                */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceAddPattern(NRC_INSTANCE ins, NRC_PATTERN p)
{
  int res;
  res = HaArrayCount(ins->patterns);
  HaArrayAddLast(ins->patterns, p);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstancePatternCount(NRC_INSTANCE ins)                            */
/*                                                                           */
/*  Return the number of patterns of ins.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcInstancePatternCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->patterns);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN NrcInstancePattern(NRC_INSTANCE ins, int i)                  */
/*                                                                           */
/*  Return the i'th pattern of ins.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN NrcInstancePattern(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->patterns, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceRetrievePattern(NRC_INSTANCE ins, char *name,            */
/*    NRC_PATTERN *p)                                                        */
/*                                                                           */
/*  Retrieve a pattern with the given name from ins.                         */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceRetrievePattern(NRC_INSTANCE ins, char *name, NRC_PATTERN *p)
{
  NRC_PATTERN p2;  int i;  char *name2;
  HaArrayForEach(ins->patterns, p2, i)
  {
    name2 = NrcPatternName(p2);
    if( name2 != NULL && strcmp(name2, name) == 0 )
    {
      *p = p2;
      return true;
    }
  }
  *p = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "pattern sets"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceAddPatternSet(NRC_INSTANCE ins, NRC_PATTERN_SET ps)       */
/*                                                                           */
/*  Add ps to ins, returning ps's index in ins.                              */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceAddPatternSet(NRC_INSTANCE ins, NRC_PATTERN_SET ps)
{
  int res;
  res = HaArrayCount(ins->pattern_sets);
  HaArrayAddLast(ins->pattern_sets, ps);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstancePatternSetCount(NRC_INSTANCE ins)                         */
/*                                                                           */
/*  Return the number of pattern sets of ins.                                */
/*                                                                           */
/*****************************************************************************/

int NrcInstancePatternSetCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->pattern_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN_SET NrcInstancePatternSet(NRC_INSTANCE ins, int i)           */
/*                                                                           */
/*  Return the i'th pattern set of ins.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN_SET NrcInstancePatternSet(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->pattern_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand constraints"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddDemandConstraint(NRC_INSTANCE ins,                    */
/*    NRC_DEMAND_CONSTRAINT c)                                               */
/*                                                                           */
/*  Add c to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddDemandConstraint(NRC_INSTANCE ins,
  NRC_DEMAND_CONSTRAINT c)
{
  HaArrayAddLast(ins->demand_constraints, c);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceDemandConstraintCount(NRC_INSTANCE ins)                   */
/*                                                                           */
/*  Return the number of demand constraints in ins.                          */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceDemandConstraintCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->demand_constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_CONSTRAINT NrcInstanceDemandConstraint(NRC_INSTANCE ins,int i)*/
/*                                                                           */
/*  Return the i'th demand constraint of ins.                                */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND_CONSTRAINT NrcInstanceDemandConstraint(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->demand_constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "worker constraints"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceAddConstraint(NRC_INSTANCE ins, NRC_CONSTRAINT c)        */
/*                                                                           */
/*  Add c to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcInstanceAddConstraint(NRC_INSTANCE ins, NRC_CONSTRAINT c)
{
  HaArrayAddLast(ins->constraints, c);
  if( NrcConstraintType(c) == NRC_CONSTRAINT_WORKLOAD )
    ins->has_workload_constraint = true;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceConstraintCount(NRC_INSTANCE ins)                         */
/*                                                                           */
/*  Return the number of constraints in ins.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceConstraintCount(NRC_INSTANCE ins)
{
  return HaArrayCount(ins->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT NrcInstanceConstraint(NRC_INSTANCE ins, int i)            */
/*                                                                           */
/*  Return the i'th constraint of ins.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_CONSTRAINT NrcInstanceConstraint(NRC_INSTANCE ins, int i)
{
  return HaArray(ins->constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcInstanceHasWorkloadConstraint(NRC_INSTANCE ins)                  */
/*                                                                           */
/*  Return true if ins has a workload constraint.                            */
/*                                                                           */
/*****************************************************************************/

bool NrcInstanceHasWorkloadConstraint(NRC_INSTANCE ins)
{
  return ins->has_workload_constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "total workload"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcInstanceTotalWorkload(NRC_INSTANCE ins)                           */
/*                                                                           */
/*  Return the total workload of ins.                                        */
/*                                                                           */
/*****************************************************************************/

int NrcInstanceTotalWorkload(NRC_INSTANCE ins)
{
  return ins->total_workload;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE NrcInstanceKheInstance(NRC_INSTANCE ins)                    */
/*                                                                           */
/*  Return the KHE instance corresponding to ins.  Abort if none.            */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE NrcInstanceKheInstance(NRC_INSTANCE ins)
{
  HnAssert(ins->khe_instance != NULL, "NrcInstanceKheInstance: not done yet");
  return ins->khe_instance;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP NrcInstanceEmptyResourceGroup(NRC_INSTANCE ins)       */
/*                                                                           */
/*  Return an empty KHE resource group.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP NrcInstanceEmptyResourceGroup(NRC_INSTANCE ins)
{
  return ins->empty_rg;
}

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP NrcInstanceAllEventGroup(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Return a KHE event group holding every event of the converted instance.  */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP NrcInstanceAllEventGroup(NRC_INSTANCE ins)
{
  return ins->all_eg;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertShiftsToTimes(NRC_INSTANCE ins)                   */
/*                                                                           */
/*  Convert the shifts of ins into times.                                    */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertShiftsToTimes(NRC_INSTANCE ins)
{
  int i, j;  NRC_SHIFT_SET ss;  NRC_SHIFT s;
  for( i = 0;  i < NrcInstanceCycleDayCount(ins);  i++ )
  {
    ss = NrcDayShiftSet(NrcInstanceCycleDay(ins, i));
    for( j = 0;  j < NrcShiftSetShiftCount(ss);  j++ )
    {
      s = NrcShiftSetShift(ss, j);
      NrcShiftConvertToTime(s, ins->khe_instance);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertDaysToTimeGroups(NRC_INSTANCE ins)                */
/*                                                                           */
/*  Convert days to Day time groups.  We do this whether they are used       */
/*  or not, because timetable printing uses Day time groups.                 */
/*                                                                           */
/*  Implementation note.  Merely asking for the time group is enough.        */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertDaysToTimeGroups(NRC_INSTANCE ins)
{
  int i;  NRC_SHIFT_SET ss;
  if( DEBUG2 )
    fprintf(stderr, "[ NrcInstanceConvertDaysToTimeGroups(%s)\n",
      NrcInstanceId(ins));
  for( i = 0;  i < NrcInstanceCycleDayCount(ins);  i++ )
  {
    ss = NrcDayShiftSet(NrcInstanceCycleDay(ins, i));
    NrcShiftSetTimeGroup(ss, ins->khe_instance);
  }
  if( DEBUG2 )
    fprintf(stderr, "] NrcInstanceConvertDaysToTimeGroups\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertWeeksToTimeGroups(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Convert weeks to Week time groups.  We do this whether they are used     */
/*  or not, because timetable printing uses Week time groups.                */
/*                                                                           */
/*  Implementation note.  Merely asking for the time group is enough.        */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertWeeksToTimeGroups(NRC_INSTANCE ins)
{
  int i, count;  NRC_SHIFT_SET ss;
  count = (ins->weeks_shift_set_set == NULL ? -1 :
    NrcShiftSetSetShiftSetCount(ins->weeks_shift_set_set));
  if( DEBUG2 )
    fprintf(stderr, "[ NrcInstanceConvertWeeksToTimeGroups(%s, %d weeks)\n",
      NrcInstanceId(ins), count);
  if( ins->weeks_shift_set_set != NULL )
  {
    for( i = 0;  i < count;  i++ )
    {
      ss = NrcShiftSetSetShiftSet(ins->weeks_shift_set_set, i);
      NrcShiftSetTimeGroup(ss, ins->khe_instance);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] NrcInstanceConvertWeeksToTimeGroups\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertWorkersToResources(NRC_INSTANCE ins)              */
/*                                                                           */
/*  Convert the workers of ins into resources of res, including adding       */
/*  a "Workers" resource type.                                               */
/*                                                                           */
/*  NB empty_rg is needed by prefer resources constraints that implement     */
/*  penalities for assigning, as opposed to not assigning, nurses.           */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertWorkersToResources(NRC_INSTANCE ins)
{
  int i;  KHE_RESOURCE_TYPE rt;
  if( !KheResourceTypeMake(ins->khe_instance, ins->worker_word,
	ins->worker_word, false, &rt) )
    HnAbort("NrcInstanceConvertWorkersToResources internal error 1");
  for( i = 0;  i < NrcInstanceStaffingWorkerCount(ins);  i++ )
    NrcWorkerConvertToResource(NrcInstanceStaffingWorker(ins, i), rt);
  if( !KheResourceGroupMake(rt, "RG:Empty", "RG:Empty", false, &ins->empty_rg) )
    HnAbort("NrcInstanceConvertWorkersToResources internal error 2");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertShiftsToEvents(NRC_INSTANCE ins)                  */
/*                                                                           */
/*  Convert the shifts of ins into events in res.                            */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertShiftsToEvents(NRC_INSTANCE ins)
{
  int i, j;  NRC_SHIFT_SET ss;  NRC_SHIFT s;
  if( !KheEventGroupMake(ins->khe_instance, KHE_EVENT_GROUP_KIND_ORDINARY,
	"EG:All", "EG:All", &ins->all_eg) )
    HnAbort("NrcInstanceConvertShiftsToEvents internal error 1");
  for( i = 0;  i < NrcInstanceCycleDayCount(ins);  i++ )
  {
    ss = NrcDayShiftSet(NrcInstanceCycleDay(ins, i));
    for( j = 0;  j < NrcShiftSetShiftCount(ss);  j++ )
    {
      s = NrcShiftSetShift(ss, j);
      NrcShiftConvertToEvent(s, ins->khe_instance);
      KheEventGroupAddEvent(ins->all_eg, NrcShiftEvent(s));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertDemandsToEventResourceConstraints(                */
/*    NRC_INSTANCE ins)                                                      */
/*                                                                           */
/*  Convert demands to event resource constraints.                           */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertDemandsToEventResourceConstraints(
  NRC_INSTANCE ins)
{
  int i;  NRC_DEMAND d;
  HaArrayForEach(ins->demands, d, i)
    NrcDemandConvertToEventResourceConstraints(d, ins->khe_instance);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertStaffingToAvoidClashesConstraint(NRC_INSTANCE ins)*/
/*                                                                           */
/*  Add an avoid clashes constraint for the whole staffing.                  */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertStaffingToAvoidClashesConstraint(NRC_INSTANCE ins)
{
  KHE_AVOID_CLASHES_CONSTRAINT c;  KHE_RESOURCE_GROUP rg;  NRC_PENALTY p;
  p = ins->avoid_clashes_penalty;
  if( NrcPenaltyWeight(p) > 0 )
  {
    if( !KheAvoidClashesConstraintMake(ins->khe_instance,
	  "AvoidClashesConstraint", "Avoid clashes", NrcPenaltyHard(p),
	  NrcPenaltyWeight(p), NrcPenaltyKheCostFn(p), &c) )
      HnAbort("cannot generate AvoidClashesConstraint");
    rg = NrcWorkerSetResourceGroup(NrcInstanceStaffing(ins), ins->khe_instance);
    KheAvoidClashesConstraintAddResourceGroup(c, rg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertDemandConstraints(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Convert demand constraints into KHE constraints.                         */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertDemandConstraints(NRC_INSTANCE ins)
{
  int i, j, name_index;  NRC_DEMAND_CONSTRAINT ci, cj;

  /* sort the constraints to bring mergeable ones together */
  HaArraySort(ins->demand_constraints,
    &NrcDemandConstraintCmpBoundAndWorkerSets);

  /* pass intervals of mergeable constraints to be converted */
  name_index = 1;
  for( i = 0;  i < HaArrayCount(ins->demand_constraints);  i = j )
  {
    ci = HaArray(ins->demand_constraints, i);
    for( j = i + 1;  j <  HaArrayCount(ins->demand_constraints);  j++ )
    {
      cj = HaArray(ins->demand_constraints, j);
      if( NrcDemandConstraintTypedCmpBoundAndWorkerSets(ci, cj) != 0 )
	break;
    }
    NrcDemandConstraintsConvert(ci, name_index++, i, j - 1, ins->khe_instance);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceDeleteConstraintsWithNoWorkers(NRC_INSTANCE ins)         */
/*                                                                           */
/*  Delete worker constraints with no workers.                               */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceDeleteConstraintsWithNoWorkers(NRC_INSTANCE ins)
{
  int i;  NRC_CONSTRAINT c;  NRC_WORKER_SET ws;
  for( i = 0;  i < HaArrayCount(ins->constraints);  i++ )
  {
    c = HaArray(ins->constraints, i);
    ws = NrcConstraintWorkerSet(c);
    if( NrcWorkerSetWorkerCount(ws) == 0 )
    {
      HaArrayDeleteAndPlug(ins->constraints, i);
      i--;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertWorkerConstraints(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Convert worker constraints into KHE constraints.                         */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertWorkerConstraints(NRC_INSTANCE ins)
{
  int i, j, name_index;  NRC_CONSTRAINT ci, cj;  NRC_CONSTRAINT_CONDENSER cc;
  NRC_CONDENSED_CONSTRAINT c;
  if( DEBUG3 )
    fprintf(stderr, "[ NrcInstanceConvertWorkerConstraints(%s; %d)\n",
      NrcInstanceId(ins), HaArrayCount(ins->constraints));

  /* condense constraints, where possible */
  /* this may kill some constraints and add new ones to ins->constraints */
  cc = NrcConstraintCondenserBegin(ins);
  HaArrayForEach(ins->constraints, ci, i)
  {
    c = NrcConstraintCondensedConstraint(ci);
    if( c != NULL )
      NrcConstraintCondenserAddCondensedConstraint(cc, c);
  }
  if( DEBUG7 )
    NrcConstraintCondenserDebug(cc, 0, stderr);
  NrcConstraintCondenserMerge(cc);  /* this may add to ins->constraints */
  NrcConstraintCondenserEnd(cc);

  /* delete constraints with empty worker sets */
  NrcInstanceDeleteConstraintsWithNoWorkers(ins);
  if( DEBUG3 )
    fprintf(stderr, "  NrcInstanceConvertWorkerConstraints after drop1 (%d)\n",
      HaArrayCount(ins->constraints));

  /* sort the constraints to bring together those which are equal, */
  /* including worker sets, ignoring their names and bounds, and   */
  /* merge intervals of equal constraints, where possible */
  HaArraySort(ins->constraints, &NrcConstraintCmpIgnoreNamesAndBounds);
  if( DEBUG3 )
    fprintf(stderr, "  NrcInstanceConvertWorkerConstraints after sort1 (%d)\n",
      HaArrayCount(ins->constraints));
  for( i = 0;  i < HaArrayCount(ins->constraints);  i = j )
  {
    ci = HaArray(ins->constraints, i);
    for( j = i + 1;  j < HaArrayCount(ins->constraints);  j++ )
    {
      cj = HaArray(ins->constraints, j);
      if( NrcConstraintTypedCmpIgnoreNamesAndBounds(ci, cj) != 0 )
	break;
    }
    if( DEBUG3 )
      fprintf(stderr, "  calling NrcConstraintMergeBounds(ci, %d, %d)\n",
	i, j-1);
    NrcConstraintMergeBounds(ci, i, j - 1);
  }
  if( DEBUG3 )
    fprintf(stderr, "  NrcInstanceConvertWorkerConstraints after merge\n");

  /* delete constraints with empty worker sets (again) */
  NrcInstanceDeleteConstraintsWithNoWorkers(ins);
  if( DEBUG3 )
    fprintf(stderr, "  NrcInstanceConvertWorkerConstraints after drop2 (%d)\n",
      HaArrayCount(ins->constraints));

  /* sort the constraints to bring together those which are equal, */
  /* including bounds, excluding only their names and worker sets,  */
  /* and convert intervals of equal constraints */
  HaArraySort(ins->constraints, &NrcConstraintCmpIgnoreNamesAndWorkerSets);
  name_index = 1;
  for( i = 0;  i < HaArrayCount(ins->constraints);  i = j )
  {
    ci = HaArray(ins->constraints, i);
    for( j = i + 1;  j <  HaArrayCount(ins->constraints);  j++ )
    {
      cj = HaArray(ins->constraints, j);
      if( NrcConstraintTypedCmpIgnoreNamesAndWorkerSets(ci, cj) != 0 )
	break;
    }
    NrcConstraintConvert(ci, name_index++, i, j - 1, ins->khe_instance);
  }
  if( DEBUG3 )
    fprintf(stderr, "] NrcInstanceConvertWorkerConstraints returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertContractsAndSkillsToResourceGroups(               */
/*    NRC_INSTANCE ins, KHE_INSTANCE res)                                    */
/*                                                                           */
/*  Ensure that there is a KHE resource group for each contract worker-set   */
/*  and each skill worker-set.                                               */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertContractsAndSkillsToResourceGroups(
  NRC_INSTANCE ins)
{
  int i;  NRC_WORKER_SET ws;
  for( i = 0;  i < NrcWorkerSetSetWorkerSetCount(ins->contracts_wss);  i++ )
  {
    ws = NrcWorkerSetSetWorkerSet(ins->contracts_wss, i);
    NrcWorkerSetResourceGroup(ws, ins->khe_instance);
  }
  for( i = 0;  i < NrcWorkerSetSetWorkerSetCount(ins->skills_wss);  i++ )
  {
    ws = NrcWorkerSetSetWorkerSet(ins->skills_wss, i);
    NrcWorkerSetResourceGroup(ws, ins->khe_instance);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertDayAndShiftOffRequests(NRC_INSTANCE ins)          */
/*                                                                           */
/*  Convert shift off requests into avoid unavailable times constraints.     */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertDayAndShiftOffRequests(NRC_INSTANCE ins)
{
  int i;  NRC_WORKER w;
  for( i = 0;  i < NrcWorkerSetWorkerCount(ins->staffing_ws);  i++ )
  {
    w = NrcWorkerSetWorker(ins->staffing_ws, i);
    NrcWorkerConvertDayAndShiftOffRequests(w, ins->khe_instance);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertDayAndShiftOnRequests(NRC_INSTANCE ins)           */
/*                                                                           */
/*  Convert shift on requests into limit busy times constraints.             */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertDayAndShiftOnRequests(NRC_INSTANCE ins)
{
  int i;  NRC_WORKER w;
  for( i = 0;  i < NrcWorkerSetWorkerCount(ins->staffing_ws);  i++ )
  {
    w = NrcWorkerSetWorker(ins->staffing_ws, i);
    NrcWorkerConvertDayAndShiftOnRequests(w, ins->khe_instance);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcInstanceConvertStartAndEndDayRequests(NRC_INSTANCE ins)          */
/*                                                                           */
/*  Convert shift on requests into limit busy times constraints.             */
/*                                                                           */
/*****************************************************************************/

static void NrcInstanceConvertStartAndEndDayRequests(NRC_INSTANCE ins)
{
  int i;  NRC_WORKER w;
  for( i = 0;  i < NrcWorkerSetWorkerCount(ins->staffing_ws);  i++ )
  {
    w = NrcWorkerSetWorker(ins->staffing_ws, i);
    NrcWorkerConvertStartAndEndDayRequests(w, ins->khe_instance);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE NrcInstanceConvert(NRC_INSTANCE ins)                        */
/*                                                                           */
/*  Convernt NRC instance ins into a KHE instance.                           */
/*                                                                           */
/*  Implementation note.  This function also sorts the shifts of each        */
/*  shift-set into canonical order (needed when grouping constraints).       */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE NrcInstanceConvert(NRC_INSTANCE ins)
{
  NRC_SHIFT_SET ss;  int i;  char *str;

  /* sort every shift-set's shifts into canonical order */
  if( DEBUG1 )
    fprintf(stderr, "[ NrcInstanceConvert(%s)\n", NrcInstanceId(ins));
  HnAssert(NrcInstanceComplete(ins),
    "NrcInstanceConvert called on instance lacking NrcInstanceMakeEnd");
  HaArrayForEach(ins->shift_sets, ss, i)
    NrcShiftSetSortShifts(ss);

  /* make the KHE instance */
  /* ***
  kmd = KheInstanceMetaDataMake(
    NrcInstanceMetaDataName(ins->meta_data),
    NrcInstanceMetaDataContributor(ins->meta_data),
    NrcInstanceMetaDataDate(ins->meta_data),
    NrcInstanceMetaDataCountry(ins->meta_data),
    NrcInstanceMetaDataDescription(ins->meta_data),
    NrcInstanceMetaDataRemarks(ins->meta_data));
  *** */
  ins->khe_instance = KheInstanceMakeBegin(ins->id,
    KHE_MODEL_EMPLOYEE_SCHEDULE, NULL);
  KheInstanceSetMetaData(ins->khe_instance, ins->meta_name,
    ins->meta_contributor, ins->meta_date, ins->meta_country,
    ins->meta_description, ins->meta_remarks);

  NrcInstanceConvertShiftsToTimes(ins);
  NrcInstanceConvertDaysToTimeGroups(ins);
  NrcInstanceConvertWeeksToTimeGroups(ins);
  NrcInstanceConvertWorkersToResources(ins);
  NrcInstanceConvertContractsAndSkillsToResourceGroups(ins);
  NrcInstanceConvertShiftsToEvents(ins);
  NrcInstanceConvertDemandsToEventResourceConstraints(ins);
  NrcInstanceConvertStaffingToAvoidClashesConstraint(ins);
  NrcInstanceConvertDemandConstraints(ins);
  NrcInstanceConvertWorkerConstraints(ins);
  NrcInstanceConvertDayAndShiftOffRequests(ins);
  NrcInstanceConvertDayAndShiftOnRequests(ins);
  NrcInstanceConvertStartAndEndDayRequests(ins);
  if( DEBUG1 )
    fprintf(stderr, "  NrcInstanceConvert calling KheInstanceMakeEnd:\n");
  if( !KheInstanceMakeEnd(ins->khe_instance, false, false, false, &str) )
    HnAbort("NrcInstanceConvert: %s", str);
  if( DEBUG1 )
    fprintf(stderr, "] NrcInstanceConvert returning\n");
  return ins->khe_instance;
}

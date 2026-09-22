
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
/*  FILE:         inrc1.c                                                    */
/*  MODULE:       ConvertINRC1 (first international nurse rostering comp)    */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG9 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  INRC1_INFO - miscellaneous information about a INRC1 instance            */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER_SET) ARRAY_NRC_WORKER_SET;
typedef HA_ARRAY(NRC_DAY_SET)	 ARRAY_NRC_DAY_SET;
typedef HA_ARRAY(NRC_PENALTY)	 ARRAY_NRC_PENALTY;

typedef struct inrc1_info_rec {
  HA_ARENA			arena;
  char				*file_name;
  KML_ELT			root_elt;
  NRC_INSTANCE			ins;
  HA_ARRAY_BOOL			shift_type_is_night;
  ARRAY_NRC_WORKER_SET		shift_type_skill_ws;
  HA_ARRAY_INT			pattern_weight;
  ARRAY_NRC_DAY_SET		pattern_starting_day_set;
  ARRAY_NRC_PENALTY		alternative_skill_penalties;
} *INRC1_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddCycle(INRC1_INFO ii)                                             */
/*                                                                           */
/*  Add the cycle and the days_of_week.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddCycle(INRC1_INFO ii)
{
  KML_ELT start_day_elt, end_day_elt;  char *err_str;
  start_day_elt = KmlChild(ii->root_elt, 0);
  end_day_elt = KmlChild(ii->root_elt, 1);
  if( !NrcCalendarCycleMake(ii->ins, KmlText(start_day_elt),
	KmlText(end_day_elt), &err_str) )
    KmlEltFatalError(ii->root_elt, ii->file_name, "%s", err_str);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "skills"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSkills(INRC1_INFO ii)                                            */
/*                                                                           */
/*  Add skills to ii.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddSkills(INRC1_INFO ii)
{
  KML_ELT skills_elt, skill_elt;  int i;  KML_ERROR ke;
  NRC_WORKER_SET skill_ws;  char *name;
  if( DEBUG9 )
    fprintf(stderr, "[ AddSkills(ii)\n");
  if( KmlContainsChild(ii->root_elt, "Skills", &skills_elt) )
  {
    if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(skills_elt);  i++ )
    {
      skill_elt = KmlChild(skills_elt, i);
      name = KmlText(skill_elt);
      if( NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	KmlEltFatalError(skill_elt, "skill name %s appears twice", name);
      skill_ws = NrcWorkerSetMake(ii->ins, name);
      NrcInstanceSkillsAddSkill(ii->ins, skill_ws);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddSkills returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift types"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddShiftTypes(INRC1_INFO ii)                                        */
/*                                                                           */
/*  Add shift types to ii.  Also record information about whether each       */
/*  shift type is a night shift type or not, and one skill for each type.    */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypes(INRC1_INFO ii)
{
  KML_ELT shift_types_elt, shift_type_elt, skills_elt, skill_elt;
  KML_ELT start_time_elt, end_time_elt;  KML_ERROR ke;
  int i, start_hour, end_hour;  char *id, *val;  bool is_night;
  NRC_WORKER_SET skill_ws;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftTypes(ii)\n");
  if( KmlContainsChild(ii->root_elt, "ShiftTypes", &shift_types_elt) )
  {
    for( i = 0;  i < KmlChildCount(shift_types_elt);  i++ )
    {
      /* find one shift type and its id */
      shift_type_elt = KmlChild(shift_types_elt, i);
      if( !KmlCheck(shift_type_elt,
	    "ID : $StartTime $EndTime $Description Skills", &ke) )
	KmlFatalError(ke, ii->file_name);
      id = KmlAttributeValue(shift_type_elt, 0);
      if( DEBUG9 )
	fprintf(stderr, "  id %s:\n", id);

      /* find is_night (a shift is a night shift if it includes midnight) */
      start_time_elt = KmlChild(shift_type_elt, 0);
      end_time_elt = KmlChild(shift_type_elt, 1);
      sscanf(KmlText(start_time_elt), "%d:%*d:%*d", &start_hour);
      sscanf(KmlText(end_time_elt), "%d:%*d:%*d", &end_hour);
      is_night = (start_hour > end_hour);
      if( DEBUG9 )
	fprintf(stderr, "  Shift type %s: is_night = %s\n", id,
	  is_night ? "true" : "false");

      /* find the skill */
      skill_ws = NULL;
      if( KmlContainsChild(shift_type_elt, "Skills", &skills_elt) &&
	  KmlChildCount(skills_elt) == 1 )
      {
	skill_elt = KmlChild(skills_elt, 0);
	val = KmlText(skill_elt);
	if( !NrcInstanceSkillsRetrieveSkill(ii->ins, val, &skill_ws) )
	  KmlEltFatalError(skill_elt, ii->file_name, "unknown skill %s", val);
      }

      /* make the shift type and remember is_night and skill_ws */
      NrcShiftTypeMake(ii->ins, id, NRC_NO_WORKLOAD);
      HaArrayAddLast(ii->shift_type_is_night, is_night);
      HaArrayAddLast(ii->shift_type_skill_ws, skill_ws);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftTypes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "patterns"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PATTERN PatternMakeFromKmlElt(INRC1_INFO ii, KML_ELT pattern_elt)        */
/*                                                                           */
/*  Make a pattern from pattern_elt, assumed to have tag Pattern.            */
/*                                                                           */
/*****************************************************************************/

static NRC_PATTERN PatternMakeFromKmlElt(INRC1_INFO ii, KML_ELT pattern_elt)
{
  KML_ERROR ke;  char *id, *text;  int i, weight;  NRC_PATTERN res;
  KML_ELT pattern_entries_elt, pattern_entry_elt, day_elt, shift_type_elt;
  NRC_SHIFT_TYPE st;  NRC_SHIFT_TYPE_SET sts;  NRC_DAY_SET ds;
  NRC_POLARITY po;

  /* check format of pattern_elt, and get Id */
  if( !KmlCheck(pattern_elt, "ID #weight : PatternEntries", &ke) )
    KmlFatalError(ke, ii->file_name);
  id = KmlAttributeValue(pattern_elt, 0);
  sscanf(KmlAttributeValue(pattern_elt, 1), "%d", &weight);

  /* get terms and build res */
  res = NULL;
  pattern_entries_elt = KmlChild(pattern_elt, 0);
  if( !KmlCheck(pattern_entries_elt, ": *PatternEntry", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(pattern_entries_elt);  i++ )
  {
    pattern_entry_elt = KmlChild(pattern_entries_elt, i);
    if( !KmlCheck(pattern_entry_elt, "#index : ShiftType Day", &ke) )
      KmlFatalError(ke, ii->file_name);

    /* if first term, build res with extra info in pattern_weight etc. */
    if( res == NULL )
    {
      day_elt = KmlChild(pattern_entry_elt, 1);
      text = KmlText(day_elt);
      if( strcmp(text, "Any") == 0 )
        ds = NrcInstanceCycle(ii->ins);
      else if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, text, &ds) )
	KmlEltFatalError(day_elt, ii->file_name, "unknown <Day> %s", text);
      res = NrcPatternMake(ii->ins, id);
      HaArrayAddLast(ii->pattern_weight, weight);
      HaArrayAddLast(ii->pattern_starting_day_set, ds);
    }

    /* add one term to res */
    shift_type_elt = KmlChild(pattern_entry_elt, 0);
    text = KmlText(shift_type_elt);
    if( strcmp(text, "Any") == 0 )
      sts = NrcInstanceAllShiftTypes(ii->ins), po = NRC_POSITIVE;
    else if( strcmp(text, "None") == 0 )
      sts = NrcInstanceAllShiftTypes(ii->ins), po = NRC_NEGATIVE;
    else if( NrcInstanceRetrieveShiftType(ii->ins, text, &st) )
      sts = NrcShiftTypeSingletonShiftTypeSet(st), po = NRC_POSITIVE;
    else
    {
      KmlEltFatalError(shift_type_elt, ii->file_name,
	"unknown <ShiftType> %s", text);
      sts = NULL, po = NRC_POSITIVE;  /* keep compiler happy */
    }
    NrcPatternAddTerm(res, sts, po);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPatterns(INRC1_INFO ii)                                          */
/*                                                                           */
/*  Add patterns to ii.                                                      */
/*                                                                           */
/*****************************************************************************/

static void AddPatterns(INRC1_INFO ii)
{
  KML_ELT patterns_elt, pattern_elt;  KML_ERROR ke;  int i;
  if( KmlContainsChild(ii->root_elt, "Patterns", &patterns_elt) )
  {
    if( !KmlCheck(patterns_elt, ": *Pattern", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(patterns_elt);  i++ )
    {
      pattern_elt = KmlChild(patterns_elt, i);
      PatternMakeFromKmlElt(ii, pattern_elt);
    }
  }
  if( DEBUG7 )
  {
    NRC_PATTERN pattern;
    fprintf(stderr, "[ Patterns:\n");
    for( i = 0;  i < NrcInstancePatternCount(ii->ins);  i++ )
    {
      pattern = NrcInstancePattern(ii->ins, i);
      NrcPatternDebug(pattern, 2, stderr);
    }
    fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "weekends"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool DayBeginsWeekend(NRC_INSTANCE ins, int i, NRC_DAY_SET_SET gdays)    */
/*                                                                           */
/*  Return true if the i'th day of cycle is the starting day of a weekend    */
/*  as defined generically by gdays.                                         */
/*                                                                           */
/*****************************************************************************/

static bool DayBeginsWeekend(NRC_INSTANCE ins, int i, NRC_DAY_SET_SET gdays)
{
  NRC_DAY d;  int j;
  for( j = 0;  j < NrcDaySetSetDaySetCount(gdays);  j++ )
  {
    d = NrcInstanceCycleDay(ins, i + j);
    if( NrcDayDayOfWeek(d) != NrcDaySetSetDaySet(gdays, j) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET DayWeekend(NRC_INSTANCE ins, int i, NRC_DAY_SET_SET gdays)   */
/*                                                                           */
/*  Return the set of days just found by DayBeginsWeekend.                   */
/*                                                                           */
/*  The short name is the first day name followed by the other days'         */
/*  day of the week name, e.g. "1SatSun"; the long name is the same,         */
/*  only using long names.                                                   */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY_SET DayWeekend(NRC_INSTANCE ins, int i, NRC_DAY_SET_SET gdays,
  HA_ARENA a)
{
  NRC_DAY_SET res;  NRC_DAY d;  int j;  char *short_name, *long_name;

  /* build short and long names */
  d = NrcInstanceCycleDay(ins, i);
  short_name = NrcDayShortName(d);
  long_name = NrcDayLongName(d);
  for( j = 1;  j < NrcDaySetSetDaySetCount(gdays);  j++ )
  {
    d = NrcInstanceCycleDay(ins, i + j);
    short_name = HnStringMake(a, "%s%s", short_name, NrcDayShortName(d));
    long_name = HnStringMake(a, "%s%s", long_name, NrcDayLongName(d));
  }

  /* build res and add its days to it */
  res = NrcDaySetMake(ins, short_name, long_name);
  for( j = 0;  j < NrcDaySetSetDaySetCount(gdays);  j++ )
  {
    d = NrcInstanceCycleDay(ins, i + j);
    NrcDaySetAddDay(res, d);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET_SET BuildWeekends(INRC1_INFO ii, KML_ELT contract_elt)       */
/*                                                                           */
/*  Build a day-set set whose day-sets are the weekends for contract_elt.    */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY_SET_SET BuildWeekends(INRC1_INFO ii, KML_ELT contract_elt)
{
  KML_ELT weekend_elt;  NRC_DAY_SET_SET res, gdays;  NRC_DAY_SET dow;
  char *str, *s, *p, ch;  int i, len;

  /* make res */
  if( !KmlContainsChild(contract_elt, "WeekendDefinition", &weekend_elt) )
    HnAbort("BuildWeekends: missing WeekendDefinition");
  str = KmlText(weekend_elt);
  res = NrcDaySetSetMake(ii->ins, str, str);

  /* get the days sets corresponding to the days */
  gdays = NrcDaySetSetMake(ii->ins, "tmp", "tmp");
  s = HnStringCopy(str, ii->arena);
  p = strstr(s, "day");
  while( p != NULL )
  {
    ch = p[3];
    p[3] = '\0';
    if( !NrcDaySetSetRetrieveDaySetLong(NrcInstanceDaysOfWeek(ii->ins),s,&dow) )
      KmlEltFatalError(weekend_elt, ii->file_name, "unknown day name %s in %s",
	s, str);
    NrcDaySetSetAddDaySet(gdays, dow);
    p[3] = ch;
    s = &p[3];
    p = strstr(s, "day");
  }
  if( NrcDaySetSetDaySetCount(gdays) == 0 )
    KmlEltFatalError(weekend_elt, ii->file_name, "no day names in %s", str);
  if( s[0] != '\0' )
    KmlEltFatalError(weekend_elt, ii->file_name,
      "unknown last day name %s in %s", s, str);
  if( DEBUG6 )
  {
    fprintf(stderr, "[ BuildWeekends(%s) generic_days:\n", str);
    NrcDaySetSetDebug(gdays, 2, stderr);
    fprintf(stderr, "]\n");
  }

  /* add one day set for each weekend to res and return */
  len = NrcDaySetSetDaySetCount(gdays);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins) - len + 1;  i++ )
    if( DayBeginsWeekend(ii->ins, i, gdays) )
      NrcDaySetSetAddDaySet(res, DayWeekend(ii->ins, i, gdays, ii->arena));
  if( DEBUG6 )
  {
    fprintf(stderr, "[ BuildWeekends(%s) returning:\n", str);
    NrcDaySetSetDebug(res, 2, stderr);
    fprintf(stderr, "]\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "contracts"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KmlTextBoolean(INRC1_INFO ii, KML_ELT elt)                          */
/*                                                                           */
/*  Make sure that the text of elt is "true" or "false", and return the      */
/*  corresponding boolean value.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KmlTextBoolean(INRC1_INFO ii, KML_ELT elt)
{
  if( strcmp(KmlText(elt), "true") == 0 )
    return true;
  else if( strcmp(KmlText(elt), "false") == 0 )
    return false;
  else
  {
    KmlEltFatalError(elt, ii->file_name, "true or false expected here");
    return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasBooleanConstraint(INRC1_INFO ii, KML_ELT contract_elt,   */
/*    char *label, int *weight)                                              */
/*                                                                           */
/*  If contract_elt contains a constraint with the given label, set *weight  */
/*  to its weight and return true, otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasBooleanConstraint(INRC1_INFO ii, KML_ELT contract_elt,
  char *label, int *weight)
{
  KML_ELT elt;  KML_ERROR ke;
  if( !KmlContainsChild(contract_elt, label, &elt) )
    return false;
  if( !KmlCheck(elt, "#weight :", &ke) )
    KmlFatalError(ke, ii->file_name);
  sscanf(KmlAttributeValue(elt, 0), "%d", weight);
  return *weight > 0 && KmlTextBoolean(ii, elt);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlTextInteger(INRC1_INFO ii, KML_ELT elt, int *val)                */
/*                                                                           */
/*  Make sure that the text of elt is an integer, set *val to it, and        */
/*  return true.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KmlTextInteger(INRC1_INFO ii, KML_ELT elt, int *val)
{
  if( sscanf(KmlText(elt), "%d", val) != 1 )
    KmlEltFatalError(elt, ii->file_name, "integer expected here");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasLimitConstraint(INRC1_INFO ii, KML_ELT contract_elt,     */
/*    char *label, int *weight, int *limit)                                  */
/*                                                                           */
/*  If contract_elt contains a constraint with the given label, set *weight  */
/*  and *limit to its attributes and return true, otherwise return false.    */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasLimitConstraint(INRC1_INFO ii, KML_ELT contract_elt,
  char *label, int *weight, int *limit)
{
  KML_ELT elt;  KML_ERROR ke;  int on;
  if( !KmlContainsChild(contract_elt, label, &elt) )
    return false;
  if( !KmlCheck(elt, "#on #weight :", &ke) )
    KmlFatalError(ke, ii->file_name);
  sscanf(KmlAttributeValue(elt, 0), "%d", &on);
  sscanf(KmlAttributeValue(elt, 1), "%d", weight);
  return on == 1 && *weight > 0 && KmlTextInteger(ii, elt, limit);
}


/*****************************************************************************/
/*                                                                           */
/*  void SingleAssignmentPerDay(INRC1_INFO ii, KML_ELT contracts_elt)        */
/*                                                                           */
/*  Generate single assignment per day constraints.                          */
/*                                                                           */
/*  NB although an entry for this appears in each contract, it is also       */
/*  stated in the documentation that this is a hard constraint for each      */
/*  nurse.  We go with this second rule.                                     */
/*                                                                           */
/*****************************************************************************/

static void SingleAssignmentPerDay(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_SHIFT_SET starting_ss;  NRC_SHIFT_SET_SET day_sss;
  NRC_PENALTY p;
  starting_ss = NrcInstanceDailyStartingShiftSet(ii->ins);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "Single assignment per day", contract_ws,
    NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(1, p, ii->ins), starting_ss);
  /* ***
  c = NrcConstraintMake(ii->ins, "Single assignment per day", contract_ws,
    p, NrcLimit(NRC_LIMIT_MAX, 1), starting_ss);
  *** */
  day_sss = NrcDayShiftSetSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSetSet(c, day_sss, NRC_POSITIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxNumAssignments(INRC1_INFO ii, KML_ELT contracts_elt)             */
/*                                                                           */
/*  Handle groups of MaxNumAssignments constraints.                          */
/*                                                                           */
/*****************************************************************************/

static void MaxNumAssignments(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "MaxNumAssignments",
	&weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "Maximum number of assignments",
      contract_ws, NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(limit, p, ii->ins),
      NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "Maximum number of assignments",
      contract_ws, p, NrcLimit(NRC_LIMIT_MAX, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinNumAssignments(INRC1_INFO ii, KML_ELT contracts_elt)             */
/*                                                                           */
/*  Handle groups of MinNumAssignments constraints.                          */
/*                                                                           */
/*****************************************************************************/

static void MinNumAssignments(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "MinNumAssignments",
	&weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "Minimum number of assignments",
      contract_ws, NRC_CONSTRAINT_ACTIVE,
      NrcBoundMakeMin(limit, false, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "Minimum number of assignments",
      contract_ws, p, NrcLimit(NRC_LIMIT_MIN, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveDays(INRC1_INFO ii, KML_ELT contracts_elt,            */
/*    bool working)                                                          */
/*                                                                           */
/*  Handle one MaxConsecutiveWorkingDays or MaxConsecutiveFreeDays element.  */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveDays(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, bool working)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label, *label2;  NRC_PENALTY p;
  label = working ? "MaxConsecutiveWorkingDays" : "MaxConsecutiveFreeDays";
  label2 = working ? "Maximum consecutive working days" :
    "Maximum consecutive free days";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      working ? NRC_POSITIVE : NRC_NEGATIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveDays(INRC1_INFO ii, KML_ELT contracts_elt,            */
/*    bool working)                                                          */
/*                                                                           */
/*  Handle MinConsecutiveWorkingDays and MinConsecutiveFreeDays.             */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveDays(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, bool working)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label, *label2;  NRC_PENALTY p;
  label = working ? "MinConsecutiveWorkingDays" : "MinConsecutiveFreeDays";
  label2 = working ? "Minimum consecutive working days" :
    "Minimum consecutive free days";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(limit, false, p, ii->ins),
      NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      working ? NRC_POSITIVE : NRC_NEGATIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveWorkingWeekends(INRC1_INFO ii, KML_ELT contract_elt,  */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxConsecutiveWorkingWeekends constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveWorkingWeekends(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label, *label2;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;  NRC_PENALTY p;
  label = "MaxConsecutiveWorkingWeekends";
  label2 = "Maximum consecutive working weekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
    *** */
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveWeekends(INRC1_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss, bool working)*/
/*                                                                           */
/*  Handle the MinConsecutiveWorkingWeekends constraints.                    */
/*                                                                           */
/*  There is in fact no MinConsecutiveFreeWeekends constraint, but it is     */
/*  easier to pretend there is, since this code follows MinConsecutiveDays.  */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveWeekends(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss, bool working)
{
  char *label, *label2;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;  NRC_PENALTY p;
  label = working?"MinConsecutiveWorkingWeekends":"MinConsecutiveFreeWeekends";
  label2 = working ? "Minimum consecutive working weekends" :
    "Minimum consecutive free weekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(limit, false, p, ii->ins),
      NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, limit), NULL);
    *** */
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds),
	working? NRC_POSITIVE : NRC_NEGATIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingWeekendsInFourWeeks(INRC1_INFO ii, KML_ELT contract_elt,  */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxWorkingWeekendsInFourWeeks constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingWeekendsInFourWeeks(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label, *label2;  int i, four, weight, limit;  NRC_CONSTRAINT c;
  NRC_DAY_SET weekend_ds;  NRC_PENALTY p;

  label = "MaxWorkingWeekendsInFourWeeks";
  label2 = "Maximum working weekends in four weeks";
  four = 4;
  if( NrcDaySetSetDaySetCount(weekends_dss) >= four &&
      ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    /* add the constraint */
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(limit, p, ii->ins),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MAX, limit),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    *** */

    /* add one time group for each of the first four weekends */
    for( i = 0;  i < four;  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CompleteWeekendsAndIdenticalShiftTypesDuringWeekend(INRC1_INFO ii,  */
/*    KML_ELT contract_elt, NRC_WORKER_SET contract_ws,                      */
/*    NRC_DAY_SET_SET weekends_dss)                                          */
/*                                                                           */
/*  Do CompleteWeekends and IdenticalShiftTypesDuringWeekend constraints.    */
/*  These are handled together because a violation of the complete weekends  */
/*  constraint is also a violation of the identical shift types during       */
/*  weekend constraint, according to the competition evaluator, but the      */
/*  constraint generated here for the identical shift types constraint does  */
/*  not cover the incomplete weekend case.                                   */
/*                                                                           */
/*****************************************************************************/

static void CompleteWeekendsAndIdenticalShiftTypesDuringWeekend(INRC1_INFO ii,
  KML_ELT contract_elt, NRC_WORKER_SET contract_ws,
  NRC_DAY_SET_SET weekends_dss)
{
  int cw_weight, is_weight, i;  char *cw_label, *is_label, *label2;
  NRC_DAY d;  NRC_DAY_SET weekend_ds;
  NRC_SHIFT_SET ss;  NRC_CONSTRAINT c;  NRC_SHIFT_TYPE st;  NRC_PENALTY p;

  /* do nothing if there are no weekends */
  if( NrcDaySetSetDaySetCount(weekends_dss) == 0 )
    return;

  /* do nothing if the first weekend has less than 2 days */
  weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
  if( NrcDaySetDayCount(weekend_ds) < 2 )
    return;

  /* work out which of the two constraints are present with non-zero weight */
  cw_label = "CompleteWeekends";
  is_label = "IdenticalShiftTypesDuringWeekend";
  if( !ContractHasBooleanConstraint(ii, contract_elt, is_label, &is_weight) )
    is_weight = 0;
  if( !ContractHasBooleanConstraint(ii, contract_elt, cw_label, &cw_weight) )
    cw_weight = 0;

  /* the identical shift types constraint, without the complete weekends bit */
  if( is_weight > 0 )
  {
    /* one constraint, repeated weekly */
    label2 = "Identical shift types during weekend";
    p = NrcPenalty(false, is_weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(1, p, ii->ins),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MAX, 1), NrcInstanceWeeklyStartingShiftSet(ii->ins));
    *** */

    /* one time-group for each shift type */
    for( i = 0;  i < NrcInstanceShiftTypeCount(ii->ins);  i++ )
    {
      st = NrcInstanceShiftType(ii->ins, i);
      ss = NrcDaySetShiftSetFromShiftType(weekend_ds, st);
      NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
    }
  }

  /* the possibly combined complete weekends constraint */
  if( is_weight + cw_weight > 0 )
  {
    /* one constraint, repeated weekly */
    if( is_weight == 0 )
      label2 = "Complete weekends";
    else if( cw_weight == 0 )
      label2 = "Identical shift types during weekend";
    else
      label2 = "Complete weekends + identical shift types during weekend";
    p = NrcPenalty(false, is_weight + cw_weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws, NRC_CONSTRAINT_ACTIVE,
      NrcBoundMakeMin(NrcDaySetDayCount(weekend_ds), true, p, ii->ins),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MIN_OR_ZERO, NrcDaySetDayCount(weekend_ds)),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    *** */

    /* one shift-set for each day on the first weekend */
    for( i = 0;  i < NrcDaySetDayCount(weekend_ds);  i++ )
    {
      d = NrcDaySetDay(weekend_ds, i);
      NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CompleteWeekends(INRC1_INFO ii, KML_ELT contract_elt,               */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the CompleteWeekends constraints.                                 */
/*                                                                           */
/*  For each weekend, one constraint containing one positive time group for  */
/*  each day of that weekend, containing the times of that day, with minimum */
/*  limit equalling the number of days in the weekend or else zero.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void CompleteWeekends(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  int i, weight;  char *label, *label2;  NRC_DAY d;
  NRC_DAY_SET weekend_ds;  NRC_CONSTRAINT c;  NRC_PENALTY p;

  label = "CompleteWeekends";
  label2 = "Complete weekends";
  if( NrcDaySetSetDaySetCount(weekends_dss) >= 1 &&
      ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
    if( NrcDaySetDayCount(weekend_ds) > 1 )
    {
      ** make constraint **
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
	NrcLimit(NRC_LIMIT_MIN_OR_ZERO, NrcDaySetDayCount(weekend_ds)),
	NrcInstanceWeeklyStartingShiftSet(ii->ins));

      ** one shift-set for each day on the first weekend **
      for( i = 0;  i < NrcDaySetDayCount(weekend_ds);  i++ )
      {
	d = NrcDaySetDay(weekend_ds, i);
	NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void IdenticalShiftTypesDuringWeekend(INRC1_INFO ii,                     */
/*    KML_ELT contract_elt, NRC_WORKER_SET contract_ws,                      */
/*    NRC_DAY_SET_SET weekends_dss)                                          */
/*                                                                           */
/*  Handle one IdenticalShiftTypesDuringWeekend element.                     */
/*                                                                           */
/*  For each weekend, one cluster busy times constraint containing one       */
/*  positive time group for each shift on that weekend, containing the       */
/*  times of the shift on the weekend, with maximum limit 1.                 */
/*                                                                           */
/*****************************************************************************/

/* *** erroneous when a resource is busy on one of the two days but not both
static void IdenticalShiftTypesDuringWeekend(INRC1_INFO ii,
  KML_ELT contract_elt, NRC_WORKER_SET contract_ws,
  NRC_DAY_SET_SET weekends_dss)
{
  int i, weight;  char *label, *label2;  NRC_DAY_SET weekend_ds;
  NRC_SHIFT_SET ss;  NRC_CONSTRAINT c;  NRC_SHIFT_TYPE st;  NRC_PENALTY p;

  label = "IdenticalShiftTypesDuringWeekend";
  label2 = "Identical shift types during weekend";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
    if( NrcDaySetDayCount(weekend_ds) > 1 )
    {
      ** one constraint, repeated weekly **
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
	NrcLimit(NRC_LIMIT_MAX, 1), NrcInstanceWeeklyStartingShiftSet(ii->ins));

      ** one time-group for each shift type **
      for( i = 0;  i < NrcInstanceShiftTypeCount(ii->ins);  i++ )
      {
	st = NrcInstanceShiftType(ii->ins, i);
	ss = NrcDaySetShiftSetFromShiftType(weekend_ds, st);
	NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void IdenticalShiftTypesDuringWeekend(INRC1_INFO ii,                     */
/*    KML_ELT contract_elt, NRC_WORKER_SET contract_ws,                      */
/*    NRC_DAY_SET_SET weekends_dss)                                          */
/*                                                                           */
/*  Handle one IdenticalShiftTypesDuringWeekend element.                     */
/*                                                                           */
/*  For each group, one limit busy times constraint containing one time      */
/*  group for each shift on each weekend, containing the times of the        */
/*  shift on the weekend, with minimum limit equal to that number of         */
/*  times, or else 0.                                                        */
/*                                                                           */
/*  If a resource has different shifts on the weekend, then two of the       */
/*  time groups will be violated, not one.  This is handled by using         */
/*  the Step cost function rather than the usual Linear.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void IdenticalShiftTypesDuringWeekend(INRC1_INFO ii,
  KML_ELT contract_elt, NRC_WORKER_SET contract_ws,
  NRC_DAY_SET_SET weekends_dss)
{
  int i, j, weight;  char *label, *label2;  NRC_DAY d;  NRC_DAY_SET weekend_ds;
  NRC_SHIFT_SET ss;  NRC_CONSTRAINT c;  NRC_SHIFT_TYPE st;  NRC_PENALTY p;

  label = "IdenticalShiftTypesDuringWeekend";
  label2 = "Identical shift types during weekend";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
    if( NrcDaySetDayCount(weekend_ds) > 1 )
    {
      for( i = 0;  i < NrcInstanceShiftTypeCount(ii->ins);  i++ )
      {
	st = NrcInstanceShiftType(ii->ins, i);

	** one constraint for each shift type **
	p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
	c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
	  NrcLimit(NRC_LIMIT_MIN_OR_ZERO, 2),
	  NrcInstanceWeeklyStartingShiftSet(ii->ins));

	** one shift-set for each day of the weekend **
	for( j = 0;  j < NrcDaySetDayCount(weekend_ds);  j++ )
	{
	  d = NrcDaySetDay(weekend_ds, j);
	  ss = NrcShiftSingletonShiftSet(NrcDayShiftFromShiftType(d, st));
	  NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
	}
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool CycleHasWeekendWithPrecedingDay(NRC_INSTANCE ins,                   */
/*    NRC_DAY_SET_SET weekends_dss, NRC_DAY_SET *weekend_ds, NRC_DAY *prec_d)*/
/*                                                                           */
/*  Given that weekends_dss contains the weekends of ins, return true if     */
/*  the cycle contains at least one weekend with a preceding day, and set    */
/*  *weekend_ds and *prec_day to that weekend and day.  Else return false.   */
/*                                                                           */
/*****************************************************************************/

static bool CycleHasWeekendWithPrecedingDay(NRC_INSTANCE ins,
  NRC_DAY_SET_SET weekends_dss, NRC_DAY_SET *weekend_ds, NRC_DAY *prec_d)
{
  NRC_DAY d;  NRC_DAY_SET ds;  int i;

  for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
  {
    /* if the i'th weekend has a preceding day, that's what we want */
    ds = NrcDaySetSetDaySet(weekends_dss, i);
    HnAssert(NrcDaySetDayCount(ds) > 0,
      "CycleHasWeekendWithPrecedingDay: empty weekend");
    d = NrcDaySetDay(ds, 0);
    if( NrcDayIndexInCycle(d) > 0 )
    {
      *weekend_ds = ds;
      *prec_d = NrcInstanceCycleDay(ins, NrcDayIndexInCycle(d) - 1);
      return true;
    }
  }

  /* no luck so return false */
  *weekend_ds = NULL;
  *prec_d = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool InstanceHasNightShifts(INRC1_INFO ii)                               */
/*                                                                           */
/*  Return true if there is at least one night shift, else false.            */
/*                                                                           */
/*****************************************************************************/

static bool InstanceHasNightShifts(INRC1_INFO ii)
{
  int i;  bool is_night;
  HaArrayForEach(ii->shift_type_is_night, is_night, i)
    if( is_night )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool InstanceHasNonNightShifts(INRC1_INFO ii)                            */
/*                                                                           */
/*  Return true if there is at least one non-night shift, else false.        */
/*                                                                           */
/*****************************************************************************/

static bool InstanceHasNonNightShifts(INRC1_INFO ii)
{
  int i;  bool is_night;
  HaArrayForEach(ii->shift_type_is_night, is_night, i)
    if( !is_night )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NightShiftsOnDay(INRC1_INFO ii, NRC_DAY d)                 */
/*                                                                           */
/*  Return the set of night shifts on day d.                                 */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET NightShiftsOnDay(INRC1_INFO ii, NRC_DAY d)
{
  NRC_SHIFT_SET res;  bool is_night;  int i;  NRC_SHIFT_TYPE st;
  res = NrcShiftSetMake(ii->ins);
  HaArrayForEach(ii->shift_type_is_night, is_night, i)
    if( is_night )
    {
      st = NrcInstanceShiftType(ii->ins, i);
      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NonNightShiftsOnDay(INRC1_INFO ii, NRC_DAY d)              */
/*                                                                           */
/*  Return the set of non-night shifts on day d.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET NonNightShiftsOnDay(INRC1_INFO ii, NRC_DAY d)
{
  NRC_SHIFT_SET res;  bool is_night;  int i;  NRC_SHIFT_TYPE st;
  res = NrcShiftSetMake(ii->ins);
  HaArrayForEach(ii->shift_type_is_night, is_night, i)
    if( !is_night )
    {
      st = NrcInstanceShiftType(ii->ins, i);
      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NoNightShiftBeforeFreeWeekend(INRC1_INFO ii, KML_ELT contract_elt,  */
/*                                                                           */
/*  Handle the NoNightShiftBeforeFreeWeekend constraints.                    */
/*                                                                           */
/*  This is just pattern Fri:[3][0][0], where Fri means the day (if any)     */
/*  before the first day of WeekendDefinition, and 3 means any night shift.  */
/*                                                                           */
/*****************************************************************************/

static void NoNightShiftBeforeFreeWeekend(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  int i, weight;  char *label, *label2;  NRC_DAY d;  NRC_PENALTY p;
  NRC_DAY_SET weekend_ds;  NRC_CONSTRAINT c;

  label = "NoNightShiftBeforeFreeWeekend";
  label2 = "No night shift before free weekend";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) &&
      InstanceHasNightShifts(ii) &&
      CycleHasWeekendWithPrecedingDay(ii->ins, weekends_dss, &weekend_ds, &d) )
  {
    p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, label2, contract_ws, NRC_CONSTRAINT_ACTIVE,
      NrcBoundMakeMax(NrcDaySetDayCount(weekend_ds), p, ii->ins),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    /* ***
    c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
      NrcLimit(NRC_LIMIT_MAX, NrcDaySetDayCount(weekend_ds)),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
    *** */

    /* the first shift-set is the set of night shifts on d, positively */
    NrcConstraintAddShiftSet(c, NightShiftsOnDay(ii, d), NRC_POSITIVE);

    /* the other shift-sets are the shifts of the weekend's days, negatively */
    for( i = 0;  i < NrcDaySetDayCount(weekend_ds);  i++ )
    {
      d = NrcDaySetDay(weekend_ds, i);
      NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_NEGATIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void TwoFreeDaysAfterNightShifts(INRC1_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the TwoFreeDaysAfterNightShifts constraints.                      */
/*                                                                           */
/*  Implementation note.  As explained in detail in the paper, because       */
/*  two busy days are not considered worse than one busy day, this is        */
/*  equivalent to unwanted patterns [3][12] and [3][0][12].                  */
/*                                                                           */
/*****************************************************************************/

static void TwoFreeDaysAfterNightShifts(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_DAY d;  char *label, *label2;  int weight;
  NRC_PENALTY p;
  label = "TwoFreeDaysAfterNightShifts";
  label2 = "Two free days after night shifts";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) &&
      InstanceHasNightShifts(ii) && InstanceHasNonNightShifts(ii) )
  {
    /* [3][12] */
    if( NrcInstanceCycleDayCount(ii->ins) >= 2 )
    {
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      c = NrcConstraintMake(ii->ins, label2, contract_ws,
	NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(1, p, ii->ins),
	NrcInstanceDailyStartingShiftSet(ii->ins));
      /* ***
      c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
	NrcLimit(NRC_LIMIT_MAX, 1), NrcInstanceDailyStartingShiftSet(ii->ins));
      *** */
      d = NrcInstanceCycleDay(ii->ins, 0);
      NrcConstraintAddShiftSet(c, NightShiftsOnDay(ii, d), NRC_POSITIVE);
      d = NrcInstanceCycleDay(ii->ins, 1);
      NrcConstraintAddShiftSet(c, NonNightShiftsOnDay(ii, d), NRC_POSITIVE);
    }

    /* [3][0][12] */
    if( NrcInstanceCycleDayCount(ii->ins) >= 3 )
    {
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      c = NrcConstraintMake(ii->ins, label2, contract_ws,
	NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(2, p, ii->ins),
	NrcInstanceDailyStartingShiftSet(ii->ins));
      /* ***
      c = NrcConstraintMake(ii->ins, label2, contract_ws, p,
	NrcLimit(NRC_LIMIT_MAX, 2), NrcInstanceDailyStartingShiftSet(ii->ins));
      *** */
      d = NrcInstanceCycleDay(ii->ins, 0);
      NrcConstraintAddShiftSet(c, NightShiftsOnDay(ii, d), NRC_POSITIVE);
      d = NrcInstanceCycleDay(ii->ins, 1);
      NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_NEGATIVE);
      d = NrcInstanceCycleDay(ii->ins, 2);
      NrcConstraintAddShiftSet(c, NonNightShiftsOnDay(ii, d), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void UnwantedPatterns(INRC1_INFO ii, KML_ELT contracts_elt)              */
/*                                                                           */
/*  Handle the UnwantedPatterns constraints.                                 */
/*                                                                           */
/*  These do not have the usual format (weights, limits, etc.).              */
/*                                                                           */
/*****************************************************************************/

static void UnwantedPatterns(INRC1_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int i, p_index, weight;  KML_ERROR ke;  char *label, *label2;
  NRC_PENALTY penalty;  NRC_PATTERN p;  KML_ELT patterns_elt, pattern_elt;
  NRC_DAY_SET starting_ds;
  label = "UnwantedPatterns";
  label2 = "Unwanted pattern";
  if( KmlContainsChild(contract_elt, label, &patterns_elt) )
  {
    if( !KmlCheck(patterns_elt, ": *$Pattern", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(patterns_elt);  i++ )
    {
      pattern_elt = KmlChild(patterns_elt, i);

      /* find the pattern and its associated weight and starting day-set */
      if( !NrcInstanceRetrievePattern(ii->ins, KmlText(pattern_elt), &p) )
	KmlEltFatalError(pattern_elt, ii->file_name, "unknown pattern %s",
	  KmlText(pattern_elt));
      p_index = NrcPatternIndexInInstance(p);
      weight = HaArray(ii->pattern_weight, p_index);
      starting_ds = HaArray(ii->pattern_starting_day_set, p_index);

      /* add a constraint for this pattern, holding the pattern */
      penalty = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcUnwantedPatternConstraintMake(ii->ins, label2, contract_ws,
	penalty, p, starting_ds);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContractsAndContractConstraints(INRC1_INFO ii)                   */
/*                                                                           */
/*  Add contracts, both the worker sets and the constraints.                 */
/*                                                                           */
/*  Implementation note.  At this point the workers have not been added      */
/*  to the contract worker sets; but it's OK to do that later, as long as    */
/*  it is done before calling NrcInstanceConvert.                            */
/*                                                                           */
/*  There is however a problem with this for AlternativeSkillCategory,       */
/*  which is handled per resource in NRC.  So it's done separately, after    */
/*  AddEmployees.                                                            */
/*                                                                           */
/*****************************************************************************/

static void AddContractsAndContractConstraints(INRC1_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  KML_ERROR ke;  int i, wt;
  NRC_DAY_SET_SET weekends_dss;  NRC_WORKER_SET contract_ws;  char *id, *label;
  NRC_PENALTY p;

  if( KmlContainsChild(ii->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      if( !KmlCheck(contract_elt, "ID : +Description +SingleAssignmentPerDay "
	    "+MaxNumAssignments +MinNumAssignments "
	    "+MaxConsecutiveWorkingDays +MinConsecutiveWorkingDays "
	    "+MaxConsecutiveFreeDays +MinConsecutiveFreeDays "
	    "+MaxConsecutiveWorkingWeekends +MinConsecutiveWorkingWeekends "
	    "+MaxWorkingWeekendsInFourWeeks +$WeekendDefinition "
	    "+CompleteWeekends +IdenticalShiftTypesDuringWeekend "
	    "+NoNightShiftBeforeFreeWeekend +TwoFreeDaysAfterNightShifts "
	    "+AlternativeSkillCategory +UnwantedPatterns ", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* create worker set and weekends for this contract */
      id = HnStringMake(ii->arena, "Contract-%s",
	KmlAttributeValue(contract_elt, 0));
      contract_ws = NrcWorkerSetMake(ii->ins, id);
      NrcInstanceContractsAddContract(ii->ins, contract_ws);
      weekends_dss = BuildWeekends(ii, contract_elt);

      /* generate constraints for the contract */
      SingleAssignmentPerDay(ii, contract_elt, contract_ws);
      MaxNumAssignments(ii, contract_elt, contract_ws);
      MinNumAssignments(ii, contract_elt, contract_ws);
      MaxConsecutiveDays(ii, contract_elt, contract_ws, true);
      MinConsecutiveDays(ii, contract_elt, contract_ws, true);
      MaxConsecutiveDays(ii, contract_elt, contract_ws, false);
      MinConsecutiveDays(ii, contract_elt, contract_ws, false);
      MaxConsecutiveWorkingWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MinConsecutiveWeekends(ii, contract_elt, contract_ws, weekends_dss, true);
      MaxWorkingWeekendsInFourWeeks(ii, contract_elt, contract_ws,weekends_dss);
      CompleteWeekendsAndIdenticalShiftTypesDuringWeekend(ii, contract_elt,
	    contract_ws, weekends_dss);
      /* ***
      CompleteWeekends(ii, contract_elt, contract_ws, weekends_dss);
      IdenticalShiftTypesDuringWeekend(ii, contract_elt, contract_ws,
	weekends_dss);
      *** */
      NoNightShiftBeforeFreeWeekend(ii, contract_elt, contract_ws,weekends_dss);
      TwoFreeDaysAfterNightShifts(ii, contract_elt, contract_ws);
      UnwantedPatterns(ii, contract_elt, contract_ws);

      /* save alternative skill penalty for the contract */
      label = "AlternativeSkillCategory";
      if( ContractHasBooleanConstraint(ii, contract_elt, label, &wt) )
	p = NrcPenalty(false, wt, NRC_COST_FUNCTION_LINEAR, ii->ins);
      else
	p = NrcPenalty(false, wt, NRC_COST_FUNCTION_LINEAR, ii->ins);
      HaArrayAddLast(ii->alternative_skill_penalties, p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "workers"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *TidyNurseIdOrName(char *id, HA_ARENA a)                            */
/*                                                                           */
/*  Tidy up a nurse Id or name, by copying it into malloced memory, and      */
/*  prepending "Nurse" if it does not start with a letter.                   */
/*                                                                           */
/*****************************************************************************/

/* *** done by NRC now
static char *TidyNurseIdOrName(char *id, HA_ARENA a)
{
  if( is_letter(id[0]) )
    return HnStringCopy(id, a);
  else
    return HnStringMake(a, "Nurse%s", id);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET RetrieveContractWorkerSet(INRC1_INFO ii,                  */
/*    KML_ELT elt, char *id)                                                 */
/*                                                                           */
/*  Return the contract worker set with this id.  The elt parameter is only  */
/*  for error messages.                                                      */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET RetrieveContractWorkerSet(INRC1_INFO ii,
  KML_ELT elt, char *id)
{
  NRC_WORKER_SET ws;  char *contract_id;
  contract_id = HnStringMake(ii->arena, "Contract-%s", id);
  if( !NrcInstanceContractsRetrieveContract(ii->ins, contract_id, &ws) )
    KmlEltFatalError(elt, ii->file_name, "unknown contract %s", contract_id);
  return ws;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddEmployees(INRC1_INFO ii)                                         */
/*                                                                           */
/*  Add one worker for each employee, including adding them to their skills  */
/*  and contract worker sets.                                                */
/*                                                                           */
/*****************************************************************************/

static void AddEmployees(INRC1_INFO ii)
{
  KML_ELT employees_elt, employee_elt, skills_elt, skill_elt;
  KML_ELT contract_elt;  KML_ERROR ke; int i, j;  char *employee_id;
  NRC_WORKER w;  NRC_WORKER_SET ws;

  KmlContainsChild(ii->root_elt, "Employees", &employees_elt);
  if( !KmlCheck(employees_elt, ": *Employee", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(employees_elt);  i++ )
  {
    /* sort out names and create one worker for employee_elt */
    employee_elt = KmlChild(employees_elt, i);
    if( !KmlCheck(employee_elt, "ID : $ContractID $Name +Skills", &ke) )
      KmlFatalError(ke, ii->file_name);
    employee_id = KmlAttributeValue(employee_elt, 0);
    /* ***
    employee_id = TidyNurseIdOrName(KmlAttributeValue(employee_elt, 0),
      ii->arena);
    *** */
    w = NrcWorkerMake(ii->ins, employee_id);

    /* add w to its contract worker set */
    contract_elt = KmlChild(employee_elt, 0);
    ws = RetrieveContractWorkerSet(ii, contract_elt, KmlText(contract_elt));
    NrcWorkerSetAddWorker(ws, w);

    /* add resource to skills */
    if( KmlContainsChild(employee_elt, "Skills", &skills_elt) )
    {
      if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
	KmlFatalError(ke, ii->file_name);
      for( j = 0;  j < KmlChildCount(skills_elt);  j++ )
      {
        skill_elt = KmlChild(skills_elt, j);
	if( !NrcInstanceSkillsRetrieveSkill(ii->ins, KmlText(skill_elt), &ws) )
	  KmlEltFatalError(skill_elt, ii->file_name, "unknown skill %s",
	    KmlText(skill_elt));
	NrcWorkerSetAddWorker(ws, w);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddAlternativeSkillCategories(INRC1_INFO ii)                        */
/*                                                                           */
/*  Add information about alternative skill categories.  This has had to     */
/*  be delayed until after AddEmployees.                                     */
/*                                                                           */
/*****************************************************************************/

/* *** NrcWorkerAddSkillPenalty is obsolete and consequently so is this
static void AddAlternativeSkillCategories(INRC1_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  NRC_WORKER w;  NRC_PENALTY p;
  int i, j, wt;  NRC_WORKER_SET ws;  KML_ERROR ke;  char *label;

  ** add AlternativeSkillCategory weights **
  KmlContainsChild(ii->root_elt, "Contracts", &contracts_elt);
  if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
  {
    contract_elt = KmlChild(contracts_elt, i);
    ws = NrcInstanceContractsContract(ii->ins, i);
    label = "AlternativeSkillCategory";
    if( ContractHasBooleanConstraint(ii, contract_elt, label, &wt) )
    {
      p = NrcPenalty(false, wt, NRC_COST_FUNCTION_LINEAR, ii->ins);
      for( j = 0;  j < NrcWorkerSetWorkerCount(ws);  j++ )
      {
	w = NrcWorkerSetWorker(ws, j);
	NrcWorkerAddSkillPenalty(w, p);
	if( DEBUG1 )
	  fprintf(stderr, "  NrcWorkerAddSkillPenalty(%s, S%d)\n",
	    NrcWorkerName(w), wt);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddCoverRequirements(INRC1_INFO ii)                                 */
/*                                                                           */
/*  Add covers.                                                              */
/*                                                                           */
/*****************************************************************************/

static void AddCoverRequirements(INRC1_INFO ii)
{
  KML_ELT cover_req_elt, day_of_week_cover_elt, day_elt, shift_elt, cover_elt;
  KML_ERROR ke;  NRC_SHIFT_TYPE st;  int i, j, k, cover;  char *shift_id;
  NRC_DAY d;  NRC_DAY_SET ds;  NRC_DEMAND dm;  NRC_DEMAND_SET dms;
  NRC_WORKER_SET skill_ws, ws;  NRC_PENALTY p0, p1;  NRC_PENALTY p;

  if( KmlContainsChild(ii->root_elt, "CoverRequirements", &cover_req_elt) )
  {
    p0 = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, ii->ins);
    p1 = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    if( !KmlCheck(cover_req_elt, ": *DayOfWeekCover", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(cover_req_elt);  i++ )
    {
      day_of_week_cover_elt = KmlChild(cover_req_elt, i);
      if( !KmlCheck(day_of_week_cover_elt, ": $Day *Cover", &ke) )
	KmlFatalError(ke, ii->file_name);
      day_elt = KmlChild(day_of_week_cover_elt, 0);
      if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, KmlText(day_elt),
	    &ds) )
        KmlEltFatalError(day_elt, ii->file_name,
	  "unknown full weekday name %s", KmlText(day_elt));
      for( j = 1;  j < KmlChildCount(day_of_week_cover_elt);  j++ )
      {
	cover_elt = KmlChild(day_of_week_cover_elt, j);
	if( !KmlCheck(cover_elt, ": $Shift #Preferred", &ke) )
	  KmlFatalError(ke, ii->file_name);
	shift_elt = KmlChild(cover_elt, 0);
	shift_id = KmlText(shift_elt);
	if( !NrcInstanceRetrieveShiftType(ii->ins, shift_id, &st) )
	  KmlEltFatalError(shift_elt, ii->file_name,
	    "unknown shift type %s", shift_id);
	sscanf(KmlText(KmlChild(cover_elt, 1)), "%d", &cover);
	if( cover > 0 )
	{
	  skill_ws = HaArray(ii->shift_type_skill_ws, NrcShiftTypeIndex(st));
	  dm = NrcDemandMakeBegin(ii->ins);
	  NrcDemandPenalizeNonAssignment(dm, NRC_PENALTY_UNIQUE, p1);
	  HaArrayForEach(ii->alternative_skill_penalties, p, k)
	  {
            ws = NrcInstanceContractsContract(ii->ins, k);
	    NrcDemandPenalizeWorkerSet(dm, ws, NRC_PENALTY_UNIQUE, p);
	  }
	  NrcDemandPenalizeWorkerSet(dm, skill_ws, NRC_PENALTY_REPLACE, p0);
	  NrcDemandMakeEnd(dm);
	  /* ***
	  p1 = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
	  p2 = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, ii->ins);
	  dm = NrcDemandMake(ii->ins, p1, p2, skill_ws, p2);
	  *** */
	  dms = NrcDemandSetMake(ii->ins);
	  NrcDemandSetAddDemandMulti(dms, dm, cover);
	  for( k = 0;  k < NrcDaySetDayCount(ds);  k++ )
	  {
	    d = NrcDaySetDay(ds, k);
	    NrcShiftAddDemandSet(NrcDayShiftFromShiftType(d, st), dms);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE RetrieveResource(INRC1_INFO ii, char *id)                   */
/*                                                                           */
/*  Retrieve from ii->instance the resource with this id.  Note that we      */
/*  are prepending "Nurse" often.                                            */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER RetrieveWorker(INRC1_INFO ii, KML_ELT nurse_elt)
{
  char *id;  NRC_WORKER w;
  id = KmlText(nurse_elt);
  /* id = TidyNurseIdOrName(KmlText(nurse_elt), ii->arena); */
  if( !NrcInstanceStaffingRetrieveWorker(ii->ins, id, &w) )
    KmlEltFatalError(nurse_elt, ii->file_name, "cannot find worker %s", id);
  return w;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDayOffRequests(INRC1_INFO ii)                                    */
/*                                                                           */
/*  Add day off requests.                                                    */
/*                                                                           */
/*****************************************************************************/

static void AddDayOffRequests(INRC1_INFO ii)
{
  KML_ELT days_off_elt, day_off_elt, date_elt;  NRC_PENALTY p;  NRC_WORKER w;
  KML_ELT employee_elt;  KML_ERROR ke;  int i, weight;  NRC_DAY d;

  /* day off requests */
  if( KmlContainsChild(ii->root_elt, "DayOffRequests", &days_off_elt) )
  {
    if( !KmlCheck(days_off_elt, ": *DayOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(days_off_elt);  i++ )
    {
      /* get one day off element and its parts */
      day_off_elt = KmlChild(days_off_elt, i);
      if( !KmlCheck(day_off_elt, "#weight : $EmployeeID $Date", &ke) )
	KmlFatalError(ke, ii->file_name);
      employee_elt = KmlChild(day_off_elt, 0);
      date_elt = KmlChild(day_off_elt, 1);

      /* convert its parts into entities and record the day off */
      sscanf(KmlAttributeValue(day_off_elt, 0), "%d", &weight);
      w = RetrieveWorker(ii, employee_elt);
      if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(date_elt), &d) )
	KmlEltFatalError(date_elt, ii->file_name, "invalid day %s",
	  KmlText(date_elt));
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddDayOff(w, d, p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOffRequests(INRC1_INFO ii)                                  */
/*                                                                           */
/*  Add shift off and day off requests.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOffRequests(INRC1_INFO ii)
{
  KML_ELT date_elt, shifts_off_elt, shift_off_elt;  NRC_PENALTY p;
  KML_ELT employee_elt, shift_type_elt;  KML_ERROR ke;  int i, weight;
  NRC_DAY d;  NRC_WORKER w;  NRC_SHIFT_TYPE st;

  /* shift off requests */
  if( KmlContainsChild(ii->root_elt, "ShiftOffRequests", &shifts_off_elt) )
  {
    if( !KmlCheck(shifts_off_elt, ": *ShiftOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shifts_off_elt);  i++ )
    {
      /* get one shift off elt and its parts */
      shift_off_elt = KmlChild(shifts_off_elt, i);
      if( !KmlCheck(shift_off_elt, "weight : $ShiftTypeID $EmployeeID $Date",
	    &ke) )
	KmlFatalError(ke, ii->file_name);
      shift_type_elt = KmlChild(shift_off_elt, 0);
      employee_elt = KmlChild(shift_off_elt, 1);
      date_elt = KmlChild(shift_off_elt, 2);

      /* convert its parts into entities and record the shift off */
      sscanf(KmlAttributeValue(shift_off_elt, 0), "%d", &weight);
      w = RetrieveWorker(ii, employee_elt);
      if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(date_elt), &d) )
	KmlEltFatalError(date_elt, ii->file_name, "invalid day %s",
	  KmlText(date_elt));
      if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(shift_type_elt), &st) )
	KmlEltFatalError(shift_type_elt, ii->file_name, "unknown shift %s",
	  KmlText(shift_type_elt));
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddShiftOff(w, NrcDayShiftFromShiftType(d, st), p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main conversion functions"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *Replace(char *str, char *old, char *new, HA_ARENA a)               */
/*                                                                           */
/*  If old appears in str, replace it by new and return the new string,      */
/*  otherwise return the old string.                                         */
/*                                                                           */
/*****************************************************************************/

static char *Replace(char *str, char *old, char *new, HA_ARENA a)
{
  char *p;
  p = strstr(str, old);
  if( p == NULL )
    return str;
  *p = '\0';
  return HnStringMake(a, "%s%s%s", str, new, &p[strlen(old)]);
}


/*****************************************************************************/
/*                                                                           */
/*  char *INRC1InstanceIdNormalize(char *ins_id, HA_ARENA a)                 */
/*                                                                           */
/*  Return a normalized version of ins_id, replacing long by Long, etc.      */
/*                                                                           */
/*****************************************************************************/

static char *INRC1InstanceIdNormalize(char *ins_id, HA_ARENA a)
{
  char *res;
  res = HnStringMake(a, "INRC1-%s", ins_id);
  res = Replace(res, "long", "L", a);
  res = Replace(res, "medium", "M", a);
  res = Replace(res, "sprint", "S", a);
  res = Replace(res, "_hidden", "H", a);
  res = Replace(res, "_late", "L", a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE INRC1ConvertInstance(INSTANCE_MODEL ins_model,              */
/*    char *instance_file_name)                                              */
/*                                                                           */
/*  Read instance_file_name, a file in the format of the first international */
/*  nurse rostering competition, and return the resulting instance.          */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE INRC1ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  /* NRC_INSTANCE_METADATA md */;  KML_ERROR ke;  FILE *fp;  INRC1_INFO ii;
  char *ins_id;  KML_ELT elt;  HA_ARENA a;  NRC_INSTANCE res;

  /* make an instance info object, initially more or less empty */
  a = HaArenaMake(as);
  HaMake(ii, a);
  ii->arena = a;
  ii->file_name = instance_file_name;
  ii->root_elt = NULL;;
  ii->ins = NULL;
  HaArrayInit(ii->shift_type_is_night, a);
  HaArrayInit(ii->shift_type_skill_ws, a);
  HaArrayInit(ii->pattern_weight, a);
  HaArrayInit(ii->pattern_starting_day_set, a);
  HaArrayInit(ii->alternative_skill_penalties, a);

  /* open and read an XML instance file, creating ii->root_elt */
  fp = fopen(instance_file_name, "r");
  if( fp == NULL )
    FatalError(instance_file_name, 0, "cannot read file \"%s\"",
      instance_file_name);
  if( !KmlReadFile(fp, NULL, &ii->root_elt, &ke, a) )
    KmlFatalError(ke, instance_file_name);
  fclose(fp);

  /* make sure the root tag and its children are all present and correct */
  if( strcmp(KmlLabel(ii->root_elt), "SchedulingPeriod") != 0 )
    KmlEltFatalError(ii->root_elt, ii->file_name,
      "root tag %s where SchedulingPeriod expected", KmlLabel(ii->root_elt));
  if( !KmlCheck(ii->root_elt, "ID +OrganisationID "
      "+xmlns:xsi +xsi:noNamespaceSchemaLocation : "
      "$StartDate $EndDate +Skills ShiftTypes +Patterns Contracts Employees "
      "CoverRequirements +DayOffRequests +DayOnRequests +ShiftOffRequests "
      "+ShiftOnRequests", &ke) )
    KmlFatalError(ke, ii->file_name);

  /* abort if there are day on or shift on requests (not handled) */
  if( KmlContainsChild(ii->root_elt, "DayOnRequests", &elt) )
    KmlEltFatalError(elt, ii->file_name, "DayOnRequests still to do");
  if( KmlContainsChild(ii->root_elt, "ShiftOnRequests", &elt) )
    KmlEltFatalError(elt, ii->file_name, "ShiftOnRequests still to do");

  /* make instance id, adjusting long to Long, etc. */
  ins_id = INRC1InstanceIdNormalize(KmlAttributeValue(ii->root_elt, 0), a);

  /* make instance */
  /* ***
  md = InstanceModelMetaData(ins_model, ins_id, NULL, NULL, NULL, NULL, NULL);
  *** */
  ii->ins = NrcInstanceMakeBegin(ins_id, /* "Nurses", */ "Nurse", as);
  /* p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins); */
  NrcInstanceSetMetaData(ii->ins, ins_id, NULL, NULL, NULL, NULL, NULL);

  /* add the various parts of the instance */
  AddSkills(ii);
  AddShiftTypes(ii);  /* must come after skills */
  AddCycle(ii);  /* must come after shift types */
  AddPatterns(ii);
  AddContractsAndContractConstraints(ii);
  AddEmployees(ii);
  /* AddAlternativeSkillCategories(ii);  delayed until after AddEmployees */
  AddCoverRequirements(ii);
  AddDayOffRequests(ii);
  AddShiftOffRequests(ii);
  NrcInstanceMakeEnd(ii->ins);
  res = ii->ins;
  HaArenaDelete(a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void INRC1ConvertSoln(char *soln_file_name, SOLN_MODEL soln_model,       */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive)                                 */
/*                                                                           */
/*  Convert soln_file_name into a solution of ins and add it to a solution   */
/*  group of archive, which is known to have at least one solution group.    */
/*                                                                           */
/*****************************************************************************/

void INRC1ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  FILE *fp;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;  char *id;  HA_ARENA a;
  NRC_SOLN soln;  KML_ERROR ke; NRC_WORKER w;  int i, j;  NRC_DAY d;
  KML_ELT soln_elt, asst_elt, nurse_elt, day_elt, shift_type_elt;

  /* must have an instance in this model */
  if( ins == NULL )
  {
    fprintf(stderr, "nrconv:  -t not permitted with -minrc1\n");
    exit(1);
  }

  /* open the file and exit if can't */
  fp = fopen(soln_file_name, "r");
  if( fp == NULL )
  {
    fprintf(stderr,
      "nrconv:  ignoring inrc1 solution file \"%s\" (cannot open)\n",
      soln_file_name);
    return;
  }

  /* read XML from the file, and exit if can't */
  a = HaArenaMake(as);
  if( !KmlReadFile(fp, NULL, &soln_elt, &ke, a) )
    KmlFatalError(ke, soln_file_name);
  fclose(fp);

  /* make sure the root tags and their children are all present and correct */
  if( strcmp(KmlLabel(soln_elt), "Solution") != 0 )
    KmlEltFatalError(soln_elt, soln_file_name,
      "root tag %s where Solution expected", KmlLabel(soln_elt));
  if( !KmlCheck(soln_elt, ": $SchedulingPeriodID $Competitor "
	"+#SoftConstraintsPenalty *Assignment", &ke) )
  {
    if( DEBUG2 )
      fprintf(stderr, "Comp2ConvertSoln failing at 1\n");
    KmlFatalError(ke, soln_file_name);
  }

  /* make a soln object and load the assignments */
  soln = NrcSolnMake(ins, as);
  if( KmlContainsChildPos(soln_elt, "Assignment", &i) )
    for( ;  i < KmlChildCount(soln_elt);  i++ )
    {
      asst_elt = KmlChild(soln_elt, i);
      if( !KmlCheck(asst_elt, " : $Date $Employee $ShiftType", &ke) )
	KmlFatalError(ke, soln_file_name);

      /* get the day */
      day_elt = KmlChild(asst_elt, 0);
      if( !NrcInstanceCycleRetrieveDay(ins, KmlText(day_elt), &d) )
	KmlEltFatalError(day_elt, soln_file_name, "unknown Day %s",
	  KmlText(day_elt));

      /* get the nurse */
      nurse_elt = KmlChild(asst_elt, 1);
      id = KmlText(nurse_elt);
      /* id = TidyNurseIdOrName(KmlText(nurse_elt), a); */
      if( !NrcInstanceStaffingRetrieveWorker(ins, id, &w) )
	KmlEltFatalError(nurse_elt, soln_file_name, "unknown Nurse %s",
	  KmlText(nurse_elt));

      /* get the shift type */
      shift_type_elt = KmlChild(asst_elt, 2);
      if( !NrcInstanceRetrieveShiftType(ins, KmlText(shift_type_elt), &st) )
	KmlEltFatalError(shift_type_elt, soln_file_name, "unknown ShiftType %s",
	  KmlText(shift_type_elt));

      /* get the shift, find a suitable demand in it, and assign */
      s = NrcDayShiftFromShiftType(d, st);
      for( j = 0;  j < NrcShiftDemandCount(s);  j++ )
	if( NrcSolnAssignment(soln, s, j) == NULL )
	  break;
      if( j == NrcShiftDemandCount(s) )
	KmlEltFatalError(asst_elt, soln_file_name,
	  "cannot find a free slot to assign in shift %s:%s",
	  NrcDayShortName(d), NrcShiftTypeName(st));
      NrcSolnAddAssignment(soln, s, j, w);
    }

  /* add soln to the appropriate soln group */
  SolnModelAddSoln(soln_model, archive, "", 0.0, soln);
  HaArenaDelete(a);
}

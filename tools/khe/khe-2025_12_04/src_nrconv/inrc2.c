
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
/*  FILE:         inrc2.c                                                    */
/*  MODULE:       ConvertINRC2 (second international nurse rostering comp)   */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0


/*****************************************************************************/
/*                                                                           */
/*  Note on constraints                                                      */
/*  -------------------                                                      */
/*                                                                           */
/*  Here is the relationship between what the competition documentation      */
/*  says about constraints, and the functions of this file:                  */
/*                                                                           */
/*  Competition documentation             Functions in this file       X  Y  */
/*  -----------------------------------------------------------------------  */
/*  H1. Single assignment per day         SingleAssignmentPerDay       -  -  */
/*  H2. Under-staffing                    AddCoverConstraints                */
/*  H3. Shift type successions            UnwantedPattern              -  H  */
/*  H4. Missing required skill            AddCoverConstraints                */
/*  -----------------------------------------------------------------------  */
/*  S1. Insuff staff for opt cover (30)   AddCoverConstraints                */
/*  S2. Consecutive assignments (15/30)   MinConsecutiveSameShiftDays  S  H  */
/*                                        MaxConsecutiveSameShiftDays  S  H  */
/*                                        MinConsecutiveWorkingDays    C  H  */
/*                                        MaxConsecutiveWorkingDays    C  H  */
/*  S3. Consecutive days off (30)         MinConsecutiveDaysOff        C  H  */
/*                                        MaxConsecutiveDaysOff        C  H  */
/*  S4. Preferences (10)                  AddShiftOffRequests          -  -  */
/*  S5. Complete week-end (30)            CompleteWeekends             C  -  */
/*  S6. Total assignments (20)            MinAssignments               C  H  */
/*                                        MaxAssignments               C  H  */
/*  S7. Total working week-ends (30)      MaxWorkingWeekends           C  H  */
/*  -----------------------------------------------------------------------  */
/*                                                                           */
/*  Hard constraints H1-4 have no weights; this code gives them weight 1     */
/*  throughout.  Soft constraints S1-7 have the weights shown.  The limits   */
/*  vary as indicated by the X column:                                       */
/*                                                                           */
/*     -    Trivial limit (1, pattern length - 1, etc.)                      */
/*     R    Limit stored in (so varies with) Requirement in weekly file      */
/*     S    Limit stored in (so varies with) Shift Type in scenario file     */
/*     C    Limit stored in (so varies with) Contract in scenario file       */
/*                                                                           */
/*                                                                           */
/*  The constraints are affected by history as indicated in the Y column:    */
/*                                                                           */
/*     -    Unaffected by history                                            */
/*     H    Affected by history                                              */
/*                                                                           */
/*  Even in the last week, the competition rules assume that there will be   */
/*  more weeks, so minimum limits do not apply at the right boundary.        */
/*                                                                           */
/*****************************************************************************/
#define INFINITY 100

/*****************************************************************************/
/*                                                                           */
/*  INRC2_INFO - miscellaneous information about a inrc2 instance            */
/*                                                                           */
/*****************************************************************************/

typedef struct inrc2_file_rec {
  char				*file_name;
  KML_ELT			root_elt;
} *INRC2_FILE;

typedef HA_ARRAY(INRC2_FILE) ARRAY_INRC2_FILE;

typedef struct inrc2_info_rec {
  HA_ARENA			arena;
  INRC2_FILE			sc_file;
  INRC2_FILE			hy_file;
  ARRAY_INRC2_FILE		wk_files;
  NRC_INSTANCE			ins;
  NRC_DAY_SET_SET		weekends;
  int				weekends_before;
  /* int			weekends_after; */
  int				days_before;
  /* int			days_after; */
} *INRC2_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "file handling"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  INRC2_FILE FileMake(char *file_name, char *type, HA_ARENA a)             */
/*                                                                           */
/*  Make an inrc2 file, including reading it.  Parameter type is the         */
/*  type of file, for error messages.                                        */
/*                                                                           */
/*****************************************************************************/

INRC2_FILE FileMake(char *file_name, char *type, HA_ARENA a)
{
  INRC2_FILE res;  FILE *fp;  KML_ERROR ke;
  HaMake(res, a);
  res->file_name = file_name;
  fp = fopen(file_name, "r");
  if( fp == NULL )
    FatalError(NULL, 0, "cannot open inrc2 %s file \"%s\"", type, file_name);
  if( !KmlReadFile(fp, NULL, &res->root_elt, &ke, a) )
    KmlFatalError(ke, file_name);
  fclose(fp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddDaysAndWeekends(INRC2_INFO ii)                                   */
/*                                                                           */
/*  Add the cycle (it begins on a Monday) and the weekends.                  */
/*                                                                           */
/*****************************************************************************/

static void AddDaysAndWeekends(INRC2_INFO ii)
{
  int i, weeks;  NRC_DAY_SET weekend;  char buff[100];
  weeks = HaArrayCount(ii->wk_files);
  NrcCycleMake(ii->ins, 7 * weeks, 1);
  ii->weekends = NrcDaySetSetMake(ii->ins, "Weekends", "Weekends");
  for( i = 0;  i < weeks;  i++ )
  {
    sprintf(buff, "Weekend%d", i + 1);
    weekend = NrcDaySetMake(ii->ins, buff, buff);
    NrcDaySetAddDay(weekend, NrcInstanceCycleDay(ii->ins, i * 7 + 5));
    NrcDaySetAddDay(weekend, NrcInstanceCycleDay(ii->ins, i * 7 + 6));
    NrcDaySetSetAddDaySet(ii->weekends, weekend);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "skills"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSkills(INRC2_INFO ii)                                            */
/*                                                                           */
/*  Add skills to ii.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddSkills(INRC2_INFO ii)
{
  KML_ELT skills_elt, skill_elt;  int i;  KML_ERROR ke;  /*  SKILL skill; */
  NRC_WORKER_SET skill_ws;  char *name;
  if( DEBUG3 )
    fprintf(stderr, "[ AddSkills(ii)\n");
  if( KmlContainsChild(ii->sc_file->root_elt, "Skills", &skills_elt) )
  {
    if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
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
  if( DEBUG3 )
    fprintf(stderr, "] AddSkills returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift types and shift type constraints"                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSameShiftDaysHistory(NRC_CONSTRAINT c, NRC_SHIFT_TYPE st,        */
/*    INRC2_INFO ii)                                                         */
/*                                                                           */
/*  Add suitable history information to constraint c, which limits the       */
/*  number of consecutive assignments to shift type st.                      */
/*                                                                           */
/*****************************************************************************/

static void AddSameShiftDaysHistory(NRC_CONSTRAINT c, NRC_SHIFT_TYPE st,
  INRC2_INFO ii)
{
  int i, v;  NRC_WORKER_SET ws;  NRC_WORKER w;  NRC_SHIFT_TYPE last_st;
  NrcConstraintAddHistory(c, ii->days_before, INFINITY);
  ws = NrcConstraintWorkerSet(c);
  for( i = 0;  i < NrcWorkerSetWorkerCount(ws);  i++ )
  {
    w = NrcWorkerSetWorker(ws, i);
    if( NrcWorkerRetrieveHistory(w, "LastAssignedShiftType", &v) )
    {
      last_st = NrcInstanceShiftType(NrcConstraintInstance(c), v);
      if( last_st == st &&
	  NrcWorkerRetrieveHistory(w, "NumberOfConsecutiveAssignments", &v)
	  && v > 0 )
	NrcConstraintAddHistoryWorker(c, w, v);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveSameShiftDays(INRC2_INFO ii, KML_ELT contracts_elt)   */
/*                                                                           */
/*  Minimum consecutive same shift days.                                     */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveSameShiftDays(INRC2_INFO ii, NRC_SHIFT_TYPE st,
  int limit)
{
  NRC_CONSTRAINT c;  NRC_WORKER_SET ws;  NRC_PENALTY p;
  if( DEBUG6 )
    fprintf(stderr, "[ MinConsecutiveSameShiftDays(ii, %s, %d)\n",
      NrcShiftTypeName(st), limit);
  ws = NrcInstanceStaffing(ii->ins);
  p = NrcPenalty(false, 15, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "S2: MinConsecutiveSameShiftDays", ws,
    NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(limit, false, p, ii->ins),
    NULL);
  /* ***
  c = NrcConstraintMake(ii->ins, "MinConsecutiveSameShiftDays", ws, p,
    NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, limit), NULL);
  *** */
  NrcConstraintAddShiftSetSet(c, NrcShiftTypeShiftSetSet(st), NRC_POSITIVE);
  AddSameShiftDaysHistory(c, st, ii);
  if( DEBUG6 )
    fprintf(stderr, "] MinConsecutiveSameShiftDays returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveSameShiftDays(INRC2_INFO ii, KML_ELT contracts_elt)   */
/*                                                                           */
/*  Maximum consecutive same shift days.                                     */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveSameShiftDays(INRC2_INFO ii, NRC_SHIFT_TYPE st,
  int limit)
{
  NRC_CONSTRAINT c;  NRC_WORKER_SET ws;  NRC_PENALTY p;
  ws = NrcInstanceStaffing(ii->ins);
  p = NrcPenalty(false, 15, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "S2: MaxConsecutiveSameShiftDays", ws,
    NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
  /* ***
  c = NrcConstraintMake(ii->ins, "MaxConsecutiveSameShiftDays", ws, p,
    NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
  *** */
  NrcConstraintAddShiftSetSet(c, NrcShiftTypeShiftSetSet(st), NRC_POSITIVE);
  AddSameShiftDaysHistory(c, st, ii);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftTypes(INRC2_INFO ii)                                        */
/*                                                                           */
/*  Add shift types to ii.                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypes(INRC2_INFO ii)
{
  KML_ELT shift_types_elt, shift_type_elt, consec_elt, min_elt, max_elt;
  KML_ERROR ke;  int i, min_consec, max_consec;  char *id;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftTypes(ii)\n");
  if( KmlContainsChild(ii->sc_file->root_elt, "ShiftTypes", &shift_types_elt) )
  {
    if( !KmlCheck(shift_types_elt, ": *ShiftType", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( i = 0;  i < KmlChildCount(shift_types_elt);  i++ )
    {
      /* find one shift type and its id */
      shift_type_elt = KmlChild(shift_types_elt, i);
      if( !KmlCheck(shift_type_elt, "Id : NumberOfConsecutiveAssignments",&ke) )
	KmlFatalError(ke, ii->sc_file->file_name);
      id = KmlAttributeValue(shift_type_elt, 0);

      /* find min_consecutive and max_consecutive (not used here) */
      consec_elt = KmlChild(shift_type_elt, 0);
      if( !KmlCheck(consec_elt, ": #Minimum #Maximum", &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);
      min_elt = KmlChild(consec_elt, 0);
      if( sscanf(KmlText(min_elt), "%d", &min_consec) != 1 )
	HnAbort("AddShiftTypes internal error 1");
      max_elt = KmlChild(consec_elt, 1);
      if( sscanf(KmlText(max_elt), "%d", &max_consec) != 1 )
	HnAbort("AddShiftTypes internal error 2");

      /* make the shift type */
      NrcShiftTypeMake(ii->ins, id, NRC_NO_WORKLOAD);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftTypes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftTypeConstraints(INRC2_INFO ii)                              */
/*                                                                           */
/*  Add shift type constraints to ii.                                        */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypeConstraints(INRC2_INFO ii)
{
  KML_ELT shift_types_elt, shift_type_elt, consec_elt, min_elt, max_elt;
  KML_ERROR ke;  int i, min_consec, max_consec;  char *id;  NRC_SHIFT_TYPE st;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftTypeConstraints(ii)\n");
  if( KmlContainsChild(ii->sc_file->root_elt, "ShiftTypes", &shift_types_elt) )
  {
    if( !KmlCheck(shift_types_elt, ": *ShiftType", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( i = 0;  i < KmlChildCount(shift_types_elt);  i++ )
    {
      /* find one shift type and its id */
      shift_type_elt = KmlChild(shift_types_elt, i);
      if( !KmlCheck(shift_type_elt, "Id : NumberOfConsecutiveAssignments",&ke) )
	KmlFatalError(ke, ii->sc_file->file_name);
      id = KmlAttributeValue(shift_type_elt, 0);

      /* find min_consecutive and max_consecutive */
      consec_elt = KmlChild(shift_type_elt, 0);
      if( !KmlCheck(consec_elt, ": #Minimum #Maximum", &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);
      min_elt = KmlChild(consec_elt, 0);
      if( sscanf(KmlText(min_elt), "%d", &min_consec) != 1 )
	HnAbort("AddShiftTypes internal error 1");
      max_elt = KmlChild(consec_elt, 1);
      if( sscanf(KmlText(max_elt), "%d", &max_consec) != 1 )
	HnAbort("AddShiftTypes internal error 2");

      /* make the shift type and its associated constraints */
      if( !NrcInstanceRetrieveShiftType(ii->ins, id, &st) )
	HnAbort("AddShiftTypeConstraints internal error (%s)", id);
      MinConsecutiveSameShiftDays(ii, st, min_consec);
      MaxConsecutiveSameShiftDays(ii, st, max_consec);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftTypeConstraints returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "patterns and unwanted pattern constraints"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void UnwantedPatterns(INRC2_INFO ii)                                     */
/*                                                                           */
/*  Unwanted patterns.  NB the pattern objects are already built.            */
/*                                                                           */
/*****************************************************************************/

static void UnwantedPattern(INRC2_INFO ii, NRC_PATTERN p)
{
  NRC_CONSTRAINT c;  NRC_PENALTY penalty;
  penalty = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "H3: UnwantedPattern",
    NrcInstanceStaffing(ii->ins), NRC_CONSTRAINT_ACTIVE,
    NrcBoundMakeMax(NrcPatternTermCount(p) - 1, penalty, ii->ins),
    NrcInstanceDailyStartingShiftSet(ii->ins));
  NrcConstraintAddPattern(c, p, NrcInstanceCycleDay(ii->ins, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPatternsAndUnwantedPatternConstraints(INRC2_INFO ii)             */
/*                                                                           */
/*  Add patterns (here called forbidden shift type successions) to ii.       */
/*  Also add one unwanted pattern constraint for each pattern.               */
/*                                                                           */
/*****************************************************************************/

static void AddPatternsAndUnwantedPatternConstraints(INRC2_INFO ii)
{
  KML_ELT patterns_elt, pattern_elt, prec_elt, succs_elt, succ_elt;
  KML_ERROR ke;  int i, j;  NRC_SHIFT_TYPE_SET sts;  NRC_PATTERN p;
  NRC_SHIFT_TYPE st;
  if( KmlContainsChild(ii->sc_file->root_elt, "ForbiddenShiftTypeSuccessions",
	&patterns_elt) )
  {
    HnAssert(NrcInstanceCycleDayCount(ii->ins) >= 2,
      "AddPatterns: less than 2 days in cycle");
    if( !KmlCheck(patterns_elt, ": *ShiftTypeSuccession", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( i = 0;  i < KmlChildCount(patterns_elt);  i++ )
    {
      /* get one pattern element and make a pattern for it */
      pattern_elt = KmlChild(patterns_elt, i);
      if( !KmlCheck(pattern_elt, ": $PrecedingShiftType SucceedingShiftTypes",
	    &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);

      /* get the two children */
      prec_elt = KmlChild(pattern_elt, 0);
      succs_elt = KmlChild(pattern_elt, 1);
      if( !KmlCheck(succs_elt, ": *$ShiftType", &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);

      if( KmlChildCount(succs_elt) > 0 )
      {
	/* a pattern is needed, so make one */
	p = NrcPatternMake(ii->ins, NULL);

	/* get preceding element and add a term for that */
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(prec_elt), &st) )
	  KmlEltFatalError(prec_elt, "unknown shift type %s",KmlText(prec_elt));
	sts = NrcShiftTypeSingletonShiftTypeSet(st);
	NrcPatternAddTerm(p, sts, NRC_POSITIVE);

	/* get succeeding element and add a term for that */
	sts = NrcShiftTypeSetMake(ii->ins, NULL);
	for( j = 0;  j < KmlChildCount(succs_elt);  j++ )
	{
	  succ_elt = KmlChild(succs_elt, j);
	  if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(succ_elt), &st) )
	   KmlEltFatalError(succ_elt,"unknown shift type %s",KmlText(succ_elt));
	  NrcShiftTypeSetAddShiftType(sts, st);
	}
	NrcPatternAddTerm(p, sts, NRC_POSITIVE);

	/* add an unwanted pattern constraint for p */
        UnwantedPattern(ii, p);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "contracts and contract constraints"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KmlTextInteger(INRC2_INFO ii, KML_ELT elt, int *val)                */
/*                                                                           */
/*  Make sure that the text of elt is an integer, set *val to it, and        */
/*  return true.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KmlTextInteger(INRC2_INFO ii, KML_ELT elt, int *val)
{
  if( sscanf(KmlText(elt), "%d", val) != 1 )
    KmlEltFatalError(elt, ii->sc_file->file_name, "integer expected here");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasLimitConstraint(INRC2_INFO ii, KML_ELT contract_elt,     */
/*    char *label1, char *label2, int *limit)                                */
/*                                                                           */
/*  Return true and set *limit to the limit if contract_elt has a limit      */
/*  constraint with label label1 and optionally label2.                      */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasLimitConstraint(INRC2_INFO ii, KML_ELT contract_elt,
  char *label1, char *label2, int *limit)
{
  KML_ELT label1_elt, label2_elt;
  if( KmlContainsChild(contract_elt, label1, &label1_elt) )
  {
    /* find label2_elt, which is just label1_elt again if label2 is NULL */
    if( label2 == NULL )
      label2_elt = label1_elt;
    else if( !KmlContainsChild(label1_elt, label2, &label2_elt) )
    {
      *limit = -1;
      return false;
    }

    /* return the limit */
    return KmlTextInteger(ii, label2_elt, limit);
  }
  else
  {
    *limit = -1;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinAssignments(INRC2_INFO ii, KML_ELT contracts_elt)                */
/*                                                                           */
/*  Minimum number of assignments.                                           */
/*                                                                           */
/*  Implementation note.  This code follows MaxAssignments except that it    */
/*  is possible for the constraint to be already satisfied by previous       */
/*  weeks, in which case the constraint is omitted.                          */
/*                                                                           */
/*****************************************************************************/

static void MinAssignments(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "NumberOfAssignments",
       "Minimum", &limit) )
  {
    p = NrcPenalty(false, 20, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S6: MinAssignments", contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMin(limit, false, p, ii->ins), NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfAssignments");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxAssignments(INRC2_INFO ii, KML_ELT contracts_elt)                */
/*                                                                           */
/*  Maximum number of assignments.                                           */
/*                                                                           */
/*****************************************************************************/

static void MaxAssignments(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "NumberOfAssignments",
       "Maximum", &limit) )
  {
    p = NrcPenalty(false, 20, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S6: MaxAssignments", contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfAssignments");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveWorkingDays(INRC2_INFO ii, KML_ELT contract_elt)      */
/*                                                                           */
/*  Minimum consecutive working days.                                        */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveWorkingDays(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "ConsecutiveWorkingDays",
	"Minimum", &limit) )
  {
    p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S2: MinConsecutiveWorkingDays", contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(limit, false, p, ii->ins),
      NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfConsecutiveWorkingDays");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveWorkingDays(INRC2_INFO ii, KML_ELT contracts_elt)     */
/*                                                                           */
/*  Maximum consecutive working days.                                        */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveWorkingDays(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "ConsecutiveWorkingDays",
	"Maximum", &limit) )
  {
    p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S2: MaxConsecutiveWorkingDays", contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfConsecutiveWorkingDays");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveDaysOff(INRC2_INFO ii, KML_ELT contracts_elt)         */
/*                                                                           */
/*  Minimum consecutive days off.                                            */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveDaysOff(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "ConsecutiveDaysOff",
	"Minimum", &limit) )
  {
    p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S3: MinConsecutiveFreeDays", contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(limit, false, p, ii->ins),
      NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_NEGATIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfConsecutiveDaysOff");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveDaysOff(INRC2_INFO ii, KML_ELT contracts_elt)         */
/*                                                                           */
/*  Maximum consecutive days off.                                            */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveDaysOff(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  if( ContractHasLimitConstraint(ii, contract_elt, "ConsecutiveDaysOff",
	"Maximum", &limit) )
  {
    p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S3: MaxConsecutiveDaysOff", contract_ws,
      NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_NEGATIVE);
    NrcConstraintAddHistoryAllWorkers(c, ii->days_before, INFINITY,
      "NumberOfConsecutiveDaysOff");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingWeekends(INRC2_INFO ii, KML_ELT contracts_elt)            */
/*                                                                           */
/*  Maxiumum number of working weekends.                                     */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingWeekends(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit, i;  NRC_CONSTRAINT c;  NRC_PENALTY p;  NRC_DAY_SET weekend;
  if( ContractHasLimitConstraint(ii, contract_elt,
	"MaximumNumberOfWorkingWeekends", NULL, &limit) )
  {
    p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "S7: MaxWorkingWeekends", contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(limit, p, ii->ins), NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(ii->weekends);  i++ )
    {
      weekend = NrcDaySetSetDaySet(ii->weekends, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend), NRC_POSITIVE);
    }
    NrcConstraintAddHistoryAllWorkers(c, ii->weekends_before,
      INFINITY, "NumberOfWorkingWeekends");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CompleteWeekends(INRC2_INFO ii, KML_ELT contracts_elt)              */
/*                                                                           */
/*  Complete weekends.                                                       */
/*                                                                           */
/*****************************************************************************/

static void CompleteWeekends(INRC2_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int limit, i, j;  NRC_CONSTRAINT c;  NRC_DAY d;  NRC_PENALTY p;
  NRC_DAY_SET weekend;
  if( ContractHasLimitConstraint(ii, contract_elt, "CompleteWeekends",
	NULL, &limit) && limit >= 1 )
  {
    for( i = 0;  i < NrcDaySetSetDaySetCount(ii->weekends);  i++ )
    {
      weekend = NrcDaySetSetDaySet(ii->weekends, i);
      limit = NrcDaySetDayCount(weekend);
      p = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
      c = NrcConstraintMake(ii->ins, "S5: CompleteWeekends", contract_ws,
	NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMin(limit, true, p, ii->ins), NULL);
      for( j = 0;  j < limit;  j++ )
      {
	d = NrcDaySetDay(weekend, j);
	NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContracts(INRC2_INFO ii)                                         */
/*                                                                           */
/*  Add contracts.                                                           */
/*                                                                           */
/*****************************************************************************/

static void AddContracts(INRC2_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  KML_ERROR ke;
  int i;  char *id;  NRC_WORKER_SET contract_ws;

  if( KmlContainsChild(ii->sc_file->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      if( !KmlCheck(contract_elt, "Id : NumberOfAssignments "
	  "ConsecutiveWorkingDays ConsecutiveDaysOff "
	  "MaximumNumberOfWorkingWeekends CompleteWeekends", &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);

      /* create worker set for this contract */
      id = HnStringMake(ii->arena, "Contract-%s",
	KmlAttributeValue(contract_elt, 0));
      contract_ws = NrcWorkerSetMake(ii->ins, id);
      NrcInstanceContractsAddContract(ii->ins, contract_ws);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContractConstraints(INRC2_INFO ii)                               */
/*                                                                           */
/*  Add contract constraints.                                                */
/*                                                                           */
/*****************************************************************************/

static void AddContractConstraints(INRC2_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  KML_ERROR ke;
  int i;  char *id;  NRC_WORKER_SET contract_ws;

  if( KmlContainsChild(ii->sc_file->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      if( !KmlCheck(contract_elt, "Id : NumberOfAssignments "
	  "ConsecutiveWorkingDays ConsecutiveDaysOff "
	  "MaximumNumberOfWorkingWeekends CompleteWeekends", &ke) )
	KmlFatalError(ke, ii->sc_file->file_name);

      /* create worker set for this contract */
      id = HnStringMake(ii->arena, "Contract-%s",
	KmlAttributeValue(contract_elt, 0));
      if( !NrcInstanceContractsRetrieveContract(ii->ins, id, &contract_ws) )
	HnAbort("AddContractConstraints internal error");

      /* ConsecutiveWorkingDays (S2) */
      MinConsecutiveWorkingDays(ii, contract_elt, contract_ws);
      MaxConsecutiveWorkingDays(ii, contract_elt, contract_ws);

      /* ConsecutiveDaysOff (S3) */
      MinConsecutiveDaysOff(ii, contract_elt, contract_ws);
      MaxConsecutiveDaysOff(ii, contract_elt, contract_ws);

      /* CompleteWeekends (S5) */
      CompleteWeekends(ii, contract_elt, contract_ws);

      /* NumberOfAssignments (S6) */
      MinAssignments(ii, contract_elt, contract_ws);
      MaxAssignments(ii, contract_elt, contract_ws);

      /* MaximumNumberOfWorkingWeekends (S7) */
      MaxWorkingWeekends(ii, contract_elt, contract_ws);
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
/*  KHE_RESOURCE_GROUP RetrieveContractResourceGroup(INRC2_INFO ii,          */
/*    KML_ELT elt, char *id)                                                 */
/*                                                                           */
/*  Return the resource group for the contract with this id.  The elt        */
/*  parameter is only for error messages.                                    */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET RetrieveContractWorkerSet(INRC2_INFO ii,
  KML_ELT elt, char *id)
{
  NRC_WORKER_SET ws;  char *contract_id;
  contract_id = HnStringMake(ii->arena, "Contract-%s", id);
  if( !NrcInstanceContractsRetrieveContract(ii->ins, contract_id, &ws) )
    KmlEltFatalError(elt, ii->sc_file->file_name, "unknown contract %s", contract_id);
  return ws;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddNurses(INRC2_INFO ii)                                            */
/*                                                                           */
/*  Add one worker for each nurse, including adding them to their skills     */
/*  and contract resource groups.                                            */
/*                                                                           */
/*****************************************************************************/

static void AddNurses(INRC2_INFO ii)
{
  KML_ELT nurses_elt, nurse_elt, skills_elt, skill_elt, contract_elt;
  KML_ERROR ke; int i, j;  char *nurse_id;
  NRC_WORKER w;  NRC_WORKER_SET ws;

  /* record everything in the Nurses child */
  if( DEBUG4 )
    fprintf(stderr, "[ AddNurses(ii)\n");
  KmlContainsChild(ii->sc_file->root_elt, "Nurses", &nurses_elt);
  if( !KmlCheck(nurses_elt, ": *Nurse", &ke) )
    KmlFatalError(ke, ii->sc_file->file_name);
  for( i = 0;  i < KmlChildCount(nurses_elt);  i++ )
  {
    /* sort out names and create one resource for nurse_elt */
    nurse_elt = KmlChild(nurses_elt, i);
    if( !KmlCheck(nurse_elt, "Id : $Contract Skills", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    nurse_id = KmlAttributeValue(nurse_elt, 0);

    w = NrcWorkerMake(ii->ins, nurse_id);

    /* add w to its contract worker set */
    contract_elt = KmlChild(nurse_elt, 0);
    ws = RetrieveContractWorkerSet(ii, contract_elt, KmlText(contract_elt));
    NrcWorkerSetAddWorker(ws, w);

    /* add resource to skills */
    skills_elt = KmlChild(nurse_elt, 1);
    if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
      KmlFatalError(ke, ii->sc_file->file_name);
    for( j = 0;  j < KmlChildCount(skills_elt);  j++ )
    {
      skill_elt = KmlChild(skills_elt, j);
      if( !NrcInstanceSkillsRetrieveSkill(ii->ins, KmlText(skill_elt), &ws) )
	KmlEltFatalError(skill_elt, ii->sc_file->file_name, "unknown skill %s",
	  KmlText(skill_elt));
      NrcWorkerSetAddWorker(ws, w);
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "] AddNurses\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "history"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddNursesHistoryAndInitialUnwantedPatterns(INRC2_INFO ii)           */
/*                                                                           */
/*  Add history information for the resources of the instance.  Also add     */
/*  initial unwanted pattern constraints, the ones that end on the first     */
/*  day of the cycle.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddNursesHistoryAndInitialUnwantedPatterns(INRC2_INFO ii)
{
  KML_ERROR ke;  int i, j;  NRC_WORKER w;  NRC_PATTERN p;  char *val;
  NRC_SHIFT_TYPE_SET sts0, sts1;  NRC_SHIFT_TYPE st;  NRC_POLARITY po;
  KML_ELT histories_elt, history_elt, elt;  NRC_SHIFT_SET ss;  NRC_DAY first_d;
  int total_shifts, total_weekends, consecutive_shifts, consecutive_days;
  int consecutive_days_off;  NRC_CONSTRAINT c;  NRC_PENALTY penalty;

  if( DEBUG5 )
    fprintf(stderr, "[ AddNursesHistoryAndInitialUnwantedPatterns(ii)\n");
  if( !KmlContainsChild(ii->hy_file->root_elt, "NursesHistory", &histories_elt) )
    KmlEltFatalError(ii->hy_file->root_elt, ii->hy_file->file_name,
      "missing NursesHistory element");
  if( !KmlCheck(histories_elt, ": *NurseHistory", &ke) )
    KmlFatalError(ke, ii->hy_file->file_name);
  first_d = NrcInstanceCycleDay(ii->ins, 0);
  for( i = 0;  i < KmlChildCount(histories_elt);  i++ )
  {
    history_elt = KmlChild(histories_elt, i);
    if( !KmlCheck(history_elt, ": $Nurse #NumberOfAssignments "
	  "#NumberOfWorkingWeekends $LastAssignedShiftType "
	  "#NumberOfConsecutiveAssignments #NumberOfConsecutiveWorkingDays "
	  "#NumberOfConsecutiveDaysOff", &ke) )
      KmlFatalError(ke, ii->hy_file->file_name);

    /* resource */
    elt = KmlChild(history_elt, 0);
    val = KmlText(elt);
    if( !NrcInstanceStaffingRetrieveWorker(ii->ins, val, &w) )
      KmlEltFatalError(elt, ii->hy_file->file_name, "unknown nurse %s", val);

    /* number of assignments */
    elt = KmlChild(history_elt, 1);
    sscanf(KmlText(elt), "%d", &total_shifts);
    NrcWorkerAddHistory(w, "NumberOfAssignments", total_shifts);

    /* number of working weekends */
    elt = KmlChild(history_elt, 2);
    sscanf(KmlText(elt), "%d", &total_weekends);
    NrcWorkerAddHistory(w, "NumberOfWorkingWeekends", total_weekends);

    /* last assigned shift type */
    elt = KmlChild(history_elt, 3);
    val = KmlText(elt);
    if( strcmp(val, "None") != 0 )
    {
      if( !NrcInstanceRetrieveShiftType(ii->ins, val, &st) )
        KmlEltFatalError(elt, ii->hy_file->file_name,
	  "unknown shift type %s", val);
      NrcWorkerAddHistory(w, "LastAssignedShiftType", NrcShiftTypeIndex(st));

      /* unwanted pattern constraints ending on the first day of the cycle */
      /* NB every pattern applies to every worker */
      penalty = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
      for( j = 0;  j < NrcInstancePatternCount(ii->ins);  j++ )
      {
	p = NrcInstancePattern(ii->ins, j);
	NrcPatternTerm(p, 0, &sts0, &po);
	NrcPatternTerm(p, 1, &sts1, &po);
	if( NrcShiftTypeSetContainsShiftType(sts0, st) )
	{
	  /* make the second term of p unwanted on the first day */
	  c = NrcConstraintMake(ii->ins, "H3: UnwantedPattern-Init",
	    NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_ACTIVE,
	    NrcBoundMakeMax(0, penalty, ii->ins), NULL);
	  ss = NrcDayShiftSetFromShiftTypeSet(first_d, sts1);
	  NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
	}
      }
    }

    /* number of consecutive assignments */
    elt = KmlChild(history_elt, 4);
    sscanf(KmlText(elt), "%d", &consecutive_shifts);
    NrcWorkerAddHistory(w, "NumberOfConsecutiveAssignments",consecutive_shifts);

    /* consecutive working days */
    elt = KmlChild(history_elt, 5);
    sscanf(KmlText(elt), "%d", &consecutive_days);
    NrcWorkerAddHistory(w, "NumberOfConsecutiveWorkingDays", consecutive_days);

    /* consecutive days off */
    elt = KmlChild(history_elt, 6);
    sscanf(KmlText(elt), "%d", &consecutive_days_off);
    NrcWorkerAddHistory(w, "NumberOfConsecutiveDaysOff", consecutive_days_off);
  }
  if( DEBUG5 )
    fprintf(stderr, "] AddNursesHistoryAndInitialUnwantedPatterns returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cover"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddCoverConstraints(INRC2_INFO ii)                                  */
/*                                                                           */
/*  Add cover constraints.                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddCoverConstraints(INRC2_INFO ii)
{
  KML_ELT reqs_elt, req_elt, day_req_elt, shift_type_elt, skill_elt;
  KML_ERROR ke;  NRC_DAY_SET ds;  NRC_PENALTY p1, p2, p3;  NRC_BOUND b;
  NRC_WORKER_SET ws;  NRC_DEMAND_SET dms;  NRC_DAY day;  INRC2_FILE wk;
  int i, j, /* k, */ min_cover, opt_cover, wknum;  char *id;  NRC_SHIFT_TYPE st;

  if( DEBUG10 )
    fprintf(stderr, "[ AddCoverConstraints(%s)\n", NrcInstanceId(ii->ins));
  HaArrayForEach(ii->wk_files, wk, wknum)
  {
    if( KmlContainsChild(wk->root_elt, "Requirements", &reqs_elt) )
    {
      if( !KmlCheck(reqs_elt, ": *Requirement", &ke) )
	KmlFatalError(ke, wk->file_name);
      for( i = 0;  i < KmlChildCount(reqs_elt);  i++ )
      {
	req_elt = KmlChild(reqs_elt, i);
	if( !KmlCheck(req_elt, ": $ShiftType $Skill RequirementOnMonday "
	  "RequirementOnTuesday RequirementOnWednesday RequirementOnThursday "
	  "RequirementOnFriday RequirementOnSaturday RequirementOnSunday",&ke) )
	  KmlFatalError(ke, wk->file_name);
	shift_type_elt = KmlChild(req_elt, 0);
	if( !NrcInstanceRetrieveShiftType(ii->ins,KmlText(shift_type_elt),&st) )
	  KmlEltFatalError(shift_type_elt, wk->file_name,
	    "unknown shift type %s", KmlText(shift_type_elt));
	skill_elt = KmlChild(req_elt, 1);
	if( !NrcInstanceSkillsRetrieveSkill(ii->ins, KmlText(skill_elt), &ws) )
	  KmlEltFatalError(skill_elt, wk->file_name, "unknown skill id %s",
	    KmlText(skill_elt));
	p1 = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
	p2 = NrcPenalty(false, 30, NRC_COST_FUNCTION_LINEAR, ii->ins);
	p3 = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, ii->ins);
	for( j = 2;  j < KmlChildCount(req_elt);  j++ )
	{
	  /* get min_cover and opt_cover */
	  day_req_elt = KmlChild(req_elt, j);
	  id = KmlLabel(day_req_elt) + strlen("RequirementOn");
	  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, id, &ds) )
	    KmlEltFatalError(day_req_elt, wk->file_name,
	      "unknown day name %s in label %s", id, KmlLabel(day_req_elt));
	  day = NrcDaySetDay(ds, wknum);
	  if( !KmlCheck(day_req_elt, ": #Minimum #Optimal", &ke) )
	    KmlFatalError(ke, wk->file_name);
	  sscanf(KmlText(KmlChild(day_req_elt, 0)), "%d", &min_cover);
	  sscanf(KmlText(KmlChild(day_req_elt, 1)), "%d", &opt_cover);
	  if( min_cover < 0 )
	    KmlEltFatalError(day_req_elt, wk->file_name, "negative Minimum");
	  if( min_cover > opt_cover )
	    KmlEltFatalError(day_req_elt, wk->file_name,
	      "Minimum exceeds Optimal");

	  /* build a suitable demand-set, dms */
	  /* if( opt_cover > 0 ) { */
	  b = NrcBoundMakeMin(min_cover, false, p1, ii->ins);
	  if( !NrcBoundAddPreferred(b, opt_cover, p2, p3) )
	    HnAbort("AddCoverConstraints: inconsistent pref and min bounds");
	  dms = NrcDemandSetMakeFromBound(ii->ins, b, opt_cover*2 + 3, ws, p1);
	  if( DEBUG10 )
	  {
	    fprintf(stderr, "  [ Week %d, Day %s, Type %s, Skill %s:  %s\n",
	      wknum, id, KmlText(shift_type_elt), KmlText(skill_elt),
	      NrcBoundShow(b));
	    NrcDemandSetDebug(dms, 4, stderr);
	    fprintf(stderr, "  ]\n");
	  }


	  /* add dms to each shift */
	  NrcShiftAddDemandSet(NrcDayShiftFromShiftType(day, st), dms);
	  /* ***
	  for( k = 0;  k < NrcDaySetDayCount(ds);  k++ )
	  {
	    d = NrcDaySetDay(ds, k);
	    NrcShiftAddDemandSet(NrcDayShiftFromShiftType(d, st), dms);
	  }
	  *** */
	  /* } */
	}
      }
    }
  }
  if( DEBUG10 )
    fprintf(stderr, "] AddCoverConstraints\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shifts off"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOffRequests(INRC2_INFO ii)                                  */
/*                                                                           */
/*  Add avoid unavailable times constraints to ii.  These handle rule S4.    */
/*  They are grouped by resource, based on day off and shift off requests.   */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOffRequests(INRC2_INFO ii)
{
  KML_ELT shifts_off_elt, shift_off_elt, nurse_elt, day_elt, shift_elt;
  NRC_DAY_SET ds;  KML_ERROR ke;  int i, wknum;  NRC_DAY d;  NRC_WORKER w;
  char *id;  NRC_SHIFT_TYPE st;  NRC_PENALTY p;  INRC2_FILE wk;

  /* accumulate shift off requests */
  HaArrayForEach(ii->wk_files, wk, wknum)
  {
    if( KmlContainsChild(wk->root_elt, "ShiftOffRequests", &shifts_off_elt) )
    {
      if( !KmlCheck(shifts_off_elt, ": *ShiftOffRequest", &ke) )
	KmlFatalError(ke, wk->file_name);
      p = NrcPenalty(false, 10, NRC_COST_FUNCTION_LINEAR, ii->ins);
      for( i = 0;  i < KmlChildCount(shifts_off_elt);  i++ )
      {
	/* get shift_off_elt and convert its attributes into NRC ones */
	shift_off_elt = KmlChild(shifts_off_elt, i);
	if( !KmlCheck(shift_off_elt, ": $Nurse $ShiftType $Day", &ke) )
	  KmlFatalError(ke, wk->file_name);

	/* find the nurse */
	nurse_elt = KmlChild(shift_off_elt, 0);
	id = KmlText(nurse_elt);
	if( !NrcInstanceStaffingRetrieveWorker(ii->ins, id, &w) )
	  KmlEltFatalError(nurse_elt, wk->file_name, "unknown nurse %s", id);

	/* find the day */
	day_elt = KmlChild(shift_off_elt, 2);
	id = KmlText(day_elt);
	if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, id, &ds) )
	  KmlEltFatalError(day_elt, wk->file_name, "unknown day %s", id);
	d = NrcDaySetDay(ds, wknum);

	/* find the shift, or "Any", and record shift off or day off */
	shift_elt = KmlChild(shift_off_elt, 1);
	id = KmlText(shift_elt);
	if( strcmp(id, "Any") == 0 )
	{
	  /* day off */
	  NrcWorkerAddDayOff(w, d, p);
	}
	else if( NrcInstanceRetrieveShiftType(ii->ins, id, &st) )
	{
	  /* shift off */
	  NrcWorkerAddShiftOff(w, NrcDayShiftFromShiftType(d, st), p);
	}
	else
	  KmlEltFatalError(shift_elt, wk->file_name, "unknown shift %s", id);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "single assignment per day constraint"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SingleAssignmentPerDay(INRC2_INFO ii)                               */
/*                                                                           */
/*  Single assignment per day.  This handles competition rule H1.            */
/*                                                                           */
/*****************************************************************************/

static void SingleAssignmentPerDay(INRC2_INFO ii)
{
  NRC_CONSTRAINT c;  NRC_SHIFT_SET starting_ss;  NRC_SHIFT_SET_SET day_sss;
  NRC_PENALTY p;
  starting_ss = NrcInstanceDailyStartingShiftSet(ii->ins);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "H1: SingleAssignmentPerDay",
    NrcInstanceStaffing(ii->ins), NRC_CONSTRAINT_ACTIVE,
    NrcBoundMakeMax(1, p, ii->ins), starting_ss);
  day_sss = NrcDayShiftSetSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSetSet(c, day_sss, NRC_POSITIVE);
  /* ***
  day_ss = NrcDayShiftSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSet(c, day_ss, NRC_POSITIVE);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions (dynamic)"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  INRC2_INFO INRC2InfoMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Make a new, basically empty instance info object.                        */
/*                                                                           */
/*****************************************************************************/

static INRC2_INFO INRC2InfoMake(HA_ARENA a)
{
  INRC2_INFO res;
  HaMake(res, a);
  res->arena = a;
  res->sc_file = NULL;
  res->hy_file = NULL;
  HaArrayInit(res->wk_files, a);
  res->ins = NULL;
  res->weekends = NULL;
  res->days_before = 0;
  /* res->days_after = 0; */
  res->weekends_before = 0;
  /* res->weekends_after = 0; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void INRC2BuildInstance(INRC2_INFO ii, char *ins_id, HA_ARENA_SET as)    */
/*                                                                           */
/*  Build an instance with the given id.                                     */
/*                                                                           */
/*****************************************************************************/

static void INRC2BuildInstance(INRC2_INFO ii, char *ins_id, HA_ARENA_SET as)
{
  ii->ins = NrcInstanceMakeBegin(ins_id, "Nurse", as);
  NrcInstanceSetMetaData(ii->ins, ins_id, NULL, NULL, NULL, NULL, NULL);

  /* add shift types, days, and weekends */
  AddShiftTypes(ii);
  AddDaysAndWeekends(ii);

  /* add nurse stuff */
  AddSkills(ii);
  AddContracts(ii);
  AddNurses(ii);
  AddNursesHistoryAndInitialUnwantedPatterns(ii);

  /* add worker constraints */
  SingleAssignmentPerDay(ii);
  AddShiftTypeConstraints(ii);
  AddPatternsAndUnwantedPatternConstraints(ii);
  AddContractConstraints(ii);

  /* add cover constraints and shift-off requests */
  AddCoverConstraints(ii);
  AddShiftOffRequests(ii);

  /* all done */
  NrcInstanceMakeEnd(ii->ins);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE INRC2ConvertInstance(INSTANCE_MODEL ins_model,              */
/*    char *instance_file_name, HA_ARENA_SET as)                             */
/*                                                                           */
/*  Read the files named by file_name_seq, which is supposed to have format  */
/*                                                                           */
/*    scenario_file_name,history_file_name,week_file_name                    */
/*                                                                           */
/*  naming three files in the format of the second international nurse       */
/*  rostering competition, and return the resulting instance.                */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE INRC2ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  char *comma1, *comma2, *comma3;  INRC2_INFO ii;  int i;
  KML_ERROR ke;  HA_ARENA a;  KML_ELT sc_elt;  INRC2_FILE wk;
  char *sc_id, *hy_id, *wk_id, *str1, *str2, *ins_id;  char buff[21];
  char *sc_file_name, *hy_file_name, *wk_file_name;
  int number_of_weeks, week_index;  NRC_INSTANCE ins;
  if( DEBUG1 )
    fprintf(stderr, "[ INRC2ConvertInstance(%s)\n", instance_file_name);

  /* make an instance info object, initially more or less empty */
  a = HaArenaMake(as);
  ii = INRC2InfoMake(a);

  /* dismantle instance_file_name */
  comma1 = strstr(instance_file_name, ",");
  if( comma1 == NULL )
  {
    fprintf(stderr, "nrconv:  error in command-line argument \"%s\" (%s)\n",
      instance_file_name, "missing first comma");
    exit(1);
  }
  comma2 = strstr(comma1 + 1, ",");
  if( comma2 == NULL )
  {
    fprintf(stderr, "nrconv:  error in command-line argument \"%s\" (%s)\n",
      instance_file_name, "missing second comma");
    exit(1);
  }
  comma3 = strstr(comma2 + 1, ",");
  if( comma3 != NULL )
  {
    fprintf(stderr, "nrconv:  error in command-line argument \"%s\" (%s)\n",
      instance_file_name, "too many commas");
    exit(1);
  }
  *comma1 = *comma2 = '\0';
  sc_file_name = instance_file_name;
  hy_file_name = comma1 + 1;
  wk_file_name =  comma2 + 1;
  if( DEBUG1 )
  {
    fprintf(stderr, "  sc_file_name = %s\n", sc_file_name);
    fprintf(stderr, "  hy_file_name = %s\n", hy_file_name);
    fprintf(stderr, "  wk_file_name = %s\n", wk_file_name);
  }

  /* open the files and exit if can't */
  ii->sc_file = FileMake(sc_file_name, "scenario", a);
  ii->hy_file = FileMake(hy_file_name, "history", a);
  HaArrayAddLast(ii->wk_files, FileMake(wk_file_name, "weekly", a));

  /* make sure the root tags and their children are all present and correct */
  /* scenario */
  if( strcmp(KmlLabel(ii->sc_file->root_elt), "Scenario") != 0 )
    KmlEltFatalError(ii->sc_file->root_elt, ii->sc_file->file_name,
      "root tag %s where Scenario expected", KmlLabel(ii->sc_file->root_elt));
  if( !KmlCheck(ii->sc_file->root_elt,
      "Id +xsi:noNamespaceSchemaLocation +xmlns:xsi +xmlns "
      ": #NumberOfWeeks +Skills ShiftTypes +ForbiddenShiftTypeSuccessions "
      "Contracts Nurses", &ke) )
    KmlFatalError(ke, ii->sc_file->file_name);
  sc_id = KmlAttributeValue(ii->sc_file->root_elt, 0);
  sscanf(KmlText(KmlChild(ii->sc_file->root_elt, 0)), "%d", &number_of_weeks);

  /* history */
  if( strcmp(KmlLabel(ii->hy_file->root_elt), "History") != 0 )
    KmlEltFatalError(ii->hy_file->root_elt, ii->hy_file->file_name,
      "root tag %s where History expected", KmlLabel(ii->hy_file->root_elt));
  if( !KmlCheck(ii->hy_file->root_elt,
      "+xsi:noNamespaceSchemaLocation +xmlns:xsi +xmlns "
      ": #Week $Scenario NursesHistory", &ke) )
    KmlFatalError(ke, ii->hy_file->file_name);
  hy_id = KmlText(KmlChild(ii->hy_file->root_elt, 0));
  sc_elt = KmlChild(ii->hy_file->root_elt, 1);
  if( strcmp(KmlText(sc_elt), sc_id) != 0 )
    KmlEltFatalError(sc_elt, ii->hy_file->file_name,
      "scenario %s in history file does not match Id %s in scenario file",
      KmlText(sc_elt), sc_id);
  sscanf(KmlText(KmlChild(ii->hy_file->root_elt, 0)), "%d", &week_index);
  ii->days_before = (week_index == 0 ? 1 : week_index) * 7;
  /* ii->days_after = (number_of_weeks - week_index - 1) * 7; */
  ii->weekends_before = (week_index == 0 ? 1 : week_index);
  /* ii->weekends_after = (number_of_weeks - week_index - 1); */

  /* week data */
  wk = HaArrayFirst(ii->wk_files);
  if( strcmp(KmlLabel(wk->root_elt), "WeekData") != 0 )
    KmlEltFatalError(wk->root_elt, wk->file_name,
      "root tag %s where WeekData expected", KmlLabel(wk->root_elt));
  if( !KmlCheck(wk->root_elt,
      "+xsi:noNamespaceSchemaLocation +xmlns:xsi +xmlns "
      ": $Scenario +Requirements +ShiftOffRequests", &ke) )
    KmlFatalError(ke, wk->file_name);
  sc_elt = KmlChild(wk->root_elt, 0);
  if( strcmp(KmlText(sc_elt), sc_id) != 0 )
    KmlEltFatalError(sc_elt, wk->file_name,
      "scenario %s in week file does not match Id %s in scenario file",
      KmlText(sc_elt), sc_id);

  /* week ID, scraped from week file name */
  str1 = strrchr(wk->file_name, '-');
  str2 = strrchr(wk->file_name, '.');
  if( str1 != NULL && str2 != NULL && str2 - str1 >= 2 )
  {
    for( i = 1;  i < str2 - str1 && i <= 20;  i++ )
      buff[i-1] = str1[i];
    buff[i-1] = '\0';
    wk_id = HnStringCopy(buff, a);
  }
  else
    wk_id = "?";

  /* build the instance and return it */
  ins_id = HnStringMake(a, "C2-%s-w%s-h%s", sc_id, wk_id, hy_id);
  INRC2BuildInstance(ii, ins_id, as);
  ins = ii->ins;
  HaArenaDelete(a);
  if( DEBUG1 )
    fprintf(stderr, "] INRC2ConvertInstance returning\n");
  return ins;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcDemandPreferredWorkerSet(NRC_DEMAND d)                 */
/*                                                                           */
/*  Return a preferred worker set for dm if there is one.                    */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET NrcDemandPreferredWorkerSet(NRC_DEMAND d)
{
  int i;  NRC_WORKER_SET ws;  NRC_PENALTY p;
  NRC_PENALIZER_TYPE pt;  NRC_PENALTY_TYPE ptype;
  for( i = 0;  i < NrcDemandPenalizerCount(d);  i++ )
  {
    NrcDemandPenalizer(d, i, &pt, &ws, &ptype, &p);
    if( pt == NRC_PENALIZER_NOT_WORKER_SET )
      return ws;
  }
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void INRC2ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,       */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)                */
/*                                                                           */
/*  Convert soln_file_name into a solution of ins and add it to a solution   */
/*  group of archive, which is known to have at least one solution group.    */
/*                                                                           */
/*****************************************************************************/

void INRC2ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  FILE *fp;  KML_ELT soln_elt;  NRC_DEMAND dm;  HA_ARENA a;
  NRC_SOLN soln;  KML_ERROR ke; NRC_WORKER w;  int i, j;  NRC_DAY_SET ds;
  NRC_DAY d;  NRC_SHIFT_TYPE st;  NRC_WORKER_SET skill_ws;  NRC_SHIFT s;
  KML_ELT assts_elt, asst_elt, nurse_elt, day_elt, shift_type_elt, skill_elt;

  /* must have an instance in this model */
  if( ins == NULL )
  {
    fprintf(stderr, "nrconv:  -t not permitted with -minrc2\n");
    exit(1);
  }

  /* open the file and exit if can't */
  fp = fopen(soln_file_name, "r");
  if( fp == NULL )
  {
    fprintf(stderr,
      "nrconv:  ignoring inrc2 solution file \"%s\" (cannot open)\n",
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
  if( !KmlCheck(soln_elt, "+xsi:noNamespaceSchemaLocation +xmlns:xsi +xmlns"
      " : #Week $Scenario Assignments", &ke) )
  {
    if( DEBUG2 )
      fprintf(stderr, "INRC2ConvertSoln failing at 1\n");
    KmlFatalError(ke, soln_file_name);
  }

  /* make a soln object and load the assignments */
  soln = NrcSolnMake(ins, as);
  assts_elt = KmlChild(soln_elt, 2);
  if( !KmlCheck(assts_elt, " : *Assignment", &ke) )
    KmlFatalError(ke, soln_file_name);
  for( i = 0;  i < KmlChildCount(assts_elt);  i++ )
  {
    asst_elt = KmlChild(assts_elt, i);
    if( !KmlCheck(asst_elt, " : $Nurse $Day $ShiftType $Skill", &ke) )
      KmlFatalError(ke, soln_file_name);

    /* get the nurse */
    nurse_elt = KmlChild(asst_elt, 0);
    if( !NrcInstanceStaffingRetrieveWorker(ins, KmlText(nurse_elt), &w) )
      KmlEltFatalError(nurse_elt, soln_file_name, "unknown Nurse %s",
	KmlText(nurse_elt));

    /* get the day */
    day_elt = KmlChild(asst_elt, 1);
    if( !NrcInstanceDaysOfWeekRetrieveDaySetShort(ins, KmlText(day_elt), &ds) )
      KmlEltFatalError(day_elt, soln_file_name, "unknown Day %s",
	KmlText(day_elt));
    if( NrcDaySetDayCount(ds) != 1 )
      KmlEltFatalError(day_elt, soln_file_name, "internal error (Day %s)",
        KmlText(day_elt));
    d = NrcDaySetDay(ds, 0);

    /* get the shift type */
    shift_type_elt = KmlChild(asst_elt, 2);
    if( !NrcInstanceRetrieveShiftType(ins, KmlText(shift_type_elt), &st) )
      KmlEltFatalError(shift_type_elt, soln_file_name, "unknown ShiftType %s",
	KmlText(shift_type_elt));

    /* get the skill */
    skill_elt = KmlChild(asst_elt, 3);
    if( !NrcInstanceSkillsRetrieveSkill(ins, KmlText(skill_elt), &skill_ws) )
      KmlEltFatalError(skill_elt, soln_file_name, "unknown Skill %s",
	KmlText(skill_elt));

    /* get the shift, find a suitable demand in it, and assign */
    s = NrcDayShiftFromShiftType(d, st);
    for( j = 0;  j < NrcShiftDemandCount(s);  j++ )
    {
      dm = NrcShiftDemand(s, j);
      if( NrcDemandPreferredWorkerSet(dm) == skill_ws &&
	  NrcSolnAssignment(soln, s, j) == NULL )
	break;
    }
    if( j == NrcShiftDemandCount(s) )
      KmlEltFatalError(asst_elt, soln_file_name,
	"cannot find a free slot for skill %s in shift %s:%s",
	NrcWorkerSetName(skill_ws), NrcDayShortName(d), NrcShiftTypeName(st));
    NrcSolnAddAssignment(soln, s, j, w);
  }

  /* add soln to the appropriate soln group */
  SolnModelAddSoln(soln_model, archive, "", 0.0, soln);
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions (static)"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool GrabInt(char **p, char *val)                                        */
/*                                                                           */
/*  If **p contains another integer, grab it, put it in **val, and move *p.  */
/*                                                                           */
/*****************************************************************************/
#define is_digit(c) ((c) >= '0' && (c) <= '9')

static bool GrabInt(char **p, char **val, HA_ARENA a)
{
  char buff[13];
  if( **p == '\0' || !sscanf(*p, "%12[0-9]", buff) )
    return *val = "", false;
  *p += strlen(buff);
  while( **p != '\0' && !is_digit(**p) )
    (*p)++;
  return *val = HnStringMake(a, "%s", buff), true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *FileNameSuffix(char *file_name)                                    */
/*                                                                           */
/*  Return a pointer to the suffix of file_name:  the part of it after the   */
/*  last '/', or all of it if there is no '/'.                               */
/*                                                                           */
/*****************************************************************************/

static char *FileNameSuffix(char *file_name)
{
  char *res;  int len;
  len = strlen(file_name);
  res = &file_name[len - 1];
  while( res != file_name && *res != '/' )
    res--;
  if( *res == '/' )  res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE INRC2ConvertInstanceStatic(INSTANCE_MODEL ins_model,        */
/*    char *instance_file_name, HA_ARENA_SET as)                             */
/*                                                                           */
/*  Convert a static INRC2 instance.  Here the instance file name has form   */
/*                                                                           */
/*    <stem>/120-4-1-5-6-9-8                                                 */
/*                                                                           */
/*  for example, where 120 is the number of nurses, 4 is the number of       */
/*  weeks, 1 identifies the history, and the other numbers are weeks.        */
/*  The files actually read lie in directory                                 */
/*                                                                           */
/*    <stem>/n120w4                                                          */
/*                                                                           */
/*  Here is what this directory contains:                                    */
/*                                                                           */
/*    H0-n120w4-0.xml  WD-n120w4-0.xml  WD-n120w4-4.xml  WD-n120w4-8.xml     */
/*    H0-n120w4-1.xml  WD-n120w4-1.xml  WD-n120w4-5.xml  WD-n120w4-9.xml     */
/*    H0-n120w4-2.xml  WD-n120w4-2.xml  WD-n120w4-6.xml                      */
/*    Sc-n120w4.xml    WD-n120w4-3.xml  WD-n120w4-7.xml                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE INRC2ConvertInstanceStatic(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  char *suffix, *p;
  int len, suffix_len, weeks, count;  char save_suffix;
  char *field1, *field2, *field3, *field4, *tag, *ins_id;
  char *dir_name, *sc_file_name, *hy_file_name, *wk_file_name;
  INRC2_INFO ii;  HA_ARENA a;  NRC_INSTANCE ins;

  if( DEBUG7 )
    fprintf(stderr, "[ INRC2ConvertInstanceStatic(model, %s, -)\n",
      instance_file_name);

  /* make an instance info object, initially more or less empty */
  a = HaArenaMake(as);
  ii = INRC2InfoMake(a);

  /* let suffix point to just after the last / in the file name */
  len = strlen(instance_file_name);
  HnAssert(len > 0, "INRC2ConvertInstanceStatic:  empty file name");
  HnAssert(len < 400, "INRC2ConvertInstanceStatic:  file name too long");
  suffix = FileNameSuffix(instance_file_name);

  /* build the tag (n120w4 or whatever) */
  suffix_len = strlen(suffix);
  HnAssert(suffix_len >= 7,
    "INRC2ConvertInstanceStatic: file name suffix \"%s\" is too short", suffix);
  p = suffix;
  GrabInt(&p, &field1, a);
  GrabInt(&p, &field2, a);
  GrabInt(&p, &field3, a);
  tag = HnStringMake(a, "n%sw%s", field1, field2);
  /* sprintf(tag, "n%sw%s", field1, field2); */
  if( DEBUG7 )
    fprintf(stderr, "  tag \"%s\"\n", tag);

  /* build the directory name */
  save_suffix = *suffix;
  *suffix = '\0';
  dir_name = HnStringMake(a, "%s%s", instance_file_name, tag);
  /* sprintf(dir_name, "%s%s", instance_file_name, tag); */
  *suffix = save_suffix;
  if( DEBUG7 )
    fprintf(stderr, "  dir \"%s\"\n", dir_name);

  /* scenario file */
  sc_file_name = HnStringMake(a, "%s/Sc-%s.xml", dir_name, tag);
  /* sprintf(sc_file_name, "%s/Sc-%s.xml", dir_name, tag); */
  ii->sc_file = FileMake(sc_file_name, "scenario", a);
  if( DEBUG7 )
    fprintf(stderr, "  sc_file_name \"%s\"\n", sc_file_name);

  /* history file */
  hy_file_name = HnStringMake(a, "%s/H0-%s-%s.xml", dir_name, tag, field3);
  /* sprintf(hy_file_name, "%s/H0-%s-%s.xml", dir_name, tag, field3); */
  if( DEBUG7 )
    fprintf(stderr, "  hy_file_name \"%s\"\n", hy_file_name);
  ii->hy_file = FileMake(hy_file_name, "history", a);

  /* weekly file names, and also instanced id */
  ins_id = HnStringMake(a, "INRC2-%s-%s-%s-", field2, field1, field3);
  /* sprintf(ins_id, "INRC2-%s-%s-%s-", field2, field1, field3); */
  sscanf(field2, "%d", &weeks);
  count = 0;
  while( GrabInt(&p, &field4, a) )
  {
    count++;
    wk_file_name = HnStringMake(a, "%s/WD-%s-%s.xml", dir_name, tag, field4);
    /* sprintf(wk_file_name, "%s/WD-%s-%s.xml", dir_name, tag, field4); */
    strcpy(&ins_id[strlen(ins_id)], field4);
    if( DEBUG7 )
      fprintf(stderr, "  wk_file_name %d: \"%s\"\n", count, wk_file_name);
    HaArrayAddLast(ii->wk_files, FileMake(wk_file_name, "weekly", a));
  }
  HnAssert(count == weeks,
    "INRC2ConvertInstanceStatic: inconsistent suffix \"%s\"", suffix);

  /* days_before and days_after (week_index is always 0) */
  ii->days_before = 7;
  /* ii->days_after = 0; */
  ii->weekends_before = 1;
  /* ii->weekends_after = 0; */

  /* build the instance and return it */
  INRC2BuildInstance(ii, ins_id, as);
  ins = ii->ins;
  HaArenaDelete(a);
  if( DEBUG7 )
    fprintf(stderr, "] INRC2ConvertInstanceStatic returning\n");
  return ins;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnFileNameToInstanceIdOmer(char *soln_file_name)                  */
/*                                                                           */
/*  Convert soln_file_name (in the Omer format) to an instance name.         */
/*                                                                           */
/*    Example:  solns_omer/n030w4_1_6-2-9-1  -->  INRC2-4-030-1-6291.        */
/*                                                                           */
/*****************************************************************************/

static char *SolnFileNameToInstanceIdOmer(char *soln_file_name, HA_ARENA a)
{
  char *suffix, *p, *field1, *field2, *field3, *res;
  suffix = FileNameSuffix(soln_file_name);
  p = &suffix[1];
  GrabInt(&p, &field1, a);
  GrabInt(&p, &field2, a);
  GrabInt(&p, &field3, a);
  res = HnStringMake(a, "INRC2-%s-%s-%s-", field2, field1, field3);
  /* sprintf(res, "INRC2-%s-%s-%s-", field2, field1, field3); */
  while( GrabInt(&p, &field1, a) )
    res = HnStringMake(a, "%s%s", res, field1);
    /* strcpy(&res[strlen(res)], field1); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnFileNameToInstanceIdSchaerf(char *soln_file_name)               */
/*                                                                           */
/*  Convert soln_file_name (in the Schaerf format) to an instance name.      */
/*                                                                           */
/*    Example:  solns_schaerf/030-4-1-6-2-9-1  -->  INRC2-4-030-1-6291.      */
/*                                                                           */
/*  This is virtually the same as SolnFileNameToInstanceIdOmer, but there    */
/*  is one tiny difference, and anyway the similarity is coincidental.       */
/*                                                                           */
/*****************************************************************************/

static char *SolnFileNameToInstanceIdSchaerf(char *soln_file_name, HA_ARENA a)
{
  char *suffix, *p, *field1, *field2, *field3, *res;
  suffix = FileNameSuffix(soln_file_name);
  p = suffix;
  GrabInt(&p, &field1, a);
  GrabInt(&p, &field2, a);
  GrabInt(&p, &field3, a);
  res = HnStringMake(a, "INRC2-%s-%s-%s-", field2, field1, field3);
  /* sprintf(res, "INRC2-%s-%s-%s-", field2, field1, field3); */
  while( GrabInt(&p, &field1, a) )
    res = HnStringMake(a, "%s%s", res, field1);
    /* strcpy(&res[strlen(res)], field1); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool GetLine(FILE *fp, char *line, int *line_num)                        */
/*                                                                           */
/*  Get the next line of fp int *line, setting *line_num to its line         */
/*  number.  Return false if there was no next line.                         */
/*                                                                           */
/*****************************************************************************/

static bool GetLine(FILE *fp, char *line, int *line_num)
{
  if( fgets(line, 500, fp) == NULL )
    return false;
  (*line_num)++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *InstanceIdToTag(char *ins_id)                                      */
/*                                                                           */
/*  Return the tag corresponding to ins_id, in static memory.                */
/*                                                                           */
/*****************************************************************************/

char *InstanceIdToTag(char *ins_id)
{
  static char res[500];
  sprintf(res, "n%c%c%cw%c", ins_id[8], ins_id[9], ins_id[10], ins_id[6]);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnAddSolnFile(NRC_SOLN soln, int week_num, char *fname,           */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Add the contents of solution file fname (which is supposed to be         */
/*  for week week_num) to soln.                                              */
/*                                                                           */
/*****************************************************************************/

static void SolnAddSolnFile(NRC_SOLN soln, int week_num, char *fname,
  HA_ARENA a)
{
  char line[500], field1[500], field2[500], field3[500], field4[500];
  FILE *fp;  int line_num, num, asst_count, j;  NRC_INSTANCE ins;
  NRC_WORKER w;  NRC_DAY_SET ds;  NRC_SHIFT_TYPE st;  NRC_WORKER_SET skill_ws;
  NRC_SHIFT s;  NRC_DEMAND dm;  NRC_DAY d;

  /* open file and abort if can't */
  ins = NrcSolnInstance(soln);
  fp = fopen(fname, "r");
  if( fp == NULL )
    FatalError(fname, 0, "cannot open file for reading");
  line_num = 0;

  /* read and check line 1: "SOLUTION\n" */
  if( !GetLine(fp, line, &line_num) )
    FatalError(fname, line_num, "cannot read line 1");
  if( strcmp(line, "SOLUTION\n") != 0 )
    FatalError(fname, line_num, "syntax error on line 1");

  /* read and check line 2: "1 n030w4" or whatever */
  if( !GetLine(fp, line, &line_num) )
    FatalError(fname, line_num, "cannot read line 2");
  if( sscanf(line, "%d %s\n", &num, field2) != 2 )
    FatalError(fname, line_num, "syntax error on line 2");
  if( num != week_num )
    FatalError(fname, line_num, "wrong week number %d (expected %d)",
      num, week_num);
  if( strcmp(InstanceIdToTag(NrcInstanceId(ins)), field2) != 0 )
    FatalError(fname, line_num, "tag %s inconsistent with instance id %s",
      field2, NrcInstanceId(ins));

  /* read and check line 3: "\n" */
  if( !GetLine(fp, line, &line_num) )
    FatalError(fname, line_num, "cannot read line 3");
  if( line[0] != '\n' )
    FatalError(fname, line_num, "line 3 should be empty, but it isn't");

  /* read and check line 4: "ASSIGNMENTS = 124" or whatever */
  if( !GetLine(fp, line, &line_num) )
    FatalError(fname, line_num, "cannot read line 4");
  if( sscanf(line, "%s %s %d\n", field1, field2, &asst_count) != 3 ||
      strcmp(field1, "ASSIGNMENTS") != 0 || strcmp(field2, "=") != 0 )
    FatalError(fname, line_num, "syntax error on line 4");

  /* read and check subsequent lines, and add them to soln */
  while( GetLine(fp, line, &line_num) )
  {
    if( sscanf(line, "%s %s %s %s\n", field1, field2, field3, field4) != 4 )
      FatalError(fname, line_num, "line does not contain four fields");

    /* get the nurse */
    if( !NrcInstanceStaffingRetrieveWorker(ins, field1, &w) )
      FatalError(fname, line_num, "unknown Nurse %s", field1);

    /* get the day */
    if( !NrcInstanceDaysOfWeekRetrieveDaySetShort(ins, field2, &ds) )
      FatalError(fname, line_num, "unknown Day %s", field2);
    d = NrcDaySetDay(ds, week_num);

    /* get the shift type */
    if( !NrcInstanceRetrieveShiftType(ins, field3, &st) )
      FatalError(fname, line_num, "unknown ShiftType %s", field3);

    /* get the skill */
    if( !NrcInstanceSkillsRetrieveSkill(ins, field4, &skill_ws) )
      FatalError(fname, line_num, "unknown Skill %s", field4);

    /* get the shift, find a suitable demand in it, and assign */
    s = NrcDayShiftFromShiftType(d, st);
    for( j = 0;  j < NrcShiftDemandCount(s);  j++ )
    {
      dm = NrcShiftDemand(s, j);
      if( NrcDemandPreferredWorkerSet(dm) == skill_ws &&
	  NrcSolnAssignment(soln, s, j) == NULL )
	break;
    }
    if( j == NrcShiftDemandCount(s) )
      FatalError(fname, line_num,
	"cannot find a free slot for skill %s in shift %s:%s",
	NrcWorkerSetName(skill_ws), NrcDayShortName(d), NrcShiftTypeName(st));
    NrcSolnAddAssignment(soln, s, j, w);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void INRC2ConvertSolnOmer(SOLN_MODEL soln_model, char *soln_file_name,   */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)                */
/*                                                                           */
/*  Convert a solution in the form used by Jeremy Omer.                      */
/*                                                                           */
/*****************************************************************************/

void INRC2ConvertSolnOmer(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  char *ins_id, fname[500];  NRC_DAY_SET cycle;  int week_count, i;
  NRC_SOLN soln;  HA_ARENA a;

  /* boilerplate */
  if( DEBUG8 )
    fprintf(stderr, "[ INRC2ConvertSolnOmer(-, %s, %s, -, -)\n",
      soln_file_name, ins == NULL ? "NULL" : NrcInstanceId(ins));
  a = HaArenaMake(as);

  /* get the instance */
  HnAssert(ins == NULL, "INRC2ConvertSolnOmer passed non-NULL instance");
  ins_id = SolnFileNameToInstanceIdOmer(soln_file_name, a);
  if( !NrcArchiveRetrieveInstance(archive, ins_id, &ins) )
    HnAbort("INRC2ConvertSolnOmer: cannot retrieve instance \"%s\"", ins_id);
  if( DEBUG8 )
    fprintf(stderr, "  found instance %s\n", ins_id);

  /* make a solution and add the contents of the solution files to it */
  soln = NrcSolnMake(ins, as);
  cycle = NrcInstanceCycle(ins);
  week_count = NrcDaySetDayCount(cycle) / 7;
  for( i = 0;  i < week_count;  i++ )
  {
    sprintf(fname, "%s/sol-week%d.txt", soln_file_name, i);
    if( DEBUG8 )
      fprintf(stderr, "  examining soln file \"%s\":\n", fname);
    SolnAddSolnFile(soln, i, fname, a);
  }

  /* add soln to the appropriate soln group and return */
  SolnModelAddSoln(soln_model, archive, "", 0.0, soln);
  HaArenaDelete(a);
  if( DEBUG8 )
    fprintf(stderr, "] INRC2ConvertSolnOmer returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void INRC2ConvertSolnSchaerf(SOLN_MODEL soln_model, char *soln_file_name,*/
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)                */
/*                                                                           */
/*  Convert a solution in the form used by Andrea Schaerf.                   */
/*                                                                           */
/*****************************************************************************/

void INRC2ConvertSolnSchaerf(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  char *ins_id, fname[500];  NRC_DAY_SET cycle;  int week_count, i;
  NRC_SOLN soln;  HA_ARENA a;

  /* boilerplate */
  if( DEBUG8 )
    fprintf(stderr, "[ INRC2ConvertSolnSchaerf(-, %s, %s, -, -)\n",
      soln_file_name, ins == NULL ? "NULL" : NrcInstanceId(ins));
  a = HaArenaMake(as);

  /* get the instance */
  HnAssert(ins == NULL, "INRC2ConvertSolnSchaerf passed non-NULL instance");
  ins_id = SolnFileNameToInstanceIdSchaerf(soln_file_name, a);
  if( !NrcArchiveRetrieveInstance(archive, ins_id, &ins) )
    HnAbort("INRC2ConvertSolnSchaerf: cannot retrieve instance \"%s\"", ins_id);
  if( DEBUG8 )
    fprintf(stderr, "  found instance %s\n", ins_id);

  /* make a solution and add the contents of the solution files to it */
  soln = NrcSolnMake(ins, as);
  cycle = NrcInstanceCycle(ins);
  week_count = NrcDaySetDayCount(cycle) / 7;
  for( i = 0;  i < week_count;  i++ )
  {
    sprintf(fname, "%s/sol-%d.out", soln_file_name, i);
    if( DEBUG8 )
      fprintf(stderr, "  examining soln file \"%s\":\n", fname);
    SolnAddSolnFile(soln, i, fname, a);
  }

  /* add soln to the appropriate soln group and return */
  SolnModelAddSoln(soln_model, archive, "", 0.0, soln);
  HaArenaDelete(a);
  if( DEBUG8 )
    fprintf(stderr, "] INRC2ConvertSolnSchaerf returning\n");
}

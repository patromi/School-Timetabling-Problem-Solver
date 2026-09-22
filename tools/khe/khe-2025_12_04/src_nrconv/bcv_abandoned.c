
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
/*  FILE:         bcv_abandoned.c                                            */
/*  MODULE:       BCV instances and solutions (abandoned)                    */
/*                                                                           */
/*  I have abandoned this conversion, because the details around the         */
/*  handling of days off, long weekends, etc., etc., are overwhelming.       */
/*  Except for Partnerships I'm sure it could all be done, but I just        */
/*  don't have the time.                                                     */
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
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  BCV_SHIFT_TYPE - extra information about a BCV shift type                */
/*                                                                           */
/*****************************************************************************/

typedef struct bcv_shift_type_rec {
  char			*start_time;		/* start time                */
  char			*end_time;		/* end time                  */
  bool			is_night;		/* true if night shift       */
  float			hours_worked;		/* hours worked              */
  float			free_time_before;	/* hours worked              */
  float			free_time_after;	/* hours worked              */
  bool			auto_allocate;		/* no idea what this is      */
  NRC_WORKER_SET	skill_ws;		/* skill worker set          */
} *BCV_SHIFT_TYPE;

typedef MARRAY(BCV_SHIFT_TYPE) ARRAY_BCV_SHIFT_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  BCV_HISTORY - history information for one employee                       */
/*                                                                           */
/*****************************************************************************/

typedef MARRAY(NRC_SHIFT_TYPE) ARRAY_NRC_SHIFT_TYPE;

typedef enum {
  BCV_LAST_DAY_WORKING,
  BCV_LAST_DAY_NON_WORKING,
  BCV_LAST_DAY_HOLIDAY_NON_WORKING
} BCV_LAST_DAY_TYPE;

typedef struct bcv_consecutive_shifts_rec {
  NRC_SHIFT_TYPE	shift_type;
  int			count;
} *BCV_CONSECUTIVE_SHIFTS;

typedef MARRAY(BCV_CONSECUTIVE_SHIFTS) ARRAY_BCV_CONSECUTIVE_SHIFTS;

typedef struct bcv_history_rec {
  BCV_LAST_DAY_TYPE		last_day_type;
  ARRAY_NRC_SHIFT_TYPE		last_day_shifts;
  int				PreviousConsecutiveWorkingDays;
  int				PreviousConsecutiveWorkingDaysAndHoliday;
  int				PreviousConsecutiveFreeDays;
  int				PreviousConsecutiveFreeDaysAndHoliday;
  int				PreviousConsecutiveWorkingWeekends;
  int				PreviousWorkingBankHolidays;
  bool				WeekendWorkedThreeWeeksAgo;
  bool				WeekendWorkedTwoWeeksAgo;
  bool				WeekendWorkedOneWeekAgo;
  bool				PreviousSaturdayWorked;
  bool				PreviousSundayWorked;
  bool				PreviousSaturdayRequestedHoliday;
  bool				PreviousSundayRequestedHoliday;
  bool				NightShiftThursday;
  bool				NightShiftFriday;
  bool				PreviousFridayWorked;
  bool				PreviousNightShift;
  int				PreviousFreeDaysAfterNightShift;
  int				PreviousConsecutiveHolidayDaysOff;
  float				PreviousOvertime;
  ARRAY_BCV_CONSECUTIVE_SHIFTS	PreviousConsecutiveShifts;
  /* unused			PreviousConsecutiveShiftGroups; */
  /* unused			DaysSinceShifts; */
} *BCV_HISTORY;

typedef MARRAY(BCV_HISTORY) ARRAY_BCV_HISTORY;


/*****************************************************************************/
/*                                                                           */
/*  BCV_INFO - miscellaneous information about a BCV instance                */
/*                                                                           */
/*****************************************************************************/

typedef MARRAY(NRC_WORKER_SET)	ARRAY_NRC_WORKER_SET;
typedef MARRAY(NRC_DAY_SET)	ARRAY_NRC_DAY_SET;
typedef MTABLE(int)		TABLE_INT;

typedef struct bcv_info_rec {
  char				*file_name;
  KML_ELT			root_elt;
  NRC_INSTANCE			ins;
  ARRAY_BCV_SHIFT_TYPE		shift_types;
  TABLE_INT			master_weights;
  NRC_DAY_SET			bank_holidays;
  ARRAY_BCV_HISTORY		histories;
} *BCV_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing times and time groups"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddStartDateAndEndDate(BCV_INFO ii)                                 */
/*                                                                           */
/*  Add the cycle and the days_of_week.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddStartDateAndEndDate(BCV_INFO ii)
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
/*  void AddShiftTypes(BCV_INFO ii)                                          */
/*                                                                           */
/*  Add shift types to ii.  Also record information about whether each       */
/*  shift type is a night shift type or not, and one skill for each type.    */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypes(BCV_INFO ii)
{
  KML_ELT shift_types_elt, shift_type_elt, skills_elt, skill_elt;
  KML_ELT start_time_elt, end_time_elt, elt;  KML_ERROR ke;
  int i, start_hour, end_hour;  char *id, *val;  bool is_night;
  NRC_WORKER_SET skill_ws;  BCV_SHIFT_TYPE bcv_st;
  float hours_worked, free_time_before, free_time_after;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftTypes(ii)\n");
  if( !KmlContainsChild(ii->root_elt, "ShiftTypes", &shift_types_elt) )
    MAbort("AddShiftTypes internal error");
  if( !KmlCheck(shift_types_elt, " : *Shift", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(shift_types_elt);  i++ )
  {
    /* find one shift type and its id */
    shift_type_elt = KmlChild(shift_types_elt, i);
    if( !KmlCheck(shift_type_elt,
	  "ID : +$Name +$Label +$Colour +$Description $StartTime $EndTime "
	  "+$HoursWorked +$FreeTimeBefore +$FreeTimeAfter +AutoAllocate Skills",
	  &ke) )
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

    /* find hours worked, free time before, and free time after */
    hours_worked = -1.0;
    if( KmlContainsChild(shift_type_elt, "HoursWorked", &elt) )
    {
      if( sscanf(KmlText(elt), "%f", &hours_worked) != 1 )
	KmlEltFatalError(elt, ii->file_name, "syntax error in hours worked");
    }
    free_time_before = -1.0;
    if( KmlContainsChild(shift_type_elt, "FreeTimeBefore", &elt) )
    {
      if( sscanf(KmlText(elt), "%f", &free_time_before) != 1 )
	KmlEltFatalError(elt, ii->file_name,"syntax error in free time before");
    }
    free_time_after = -1.0;
    if( KmlContainsChild(shift_type_elt, "FreeTimeAfter", &elt) )
    {
      if( sscanf(KmlText(elt), "%f", &free_time_after) != 1 )
	KmlEltFatalError(elt, ii->file_name,"syntax error in free time after");
    }

    /* find the skill */
    skill_ws = NULL;
    if( KmlContainsChild(shift_type_elt, "Skills", &skills_elt) )
    {
      if( !KmlCheck(skills_elt, " : Skill", &ke) )
	KmlFatalError(ke, ii->file_name);
      if( KmlChildCount(skills_elt) >= 1 )
      {
	if( KmlChildCount(skills_elt) > 1 )
	  MAbort("AddShiftTypes:  shift type %s has multiple skills", id);
	skill_elt = KmlChild(skills_elt, 0);
	val = KmlText(skill_elt);
	if( !NrcInstanceSkillsRetrieveSkill(ii->ins, val, &skill_ws) )
	  KmlEltFatalError(skill_elt, ii->file_name, "unknown skill %s", val);
      }
    }

    /* make the NRC shift type, and put extra stuff in the BCV shift type */
    NrcShiftTypeMake(ii->ins, id, NRC_NO_WORKLOAD);
    MMake(bcv_st);
    bcv_st->start_time = KmlText(start_time_elt);
    bcv_st->end_time = KmlText(end_time_elt);
    bcv_st->is_night = is_night;
    bcv_st->hours_worked = hours_worked;
    bcv_st->free_time_before = free_time_before;
    bcv_st->free_time_after = free_time_after;
    bcv_st->auto_allocate = false;
    bcv_st->skill_ws = skill_ws;
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftTypes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftGroups(BCV_INFO ii)                                         */
/*                                                                           */
/*  Add shift groups to ii.  These are shift-type sets, in NRC terminology.  */
/*                                                                           */
/*****************************************************************************/

static void AddShiftGroups(BCV_INFO ii)
{
  KML_ELT shift_groups_elt, shift_group_elt, shift_elt;  KML_ERROR ke;
  int i, j;  char *id;  NRC_SHIFT_TYPE_SET sts;  NRC_SHIFT_TYPE st;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftGroups(ii)\n");
  if( KmlContainsChild(ii->root_elt, "ShiftGroups", &shift_groups_elt) )
  {
    if( !KmlCheck(shift_groups_elt, " : *ShiftGroup", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shift_groups_elt);  i++ )
    {
      /* find one shift group and its id */
      shift_group_elt = KmlChild(shift_groups_elt, i);
      if( !KmlCheck(shift_group_elt, "ID : *$Shift", &ke) )
	KmlFatalError(ke, ii->file_name);
      id = KmlAttributeValue(shift_group_elt, 0);
      if( DEBUG9 )
	fprintf(stderr, "  id %s:\n", id);

      /* make the shift group as a shift-type set, and add its shift types */
      sts = NrcShiftTypeSetMake(ii->ins, id);
      for( j = 0;  j < KmlChildCount(shift_group_elt);  j++ )
      {
	shift_elt = KmlChild(shift_group_elt, j);
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(shift_elt), &st) )
	  KmlEltFatalError(shift_elt, ii->file_name, "unknown shift type %s",
	    KmlText(shift_elt));
	NrcShiftTypeSetAddShiftType(sts, st);
      }
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftGroups returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddBankHolidays(BCV_INFO ii)                                        */
/*                                                                           */
/*  Add bank holidarys in ii->bank_holidays.                                 */
/*                                                                           */
/*****************************************************************************/

static void AddBankHolidays(BCV_INFO ii)
{
  KML_ELT bank_holidays_elt, bank_holiday_elt, elt;  NRC_DAY d;
  int i;  KML_ERROR ke;
  if( DEBUG9 )
    fprintf(stderr, "[ AddBankHolidays(ii)\n");
  if( KmlContainsChild(ii->root_elt, "BankHolidays", &bank_holidays_elt) )
  {
    if( !KmlCheck(bank_holidays_elt, ": *BankHoliday", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(bank_holidays_elt);  i++ )
    {
      bank_holiday_elt = KmlChild(bank_holidays_elt, i);
      if( !KmlCheck(bank_holiday_elt, "ID : Name Date", &ke) )
	KmlFatalError(ke, ii->file_name);
      elt = KmlChild(bank_holiday_elt, 1);
      if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
	KmlEltFatalError(elt, ii->file_name,
	  "cannot interpret date %s", KmlText(elt));
      if( ii->bank_holidays == NULL )
	ii->bank_holidays = NrcDaySetMake(ii->ins, "BankHolidays",
	  "BankHolidays");
      NrcDaySetAddDay(ii->bank_holidays, d);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddBankHolidays returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing resources and resource groups"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *TidyNurseIdOrName(char *id)                                        */
/*                                                                           */
/*  Tidy up a nurse Id or name, by copying it into malloced memory, and      */
/*  prepending "Nurse" if it does not start with a letter.                   */
/*                                                                           */
/*****************************************************************************/

static char *TidyNurseIdOrName(char *id)
{
  if( is_letter(id[0]) )
    return MStringCopy(id);
  else
    return MStringMake("Nurse%s", id);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE RetrieveResource(BCV_INFO ii, char *id)                     */
/*                                                                           */
/*  Retrieve from ii->instance the resource with this id.  Note that we      */
/*  are prepending "Nurse" often.                                            */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER RetrieveWorker(BCV_INFO ii, KML_ELT nurse_elt)
{
  char *id;  NRC_WORKER w;
  id = TidyNurseIdOrName(KmlText(nurse_elt));
  if( !NrcInstanceStaffingRetrieveWorker(ii->ins, id, &w) )
    KmlEltFatalError(nurse_elt, ii->file_name, "unknown worker %s", id);
  return w;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET RetrieveContractWorkerSet(BCV_INFO ii,                    */
/*    KML_ELT elt, char *id)                                                 */
/*                                                                           */
/*  Return the contract worker set with this id.  The elt parameter is only  */
/*  for error messages.                                                      */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET RetrieveContractWorkerSet(BCV_INFO ii,
  KML_ELT elt, char *id)
{
  NRC_WORKER_SET ws;  char *contract_id;
  contract_id = MStringMake("Contract-%s", id);
  if( !NrcInstanceContractsRetrieveContract(ii->ins, contract_id, &ws) )
    KmlEltFatalError(elt, ii->file_name, "unknown contract %s", contract_id);
  return ws;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSkills(BCV_INFO ii)                                              */
/*                                                                           */
/*  Add skills to ii.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddSkills(BCV_INFO ii)
{
  KML_ELT skills_elt, skill_elt;  int i;  KML_ERROR ke;
  NRC_WORKER_SET skill_ws;  char *name, *id;
  if( DEBUG9 )
    fprintf(stderr, "[ AddSkills(ii)\n");
  if( KmlContainsChild(ii->root_elt, "Skills", &skills_elt) )
  {
    if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(skills_elt);  i++ )
    {
      skill_elt = KmlChild(skills_elt, i);
      if( !KmlContainsAttribute(skill_elt, "ID", &id) )
	KmlEltFatalError(skill_elt, ii->file_name,
	  "<Skill> has no ID attribute");

      /* primary */
      name = MStringMake("%s-%s", id, "Primary");
      if( NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	KmlEltFatalError(skill_elt, ii->file_name,
	  "skill name %s appears twice", id);
      skill_ws = NrcWorkerSetMake(ii->ins, name);
      NrcInstanceSkillsAddSkill(ii->ins, skill_ws);

      /* secondary */
      name = MStringMake("%s-%s", id, "Secondary");
      if( NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	KmlEltFatalError(skill_elt, ii->file_name,
	  "skill name %s appears twice", id);
      skill_ws = NrcWorkerSetMake(ii->ins, name);
      NrcInstanceSkillsAddSkill(ii->ins, skill_ws);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddSkills returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSkillGroups(BCV_INFO ii)                                         */
/*                                                                           */
/*  Add skill groups to ii.                                                  */
/*                                                                           */
/*  Implementation note.  Skill groups are not implemented, because          */
/*  none of the instances have them.                                         */
/*                                                                           */
/*****************************************************************************/

static void AddSkillGroups(BCV_INFO ii)
{
  KML_ELT skill_groups_elt;
  if( KmlContainsChild(ii->root_elt, "SkillGroups", &skill_groups_elt) )
    KmlEltFatalError(skill_groups_elt, ii->file_name,
      "<SkillGroups> not implemented");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContracts(BCV_INFO ii)                                           */
/*                                                                           */
/*  Add contracts - just the worker sets, not their constraints.             */
/*                                                                           */
/*****************************************************************************/

static void AddContracts(BCV_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  KML_ERROR ke;  int i;
  NRC_WORKER_SET contract_ws;  char *id;

  if( KmlContainsChild(ii->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      if( !KmlCheck(contract_elt, "ID : +Label +MaxShiftsPerDay "
	    "+MaxNumAssignments +MinNumAssignments "
	    "+MaxDaysOff +MinDaysOff "
	    "+MaxConsecutiveWorkingDays +MinConsecutiveWorkingDays "
	    "+MaxWorkingBankHolidays "
	    "+MaxConsecutiveFreeDays +MinConsecutiveFreeDays "
	    "+MaxConsecutiveWorkingWeekends +MinConsecutiveWorkingWeekends "
	    "+MaxConsecutiveFreeWeekends +MinConsecutiveFreeWeekends "
	    "+MaxWeekendsOff +MaxWorkingWeekends"
	    "+MaxWorkingWeekendsInFourWeeks +$MaxWorkingWeekendsIncFriNight "
	    "+MaxShiftsPerWeekStartMon +$MinShiftsPerWeekStartMon "
	    "+MinShiftsPerWeek +$MaxShiftsPerWeek "
	    "+MaxWorkingDaysPerWeek"
	    "+$WeekendDefinition "
	    "+CompleteWeekends +BrokenWeekends "
	    "+IdenticalShiftTypesDuringWeekend "
	    "+NoNightShiftBeforeFreeWeekend +TwoFreeDaysAfterNightShifts "
	    "+AlternativeSkillCategory "
	    "+MaxAssignmentsForDayOfWeek "
	    "+MinConsecutiveShiftTypes +MaxConsecutiveShiftTypes "
	    "+MaxDaysBetweenShiftSeries +MinDaysBetweenShiftSeries "
	    "+ValidNumConsecutiveShiftTypes +ValidNumConsecutiveShiftGroups "
	    "+MaxShiftTypes +MinShiftTypes +MaxShiftTypesPerWeek "
	    "+MinHoursWorkedBetweenDates +MaxHoursWorkedBetweenDates "
	    "+MaxHoursWorked +MinHoursWorked "
	    "+StandardPerformance +MaxHoursPerWeek +MaxHoursPerFortnight "
	    "+ValidShiftTypeSuccessions "
	    "+MinShiftTypeRatios +MaxShiftTypeRatios "
	    "+BadPatterns +GoodPatterns ", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* create worker set for this contract */
      id = MStringMake("Contract-%s", KmlAttributeValue(contract_elt, 0));
      contract_ws = NrcWorkerSetMake(ii->ins, id);
      NrcInstanceContractsAddContract(ii->ins, contract_ws);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddEmployees(BCV_INFO ii)                                           */
/*                                                                           */
/*  Add one worker for each employee, including adding them to their skills  */
/*  and contract worker sets.                                                */
/*                                                                           */
/*  Here's a typical Skill value:  <Skill Type="Primary">HeadNurse</Skill>   */
/*                                                                           */
/*****************************************************************************/

static void AddEmployees(BCV_INFO ii)
{
  KML_ELT employees_elt, employee_elt, skills_elt, skill_elt, elt;
  KML_ELT contract_elt;  KML_ERROR ke; int i, j;  NRC_WORKER w;
  NRC_WORKER_SET ws;  NRC_DAY d;  char *employee_id, *name, *type;

  KmlContainsChild(ii->root_elt, "Employees", &employees_elt);
  if( !KmlCheck(employees_elt, ": *Employee", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(employees_elt);  i++ )
  {
    /* sort out names and create one worker for employee_elt */
    employee_elt = KmlChild(employees_elt, i);
    if( !KmlCheck(employee_elt, "ID : +$ContractID +InRoster +$Name "
	"+$FirstName +$LastName +$EmailAddress +$PhoneNumber "
	"+$EmploymentStartDate +$EmploymentEndDate +Skills", &ke) )
      KmlFatalError(ke, ii->file_name);
    if( !KmlContainsChild(employee_elt, "InRoster", &elt) ||
	strcmp(KmlText(elt), "true") == 0 || strcmp(KmlText(elt), "1") == 0 )
    {
      /* make the worker */
      employee_id = TidyNurseIdOrName(KmlAttributeValue(employee_elt, 0));
      w = NrcWorkerMake(ii->ins, employee_id);

      /* add worker to its contract worker set, if any */
      if( KmlContainsChild(employee_elt, "ContractID", &contract_elt) )
      {
	ws = RetrieveContractWorkerSet(ii, contract_elt, KmlText(contract_elt));
	NrcWorkerSetAddWorker(ws, w);
      }

      /* start date, if any */
      if( KmlContainsChild(employee_elt, "EmploymentStartDate", &elt) )
      {
	if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
	  KmlEltFatalError(elt, ii->file_name, "unknown day %s", KmlText(elt));
	NrcWorkerAddStartDay(w, d, true, 1);
      }

      /* end date, if any */
      if( KmlContainsChild(employee_elt, "EmploymentEndDate", &elt) )
      {
	if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
	  KmlEltFatalError(elt, ii->file_name, "unknown day %s", KmlText(elt));
	NrcWorkerAddEndDay(w, d, true, 1);
      }

      /* add resource to skills (primary and secondary) */
      if( KmlContainsChild(employee_elt, "Skills", &skills_elt) )
      {
	if( !KmlCheck(skills_elt, ": *$Skill", &ke) )
	  KmlFatalError(ke, ii->file_name);
	for( j = 0;  j < KmlChildCount(skills_elt);  j++ )
	{
	  skill_elt = KmlChild(skills_elt, j);
	  if( !KmlCheck(skill_elt, "+Type :", &ke) )
	    KmlFatalError(ke, ii->file_name);
	  if( !KmlContainsAttribute(skill_elt, "Type", &type) )
	    type = "Primary";
	  name = MStringMake("%s-%s", KmlText(skill_elt), type);
	  if( !NrcInstanceSkillsRetrieveSkill(ii->ins, name, &ws) )
	    KmlEltFatalError(skill_elt, ii->file_name, "unknown skill %s",
	      KmlText(skill_elt));
	  NrcWorkerSetAddWorker(ws, w);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int GetOptionalInt(BCV_INFO ii, KML_ELT history_elt, char *label)        */
/*                                                                           */
/*  Get an integer from the child of history_elt with the given label,       */
/*  or -1 if none.                                                           */
/*                                                                           */
/*****************************************************************************/

static int GetOptionalInt(BCV_INFO ii, KML_ELT history_elt, char *label)
{
  KML_ELT elt;  int res;
  if( KmlContainsChild(history_elt, label, &elt) )
  {
    if( sscanf(KmlText(elt), "%d", &res) != 1 )
      KmlEltFatalError(elt, ii->file_name, "expected integer in <%s>", label);
    return res;
  }
  else
    return -1;
}


/*****************************************************************************/
/*                                                                           */
/*  float GetOptionalFloat(BCV_INFO ii, KML_ELT history_elt, char *label)    */
/*                                                                           */
/*  Get a float from the child of history_elt with the given label,          */
/*  or -1.0 if none.                                                         */
/*                                                                           */
/*****************************************************************************/

static float GetOptionalFloat(BCV_INFO ii, KML_ELT history_elt, char *label)
{
  KML_ELT elt;  float res;
  if( KmlContainsChild(history_elt, label, &elt) )
  {
    if( sscanf(KmlText(elt), "%f", &res) != 1 )
      KmlEltFatalError(elt, ii->file_name, "expected float in <%s>", label);
    return res;
  }
  else
    return -1.0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool BCVBool(char *str, bool *res)                                       */
/*                                                                           */
/*  If str can be interpreted as a Boolean value (if its value is "true"     */
/*  or "1" or "false" or "0"), set *res to that value and return true.       */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool BCVBool(char *str, bool *res)
{
  if( strcmp(str, "true") == 0 || strcmp(str, "1") == 0 )
  {
    *res = true;
    return true;
  }
  else if( strcmp(str, "false") == 0 || strcmp(str, "0") == 0 )
  {
    *res = false;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool GetOptionalBool(BCV_INFO ii, KML_ELT history_elt, char *label)      */
/*                                                                           */
/*  Get a boolean from the child of history_elt with the given label,        */
/*  or false if none.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool GetOptionalBool(BCV_INFO ii, KML_ELT history_elt, char *label)
{
  KML_ELT elt;  bool res;
  if( KmlContainsChild(history_elt, label, &elt) )
  {
    if( !BCVBool(KmlText(elt), &res) )
      KmlEltFatalError(elt, ii->file_name, "expected boolean in <%s>", label);
    return res;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSchedulingHistory(BCV_INFO ii)                                   */
/*                                                                           */
/*  Add history info.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddSchedulingHistory(BCV_INFO ii)
{
  KML_ELT histories_elt, history_elt, elt, elt2, elt3;  int i, j, index, count;
  KML_ERROR ke;  char *id;  NRC_WORKER w;  BCV_HISTORY wh;  NRC_SHIFT_TYPE st;
  BCV_CONSECUTIVE_SHIFTS consec_shifts;
  if( KmlContainsChild(ii->root_elt, "SchedulingHistory", &histories_elt) )
  {
    if( !KmlCheck(histories_elt, ": *EmployeeHistory", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(histories_elt);  i++ )
    {
      /* get the history of one employee and check its syntax */
      history_elt = KmlChild(histories_elt, i);
      if( !KmlCheck(history_elt, " EmployeeID : +PreviousAssignments "
	  "+LastDayType +LastDayShifts +#PreviousConsecutiveWorkingDays "
	  "+#PreviousConsecutiveWorkingDaysAndHoliday "
	  "+#PreviousConsecutiveFreeDays "
	  "+#PreviousConsecutiveFreeDaysAndHoliday "
        "+PreviousConsecutiveFreeDaysAndHolidayIncludesAtLeastOneNonWorkingDay "
	  "+#PreviousConsecutiveWorkingWeekends "
	  "+#PreviousWorkingBankHolidays "
	  "+$WeekendWorkedThreeWeeksAgo "
	  "+$WeekendWorkedTwoWeeksAgo "
	  "+$WeekendWorkedOneWeekAgo "
	  "+$PreviousSaturdayWorked "
	  "+$PreviousSundayWorked "
	  "+$PreviousSaturdayRequestedHoliday "
	  "+$PreviousSundayRequestedHoliday "
	  "+$NightShiftThursday "
	  "+$NightShiftFriday "
	  "+$PreviousFridayWorked "
	  "+$PreviousNightShift "
	  "+#PreviousFreeDaysAfterNightShift "
	  "+#PreviousConsecutiveHolidayDaysOff "
	  "+$PreviousOvertime "
	  "+PreviousConsecutiveShifts "
	  "+PreviousConsecutiveShiftGroups "
	  "+DaysSinceShifts ", &ke) )
        KmlFatalError(ke, ii->file_name);

      /* get the employee id and hence the worker */
      id = TidyNurseIdOrName(KmlAttributeValue(history_elt, 0));
      if( !NrcInstanceStaffingRetrieveWorker(ii->ins, id, &w) )
	KmlEltFatalError(history_elt, ii->file_name, "unknown employee %s", id);

      /* make a new, uninitialized history record for w and add it in */
      index = NrcWorkerIndex(w);
      MArrayFill(ii->histories, index + 1, NULL);
      MMake(wh);
      MArrayPut(ii->histories, index, wh);

      /* PreviousAssignments */
      if( KmlContainsChild(history_elt, "PreviousAssignments", &elt) )
	KmlEltFatalError(history_elt, ii->file_name,
	  "<PreviousAssignments> not implemented");

      /* LastDayType */
      if( !KmlContainsChild(history_elt, "LastDayType", &elt) )
	KmlEltFatalError(history_elt, ii->file_name,
	  "expected <EmployeeHistory> to have <LastDayType> child");
      if( strcmp(KmlText(elt), "WorkingDay") == 0 )
	wh->last_day_type = BCV_LAST_DAY_WORKING;
      else if( strcmp(KmlText(elt), "NonWorkingDay") == 0 )
	wh->last_day_type = BCV_LAST_DAY_NON_WORKING;
      else if( strcmp(KmlText(elt), "HolidayNonWorkingDay") == 0 )
	wh->last_day_type = BCV_LAST_DAY_HOLIDAY_NON_WORKING;
      else
        KmlEltFatalError(elt, ii->file_name,
	  "invalid value %s of <LastDayType>", KmlText(elt));

      /* LastDayShifts */
      MArrayInit(wh->last_day_shifts);
      if( KmlContainsChild(history_elt, "LastDayShifts", &elt) )
      {
	if( !KmlCheck(elt, ": *Shift", &ke) )
	  KmlFatalError(ke, ii->file_name);
	for( j = 0;  j < KmlChildCount(elt);  j++ )
	{
	  elt2 = KmlChild(elt, j);
	  if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt2), &st) )
            KmlEltFatalError(elt2, ii->file_name, "unknown shift type %s",
	      KmlText(elt2));
	  MArrayAddLast(wh->last_day_shifts, st);
	}
      }

      /* PreviousConsecutiveWorkingDays and other int-valued fields */
      wh->PreviousConsecutiveWorkingDays = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveWorkingDays");
      wh->PreviousConsecutiveWorkingDaysAndHoliday = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveWorkingDaysAndHoliday");
      wh->PreviousConsecutiveFreeDays = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveFreeDays");
      wh->PreviousConsecutiveFreeDaysAndHoliday = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveFreeDaysAndHoliday");
      wh->PreviousConsecutiveWorkingWeekends = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveWorkingWeekends");
      wh->PreviousWorkingBankHolidays = GetOptionalInt(ii,
	history_elt, "PreviousWorkingBankHolidays");

      /* WeekendWorkedThreeWeeksAgo and other bool-valued fields */
      wh->WeekendWorkedThreeWeeksAgo = GetOptionalBool(ii,
	history_elt, "WeekendWorkedThreeWeeksAgo");
      wh->WeekendWorkedTwoWeeksAgo = GetOptionalBool(ii,
	history_elt, "WeekendWorkedTwoWeeksAgo");
      wh->WeekendWorkedOneWeekAgo = GetOptionalBool(ii,
	history_elt, "WeekendWorkedOneWeekAgo");
      wh->PreviousSaturdayWorked = GetOptionalBool(ii,
	history_elt, "PreviousSaturdayWorked");
      wh->PreviousSundayWorked = GetOptionalBool(ii,
	history_elt, "PreviousSundayWorked");
      wh->PreviousSaturdayRequestedHoliday = GetOptionalBool(ii,
	history_elt, "PreviousSaturdayRequestedHoliday");
      wh->PreviousSundayRequestedHoliday = GetOptionalBool(ii,
	history_elt, "PreviousSundayRequestedHoliday");
      wh->NightShiftThursday = GetOptionalBool(ii,
	history_elt, "NightShiftThursday");
      wh->NightShiftFriday = GetOptionalBool(ii,
	history_elt, "NightShiftFriday");
      wh->PreviousFridayWorked = GetOptionalBool(ii,
	history_elt, "PreviousFridayWorked");
      wh->PreviousNightShift = GetOptionalBool(ii,
	history_elt, "PreviousNightShift");

      /* PreviousFreeDaysAfterNightShift and other int-valued fields */
      wh->PreviousFreeDaysAfterNightShift = GetOptionalInt(ii,
	history_elt, "PreviousFreeDaysAfterNightShift");
      wh->PreviousConsecutiveHolidayDaysOff = GetOptionalInt(ii,
	history_elt, "PreviousConsecutiveHolidayDaysOff");

      /* PreviousOvertime */
      wh->PreviousOvertime = GetOptionalFloat(ii,
	history_elt, "PreviousOvertime");

      /* PreviousConsecutiveShifts */
      MArrayInit(wh->PreviousConsecutiveShifts);
      if( KmlContainsChild(history_elt, "PreviousConsecutiveShifts", &elt) )
      {
	if( !KmlCheck(elt, ": *PreviousConsecutiveShift", &ke) )
	  KmlFatalError(ke, ii->file_name);
	for( j = 0;  j < KmlChildCount(elt);  j++ )
	{
	  elt2 = KmlChild(elt, j);
	  if( !KmlCheck(elt2, ": ShiftTypeID Count", &ke) )
	    KmlFatalError(ke, ii->file_name);

	  /* get shift type */
	  elt3 = KmlChild(elt2, 0);
	  if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt3), &st) )
            KmlEltFatalError(elt3, ii->file_name, "unknown shift type %s",
	      KmlText(elt3));

	  /* get count */
	  elt3 = KmlChild(elt2, 1);
	  if( sscanf(KmlText(elt3), "%d", &count) != 1 )
            KmlEltFatalError(elt3,ii->file_name,"expected integer in <Count>");

	  /* add to wh->PreviousConsecutiveShifts */
          MMake(consec_shifts);
          consec_shifts->shift_type = st;
          consec_shifts->count = count;
	  MArrayAddLast(wh->PreviousConsecutiveShifts, consec_shifts);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing events and cover constraints"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddMasterWeights(BCV_INFO ii)                                       */
/*                                                                           */
/*  Add master weights to ii->master_weights.                                */
/*                                                                           */
/*  There is no checking of the children's labels or order here;  they       */
/*  are simply dumped into the ii->master_weights symbol table.              */
/*                                                                           */
/*****************************************************************************/

static void AddMasterWeights(BCV_INFO ii)
{
  KML_ELT master_weights_elt, master_weight_elt;
  int i, val, val2, pos;  char *id;
  if( DEBUG9 )
    fprintf(stderr, "[ AddMasterWeights(ii)\n");
  if( KmlContainsChild(ii->root_elt, "MasterWeights", &master_weights_elt) )
    for( i = 0;  i < KmlChildCount(master_weights_elt);  i++ )
    {
      master_weight_elt = KmlChild(master_weights_elt, i);
      id = KmlLabel(master_weight_elt);
      if( sscanf(KmlText(master_weight_elt), "%d", &val) != 1 )
	KmlEltFatalError(master_weight_elt, ii->file_name,
	  "body %s is not an integer", KmlText(master_weight_elt));
      if( MTableRetrieve(ii->master_weights, id, &val2, &pos) )
	KmlEltFatalError(master_weight_elt, ii->file_name,
	  "<%s> appears twice in <MasterWeights>", id);
      MTableInsert(ii->master_weights, id, val);
    }
  if( DEBUG9 )
    fprintf(stderr, "] AddMasterWeights returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddCoverRequirements(BCV_INFO ii)                                   */
/*                                                                           */
/*  Add covers requirements.                                                 */
/*                                                                           */
/*****************************************************************************/

/* auditing this is still to do; it probably needs some changes */
static void AddCoverRequirements(BCV_INFO ii)
{
  KML_ELT cover_req_elt, day_of_week_cover_elt, day_elt, shift_elt, cover_elt;
  KML_ERROR ke;  NRC_SHIFT_TYPE st;  int i, j, k, cover;  char *shift_id;
  NRC_DAY d;  NRC_DAY_SET ds;  NRC_DEMAND dm;  NRC_DEMAND_SET dms;
  BCV_SHIFT_TYPE bcv_st;

  if( KmlContainsChild(ii->root_elt, "CoverRequirements", &cover_req_elt) )
  {
    if( !KmlCheck(cover_req_elt, ": *DayOfWeekCover *DateSpecificCover", &ke) )
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
	  bcv_st = MArrayGet(ii->shift_types, NrcShiftTypeIndex(st));
	  dm = NrcDemandMake(ii->ins, true, 1, false, 0, bcv_st->skill_ws,
	    false, 0);
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

static NRC_DAY_SET DayWeekend(NRC_INSTANCE ins, int i, NRC_DAY_SET_SET gdays)
{
  NRC_DAY_SET res;  NRC_DAY d;  int j;  char *short_name, *long_name;

  /* build short and long names */
  d = NrcInstanceCycleDay(ins, i);
  short_name = NrcDayShortName(d);
  long_name = NrcDayLongName(d);
  for( j = 1;  j < NrcDaySetSetDaySetCount(gdays);  j++ )
  {
    d = NrcInstanceCycleDay(ins, i + j);
    short_name = MStringMake("%s%s", short_name, NrcDayShortName(d));
    long_name = MStringMake("%s%s", long_name, NrcDayLongName(d));
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
/*  NRC_DAY_SET_SET BuildWeekends(BCV_INFO ii, KML_ELT contract_elt)         */
/*                                                                           */
/*  Build a day-set set whose day-sets are the weekends for contract_elt.    */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY_SET_SET BuildWeekends(BCV_INFO ii, KML_ELT contract_elt)
{
  KML_ELT weekend_elt;  NRC_DAY_SET_SET res, gdays;  NRC_DAY_SET dow;
  char *str, *s, *p, ch;  int i, len;

  /* make res */
  if( !KmlContainsChild(contract_elt, "WeekendDefinition", &weekend_elt) )
    MAssert(false, "BuildWeekends: missing WeekendDefinition");
  str = KmlText(weekend_elt);
  res = NrcDaySetSetMake(ii->ins, str, str);

  /* get the day-sets corresponding to the days */
  gdays = NrcDaySetSetMake(ii->ins, "tmp", "tmp");
  s = MStringCopy(str);
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
      NrcDaySetSetAddDaySet(res, DayWeekend(ii->ins, i, gdays));
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
/*  Submodule "add elements producing resource constraints (contractual)     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasBooleanConstraint(BCV_INFO ii, KML_ELT contract_elt,     */
/*    char *label, int *weight)                                              */
/*                                                                           */
/*  If contract_elt contains a constraint with the given label (it will      */
/*  have type WeightOnly) which is on and has positive weight, set *weight   */
/*  to its weight and return true, otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasBooleanConstraint(BCV_INFO ii, KML_ELT contract_elt,
  char *label, int *weight)
{
  KML_ELT elt;  KML_ERROR ke;  bool on;  char *val;  int pos;

  /* find the child and check its syntax */
  if( !KmlContainsChild(contract_elt, label, &elt) )
    return false;
  if( !KmlCheck(elt, "+#weight :", &ke) )
    KmlFatalError(ke, ii->file_name);

  /* find the weight attribute, either here or in MasterWeights */
  *weight = 0;  /* keep compiler happy */
  if( KmlContainsAttribute(elt, "weight", &val) )
  {
    if( sscanf(val, "%d", weight) != 1 )
      KmlEltFatalError(elt, ii->file_name,
	"integer expected in weight attribute");
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"non-negative integer expected in weight attribute");
  }
  else
  {
    if( !MTableRetrieve(ii->master_weights, label, weight, &pos) )
      KmlEltFatalError(elt, ii->file_name,
	"no value for <%s> in <MasterWeights>", label);
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"negative value for <%s> in <MasterWeights>", label);
  }

  /* find whether the constraint is on */
  if( !BCVBool(KmlText(elt), &on) )
    KmlEltFatalError(elt, ii->file_name, "Boolean expected here");
  return *weight > 0 && on;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasLimitConstraint(BCV_INFO ii, KML_ELT contract_elt,       */
/*    char *label, int *weight, int *limit)                                  */
/*                                                                           */
/*  If contract_elt contains a constraint with the given label (it will      */
/*  have type OnAndWeight) which is on and has positive weight, set *weight  */
/*  and *limit to its attributes and return true, otherwise return false.    */
/*                                                                           */
/*  The weight may have to be retrieved from MasterWeights.                  */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasLimitConstraint(BCV_INFO ii, KML_ELT contract_elt,
  char *label, int *weight, int *limit)
{
  KML_ELT elt;  KML_ERROR ke;  int pos;  bool on;  char *val;

  /* find the child and check its syntax */
  if( !KmlContainsChild(contract_elt, label, &elt) )
    return false;
  if( !KmlCheck(elt, "+#on +#weight :", &ke) )
    KmlFatalError(ke, ii->file_name);

  /* find the on attribute; we assume that not present means true */
  if( KmlContainsAttribute(elt, "on", &val) )
  {
    if( !BCVBool(val, &on) )
      KmlEltFatalError(elt, ii->file_name, "Boolean expected in on attribute");
    if( !on )
      return false;
  }

  /* find the weight attribute, either here or in MasterWeights */
  *weight = 0;  /* keep compiler happy */
  if( KmlContainsAttribute(elt, "weight", &val) )
  {
    if( sscanf(val, "%d", weight) != 1 )
      KmlEltFatalError(elt, ii->file_name,
	"integer expected in weight attribute");
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"non-negative integer expected in weight attribute");
  }
  else
  {
    if( !MTableRetrieve(ii->master_weights, label, weight, &pos) )
      KmlEltFatalError(elt, ii->file_name,
	"no value for <%s> in <MasterWeights>", label);
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"negative value for <%s> in <MasterWeights>", label);
  }

  /* find the limit */
  if( sscanf(KmlText(elt), "%d", limit) != 1 )
    KmlEltFatalError(elt, ii->file_name, "integer expected here");
  if( *limit < 0 )
    KmlEltFatalError(elt, ii->file_name, "non-negative integer expected here");
  return *weight > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ContractHasFloatLimitConstraint(BCV_INFO ii, KML_ELT contract_elt,  */
/*    char *label, int *weight, float *limit)                                */
/*                                                                           */
/*  Like ConstraintHasLimitConstraint, except that the limit is a float.     */
/*                                                                           */
/*****************************************************************************/

static bool ContractHasFloatLimitConstraint(BCV_INFO ii, KML_ELT contract_elt,
  char *label, int *weight, float *limit)
{
  KML_ELT elt;  KML_ERROR ke;  int pos;  bool on;  char *val;

  /* find the child and check its syntax */
  if( !KmlContainsChild(contract_elt, label, &elt) )
    return false;
  if( !KmlCheck(elt, "+#on +#weight :", &ke) )
    KmlFatalError(ke, ii->file_name);

  /* find the on attribute; we assume that not present means true */
  if( KmlContainsAttribute(elt, "on", &val) )
  {
    if( !BCVBool(val, &on) )
      KmlEltFatalError(elt, ii->file_name, "Boolean expected in on attribute");
    if( !on )
      return false;
  }

  /* find the weight attribute, either here or in MasterWeights */
  *weight = 0;  /* keep compiler happy */
  if( KmlContainsAttribute(elt, "weight", &val) )
  {
    if( sscanf(val, "%d", weight) != 1 )
      KmlEltFatalError(elt, ii->file_name,
	"integer expected in weight attribute");
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"non-negative integer expected in weight attribute");
  }
  else
  {
    if( !MTableRetrieve(ii->master_weights, label, weight, &pos) )
      KmlEltFatalError(elt, ii->file_name,
	"no value for <%s> in <MasterWeights>", label);
    if( *weight < 0 )
      KmlEltFatalError(elt, ii->file_name,
	"negative value for <%s> in <MasterWeights>", label);
  }

  /* find the limit */
  if( sscanf(KmlText(elt), "%f", limit) != 1 )
    KmlEltFatalError(elt, ii->file_name, "float expected here");
  if( *limit < 0 )
    KmlEltFatalError(elt, ii->file_name, "non-negative float expected here");
  return *weight > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftsPerDay(BCV_INFO ii, KML_ELT contracts_elt)                 */
/*                                                                           */
/*  Generate max shifts per day constraints.                                 */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftsPerDay(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_SHIFT_SET starting_ss;  NRC_SHIFT_SET_SET day_sss;
  int weight, limit;  bool reqd;  char *label;

  /* get the limit and weights, assume hard constraint of 1 if absent */
  label = "MaxShiftsPerDay";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
    reqd = false;
  else
    limit = 1, reqd = true, weight = 1;

  /* add the constraint for the first day, repeated for every day */
  starting_ss = NrcInstanceDailyStartingShiftSet(ii->ins);
  c = NrcConstraintMake(ii->ins, label, contract_ws,
    reqd, weight, NRC_LIMIT_MAX, limit, false, starting_ss);
  day_sss = NrcDayShiftSetSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSetSet(c, day_sss, NRC_POSITIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxNumAssignments(BCV_INFO ii, KML_ELT contracts_elt)               */
/*                                                                           */
/*  Handle MaxNumAssignments constraints.                                    */
/*                                                                           */
/*****************************************************************************/

static void MaxNumAssignments(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MaxNumAssignments";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, false, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinNumAssignments(BCV_INFO ii, KML_ELT contracts_elt)               */
/*                                                                           */
/*  Handle MinNumAssignments constraints.                                    */
/*                                                                           */
/*****************************************************************************/

static void MinNumAssignments(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MinNumAssignments";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, false, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceShiftsShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxDaysOff(BCV_INFO ii, KML_ELT contract_elt,                       */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MaxDaysOff constraints.                                           */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MaxDaysOff(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  KML_ELT elt;  char *label;
  label = "MaxDaysOff";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinDaysOff(BCV_INFO ii, KML_ELT contract_elt,                       */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MinDaysOff constraints.                                           */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MinDaysOff(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  KML_ELT elt;  char *label;
  label = "MinDaysOff";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveDays(BCV_INFO ii, KML_ELT contracts_elt,              */
/*    bool working)                                                          */
/*                                                                           */
/*  Handle one MaxConsecutiveWorkingDays or MaxConsecutiveFreeDays element.  */
/*                                                                           */
/*****************************************************************************/

/* may call on this from below
static void MaxConsecutiveDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, bool working)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = working ? "MaxConsecutiveWorkingDays" : "MaxConsecutiveFreeDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      working ? NRC_POSITIVE : NRC_NEGATIVE);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveWorkingDays(BCV_INFO ii, KML_ELT contract_elt,        */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MaxConsecutiveWorkingDays constraints.                            */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveWorkingDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MaxConsecutiveWorkingDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveWorkingDays(BCV_INFO ii, KML_ELT contract_elt,        */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MinConsecutiveWorkingDays constraints.                            */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveWorkingDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MinConsecutiveWorkingDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingBankHolidays(BCV_INFO ii, KML_ELT contract_elt,           */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MaxConsecutiveWorkingDays constraints.                            */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingBankHolidays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MaxWorkingBankHolidays";
  if( ii->bank_holidays != NULL &&
      ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, false, NULL);
    NrcConstraintAddShiftSetSet(c, NrcDaySetShiftSetSet(ii->bank_holidays),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveDays(BCV_INFO ii, KML_ELT contracts_elt,              */
/*    bool working)                                                          */
/*                                                                           */
/*  Handle MinConsecutiveWorkingDays and MinConsecutiveFreeDays.             */
/*                                                                           */
/*****************************************************************************/

/* *** no doubt this will get used somewhere nearby
static void MinConsecutiveDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, bool working)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = working ? "MinConsecutiveWorkingDays" : "MinConsecutiveFreeDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      working ? NRC_POSITIVE : NRC_NEGATIVE);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveFreeDays(BCV_INFO ii, KML_ELT contract_elt,           */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MaxConsecutiveFreeDays constraints.                               */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveFreeDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MaxConsecutiveFreeDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_NEGATIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveFreeDays(BCV_INFO ii, KML_ELT contract_elt,           */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle MinConsecutiveFreeDays constraints.                               */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveFreeDays(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int weight, limit;  NRC_CONSTRAINT c;  char *label;
  label = "MinConsecutiveFreeDays";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_NEGATIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxConsecutiveWorkingWeekends constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = "MaxConsecutiveWorkingWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxConsecutiveWorkingWeekends constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = "MinConsecutiveWorkingWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveWeekends(BCV_INFO ii, KML_ELT contract_elt,           */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss, bool working)*/
/*                                                                           */
/*  Handle the MinConsecutiveWorkingWeekends constraints.                    */
/*                                                                           */
/*  There is in fact no MinConsecutiveFreeWeekends constraint, but it is     */
/*  easier to pretend there is, since this code follows MinConsecutiveDays.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void MinConsecutiveWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss, bool working)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = working?"MinConsecutiveWorkingWeekends":"MinConsecutiveFreeWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds),
	working? NRC_POSITIVE : NRC_NEGATIVE);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveFreeWeekends(BCV_INFO ii, KML_ELT contract_elt,       */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle MaxConsecutiveFreeWeekends constraints.                           */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveFreeWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = "MaxConsecutiveFreeWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_NEGATIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveFreeWeekends(BCV_INFO ii, KML_ELT contract_elt,       */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle MinConsecutiveFreeWeekends constraints.                           */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveFreeWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = "MinConsecutiveFreeWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MIN, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_NEGATIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWeekendsOff(BCV_INFO ii, KML_ELT contract_elt,                   */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle MaxWeekendsOff constraints.                                       */
/*                                                                           */
/*  Implementation note.  Although this constraint could be implemented      */
/*  very easily (it is just MaxConsecutiveFreeWeekends without Conecutive),  */
/*  it has not been, because the instances do not use it.                    */
/*                                                                           */
/*****************************************************************************/

static void MaxWeekendsOff(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MaxWeekendsOff";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,               */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle MaxWorkingWeekends constraints.                                   */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, limit, weight;  NRC_DAY_SET weekend_ds;
  NRC_CONSTRAINT c;
  label = "MaxWorkingWeekends";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws,
      false, weight, NRC_LIMIT_MAX, limit, true, NULL);
    for( i = 0;  i < NrcDaySetSetDaySetCount(weekends_dss);  i++ )
    {
      weekend_ds = NrcDaySetSetDaySet(weekends_dss, i);
      NrcConstraintAddShiftSet(c, NrcDaySetShiftSet(weekend_ds), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingWeekendsInFourWeeks(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxWorkingWeekendsInFourWeeks constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingWeekendsInFourWeeks(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int i, four, weight, limit;  NRC_CONSTRAINT c;
  NRC_DAY_SET weekend_ds;

  label = "MaxWorkingWeekendsInFourWeeks";
  four = 4;
  if( NrcDaySetSetDaySetCount(weekends_dss) >= four &&
      ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    /* add the constraint */
    c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
      NRC_LIMIT_MAX, limit, false, NrcInstanceWeeklyStartingShiftSet(ii->ins));

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
/*  void MaxWorkingWeekendsIncFriNight(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the MaxWorkingWeekendsIncFriNight constraints.                    */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingWeekendsIncFriNight(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MaxWorkingWeekendsIncFriNight";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftsPerWeekStartMon(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxShiftsPerWeekStartMon constraints.                         */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftsPerWeekStartMon(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MaxShiftsPerWeekStartMon";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinShiftsPerWeekStartMon(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinShiftsPerWeekStartMon constraints.                         */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MinShiftsPerWeekStartMon(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MinShiftsPerWeekStartMon";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinShiftsPerWeek(BCV_INFO ii, KML_ELT contract_elt,                 */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinShiftsPerWeek constraints.                                 */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MinShiftsPerWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MinShiftsPerWeek";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftsPerWeek(BCV_INFO ii, KML_ELT contract_elt,                 */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxShiftsPerWeek constraints.                                 */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftsPerWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MaxShiftsPerWeek";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxWorkingDaysPerWeek(BCV_INFO ii, KML_ELT contract_elt,            */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxWorkingDaysPerWeek constraints.                            */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void MaxWorkingDaysPerWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  int limit, weight;  KML_ELT elt;
  label = "MaxWorkingDaysPerWeek";
  if( ContractHasLimitConstraint(ii, contract_elt, label, &weight, &limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CompleteWeekends(BCV_INFO ii, KML_ELT contract_elt,                 */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the CompleteWeekends constraints.                                 */
/*                                                                           */
/*****************************************************************************/

static void CompleteWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  int i, weight;  char *label;  NRC_DAY d;
  NRC_DAY_SET weekend_ds;  NRC_CONSTRAINT c;

  label = "CompleteWeekends";
  if( NrcDaySetSetDaySetCount(weekends_dss) >= 1 &&
      ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
    if( NrcDaySetDayCount(weekend_ds) > 1 )
    {
      /* make constraint */
      c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	NRC_LIMIT_MIN_OR_ZERO, NrcDaySetDayCount(weekend_ds), false,
	NrcInstanceWeeklyStartingShiftSet(ii->ins));

      /* one shift-set for each day on the first weekend */
      for( i = 0;  i < NrcDaySetDayCount(weekend_ds);  i++ )
      {
	d = NrcDaySetDay(weekend_ds, i);
	NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void BrokenWeekends(BCV_INFO ii, KML_ELT contract_elt,                   */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the BrokenWeekends constraints.                                   */
/*                                                                           */
/*  Implementation note.  Although this constraint could presumably be       */
/*  implemented easily if I knew what it meant, it has not been, because     */
/*  the instances do not use it.                                             */
/*                                                                           */
/*****************************************************************************/

static void BrokenWeekends(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  char *label;  int weight;  KML_ELT elt;
  label = "BrokenWeekends";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void IdenticalShiftTypesDuringWeekend(BCV_INFO ii,                       */
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
/*****************************************************************************/

static void IdenticalShiftTypesDuringWeekend(BCV_INFO ii,
  KML_ELT contract_elt, NRC_WORKER_SET contract_ws,
  NRC_DAY_SET_SET weekends_dss)
{
  int i, j, weight;  char *label;  NRC_DAY d;  NRC_SHIFT_SET ss;
  NRC_DAY_SET weekend_ds;  NRC_CONSTRAINT c;  NRC_SHIFT_TYPE st;

  label = "IdenticalShiftTypesDuringWeekend";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) )
  {
    weekend_ds = NrcDaySetSetDaySet(weekends_dss, 0);
    if( NrcDaySetDayCount(weekend_ds) > 1 )
    {
      for( i = 0;  i < NrcInstanceShiftTypeCount(ii->ins);  i++ )
      {
	st = NrcInstanceShiftType(ii->ins, i);

	/* one constraint for each shift type */
	c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	  NRC_LIMIT_MIN_OR_ZERO, NrcDaySetDayCount(weekend_ds), false,
	  NrcInstanceWeeklyStartingShiftSet(ii->ins));

	/* one shift-set for each day of the weekend */
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


/*****************************************************************************/
/*                                                                           */
/*  bool CycleHasWeekendWithPrecedingDay(NRC_INSTANCE ins,                   */
/*    NRC_DAY_SET_SET weekends_dss, NRC_DAY_SET *weekend_ds, NRC_DAY *prec_d)*/
/*                                                                           */
/*  Given that weekends_dss contains the weekends of ins, return true if     */
/*  the cycle contains at least one weekend with a preceding day, and set    */
/*  *weekend_ds and *prec_day to the first such weekend and day.  Else       */
/*  return false.                                                            */
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
    MAssert(NrcDaySetDayCount(ds) > 0,
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
/*  bool InstanceHasNightShifts(BCV_INFO ii)                                 */
/*                                                                           */
/*  Return true if there is at least one night shift, else false.            */
/*                                                                           */
/*****************************************************************************/

static bool InstanceHasNightShifts(BCV_INFO ii)
{
  int i;  BCV_SHIFT_TYPE bcv_st;
  MArrayForEach(ii->shift_types, &bcv_st, &i)
    if( bcv_st->is_night )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool InstanceHasNonNightShifts(BCV_INFO ii)                              */
/*                                                                           */
/*  Return true if there is at least one non-night shift, else false.        */
/*                                                                           */
/*****************************************************************************/

static bool InstanceHasNonNightShifts(BCV_INFO ii)
{
  int i;  BCV_SHIFT_TYPE bcv_st;
  MArrayForEach(ii->shift_types, &bcv_st, &i)
    if( !bcv_st->is_night )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NightShiftsOnDay(BCV_INFO ii, NRC_DAY d)                   */
/*                                                                           */
/*  Return the set of night shifts on day d.                                 */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET NightShiftsOnDay(BCV_INFO ii, NRC_DAY d)
{
  NRC_SHIFT_SET res;  bool is_night;  int i;  NRC_SHIFT_TYPE st;
  res = NrcShiftSetMake(ii->ins);
  MArrayForEach(ii->shift_types, &is_night, &i)
    if( is_night )
    {
      st = NrcInstanceShiftType(ii->ins, i);
      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NonNightShiftsOnDay(BCV_INFO ii, NRC_DAY d)                */
/*                                                                           */
/*  Return the set of non-night shifts on day d.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET NonNightShiftsOnDay(BCV_INFO ii, NRC_DAY d)
{
  NRC_SHIFT_SET res;  int i;  NRC_SHIFT_TYPE st;  BCV_SHIFT_TYPE bcv_st;
  res = NrcShiftSetMake(ii->ins);
  MArrayForEach(ii->shift_types, &bcv_st, &i)
    if( !bcv_st->is_night )
    {
      st = NrcInstanceShiftType(ii->ins, i);
      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NoNightShiftBeforeFreeWeekend(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)              */
/*                                                                           */
/*  Handle the NoNightShiftBeforeFreeWeekend constraints.                    */
/*                                                                           */
/*  This is just pattern Fri:[3][0][0], where Fri means the day (if any)     */
/*  before the first day of WeekendDefinition, and 3 means any night shift.  */
/*                                                                           */
/*****************************************************************************/

static void NoNightShiftBeforeFreeWeekend(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws, NRC_DAY_SET_SET weekends_dss)
{
  int i, weight;  char *label;  NRC_DAY d;
  NRC_DAY_SET weekend_ds;  NRC_CONSTRAINT c;

  label = "NoNightShiftBeforeFreeWeekend";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) &&
      InstanceHasNightShifts(ii) &&
      CycleHasWeekendWithPrecedingDay(ii->ins, weekends_dss, &weekend_ds, &d) )
  {
    c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	NRC_LIMIT_MAX, NrcDaySetDayCount(weekend_ds), false,
	NrcInstanceWeeklyStartingShiftSet(ii->ins));

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
/*  void TwoFreeDaysAfterNightShifts(BCV_INFO ii, KML_ELT contract_elt,      */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the TwoFreeDaysAfterNightShifts constraints.                      */
/*                                                                           */
/*  Implementation note.  As explained in detail in the paper, because       */
/*  two busy days are not considered worse than one busy day, this is        */
/*  equivalent to unwanted patterns [3][12] and [3][0][12].                  */
/*                                                                           */
/*****************************************************************************/

static void TwoFreeDaysAfterNightShifts(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* audit this (and compare with ANRON.pdf) is still to do */
  NRC_CONSTRAINT c;  NRC_DAY d;  char *label;  int weight;
  label = "TwoFreeDaysAfterNightShifts";
  if( ContractHasBooleanConstraint(ii, contract_elt, label, &weight) &&
      InstanceHasNightShifts(ii) && InstanceHasNonNightShifts(ii) )
  {
    /* [3][12] */
    if( NrcInstanceCycleDayCount(ii->ins) >= 2 )
    {
      c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	NRC_LIMIT_MAX, 1, false, NrcInstanceDailyStartingShiftSet(ii->ins));
      d = NrcInstanceCycleDay(ii->ins, 0);
      NrcConstraintAddShiftSet(c, NightShiftsOnDay(ii, d), NRC_POSITIVE);
      d = NrcInstanceCycleDay(ii->ins, 1);
      NrcConstraintAddShiftSet(c, NonNightShiftsOnDay(ii, d), NRC_POSITIVE);
    }

    /* [3][0][12] */
    if( NrcInstanceCycleDayCount(ii->ins) >= 3 )
    {
      c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	NRC_LIMIT_MAX, 2, false, NrcInstanceDailyStartingShiftSet(ii->ins));
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
/*  void AddAlternativeSkillCategories(BCV_INFO ii)                          */
/*                                                                           */
/*  Add information about alternative skill categories.  This has had to     */
/*  be delayed until after AddEmployees.                                     */
/*                                                                           */
/*****************************************************************************/

/* *** from INRC1
static void AddAlternativeSkillCategories(BCV_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  NRC_WORKER w;
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
      for( j = 0;  j < NrcWorkerSetWorkerCount(ws);  j++ )
      {
	w = NrcWorkerSetWorker(ws, j);
	NrcWorkerAddSkillPenalty(w, false, wt);
	if( DEBUG10 )
	  fprintf(stderr, "  NrcWorkerAddSkillPenalty(%s, S%d)\n",
	    NrcWorkerName(w), wt);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void AlternativeSkillCategory(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the alternative skill category constraint.                        */
/*                                                                           */
/*****************************************************************************/

static void AlternativeSkillCategory(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do, based on the previous old function presumably */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxAssignmentsForDayOfWeek(BCV_INFO ii, KML_ELT contract_elt,       */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxAssignmentsForDayOfWeek constraints.                       */
/*                                                                           */
/*****************************************************************************/

static void MaxAssignmentsForDayOfWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
  /* this apparently means that the total number of Wednesdays worked, */
  /* or whatever, is limited. */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinConsecutiveShiftTypes constraints.                         */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxConsecutiveShiftTypes constraints.                         */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinDaysBetweenShiftSeries(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinDaysBetweenShiftSeries constraints.                         */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MinDaysBetweenShiftSeries(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "MinDaysBetweenShiftSeries";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxDaysBetweenShiftSeries(BCV_INFO ii, KML_ELT contract_elt,         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxDaysBetweenShiftSeries constraints.                         */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MaxDaysBetweenShiftSeries(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "MaxDaysBetweenShiftSeries";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void ValidNumConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,    */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the ValidNumConsecutiveShiftTypes constraints.                    */
/*                                                                           */
/*****************************************************************************/

static void ValidNumConsecutiveShiftTypes(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void ValidNumConsecutiveShiftGroups(BCV_INFO ii, KML_ELT contract_elt,   */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the ValidNumConsecutiveShiftGroups constraints.                   */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void ValidNumConsecutiveShiftGroups(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "ValidNumConsecutiveShiftGroups";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftTypes(BCV_INFO ii, KML_ELT contract_elt,                    */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxShiftTypes constraints.                                    */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftTypes(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinShiftTypes(BCV_INFO ii, KML_ELT contract_elt,                    */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinShiftTypes constraints.                                    */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MinShiftTypes(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "MinShiftTypes";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftTypesPerWeek(BCV_INFO ii, KML_ELT contract_elt,             */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxShiftTypesPerWeek constraints.                             */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftTypesPerWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinHoursWorkedBetweenDates(BCV_INFO ii, KML_ELT contract_elt,       */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinHoursWorkedBetweenDates constraints.                       */
/*                                                                           */
/*****************************************************************************/

static void MinHoursWorkedBetweenDates(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxHoursWorkedBetweenDates(BCV_INFO ii, KML_ELT contract_elt,       */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxHoursWorkedBetweenDates constraints.                       */
/*                                                                           */
/*****************************************************************************/

static void MaxHoursWorkedBetweenDates(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxHoursWorked(BCV_INFO ii, KML_ELT contract_elt,                   */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxHoursWorked constraints.                                   */
/*                                                                           */
/*****************************************************************************/

static void MaxHoursWorked(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinHoursWorked(BCV_INFO ii, KML_ELT contract_elt,                   */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinHoursWorked constraints.                                   */
/*                                                                           */
/*****************************************************************************/

static void MinHoursWorked(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void StandardPerformance(BCV_INFO ii, KML_ELT contract_elt,              */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the StandardPerformance constraints.                              */
/*                                                                           */
/*****************************************************************************/

static void StandardPerformance(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxHoursPerWeek(BCV_INFO ii, KML_ELT contract_elt,                  */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxHoursPerWeek constraints.                                  */
/*                                                                           */
/*  Implementation note.  This constraint occurs in the instance files,      */
/*  but always with on="0".  So I've chosen not to implement it in full.     */
/*  If it was implemented in full, it would require a generalization of      */
/*  the XESTT limit workload constraint to allow it to limit workload        */
/*  over a subset of the cycle.                                              */
/*                                                                           */
/*****************************************************************************/

static void MaxHoursPerWeek(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  float limit;  int weight;  KML_ELT elt;
  label = "MaxHoursPerWeek";
  if( ContractHasFloatLimitConstraint(ii, contract_elt, label,&weight,&limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxHoursPerFortnight(BCV_INFO ii, KML_ELT contract_elt,             */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxHoursPerFortnight constraints.                             */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void MaxHoursPerFortnight(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  float limit;  int weight;  KML_ELT elt;
  label = "MaxHoursPerFortnight";
  if( ContractHasFloatLimitConstraint(ii, contract_elt, label,&weight,&limit) )
  {
    KmlContainsChild(contract_elt, label, &elt);
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ValidShiftTypeSuccessions(BCV_INFO ii, KML_ELT contract_elt,        */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the ValidShiftTypeSuccessions constraints.                        */
/*                                                                           */
/*****************************************************************************/

static void ValidShiftTypeSuccessions(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MinShiftTypeRatios(BCV_INFO ii, KML_ELT contract_elt,               */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MinShiftTypeRatios constraints.                               */
/*                                                                           */
/*****************************************************************************/

static void MinShiftTypeRatios(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftTypeRatios(BCV_INFO ii, KML_ELT contract_elt,               */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the MaxShiftTypeRatios constraints.                               */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftTypeRatios(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void BadPatterns(BCV_INFO ii, KML_ELT contract_elt,                      */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the BadPatterns constraints.                                      */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void BadPatterns(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "BadPatterns";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void GoodPatterns(BCV_INFO ii, KML_ELT contract_elt,                     */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Handle the GoodPatterns constraints.                                     */
/*                                                                           */
/*  Implementation note.  At present I know of no instances that use         */
/*  this constraint, so I've chosen not to implement it.                     */
/*                                                                           */
/*****************************************************************************/

static void GoodPatterns(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;  KML_ELT elt;
  label = "GoodPatterns";
  if( KmlContainsChild(contract_elt, label, &elt) )
    KmlEltFatalError(elt, ii->file_name, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void UnwantedPatterns(BCV_INFO ii, KML_ELT contracts_elt)                */
/*                                                                           */
/*  Handle the UnwantedPatterns constraints.                                 */
/*                                                                           */
/*  These do not have the usual format (weights, limits, etc.).              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by BadPatterns and GoodPatterns
static void UnwantedPatterns(BCV_INFO ii, KML_ELT contract_elt,
  NRC_WORKER_SET contract_ws)
{
  int i, p_index, weight;  NRC_PATTERN p;  char *label;  KML_ERROR ke;
  NRC_DAY_SET starting_day_set;  KML_ELT patterns_elt, pattern_elt;
  NRC_CONSTRAINT c;
  label = "UnwantedPatterns";
  if( KmlContainsChild(contract_elt, "UnwantedPatterns", &patterns_elt) )
  {
    if( !KmlCheck(patterns_elt, ": *$Pattern", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(patterns_elt);  i++ )
    {
      pattern_elt = KmlChild(patterns_elt, i);

      ** find the pattern and its associated weight and starting day-set **
      if( !NrcInstanceRetrievePattern(ii->ins, KmlText(pattern_elt), &p) )
	KmlEltFatalError(pattern_elt, ii->file_name, "unknown pattern %s",
	  KmlText(pattern_elt));
      p_index = NrcPatternIndexInInstance(p);
      weight = MArrayGet(ii->pattern_weight, p_index);
      starting_day_set = MArrayGet(ii->pattern_starting_day_set, p_index);

      ** add a constraint for this pattern, holding the pattern **
      c = NrcConstraintMake(ii->ins, label, contract_ws, false, weight,
	NRC_LIMIT_MAX, NrcPatternTermCount(p) - 1, false,
	NrcDaySetStartingShiftSet(starting_day_set));
      NrcConstraintAddPattern(c, p, NrcDaySetDay(starting_day_set, 0));
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void AddContractConstraints(BCV_INFO ii)                                 */
/*                                                                           */
/*  Add the contract constraints.                                            */
/*                                                                           */
/*  Implementation note.  At this point the workers have been added.         */
/*                                                                           */
/*****************************************************************************/

static void AddContractConstraints(BCV_INFO ii)
{
  KML_ELT contracts_elt, contract_elt;  KML_ERROR ke;  int i;
  NRC_DAY_SET_SET weekends_dss;  NRC_WORKER_SET contract_ws;  char *id;

  if( KmlContainsChild(ii->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      if( !KmlCheck(contract_elt, "ID : +Label +MaxShiftsPerDay "
	    "+MaxNumAssignments +MinNumAssignments "
	    "+MaxDaysOff +MinDaysOff "
	    "+MaxConsecutiveWorkingDays +MinConsecutiveWorkingDays "
	    "+MaxWorkingBankHolidays "
	    "+MaxConsecutiveFreeDays +MinConsecutiveFreeDays "
	    "+MaxConsecutiveWorkingWeekends +MinConsecutiveWorkingWeekends "
	    "+MaxConsecutiveFreeWeekends +MinConsecutiveFreeWeekends "
	    "+MaxWeekendsOff +MaxWorkingWeekends"
	    "+MaxWorkingWeekendsInFourWeeks +$MaxWorkingWeekendsIncFriNight "
	    "+MaxShiftsPerWeekStartMon +$MinShiftsPerWeekStartMon "
	    "+MinShiftsPerWeek +$MaxShiftsPerWeek "
	    "+MaxWorkingDaysPerWeek"
	    "+$WeekendDefinition "
	    "+CompleteWeekends +BrokenWeekends "
	    "+IdenticalShiftTypesDuringWeekend "
	    "+NoNightShiftBeforeFreeWeekend +TwoFreeDaysAfterNightShifts "
	    "+AlternativeSkillCategory "
	    "+MaxAssignmentsForDayOfWeek "
	    "+MinConsecutiveShiftTypes +MaxConsecutiveShiftTypes "
	    "+MaxDaysBetweenShiftSeries +MinDaysBetweenShiftSeries "
	    "+ValidNumConsecutiveShiftTypes +ValidNumConsecutiveShiftGroups "
	    "+MaxShiftTypes +MinShiftTypes +MaxShiftTypesPerWeek "
	    "+MinHoursWorkedBetweenDates +MaxHoursWorkedBetweenDates "
	    "+MaxHoursWorked +MinHoursWorked "
	    "+StandardPerformance +MaxHoursPerWeek +MaxHoursPerFortnight "
	    "+ValidShiftTypeSuccessions "
	    "+MinShiftTypeRatios +MaxShiftTypeRatios "
	    "+BadPatterns +GoodPatterns ", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* retrieve worker set and create weekends for this contract */
      id = MStringMake("Contract-%s", KmlAttributeValue(contract_elt, 0));
      if( !NrcInstanceRetrieveWorkerSet(ii->ins, id, &contract_ws) )
	MAbort("AddContractConstraints internal error");
      weekends_dss = BuildWeekends(ii, contract_elt);

      /* generate constraints for the contract */
      MaxShiftsPerDay(ii, contract_elt, contract_ws);
      MaxNumAssignments(ii, contract_elt, contract_ws);
      MinNumAssignments(ii, contract_elt, contract_ws);
      MaxDaysOff(ii, contract_elt, contract_ws);
      MinDaysOff(ii, contract_elt, contract_ws);
      MaxConsecutiveWorkingDays(ii, contract_elt, contract_ws);
      MinConsecutiveWorkingDays(ii, contract_elt, contract_ws);
      MaxWorkingBankHolidays(ii, contract_elt, contract_ws);
      MaxConsecutiveFreeDays(ii, contract_elt, contract_ws);
      MinConsecutiveFreeDays(ii, contract_elt, contract_ws);
      MaxConsecutiveWorkingWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MinConsecutiveWorkingWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MaxConsecutiveFreeWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MinConsecutiveFreeWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MaxWeekendsOff(ii, contract_elt, contract_ws,weekends_dss);
      MaxWorkingWeekends(ii, contract_elt, contract_ws,weekends_dss);
      MaxWorkingWeekendsInFourWeeks(ii, contract_elt, contract_ws,weekends_dss);
      MaxWorkingWeekendsIncFriNight(ii, contract_elt, contract_ws,weekends_dss);
      MaxShiftsPerWeekStartMon(ii, contract_elt, contract_ws);
      MinShiftsPerWeekStartMon(ii, contract_elt, contract_ws);
      MinShiftsPerWeek(ii, contract_elt, contract_ws);
      MaxShiftsPerWeek(ii, contract_elt, contract_ws);
      MaxWorkingDaysPerWeek(ii, contract_elt, contract_ws);
      CompleteWeekends(ii, contract_elt, contract_ws, weekends_dss);
      BrokenWeekends(ii, contract_elt, contract_ws, weekends_dss);
      IdenticalShiftTypesDuringWeekend(ii, contract_elt, contract_ws,
	weekends_dss);
      NoNightShiftBeforeFreeWeekend(ii, contract_elt, contract_ws,weekends_dss);
      TwoFreeDaysAfterNightShifts(ii, contract_elt, contract_ws);
      AlternativeSkillCategory(ii, contract_elt, contract_ws);
      MaxAssignmentsForDayOfWeek(ii, contract_elt, contract_ws);
      MinConsecutiveShiftTypes(ii, contract_elt, contract_ws);
      MaxConsecutiveShiftTypes(ii, contract_elt, contract_ws);
      MinDaysBetweenShiftSeries(ii, contract_elt, contract_ws);
      MaxDaysBetweenShiftSeries(ii, contract_elt, contract_ws);
      ValidNumConsecutiveShiftTypes(ii, contract_elt, contract_ws);
      ValidNumConsecutiveShiftGroups(ii, contract_elt, contract_ws);
      MaxShiftTypes(ii, contract_elt, contract_ws);
      MinShiftTypes(ii, contract_elt, contract_ws);
      MaxShiftTypesPerWeek(ii, contract_elt, contract_ws);
      MinHoursWorkedBetweenDates(ii, contract_elt, contract_ws);
      MaxHoursWorkedBetweenDates(ii, contract_elt, contract_ws);
      MaxHoursWorked(ii, contract_elt, contract_ws);
      MinHoursWorked(ii, contract_elt, contract_ws);
      StandardPerformance(ii, contract_elt, contract_ws);
      MaxHoursPerWeek(ii, contract_elt, contract_ws);
      MaxHoursPerFortnight(ii, contract_elt, contract_ws);
      ValidShiftTypeSuccessions(ii, contract_elt, contract_ws);
      MinShiftTypeRatios(ii, contract_elt, contract_ws);
      MaxShiftTypeRatios(ii, contract_elt, contract_ws);
      BadPatterns(ii, contract_elt, contract_ws);
      GoodPatterns(ii, contract_elt, contract_ws);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing resource constraints (individual)      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddDayOffRequests(BCV_INFO ii)                                      */
/*                                                                           */
/*  Add day off requests.                                                    */
/*                                                                           */
/*  Implementation note.  The "working" attribute does not occur in any      */
/*  instances, and the code below reflects this.  The "holiday" attribute    */
/*  does occur, with both "false" and "true" values.  Working out the        */
/*  significance of this is still to do.                                     */
/*                                                                           */
/*****************************************************************************/

static void AddDayOffRequests(BCV_INFO ii)
{
  KML_ELT days_off_elt, day_off_elt, elt;
  KML_ELT employee_elt;  KML_ERROR ke;  int i, weight, day_num;  char *val;
  NRC_DAY d;  NRC_WORKER w;  bool holiday, working;

  /* day off requests */
  if( KmlContainsChild(ii->root_elt, "DayOffRequests", &days_off_elt) )
  {
    if( !KmlCheck(days_off_elt, ": *DayOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(days_off_elt);  i++ )
    {
      /* get one day off element */
      day_off_elt = KmlChild(days_off_elt, i);
      if( !KmlCheck(day_off_elt,
	  "#weight +$holiday +$working : $EmployeeID +$Date +$Day", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* weight */
      if( sscanf(KmlAttributeValue(day_off_elt, 0), "%d", &weight) != 1 )
	KmlEltFatalError(day_off_elt, ii->file_name, "invalid weight %s",
	  KmlAttributeValue(day_off_elt, 0));

      /* optional holiday, read but using it is still to do */
      holiday = false;
      if( KmlContainsAttribute(day_off_elt, "holiday", &val) )
      {
	if( !BCVBool(val, &holiday) )
	  KmlEltFatalError(day_off_elt, ii->file_name,
	    "invalid holiday %s", val);
      }

      /* optional working, read but rejected since does not occur */
      working = false;
      if( KmlContainsAttribute(day_off_elt, "working", &val) )
      {
	if( !BCVBool(val, &working) )
	  KmlEltFatalError(day_off_elt, ii->file_name,
	    "invalid working %s", val);
	KmlEltFatalError(day_off_elt, ii->file_name, "working not implemented");
      }

      /* employee ID */
      employee_elt = KmlChild(day_off_elt, 0);
      w = RetrieveWorker(ii, employee_elt);

      /* date or day */
      if( KmlContainsChild(day_off_elt, "Date", &elt) )
      {
	/* date */
	if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
	  KmlEltFatalError(elt, ii->file_name, "invalid date %s", KmlText(elt));
      }
      else if( KmlContainsChild(day_off_elt, "Day", &elt) )
      {
	/* day, starting at 0, according to xsd */
	if( sscanf(KmlText(elt), "%d", &day_num) != 1 )
	  KmlEltFatalError(elt, ii->file_name, "expected integer <Day>",
	    KmlText(elt));
	if( day_num < 0 || day_num >= NrcInstanceCycleDayCount(ii->ins) )
	  KmlEltFatalError(elt, ii->file_name, "<Day> %d out of range (0..%d)",
	    day_num, NrcInstanceCycleDayCount(ii->ins) - 1);
	d = NrcInstanceCycleDay(ii->ins, day_num);
      }
      else
	KmlEltFatalError(day_off_elt, ii->file_name, "<Date> or <Day> missing");

      /* record the day off */
      NrcWorkerAddDayOff(w, d, false, weight);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY GetDateOrDay(BCV_INFO ii, KML_ELT prnt_elt)                      */
/*                                                                           */
/*  Get a day from a <Date> or <Day> child of prnt_elt.                      */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY GetDateOrDay(BCV_INFO ii, KML_ELT prnt_elt)
{
  KML_ELT elt;  NRC_DAY d;  int day_num;
  if( KmlContainsChild(prnt_elt, "Date", &elt) )
  {
    /* date */
    if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
      KmlEltFatalError(elt, ii->file_name, "invalid date %s", KmlText(elt));
  }
  else if( KmlContainsChild(prnt_elt, "Day", &elt) )
  {
    /* day, starting at 0, according to xsd */
    if( sscanf(KmlText(elt), "%d", &day_num) != 1 )
      KmlEltFatalError(elt, ii->file_name, "expected integer <Day>",
	KmlText(elt));
    if( day_num < 0 || day_num >= NrcInstanceCycleDayCount(ii->ins) )
      KmlEltFatalError(elt, ii->file_name, "<Day> %d out of range (0..%d)",
	day_num, NrcInstanceCycleDayCount(ii->ins) - 1);
    d = NrcInstanceCycleDay(ii->ins, day_num);
  }
  else
    KmlEltFatalError(prnt_elt, ii->file_name, "<Date> or <Day> missing");
  return d;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDayOnRequests(BCV_INFO ii)                                       */
/*                                                                           */
/*  Add day on requests.  These are the same as day off requests except      */
/*  that "holiday" and "working" are omitted.                                */
/*                                                                           */
/*****************************************************************************/

static void AddDayOnRequests(BCV_INFO ii)
{
  KML_ELT days_on_elt, day_on_elt;  NRC_DAY d;  NRC_WORKER w;
  KML_ELT employee_elt;  KML_ERROR ke;  int i, weight;

  /* day on requests */
  if( KmlContainsChild(ii->root_elt, "DayOnRequests", &days_on_elt) )
  {
    if( !KmlCheck(days_on_elt, ": *DayOn", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(days_on_elt);  i++ )
    {
      /* get one day on element */
      day_on_elt = KmlChild(days_on_elt, i);
      if( !KmlCheck(day_on_elt, "#weight : $EmployeeID +$Date +$Day", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* weight */
      if( sscanf(KmlAttributeValue(day_on_elt, 0), "%d", &weight) != 1 )
	KmlEltFatalError(day_on_elt, ii->file_name, "invalid weight %s",
	  KmlAttributeValue(day_on_elt, 0));

      /* employee ID */
      employee_elt = KmlChild(day_on_elt, 0);
      w = RetrieveWorker(ii, employee_elt);

      /* date or day */
      d = GetDateOrDay(ii, day_on_elt);

      /* record the day on */
      NrcWorkerAddDayOn(w, d, false, weight);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOffRequests(BCV_INFO ii)                                    */
/*                                                                           */
/*  Add shift off and day off requests.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOffRequests(BCV_INFO ii)
{
  KML_ELT shifts_off_elt, shift_off_elt, employee_elt, shift_type_elt;
  KML_ERROR ke;  int i, weight;  NRC_DAY d;  NRC_WORKER w;  NRC_SHIFT_TYPE st;

  if( KmlContainsChild(ii->root_elt, "ShiftOffRequests", &shifts_off_elt) )
  {
    if( !KmlCheck(shifts_off_elt, ": *ShiftOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shifts_off_elt);  i++ )
    {
      /* get one shift off elt and its parts */
      shift_off_elt = KmlChild(shifts_off_elt, i);
      if( !KmlCheck(shift_off_elt,
	    "#weight : $ShiftTypeID $EmployeeID +$Date +$Day", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* weight */
      sscanf(KmlAttributeValue(shift_off_elt, 0), "%d", &weight);

      /* shift type */
      shift_type_elt = KmlChild(shift_off_elt, 0);
      if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(shift_type_elt), &st) )
	KmlEltFatalError(shift_type_elt, ii->file_name, "unknown shift %s",
	  KmlText(shift_type_elt));

      /* employee */
      employee_elt = KmlChild(shift_off_elt, 1);
      w = RetrieveWorker(ii, employee_elt);

      /* date or day */
      d = GetDateOrDay(ii, shift_off_elt);

      /* record the shift off */
      NrcWorkerAddShiftOff(w, NrcDayShiftFromShiftType(d, st), false, weight);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOnRequests(BCV_INFO ii)                                     */
/*                                                                           */
/*  Add shift on requests.                                                   */
/*                                                                           */
/*  Implementation note.  The syntax for shift on requests is the same       */
/*  as for shift off requests, except that it does allow requests for a      */
/*  shift from a shift group.  This would be easy to implement but I         */
/*  have not done it yet.  Let's see if it springs any instances first.      */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOnRequests(BCV_INFO ii)
{
  KML_ELT shifts_on_elt, shift_on_elt, employee_elt, shift_type_elt;
  KML_ERROR ke;  int i, weight;  NRC_DAY d;  NRC_WORKER w;  NRC_SHIFT_TYPE st;

  if( KmlContainsChild(ii->root_elt, "ShiftOnRequests", &shifts_on_elt) )
  {
    if( !KmlCheck(shifts_on_elt, ": *ShiftOn", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shifts_on_elt);  i++ )
    {
      /* get one shift on elt and its parts */
      shift_on_elt = KmlChild(shifts_on_elt, i);
      if( !KmlCheck(shift_on_elt,
	    "#weight : $ShiftTypeID $EmployeeID +$Date +$Day", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* weight */
      sscanf(KmlAttributeValue(shift_on_elt, 0), "%d", &weight);

      /* shift type */
      shift_type_elt = KmlChild(shift_on_elt, 0);
      if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(shift_type_elt), &st) )
	KmlEltFatalError(shift_type_elt, ii->file_name, "unknown shift %s",
	  KmlText(shift_type_elt));

      /* employee */
      employee_elt = KmlChild(shift_on_elt, 1);
      w = RetrieveWorker(ii, employee_elt);

      /* date or day */
      d = GetDateOrDay(ii, shift_on_elt);

      /* record the shift on */
      NrcWorkerAddShiftOn(w, NrcDayShiftFromShiftType(d, st), false, weight);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDayOffRequestsBetweenDates(BCV_INFO ii)                          */
/*                                                                           */
/*  Add day off between dates requests.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddDayOffRequestsBetweenDates(BCV_INFO ii)
{
  /* still to do */
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPartnerships(BCV_INFO ii)                                        */
/*                                                                           */
/*  Add partnerships.                                                        */
/*                                                                           */
/*  Implementation note.  Many instances contain the <Partnerships> tag,     */
/*  but it is empty.  One one instance, BCV-A.12.1.xml, actually contains    */
/*  partnerships, but we don't aspire to convert that instance, because      */
/*  KHE does not offer any suitable constraint.                              */
/*                                                                           */
/*****************************************************************************/

static void AddPartnerships(BCV_INFO ii)
{
  KML_ELT elt, elt2;  int i;  KML_ERROR ke;
  if( KmlContainsChild(ii->root_elt, "Partnerships", &elt) )
  {
    if( !KmlCheck(elt, ": *Partnership", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(elt);  i++ )
    {
      /* if we get to here, there is at least one actual partnership */
      elt2 = KmlChild(elt, i);
      KmlEltFatalError(elt2, ii->file_name, "<Partnership> not implemented");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPreAssignments(BCV_INFO ii)                                      */
/*                                                                           */
/*  Add preassignments.                                                      */
/*                                                                           */
/*  Implementation note.  There are none of these in the instances, so       */
/*  I have chosen not to implement them.  NRC does not currently support     */
/*  them, but that would be easy to fix.                                     */
/*                                                                           */
/*****************************************************************************/

static void AddPreAssignments(BCV_INFO ii)
{
  KML_ELT elt;
  if( KmlContainsChild(ii->root_elt, "PreAssignments", &elt) )
    KmlEltFatalError(elt, ii->file_name, "<PreAssignments> not implemented");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddFrozenDays(BCV_INFO ii)                                          */
/*                                                                           */
/*  Add frozen days.                                                         */
/*                                                                           */
/*  Implementation note.  There are none of these in the instances, so       */
/*  I have chosen not to implement them.                                     */
/*                                                                           */
/*****************************************************************************/

static void AddFrozenDays(BCV_INFO ii)
{
  KML_ELT elt;
  if( KmlContainsChild(ii->root_elt, "FrozenDays", &elt) )
    KmlEltFatalError(elt, ii->file_name, "<FrozenDays> not implemented");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "patterns"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PATTERN PatternMakeFromKmlElt(BCV_INFO ii, KML_ELT pattern_elt)          */
/*                                                                           */
/*  Make a pattern from pattern_elt, assumed to have tag Pattern.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static NRC_PATTERN PatternMakeFromKmlElt(BCV_INFO ii, KML_ELT pattern_elt)
{
  KML_ERROR ke;  char *id, *text;  int i, weight;  NRC_PATTERN res;
  KML_ELT pattern_entries_elt, pattern_entry_elt, day_elt, shift_type_elt;
  NRC_SHIFT_TYPE st;  NRC_SHIFT_TYPE_SET sts;  NRC_DAY_SET ds;
  NRC_POLARITY po;

  ** check format of pattern_elt, and get Id **
  if( !KmlCheck(pattern_elt, "ID #weight : PatternEntries", &ke) )
    KmlFatalError(ke, ii->file_name);
  id = KmlAttributeValue(pattern_elt, 0);
  sscanf(KmlAttributeValue(pattern_elt, 1), "%d", &weight);

  ** get terms and build res **
  res = NULL;
  pattern_entries_elt = KmlChild(pattern_elt, 0);
  if( !KmlCheck(pattern_entries_elt, ": *PatternEntry", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(pattern_entries_elt);  i++ )
  {
    pattern_entry_elt = KmlChild(pattern_entries_elt, i);
    if( !KmlCheck(pattern_entry_elt, "#index : ShiftType Day", &ke) )
      KmlFatalError(ke, ii->file_name);

    ** if first term, build res with extra info in pattern_weight etc. **
    if( res == NULL )
    {
      day_elt = KmlChild(pattern_entry_elt, 1);
      text = KmlText(day_elt);
      if( strcmp(text, "Any") == 0 )
        ds = NrcInstanceCycle(ii->ins);
      else if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, text, &ds) )
	KmlEltFatalError(day_elt, ii->file_name, "unknown <Day> %s", text);
      res = NrcPatternMake(ii->ins, id);
      MArrayAddLast(ii->pattern_weight, weight);
      MArrayAddLast(ii->pattern_starting_day_set, ds);
    }

    ** add one term to res **
    shift_type_elt = KmlChild(pattern_entry_elt, 0);
    text = KmlText(shift_type_elt);
    if( strcmp(text, "Any") == 0 )
      sts = NrcInstanceAllShiftTypes(ii->ins), po = NRC_POSITIVE;
    else if( strcmp(text, "None") == 0 )
      sts = NrcInstanceAllShiftTypes(ii->ins), po = NRC_NEGATIVE;
    else if( NrcInstanceRetrieveShiftType(ii->ins, text, &st) )
      sts = NrcShiftTypeSingletonShiftTypeSet(st), po = NRC_POSITIVE;
    else
      KmlEltFatalError(shift_type_elt, ii->file_name,
	"unknown <ShiftType> %s", text);
    NrcPatternAddTerm(res, sts, po);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void AddPatterns(BCV_INFO ii)                                            */
/*                                                                           */
/*  Add patterns to ii.                                                      */
/*                                                                           */
/*****************************************************************************/

/* *** BCV has no Patterns section
static void AddPatterns(BCV_INFO ii)
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main conversion functions"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *Replace(char *str, char *old, char *new)                           */
/*                                                                           */
/*  If old appears in str, replace it by new and return the new string,      */
/*  otherwise return the old string.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *Replace(char *str, char *old, char *new)
{
  char *p;
  p = strstr(str, old);
  if( p == NULL )
    return str;
  *p = '\0';
  return MStringMake("%s%s%s", str, new, &p[strlen(old)]);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *BCVInstanceIdNormalize(char *ins_id)                               */
/*                                                                           */
/*  Return a normalized version of ins_id, replacing long by Long, etc.      */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *BCVInstanceIdNormalize(char *ins_id)
{
  char *res;
  res = MStringMake("BCV-%s", ins_id);
  res = Replace(res, "long", "L");
  res = Replace(res, "medium", "M");
  res = Replace(res, "sprint", "S");
  res = Replace(res, "_hidden", "H");
  res = Replace(res, "_late", "L");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE BCVConvertInstance(INSTANCE_MODEL ins_model,                */
/*    char *instance_file_name)                                              */
/*                                                                           */
/*  Read instance_file_name, a file in the format of the first international */
/*  nurse rostering competition, and return the resulting instance.          */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE BCVConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name)
{
  NRC_INSTANCE_METADATA md;  KML_ERROR ke;  FILE *fp;  BCV_INFO ii;
  char *ins_id;

  /* make an instance info object, initially more or less empty */
  MMake(ii);
  ii->file_name = instance_file_name;
  ii->root_elt = NULL;;
  ii->ins = NULL;
  MArrayInit(ii->shift_types);
  MTableInit(ii->master_weights);
  ii->bank_holidays = NULL;  /* filled in later */
  MArrayInit(ii->histories);

  /* open and read an XML instance file, creating ii->root_elt */
  fp = fopen(instance_file_name, "r");
  if( fp == NULL )
    FatalError(instance_file_name, 0, "cannot read file \"%s\"",
      instance_file_name);
  if( !KmlReadFile(fp, &ii->root_elt, &ke, NULL) )
    KmlFatalError(ke, instance_file_name);
  fclose(fp);

  /* make sure the root tag and its children are all present and correct */
  if( strcmp(KmlLabel(ii->root_elt), "SchedulingPeriod") != 0 )
    KmlEltFatalError(ii->root_elt, ii->file_name,
      "root tag %s where SchedulingPeriod expected", KmlLabel(ii->root_elt));
  if( !KmlCheck(ii->root_elt, "ID +DepartmentID +OrganisationID "
      "+xmlns:xsi +xsi:noNamespaceSchemaLocation : "
      "$StartDate $EndDate +Skills +SkillGroups ShiftTypes +ShiftGroups "
      "Contracts Employees CoverRequirements +MasterWeights +DayOffRequests "
      "+DayOnRequests +ShiftOffRequests +ShiftOnRequests "
      "+DayOffRequestsBetweenDates +Partnerships +BankHolidays "
      "+SchedulingHistory +PreAssignments +FrozenDays ", &ke) )
    KmlFatalError(ke, ii->file_name);

  /* make instance id, no adjustment needed */
  ins_id = KmlAttributeValue(ii->root_elt, 0);

  /* make instance */
  md = InstanceModelMetaData(ins_model, ins_id, NULL, NULL, NULL, NULL, NULL);
  ii->ins = NrcInstanceMake(ins_id, md, "Nurse", true, 1);

  /* add elements producing times and time groups */
  AddStartDateAndEndDate(ii);
  AddShiftTypes(ii);
  AddShiftGroups(ii);
  AddBankHolidays(ii);

  /* add elements producing resources (including history) and resource groups */
  AddSkills(ii);
  AddSkillGroups(ii);
  AddContracts(ii);
  AddEmployees(ii);
  AddSchedulingHistory(ii);

  /* add elements producing events and cover constraints */
  AddMasterWeights(ii);	/* must precede contracts and possibly cover reqts */
  AddCoverRequirements(ii);

  /* add elements producing resource constraints (contractual) */
  AddContractConstraints(ii);
  /* AddAlternativeSkillCategories(ii); */

  /* add elements producing resource constraints (individual) */
  AddDayOffRequests(ii);
  AddDayOnRequests(ii);
  AddShiftOffRequests(ii);
  AddShiftOnRequests(ii);
  AddDayOffRequestsBetweenDates(ii);
  AddPartnerships(ii);
  AddPreAssignments(ii);
  AddFrozenDays(ii);
  return ii->ins;
}


/*****************************************************************************/
/*                                                                           */
/*  void BCVConvertSoln(char *soln_file_name, SOLN_MODEL soln_model,         */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive)                                 */
/*                                                                           */
/*  Convert soln_file_name into a solution of ins and add it to a solution   */
/*  group of archive, which is known to have at least one solution group.    */
/*                                                                           */
/*****************************************************************************/

void BCVConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive)
{
  FILE *fp;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;  char *id;
  NRC_SOLN soln;  KML_ERROR ke; NRC_WORKER w;  int i, j;  NRC_DAY d;
  KML_ELT soln_elt, asst_elt, nurse_elt, day_elt, shift_type_elt;
  NRC_SOLN_GROUP sg;

  /* must have an instance in this model */
  if( ins == NULL )
  {
    fprintf(stderr, "nrconv:  -t not permitted with -mbcv\n");
    exit(1);
  }

  /* open the file and exit if can't */
  fp = fopen(soln_file_name, "r");
  if( fp == NULL )
  {
    fprintf(stderr,
      "nrconv:  ignoring bcv solution file \"%s\" (cannot open)\n",
      soln_file_name);
    return;
  }

  /* read XML from the file, and exit if can't */
  if( !KmlReadFile(fp, &soln_elt, &ke, NULL) )
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
  soln = NrcSolnMake(ins);
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
      id = TidyNurseIdOrName(KmlText(nurse_elt));
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
  sg = SolnModelSolnGroup(soln_model, archive, "", 0.0);
  NrcSolnGroupAddSoln(sg, soln);
}

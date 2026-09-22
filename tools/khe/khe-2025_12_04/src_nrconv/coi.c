
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
/*  FILE:         coi.c                                                      */
/*  MODULE:       ConvertCOI (Curtois original instances)                    */
/*                                                                           */
/*****************************************************************************/
#include <setjmp.h>
#include "externs.h"
#define str_eq(a, b) (strcmp((a), (b)) == 0)
#define str_eq2(a, b1, b2) (str_eq((a), (b1)) || str_eq((a), (b2)))

#define HED01_SPECIAL 0		/* add special constraints to HED01 */

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
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0
#define DEBUG16 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct coi_info_rec *COI_INFO;

/*****************************************************************************/
/*                                                                           */
/*  COI_SPECIAL - a point where special-case code needs to be invoked        */
/*                                                                           */
/*****************************************************************************/

typedef void (*COI_SPECIAL_FN)(COI_INFO ii, KML_ELT elt,
  NRC_WORKER_SET contract_ws);

typedef struct coi_special_rec {
  char				*instance_name;
  int				start_line_num;
  int				end_line_num;
  COI_SPECIAL_FN		fn;
} COI_SPECIAL;

typedef HN_TABLE(COI_SPECIAL_FN) TABLE_COI_SPECIAL_FN;


/*****************************************************************************/
/*                                                                           */
/*  COI_INFO - miscellaneous information about a COI instance                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER_SET)	ARRAY_NRC_WORKER_SET;
typedef HA_ARRAY(NRC_DAY_SET)	ARRAY_NRC_DAY_SET;
typedef HA_ARRAY(NRC_PATTERN)	ARRAY_NRC_PATTERN;    /* used in AddPatterns */

struct coi_info_rec {
  HA_ARENA			arena;
  char				*file_name;
  KML_ELT			root_elt;
  NRC_INSTANCE			ins;
  NRC_WORKER_SET_TREE		worker_set_tree;
  HA_ARRAY_INT			pattern_weight;
  ARRAY_NRC_DAY_SET		pattern_starting_day_set;
  NRC_SHIFT_TYPE_SET		empty_sts;
  HA_ARRAY_BOOL			shift_type_auto_allocate;
  TABLE_COI_SPECIAL_FN		specials_table;
  jmp_buf			env;
};


/*****************************************************************************/
/*                                                                           */
/*  COI_PATTERN - a <Pattern> within <Match>                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct coi_pattern_rec {
  NRC_DAY_SET		starting_ds;	/* <Start>, <StartDate>, <StartDay>  */
  NRC_PATTERN		pattern;	/* the shift sets and polarities     */
} *COI_PATTERN;

typedef HA_ARRAY(COI_PATTERN) ARRAY_COI_PATTERN;


/*****************************************************************************/
/*                                                                           */
/*  COI_MATCH   - a <Match>                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct coi_match_rec {
  COI_BOUND		min_bound;	/* <Min>                             */
  COI_BOUND		max_bound;	/* <Max>                             */
  NRC_DAY		start_day;	/* <RegionStart>, <RegionStartDate>  */
  NRC_DAY		end_day;	/* <RegionEnd>, <RegionEndDate>      */
  int			start_day_index;
  int			end_day_index;
  ARRAY_COI_PATTERN	patterns;	/* one or more <Pattern> elements    */
} *COI_MATCH;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "unimplemented cases"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void OmitInstance(COI_INFO ii, KML_ELT elt, char *fmt, ...)              */
/*                                                                           */
/*  This function is to be called when something in an instance is not       */
/*  implemented.  It prints a warning message and abandons the instance.     */
/*                                                                           */
/*****************************************************************************/

static void OmitInstance(COI_INFO ii, KML_ELT elt, char *fmt, ...)
{
  va_list args;
  if( elt != NULL )
    fprintf(stderr, "%s:%d:%d: ", ii->file_name, KmlLineNum(elt),
      KmlColNum(elt));
  else
    fprintf(stderr, "%s: ", ii->file_name);
  fprintf(stderr, "instance omitted: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  longjmp(ii->env, 1);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing times and time groups"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddStartDateAndEndDate(COI_INFO ii)                                 */
/*                                                                           */
/*  Add the cycle and the days_of_week.                                      */
/*                                                                           */
/*****************************************************************************/

static void AddStartDateAndEndDate(COI_INFO ii)
{
  KML_ELT start_day_elt, end_day_elt;  char *err_str;
  start_day_elt = KmlChild(ii->root_elt, 0);
  end_day_elt = KmlChild(ii->root_elt, 1);
  if( !NrcCalendarCycleMake(ii->ins, KmlText(start_day_elt),
	KmlText(end_day_elt), &err_str) )
    OmitInstance(ii, ii->root_elt, "%s", err_str);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftTypes(COI_INFO ii)                                          */
/*                                                                           */
/*  Add shift types to ii.                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypes(COI_INFO ii)
{
  KML_ELT shift_types_elt, shift_type_elt, time_units_elt;
  KML_ELT elt, start_time_elt, end_time_elt;  KML_ERROR ke;
  char *id, *label;  NRC_SHIFT_TYPE st;
  int i, j, wd;  bool auto_allocate;  NRC_TIME_INTERVAL ti;
  if( DEBUG3 )
    fprintf(stderr, "[ AddShiftTypes(ii)\n");
  if( KmlContainsChild(ii->root_elt, "ShiftTypes", &shift_types_elt) )
  {
    if( !KmlCheck(shift_types_elt, ": *Shift", &ke) )
      KmlFatalError(ke, ii->file_name);
    label = NULL;
    for( i = 0;  i < KmlChildCount(shift_types_elt);  i++ )
    {
      /* find one shift type and its id */
      shift_type_elt = KmlChild(shift_types_elt, i);

      /* can't do a conventional check because order varies between instances
      if( !KmlCheck(shift_type_elt,
	    "ID : +$StartTime +$EndTime +$Duration +$Name +$Label +$Color "
	    "+$TimeUnits +$AutoAllocate +TimePeriods +Resources", &ke) )
	KmlFatalError(ke, ii->file_name);
      */

      /* get Id */
      if( KmlAttributeCount(shift_type_elt) != 1 )
	OmitInstance(ii, shift_type_elt,
	  "<Shift> does not have exactly one attribute");
      if( !str_eq(KmlAttributeName(shift_type_elt, 0), "ID") )
	OmitInstance(ii, shift_type_elt, "<Shift> has no ID attribute");
      id = KmlAttributeValue(shift_type_elt, 0);
      if( DEBUG3 )
	fprintf(stderr, "  id %s:\n", id);

      /* get other fields, in any order */
      time_units_elt = NULL;
      ti = NULL;
      auto_allocate = true;
      start_time_elt = end_time_elt = NULL;
      for( j = 0;  j < KmlChildCount(shift_type_elt);  j++ )
      {
	elt = KmlChild(shift_type_elt, j);
	if( str_eq(KmlLabel(elt), "StartTime") )
	{
	  /* StartTime */
	  start_time_elt = elt;
	}
	else if( str_eq(KmlLabel(elt), "EndTime") )
	{
	  /* EndTime */
	  end_time_elt = elt;
	}
	else if( str_eq(KmlLabel(elt), "Duration") )
	{
	  /* Duration (not in instances) */
          OmitInstance(ii, elt, "Duration not implemented");
	}
	else if( str_eq(KmlLabel(elt), "Name") )
	{
	  /* Name (not used) */
	}
	else if( str_eq(KmlLabel(elt), "Label") )
	{
	  /* Label */
	  label = KmlText(elt);
	}
	else if( str_eq(KmlLabel(elt), "Color") )
	{
	  /* Color (not used) */
	}
	else if( str_eq(KmlLabel(elt), "TimeUnits") )
	{
	  /* TimeUnits */
	  time_units_elt = elt;
	}
	else if( str_eq(KmlLabel(elt), "AutoAllocate") )
	{
	  /* AutoAllocate */
	  if( !str_eq(KmlText(elt), "true") && !str_eq(KmlText(elt), "false") )
	    OmitInstance(ii, elt, "AutoAllocate neither true not false");
	  if( str_eq(KmlText(elt), "false") )
	    auto_allocate = false;
	}
	else if( str_eq(KmlLabel(elt), "TimePeriods") )
	{
	  /* TimePeriods (not in instances) */
          OmitInstance(ii, elt, "TimePeriods not implemented");
	}
	else if( str_eq(KmlLabel(elt), "Resources") )
	{
	  /* Resources (not in instances) */
          OmitInstance(ii, elt, "Resources not implemented");
	}
	else
	  OmitInstance(ii, elt, "<%s> not expected here", KmlLabel(elt));
      }

      /* get the time interval, if any */
      if( start_time_elt != NULL && end_time_elt != NULL )
      {
	if( !NrcTimeIntervalMakeFromHMS(KmlText(start_time_elt),
	      KmlText(end_time_elt), &ti, ii->ins) )
          OmitInstance(ii, shift_type_elt,
	    "error in <StartTime> (%s) or <EndTime> (%s)",
	    KmlText(start_time_elt), KmlText(end_time_elt));
      }

      /* set workload */
      if( time_units_elt != NULL )
      {
	if( strstr(KmlText(time_units_elt), ".") != NULL )
          OmitInstance(ii, time_units_elt,
	    "non-integral TimeUnits not implemented");
	if( sscanf(KmlText(time_units_elt), "%d", &wd) != 1 )
	  OmitInstance(ii, time_units_elt, "format error in TimeUnits");
      }
      else if( ti != NULL )
	wd = (NrcTimeIntervalEndSecs(ti) - NrcTimeIntervalStartSecs(ti)) / 60;
      else
	wd = 0;

      /* add the shift type, adding start and end times separately */
      /* also add "means time off" separately                      */
      st = NrcShiftTypeMake(ii->ins, id, wd);
      if( ti != NULL )
	NrcShiftTypeAddTimeInterval(st, ti);
      if( DEBUG15 )
	fprintf(stderr, "  making shift type %s in instance %s\n",
	  id, NrcInstanceId(ii->ins));
      if( label != NULL && (str_eq(NrcInstanceId(ii->ins), "COI-BCDT-Sep") ||
	    str_eq(NrcInstanceId(ii->ins), "COI-HED01")) )
	NrcShiftTypeUseLabelInEventName(st, label);
      /* *** abandoned
      if( str_eq(NrcInstanceId(ii->ins), "COI-QMC-1") && str_eq(id, "O") )
	NrcShiftTypeSetMeansTimeOff(st, true);
      *** */
      HaArrayAddLast(ii->shift_type_auto_allocate, auto_allocate);
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] AddShiftTypes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool ShiftTypeAutoAllocate(NRC_SHIFT_TYPE st, COI_INFO ii)               */
/*                                                                           */
/*  Return the auto-allocate attribute of st, actually stored in ii.         */
/*                                                                           */
/*****************************************************************************/

static bool ShiftTypeAutoAllocate(NRC_SHIFT_TYPE st, COI_INFO ii)
{
  return HaArray(ii->shift_type_auto_allocate, NrcShiftTypeIndex(st));
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftGroups(COI_INFO ii)                                         */
/*                                                                           */
/*  Add shift groups.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddShiftGroups(COI_INFO ii)
{
  KML_ELT shift_groups_elt, shift_group_elt, shift_elt;  KML_ERROR ke;
  NRC_SHIFT_TYPE_SET sts;  NRC_SHIFT_TYPE st;  int i, j;
  if( DEBUG3 )
    fprintf(stderr, "[ AddShiftGroups(ii)\n");
  if( KmlContainsChild(ii->root_elt, "ShiftGroups", &shift_groups_elt) )
  {
    if( !KmlCheck(shift_groups_elt, ": *ShiftGroup", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shift_groups_elt);  i++ )
    {
      shift_group_elt = KmlChild(shift_groups_elt, i);
      if( !KmlCheck(shift_group_elt, "ID : *Shift", &ke) )
	KmlFatalError(ke, ii->file_name);
      sts = NrcShiftTypeSetMake(ii->ins, KmlAttributeValue(shift_group_elt, 0));
      for( j = 0;  j < KmlChildCount(shift_group_elt);  j++ )
      {
        shift_elt = KmlChild(shift_group_elt, j);
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(shift_elt), &st) )
	  OmitInstance(ii, shift_elt, "unknown shift type %s",
	    KmlText(shift_elt));
	NrcShiftTypeSetAddShiftType(sts, st);
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] AddShiftGroups\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing resources and resource groups"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *TidySkillName(char *name, HA_ARENA a)                              */
/*                                                                           */
/*  Tidy up a skill name.                                                    */
/*                                                                           */
/*****************************************************************************/

static char *TidySkillName(char *name, HA_ARENA a)
{
  return HnStringMake(a, "Skill-%s", name);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSkills(COI_INFO ii)                                              */
/*                                                                           */
/*  Add skills to ii.                                                        */
/*                                                                           */
/*****************************************************************************/

static void AddSkills(COI_INFO ii)
{
  KML_ELT skills_elt, skill_elt;  int i;  KML_ERROR ke;
  NRC_WORKER_SET skill_ws;  char *name;
  if( DEBUG3 )
    fprintf(stderr, "[ AddSkills(ii)\n");
  if( KmlContainsChild(ii->root_elt, "Skills", &skills_elt) )
  {
    if( !KmlCheck(skills_elt, ": *Skill", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(skills_elt);  i++ )
    {
      skill_elt = KmlChild(skills_elt, i);
      if( !KmlCheck(skill_elt, "ID : +$Label", &ke) )
	KmlFatalError(ke, ii->file_name);
      name = TidySkillName(KmlAttributeValue(skill_elt, 0), ii->arena);
      if( NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	OmitInstance(ii, skill_elt, "skill name %s appears twice",
	  KmlAttributeValue(skill_elt, 0));
      skill_ws = NrcWorkerSetMake(ii->ins, name);
      NrcInstanceSkillsAddSkill(ii->ins, skill_ws);
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] AddSkills returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  char *TidyNurseIdOrName(char *id, HA_ARENA a)                            */
/*                                                                           */
/*  Tidy up a nurse Id or name, by copying it into arena memory, and         */
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
/*  char *TidyContractName(char *name, HA_ARENA a)                           */
/*                                                                           */
/*  Return a tidied up contract name.                                        */
/*                                                                           */
/*****************************************************************************/

static char *TidyContractName(char *name, HA_ARENA a)
{
  return HnStringMake(a, "Contract-%s", name);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET RetrieveContractWorkerSet(COI_INFO ii,                    */
/*    KML_ELT elt, char *id)                                                 */
/*                                                                           */
/*  Return the contract worker set with this id.  The elt parameter is only  */
/*  for error messages.                                                      */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET RetrieveContractWorkerSet(COI_INFO ii,
  KML_ELT elt, char *id)
{
  NRC_WORKER_SET ws;  char *contract_id;
  contract_id = TidyContractName(id, ii->arena);
  if( !NrcInstanceContractsRetrieveContract(ii->ins, contract_id, &ws) )
    OmitInstance(ii, elt, "unknown contract %s", contract_id);
  return ws;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContracts(COI_INFO ii)                                           */
/*                                                                           */
/*  Add contracts, just the worker sets, not the constraints.                */
/*                                                                           */
/*****************************************************************************/

static void AddContracts(COI_INFO ii)
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
      /* *** order varies, we can't do this check
      if( !KmlCheck(contract_elt, "ID : +Conditionals +DailyRest +$Label "
	    "+MaxSeq +MaxTot +MaxWeekends +MinRestTime +MinSeq +MinTot "
	    "+MinWeekends +MultipleShiftsPerDay +Patterns +RestBetweenDates "
	    "+ValidShifts +Workload ",  &ke) )
	KmlFatalError(ke, ii->file_name);
      *** */

      /* create worker set for this contract */
      if( KmlAttributeCount(contract_elt) != 1 ||
	  !str_eq(KmlAttributeName(contract_elt, 0), "ID") )
	OmitInstance(ii, contract_elt, "<Contract> has wrong attribute(s)");
      id = TidyContractName(KmlAttributeValue(contract_elt, 0), ii->arena);
      contract_ws = NrcWorkerSetMake(ii->ins, id);
      NrcInstanceContractsAddContract(ii->ins, contract_ws);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddEmployees(COI_INFO ii)                                           */
/*                                                                           */
/*  Add one worker for each employee, including adding them to their skills  */
/*  and contract worker sets.                                                */
/*                                                                           */
/*****************************************************************************/

static void AddEmployees(COI_INFO ii)
{
  KML_ELT employees_elt, employee_elt, skill_elt, elt;  NRC_WORKER_SET ws;
  KML_ERROR ke;  int i, j, k;  char *employee_id, *skill_name;  NRC_WORKER w;

  KmlContainsChild(ii->root_elt, "Employees", &employees_elt);
  if( !KmlCheck(employees_elt, ": *Employee", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(employees_elt);  i++ )
  {
    /* sort out names and create one worker for employee_elt */
    employee_elt = KmlChild(employees_elt, i);
    /* *** this order is not always followed
    if( !KmlCheck(employee_elt,
	  "ID : *$ContractID +$Name +Skills +CoverResources", &ke) )
      KmlFatalError(ke, ii->file_name);
    *** */
    if( KmlAttributeCount(employee_elt) != 1 ||
	!str_eq(KmlAttributeName(employee_elt, 0), "ID") )
      OmitInstance(ii, employee_elt, "<Employee> has wrong attribute(s)");
    employee_id = KmlAttributeValue(employee_elt, 0);
    /* ***
    employee_id = TidyNurseIdOrName(KmlAttributeValue(employee_elt, 0),
      ii->arena);
    *** */
    w = NrcWorkerMake(ii->ins, employee_id);
    if( DEBUG13 )
      fprintf(stderr, "added worker %s (from %s)\n", employee_id,
        KmlAttributeValue(employee_elt, 0));

    /* handle the children */
    for( j = 0;  j < KmlChildCount(employee_elt);  j++ )
    {
      elt = KmlChild(employee_elt, j);
      if( str_eq(KmlLabel(elt), "ContractID") )
      {
	ws = RetrieveContractWorkerSet(ii, elt, KmlText(elt));
	NrcWorkerSetAddWorker(ws, w);
      }
      else if( str_eq(KmlLabel(elt), "Name") )
      {
	/* ignoring this */
      }
      else if( str_eq(KmlLabel(elt), "Skills") )
      {
	if( !KmlCheck(elt, ": *$Skill", &ke) )
	  KmlFatalError(ke, ii->file_name);
	for( k = 0;  k < KmlChildCount(elt);  k++ )
	{
	  skill_elt = KmlChild(elt, k);
	  skill_name = TidySkillName(KmlText(skill_elt), ii->arena);
	  if( !NrcInstanceSkillsRetrieveSkill(ii->ins, skill_name, &ws) )
	    OmitInstance(ii, skill_elt, "unknown skill %s", KmlText(skill_elt));
	  NrcWorkerSetAddWorker(ws, w);
	}
      }
      else if( str_eq(KmlLabel(elt), "CoverResources") )
      {
	/* ignoring this */
      }
      else
	OmitInstance(ii, elt, "invalid child <%s> of <Employee>",KmlLabel(elt));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *TidySkillGroupName(char *name, HA_ARENA a)                         */
/*                                                                           */
/*  Tidy up a skill group name.                                              */
/*                                                                           */
/*****************************************************************************/

static char *TidySkillGroupName(char *name, HA_ARENA a)
{
  return HnStringMake(a, "SkillGroup-%s", name);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSkillGroups(COI_INFO ii)                                         */
/*                                                                           */
/*  Add skill groups.                                                        */
/*                                                                           */
/*  Implementation note.  A skill group is implemented as a worker set       */
/*  containing the union of the sets of workers with the individual skills.  */
/*  Accordingly, this function must be called after those sets of workers    */
/*  have been made.                                                          */
/*                                                                           */
/*****************************************************************************/

static void AddSkillGroups(COI_INFO ii)
{
  KML_ELT skill_groups_elt, skill_group_elt, skill_elt;  int i, j, k;
  KML_ERROR ke;  NRC_WORKER_SET skill_group_ws, skill_ws;  char *name, *sname;
  NRC_WORKER w;
  if( DEBUG3 )
    fprintf(stderr, "[ AddSkillGroups(ii)\n");
  if( KmlContainsChild(ii->root_elt, "SkillGroups", &skill_groups_elt) )
  {
    if( !KmlCheck(skill_groups_elt, ": *SkillGroup", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(skill_groups_elt);  i++ )
    {
      skill_group_elt = KmlChild(skill_groups_elt, i);
      if( !KmlCheck(skill_group_elt, "ID : *$Skill", &ke) )
	KmlFatalError(ke, ii->file_name);
      name = TidySkillGroupName(KmlAttributeValue(skill_group_elt, 0),
	ii->arena);
      if( NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	OmitInstance(ii, skill_group_elt, "skill group name %s appears twice",
	  KmlAttributeValue(skill_group_elt, 0));
      skill_group_ws = NrcWorkerSetMake(ii->ins, name);
      NrcInstanceSkillsAddSkill(ii->ins, skill_group_ws);
      for( j = 0;  j < KmlChildCount(skill_group_elt);  j++ )
      {
        skill_elt = KmlChild(skill_group_elt, j);
	sname = TidySkillName(KmlText(skill_elt), ii->arena);
	if( !NrcInstanceSkillsRetrieveSkill(ii->ins, sname, &skill_ws) )
	  OmitInstance(ii, skill_elt, "skill group name %s appears twice",
	    KmlAttributeValue(skill_elt, 0));
	for( k = 0;  k < NrcWorkerSetWorkerCount(skill_ws);  k++ )
	{
	  w = NrcWorkerSetWorker(skill_ws, k);
	  if( !NrcWorkerSetContainsWorker(skill_group_ws, w) )
	    NrcWorkerSetAddWorker(skill_group_ws, w);
	}
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] AddSkillGroups returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing events and cover constraints"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_SET ReadCover(KML_ELT cover_elt, COI_INFO ii,                 */
/*    int dft_min_below_weight, int dft_max_above_weight,                    */
/*    int dft_pref_above_weight, int dft_pref_below_weight,                  */
/*    NRC_SHIFT_TYPE *st)                                                    */
/*                                                                           */
/*  Read cover_elt and return the corresponding demand set.                  */
/*                                                                           */
/*****************************************************************************/

static void CoiCoverReqtsReadCover(COI_COVER_REQTS ccr, COI_DAY_COVER dc,
  KML_ELT cover_elt, COI_INFO ii, int *tp_count)
{
  COI_COVER cv;  int i /* , j */;  char *name, *err /* , *sts_desc */;
  NRC_TIME_INTERVAL /* st_ti, */ ti;
  KML_ELT elt, start_elt, end_elt;  KML_ERROR ke;  NRC_SHIFT_TYPE st;
  NRC_WORKER_SET skill_ws;  /* NRC_SHIFT_TYPE_SET sts; */
  COI_BOUND min_bound, max_bound, pref_bound;
  if( DEBUG11 )
    fprintf(stderr, "[ CoiCoverReqtsReadCover(...)\n");
  /* res = CoiCover Make(cover_elt); */
  skill_ws = NULL;
  /* sts = NULL; */
  /* sts_desc = NULL; */
  st = NULL;
  ti = NULL;
  min_bound = max_bound = pref_bound = NULL;
  for( i = 0;  i < KmlChildCount(cover_elt);  i++ )
  {
    elt = KmlChild(cover_elt, i);
    if( str_eq(KmlLabel(elt), "Skill") )
    {
      if( skill_ws != NULL )
	OmitInstance(ii, elt,
	  "<Skill> or <SkillGroup> appears twice in <Cover>");
      name = TidySkillName(KmlText(elt), ii->arena);
      if( !NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	OmitInstance(ii, elt, "unknown skill %s", KmlText(elt));
    }
    else if( str_eq(KmlLabel(elt), "SkillGroup") )
    {
      if( skill_ws != NULL )
	OmitInstance(ii, elt,
	  "<Skill> or <SkillGroup> appears twice in <Cover>");
      name = TidySkillGroupName(KmlText(elt), ii->arena);
      if( !NrcInstanceSkillsRetrieveSkill(ii->ins, name, &skill_ws) )
	OmitInstance(ii, elt, "unknown skill group %s", KmlText(elt));
    }
    else if( str_eq(KmlLabel(elt), "TimePeriod") )
    {
      if( st != NULL )
        OmitInstance(ii, elt,"<Shift> and <TimePeriod> both appear in <Cover>");
      if( ti != NULL )
        OmitInstance(ii, elt,"<TimePeriod> appears twice in <Cover>");
      if( !KmlCheck(elt, ": $Start $End", &ke) )
	KmlFatalError(ke, ii->file_name);
      start_elt = KmlChild(elt, 0);
      end_elt = KmlChild(elt, 1);
      if( !NrcTimeIntervalMakeFromHMS(KmlText(start_elt),KmlText(end_elt),&ti,
	    ii->ins) )
	OmitInstance(ii, elt, "format error in <Start> or <End>");
      if( DEBUG11 )
	fprintf(stderr, "  time period \"%s\" to \"%s\" (%s)\n",
          KmlText(start_elt), KmlText(end_elt),
	  NrcTimeIntervalShow(ti, ii->arena));
      name = HnStringMake(ii->arena, "TimePeriod%d", ++(*tp_count));
    }
    else if( str_eq(KmlLabel(elt), "Shift") )
    {
      if( st != NULL )
        OmitInstance(ii, elt,"<Shift> appears twice in <Cover>");
      if( ti != NULL )
        OmitInstance(ii, elt,"<Shift> and <TimePeriod> both appear in <Cover>");
      if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	OmitInstance(ii, elt, "unknown <Shift> %s", KmlText(elt));
    }
    else if( str_eq(KmlLabel(elt), "ShiftGroup") )
    {
      /* ShiftGroup not in instances (?) */
      OmitInstance(ii, elt, "<ShiftGroup> not implemented");
    }
    else if( str_eq(KmlLabel(elt), "ShiftBlock") )
    {
      /* ShiftBlock not in instances */
      OmitInstance(ii, elt, "<ShiftBlock> not implemented");
    }
    else if( str_eq(KmlLabel(elt), "Min") )
    {
      if( min_bound != NULL )
	OmitInstance(ii, elt, "<Min> appears twice in <Cover>");
      if( !CoiMinBoundRead(elt, false, CoiCoverReqtsDftMinBelowWeight(ccr),
	    &min_bound, &err, ii->arena, ii->ins) )
	OmitInstance(ii, elt, err);
    }
    else if( str_eq(KmlLabel(elt), "Max") )
    {
      if( max_bound != NULL )
	OmitInstance(ii, elt, "<Max> appears twice in <Cover>");
      if( !CoiMaxBoundRead(elt, false, CoiCoverReqtsDftMaxAboveWeight(ccr),
	    &max_bound, &err, ii->arena, ii->ins) )
	OmitInstance(ii, elt, err);
    }
    else if( str_eq(KmlLabel(elt), "Preferred") )
    {
      /* not documented but present in GPost.ros */
      if( pref_bound != NULL )
	OmitInstance(ii, elt, "<Preferred> appears twice in <Cover>");
      if( !CoiPreferredBoundRead(elt, false,
	    CoiCoverReqtsDftPrefBelowWeight(ccr), false,
	    CoiCoverReqtsDftPrefAboveWeight(ccr), &pref_bound, &err,
	    ii->arena, ii->ins) )
	OmitInstance(ii, elt, err);
    }
    else if( str_eq(KmlLabel(elt), "Label") )
    {
      /* Label is unused */
    }
    else if( str_eq(KmlLabel(elt), "CoverResource") )
    {
      /* CoverResource not in instances */
      OmitInstance(ii, elt, "<CoverResource> not implemented");
    }
    else
      OmitInstance(ii, elt, "unexpected child <%s> of <Cover>", KmlLabel(elt));
  }
  if( st == NULL && ti == NULL )
    OmitInstance(ii, cover_elt, "<Cover> has neither <Shift> nor <TimePeriod>");
  cv = CoiCoverMake(cover_elt, st, ti, skill_ws, min_bound, pref_bound,
    max_bound, ii->arena);
  CoiDayCoverAddCover(dc, cv);
  if( DEBUG11 )
    fprintf(stderr, "] CoiCoverReqtsReadCover returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY DayNumToDay(KML_ELT day_elt, COI_INFO ii)                        */
/*                                                                           */
/*  Convert the value of day_elt, supposed to be a day number, into a day.   */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY DayNumToDay(KML_ELT day_elt, COI_INFO ii)
{
  int val;
  if( sscanf(KmlText(day_elt), "%d", &val) != 1 )
    OmitInstance(ii, day_elt, "invalid <%s>", KmlLabel(day_elt));
  if( val < 0 || val >= NrcInstanceCycleDayCount(ii->ins) )
    OmitInstance(ii, day_elt, "<%s> %d out of range 0 .. %d",
      KmlLabel(day_elt), val, NrcInstanceCycleDayCount(ii->ins) - 1);
  return NrcInstanceCycleDay(ii->ins, val);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY DateToDay(KML_ELT day_elt, COI_INFO ii)                          */
/*                                                                           */
/*  Convert the value of day_elt, supposed to be a date, into a day.         */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY DateToDay(KML_ELT day_elt, COI_INFO ii)
{
  NRC_DAY res;
  if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(day_elt), &res) )
    OmitInstance(ii, day_elt, "invalid <%s>", KmlLabel(day_elt));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void GetWeight(KML_ELT elt, int *weight, COI_INFO ii)                    */
/*                                                                           */
/*  Read one weight from a child of a <CoverWeights> element.                */
/*                                                                           */
/*****************************************************************************/

static void GetWeight(KML_ELT elt, int *weight, COI_INFO ii)
{
  if( *weight != -1 )
    OmitInstance(ii, elt, "<%s> appears twice in <CoverWeights>",KmlLabel(elt));
  if( sscanf(KmlText(elt), "%d", weight) != 1 )
    OmitInstance(ii, elt, "invalid value of <%s>", KmlLabel(elt));
}


/*****************************************************************************/
/*                                                                           */
/*  COI_COVER_REQTS CoiCoverReqtsRead(COI_INFO ii)                           */
/*                                                                           */
/*  Read <CoverRequirements> and <CoverWeights>, producing a cover reqts     */
/*  object.                                                                  */
/*                                                                           */
/*  From Ozkarahan.ros:                                                      */
/*                                                                           */
/*    <CoverWeights>                                                         */
/*      <MinUnderStaffing>200</MinUnderStaffing>                             */
/*      <MaxOverStaffing>200</MaxOverStaffing>                               */
/*      <PrefOverStaffing>200</PrefOverStaffing>                             */
/*      <PrefUnderStaffing>200</PrefUnderStaffing>                           */
/*    </CoverWeights>                                                        */
/*                                                                           */
/*****************************************************************************/

static COI_COVER_REQTS CoiCoverReqtsRead(COI_INFO ii)
{
  KML_ELT cover_reqts_elt, cover_weights_elt, cover_elt, day_elt, elt;
  KML_ERROR ke;  int i, j, tp_count;  COI_COVER_REQTS res;  COI_DAY_COVER dc;
  NRC_DAY_SET day_set;
  int dft_min_below_weight, dft_max_above_weight;
  int dft_pref_above_weight, dft_pref_below_weight;

  if( KmlContainsChild(ii->root_elt, "CoverRequirements", &cover_reqts_elt) )
  {
    /* optional <CoverWeights> */
    dft_min_below_weight = dft_max_above_weight = -1;
    dft_pref_above_weight = dft_pref_below_weight = -1;
    if( KmlContainsChild(ii->root_elt, "CoverWeights", &cover_weights_elt) )
    {
      for( i = 0;  i < KmlChildCount(cover_weights_elt);  i++ )
      {
	elt = KmlChild(cover_weights_elt, i);
	if( str_eq(KmlLabel(elt), "MinUnderStaffing") )
	  GetWeight(elt, &dft_min_below_weight, ii);
	else if( str_eq(KmlLabel(elt), "MaxOverStaffing") )
	  GetWeight(elt, &dft_max_above_weight, ii);
	else if( str_eq(KmlLabel(elt), "PrefOverStaffing") )
	  GetWeight(elt, &dft_pref_above_weight, ii);
	else if( str_eq(KmlLabel(elt), "PrefUnderStaffing") )
	  GetWeight(elt, &dft_pref_below_weight, ii);
	else
	  OmitInstance(ii, elt, "unexpected child <%s> of <CoverWeights>",
	    KmlLabel(elt));
      }
    }
    res = CoiCoverReqtsMake(cover_reqts_elt, dft_min_below_weight,
      dft_max_above_weight, dft_pref_above_weight, dft_pref_below_weight,
      ii->arena);

    /* back to <CoverRequirements> */
    if( !KmlCheck(cover_reqts_elt, ": *DayOfWeekCover *DateSpecificCover",&ke) )
      KmlFatalError(ke, ii->file_name);
    tp_count = 0;
    for( i = 0;  i < KmlChildCount(cover_reqts_elt);  i++ )
    {
      /* for each <DayOfWeekCover> or <DateSpecificCover> */
      day_elt = KmlChild(cover_reqts_elt, i);
      /* HaArrayAddLast(res->day_covers, dc); */

      /* get the day_set */
      if( str_eq(KmlLabel(day_elt), "DayOfWeekCover") )
      {
	if( !KmlCheck(day_elt, ": $Day *Cover", &ke) )
	  KmlFatalError(ke, ii->file_name);
	elt = KmlChild(day_elt, 0);
	if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, KmlText(elt),
	      &day_set) )
	  OmitInstance(ii, elt, "unknown full weekday name %s", KmlText(elt));
        if( DEBUG9 )
	{
	  fprintf(stderr, "  DayOfWeekCover for %s:\n", KmlText(elt));
	  NrcDaySetDebug(day_set, 2, stderr);
	}
      }
      else
      {
	if( !KmlCheck(day_elt, ": +$Date +$Day *Cover", &ke) )
	  KmlFatalError(ke, ii->file_name);
	elt = KmlChild(day_elt, 0);
	if( str_eq(KmlLabel(elt), "Date") )
	  day_set = NrcDaySingletonDaySet(DateToDay(elt, ii));
	else if( str_eq(KmlLabel(elt), "Day") )
	  day_set = NrcDaySingletonDaySet(DayNumToDay(elt, ii));
	else
	  OmitInstance(ii, elt, "<Date> or <Day> expected here");
      }

      /* make the day cover object, add individual covers, and add to res */
      dc = CoiDayCoverMake(day_elt, day_set, ii->arena);
      for( j = 1;  j < KmlChildCount(day_elt);  j++ )
      {
	cover_elt = KmlChild(day_elt, j);
	CoiCoverReqtsReadCover(res, dc, cover_elt, ii, &tp_count);
      }
      CoiCoverReqtsAddDayCover(res, dc);
    }
  }
  else
    res = CoiCoverReqtsMake(NULL, -1, -1, -1, -1, ii->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddCoverWeightsAndCoverRequirements(COI_INFO ii)                    */
/*                                                                           */
/*  Add cover weights and cover requirements.                                */
/*                                                                           */
/*****************************************************************************/

static void AddCoverWeightsAndCoverRequirements(COI_INFO ii)
{
  COI_COVER_REQTS cr;  int i, j, demand_count;  NRC_DAY d;
  NRC_SHIFT_TYPE st;  NRC_SHIFT s;  NRC_DEMAND dm;  char *err_mess;
  KML_ELT problem_elt;  /* NRC_PENALTY p; */

  /* read the requirements and make them disjoint */
  cr = CoiCoverReqtsRead(ii);
  CoiCoverReqtsMakeDayCoversDisjoint(cr);

  /* check for overlapping day-sets */
  /* *** doing this within CoiCoverReqtsRead now
  if( !CoiCoverReqtsCheckForOverlappingDaySets(cr, &err_mess) )
    OmitInstance(ii, CoiCoverReqtsElt(cr), "%s", err_mess);
  *** */

  /* generate covers */
  /* p = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, ii->ins); */
  /* dm = NrcDemandMake(ii->ins, p, p, NULL, p); */
  dm = NrcDemandMakeBegin(ii->ins);
  NrcDemandMakeEnd(dm);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    for( j = 0;  j < NrcInstanceShiftTypeCount(ii->ins);  j++ )
    {
      st = NrcInstanceShiftType(ii->ins, j);
      s = NrcDayShiftFromShiftType(d, st);
      if( ShiftTypeAutoAllocate(st, ii) )
      {
	/* just an ordinary shift, add demand_count demands */
	demand_count = CoiCoverReqtsDemandCountForShift(cr, s);
	NrcShiftAddDemandMulti(s, dm, demand_count);
      }
      else
      {
	/* a non-AutoAllocate shift, add one demand for each fixed asst */
	/* these will be added when the preassignments are added */
      }
    }
  }

  /* generate demand constraints */
  if( !CoiCoverReqtsGenerateDemandConstraints(cr, ii->ins, ii->file_name,
      NrcInstanceStaffing(ii->ins), &err_mess, &problem_elt, ii->arena) )
    OmitInstance(ii, problem_elt, "%s", err_mess);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing resource constraints (contractual)"    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddConditionals(COI_INFO ii, KML_ELT conditionals_elt,              */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  Conditionals occur in instances HED01.ros and HED01b.ros.                */
/*                                                                           */
/*****************************************************************************/
/* ***
static bool CoiSpecial(COI_INFO ii, KML_ELT elt, NRC_WORKER_SET contract_ws);
*** */

static void AddConditionals(COI_INFO ii, KML_ELT conditionals_elt,
  NRC_WORKER_SET contract_ws)
{
  /* char *label; */  KML_ELT elt;  COI_SPECIAL_FN sfn;  int i, pos;
  /* label = "Conditionals"; */
  for( i = 0;  i < KmlChildCount(conditionals_elt);  i++ )
  {
    elt = KmlChild(conditionals_elt, i);
    if( HnTableRetrieve(ii->specials_table, KmlLabel(elt), sfn, pos) )
      sfn(ii, elt, contract_ws);
    else
      OmitInstance(ii, elt, "<%s> not implemented", KmlLabel(elt));
  }
  /* *** no longer doing it this way
  if( CoiSpecial(ii, conditionals_elt, contract_ws) )
  {
    ** handled specially by CoiSpecial, so do nothing here **
  }
  else
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDailyRest(COI_INFO ii, KML_ELT daily_rest_elt,                   */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddDailyRest(COI_INFO ii, KML_ELT daily_rest_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "DailyRest";
  OmitInstance(ii, daily_rest_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMaxSeq(COI_INFO ii, KML_ELT max_seq_elt,                         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMaxSeq(COI_INFO ii, KML_ELT max_seq_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MaxSeq";
  OmitInstance(ii, max_seq_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMaxTot(COI_INFO ii, KML_ELT max_tot_elt,                         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMaxTot(COI_INFO ii, KML_ELT max_tot_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MaxTot";
  OmitInstance(ii, max_tot_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMaxWeekends(COI_INFO ii, KML_ELT max_weekends_elt,               */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMaxWeekends(COI_INFO ii, KML_ELT max_weekends_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MaxWeekends";
  OmitInstance(ii, max_weekends_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMinRestTime(COI_INFO ii, KML_ELT min_rest_time_elt,              */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMinRestTime(COI_INFO ii, KML_ELT min_rest_time_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MinRestTime";
  OmitInstance(ii, min_rest_time_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMinSeq(COI_INFO ii, KML_ELT min_seq_elt,                         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMinSeq(COI_INFO ii, KML_ELT min_seq_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MinSeq";
  OmitInstance(ii, min_seq_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMinTot(COI_INFO ii, KML_ELT min_tot_elt,                         */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMinTot(COI_INFO ii, KML_ELT min_tot_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MinTot";
  OmitInstance(ii, min_tot_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMinWeekends(COI_INFO ii, KML_ELT min_weekends_elt,               */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddMinWeekends(COI_INFO ii, KML_ELT min_weekends_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MinWeekends";
  OmitInstance(ii, min_weekends_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSingleShiftPerDay(COI_INFO ii, NRC_WORKER_SET contract_ws)       */
/*                                                                           */
/*  Make a single shift per day a hard constraint.                           */
/*                                                                           */
/*****************************************************************************/

static void AddSingleShiftPerDay(COI_INFO ii, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_SHIFT_SET starting_ss;  NRC_SHIFT_SET_SET day_sss;
  NRC_PENALTY p;
  starting_ss = NrcInstanceDailyStartingShiftSet(ii->ins);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "Single assignment per day", contract_ws,
        NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(1, p, ii->ins), starting_ss);
  day_sss = NrcDayShiftSetSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSetSet(c, day_sss, NRC_POSITIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddMultipleShiftsPerDay(COI_INFO ii,                                */
/*    KML_ELT multiple_shifts_per_day_elt, NRC_WORKER_SET contract_ws)       */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances, which means         */
/*  that in every instance we need a single shift per day.                   */
/*                                                                           */
/*****************************************************************************/

static void AddMultipleShiftsPerDay(COI_INFO ii,
  KML_ELT multiple_shifts_per_day_elt, NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "MultipleShiftsPerDay";
  OmitInstance(ii, multiple_shifts_per_day_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  COI_PATTERN CoiPatternGet(KML_ELT pattern_elt, COI_INFO ii)              */
/*                                                                           */
/*  Make a COI_PATTERN from pattern_elt.                                     */
/*                                                                           */
/*****************************************************************************/

static COI_PATTERN CoiPatternGet(KML_ELT pattern_elt, COI_INFO ii)
{
  COI_PATTERN res;  NRC_PATTERN p;  KML_ELT elt;  int i, val;  NRC_DAY d;
  NRC_SHIFT_TYPE st;  NRC_SHIFT_TYPE_SET sts;
  /* *** this order is not always followed
  if( !KmlCheck(pattern_elt, ": +#Start +$StartDay +$StartDate +Starts "
      "+StartExcludes *Shift *ShiftGroup *NotShift *NotGroup", &ke) )
    KmlFatalError(ke, ii->file_name);
  *** */

  /* make the pattern object */
  HaMake(res, ii->arena);
  /* res->start_day = NULL; */
  res->starting_ds = NULL;
  res->pattern = p = NrcPatternMake(ii->ins, NULL);
  for( i = 0;  i < KmlChildCount(pattern_elt);  i++ )
  {
    elt = KmlChild(pattern_elt, i);
    if( str_eq(KmlLabel(elt), "Start") )
    {
      if( res->starting_ds != NULL )
	OmitInstance(ii, elt,
	  "<Start> follows <Start>, <StartDay>, or <StartDate>");
      if( sscanf(KmlText(elt), "%d", &val) != 1 )
	OmitInstance(ii, elt, "<Start> value is not an integer");
      if( val < 0 || val >= NrcInstanceCycleDayCount(ii->ins) )
	OmitInstance(ii, elt, "<Start> value %d out of range (0..%d)", val,
	  NrcInstanceCycleDayCount(ii->ins));
      d = NrcInstanceCycleDay(ii->ins, val);
      res->starting_ds = NrcDaySingletonDaySet(d);
    }
    else if( str_eq(KmlLabel(elt), "StartDay") )
    {
      if( res->starting_ds != NULL )
	OmitInstance(ii, elt,
	  "<StartDay> follows <Start>, <StartDay>, or <StartDate>");
      if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins,
	    KmlText(elt), &res->starting_ds) )
	OmitInstance(ii, elt, "unknown <StartDay> %s", KmlText(elt));
    }
    else if( str_eq(KmlLabel(elt), "StartDate") )
    {
      if( res->starting_ds != NULL )
	OmitInstance(ii, elt,
	  "<StartDate> follows <Start>, <StartDay>, or <StartDate>");
      if( !NrcInstanceCycleRetrieveDay(ii->ins, KmlText(elt), &d) )
	OmitInstance(ii, elt, "unknown <StartDate> %s", KmlText(elt));
      res->starting_ds = NrcDaySingletonDaySet(d);
    }
    else if( str_eq(KmlLabel(elt), "Starts") )
    {
      OmitInstance(ii, elt, "<Starts> not implemented");
    }
    else if( str_eq(KmlLabel(elt), "StartExcludes") )
    {
      OmitInstance(ii, elt, "<StartExcludes> not implemented");
    }
    else if( str_eq(KmlLabel(elt), "Shift") )
    {
      if( str_eq(KmlText(elt), "$") )
      {
	sts = NrcInstanceAllShiftTypes(ii->ins);
	NrcPatternAddTerm(p, sts, NRC_POSITIVE);
      }
      else if( str_eq(KmlText(elt), "-") )
      {
	sts = NrcInstanceAllShiftTypes(ii->ins);
	NrcPatternAddTerm(p, sts, NRC_NEGATIVE);
      }
      else if( str_eq(KmlText(elt), "*") )
	NrcPatternAddTerm(p, ii->empty_sts, NRC_NEGATIVE);
      else
      {
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	  OmitInstance(ii, elt, "<Shift> %s unknown", KmlText(elt));
	sts = NrcShiftTypeSingletonShiftTypeSet(st);
	NrcPatternAddTerm(p, sts, NRC_POSITIVE);
      }
    }
    else if( str_eq(KmlLabel(elt), "ShiftGroup") )
    {
      if( !NrcInstanceRetrieveShiftTypeSet(ii->ins, KmlText(elt), &sts) )
	OmitInstance(ii, elt, "<ShiftGroup> %s unknown", KmlText(elt));
      NrcPatternAddTerm(p, sts, NRC_POSITIVE);
    }
    else if( str_eq(KmlLabel(elt), "NotShift") )
    {
      if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	OmitInstance(ii, elt, "<Shift> %s unknown", KmlText(elt));
      sts = NrcShiftTypeSingletonShiftTypeSet(st);
      NrcPatternAddTerm(p, sts, NRC_NEGATIVE);
    }
    else if( str_eq(KmlLabel(elt), "NotGroup") )
    {
      if( !NrcInstanceRetrieveShiftTypeSet(ii->ins, KmlText(elt), &sts) )
	OmitInstance(ii, elt, "<NotGroup> shift group %s unknown",KmlText(elt));
      NrcPatternAddTerm(p, sts, NRC_NEGATIVE);
    }
    else
      OmitInstance(ii, pattern_elt, "<%s> unknown", KmlLabel(elt));
  }
  if( NrcPatternTermCount(p) == 0 )
    OmitInstance(ii, pattern_elt, "<Pattern> is empty");
  if( res->starting_ds == NULL )
    res->starting_ds = NrcInstanceCycle(ii->ins);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiPatternReduceStarts(COI_PATTERN p, int start_day_index,          */
/*    int end_day_index, HA_ARENA a)                                         */
/*                                                                           */
/*  Reduce the starting day-set of p so that p can start only on days that   */
/*  would place it entirely within the region bounded by start_day_index     */
/*  and end_day_index.                                                       */
/*                                                                           */
/*  NB if the original starting_ds has its days in increasing order,         */
/*  then so does the new one.                                                */
/*                                                                           */
/*****************************************************************************/

static void CoiPatternReduceStarts(COI_PATTERN p, int start_day_index,
  int end_day_index, HA_ARENA a)
{
  NRC_DAY_SET new_ds;  NRC_DAY d;  int i, d_index;
  char *short_name, *long_name;
  short_name = HnStringMake(a, "%s-reduced",NrcDaySetShortName(p->starting_ds));
  long_name = HnStringMake(a, "%s-reduced", NrcDaySetLongName(p->starting_ds));
  new_ds = NrcDaySetMake(NrcDaySetInstance(p->starting_ds), short_name,
    long_name);
  for( i = 0;  i < NrcDaySetDayCount(p->starting_ds);  i++ )
  {
    d = NrcDaySetDay(p->starting_ds, i);
    d_index = NrcDayIndexInCycle(d);
    if( start_day_index <= d_index &&
	d_index + NrcPatternTermCount(p->pattern) - 1 <= end_day_index )
      NrcDaySetAddDay(new_ds, d);
  }
  p->starting_ds = new_ds;
}


/*****************************************************************************/
/*                                                                           */
/*  COI_MATCH CoiMatchGet(KML_ELT match_elt, COI_INFO ii)                    */
/*                                                                           */
/*  Make a COI_MATCH from match_elt.                                         */
/*                                                                           */
/*****************************************************************************/

static COI_MATCH CoiMatchGet(KML_ELT match_elt, COI_INFO ii)
{
  COI_MATCH res;  COI_PATTERN p;  int i, cycle_day_count;  KML_ELT elt;
  char *err;  NRC_BOUND b1, b2;  int min_val, max_val;  NRC_PENALTY junk;
  bool allow_zero;
  /* *** can't make this check, because order varies
  if( !KmlCheck(match_elt, ": +Max +Min +#RegionStart +$RegionStartDate "
      "+#RegionEnd +$RegionEndDate *Pattern", &ke) )
    KmlFatalError(ke, ii->file_name);
  *** */
  if( KmlAttributeCount(match_elt) > 0 )
    OmitInstance(ii, match_elt, "unexpected attribute(s) in <Match>");

  /* initialize res */
  HaMake(res, ii->arena);
  res->min_bound = res->max_bound = NULL;
  res->start_day = res->end_day = NULL;
  HaArrayInit(res->patterns, ii->arena);

  /* get its children */
  for( i = 0;  i < KmlChildCount(match_elt);  i++ )
  {
    elt = KmlChild(match_elt, i);
    if( str_eq(KmlLabel(elt), "Min") )
    {
      if( !CoiMinBoundRead(elt, true, 1, &res->min_bound, &err,
	    ii->arena, ii->ins) )
	OmitInstance(ii, elt, err);

      /* limits in Match don't count when they have Var */
      if( CoiBoundVar(res->min_bound) != NULL )
        res->min_bound = NULL;
    }
    else if( str_eq(KmlLabel(elt), "Max") )
    {
      if( !CoiMaxBoundRead(elt, true, 1, &res->max_bound, &err,
	    ii->arena, ii->ins) )
	OmitInstance(ii, elt, err);

      /* limits in Match don't count when they have Var */
      if( CoiBoundVar(res->max_bound) != NULL )
        res->max_bound = NULL;
    }
    else if( str_eq(KmlLabel(elt), "RegionStart") )
    {
      if( res->start_day != NULL )
	OmitInstance(ii, elt, "start day given twice");
      res->start_day = DayNumToDay(elt, ii);
    }
    else if( str_eq(KmlLabel(elt), "RegionStartDate") )
    {
      if( res->start_day != NULL )
	OmitInstance(ii, elt, "start day given twice");
      res->start_day = DateToDay(elt, ii);
    }
    else if( str_eq(KmlLabel(elt), "RegionEnd") )
    {
      if( res->end_day != NULL )
	OmitInstance(ii, elt, "end day given twice");
      res->end_day = DayNumToDay(elt, ii);
    }
    else if( str_eq(KmlLabel(elt), "RegionEndDate") )
    {
      if( res->end_day != NULL )
	OmitInstance(ii, elt, "end day given twice");
      res->end_day = DateToDay(elt, ii);
    }
    else if( str_eq(KmlLabel(elt), "Pattern") )
      HaArrayAddLast(res->patterns, CoiPatternGet(elt, ii));
    else
      OmitInstance(ii, elt, "<%s> out of place", KmlLabel(elt));
  }

  /* check consistency of <Min> and <Max> */
  if( res->min_bound != NULL && res->max_bound != NULL )
  {
    b1 = CoiBoundBound(res->min_bound);
    b2 = CoiBoundBound(res->max_bound);
    if( !NrcBoundMin(b1, &min_val, &allow_zero, &junk) )
      HnAbort("CoiMatchGet internal error 1");
    if( !NrcBoundMax(b2, &max_val, &junk) )
      HnAbort("CoiMatchGet internal error 2");
    if( min_val > max_val )
      OmitInstance(ii, match_elt, "<Min> exceeds <Max> in <Match>");
  }

  /* get start_day_index and end_day_index */
  cycle_day_count = NrcInstanceCycleDayCount(ii->ins);
  res->start_day_index =
    (res->start_day != NULL ? NrcDayIndexInCycle(res->start_day) : 0);
  res->end_day_index = (res->end_day!=NULL ? NrcDayIndexInCycle(res->end_day) :
    cycle_day_count - 1);

  /* must have at least one pattern */
  if( HaArrayCount(res->patterns) == 0 )
    OmitInstance(ii, match_elt, "<Match> has no <Pattern>");

  /* reduce starting day-sets so they lie entirely within the region */
  HaArrayForEach(res->patterns, p, i)
    CoiPatternReduceStarts(p, res->start_day_index, res->end_day_index,
      ii->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetEqual(NRC_DAY_SET ds1, NRC_DAY_SET ds2)                    */
/*                                                                           */
/*  Return true if ds1 and ds2 contain the same days.                        */
/*                                                                           */
/*  Implementation note.  This function assumes that the days appear in      */
/*  both sets in increasing order.  This is not true in general, but it      */
/*  is true of the day sets that this function is passed.                    */
/*                                                                           */
/*****************************************************************************/

static bool NrcDaySetEqual(NRC_DAY_SET ds1, NRC_DAY_SET ds2)
{
  int i;  NRC_DAY d1, d2;
  if( NrcDaySetDayCount(ds1) != NrcDaySetDayCount(ds2) )
    return false;
  for( i = 0;  i < NrcDaySetDayCount(ds1);  i++ )
  {
    d1 = NrcDaySetDay(ds1, i);
    d2 = NrcDaySetDay(ds2, i);
    if( d1 != d2 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MatchLastThreePatternsHaveSameStart(COI_MATCH m)                    */
/*                                                                           */
/*  Return true if the last three patterns of m have the same starting_ds.   */
/*                                                                           */
/*****************************************************************************/

static bool MatchLastThreePatternsHaveSameStart(COI_MATCH m)
{
  COI_PATTERN p1, p2, p3;
  if( HaArrayCount(m->patterns) < 3 )
    return false;
  p1 = HaArray(m->patterns, HaArrayCount(m->patterns) - 3);
  p2 = HaArray(m->patterns, HaArrayCount(m->patterns) - 2);
  p3 = HaArray(m->patterns, HaArrayCount(m->patterns) - 1);
  return NrcDaySetEqual(p1->starting_ds, p2->starting_ds) &&
    NrcDaySetEqual(p2->starting_ds, p3->starting_ds);
}


/*****************************************************************************/
/*                                                                           */
/*  bool MatchLastThreePatternsHaveSameLength(COI_MATCH m, int *len)         */
/*                                                                           */
/*  Return true if the last three patterns of m have the same length, and    */
/*  in that case set *len to the length.                                     */
/*                                                                           */
/*****************************************************************************/

static bool MatchLastThreePatternsHaveSameLength(COI_MATCH m, int *len)
{
  COI_PATTERN p1, p2, p3;
  *len = 0;
  if( HaArrayCount(m->patterns) < 3 )
    return false;
  p1 = HaArray(m->patterns, HaArrayCount(m->patterns) - 3);
  p2 = HaArray(m->patterns, HaArrayCount(m->patterns) - 2);
  p3 = HaArray(m->patterns, HaArrayCount(m->patterns) - 1);
  *len = NrcPatternTermCount(p1->pattern);
  return *len == NrcPatternTermCount(p2->pattern) &&
    *len == NrcPatternTermCount(p3->pattern);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET PatternShiftSetIgnoringPolarity(NRC_PATTERN p,             */
/*    NRC_DAY start_d)                                                       */
/*                                                                           */
/*  Return the set of shifts of p starting on start_d, ignoring polarity.    */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET PatternShiftSetIgnoringPolarity(NRC_PATTERN p,
  NRC_DAY start_d)
{
  NRC_SHIFT_SET res;  NRC_SHIFT_TYPE_SET sts;  NRC_POLARITY po;
  int i, di;
  NRC_DAY d;  NRC_INSTANCE ins;
  ins = NrcPatternInstance(p);
  res = NrcShiftSetMake(ins);
  di = NrcDayIndexInCycle(start_d);
  for( i = 0;  i < NrcPatternTermCount(p);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    d = NrcInstanceCycleDay(ins, di + i);
    NrcShiftSetAddShiftSet(res, NrcDayShiftSetFromShiftTypeSet(d, sts));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void MatchAddPatternOnDay(NRC_PATTERN p, NRC_DAY d, NRC_CONSTRAINT c)    */
/*                                                                           */
/*  Add p on day d to c, assuming that it can be expressed by one shift set. */
/*                                                                           */
/*****************************************************************************/

static void MatchAddPatternOnDay(NRC_PATTERN p, NRC_DAY d, NRC_CONSTRAINT c)
{
  NRC_SHIFT_SET ss;  int di;  NRC_INSTANCE ins;
  if( NrcPatternTermCount(p) == 1 )
  {
    /* single-term pattern, just add it as usual */
    NrcConstraintAddPattern(c, p, d);
  }
  else
  {
    /* multi-term pattern, all polarities known to be negative */
    ins = NrcConstraintInstance(c);
    di = NrcDayIndexInCycle(d);
    if( di + NrcPatternTermCount(p) <= NrcInstanceCycleDayCount(ins))
    {
      ss = PatternShiftSetIgnoringPolarity(p, d);
      NrcConstraintAddShiftSet(c, ss, NRC_NEGATIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MatchAddPatterns(COI_MATCH m, NRC_CONSTRAINT c, COI_PATTERN wp)     */
/*                                                                           */
/*  Add shift-sets representing the patterns of m.                           */
/*                                                                           */
/*  If wp != NULL, then the last three patterns work together to mean "busy  */
/*  at some time over two consecutive days", and wp is one of the patterns,  */
/*  the one with both terms positive.                                        */
/*                                                                           */
/*****************************************************************************/

static void MatchAddPatterns(COI_MATCH m, NRC_CONSTRAINT c, COI_PATTERN wp)
{
  int i, j, stop;  NRC_DAY d;  NRC_SHIFT_SET ss;  COI_PATTERN p;
  stop = HaArrayCount(m->patterns) - (wp != NULL ? 3 : 0);

  /* ordinary patterns */
  for( i = 0;  i < stop;  i++ )
  {
    p = HaArray(m->patterns, i);
    if( NrcDaySetDayCount(p->starting_ds) > 0 )
      for( j = 0;  j < NrcDaySetDayCount(p->starting_ds);  j++ )
	MatchAddPatternOnDay(p->pattern, NrcDaySetDay(p->starting_ds, j), c);
  }

  /* busy weekend patterns */
  if( wp != NULL && NrcDaySetDayCount(wp->starting_ds) > 0 )
    for( j = 0;  j < NrcDaySetDayCount(wp->starting_ds);  j++ )
    {
      d = NrcDaySetDay(wp->starting_ds, j);
      ss = PatternShiftSetIgnoringPolarity(wp->pattern, d);
      NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  bool PatternHasPolarities(NRC_PATTERN p, NRC_POLARITY *polarities)       */
/*                                                                           */
/*  Return true when p has the given polarities, term by term.               */
/*                                                                           */
/*****************************************************************************/

static bool PatternHasPolarities(NRC_PATTERN p, NRC_POLARITY *polarities)
{
  int i;  NRC_SHIFT_TYPE_SET sts;  NRC_POLARITY po;
  for( i = 0;  i < NrcPatternTermCount(p);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    if( po != polarities[i] )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ShiftTypeSetEqual(NRC_SHIFT_TYPE_SET sts1, NRC_SHIFT_TYPE_SET sts2) */
/*                                                                           */
/*  Return true if sts1 and sts2 are equal.                                  */
/*                                                                           */
/*****************************************************************************/

static bool ShiftTypeSetEqual(NRC_SHIFT_TYPE_SET sts1, NRC_SHIFT_TYPE_SET sts2)
{
  int i;  NRC_SHIFT_TYPE st;
  if( sts1 == sts2 )
    return true;
  if(NrcShiftTypeSetShiftTypeCount(sts1) != NrcShiftTypeSetShiftTypeCount(sts2))
    return false;
  for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(sts1);  i++ )
  {
    st = NrcShiftTypeSetShiftType(sts1, i);
    if( !NrcShiftTypeSetContainsShiftType(sts2, st) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MatchLastThreePatternsHavePolarities(COI_MATCH m,                   */
/*    NRC_POLARITY *polarities)                                              */
/*                                                                           */
/*  Return true when one of the last three patterns of m contains has the    */
/*  given polarities.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool MatchLastThreePatternsHaveOneWithPolarity(COI_MATCH m,
  NRC_POLARITY *polarities, COI_PATTERN *p)
{
  int i;
  if( HaArrayCount(m->patterns) < 3 )
    return false;
  for( i = 1;  i <= 3;  i++ )
  {
    *p = HaArray(m->patterns, HaArrayCount(m->patterns) - i);
    if( PatternHasPolarities((*p)->pattern, polarities) )
      return true;
  }
  *p = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MatchLastThreePatternsDefineBusyOverTwoDays(COI_MATCH m,            */
/*    COI_PATTERN *p)                                                        */
/*                                                                           */
/*  Return true if m's patterns match when the resource is busy at some      */
/*  point in two adjacent days.  In that case, set *p to the pattern that    */
/*  has the two positive shift-type sets.                                    */
/*                                                                           */
/*****************************************************************************/

static bool MatchLastThreePatternsDefineBusyOverTwoDays(COI_MATCH m,
  COI_PATTERN *p)
{
  static NRC_POLARITY pos_pos[] = {NRC_POSITIVE, NRC_POSITIVE};
  static NRC_POLARITY neg_pos[] = {NRC_NEGATIVE, NRC_POSITIVE};
  static NRC_POLARITY pos_neg[] = {NRC_POSITIVE, NRC_NEGATIVE};
  COI_PATTERN p1, p2, p3;  NRC_POLARITY po;  int len;
  NRC_SHIFT_TYPE_SET sts10, sts11, sts20, sts21, sts30, sts31;
  *p = NULL;
  if( HaArrayCount(m->patterns) >= 3 &&
    MatchLastThreePatternsHaveSameStart(m) &&
    MatchLastThreePatternsHaveSameLength(m, &len) && len == 2 &&
    MatchLastThreePatternsHaveOneWithPolarity(m, pos_pos, &p1) &&
    MatchLastThreePatternsHaveOneWithPolarity(m, pos_neg, &p2) &&
    MatchLastThreePatternsHaveOneWithPolarity(m, neg_pos, &p3) )
  {
    /* get the shift-type sets; we already know that the polarity is OK */
    NrcPatternTerm(p1->pattern, 0, &sts10, &po);
    NrcPatternTerm(p1->pattern, 1, &sts11, &po);
    NrcPatternTerm(p2->pattern, 0, &sts20, &po);
    NrcPatternTerm(p2->pattern, 1, &sts21, &po);
    NrcPatternTerm(p3->pattern, 0, &sts30, &po);
    NrcPatternTerm(p3->pattern, 1, &sts31, &po);

    /* the first shift-type sets of p1, p2, and p3 must be equal */
    if( !ShiftTypeSetEqual(sts10, sts20) || !ShiftTypeSetEqual(sts20, sts30) )
      return false;

    /* the second shift-type sets of p1, p2, and p3 must be equal */
    if( !ShiftTypeSetEqual(sts11, sts21) || !ShiftTypeSetEqual(sts21, sts31) )
      return false;

    /* all good, return true */
    *p = p1;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool AllTermsAreNegative(NRC_PATTERN p, COI_INFO ii)                     */
/*                                                                           */
/*  Return true if all the terms of p are zero.                              */
/*                                                                           */
/*****************************************************************************/

static bool AllTermsAreNegative(NRC_PATTERN p)
{
  int i;  NRC_SHIFT_TYPE_SET sts;  NRC_POLARITY po;
  for( i = 0;  i < NrcPatternTermCount(p);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    if( po != NRC_NEGATIVE )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MatchPatternsConvertible(COI_MATCH m, COI_PATTERN *wp)              */
/*                                                                           */
/*  Return true when m is convertible to XESTT.  It is convertible when it   */
/*  has no min_lim and no max_lim, so that it can be ignored, or when each   */
/*  of its patterns satisfies one of these conditions:                       */
/*                                                                           */
/*    * It has no starting days, so it can be ignored;                       */
/*                                                                           */
/*    * It contains a single term;                                           */
/*                                                                           */
/*    * Its terms are all negative;                                          */
/*                                                                           */
/*    * It is one of the last three patterns of m, and these three patterns  */
/*      collectively mean "busy at some time on two consecutive days".       */
/*                                                                           */
/*  If *wp != NULL on return, it means that the last three patterns of m     */
/*  collectively mean "busy at some time on two consecutive days".  In that  */
/*  case, *wp is one of these patterns, the one with both terms positive.    */
/*                                                                           */
/*****************************************************************************/

static bool MatchPatternsConvertible(COI_MATCH m, COI_PATTERN *wp)
{
  COI_PATTERN p2;  int i;  COI_PATTERN xp;

  /* ignore m if it has no min_bound and no max_bound */
  if( m->min_bound == NULL && m->max_bound == NULL )
    return true;

  /* let i index the first pattern that doesn't work without busy weekends */
  HaArrayForEach(m->patterns, p2, i)
    if( NrcDaySetDayCount(p2->starting_ds) > 0 &&
        NrcPatternTermCount(p2->pattern) > 1 &&
	!AllTermsAreNegative(p2->pattern) )
      break;

  /* if all patterns work, return true without busy weekends */
  if( i >= HaArrayCount(m->patterns) )
    return *wp = NULL, true;

  /* if all but the last three or fewer work, try with busy weekends */
  if( i >= HaArrayCount(m->patterns) - 3 &&
      MatchLastThreePatternsDefineBusyOverTwoDays(m, &xp) )
    return *wp = xp, true;

  /* no luck either way */
  return *wp = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiMaxBoundLimit(COI_BOUND max_bound)                                */
/*                                                                           */
/*  Return the limit of max_bound.                                           */
/*                                                                           */
/*****************************************************************************/

static int CoiMinBoundLimit(COI_BOUND min_bound)
{
  NRC_BOUND b;  int min_value;  NRC_PENALTY junk;  bool allow_zero;
  b = CoiBoundBound(min_bound);
  if( !NrcBoundMin(b, &min_value, &allow_zero, &junk) )
    HnAbort("CoiMinBoundLimit internal error");
  return min_value;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiMaxBoundLimit(COI_BOUND max_bound)                                */
/*                                                                           */
/*  Return the limit of max_bound.                                           */
/*                                                                           */
/*****************************************************************************/

static int CoiMaxBoundLimit(COI_BOUND max_bound)
{
  NRC_BOUND b;  int max_value;  NRC_PENALTY junk;
  b = CoiBoundBound(max_bound);
  if( !NrcBoundMax(b, &max_value, &junk) )
    HnAbort("CoiMaxBoundHasZeroLimit internal error");
  return max_value;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY CoiMaxBoundPenaltyAbove(COI_BOUND max_bound)                 */
/*                                                                           */
/*  Return the penalty for being above max_bound.                            */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY CoiMaxBoundPenaltyAbove(COI_BOUND max_bound)
{
  NRC_BOUND b;  int max_value;  NRC_PENALTY res;
  b = CoiBoundBound(max_bound);
  if( !NrcBoundMax(b, &max_value, &res) )
    HnAbort("CoiMaxBoundPenaltyAbove internal error");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY CoiMinBoundPenaltyBelow(COI_BOUND min_bound)                 */
/*                                                                           */
/*  Return the penalty for being below min_bound.                            */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY CoiMinBoundPenaltyBelow(COI_BOUND min_bound)
{
  NRC_BOUND b;  int min_value;  bool allow_zero;  NRC_PENALTY res;
  b = CoiBoundBound(min_bound);
  if( !NrcBoundMin(b, &min_value, &allow_zero, &res) )
    HnAbort("CoiMinBoundPenaltyBelow internal error");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPatterns(COI_INFO ii, KML_ELT patterns_elt,                      */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*****************************************************************************/
/* ***
static bool CoiSpecial(COI_INFO ii, KML_ELT elt, NRC_WORKER_SET contract_ws);
*** */

static void AddPatterns(COI_INFO ii, KML_ELT patterns_elt,
  NRC_WORKER_SET contract_ws)
{
  COI_MATCH m;  COI_PATTERN p, wp;  KML_ELT match_elt;  NRC_CONSTRAINT c;
  int i, j, limit, pos;  /* KML_ERROR ke; */  char *name;  NRC_PENALTY penalty;
  COI_SPECIAL_FN sfn;

  if( DEBUG5 )
    fprintf(stderr, "[ AddPatterns(%s, -, -)\n", ii->file_name);
  /* ***
  if( !KmlCheck(patterns_elt, ": *Match", &ke) )
    KmlFatalError(ke, ii->file_name);
  *** */
  for( i = 0;  i < KmlChildCount(patterns_elt);  i++ )
  {
    /* get one pattern constraint, called a <Match> */
    match_elt = KmlChild(patterns_elt, i);
    if( str_eq(KmlLabel(match_elt), "Match") )
    {
      m = CoiMatchGet(match_elt, ii);
      if( m->max_bound != NULL && CoiMaxBoundLimit(m->max_bound) == 0 )
      {
	/* each pattern is just an unwanted pattern */
	/* NB no min_limit when m->max_lim->limit == 0 */
	penalty = CoiMaxBoundPenaltyAbove(m->max_bound);
	name = CoiBoundLabel(m->max_bound);
	if( name == NULL )
	  name = "Unwanted pattern";
	HaArrayForEach(m->patterns, p, j)
	  if( NrcDaySetDayCount(p->starting_ds) > 0 )
	    c = NrcUnwantedPatternConstraintMake(ii->ins, name, contract_ws,
	      penalty, p->pattern, p->starting_ds);
      }
      else if( MatchPatternsConvertible(m, &wp) )
      {
	if( m->max_bound != NULL )
	{
	  /* generate one cluster busy times constraint with a max limit */
	  penalty = CoiMaxBoundPenaltyAbove(m->max_bound);
	  limit = CoiMaxBoundLimit(m->max_bound);
	  name = CoiBoundLabel(m->max_bound);
	  if( name == NULL )
	    name = HnStringMake(ii->arena, "At most %d pattern matches", limit);
	  c = NrcConstraintMake(ii->ins, name, contract_ws,
	    NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(limit, penalty, ii->ins),
	    NULL);
	  MatchAddPatterns(m, c, wp);
	}
	if( m->min_bound != NULL && CoiMinBoundLimit(m->min_bound) > 0 )
	{
	  /* generate one cluster busy times constraint with a min limit */
	  penalty = CoiMinBoundPenaltyBelow(m->min_bound);
	  limit = CoiMinBoundLimit(m->min_bound);
	  name = CoiBoundLabel(m->min_bound);
	  if( name == NULL )
	    name = HnStringMake(ii->arena, "At least %d pattern matches",limit);
	  c = NrcConstraintMake(ii->ins, name, contract_ws,
	    NRC_CONSTRAINT_ACTIVE,
	    NrcBoundMakeMin(limit, false, penalty, ii->ins), NULL);
	  MatchAddPatterns(m, c, wp);
	}
      }
      else
      {
	/* multiple patterns not implemented */
	OmitInstance(ii, match_elt, "this <Match> not implemented");
      }
    }
    else if( HnTableRetrieve(ii->specials_table, KmlLabel(match_elt),sfn,pos) )
      sfn(ii, match_elt, contract_ws);
    else
      KmlEltFatalError(match_elt, ii->file_name, "unexpected category <%s>",
        KmlLabel(match_elt));
  }

  if( DEBUG5 )
    fprintf(stderr, "] AddPatterns\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddRestBetweenDates(COI_INFO ii, KML_ELT rest_between_dates_elt,    */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddRestBetweenDates(COI_INFO ii, KML_ELT rest_between_dates_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "RestBetweenDates";
  OmitInstance(ii, rest_between_dates_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddValidShifts(COI_INFO ii, KML_ELT valid_shifts_elt,               */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*  This constraint does not occur in the COI instances.                     */
/*                                                                           */
/*****************************************************************************/

static void AddValidShifts(COI_INFO ii, KML_ELT valid_shifts_elt,
  NRC_WORKER_SET contract_ws)
{
  char *label;
  label = "ValidShifts";
  OmitInstance(ii, valid_shifts_elt, "<%s> not implemented", label);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddWorkload(COI_INFO ii, KML_ELT workload_elt,                      */
/*    NRC_WORKER_SET contract_ws)                                            */
/*                                                                           */
/*  Add this contract constraint.                                            */
/*                                                                           */
/*****************************************************************************/

static void AddWorkload(COI_INFO ii, KML_ELT workload_elt,
  NRC_WORKER_SET contract_ws)
{
  KML_ELT tu_elt, e;  int i, j;  KML_ERROR ke;  NRC_CONSTRAINT c;
  COI_BOUND min_bound, max_bound;  NRC_DAY start_day, end_day, d;
  NRC_SHIFT_TYPE_SET sts;  NRC_SHIFT_SET ss;  char *name, *err_mess;
  if( !KmlCheck(workload_elt, ": *TimeUnits", &ke) )
    KmlFatalError(ke, ii->file_name);
  for( i = 0;  i < KmlChildCount(workload_elt);  i++ )
  {
    tu_elt = KmlChild(workload_elt, i);
    /* *** order varies, can't make this check
    if( !KmlCheck(tu_elt, ": +Min +Max +#RegionStart +$RegionStartDate "
	"+#RegionEnd +$RegionEndDate +$ShiftGroup +$Resource", &ke) )
      KmlFatalError(ke, ii->file_name);
    *** */

    /* sort out the parameters of tu_elt */
    start_day = end_day = NULL;
    min_bound = max_bound = NULL;
    sts = NULL;
    for( j = 0;  j < KmlChildCount(tu_elt);  j++ )
    {
      e = KmlChild(tu_elt, j);
      if( str_eq(KmlLabel(e), "Min") )
      {
	if( !CoiMinBoundRead(e, false, -1, &min_bound, &err_mess,
	      ii->arena, ii->ins) )
	  OmitInstance(ii, e, err_mess);
	/* min_lim = CoiLimitMakeWorkload(e, ii); */
      }
      else if( str_eq(KmlLabel(e), "Max") )
      {
	if( !CoiMaxBoundRead(e, false, -1, &max_bound, &err_mess,
	      ii->arena, ii->ins) )
	  OmitInstance(ii, e, err_mess);
	/* max_lim = CoiLimitMakeWorkload(e, ii); */
      }
      else if( str_eq(KmlLabel(e), "RegionStart") )
      {
	if( start_day != NULL )
	  OmitInstance(ii, e, "start day defined twice");
	start_day = DayNumToDay(e, ii);
      }
      else if( str_eq(KmlLabel(e), "RegionStartDate") )
      {
	if( start_day != NULL )
	  OmitInstance(ii, e, "start day defined twice");
	start_day = DateToDay(e, ii);
      }
      else if( str_eq(KmlLabel(e), "RegionEnd") )
      {
	if( end_day != NULL )
	  OmitInstance(ii, e, "end day defined twice");
	end_day = DayNumToDay(e, ii);
      }
      else if( str_eq(KmlLabel(e), "RegionEndDate") )
      {
	if( end_day != NULL )
	  OmitInstance(ii, e, "end day defined twice");
	end_day = DateToDay(e, ii);
      }
      else if( str_eq(KmlLabel(e), "ShiftGroup") )
      {
	if( !NrcInstanceRetrieveShiftTypeSet(ii->ins, KmlText(e), &sts) )
	  OmitInstance(ii, e, "unknown <ShiftGroup> %s", KmlText(e));
      }
      else if( str_eq(KmlLabel(e), "Resource") )
      {
	OmitInstance(ii, e, "<Resource> not implemented");
      }
      else
	OmitInstance(ii, e, "unknown element <%s>", KmlLabel(e));
    }

    /* less than whole cycle is not implemented */
    /* ***
    if( start_day != NULL && start_day != NrcInstanceCycleDay(ii->ins, 0) )
      OmitInstance(ii, workload_elt,
	"<Workload> for less than full cycle not implemented");
    if( end_day != NULL && end_day !=
	NrcInstanceCycleDay(ii->ins, NrcInstanceCycleDayCount(ii->ins) - 1) )
      OmitInstance(ii, workload_elt,
	"<Workload> for less than full cycle not implemented");
    *** */

    /* less than all shift types is not implemented */
    /* ***
    if( sts != NULL &&
       NrcShiftTypeSetShiftTypeCount(sts)!=NrcInstanceShiftTypeCount(ii->ins))
      OmitInstance(ii, workload_elt,
	"<Workload> for less than all shift types not implemented");
    *** */

    /* default values for start_day, end_day, and sts */
    if( start_day == NULL )
      start_day = NrcInstanceCycleDay(ii->ins, 0);
    if( end_day == NULL )
      end_day = NrcInstanceCycleDay(ii->ins,
	NrcInstanceCycleDayCount(ii->ins) - 1);
    if( sts == NULL )
      sts = NrcInstanceAllShiftTypes(ii->ins);

    /* the shift set (if needed) */
    if( start_day != NrcInstanceCycleDay(ii->ins, 0) ||
	end_day != NrcInstanceCycleDay(ii->ins,
	NrcInstanceCycleDayCount(ii->ins) - 1) ||
	sts != NULL )
    {
      ss = NrcShiftSetMake(ii->ins);
      for( j=NrcDayIndexInCycle(start_day); j<=NrcDayIndexInCycle(end_day); j++)
      {
	d = NrcInstanceCycleDay(ii->ins, j);
	NrcShiftSetAddShiftSet(ss, NrcDayShiftSetFromShiftTypeSet(d, sts));
      }
    }
    else
      ss = NULL;

    /* min_bound */
    name = "Workload limit";
    if( min_bound != NULL )
    {
      c = NrcConstraintMake(ii->ins, name, contract_ws, NRC_CONSTRAINT_WORKLOAD,
	CoiBoundBound(min_bound), NULL);
      if( ss != NULL )
	NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
      /* ***
      CoiLimitWorkloadLimitConstraintMake(min_bound, ii->ins, contract_ws,
        ss, &c);
      *** */
    }

    /* max_bound */
    if( max_bound != NULL )
    {
      c = NrcConstraintMake(ii->ins, name, contract_ws, NRC_CONSTRAINT_WORKLOAD,
        CoiBoundBound(max_bound), NULL);
      if( ss != NULL )
	NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
      /* ***
      CoiLimitWorkloadLimitConstraintMake(max_bound, ii->ins, contract_ws,
        ss, &c);
      *** */
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddHED01Specials(COI_INFO ii, char *shift_type_name, char *desc)    */
/*                                                                           */
/*  Add special constraints for instance COI-HED01.                          */
/*                                                                           */
/*****************************************************************************/

static void AddHED01Specials(COI_INFO ii, char *shift_type_name, char *desc)
{
  NRC_CONSTRAINT c;  NRC_PENALTY p;  NRC_BOUND b;  NRC_DAY d;  NRC_SHIFT s;
  NRC_SHIFT_TYPE st;  int i;
  if( !NrcInstanceRetrieveShiftTypeByLabel(ii->ins, shift_type_name, &st) )
    HnAbort("AddHED01Specials: unknown shift type \"%s\"", shift_type_name);
  p = NrcPenalty(false, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  b = NrcBoundMakeMin(4, false, p, ii->ins);
  c = NrcConstraintMake(ii->ins, desc, NrcInstanceStaffing(ii->ins),
    NRC_CONSTRAINT_CONSECUTIVE, b, NULL);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    if( strcmp(NrcDayShortName(d), "3Tue") != 0 )
    {
      s = NrcDayShiftFromShiftType(d, st);
      NrcConstraintAddShiftSet(c, NrcShiftSingletonShiftSet(s), NRC_POSITIVE);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddContractConstraints(COI_INFO ii)                                 */
/*                                                                           */
/*  Add contract constraints.                                                */
/*                                                                           */
/*****************************************************************************/

static void AddContractConstraints(COI_INFO ii)
{
  KML_ELT contracts_elt, contract_elt, elt;  KML_ERROR ke;  int i, j;
  NRC_WORKER_SET contract_ws;  char *id;

  if( KmlContainsChild(ii->root_elt, "Contracts", &contracts_elt) )
  {
    if( !KmlCheck(contracts_elt, ": *Contract", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(contracts_elt);  i++ )
    {
      /* check syntax of one contract */
      contract_elt = KmlChild(contracts_elt, i);
      /* *** order varies, can't do this check
      if( !KmlCheck(contract_elt, "ID : +Conditionals +DailyRest +$Label "
	    "+MaxSeq +MaxTot +MaxWeekends +MinRestTime +MinSeq +MinTot "
	    "+MinWeekends +MultipleShiftsPerDay +Patterns +RestBetweenDates "
	    "+ValidShifts +Workload ", &ke) )
	KmlFatalError(ke, ii->file_name);
      *** */

      /* get contract_ws and weekends_dss for this contract */
      id = TidyContractName(KmlAttributeValue(contract_elt, 0), ii->arena);
      if( !NrcInstanceContractsRetrieveContract(ii->ins, id, &contract_ws) )
	OmitInstance(ii, contract_elt, "unknown contract %s",
          KmlAttributeValue(contract_elt, 0));
      /* weekends_dss = BuildWeekends(ii, contract_elt); */

      /* generate constraints for the contract */
      AddSingleShiftPerDay(ii, contract_ws);
      for( j = 0;  j < KmlChildCount(contract_elt);  j++ )
      {
	elt = KmlChild(contract_elt, j);
	if( str_eq(KmlLabel(elt), "Conditionals") )
	  AddConditionals(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "DailyRest") )
	  AddDailyRest(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "Label") )
	  /* do nothing */;
	else if( str_eq(KmlLabel(elt), "MaxSeq") )
	  AddMaxSeq(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MaxTot") )
	  AddMaxTot(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MaxWeekends") )
	  AddMaxWeekends(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MinRestTime") )
	  AddMinRestTime(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MinSeq") )
	  AddMinSeq(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MinTot") )
	  AddMinTot(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MinWeekends") )
	  AddMinWeekends(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "MultipleShiftsPerDay") )
	  AddMultipleShiftsPerDay(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "Patterns") )
	  AddPatterns(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "RestBetweenDates") )
	  AddRestBetweenDates(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "ValidShifts") )
	  AddValidShifts(ii, elt, contract_ws);
	else if( str_eq(KmlLabel(elt), "Workload") )
	  AddWorkload(ii, elt, contract_ws);
	else
	  OmitInstance(ii, elt, "unexpected child <%s> of <Contract>",
	    KmlLabel(elt));
      }
    }
    if( HED01_SPECIAL && strcmp(NrcInstanceId(ii->ins), "COI-HED01") == 0 )
    {
      /* add "at least 4 consecutive M shifts" to HED01 instance */
      AddHED01Specials(ii, "M", "at least 4 consecutive M shifts (JeffK)");

      /* add "at least 4 consecutive A shifts" to HED01 instance */
      AddHED01Specials(ii, "A", "at least 4 consecutive A shifts (JeffK)");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "add elements producing resource constraints (individual)"     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE RetrieveResource(COI_INFO ii, char *id)                     */
/*                                                                           */
/*  Retrieve from ii->instance the resource with this id.  Note that we      */
/*  are prepending "Nurse" often.                                            */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER RetrieveWorker(COI_INFO ii, KML_ELT nurse_elt)
{
  char *id;  NRC_WORKER w;
  id = KmlText(nurse_elt);
  /* id = TidyNurseIdOrName(KmlText(nurse_elt), ii->arena); */
  if( !NrcInstanceStaffingRetrieveWorker(ii->ins, id, &w) )
    OmitInstance(ii, nurse_elt, "cannot find worker %s", id);
  return w;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY GetDateOrDay(KML_ELT elt, COI_INFO ii)                           */
/*                                                                           */
/*  Get a day from a <Date> or <Day> child of elt.                           */
/*                                                                           */
/*****************************************************************************/

static NRC_DAY GetDateOrDay(KML_ELT elt, COI_INFO ii)
{
  KML_ELT e;  NRC_DAY res;
  if( KmlContainsChild(elt, "Date", &e) )
    res = DateToDay(e, ii);
  else if( KmlContainsChild(elt, "Day", &e) )
    res = DayNumToDay(e, ii);
  else
    OmitInstance(ii, elt, "no <Date> or <Day> in <%s>", KmlLabel(elt));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDayOffRequests(COI_INFO ii)                                      */
/*                                                                           */
/*  Add day off requests.                                                    */
/*                                                                           */
/*****************************************************************************/

static void AddDayOffRequests(COI_INFO ii)
{
  KML_ELT days_off_elt, day_off_elt, employee_elt;  NRC_PENALTY p;
  NRC_DAY d;  NRC_WORKER w;  KML_ERROR ke;  int i, weight;
  if( KmlContainsChild(ii->root_elt, "DayOffRequests", &days_off_elt) )
  {
    if( !KmlCheck(days_off_elt, ": *DayOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(days_off_elt);  i++ )
    {
      /* get one day off element and its parts */
      day_off_elt = KmlChild(days_off_elt, i);
      if( !KmlCheck(day_off_elt, "#weight : $EmployeeID +$Date +#Day", &ke) )
	KmlFatalError(ke, ii->file_name);
      employee_elt = KmlChild(day_off_elt, 0);

      /* convert its parts into entities and record the day off */
      sscanf(KmlAttributeValue(day_off_elt, 0), "%d", &weight);
      w = RetrieveWorker(ii, employee_elt);
      d = GetDateOrDay(day_off_elt, ii);
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddDayOff(w, d, p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddDayOnRequests(COI_INFO ii)                                       */
/*                                                                           */
/*  Add day on requests.                                                     */
/*                                                                           */
/*****************************************************************************/

static void AddDayOnRequests(COI_INFO ii)
{
  KML_ELT days_on_elt, day_on_elt, employee_elt;  NRC_PENALTY p;
  NRC_DAY d;  NRC_WORKER w;  KML_ERROR ke;  int i, weight;
  if( KmlContainsChild(ii->root_elt, "DayOnRequests", &days_on_elt) )
  {
    if( !KmlCheck(days_on_elt, ": *DayOn", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(days_on_elt);  i++ )
    {
      /* get one day on element and its parts */
      day_on_elt = KmlChild(days_on_elt, i);
      if( !KmlCheck(day_on_elt, "#weight : $EmployeeID +$Date +#Day", &ke) )
	KmlFatalError(ke, ii->file_name);
      employee_elt = KmlChild(day_on_elt, 0);

      /* convert its parts into entities and record the day on */
      sscanf(KmlAttributeValue(day_on_elt, 0), "%d", &weight);
      w = RetrieveWorker(ii, employee_elt);
      d = GetDateOrDay(day_on_elt, ii);
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddDayOn(w, d, p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOffRequests(COI_INFO ii)                                    */
/*                                                                           */
/*  Add shift off requests.                                                  */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOffRequests(COI_INFO ii)
{
  KML_ELT shifts_off_elt, shift_off_elt, employee_elt, elt;  NRC_PENALTY p;
  KML_ERROR ke;  int i, weight;  NRC_DAY d;  NRC_WORKER w;  NRC_SHIFT_TYPE st;
  if( KmlContainsChild(ii->root_elt, "ShiftOffRequests", &shifts_off_elt) )
  {
    if( !KmlCheck(shifts_off_elt, ": *ShiftOff", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shifts_off_elt);  i++ )
    {
      /* get one shift off elt and its parts */
      shift_off_elt = KmlChild(shifts_off_elt, i);
      if( !KmlCheck(shift_off_elt, "#weight : +$Shift +$ShiftTypeID "
	  "+$ShiftGroupId +ShiftGroup $EmployeeID +$Date +#Day", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* get the shift type, from <Shift> or <ShiftTypeId> */
      if( KmlContainsChild(shift_off_elt, "Shift", &elt) )
      {
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	  OmitInstance(ii, elt, "unknown <Shift> %s", KmlText(elt));
	if( !ShiftTypeAutoAllocate(st, ii) )
	  OmitInstance(ii, elt,
	    "shift-off request for non-auto-allocate <Shift> %s", KmlText(elt));
      }
      else if( KmlContainsChild(shift_off_elt, "ShiftTypeID", &elt) )
      {
	if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	  OmitInstance(ii, elt, "unknown <ShiftTypeId> %s", KmlText(elt));
	if( !ShiftTypeAutoAllocate(st, ii) )
	  OmitInstance(ii, elt,
	    "shift-off request for non-auto-allocate <Shift> %s", KmlText(elt));
      }
      else
	OmitInstance(ii, shift_off_elt,
	  "no <Shift> or <ShiftTypeID> not implemented");

      /* shift group not implemented */
      if( KmlContainsChild(shift_off_elt, "ShiftGroupId", &elt) )
	OmitInstance(ii, elt, "<ShiftGroupId> not implemented");
      if( KmlContainsChild(shift_off_elt, "ShiftGroup", &elt) )
	OmitInstance(ii, elt, "<ShiftGroup> not implemented");

      /* get the employee */
      KmlContainsChild(shift_off_elt, "EmployeeID", &employee_elt);
      w = RetrieveWorker(ii, employee_elt);

      /* convert its parts into entities and record the shift off */
      sscanf(KmlAttributeValue(shift_off_elt, 0), "%d", &weight);
      d = GetDateOrDay(shift_off_elt, ii);
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddShiftOff(w, NrcDayShiftFromShiftType(d, st), p);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddShiftOnRequests(COI_INFO ii)                                     */
/*                                                                           */
/*  Add shift on requests.                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddShiftOnRequests(COI_INFO ii)
{
  KML_ELT shifts_on_elt, shift_on_elt, elt, elt2;  KML_ERROR ke;
  int i, j, k, weight;  bool omit;  NRC_PENALTY p;
  NRC_DAY d;  NRC_WORKER w;  NRC_SHIFT_TYPE st, st2;  NRC_SHIFT_TYPE_SET sts;

  if( KmlContainsChild(ii->root_elt, "ShiftOnRequests", &shifts_on_elt) )
  {
    if( !KmlCheck(shifts_on_elt, ": *ShiftOn", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(shifts_on_elt);  i++ )
    {
      /* get one shift on elt and its parts */
      shift_on_elt = KmlChild(shifts_on_elt, i);
      omit = false;
      /* *** order varies, can't make this check
      if( !KmlCheck(shift_on_elt, "#weight : +$Shift +$ShiftTypeID "
	  "+$ShiftGroupId +ShiftGroup $EmployeeID +$Date +#Day", &ke) )
	KmlFatalError(ke, ii->file_name);
      *** */

      /* get the weight */
      if( KmlAttributeCount(shift_on_elt) != 1 )
	OmitInstance(ii, shift_on_elt,
	  "<ShiftOn> does not have exactly one attribute");
      if( !str_eq(KmlAttributeName(shift_on_elt, 0), "weight") )
	OmitInstance(ii, shift_on_elt, "<ShiftOn> weight attribute missing");
      if( sscanf(KmlAttributeValue(shift_on_elt, 0), "%d", &weight) != 1 )
	OmitInstance(ii, shift_on_elt,
	  "format error in <ShiftOn> weight attribute");

      st = NULL;  sts = NULL;  w = NULL;
      for( j = 0;  j < KmlChildCount(shift_on_elt);  j++ )
      {
	elt = KmlChild(shift_on_elt, j);
	if( str_eq(KmlLabel(elt), "Shift") )
	{
	  if( st != NULL || sts != NULL )
	    OmitInstance(ii, elt,
	      "<Shift>, <ShiftTypeId>, or <ShiftGroupId> given twice");
	  if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	    OmitInstance(ii, elt, "unknown <Shift> %s", KmlText(elt));
	  if( !ShiftTypeAutoAllocate(st, ii) )
	  {
	    KmlEltWarning(elt, ii->file_name,
	      "omitting shift-on request for non-auto-allocate <Shift> %s",
	      KmlText(elt));
	    omit = true;
	  }
	}
	else if( str_eq(KmlLabel(elt), "ShiftTypeID") )
	{
	  if( st != NULL || sts != NULL )
	    OmitInstance(ii, elt,
	      "<Shift>, <ShiftTypeId>, or <ShiftGroupId> given twice");
	  if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	    OmitInstance(ii, elt, "unknown <shiftTypeId> %s", KmlText(elt));
	  if( !ShiftTypeAutoAllocate(st, ii) )
	  {
	    KmlEltWarning(elt, ii->file_name,
	      "omitting shift-on request for non-auto-allocate <Shift> %s",
	      KmlText(elt));
	    omit = true;
	  }
	}
	else if( str_eq(KmlLabel(elt), "ShiftGroupID") )
	{
	  if( st != NULL || sts != NULL )
	    OmitInstance(ii, elt, "<Shift>, <ShiftTypeId>, <ShiftGroup>, "
	      "or <ShiftGroupId> given twice");
	  if( !NrcInstanceRetrieveShiftTypeSet(ii->ins, KmlText(elt), &sts) )
	    OmitInstance(ii, elt, "unknown <ShiftGroupId> %s", KmlText(elt));
	}
	else if( str_eq(KmlLabel(elt), "ShiftGroup") )
	{
	  if( st != NULL || sts != NULL )
	    OmitInstance(ii, elt, "<Shift>, <ShiftTypeId>, <ShiftGroup>, "
	      "or <ShiftGroupId> given twice");
	  if( !KmlCheck(elt, ": *Shift", &ke) )
	    KmlFatalError(ke, ii->file_name);
	  sts = NrcShiftTypeSetMake(ii->ins, NULL);
	  for( k = 0;  k < KmlChildCount(elt);  k++ )
	  {
	    elt2 = KmlChild(elt, k);
	    if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt2), &st2) )
	      OmitInstance(ii, elt, "unknown <Shift> %s", KmlText(elt2));
	    NrcShiftTypeSetAddShiftType(sts, st2);
	  }
	}
	else if( str_eq(KmlLabel(elt), "EmployeeID") )
	{
	  w = RetrieveWorker(ii, elt);
	}
	else if( str_eq(KmlLabel(elt), "Date") )
	{
	  /* OK, but read separately */
	}
	else if( str_eq(KmlLabel(elt), "Day") )
	{
	  /* OK, but read separately */
	}
	else
	  OmitInstance(ii,elt, "unexpected child <%s> of <ShiftOn>",
	    KmlLabel(elt));
      }

      /* get date or day and check all in order */
      d = GetDateOrDay(shift_on_elt, ii);
      if( w == NULL )
	OmitInstance(ii, shift_on_elt, "<ShiftOn> has no <EmployeeID>");

      /* convert its parts into entities and record the shift on */
      if( !omit )
      {
	p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
	if( st != NULL )
	  NrcWorkerAddShiftOn(w, NrcDayShiftFromShiftType(d, st), p);
	else if( sts != NULL )
	  NrcWorkerAddShiftSetOn(w, NrcDayShiftSetFromShiftTypeSet(d, sts), p);
	else
	  OmitInstance(ii, shift_on_elt,
	    "no <Shift>, <ShiftTypeID>, or <ShiftGroup>");
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddFixedAssignments(COI_INFO ii)                                    */
/*                                                                           */
/*  Add fixed assignments.                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddFixedAssignments(COI_INFO ii)
{
  KML_ELT fixed_assts_elt, employee_elt, assign_elt, elt;  char *name;
  KML_ERROR ke;  int i, j, k;  NRC_WORKER w;  NRC_DEMAND dm;  /* NRC_PENALTY p; */
  NRC_SHIFT_TYPE st;  NRC_DAY d;  NRC_SHIFT s;  bool no_shift;
  /* ***
  p = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR, ii->ins);
  dm = NrcDemandMake(ii->ins, p, p, NULL, p);
  *** */
  dm = NrcDemandMakeBegin(ii->ins);
  NrcDemandMakeEnd(dm);
  if( KmlContainsChild(ii->root_elt, "FixedAssignments", &fixed_assts_elt) )
  {
    if( !KmlCheck(fixed_assts_elt, ": *Employee", &ke) )
      KmlFatalError(ke, ii->file_name);
    for( i = 0;  i < KmlChildCount(fixed_assts_elt);  i++ )
    {
      employee_elt = KmlChild(fixed_assts_elt, i);
      if( !KmlCheck(employee_elt, ": EmployeeID *Assign", &ke) )
	KmlFatalError(ke, ii->file_name);

      /* get the worker */
      elt = KmlChild(employee_elt, 0);
      name = KmlText(elt);
      /* name = TidyNurseIdOrName(KmlText(elt), ii->arena); */
      if( !NrcInstanceStaffingRetrieveWorker(ii->ins, name, &w) )
	OmitInstance(ii, elt, "unknown <EmployeeID> \"%s\"", KmlText(elt));

      /* get the assignments */
      for( j = 1;  j < KmlChildCount(employee_elt);  j++ )
      {
        assign_elt = KmlChild(employee_elt, j);

	/* get the shift type and day */
	st = NULL;
	no_shift = false;
	d = NULL;
	for( k = 0;  k < KmlChildCount(assign_elt);  k++ )
	{
	  elt = KmlChild(assign_elt, k);
	  if( str_eq(KmlLabel(elt), "Shift") )
	  {
	    if( st != NULL || no_shift )
	      OmitInstance(ii, elt, "<Shift> given twice in <Assign>");
	    if( str_eq(KmlText(elt), "-") )
	      no_shift = true;
	    else if( !NrcInstanceRetrieveShiftType(ii->ins, KmlText(elt), &st) )
	      OmitInstance(ii, elt, "unknown <Shift> %s", KmlText(elt));
	  }
	  else if( str_eq(KmlLabel(elt), "Date") )
	  {
	    if( d != NULL )
	      OmitInstance(ii, elt, "<Day> or <Date> given twice in <Assign>");
	    d = DateToDay(elt, ii);
	  }
	  else if( str_eq(KmlLabel(elt), "Day") )
	  {
	    if( d != NULL )
	      OmitInstance(ii, elt, "<Day> or <Date> given twice in <Assign>");
	    d = DayNumToDay(elt, ii);
	  }
	  else
	    OmitInstance(ii, elt, "unexpected child <%s> of <Assign>",
              KmlLabel(elt));
	}

	if( no_shift )
	{
	  /* this is asking for a free day - a hard shift off request */
	  NrcWorkerAddDayOff(w, d,
	    NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins));
	}
	else
	{
	  /* this is an ordinary fixed assignment */
	  s = NrcDayShiftFromShiftType(d, st);
	  if( !ShiftTypeAutoAllocate(st, ii) )
	    NrcShiftAddDemand(s, dm);
	  NrcShiftAddPreassignment(s, w);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "specials"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void CoiERMGHSpecialConsecutiveWeekends(COI_INFO ii, KML_ELT elt,        */
/*    NRC_WORKER_SET contract_ws, NRC_POLARITY po, int limit,                */
/*    bool include_ends)                                                     */
/*                                                                           */
/*  Implement a limit in ERMGH on the number of consecutive busy or free     */
/*  weekends.  Setting po to NRC_POSITIVE limits busy weekends, setting it   */
/*  to NRC_NEGATIVE limits free weekends.  Parameter limit is the limit.     */
/*  If include_ends is true, include the ends weekends (the first Sunday     */
/*  and the last Saturday), else don't include them.  The penalty is 100.    */
/*                                                                           */
/*****************************************************************************/

static void CoiERMGHSpecialConsecutiveWeekends(COI_INFO ii, KML_ELT elt,
  NRC_WORKER_SET contract_ws, int weight, NRC_POLARITY po, int limit,
  bool include_ends)
{
  NRC_CONSTRAINT c;  char *name;  NRC_SHIFT_SET ss;  int i, stop_i;  NRC_DAY d;
  NRC_PENALTY p;

  /* make the constraint */
  name = HnStringMake(ii->arena,
    "At most %d consecutive %s weekend%s (recoded)", limit,
    po == NRC_POSITIVE ? "busy" : "free", limit == 1 ? "" : "s");
  p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, name, contract_ws, NRC_CONSTRAINT_CONSECUTIVE,
    NrcBoundMakeMax(limit, p, ii->ins), NULL);
  /* ***
  c = NrcConstraintMake(ii->ins, name, contract_ws, p,
    NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
  *** */

  /* add one shift-set for each weekend */
  stop_i = NrcInstanceCycleDayCount(ii->ins);
  if( !include_ends )  stop_i--;
  for( i = include_ends ? -1 : 6;  i < stop_i;  i += 7 )
  {
    /* build a shift-set for the weekend beginning at day i, assuming that */
    /* the first day is a Sunday and the last day is a Saturday, as in ERMGH */
    ss = NrcShiftSetMake(ii->ins);
    if( i >= 0 )
    {
      d = NrcInstanceCycleDay(ii->ins, i);
      NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(d));
    }
    if( i + 1 < NrcInstanceCycleDayCount(ii->ins) )
    {
      d = NrcInstanceCycleDay(ii->ins, i + 1);
      NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(d));
    }

    /* add ss to c */
    NrcConstraintAddShiftSet(c, ss, po);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiERMGHSpecialMaxOneBusyConsecutiveWeekends(COI_INFO ii,           */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the special case of max one busy consecutive weekends.            */
/*                                                                           */
/*****************************************************************************/

static void CoiERMGHSpecialMaxOneBusyConsecutiveWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  CoiERMGHSpecialConsecutiveWeekends(ii, elt, contract_ws, 100,
    NRC_POSITIVE, 1, true);
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends(COI_INFO ii,           */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the special case of max two busy consecutive weekends.            */
/*                                                                           */
/*****************************************************************************/

static void CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  CoiERMGHSpecialConsecutiveWeekends(ii, elt, contract_ws, 100,
    NRC_POSITIVE, 2, true);
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiORTEC02SpecialMaxThreeBusyWeekends(COI_INFO ii,                  */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the ORTEC02 special of maximum of three busy weekends, including  */
/*  the night shift on the concluding Friday as a weekend.                   */
/*                                                                           */
/*****************************************************************************/

static void CoiORTEC02SpecialMaxThreeBusyWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_DAY_SET ds;  NRC_DAY d;  int i;  NRC_CONSTRAINT c;  NRC_SHIFT_SET ss;
  NRC_SHIFT_TYPE st;  NRC_PENALTY p;
  p = NrcPenalty(false, 1000, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "Max 3 weekends (recoded)", contract_ws,
    NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMax(3, p, ii->ins), NULL);
  /* ***
  c = NrcConstraintMake(ii->ins, "Max 3 weekends (recoded)", contract_ws, p,
    NrcLimit(NRC_LIMIT_MAX, 3), NULL);
  *** */
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Friday", &ds) )
    HnAbort("CoiORTEC02SpecialMaxThreeBusyWeekends: cannot find Friday");
  for( i = 0;  i < NrcDaySetDayCount(ds);  i++ )
  {
    d = NrcDaySetDay(ds, i);
    if( NrcDayIndexInCycle(d) != NrcInstanceCycleDayCount(ii->ins) - 1 )
    {
      /* d (a Friday) is not the last day of the cycle; add following weekend */
      ss = NrcShiftSetMake(ii->ins);
      d = NrcInstanceCycleDay(ii->ins, NrcDayIndexInCycle(d) + 1);
      NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(d));
      d = NrcInstanceCycleDay(ii->ins, NrcDayIndexInCycle(d) + 1);
      NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(d));
    }
    else
    {
      /* d (a Friday) is the last day of the cycle; add night shift */
      if( !NrcInstanceRetrieveShiftType(ii->ins, "N", &st) )
        HnAbort("CoiORTEC02SpecialMaxThreeBusyWeekends: cannot find shift N");
      ss = NrcShiftSingletonShiftSet(NrcDayShiftFromShiftType(d, st));
    }
    NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiHED01SpecialConditionals(COI_INFO ii,                            */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the HED01 special of conditional constraints.                     */
/*                                                                           */
/*****************************************************************************/

static void CoiHED01SpecialConditionals(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_DAY_SET_SET dss;  NRC_DAY_SET ds, ds2, sat_ds, sun_ds;  NRC_DAY d;
  int i, j, k;  NRC_CONSTRAINT c;  NRC_SHIFT_SET ss;  NRC_PENALTY p;
  NRC_SHIFT_TYPE st, st2;  char *name;
  static char *shift_types[] = {"1", "2", "3"};  /* i.e. M, A, and N */
  static int limits[] = {4, 5, 4, 5, 4};
  if( DEBUG10 )
    fprintf(stderr, "[ CoiHED01SpecialConditionals(...)\n");

  /* get the day-sets for the weekend days */
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Saturday", &sat_ds) )
    HnAbort("CoiHED01SpecialConditionals: no sat_ds");
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Sunday", &sun_ds) )
    HnAbort("CoiHED01SpecialConditionals: no sun_ds");

  dss = NrcDaySetSetMake(ii->ins, "weeks", "weeks");
  ds = NrcDaySetMake(ii->ins, "special", "special");
  NrcDaySetSetAddDaySet(dss, ds);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    if( NrcDayDayOfWeek(d) == sat_ds )
    {
      /* time for a new day-set, but don't include d in it */
      ds = NrcDaySetMake(ii->ins, "special", "special");
      NrcDaySetSetAddDaySet(dss, ds);
    }
    else if( NrcDayDayOfWeek(d) == sun_ds )
    {
      /* ignore Sundays */
    }
    else
      NrcDaySetAddDay(ds, d);
  }
  if( DEBUG10 )
  {
    fprintf(stderr, "  day-sets:\n");
    NrcDaySetSetDebug(dss, 4, stderr);
  }
  HnAssert(NrcDaySetSetDaySetCount(dss) == 5,
    "CoiHED01SpecialConditionals day-set sets (%d, expected 5)",
    NrcDaySetSetDaySetCount(dss));

  /* if at least one [MA] in week [12345], then at least [45] in that week */
  p = NrcPenalty(false, 9, NRC_COST_FUNCTION_LINEAR, ii->ins);
  for( i = 0;  i < 2;  i++ )  /* there are 3 shift_types but only 2 used here */
  {
    if( !NrcInstanceRetrieveShiftType(ii->ins, shift_types[i], &st) )
      HnAbort("CoiHED01SpecialConditionals: shift type %s", shift_types[i]);
    for( j = 0;  j < NrcDaySetSetDaySetCount(dss);  j++ )
    {
      ds = NrcDaySetSetDaySet(dss, j);
      name = HnStringMake(ii->arena,
	"If 1 \"%s\" then at least %d \"%s\" in Week %d (recoded)",
	NrcShiftTypeName(st), limits[j], NrcShiftTypeName(st), j + 1);
      if( DEBUG10 )
	fprintf(stderr, "  %s\n", name);
      c = NrcConstraintMake(ii->ins, name, contract_ws,
	NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMin(limits[j], true, p, ii->ins),
	NULL);
      /* ***
      c = NrcConstraintMake(ii->ins, name, contract_ws, p,
	NrcLimit(NRC_LIMIT_MIN_OR_ZERO, limits[j]), NULL);
      *** */
      for( k = 0;  k < NrcDaySetDayCount(ds);  k++ )
      {
	d = NrcDaySetDay(ds, k);
	ss = NrcShiftSingletonShiftSet(NrcDayShiftFromShiftType(d, st));
	NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);
      }
    }
  }

  /* rotations */
  p = NrcPenalty(false, 4, NRC_COST_FUNCTION_LINEAR, ii->ins);
  for( j = 0;  j < NrcDaySetSetDaySetCount(dss) - 1;  j++ )
  {
    ds = NrcDaySetSetDaySet(dss, j);
    ds2 = NrcDaySetSetDaySet(dss, j + 1);
    for( i = 0;  i < 3;  i++ )
    {
      if( !NrcInstanceRetrieveShiftType(ii->ins, shift_types[i], &st) )
	HnAbort("CoiHED01SpecialConditionals: shift type %s", shift_types[i]);
      if( !NrcInstanceRetrieveShiftType(ii->ins, shift_types[(i+1) % 3], &st2) )
	HnAbort("CoiHED01SpecialConditionals: shift type %s",
	  shift_types[(i + 1) % 3]);
      name = HnStringMake(ii->arena,
	"Rotate from \"%s\" in Week %d to \"%s\" in Week %d (recoded)",
        NrcShiftTypeName(st), j + 1, NrcShiftTypeName(st2), j + 2);
      if( DEBUG10 )
	fprintf(stderr, "  %s\n", name);
      c = NrcConstraintMake(ii->ins, name, contract_ws, NRC_CONSTRAINT_ACTIVE,
	NrcBoundMakeMax(1, p, ii->ins), NULL);
      /* ***
      c = NrcConstraintMake(ii->ins, name, contract_ws, p,
	NrcLimit(NRC_LIMIT_MAX, 1), NULL);
      *** */

      /* Week j shift-set appears positively */
      ss = NrcDaySetShiftSetFromShiftType(ds, st);
      NrcConstraintAddShiftSet(c, ss, NRC_POSITIVE);

      /* Week j + 1 shift-set appears negatively */
      ss = NrcDaySetShiftSetFromShiftType(ds2, st2);
      NrcConstraintAddShiftSet(c, ss, NRC_NEGATIVE);
    }
  }
  if( DEBUG10 )
    fprintf(stderr, "] CoiHED01SpecialConditionals returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiERMGHSpecialCompleteWeekends(COI_INFO ii,                        */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the ERMGH complete weekends special, which looks like this:       */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>0</Count><Weight>100</Weight></Max>                      */
/*      <RegionStart>0</RegionStart><RegionEnd>41</RegionEnd>                */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>$</Shift><Shift>-</Shift></Pattern>                         */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><Shift>$</Shift></Pattern>                         */
/*    </Match>                                                               */
/*                                                                           */
/*  This function also handles equivalent patterns from GPost and ERRVH,     */
/*  which coincidentally have the same weight.  The ERRVH one is:            */
/*                                                                           */
/*    <Match>                                                                */
/*     <Max><Count>0</Count><Weight>100</Weight></Max>                       */
/*     <Pattern><StartDay>Saturday</StartDay>                                */
/*       <Shift>$</Shift><Shift>-</Shift></Pattern>                          */
/*     <Pattern><StartDay>Saturday</StartDay>                                */
/*       <Shift>-</Shift><Shift>$</Shift></Pattern>                          */
/*    </Match>                                                               */
/*                                                                           */
/*****************************************************************************/

static void CoiERMGHSpecialCompleteWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_DAY_SET sat_ds;  NRC_DAY first_sat, first_sun;  NRC_CONSTRAINT c;
  NRC_PENALTY p;
  if( DEBUG10 )
    fprintf(stderr, "[ CoiERMGHSpecialCompleteWeekends(...)\n");

  /* get the first Saturday and the immediately following Sunday */
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Saturday", &sat_ds) )
    HnAbort("CoiERMGHSpecialCompleteWeekends: no sat_ds");
  first_sat = NrcDaySetDay(sat_ds, 0);
  first_sun = NrcDayNext(first_sat);

  /* add the constraint and the two days' shift sets */
  p = NrcPenalty(false, 100, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "Complete weekends (recoded)", contract_ws,
      NRC_CONSTRAINT_ACTIVE, NrcBoundMakeMin(2, true, p, ii->ins),
      NrcInstanceWeeklyStartingShiftSet(ii->ins));
  NrcConstraintAddShiftSet(c, NrcDayShiftSet(first_sat), NRC_POSITIVE);
  NrcConstraintAddShiftSet(c, NrcDayShiftSet(first_sun), NRC_POSITIVE);
  if( DEBUG10 )
    fprintf(stderr, "] CoiERMGHSpecialCompleteWeekends returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiERMGHSpecialMaxOneFreeConsecutiveWeekends(COI_INFO ii,           */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a maximum limit of one on the        */
/*  number of consecutive free weekends.                                     */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>1</Count><Weight>100</Weight></Max>                      */
/*      <RegionStart>6</RegionStart><RegionEnd>14</RegionEnd>                */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><Shift>-</Shift></Pattern>                         */
/*    </Match>                                                               */
/*    <Match>                                                                */
/*      <Max><Count>1</Count><Weight>100</Weight></Max>                      */
/*      <RegionStart>13</RegionStart><RegionEnd>21</RegionEnd>               */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><Shift>-</Shift></Pattern>                         */
/*    </Match>                                                               */
/*    <Match>                                                                */
/*      <Max><Count>1</Count><Weight>100</Weight></Max>                      */
/*      <RegionStart>20</RegionStart><RegionEnd>28</RegionEnd>               */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><Shift>-</Shift></Pattern>                         */
/*    </Match>                                                               */
/*    <Match>                                                                */
/*      <Max><Count>1</Count><Weight>100</Weight></Max>                      */
/*      <RegionStart>27</RegionStart><RegionEnd>35</RegionEnd>               */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><Shift>-</Shift></Pattern>                         */
/*    </Match>                                                               */
/*                                                                           */
/*****************************************************************************/

static void CoiERMGHSpecialMaxOneFreeConsecutiveWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  CoiERMGHSpecialConsecutiveWeekends(ii, elt, contract_ws, 100,
    NRC_NEGATIVE, 1, false);
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialMinTwoFreeConsecutiveDays(COI_INFO ii,                */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a minimum limit of two on the        */
/*  number of consecutive free days:                                         */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>10</Weight>                                */
/*          <Label>Min 2 consecutive free shifts</Label>                     */
/*      </Max>                                                               */
/*        <Pattern><Start>0</Start><Shift>-</Shift>                          */
/*          <ShiftGroup>All</ShiftGroup></Pattern>                           */
/*        <Pattern><ShiftGroup>All</ShiftGroup><Shift>-</Shift>              */
/*          <ShiftGroup>All</ShiftGroup></Pattern>                           */
/*    </Match>                                                               */
/*                                                                           */
/*  This does permit a single free day at the end, however.  So it needs a   */
/*  non-zero history_after value (this code generates history_after = 1).    */
/*                                                                           */
/*****************************************************************************/

/* *** no longer a special case (condensing takes care of it now)
static void CoiPostSpecialMinTwoFreeConsecutiveDays(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  char *name;  int i;  NRC_DAY d;  NRC_PENALTY p;

  ** make the constraint **
  name = HnStringMake(ii->arena, "At least 2 consecutive free days (recoded)");
  p = NrcPenalty(false, 10, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, name, contract_ws, NRC_CONSTRAINT_CONSECUTIVE,
    NrcBoundMakeMin(2, false, p, ii->ins), NULL);

  ** add one shift-set for each day **
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_NEGATIVE);
  }

  ** add history_after **
  NrcConstraintAddHistory(c, 0, 1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialWeekends(COI_INFO ii, KML_ELT elt,                    */
/*    NRC_WORKER_SET contract_ws, int weight, NRC_POLARITY po, int limit)    */
/*                                                                           */
/*  Place a maximum limit on free or busy weekends.  Unlike                  */
/*  CoiERMGHSpecialConsecutiveWeekends, this does not hande the ends in a    */
/*  special way.                                                             */
/*                                                                           */
/*****************************************************************************/

static void CoiPostSpecialWeekends(COI_INFO ii, KML_ELT elt,
  NRC_WORKER_SET contract_ws, int weight, NRC_POLARITY po, int limit,
  NRC_CONSTRAINT_TYPE type)
{
  NRC_CONSTRAINT c;  char *name;  NRC_SHIFT_SET ss;  int i;
  NRC_DAY sat_d, sun_d;  NRC_PENALTY p;  NRC_DAY_SET sat_ds, sun_ds;

  /* make the constraint */
  name = HnStringMake(ii->arena,
    "At most %d %s%s weekend%s (recoded)", limit,
    type == NRC_CONSTRAINT_CONSECUTIVE ? "consecutive " : "",
    po == NRC_POSITIVE ? "busy" : "free", limit == 1 ? "" : "s");
  p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, name, contract_ws, type,
    NrcBoundMakeMax(limit, p, ii->ins), NULL);
  /* ***
  c = NrcConstraintMake(ii->ins, name, contract_ws, p,
    NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
  *** */

  /* get the day-sets for the weekend days */
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Saturday", &sat_ds) )
    HnAbort("CoiPostSpecialWeekends: no sat_ds");
  if( !NrcInstanceDaysOfWeekRetrieveDaySetLong(ii->ins, "Sunday", &sun_ds) )
    HnAbort("CoiPostSpecialWeekends: no sun_ds");
  HnAssert(NrcDaySetDayCount(sat_ds) == NrcDaySetDayCount(sun_ds),
    "CoiPostSpecialWeekends: %d Saturdays != %d Sundays",
    NrcDaySetDayCount(sat_ds), NrcDaySetDayCount(sun_ds));

  /* add one shift-set for each weekend */
  for( i = 0;  i < NrcDaySetDayCount(sat_ds);  i++ )
  {
    sat_d = NrcDaySetDay(sat_ds, i);
    sun_d = NrcDaySetDay(sun_ds, i);
    ss = NrcShiftSetMake(ii->ins);
    NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(sat_d));
    NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(sun_d));
    NrcConstraintAddShiftSet(c, ss, po);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialMaxTwoConsecutiveBusyWeekends(COI_INFO ii,            */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a maximum limit of two on the        */
/*  number of consecutive busy weekends:                                     */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>2</Count><Weight>1000</Weight>                           */
/*        <Label>Max 2 consecutive working weekends</Label></Max>            */
/*      <RegionStart>5</RegionStart>                                         */
/*      <RegionEnd>20</RegionEnd>                                            */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <ShiftGroup>All</ShiftGroup><Shift>-</Shift></Pattern>             */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><ShiftGroup>All</ShiftGroup></Pattern>             */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <ShiftGroup>All</ShiftGroup><ShiftGroup>All</ShiftGroup></Pattern> */
/*    </Match>                                                               */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>2</Count><Weight>1000</Weight>                           */
/*        <Label>Max 2 consecutive working weekends</Label></Max>            */
/*      <RegionStart>12</RegionStart>                                        */
/*      <RegionEnd>27</RegionEnd>                                            */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <ShiftGroup>All</ShiftGroup><Shift>-</Shift></Pattern>             */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <Shift>-</Shift><ShiftGroup>All</ShiftGroup></Pattern>             */
/*      <Pattern><StartDay>Saturday</StartDay>                               */
/*        <ShiftGroup>All</ShiftGroup><ShiftGroup>All</ShiftGroup></Pattern> */
/*    </Match>                                                               */
/*                                                                           */
/*  This is the time window implementation with two windows, weeks 1-3       */
/*  and weeks 2-4.                                                           */
/*                                                                           */
/*****************************************************************************/

static void CoiPostSpecialMaxTwoConsecutiveBusyWeekends(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  CoiPostSpecialWeekends(ii, elt, contract_ws, 1000,
    NRC_POSITIVE, 2, NRC_CONSTRAINT_CONSECUTIVE);
  /* *** ends are wrong
  CoiERMGHSpecialConsecutiveWeekends(ii, elt, contract_ws, 1000,
    NRC_POSITIVE, 2, true);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiOzkarahanSpecialNoBusyWeekend(COI_INFO ii,                       */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a maximum limit of zero on the       */
/*  number of busy weekends:                                                 */
/*                                                                           */
/*    <Match>                                                                */
/*       <Max><Count>0</Count><Weight>400</Weight></Max>                     */
/*       <Pattern><StartDay>Saturday</StartDay>                              */
/*         <Shift>$</Shift><Shift>-</Shift></Pattern>                        */
/*       <Pattern><StartDay>Saturday</StartDay>                              */
/*         <Shift>-</Shift><Shift>$</Shift></Pattern>                        */
/*       <Pattern><StartDay>Saturday</StartDay>                              */
/*         <Shift>$</Shift><Shift>$</Shift></Pattern>                        */
/*    </Match>                                                               */
/*                                                                           */
/*  This can be generated as is as three unwanted patterns, but that is      */
/*  not a natural way to express what is wanted here.                        */
/*                                                                           */
/*****************************************************************************/

static void CoiOzkarahanSpecialNoBusyWeekend(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  CoiPostSpecialWeekends(ii, elt, contract_ws, 400, NRC_POSITIVE, 0,
    NRC_CONSTRAINT_ACTIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiIkegamiSpecialMaxSixDaysBetweenDayShifts(COI_INFO ii,            */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a maximum limit of 6 on the          */
/*  number of consecutive days following a day shift during which            */
/*  the nurse is not assigned a day shift:                                   */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>0</Count><Weight>100</Weight>                            */
/*        <Label>Max 6 days between day shifts</Label></Max>                 */
/*      <Pattern><Shift>D</Shift><NotShift>D</NotShift>                      */
/*        <NotShift>D</NotShift><NotShift>D</NotShift><NotShift>D</NotShift> */
/*        <NotShift>D</NotShift><NotShift>D</NotShift><NotShift>D</NotShift> */
/*      </Pattern>                                                           */
/*    </Match>                                                               */
/*                                                                           */
/*  Replace it with a limit active intervals constraint.  Each time group    */
/*  is a negative time group with one day shift in it.  The maximum limit    */
/*  is 6.  We add a history of 7 at the start for each resource, so that     */
/*  an initial run not preceded by a day shift is not penalized.  This only  */
/*  works because the cost function is NRC_COST_FUNCTION_STEP.               */
/*                                                                           */
/*****************************************************************************/

static void CoiIkegamiSpecialMaxSixDaysBetweenDayShifts(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_BOUND b;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;
  NRC_PENALTY p;  int i;  NRC_WORKER w;
  if( NrcInstanceCycleDayCount(ii->ins) > 6 )
  {
    /* consective constraint, max 6, weight 100 */
    p = NrcPenalty(false, 100, NRC_COST_FUNCTION_STEP, ii->ins);
    b = NrcBoundMakeMax(6, p, ii->ins);
    c = NrcConstraintMake(ii->ins, "Max 6 days between day shifts (recoded)",
      contract_ws, NRC_CONSTRAINT_CONSECUTIVE, b, NULL);

    /* for each day, one negative time group containing its day shift */
    if( !NrcInstanceRetrieveShiftType(ii->ins, "D", &st) )
      HnAbort("CoiIkegamiSpecialMaxSixDaysBetweenDayShifts: no D shift type");
    for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
    {
      s = NrcDayShiftFromShiftType(NrcInstanceCycleDay(ii->ins, i), st);
      NrcConstraintAddShiftSet(c, NrcShiftSingletonShiftSet(s), NRC_NEGATIVE);
    }

    /* history before of 7 for each worker */
    NrcConstraintAddHistory(c, 7, 0);
    for( i = 0;  i < NrcWorkerSetWorkerCount(contract_ws);  i++ )
    {
      w = NrcWorkerSetWorker(contract_ws, i);
      NrcConstraintAddHistoryWorker(c, w, 7);
    }
  }
}

/*****************************************************************************/
/*                                                                           */
/*  void CoiIkegamiSpecialMinSixDaysBetweenNightShifts(COI_INFO ii,          */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Replace the following, which places a minimum limit of 6 on the          */
/*  number of consecutive days between two night shifts:                     */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max><Count>0</Count><Weight>100</Weight>                            */
/*        <Label>Min 6 days between N shifts</Label></Max>                   */
/*      <Pattern><Shift>N</Shift><Shift>*</Shift><Shift>N</Shift></Pattern>  */
/*      <Pattern><Shift>N</Shift><Shift>*</Shift><Shift>*</Shift>            */
/*        <Shift>N</Shift></Pattern>                                         */
/*      <Pattern><Shift>N</Shift><Shift>*</Shift><Shift>*</Shift>            */
/*        <Shift>*</Shift><Shift>N</Shift></Pattern>                         */
/*      <Pattern><Shift>N</Shift><Shift>*</Shift><Shift>*</Shift>            */
/*        <Shift>*</Shift><Shift>*</Shift><Shift>N</Shift></Pattern>         */
/*      <Pattern><Shift>N</Shift><Shift>*</Shift><Shift>*</Shift>            */
/*        <Shift>*</Shift><Shift>*</Shift><Shift>*</Shift><Shift>N</Shift>   */
/*      </Pattern>                                                           */
/*    </Match>                                                               */
/*                                                                           */
/*  Replace it with a limit active intervals constraint.  Each time group    */
/*  is a negative time group with one night shift in it.  The maximum limit  */
/*  is 6.  We add a history of 7 at the start for each resource, so that     */
/*  an initial run not preceded by a day shift is not penalized.             */
/*                                                                           */
/*  This conversion is not exact.  It is acceptable only because the         */
/*  constraint's weight is higher than the cost of good solutions, and so    */
/*  all that matters is that violations in one formulation are violations    */
/*  in the other.                                                            */
/*                                                                           */
/*****************************************************************************/

static void CoiIkegamiSpecialMinSixDaysBetweenNightShifts(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_BOUND b;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;
  NRC_PENALTY p;  int i;  NRC_WORKER w;
  if( NrcInstanceCycleDayCount(ii->ins) > 6 )
  {
    /* consective constraint, min 6, weight 100 */
    p = NrcPenalty(false, 100, NRC_COST_FUNCTION_STEP, ii->ins);
    b = NrcBoundMakeMin(6, false, p, ii->ins);
    c = NrcConstraintMake(ii->ins,
      "Min 6 days between N shifts (recoded inexactly)",
      contract_ws, NRC_CONSTRAINT_CONSECUTIVE, b, NULL);

    /* for each day, one negative time group containing its N shift */
    if( !NrcInstanceRetrieveShiftType(ii->ins, "N", &st) )
      HnAbort("CoiIkegamiSpecialMinSixDaysBetweenNightShifts: no N shift type");
    for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
    {
      s = NrcDayShiftFromShiftType(NrcInstanceCycleDay(ii->ins, i), st);
      NrcConstraintAddShiftSet(c, NrcShiftSingletonShiftSet(s), NRC_NEGATIVE);
    }

    /* history before of 6 for each worker */
    NrcConstraintAddHistory(c, 6, 6);
    for( i = 0;  i < NrcWorkerSetWorkerCount(contract_ws);  i++ )
    {
      w = NrcWorkerSetWorker(contract_ws, i);
      NrcConstraintAddHistoryWorker(c, w, 6);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialMinTwoConsecutiveNightShifts(COI_INFO ii,             */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  At least two consecutive night shifts, except at the end (i.e.           */
/*  history_after = 1):                                                      */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>1000</Weight>                              */
/*      </Max>                                                               */
/*      <Pattern><Start>0</Start>                                            */
/*        <Shift>N</Shift><NotShift>N</NotShift></Pattern>                   */
/*      <Pattern><NotShift>N</NotShift><Shift>N</Shift>                      */
/*        <NotShift>N</NotShift></Pattern>                                   */
/*    </Match>                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** no longer a special case (condensing takes care of it now)
static void CoiPostSpecialMinTwoConsecutiveNightShifts(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_SHIFT_TYPE st;  NRC_CONSTRAINT c;  NRC_PENALTY p;  NRC_SHIFT s;
  NRC_DAY d;  int i;

  ** add a constraint **
  p = NrcPenalty(false, 1000, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins,
    "At least two consecutive night shifts (recoded)", contract_ws,
    NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(2, false, p, ii->ins), NULL);
  ** ***
  c = NrcConstraintMake(ii->ins,
    "At least two consecutive night shifts (recoded)", contract_ws, p,
    NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, 2), NULL);
  *** **

  ** add all night shifts as shift sets **
  if( !NrcInstanceRetrieveShiftType(ii->ins, "N", &st) )
    HnAbort("CoiPostSpecialMinTwoConsecutiveNightShifts: no \"N\" shift type");
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    s = NrcDayShiftFromShiftType(d, st);
    NrcConstraintAddShiftSet(c, NrcShiftSingletonShiftSet(s), NRC_POSITIVE);
  }

  ** add history **
  NrcConstraintAddHistory(c, 0, 1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialMinFourConsecutiveShifts(COI_INFO ii,                 */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  At least 4 consecutive shifts, except at the end (history_after = 4).    */
/*  But there is a complication:  the penalty for 3 is 1, the penalty for    */
/*  2 is 4, and the penalty for 1 is 100.  So this is recoded using two      */
/*  constraints, one with minimum limit 4, penalty 1, and quadratic, and     */
/*  the other with minimum limit 2 and penalty 91.   This way it adds up:    */
/*                                                                           */
/*     Number consecutive    Required penalty   Constraint 1 + Constraint 2  */
/*     --------------------------------------------------------------------  */
/*       4                     0                  0          +   0           */
/*       3                     1                  1          +   0           */
/*       2                     4                  4          +   0           */
/*       1                   100                  9          +  91           */
/*     --------------------------------------------------------------------  */
/*                                                                           */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>100</Weight>                               */
/*      </Max>                                                               */
/*      <Pattern><Start>0</Start><ShiftGroup>All</ShiftGroup>                */
/*        <Shift>-</Shift></Pattern>                                         */
/*      <Pattern><Shift>-</Shift><ShiftGroup>All</ShiftGroup>                */
/*        <Shift>-</Shift></Pattern>                                         */
/*    </Match>                                                               */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>4</Weight>                                 */
/*      </Max>                                                               */
/*      <Pattern><Start>0</Start><ShiftGroup>All</ShiftGroup>                */
/*        <ShiftGroup>All</ShiftGroup><Shift>-</Shift></Pattern>             */
/*      <Pattern><Shift>-</Shift><ShiftGroup>All</ShiftGroup>                */
/*        <ShiftGroup>All</ShiftGroup><Shift>-</Shift></Pattern>             */
/*    </Match>                                                               */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>1</Weight>                                 */
/*      </Max>                                                               */
/*      <Pattern><Start>0</Start><ShiftGroup>All</ShiftGroup>                */
/*        <ShiftGroup>All</ShiftGroup><ShiftGroup>All</ShiftGroup>           */
/*        <Shift>-</Shift></Pattern>                                         */
/*      <Pattern><Shift>-</Shift><ShiftGroup>All</ShiftGroup>                */
/*        <ShiftGroup>All</ShiftGroup><ShiftGroup>All</ShiftGroup>           */
/*        <Shift>-</Shift></Pattern>                                         */
/*    </Match>                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** no longer a special case (condensing takes care of it now)
static void CoiPostSpecialMinFourConsecutiveShifts(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_PENALTY p;  NRC_DAY d;  int i;

  ** Constraint 1: min limit 4, penalty 1, quadratic **
  p = NrcPenalty(false, 1, NRC_COST_FUNCTION_QUADRATIC, ii->ins);
  c = NrcConstraintMake(ii->ins,
    "At least four consecutive shifts (recoded, part 1)", contract_ws,
    NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(4, false, p, ii->ins), NULL);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
  }
  NrcConstraintAddHistory(c, 0, 4);

  ** Constraint 2: min limit 2, penalty 91 **
  p = NrcPenalty(false, 91, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins,
    "At least four consecutive shifts (recoded, part 2)", contract_ws,
    NRC_CONSTRAINT_CONSECUTIVE, NrcBoundMakeMin(2, false, p, ii->ins), NULL);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
  }
  NrcConstraintAddHistory(c, 0, 4);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiPostSpecialMinTwoConsecutiveShifts(COI_INFO ii,                  */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  At least 2 consecutive shifts, except at the end (history_after = 2).    */
/*  This is from the part-time contract; the complication in the previous    */
/*  special, which was from the full-time contract, is absent.  Luckily,     */
/*  the layout in GPost.ros is different so the two won't be confused.       */
/*                                                                           */
/*    <Match>                                                                */
/*      <Max>                                                                */
/*        <Count>0</Count><Weight>100</Weight>                               */
/*      </Max>                                                               */
/*      <Pattern><Start>0</Start><ShiftGroup>All</ShiftGroup>                */
/*        <Shift>-</Shift></Pattern>                                         */
/*      <Pattern><Shift>-</Shift><ShiftGroup>All</ShiftGroup>                */
/*        <Shift>-</Shift></Pattern>                                         */
/*    </Match>                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** no longer a special case (condensing takes care of it now)
static void CoiPostSpecialMinTwoConsecutiveShifts(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  NRC_CONSTRAINT c;  NRC_PENALTY p;  NRC_DAY d;  int i;

  p = NrcPenalty(false, 100, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "At least two consecutive shifts (recoded)",
    contract_ws, NRC_CONSTRAINT_CONSECUTIVE,
    NrcBoundMakeMin(2, false, p, ii->ins), NULL);
  for( i = 0;  i < NrcInstanceCycleDayCount(ii->ins);  i++ )
  {
    d = NrcInstanceCycleDay(ii->ins, i);
    NrcConstraintAddShiftSet(c, NrcDayShiftSet(d), NRC_POSITIVE);
  }
  NrcConstraintAddHistory(c, 0, 2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiHED01bSpecialConditionals(COI_INFO ii,                           */
/*    KML_ELT elt, NRC_WORKER_SET contract_ws)                               */
/*                                                                           */
/*  Handle the HED01b special of conditional constraints.                    */
/*                                                                           */
/*****************************************************************************/

/* *** omitting HED01b, because Curtois omits it in his paper
static void CoiHED01bSpecialConditionals(COI_INFO ii,
  KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  ** sti ll to do **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  specials - static variable holding special cases                         */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static COI_SPECIAL specials[] =
{
  {"COI-ERMGH",   107,  148, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   232,  273, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   371,  412, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   503,  544, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   647,  688, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   796,  837, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",   936,  977, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  1306, 1347, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  1446, 1487, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  1590, 1631, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  1739, 1780, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  1883, 1924, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2037, 2078, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2187, 2228, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2346, 2387, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2505, 2546, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2654, 2695, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  2802, 2843, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  3235, 3276, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  3379, 3420, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  3687, 3728, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  3976, 4017, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  4130, 4171, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  4284, 4325, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  4420, 4461, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  4585, 4626, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  4845, 4886, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  5258, 5299, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  5412, 5453, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  5561, 5602, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  5720, 5761, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERMGH",  5864, 5905, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},

  {"COI-ERMGH",  1173, 1209, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  2941, 2977, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  3076, 3112, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  3529, 3565, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  3846, 3882, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  4989, 5025, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},
  {"COI-ERMGH",  5122, 5158, &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends},

  {"COI-ORTEC02",  91,   99, &CoiORTEC02SpecialMaxThreeBusyWeekends},
  {"COI-ORTEC02", 305,  313, &CoiORTEC02SpecialMaxThreeBusyWeekends},
  {"COI-ORTEC02", 519,  527, &CoiORTEC02SpecialMaxThreeBusyWeekends},

  ** CHILD has same weekend thing that ERMGH does **
  {"COI-CHILD",   104,  145, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",   268,  309, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",   432,  473, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",   611,  652, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",   775,  816, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",   954,  995, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1113, 1154, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1287, 1328, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1457, 1498, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1632, 1673, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1806, 1847, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  1966, 2007, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  2236, 2277, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  2400, 2441, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  2564, 2605, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  2728, 2769, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  2892, 2933, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3066, 3107, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3235, 3276, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3414, 3455, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3583, 3624, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3742, 3783, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  3927, 3968, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4106, 4147, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4270, 4311, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4430, 4475, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4598, 4639, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4773, 4814, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  4952, 4993, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5111, 5152, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5285, 5326, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5449, 5490, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5613, 5654, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5777, 5818, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  5941, 5982, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  6101, 6142, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  6275, 6316, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-CHILD",  6536, 6577, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},

  ** ERRVH has same weekend thing that ERMGH does **
  {"COI-ERRVH",   110,  151, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",   279,  320, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",   432,  473, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",   605,  646, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",   770,  811, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",   931,  972, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1096, 1137, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1257, 1298, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1418, 1459, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1579, 1620, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1740, 1781, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  1909, 1950, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2074, 2115, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2247, 2288, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2416, 2457, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2581, 2622, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2746, 2787, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  2915, 2956, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3082, 3123, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3255, 3296, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3424, 3465, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3581, 3622, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3738, 3779, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  3911, 3952, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4072, 4113, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4241, 4282, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4398, 4439, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4555, 4596, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4724, 4765, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  4893, 4934, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5058, 5099, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5219, 5260, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5372, 5413, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5529, 5570, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5694, 5735, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  5863, 5904, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6032, 6073, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6185, 6226, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6354, 6395, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6515, 6556, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6684, 6725, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  6853, 6894, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7006, 7047, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7159, 7200, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7332, 7373, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7481, 7522, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7654, 7695, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7819, 7860, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  7984, 8025, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  8149, 8190, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-ERRVH",  8310, 8351, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},

  ** MER has same weekend thing that ERMGH does **
  {"COI-MER",    482,   523, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",    891,   932, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   1300,  1341, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   1709,  1750, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   2118,  2159, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   2527,  2568, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   2946,  2987, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   3365,  3406, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   3784,  3825, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   4193,  4234, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   4202,  4643, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   5011,  5052, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   5420,  5461, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   5829,  5870, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   6238,  6279, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   6647,  6688, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   7056,  7097, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   7465,  7506, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   7874,  7915, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   8293,  8334, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   8712,  8753, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   9121,  9162, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   9540,  9581, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",   9949,  9990, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  10358, 10399, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  10767, 10808, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  11186, 11227, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  11595, 11636, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  12014, 12055, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  12423, 12464, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  12832, 12873, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  13241, 13282, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  13650, 13691, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  14059, 14100, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  14468, 14509, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  14877, 14918, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  15296, 15337, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  15705, 15746, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  16114, 16155, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  16523, 16564, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  16942, 16983, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  17351, 17392, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  17760, 17801, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  18179, 18220, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  18588, 18629, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  19007, 19048, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  19426, 19467, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  19835, 19876, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  20244, 20285, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  20653, 20694, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  21072, 21113, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  21481, 21522, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},
  {"COI-MER",  21900, 21941, &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends},

  {"COI-HED01",  254,   387, &CoiHED01SpecialConditionals},
  ** {"HED01b.ros", 255,   390, &CoiHED01bSpecialConditionals}, **

  {NULL, 0, 0, NULL}
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool CoiSpecial(COI_INFO ii, KML_ELT elt, NRC_WORKER_SET contract_ws)    */
/*                                                                           */
/*  If elt is to be handled specially, do that and return true.              */
/*                                                                           */
/*****************************************************************************/

/* *** no longer doing it this way
static bool CoiSpecial(COI_INFO ii, KML_ELT elt, NRC_WORKER_SET contract_ws)
{
  int line_num, i;
  line_num = KmlLineNum(elt);
  for( i = 0;  specials[i].instance_name != NULL;  i++ )
  {
    if( specials[i].start_line_num <= line_num &&
        line_num <= specials[i].end_line_num &&
        str_eq(NrcInstanceId(ii->ins), specials[i].instance_name) )
    {
      ** it's a special, but only call the function the first time **
      if( specials[i].start_line_num == line_num )
	specials[i].fn(ii, elt, contract_ws);
      return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main conversion functions"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *COIFileStem(char *file_name)                                       */
/*                                                                           */
/*  Return the part of file_name that follows the last '/' or '\', or the    */
/*  whole of it if none.                                                     */
/*                                                                           */
/*****************************************************************************/

static char *COIFileStem(char *file_name)
{
  char *p;  int len;
  len = strlen(file_name);
  for( p = &file_name[len-1]; p != file_name && *p != '/' && *p != '\\'; p-- );
  if( *p == '/' || *p == '\\' )  p++;
  return p;
}


/*****************************************************************************/
/*                                                                           */
/*  char *COIInstanceIdNormalize(char *ins_id, HA_ARENA a)                   */
/*                                                                           */
/*  Return a normalized version of ins_id, replacing long by Long, etc.      */
/*                                                                           */
/*****************************************************************************/

static char *COIInstanceIdNormalize(char *ins_id, HA_ARENA a)
{
  char *res, *p;
  p = strstr(ins_id, "Shift-DATA");
  if( p == NULL )
    res = HnStringMake(a, "COI-%s", ins_id);
  else
  {
    *p = '\0';
    res = HnStringMake(a, "COI-%s.%s", ins_id, &p[strlen("Shift-DATA")]);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *COIInstanceName(char *str, HA_ARENA a)                             */
/*                                                                           */
/*  Find the name of an instance within str, and return that name in         */
/*  arena memory.                                                            */
/*                                                                           */
/*****************************************************************************/

static char *COIInstanceName(char *str, HA_ARENA a)
{
  char *tmp, *p;
  tmp = HnStringCopy(str, a);
  p = strstr(tmp, ".txt");
  if( p != NULL && p != tmp )
    *p = '\0';
  p = strstr(tmp, ".ros");
  if( p != NULL && p != tmp )
    *p = '\0';
  p = strstr(tmp, ".mros");
  if( p != NULL && p != tmp )
    *p = '\0';
  return COIInstanceIdNormalize(COIFileStem(tmp), a);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE COIConvertInstance(INSTANCE_MODEL ins_model,                */
/*    char *instance_file_name, HA_ARENA_SET as)                             */
/*                                                                           */
/*  Read instance_file_name, a file in the format of the first international */
/*  nurse rostering competition, and return the resulting instance.          */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE COIConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  /* NRC_INSTANCE_METADATA md; */  KML_ERROR ke;  FILE *fp;  COI_INFO ii;
  char *ins_id, *str;  HA_ARENA a;  NRC_INSTANCE res;
  if( DEBUG6 )
    fprintf(stderr, "[ COIConvertInstance(-, %s)\n", instance_file_name);

  /* make an instance info object, initially more or less empty */
  a = HaArenaMake(as);
  HaMake(ii, a);
  ii->arena = a;
  ii->file_name = instance_file_name;
  ii->root_elt = NULL;;
  ii->ins = NULL;
  ii->worker_set_tree = NULL;
  HaArrayInit(ii->pattern_weight, a);
  HaArrayInit(ii->pattern_starting_day_set, a);
  if( DEBUG14 )
    fprintf(stderr, "  COIConvertInstance at (1)\n");

  /* open and read an XML instance file, creating ii->root_elt */
  fp = fopen(instance_file_name, "r");
  if( fp == NULL )
    FatalError(instance_file_name, 0, "cannot read file \"%s\"",
      instance_file_name);
  if( !KmlReadFile(fp, NULL, &ii->root_elt, &ke, a) )
    KmlFatalError(ke, instance_file_name);
  fclose(fp);
  if( DEBUG14 )
    fprintf(stderr, "  COIConvertInstance at (2)\n");

  if( setjmp(ii->env) == 0 )
  {
    /* make sure the root tag and its children are all present and correct */
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (3)\n");
    if( !str_eq(KmlLabel(ii->root_elt), "SchedulingPeriod") )
      OmitInstance(ii, ii->root_elt,
	"root tag %s where SchedulingPeriod expected", KmlLabel(ii->root_elt));
    if( !KmlCheck(ii->root_elt, "+ID +OrganisationID "
	"+xmlns:xsi +xsi:noNamespaceSchemaLocation : "
	"$StartDate $EndDate +Skills +SkillGroups +ShiftTypes +ShiftGroups "
	"+Contracts +Employees +CoverRequirements +CoverWeights +DayOffRequests"
	" +DayOnRequests +ShiftOffRequests +ShiftOnRequests +FixedAssignments",
	&ke) )
      KmlFatalError(ke, ii->file_name);

    /* make instance id */
    if( KmlContainsAttribute(ii->root_elt, "ID", &str) )
      ins_id = COIInstanceName(str, a);
    else
      ins_id = COIInstanceName(instance_file_name, a);
    /* *** rubbishy old version
    if( KmlContainsAttribute(ii->root_elt, "ID", &str) )
      ins_id = COIInstanceIdNormalize(MStringCopy(str));
    else
    {
      ins_id = MStringCopy(instance_file_name);
      p = strstr(ins_id, ".ros");
      if( p != NULL && p != ins_id )
	*p = '\0';
      ins_id = COIInstanceIdNormalize(ins_id);
    }
    *** */
    if( !KmlContainsAttribute(ii->root_elt, "OrganisationId", &str) )
      str = NULL;

    /* make instance */
    if( DEBUG12 )
      fprintf(stderr, "  NrcInstanceMake(\"%s\", -, -, -)\n", ins_id);
    ii->ins = NrcInstanceMakeBegin(ins_id, /* "Nurses", */ "Nurse", as);
    /* ***
    penalty = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    md = InstanceModelMetaData(ins_model, ins_id, NULL, NULL, NULL, str, NULL);
    *** */
    NrcInstanceSetMetaData(ii->ins, ins_id, NULL, NULL, NULL, str, NULL);
    ii->empty_sts = NrcShiftTypeSetMake(ii->ins, NULL);
    HaArrayInit(ii->shift_type_auto_allocate, a);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (4)\n");

    /* initialize specials table */
    HnTableInit(ii->specials_table, a);
    HnTableAdd(ii->specials_table, "COI-ERMGH-1CW",
      &CoiERMGHSpecialMaxOneBusyConsecutiveWeekends);
    HnTableAdd(ii->specials_table, "COI-ERMGH-2CW",
      &CoiERMGHSpecialMaxTwoBusyConsecutiveWeekends);
    HnTableAdd(ii->specials_table, "COI-ORTEC02-3BW",
      &CoiORTEC02SpecialMaxThreeBusyWeekends);
    HnTableAdd(ii->specials_table, "COI-HED01-COND",
      &CoiHED01SpecialConditionals);
    HnTableAdd(ii->specials_table, "COI-ERMGH-CW",
      &CoiERMGHSpecialCompleteWeekends);
    HnTableAdd(ii->specials_table, "COI-ERMGH-1CFW",
      &CoiERMGHSpecialMaxOneFreeConsecutiveWeekends);
    HnTableAdd(ii->specials_table, "COI-GPost-CW",
      &CoiERMGHSpecialCompleteWeekends);
    HnTableAdd(ii->specials_table, "COI-ERRVH-CW",
      &CoiERMGHSpecialCompleteWeekends);
    HnTableAdd(ii->specials_table, "COI-Ozkarahan-1BW",
      &CoiOzkarahanSpecialNoBusyWeekend);
    HnTableAdd(ii->specials_table, "COI-Ikegami-MAX6",
      &CoiIkegamiSpecialMaxSixDaysBetweenDayShifts);
    HnTableAdd(ii->specials_table, "COI-Ikegami-MIN6",
      &CoiIkegamiSpecialMinSixDaysBetweenNightShifts);
    /* ***
    HnTableAdd(ii->specials_table, "COI-GPost-2CF",
      &CoiPostSpecialMinTwoFreeConsecutiveDays);
    *** */
    HnTableAdd(ii->specials_table, "COI-GPost-2CBW",
      &CoiPostSpecialMaxTwoConsecutiveBusyWeekends);
    /* ***
    HnTableAdd(ii->specials_table, "COI-GPost-2BN",
      &CoiPostSpecialMinTwoConsecutiveNightShifts);
    HnTableAdd(ii->specials_table, "COI-GPost-4B",
      &CoiPostSpecialMinFourConsecutiveShifts);
    HnTableAdd(ii->specials_table, "COI-GPost-2B",
      &CoiPostSpecialMinTwoConsecutiveShifts);
    *** */

    /* add elements producing times and time groups */
    AddShiftTypes(ii);
    AddStartDateAndEndDate(ii);
    AddShiftGroups(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (5)\n");

    /* add elements producing resources and resource groups */
    AddSkills(ii);
    AddContracts(ii);
    AddEmployees(ii);
    AddSkillGroups(ii);  /* must come after AddEmployees */
    /* AddSkillWorkerSetTree(ii); */

    /* add elements producing events and cover constraints */
    AddCoverWeightsAndCoverRequirements(ii);

    /* add elements producing resource constraints (contractual) */
    AddContractConstraints(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6)\n");

    /* add elements producing resource constraints (individual) */
    /* AddPatterns(ii); */
    /* AddAlternativeSkillCategories(ii); */
    AddDayOffRequests(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6a)\n");
    AddDayOnRequests(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6b)\n");
    AddShiftOffRequests(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6c)\n");
    AddShiftOnRequests(ii);
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6d)\n");
    AddFixedAssignments(ii);
    NrcInstanceMakeEnd(ii->ins);
    res = ii->ins;
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (6e)\n");
    HaArenaDelete(a);
    if( DEBUG6 )
      fprintf(stderr, "] COIConvertInstance returning (success)\n");
    return res;
  }
  else
  {
    /* failure to convert ins */
    if( DEBUG14 )
      fprintf(stderr, "  COIConvertInstance at (7)\n");
    HaArenaDelete(a);
    if( DEBUG6 )
      fprintf(stderr, "] COIConvertInstance returning (failure)\n");
    return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int COIExtractPenaltyFromFileName(char *file_name)                       */
/*                                                                           */
/*  Extract the penalty value from a file name like                          */
/*  Instance19.Solution.4032_1.roster or Instance19.Solution.4032.roster.    */
/*                                                                           */
/*****************************************************************************/

static int COIExtractPenaltyFromFileName(char *file_name)
{
  char *p;  int res;
  if( DEBUG4 )
    fprintf(stderr, "[ COIExtractPenaltyFromFileName(%s)\n", file_name);
  p = strstr(file_name, ".ros");
  if( p == NULL )
    FatalError(file_name, -1, "cannot extract penalty from soln file name");
  for( p--;  p >= file_name && (is_digit(*p) || *p == '_');  p-- );
  if( sscanf(p+1, "%d", &res) != 1 )
    FatalError(file_name, -1, "cannot extract penalty from soln file name");
  if( DEBUG4 )
    fprintf(stderr, "] COIExtractPenaltyFromFileName returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  SHIFT_OVERLOAD                                                           */
/*                                                                           */
/*****************************************************************************/

typedef struct shift_overload_rec {
  NRC_SHIFT	shift;
  int		overload;
} *SHIFT_OVERLOAD;

typedef HA_ARRAY(SHIFT_OVERLOAD) ARRAY_SHIFT_OVERLOAD;


/*****************************************************************************/
/*                                                                           */
/*  void ShiftOverloadAdd(ARRAY_SHIFT_OVERLOAD *overloads, NRC_SHIFT shift)  */
/*                                                                           */
/*  Add a shift overload to *overloads.                                      */
/*                                                                           */
/*****************************************************************************/

static void ShiftOverloadAdd(ARRAY_SHIFT_OVERLOAD *overloads, NRC_SHIFT shift,
  HA_ARENA a)
{
  SHIFT_OVERLOAD so;  int i;

  /* find or make shift overload record */
  HaArrayForEach(*overloads, so, i)
    if( so->shift == shift )
    {
      so->overload++;
      return;
    }

  /* none, so make one */
  HaMake(so, a);
  so->shift = shift;
  so->overload = 1;
  HaArrayAddLast(*overloads, so);
}


/*****************************************************************************/
/*                                                                           */
/*  SHIFT_OVERLOAD ShiftOverloadMax(ARRAY_SHIFT_OVERLOAD *overloads)         */
/*                                                                           */
/*  Return the most overloaded shift, or NULL if none.                       */
/*                                                                           */
/*****************************************************************************/

static SHIFT_OVERLOAD ShiftOverloadMax(ARRAY_SHIFT_OVERLOAD *overloads)
{
  SHIFT_OVERLOAD so, res;  int i;
  res = NULL;
  HaArrayForEach(*overloads, so, i)
    if( res == NULL || so->overload > res->overload )
      res = so;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void ShiftOverloadDebug(ARRAY_SHIFT_OVERLOAD *overloads, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Display *overloads onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void ShiftOverloadDebug(ARRAY_SHIFT_OVERLOAD *overloads, int indent,
  FILE *fp)
{
  SHIFT_OVERLOAD so;  int i;
  HaArrayForEach(*overloads, so, i)
    fprintf(fp, "%*sShiftOverload(%s:%s, max %d, over %d)\n",
      indent, "", NrcDayShortName(NrcShiftDay(so->shift)),
      NrcShiftTypeName(NrcShiftType(so->shift)),
      NrcShiftDemandCount(so->shift), so->overload);
}


/*****************************************************************************/
/*                                                                           */
/*  COI_RUNTIME_ELT                                                          */
/*                                                                           */
/*****************************************************************************/

typedef enum runtime_elt {
  COI_RUNTIME_FLOAT,
  COI_RUNTIME_INT,
  COI_RUNTIME_OTHER,
  COI_RUNTIME_NONE
} COI_RUNTIME_TYPE;

typedef struct runtime_elt_rec {
  COI_RUNTIME_TYPE	type;
  char			*str_val;
  int			int_val;
  float			float_val;
} COI_RUNTIME_ELT;


/*****************************************************************************/
/*                                                                           */
/*  COI_RUNTIME_ELT COIRunTimeNone(void)                                     */
/*                                                                           */
/*  Return the special "none" value of COI_RUNTIME_ELT.                      */
/*                                                                           */
/*****************************************************************************/

static COI_RUNTIME_ELT COIRunTimeNone(void)
{
  COI_RUNTIME_ELT res;
  res.type = COI_RUNTIME_NONE;
  res.str_val = NULL;
  res.int_val = -1;
  res.float_val = -1.0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void COIRunTimeDebug(COI_RUNTIME_ELT elt, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug print of elt.                                                      */
/*                                                                           */
/*****************************************************************************/

static void COIRunTimeDebug(COI_RUNTIME_ELT elt, int indent, FILE *fp)
{
  switch( elt.type )
  {
    case COI_RUNTIME_FLOAT:

      fprintf(fp, "%*sRunTime(%f)\n", indent, "", elt.float_val);
      break;

    case COI_RUNTIME_INT:

      fprintf(fp, "%*sRunTime(%d)\n", indent, "", elt.int_val);
      break;

    case COI_RUNTIME_OTHER:

      fprintf(fp, "%*sRunTime(\"%s\")\n", indent, "", elt.str_val);
      break;

    case COI_RUNTIME_NONE:

      fprintf(fp, "%*sRunTime(-)\n", indent, "");
      break;

    default:

      fprintf(fp, "%*sRunTime(?)\n", indent, "");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  COI_RUNTIME_TYPE COIRunTimeNext(char **p, char **val)                    */
/*                                                                           */
/*  Set **val to the next element of *p, move *p past *val, and return       */
/*  the type of *val.                                                        */
/*                                                                           */
/*****************************************************************************/

static COI_RUNTIME_ELT COIRunTimeNext(char **str)
{
  char *p, *q;  COI_RUNTIME_ELT res;

  /* initialize res with default values */
  res = COIRunTimeNone();

  /* skip initial white space */
  p = *str;
  while( *p == ' ' || *p == '\t' )
    p++;

  /* return if at end */
  if( *p == '\0' )
  {
    *str = p;
    return res;
  }
  q = p;

  /* look for an int or float */
  if( is_digit(*p) )
  {
    do { p++; } while( is_digit(*p) );
    if( *p == '.' )
    {
      do { p++; } while( is_digit(*p) );
      *str = p;
      sscanf(q, "%f", &res.float_val);
      res.type = COI_RUNTIME_FLOAT;
      return res;
    }
    else
    {
      *str = p;
      sscanf(q, "%d", &res.int_val);
      res.type = COI_RUNTIME_INT;
      return res;
    }
  }
  else
  {
    while( *p != ' ' && *p != '\t' && *p != '\0' )
      p++;
    if( *p == '\0' )
      *str = p;
    else
    {
      *p = '\0';
      *str = p + 1;
    }
    res.str_val = q;
    res.type = COI_RUNTIME_OTHER;
    return res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DropLastIfPresent(char *str, char ch)                               */
/*                                                                           */
/*  Drop ch from the end of str, if it ends it.                              */
/*                                                                           */
/*****************************************************************************/

static void DropLastIfPresent(char *str, char ch)
{
  int len;
  len = strlen(str);
  if( len > 0 && str[len - 1] == ch )
    str[len - 1] = '\0';
}


/*****************************************************************************/
/*                                                                           */
/*  void COISetCpuTime(NRC_SOLN soln, char *soln_file_name, KML_ELT time_elt)*/
/*                                                                           */
/*  Set the CPU time attribute of soln based on the value of time_elt,       */
/*  which could look like any of these examples:                             */
/*                                                                           */
/*    "28.3 seconds"                                                         */
/*    "1 second"                                                             */
/*    "10 seconds"                                                           */
/*    "1 minute, 36 seconds"                                                 */
/*    "10 minutes"                                                           */
/*    "4 hours, 57 minutes, 20 seconds (10 runs)"                            */
/*    "1 minute, 45 seconds (10 runs)"                                       */
/*    "49 minutes, 53 seconds (445 runs)"                                    */
/*    "6 hours, 26 minutes, 19 seconds (5000 runs)"                          */
/*    "13 seconds"                                                           */
/*    "693 minutes, 46 seconds"                                              */
/*    "47 min"                                                               */
/*    "902 minutes, 22 seconds"                                              */
/*    "47 min"                                                               */
/*    "6 hours"                                                              */
/*    "5783 minutes, 1 second"                                               */
/*    "693 minutes, 46 seconds"                                              */
/*    "47 min"                                                               */
/*    "902 minutes, 22 seconds"                                              */
/*    "47 min"                                                               */
/*                                                                           */
/*****************************************************************************/

static void COISetCpuTime(NRC_SOLN soln, char *soln_file_name,
  KML_ELT time_elt, HA_ARENA a)
{
  float secs;  char *s;  COI_RUNTIME_ELT elt, prev_elt;
  if( DEBUG7 )
    fprintf(stderr, "[ COISetCpuTime(soln, -, \"%s\")\n", KmlText(time_elt));
  secs = 0.0;
  s = HnStringCopy(KmlText(time_elt), a);
  prev_elt = COIRunTimeNone();
  while( true )
  {
    elt = COIRunTimeNext(&s);
    if( DEBUG7 )
      COIRunTimeDebug(elt, 4, stderr);
    switch( elt.type )
    {
      case COI_RUNTIME_FLOAT:

	/* nothing to do here */
	break;

      case COI_RUNTIME_INT:

	/* nothing to do here */
	break;

      case COI_RUNTIME_NONE:

	if( DEBUG7 )
	  fprintf(stderr, "] COISetCpuTime returning (%f)\n", secs);
	if( secs > 0.0 )
	  NrcSolnSetRunningTime(soln, secs);
	return;

      case COI_RUNTIME_OTHER:

        DropLastIfPresent(elt.str_val, ',');
        DropLastIfPresent(elt.str_val, 's');
	if( str_eq(elt.str_val, "millisecond") )
	{
	  if( prev_elt.type == COI_RUNTIME_INT )
	    secs += (float) prev_elt.int_val / 1000.0;
	  else if( prev_elt.type == COI_RUNTIME_FLOAT )
	    secs += prev_elt.float_val / 1000.0;
	  else
	  {
	    KmlEltWarning(time_elt, soln_file_name,
	      "cannot parse CpuTime \"%s\" (no amount before milliseconds)",
	      KmlText(time_elt));
	    return;
	  }
	}
	else if( str_eq(elt.str_val, "second") )
	{
	  if( prev_elt.type == COI_RUNTIME_INT )
	    secs += (float) prev_elt.int_val;
	  else if( prev_elt.type == COI_RUNTIME_FLOAT )
	    secs += prev_elt.float_val;
	  else
	  {
	    KmlEltWarning(time_elt, soln_file_name,
	      "cannot parse CpuTime \"%s\" (no amount before seconds)",
	      KmlText(time_elt));
	    return;
	  }
	}
	else if( str_eq(elt.str_val, "minute") || str_eq(elt.str_val, "min") )
	{
	  if( prev_elt.type == COI_RUNTIME_INT )
	    secs += (float) prev_elt.int_val * 60;
	  else if( prev_elt.type == COI_RUNTIME_FLOAT )
	    secs += prev_elt.float_val * 60;
	  else
	  {
	    KmlEltWarning(time_elt, soln_file_name,
	      "cannot parse CpuTime \"%s\" (no amount before minutes)",
	      KmlText(time_elt));
	    return;
	  }
	}
	else if( str_eq(elt.str_val, "hour") )
	{
	  if( prev_elt.type == COI_RUNTIME_INT )
	    secs += (float) prev_elt.int_val * 60 * 60;
	  else if( prev_elt.type == COI_RUNTIME_FLOAT )
	    secs += prev_elt.float_val * 60 * 60;
	  else
	  {
	    KmlEltWarning(time_elt, soln_file_name,
	      "cannot parse CpuTime \"%s\" (no amount before hours)",
	      KmlText(time_elt));
	    return;
	  }
	}
	else if( str_eq(elt.str_val, "day") )
	{
	  if( prev_elt.type == COI_RUNTIME_INT )
	    secs += (float) prev_elt.int_val * 60 * 60 * 24;
	  else if( prev_elt.type == COI_RUNTIME_FLOAT )
	    secs += prev_elt.float_val * 60 * 60 * 24;
	  else
	  {
	    KmlEltWarning(time_elt, soln_file_name,
	      "cannot parse CpuTime \"%s\" (no amount before days)",
	      KmlText(time_elt));
	    return;
	  }
	}
	else if(elt.str_val[0]=='(' || elt.str_val[strlen(elt.str_val)-1]==')')
	{
	  /* ignore this */
	}
	else if( str_eq(elt.str_val, "-") || str_eq(elt.str_val, "=") )
	{
	  /* ignore this */
	}
	else if( str_eq(elt.str_val, "Time") || str_eq(elt.str_val, "Elapsed") )
	{
	  /* ignore this */
	}
	else
	{
	  KmlEltWarning(time_elt, soln_file_name,
	    "cannot parse CpuTime \"%s\" (fragment \"%s\")",
	    KmlText(time_elt), elt.str_val);
	}
	break;
    }
    prev_elt = elt;
  }
  /* unreachable here */
}


/*****************************************************************************/
/*                                                                           */
/*  bool UnassignedDemandHasSkill(NRC_DEMAND d, NRC_WORKER w)                */
/*                                                                           */
/*  Return true if d prefers w.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool UnassignedDemandHasSkill(NRC_DEMAND d, NRC_WORKER w)
{
  NRC_WORKER_SET ws;  NRC_PENALTY p;  int i;
  NRC_PENALIZER_TYPE pt;  NRC_PENALTY_TYPE ptype;
  for( i = 0;  i < NrcDemandPenalizerCount(d);  i++ )
  {
    NrcDemandPenalizer(d, i, &pt, &ws, &ptype, &p);
    if( pt == NRC_PENALIZER_NOT_WORKER_SET && NrcWorkerSetContainsWorker(ws,w) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool ShiftHasUnassignedDemandWithSkill(NRC_SOLN soln, NRC_SHIFT s,       */
/*    NRC_WORKER w, int *index)                                              */
/*                                                                           */
/*  If s has an unassigned demand with a non-NULL skill suited to w,         */
/*  then set *index to its index and return true.  Otherwise return false.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool ShiftHasUnassignedDemandWithSkill(NRC_SOLN soln, NRC_SHIFT s,
  NRC_WORKER w, int *index)
{
  int i;  NRC_DEMAND d;   ** NRC_WORKER_SET ws; **
  for( i = 0;  i < NrcShiftDemandCount(s);  i++ )
  {
    d = NrcShiftDemand(s, i);
    ** ***
    ws = NrcDemandPreferredWorkerSet(d);
    if( NrcSolnAssignment(soln, s, i) == NULL && ws != NULL &&
	NrcWorkerSetContainsWorker(ws, w) )
    *** **
    if( NrcSolnAssignment(soln,s,i) == NULL && UnassignedDemandHasSkill(d, w) )
    {
      *index = i;
      return true;
    }
  }
  *index = -1;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool ShiftHasUnassignedDemand(NRC_SOLN soln, NRC_SHIFT s,                */
/*    NRC_WORKER w, int *index)                                              */
/*                                                                           */
/*  If s has an unassigned demand, set *index to the index of the first best */
/*  unassigned demand to assign w to, and return true.  Else return false.   */
/*                                                                           */
/*****************************************************************************/

static bool ShiftHasUnassignedDemand(NRC_SOLN soln, NRC_SHIFT s,
  NRC_WORKER w, int *index)
{
  int i, best_i;  NRC_DEMAND d;  NRC_PENALTY p, best_p;
  if( DEBUG16 )
    fprintf(stderr, "[ ShiftHasUnassignedDemand(soln of %s, %s, %s, -)\n",
      NrcInstanceId(NrcSolnInstance(soln)), NrcShiftName(s), NrcWorkerName(w));
  best_i = -1, best_p = NULL;
  for( i = 0;  i < NrcShiftDemandCount(s);  i++ )
    if( NrcSolnAssignment(soln, s, i) == NULL )
    {
      d = NrcShiftDemand(s, i);
      p = NrcDemandWorkerPenalty(d, w);
      if( DEBUG16 )
	fprintf(stderr, "  DemandWorkerPenalty(\"%s\", %s) = %s%s\n",
	  NrcDemandName(d), NrcWorkerName(w), NrcPenaltyShow(p),
          best_p == NULL || NrcPenaltyLessThan(p, best_p) ? " (new best)" : "");
      if( best_p == NULL || NrcPenaltyLessThan(p, best_p) )
	best_i = i, best_p = p;
    }
  if( DEBUG16 )
    fprintf(stderr, "] ShiftHasUnassignedDemand returning %s, *index = %d\n",
      best_p != NULL ? "true" : "false", *index);
  return *index = best_i, best_p != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ShiftHasUnassignedDemandWithoutSkill(NRC_SOLN soln, NRC_SHIFT s,    */
/*    int *index)                                                            */
/*                                                                           */
/*  If s has any unassigned demand at all, set *index to its index and       */
/*  return true.  Otherwise return false.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool ShiftHasUnassignedDemandWithoutSkill(NRC_SOLN soln, NRC_SHIFT s,
  int *index)
{
  int i;
  for( i = 0;  i < NrcShiftDemandCount(s);  i++ )
  {
    if( NrcSolnAssignment(soln, s, i) == NULL )
    {
      *index = i;
      return true;
    }
  }
  *index = -1;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void COIConvertSoln(char *soln_file_name, SOLN_MODEL soln_model,         */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)                */
/*                                                                           */
/*  Convert soln_file_name into a solution of ins and add it to a solution   */
/*  group of archive, which is known to have at least one solution group.    */
/*                                                                           */
/*****************************************************************************/

void COIConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  FILE *fp;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;  char *ins_id, *id;  NRC_DAY d;
  NRC_SOLN soln;  KML_ERROR ke;  NRC_WORKER w;  int i, j, day_num, index;
  KML_ELT soln_elt, employee_elt, assign_elt, day_elt, shift_type_elt;
  KML_ELT ins_elt, time_elt;  HA_ARENA a;
  ARRAY_SHIFT_OVERLOAD overloads;  SHIFT_OVERLOAD max_so;
  if( DEBUG2 )
    fprintf(stderr, "[ COIConvertSoln(%s, %s, archive)\n",
      soln_file_name, ins != NULL ? NrcInstanceId(ins) : "NULL");

  /* open the file and return if can't */
  fp = fopen(soln_file_name, "r");
  if( fp == NULL )
  {
    Warning(soln_file_name, -1, "ignoring solution file (cannot open)");
    if( DEBUG2 )
      fprintf(stderr, "] COIConvertSoln failed (cannot open file)\n");
    return;
  }

  /* read XML from the file, and exit if can't */
  a = HaArenaMake(as);
  if( !KmlReadFile(fp, NULL, &soln_elt, &ke, a) )
  {
    KmlWarning(ke, soln_file_name);
    fclose(fp);
    if( DEBUG2 )
      fprintf(stderr, "] COIConvertSoln failed (cannot read XML from file)\n");
    return;
  }
  fclose(fp);

  /* make sure the root tags and their children are all present and correct */
  /* not all solutions contain <Penalty>, but all file names have its value */
  if( strcmp(KmlLabel(soln_elt), "Roster") != 0 )
    KmlEltFatalError(soln_elt, soln_file_name,
      "root tag %s where Roster expected", KmlLabel(soln_elt));
  if( !KmlCheck(soln_elt, "+xmlns:xsi +xsi:noNamespaceSchemaLocation : "
	"$SchedulingPeriodFile +#Penalty +$DateFound +$FoundBy +$System "
	"+$CPU +$Algorithm +$CpuTime *Employee +Violations", &ke) )
  {
    if( DEBUG1 )
      fprintf(stderr, "COIConvertSoln failing at 1\n");
    KmlFatalError(ke, soln_file_name);
  }

  /* work out which instance this is for */
  KmlContainsChild(soln_elt, "SchedulingPeriodFile", &ins_elt);
  ins_id = COIInstanceName(KmlText(ins_elt), a);
  if( ins == NULL )
  {
    if( !NrcArchiveRetrieveInstance(archive, ins_id, &ins) )
    {
      KmlEltWarning(ins_elt, soln_file_name,
	"ignoring solution file; instance %s unknown", ins_id);
      HaArenaDelete(a);
      if( DEBUG2 )
	fprintf(stderr, "] COIConvertSoln failed (cannot find instance)\n");
      return;
    }
    if( DEBUG2 )
      fprintf(stderr, "  retrieved instance %s\n", ins_id);
  }
  else
  {
    if( strcmp(NrcInstanceId(ins), ins_id) != 0 )
      KmlEltFatalError(ins_elt, soln_file_name,
	"instance id %s disagrees with id %s of instance passed after -s flag",
	ins_id, NrcInstanceId(ins));
  }

  /* make a soln object and set the description to include the reported cost */
  /* extract the penalty from the file name */
  soln = NrcSolnMake(ins, as);
  NrcSolnSetDescription(soln, HnStringMake(a, "Reported cost %d",
    COIExtractPenaltyFromFileName(soln_file_name)));

  /* load the assignments */
  HaArrayInit(overloads, a);
  for( i = 1;  i < KmlChildCount(soln_elt);  i++ )
  {
    employee_elt = KmlChild(soln_elt, i);
    if( strcmp(KmlLabel(employee_elt), "Employee") != 0 )
      continue;
    if( !KmlCheck(employee_elt, "ID : *Assign", &ke) )
      KmlFatalError(ke, soln_file_name);

    /* get the nurse */
    id = KmlAttributeValue(employee_elt, 0);
    /* id = TidyNurseIdOrName(KmlAttributeValue(employee_elt, 0), a); */
    if( !NrcInstanceStaffingRetrieveWorker(ins, id, &w) )
      KmlEltFatalError(employee_elt, soln_file_name,
	"unknown Employee %s (originally %s)", id,
	KmlAttributeValue(employee_elt, 0));

    /* get and make the assignments */
    if( DEBUG2 )
      fprintf(stderr, "  making %d assignments for employee %s\n",
        KmlChildCount(employee_elt), id);
    for( j = 0;  j < KmlChildCount(employee_elt);  j++ )
    {
      assign_elt = KmlChild(employee_elt, j);
      if( !KmlCheck(assign_elt, ": #Day $Shift", &ke) )
	KmlFatalError(ke, soln_file_name);

      /* get the day */
      day_elt = KmlChild(assign_elt, 0);
      if( sscanf(KmlText(day_elt), "%d", &day_num) != 1 )
	KmlEltFatalError(day_elt, soln_file_name, "Day %s is not an integer",
	  KmlText(day_elt));
      if( day_num < 0 || day_num >= NrcInstanceCycleDayCount(ins) )
	KmlEltFatalError(day_elt, soln_file_name, "Day %s out of range",
	  KmlText(day_elt));
      d = NrcInstanceCycleDay(ins, day_num);

      /* get the shift type */
      shift_type_elt = KmlChild(assign_elt, 1);
      if( !NrcInstanceRetrieveShiftType(ins, KmlText(shift_type_elt), &st) )
	KmlEltFatalError(shift_type_elt, soln_file_name, "unknown Shift %s",
	  KmlText(shift_type_elt));

      /* get the shift, find a suitable demand in it, and assign */
      s = NrcDayShiftFromShiftType(d, st);
      if( ShiftHasUnassignedDemand(soln, s, w, &index) )
	NrcSolnAddAssignment(soln, s, index, w);
      else
        ShiftOverloadAdd(&overloads, s, a);
      /* ***
      if( ShiftHasUnassignedDemandWithSkill(soln, s, w, &index) )
	NrcSolnAddAssignment(soln, s, index, w);
      else if( ShiftHasUnassignedDemandWithoutSkill(soln, s, &index) )
	NrcSolnAddAssignment(soln, s, index, w);
      else
        ShiftOverloadAdd(&overloads, s, a);
      *** */
    }
  }

  /* add description */
  NrcSolnSetDescription(soln,
    HnStringMake(a, "Converted from file %s.", COIFileStem(soln_file_name)));

  /* add running time, if available */
  if( KmlContainsChild(soln_elt, "CpuTime", &time_elt) )
    COISetCpuTime(soln, soln_file_name, time_elt, a);

  /* fail with debug print if overloads */
  if( HaArrayCount(overloads) > 0 )
  {
    max_so = ShiftOverloadMax(&overloads);
    Warning(soln_file_name, -1, "ignoring solution (overloaded shifts, "
      "worst is %s:%s with demand %d and overload %d)",
      NrcDayShortName(NrcShiftDay(max_so->shift)),
      NrcShiftTypeName(NrcShiftType(max_so->shift)),
      NrcShiftDemandCount(max_so->shift), max_so->overload);
    HaArenaDelete(a);
    if( DEBUG2 )
    {
      ShiftOverloadDebug(&overloads, 2, stderr);
      fprintf(stderr, "] COIConvertSoln failed (shift overload)\n");
    }
    return;
  }

  /* add soln */
  SolnModelAddSoln(soln_model, archive, "", 0.0, soln);
  if( DEBUG8 )
    Warning(soln_file_name, -1, "including solution");
  /* KmlFree(soln_elt); */
  HaArenaDelete(a);
  if( DEBUG2 )
    fprintf(stderr, "] COIConvertSoln returning\n");
}


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
/*  FILE:         cq14.c                                                     */
/*  MODULE:       ConvertCQ (Curtois and Qu 2014 format)                     */
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
#define DEBUG12 0


/*****************************************************************************/
/*                                                                           */
/*   Summary of constraints                                                  */
/*   ----------------------                                                  */
/*                                                                           */
/*   Names used in doc              Names used here                  L  W    */
/*   ----------------------------------------------------------------------  */
/*   1.  One shift per day          MaxShiftsPerDay                  -  -    */
/*   2.  Shift rotation             UnwantedPattern                  -  -    */
/*   3.  Max shifts of each type    MaxSameShiftDays                 S  -    */
/*   4.  Min and max work time      MaxBusyMinutes, MinBusyMinutes   S  -    */
/*   5.  Max consecutive shifts     MaxConsecutiveBusyDays           S  -    */
/*   6.  Min consecutive shifts     MinConsecutiveBusyDays           S  -    */
/*   7.  Min consecutive days off   MinConsecutiveFreeDays           S  -    */
/*   8.  Max number of weekends     MaxBusyWeekends (*)              S  -    */
/*   9.  Days off                                                    -  -    */
/*   10. Cover requirements                                          R  R    */
/*       Shift on requests                                           -  R    */
/*       Shift off requests                                          -  R    */
/*   ----------------------------------------------------------------------  */
/*                                                                           */
/*   The L column indicates where each limit comes from:                     */
/*                                                                           */
/*     -  Trivial limit (1, pattern length - 1, etc.)                        */
/*     S  Limit is taken from a line describing a worker in SECTION_STAFF    */
/*     R  Limit accompanies the requirement or request                       */
/*                                                                           */
/*   The W column indicates where each weight comes from:                    */
/*                                                                           */
/*     -  Hard constraint, no weight (given weight 1 here)                   */
/*     R  Weight accompanies the requirement or request                      */
/*                                                                           */
/*   (*) "All instances start on a Monday and the planning horizon h is      */
/*       always a whole number of weeks (h mod 7 = 0)."                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  CQ_FACTOR - one factor of one line, e.g. "D=14"                          */
/*                                                                           */
/*****************************************************************************/

typedef struct cq_factor_rec {
  char			*val1;
  char			*val2;		/* optional */
} *CQ_FACTOR;


/*****************************************************************************/
/*                                                                           */
/*  CQ_TERM - one term of one line, e.g. "D=14|F=15"                         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(CQ_FACTOR) ARRAY_CQ_FACTOR;

typedef struct cq_term_rec {
  ARRAY_CQ_FACTOR	factors;
} *CQ_TERM;


/*****************************************************************************/
/*                                                                           */
/*  CQ_LINE - one line of the file, e.g. "F,D=14|F=15,4320,3360,5,2,2,1"     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(CQ_TERM) ARRAY_CQ_TERM;

typedef struct cq_line_rec {
  int			line_num;	/* line num in file */
  ARRAY_CQ_TERM		terms;
} *CQ_LINE;


/*****************************************************************************/
/*                                                                           */
/*  CQ_SECTION - one section of the cq file, from header to blank line       */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(CQ_LINE) ARRAY_CQ_LINE;

typedef struct cq_section_rec {
  char			*header;
  ARRAY_CQ_LINE		 lines;
} *CQ_SECTION;


/*****************************************************************************/
/*                                                                           */
/*  CQ_FILE - one file in the plain text format of Curtois and Qu            */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(CQ_SECTION) ARRAY_CQ_SECTION;

typedef struct cq_file_rec {
  char			*file_name;
  ARRAY_CQ_SECTION	sections;
} *CQ_FILE;


/*****************************************************************************/
/*                                                                           */
/*  CQ_INFO - miscellaneous information about a CQ instance                  */
/*                                                                           */
/*****************************************************************************/

typedef struct cq_info_rec {
  HA_ARENA			arena;
  char				*file_name;
  CQ_FILE			cq_file;
  NRC_INSTANCE			ins;
  NRC_SHIFT_SET_SET		weekends;
  NRC_WORKER_SET_SET		preferred_worker_sets;
} *CQ_INFO;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading CQ files"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool LineIsBlank(char *line)                                             */
/*                                                                           */
/*  Return true if line is blank.                                            */
/*                                                                           */
/*****************************************************************************/

static bool LineIsBlank(char *line)
{
  char *p;
  for( p = line;  *p != '\0';  p++ )
    if( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *ParseString(char **p, int line_num, HA_ARENA a)                    */
/*                                                                           */
/*  Parse a non-empty string of alphanumeric characters with an optional     */
/*  initial '-', and return it.                                              */
/*                                                                           */
/*****************************************************************************/

char *ParseString(char **p, int line_num, HA_ARENA a)
{
  char *start_p, *res, save_ch;
  HnAssert(is_letter(**p) || is_digit(**p) || **p == '-',
    "ParseString syntax error (line %d)", line_num);
  start_p = *p;
  do
  {
    (*p)++;
  } while( is_letter(**p) || is_digit(**p) );
  save_ch = **p;
  **p = '\0';
  res = HnStringCopy(start_p, a);
  **p = save_ch;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  CQ_FACTOR ParseFactor(char **p, int line_num, HA_ARENA a)                */
/*                                                                           */
/*  Parse one factor and return it.  The syntax is                           */
/*                                                                           */
/*    factor ::= string [ "=" string ]                                       */
/*                                                                           */
/*  where string is a non-empty sequence of alphanumeric characters.         */
/*                                                                           */
/*****************************************************************************/

CQ_FACTOR ParseFactor(char **p, int line_num, HA_ARENA a)
{
  CQ_FACTOR res;

  /* make a factor */
  HaMake(res, a);
  res->val1 = NULL;
  res->val2 = NULL;

  /* parse one or two strings */
  res->val1 = ParseString(p, line_num, a);
  if( **p == '=' )
  {
    (*p)++;
    res->val2 = ParseString(p, line_num, a);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  CQ_TERM ParseTerm(char **p, int line_num, HA_ARENA a)                    */
/*                                                                           */
/*  Parse one term and return it in *res.  The grammar is                    */
/*                                                                           */
/*    term ::= [ factor { "|" factor } ]                                     */
/*                                                                           */
/*****************************************************************************/

CQ_TERM ParseTerm(char **p, int line_num, HA_ARENA a)
{
  CQ_TERM res;

  /* make a term */
  HaMake(res, a);
  HaArrayInit(res->factors, a);

  /* parse the term */
  if( is_letter(**p) || is_digit(**p) || **p == '-' )
  {
    HaArrayAddLast(res->factors, ParseFactor(p, line_num, a));
    while( **p == '|' )
    {
      (*p)++;
      HaArrayAddLast(res->factors, ParseFactor(p, line_num, a));
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  CQ_LINE ParseLine(char *line, int line_num, HA_ARENA a)                  */
/*                                                                           */
/*  Parse non-blank, non-header line and add it to section.  The grammar is  */
/*                                                                           */
/*    line ::= term { "," term }                                             */
/*                                                                           */
/*  There is always at least one term.                                       */
/*                                                                           */
/*****************************************************************************/

static CQ_LINE ParseLine(char *line, int line_num, HA_ARENA a)
{
  CQ_LINE res;  char *p;

  /* make the line */
  HaMake(res, a);
  res->line_num = line_num;
  HaArrayInit(res->terms, a);

  /* parse the terms and add them to line */
  p = line;
  HaArrayAddLast(res->terms, ParseTerm(&p, line_num, a));
  while( *p == ',' )
  {
    p++;
    HaArrayAddLast(res->terms, ParseTerm(&p, line_num, a));
  }
  HnAssert(*p == '\0', "ParseLine syntax error, line %d", line_num);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void CQLineCheck(CQ_INFO ii, CQ_LINE line, int expected_term_count)      */
/*                                                                           */
/*  Check that line has the expected number of terms; fatal error if not.    */
/*                                                                           */
/*****************************************************************************/

static void CQLineCheck(CQ_INFO ii, CQ_LINE line, int expected_term_count)
{
  int count = HaArrayCount(line->terms);
  if( count != expected_term_count )
    FatalError(ii->file_name, line->line_num,
      "line has %d term%s (expected %d)",
      count, count == 1 ? "" : "s", expected_term_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void CQTermCheck(CQ_INFO ii, CQ_LINE line, CQ_TERM term,                 */
/*    int expected_factor_count)                                             */
/*                                                                           */
/*  Check that term of line has the expected number of factors.              */
/*                                                                           */
/*****************************************************************************/

static void CQTermCheck(CQ_INFO ii, CQ_LINE line, CQ_TERM term,
  int expected_factor_count)
{
  int count, pos;
  count = HaArrayCount(term->factors);
  if( count != expected_factor_count )
  {
    if( !HaArrayContains(line->terms, term, &pos) )
      HnAbort("CQTermCheck internal error");
    FatalError(ii->file_name, line->line_num,
      "term %d has %d factor%s (expected %d)", pos + 1,
      count, count == 1 ? "" : "s", expected_factor_count);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CQFactorCheck(CQ_INFO ii, CQ_LINE line, CQ_TERM term,               */
/*    CQ_FACTOR factor, int expected_part_count)                             */
/*                                                                           */
/*  Check that factor of line/term has the expected number of parts.         */
/*                                                                           */
/*****************************************************************************/

static void CQFactorCheck(CQ_INFO ii, CQ_LINE line, CQ_TERM term,
  CQ_FACTOR factor, int expected_part_count)
{
  int count, tpos, fpos;
  count = (factor->val2 == NULL ? 1 : 2);
  if( count != expected_part_count )
  {
    if( !HaArrayContains(line->terms, term, &tpos) )
      HnAbort("CQFactorCheck internal error 1");
    if( !HaArrayContains(term->factors, factor, &fpos) )
      HnAbort("CQFactorCheck internal error 2");
    FatalError(ii->file_name, line->line_num,
      "factor %d of term %d has %d part%s (expected %d)", fpos + 1, tpos + 1,
      count, count == 1 ? "" : "s", expected_part_count);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CQFactorDebug(CQ_FACTOR factor, FILE *fp)                           */
/*                                                                           */
/*  Debug print of factor onto fp.                                           */
/*                                                                           */
/*****************************************************************************/

static void CQFactorDebug(CQ_FACTOR factor, FILE *fp)
{
  if( factor->val1 != NULL )
    fprintf(fp, "%s", factor->val1);
  if( factor->val1 != NULL && factor->val2 != NULL )
    fprintf(fp, "%s", "=");
  if( factor->val2 != NULL )
    fprintf(fp, "%s", factor->val2);
}


/*****************************************************************************/
/*                                                                           */
/*  void CQTermDebug(CQ_TERM term, FILE *fp)                                 */
/*                                                                           */
/*  Debug print of term onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

static void CQTermDebug(CQ_TERM term, FILE *fp)
{
  CQ_FACTOR factor;  int i;
  HaArrayForEach(term->factors, factor, i)
  {
    if( i > 0 )
      fprintf(fp, "|");
    CQFactorDebug(factor, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CQLineDebug(CQ_LINE line, int indent, FILE *fp)                     */
/*                                                                           */
/*  Debug print of line with the given indent.                               */
/*                                                                           */
/*****************************************************************************/

static void CQLineDebug(CQ_LINE line, int indent, FILE *fp)
{
  CQ_TERM term;  int i;
  fprintf(fp, "%*s line %d: ", indent, "", line->line_num);
  HaArrayForEach(line->terms, term, i)
  {
    if( i > 0 )
      fprintf(fp, ",");
    CQTermDebug(term, fp);
  }
  fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void CQSectionDebug(CQ_SECTION section, int indent, FILE *fp)            */
/*                                                                           */
/*  Debug print of section with the given indent.                            */
/*                                                                           */
/*****************************************************************************/

static void CQSectionDebug(CQ_SECTION section, int indent, FILE *fp)
{
  CQ_LINE line;  int i;
  fprintf(fp, "%*s[ CQ Section %s:\n", indent, "", section->header);
  HaArrayForEach(section->lines, line, i)
    CQLineDebug(line, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void CQFileDebug(CQ_FILE cqf, int indent, FILE *fp)                      */
/*                                                                           */
/*  Debug print of cqf with the given indent.                                */
/*                                                                           */
/*****************************************************************************/

static void CQFileDebug(CQ_FILE cqf, int indent, FILE *fp)
{
  CQ_SECTION section;  int i;
  fprintf(fp, "%*s[ CQ File %s:\n", indent, "", cqf->file_name);
  HaArrayForEach(cqf->sections, section, i)
    CQSectionDebug(section, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void StripEndLine(char *buff)                                            */
/*                                                                           */
/*  Strip any trailing newline characters from buff.                         */
/*                                                                           */
/*****************************************************************************/

static void StripEndLine(char *buff)
{
  int pos;
  pos = strlen(buff) - 1;
  while( pos >= 0 && (buff[pos] == '\n' || buff[pos] == '\r') )
    pos--;
  buff[pos + 1] = '\0';
}


/*****************************************************************************/
/*                                                                           */
/*  CQ_FILE CQFileRead(char *file_name)                                      */
/*                                                                           */
/*  Read file_name, which is in CQ-file format, and return it as a           */
/*  CQ_FILE object.                                                          */
/*                                                                           */
/*****************************************************************************/

static CQ_FILE CQFileRead(char *file_name, HA_ARENA a)
{
  char buff[2000];  FILE *fp;  int line_num;  CQ_FILE res;
  CQ_SECTION section;  enum { HEADER_IS_NEXT, ORDINARY_IS_NEXT } state;

  /* open file */
  fp = fopen(file_name, "r");
  if( fp == NULL )
  {
    fprintf(stderr, "cannot open file %s\n", file_name);
    exit(1);
  }

  /* make the cq file object */
  HaMake(res, a);
  res->file_name = file_name;
  HaArrayInit(res->sections, a);

  /* read the lines */
  section = NULL;
  state = HEADER_IS_NEXT;
  for( line_num = 1;  fgets(buff, 2000, fp);  line_num++ )
  {
    /* bail out if did not read whole line */
    if( strlen(buff) > 1990 )
      HnAbort("line too long");

    /* skip comment lines */
    if( buff[0] == '#' )
      continue;

    /* remove final newline characters, if any */
    StripEndLine(buff);
    if( DEBUG6 )
      fprintf(stderr, "  read non-comment line %d (%d chars): %s\n", line_num,
	(int) strlen(buff), buff);

    /* read one header or ordinary line */
    switch( state )
    {
      case HEADER_IS_NEXT:

	if( !LineIsBlank(buff) )
	{
	  HaMake(section, a);
	  section->header = HnStringCopy(buff, a);
	  HaArrayInit(section->lines, a);
	  HaArrayAddLast(res->sections, section);
	  state = ORDINARY_IS_NEXT;
	}
	break;

      case ORDINARY_IS_NEXT:

	if( LineIsBlank(buff) )
	  state = HEADER_IS_NEXT;
	else
	  HaArrayAddLast(section->lines, ParseLine(buff, line_num, a));
	break;
    }
  }
  if( DEBUG1 )
    CQFileDebug(res, 0, stderr);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CQFileRetrieveSection(CQ_FILE cqf, char *header,CQ_SECTION *section)*/
/*                                                                           */
/*  If cqf contains a section with this header, set *section to that         */
/*  section and return true.  Else set *section to NULL and return false.    */
/*                                                                           */
/*****************************************************************************/

static bool CQFileRetrieveSection(CQ_FILE cqf, char *header,
  CQ_SECTION *section)
{
  CQ_SECTION s;  int i;
  HaArrayForEach(cqf->sections, s, i)
    if( strcmp(header, s->header) == 0 )
    {
      *section = s;
      return true;
    }
  *section = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSectionHorizon(CQ_INFO ii)                                       */
/*                                                                           */
/*  Add the SECTION_HORIZON section (the cycle, beginning on a Monday, and   */
/*  the days of the week).                                                   */
/*                                                                           */
/*****************************************************************************/

static void AddSectionHorizon(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  CQ_TERM term;  CQ_FACTOR factor;
  int day_count;
  if( DEBUG4 )
    fprintf(stderr, "[ AddSectionHorizon(ii)\n");
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_HORIZON", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_HORIZON");
  if( HaArrayCount(section->lines) != 1 )
    FatalError(ii->file_name, 0, "SECTION_HORIZON has %d lines (should be 1)",
      HaArrayCount(section->lines));
  line = HaArrayFirst(section->lines);
  CQLineCheck(ii, line, 1);
  term = HaArrayFirst(line->terms);
  CQTermCheck(ii, line, term, 1);
  factor = HaArrayFirst(term->factors);
  CQFactorCheck(ii, line, term, factor, 1);
  if( sscanf(factor->val1, "%d", &day_count) != 1 || day_count <= 0 )
    FatalError(ii->file_name, line->line_num,
      "first factor of SECTION_HORIZON is not a positive integer");
  /* ***
  ii->cycle = DaySetMakeCycleStartingMonday(day_count, &ii->days_of_week);
  *** */
  NrcCycleMake(ii->ins, day_count, 1);
  if( DEBUG4 )
  {
    /* DaySetDebug(ii->cycle, 2, stderr); */
    fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift types, patterns, and unwanted pattern constraints"      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddShiftTypes(CQ_INFO ii)                                           */
/*                                                                           */
/*  Add shift types.                                                         */
/*                                                                           */
/*****************************************************************************/

static void AddShiftTypes(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  CQ_TERM id_term, durn_term;
  CQ_FACTOR id_factor, durn_factor;  int i, durn_in_minutes;  char *id;
  NRC_WORKER_SET ws;
  if( DEBUG9 )
    fprintf(stderr, "[ AddShiftTypes:\n");
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_SHIFTS", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_SHIFTS");
  HaArrayForEach(section->lines, line, i)
  {
    /* make sure the line has 3 terms:  ShiftID, Length in mins, patterns */
    CQLineCheck(ii, line, 3);

    /* make sure the ShiftId is OK */
    id_term = HaArray(line->terms, 0);
    CQTermCheck(ii, line, id_term, 1);
    id_factor = HaArrayFirst(id_term->factors);
    CQFactorCheck(ii, line, id_term, id_factor, 1);
    id = id_factor->val1;

    /* make sure the duration is OK */
    durn_term = HaArray(line->terms, 1);
    CQTermCheck(ii, line, durn_term, 1);
    durn_factor = HaArrayFirst(durn_term->factors);
    CQFactorCheck(ii, line, durn_term, durn_factor, 1);
    if( sscanf(durn_factor->val1, "%d", &durn_in_minutes) != 1 )
      FatalError(ii->file_name, line->line_num,
	"second term's first factor is not an integer, as expected");

    /* add the shift type */
    NrcShiftTypeMake(ii->ins, id, durn_in_minutes);
    /* HaArrayAddLast(ii->shift_type_duration, durn_in_minutes); */
    /* ShiftMakeCQ(ii->all_shifts, id, durn_in_minutes); */

    /* add a preferred_worker_set for this shift type */
    ws = NrcWorkerSetMake(ii->ins, HnStringMake(ii->arena, "Preferred-%s", id));
    NrcWorkerSetSetAddWorkerSet(ii->preferred_worker_sets, ws);
  }
  if( DEBUG9 )
    fprintf(stderr, "] AddShiftTypes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET CQTermShiftTypeSet(CQ_INFO ii, CQ_LINE line,          */
/*    CQ_TERM term)                                                          */
/*                                                                           */
/*  Return the shift-type set given in term, which lies on line.             */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_TYPE_SET CQTermShiftTypeSet(CQ_INFO ii, CQ_LINE line,
  CQ_TERM term)
{
  NRC_SHIFT_TYPE_SET res;  NRC_SHIFT_TYPE st;  int i;  CQ_FACTOR factor;
  res = NrcShiftTypeSetMake(ii->ins, NULL);
  HaArrayForEach(term->factors, factor, i)
  {
    CQFactorCheck(ii, line, term, factor, 1);
    if( !NrcInstanceRetrieveShiftType(ii->ins, factor->val1, &st) )
      FatalError(ii->file_name, line->line_num, "unknown shift type %s",
	factor->val1);
    NrcShiftTypeSetAddShiftType(res, st);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  PATTERN PatternMakeFromCQTerm(CQ_INFO ii, CQ_TERM id_term,               */
/*    CQ_TERM pat_term)                                                      */
/*                                                                           */
/*  Return a pattern with id_term first and pat_term next.                   */
/*                                                                           */
/*****************************************************************************/

static NRC_PATTERN PatternMakeFromCQTerm(CQ_INFO ii, CQ_LINE line,
  CQ_TERM id_term, CQ_TERM pat_term)
{
  NRC_PATTERN res;
  res = NrcPatternMake(ii->ins, NULL);
  NrcPatternAddTerm(res, CQTermShiftTypeSet(ii, line, id_term), NRC_POSITIVE);
  NrcPatternAddTerm(res, CQTermShiftTypeSet(ii, line, pat_term), NRC_POSITIVE);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void UnwantedPattern(CQ_INFO ii, NRC_PATTERN p)                          */
/*                                                                           */
/*  Add an unwanted pattern constraint for p, applicable to all workers.     */
/*                                                                           */
/*****************************************************************************/

static void UnwantedPattern(CQ_INFO ii, NRC_PATTERN p)
{
  NRC_PENALTY penalty;
  if( DEBUG3 )
    fprintf(stderr, "[ UnwantedPattern(ii)\n");
  penalty = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  NrcUnwantedPatternConstraintMake(ii->ins, "UnwantedPattern",
    NrcInstanceStaffing(ii->ins), penalty, p, NrcInstanceCycle(ii->ins));
  if( DEBUG3 )
    fprintf(stderr, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void AddPatternsAndUnwantedPatternConstraints(CQ_INFO ii)                */
/*                                                                           */
/*  Add patterns to ii.                                                      */
/*                                                                           */
/*****************************************************************************/

static void AddPatternsAndUnwantedPatternConstraints(CQ_INFO ii)
{
  CQ_LINE line;  int i;  CQ_TERM id_term, pat_term;  NRC_PATTERN p;
  CQ_SECTION section;  NRC_PATTERN_SET ps;
  if( DEBUG7 )
    fprintf(stderr, "[ AddPatterns (%s):\n", ii->file_name);
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_SHIFTS", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_SHIFTS");

  /* check the lines, get the patterns, and add them to ps */
  ps = NrcPatternSetMake(ii->ins);
  HaArrayForEach(section->lines, line, i)
  {
    /* make sure the line has 3 terms:  ShiftID, Length in mins, patterns */
    CQLineCheck(ii, line, 3);
    id_term = HaArray(line->terms, 0);
    pat_term = HaArray(line->terms, 2);
    if( HaArrayCount(pat_term->factors) > 0 )
    {
      p = PatternMakeFromCQTerm(ii, line, id_term, pat_term);
      if( DEBUG7 )
	NrcPatternDebug(p, 2, stderr);
      NrcPatternSetAddPattern(ps, p);
      /* ***
      UnwantedPattern(ii, p);
      *** */
    }
  }

  /* reduce the patterns then generate each reduced one */
  if( DEBUG7 )
  {
    fprintf(stderr, "  patterns before reduction:\n");
    NrcPatternSetDebug(ps, 2, stderr);
  }
  ps = NrcPatternSetReduce(ps);
  if( DEBUG7 )
  {
    fprintf(stderr, "  patterns after reduction:\n");
    NrcPatternSetDebug(ps, 2, stderr);
  }
  for( i = 0;  i < NrcPatternSetPatternCount(ps);  i++ )
    UnwantedPattern(ii, NrcPatternSetPattern(ps, i));
  if( DEBUG7 )
    fprintf(stderr, "]\n");
}

/* *** old version without reduction using pattern sets
static void AddPatternsAndUnwantedPatternConstraints(CQ_INFO ii)
{
  CQ_LINE line;  int i;  CQ_TERM id_term, pat_term;  NRC_PATTERN p;
  CQ_SECTION section;
  if( DEBUG7 )
    fprintf(stderr, "[ AddPatterns:\n");
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_SHIFTS", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_SHIFTS");
  HaArrayForEach(section->lines, line, i)
  {
    ** make sure the line has 3 terms:  ShiftID, Length in mins, patterns **
    CQLineCheck(ii, line, 3);
    id_term = HaArray(line->terms, 0);
    pat_term = HaArray(line->terms, 2);
    if( HaArrayCount(pat_term->factors) > 0 )
    {
      p = PatternMakeFromCQTerm(ii, line, id_term, pat_term);
      if( DEBUG7 )
	NrcPatternDebug(p, 2, stderr);
      UnwantedPattern(ii, p);
    }
  }
  if( DEBUG7 )
    fprintf(stderr, "]\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void AddSectionShifts(CQ_INFO ii)                                        */
/*                                                                           */
/*  Add the SECTION_SHIFTS section (the shift types and unwanted patterns).  */
/*                                                                           */
/*  Implementation note.  There may be forward references in the patterns,   */
/*  so we add all the shift types first, then add the patterns.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void AddSectionShifts(CQ_INFO ii)
{
  AddShiftTypes(ii);
  AddPatternsAndUnwantedPatternConstraints(ii);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "weekends"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddWeekends(CQ_INFO ii)                                             */
/*                                                                           */
/*  Add weekends to ii.  What is needed is a shift-set set, with each        */
/*  shift-set holding the shifts of one weekend.  This needs to be done      */
/*  after the days and shift types are added.                                */
/*                                                                           */
/*****************************************************************************/

static void AddWeekends(CQ_INFO ii)
{
  NRC_SHIFT_SET ss;  NRC_DAY sat_d, sun_d;  int i;
  ii->weekends = NrcShiftSetSetMake(ii->ins);
  for( i = 5;  i + 1 < NrcInstanceCycleDayCount(ii->ins);  i += 7 )
  {
    ss = NrcShiftSetMake(ii->ins);
    sat_d = NrcInstanceCycleDay(ii->ins, i);
    sun_d = NrcInstanceCycleDay(ii->ins, i + 1);
    NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(sat_d));
    NrcShiftSetAddShiftSet(ss, NrcDayShiftSet(sun_d));
    NrcShiftSetSetAddShiftSet(ii->weekends, ss);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "workers and their constraints"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int GetValue(CQ_INFO ii, CQ_LINE line, int term_index,                   */
/*    char *factor_label)                                                    */
/*                                                                           */
/*  Extract one integer value from line.                                     */
/*                                                                           */
/*****************************************************************************/

static int GetValue(CQ_INFO ii, CQ_LINE line, int term_index,
  char *factor_label)
{
  CQ_TERM term;  CQ_FACTOR factor;  int i, res;

  /* find the term */
  if( term_index < 0 || term_index >= HaArrayCount(line->terms) )
    FatalError(ii->file_name, line->line_num, "no term with index %d on line",
      term_index);
  term = HaArray(line->terms, term_index);

  /* find the factor and its value */
  if( factor_label == NULL )
  {
    CQTermCheck(ii, line, term, 1);
    factor = HaArray(term->factors, 0);
    CQFactorCheck(ii, line, term, factor, 1);
    if( sscanf(factor->val1, "%d", &res) != 1 )
      FatalError(ii->file_name, line->line_num, "term %d not integer",
	term_index+1);
    return res;
  }
  else
  {
    HaArrayForEach(term->factors, factor, i)
      if( strcmp(factor->val1, factor_label) == 0 )
      {
	if( factor->val2 == NULL )
	  FatalError(ii->file_name, line->line_num,
	    "no second part of factor %d of term %d", i+1, term_index+1);
        if( sscanf(factor->val2, "%d", &res) != 1 )
	  FatalError(ii->file_name, line->line_num,
	    "second part of factor %d of term %d not integer", i+1,
	    term_index+1);
	return res;
      }
    FatalError(ii->file_name, line->line_num,
      "no factor with first part %s in term %d of line",
      factor_label, term_index+1);
    return -1;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxSameShiftDays(CQ_INFO ii)                                        */
/*                                                                           */
/*  Add max assignments per shift type constraints.                          */
/*                                                                           */
/*****************************************************************************/

static void MaxSameShiftDays(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int i, limit;  NRC_CONSTRAINT c;  char *shift_type_name;  NRC_SHIFT_TYPE st;
  NRC_PENALTY p;  NRC_WORKER_SET ws;
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  for( i = 0;  i < NrcInstanceShiftTypeCount(ii->ins);  i++ )
  {
    st = NrcInstanceShiftType(ii->ins, i);
    shift_type_name = NrcShiftTypeName(st);
    limit = GetValue(ii, line, 1, shift_type_name);
    if( limit == 0 )
    {
      /* use a prefer resources constraint - so do nothing here */
    }
    else
    {
      /* constraint w to at most limit shifts of type st */
      if( limit < NrcInstanceCycleDayCount(ii->ins) )
      {
	/* should this be consecutive days?  No, I've checked the doc */
	c = NrcConstraintMake(ii->ins, "MaxSameShiftDays",
	  NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_ACTIVE,
	  NrcBoundMakeMax(limit, p, ii->ins), NULL);
	NrcConstraintAddShiftSetSet(c, NrcShiftTypeShiftSetSet(st),
	  NRC_POSITIVE);
      }

      /* add w to the preferred worker set for st */
      ws = NrcWorkerSetSetWorkerSet(ii->preferred_worker_sets, i);
      NrcWorkerSetAddWorker(ws, w);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxAndMinBusyMinutes(CQ_INFO ii)                                    */
/*                                                                           */
/*  Add max and min minutes worked constraints.                              */
/*                                                                           */
/*****************************************************************************/

static void MaxBusyMinutes(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;  NRC_PENALTY p;
  /* max term has index 2, min term has index 3 */
  limit = GetValue(ii, line, 2, NULL);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  NrcConstraintMake(ii->ins, "MaxBusyMinutes", NrcWorkerSingletonWorkerSet(w),
    NRC_CONSTRAINT_WORKLOAD, NrcBoundMakeMax(limit, p, ii->ins), NULL);
  /* ***
  NrcConstraintMake(ii->ins, "MaxBusyMinutes", NrcWorkerSingletonWorkerSet(w),
    p, NrcLimit(NRC_LIMIT_MAX_WORKLOAD, limit), NULL);
  *** */
  /* no shift-sets wanted with workload constraints */
}

static void MinBusyMinutes(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;   NRC_PENALTY p;
  /* max term has index 2, min term has index 3 */
  limit = GetValue(ii, line, 3, NULL);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  NrcConstraintMake(ii->ins, "MinBusyMinutes", NrcWorkerSingletonWorkerSet(w),
    NRC_CONSTRAINT_WORKLOAD, NrcBoundMakeMin(limit, false, p, ii->ins), NULL);
  /* ***
  NrcConstraintMake(ii->ins, "MinBusyMinutes", NrcWorkerSingletonWorkerSet(w),
    p, NrcLimit(NRC_LIMIT_MIN_WORKLOAD, limit), NULL);
  *** */
  /* no shift-sets wanted with workload constraints */
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxConsecutiveBusyDays(CQ_INFO ii)                                  */
/*                                                                           */
/*  Add max consecutive days constraints.                                    */
/*                                                                           */
/*****************************************************************************/

static void MaxConsecutiveBusyDays(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  /* max consecutive term has index 4 */
  limit = GetValue(ii, line, 4, NULL);
  if( limit < NrcInstanceCycleDayCount(ii->ins) )
  {
    p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "MaxConsecutiveBusyDays",
      NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_CONSECUTIVE,
      NrcBoundMakeMax(limit, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "MaxConsecutiveBusyDays",
      NrcWorkerSingletonWorkerSet(w), p,
      NrcLimit(NRC_LIMIT_MAX_CONSECUTIVE, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveBusyDays(CQ_INFO ii)                                  */
/*                                                                           */
/*  Add min consecutive busy days constraints.                               */
/*                                                                           */
/*  This includes artificial values for history:  ai = L, xi = L, ci = L,    */
/*  which ensure that sequences that include the first and last day never    */
/*  attract a penalty, as required (somewhat strangely) by the CQ14 model.   */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveBusyDays(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  /* min consecutive term has index 5 */
  limit = GetValue(ii, line, 5, NULL);
  if( limit < NrcInstanceCycleDayCount(ii->ins) )
  {
    p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "MinConsecutiveBusyDays",
      NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_CONSECUTIVE,
      NrcBoundMakeMin(limit, false, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "MinConsecutiveBusyDays",
      NrcWorkerSingletonWorkerSet(w), p,
      NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_POSITIVE);
    NrcConstraintAddHistory(c, limit, limit);
    NrcConstraintAddHistoryWorker(c, w, limit);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MinConsecutiveFreeDays(CQ_INFO ii)                                  */
/*                                                                           */
/*  Add min consecutive free days constraints.                               */
/*                                                                           */
/*****************************************************************************/

static void MinConsecutiveFreeDays(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  /* min consecutive free term has index 6 */
  limit = GetValue(ii, line, 6, NULL);
  if( limit < NrcInstanceCycleDayCount(ii->ins) )
  {
    p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "MinConsecutiveFreeDays",
      NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_CONSECUTIVE,
      NrcBoundMakeMin(limit, false, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "MinConsecutiveFreeDays",
      NrcWorkerSingletonWorkerSet(w), p,
      NrcLimit(NRC_LIMIT_MIN_CONSECUTIVE, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, NrcInstanceDaysShiftSetSet(ii->ins),
      NRC_NEGATIVE);
    NrcConstraintAddHistory(c, limit, limit);
    NrcConstraintAddHistoryWorker(c, w, limit);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MaxBusyWeekends(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)             */
/*                                                                           */
/*  Add max weekends worked constraints.                                     */
/*                                                                           */
/*****************************************************************************/

static void MaxBusyWeekends(CQ_INFO ii, NRC_WORKER w, CQ_LINE line)
{
  int limit;  NRC_CONSTRAINT c;  NRC_PENALTY p;
  /* MaxWeekends term has index 7 */
  limit = GetValue(ii, line, 7, NULL);
  if( limit < NrcShiftSetSetShiftSetCount(ii->weekends) )
  {
    p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    c = NrcConstraintMake(ii->ins, "MaxWeekends",
      NrcWorkerSingletonWorkerSet(w), NRC_CONSTRAINT_ACTIVE,
      NrcBoundMakeMax(limit, p, ii->ins), NULL);
    /* ***
    c = NrcConstraintMake(ii->ins, "MaxWeekends",
      NrcWorkerSingletonWorkerSet(w), p, NrcLimit(NRC_LIMIT_MAX, limit), NULL);
    *** */
    NrcConstraintAddShiftSetSet(c, ii->weekends, NRC_POSITIVE);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSectionStaff(CQ_INFO ii)                                         */
/*                                                                           */
/*  Add the SECTION_STAFF section (the workers and their constraints).       */
/*                                                                           */
/*****************************************************************************/

static void AddSectionStaff(CQ_INFO ii)
{
  CQ_LINE line;  int i;  CQ_TERM term;  CQ_SECTION section;
  CQ_FACTOR factor;   /* KHE_RESOURCE r; */  NRC_WORKER w;
  if( DEBUG2 )
    fprintf(stderr, "[ AddSectionStaff:\n");
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_STAFF", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_STAFF");
  HaArrayForEach(section->lines, line, i)
  {
    /* make sure the line has 8 terms: ID, MaxShifts, MaxTotalMinutes, */
    /* MinTotalMinutes, MaxConsecutiveShifts, MinConsecutiveShifts,    */
    /* MinConsecutiveDaysOff, MaxWeekends                              */
    CQLineCheck(ii, line, 8);

    /* first term - the resource Id */
    term = HaArray(line->terms, 0);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    w = NrcWorkerMake(ii->ins, factor->val1);

    /* constraints along the line */
    MaxSameShiftDays(ii, w, line);
    MaxBusyMinutes(ii, w, line);
    MinBusyMinutes(ii, w, line);
    MaxConsecutiveBusyDays(ii, w, line);
    MinConsecutiveBusyDays(ii, w, line);
    MinConsecutiveFreeDays(ii, w, line);
    MaxBusyWeekends(ii, w, line);
  }
  if( DEBUG2 )
    fprintf(stderr, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "days off"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DaysOff(CQ_INFO ii)                                                 */
/*                                                                           */
/*  Add the SECTION_DAYS_OFF section (days off)                              */
/*                                                                           */
/*****************************************************************************/

static void AddSectionDaysOff(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  CQ_TERM term;  CQ_FACTOR factor;
  int i, j, day_index;  NRC_WORKER w;  NRC_DAY d;  NRC_PENALTY p;
  if( CQFileRetrieveSection(ii->cq_file, "SECTION_DAYS_OFF", &section) )
  {
    p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
    HaArrayForEach(section->lines, line, i)
    {
      /* first term - nurse id */
      term = HaArray(line->terms, 0);
      CQTermCheck(ii, line, term, 1);
      factor = HaArrayFirst(term->factors);
      CQFactorCheck(ii, line, term, factor, 1);
      if( !NrcInstanceStaffingRetrieveWorker(ii->ins, factor->val1, &w) )
        FatalError(ii->file_name, line->line_num, "unknown resource %s",
	  factor->val1);

      /* day indexes */
      for( j = 1;  j < HaArrayCount(line->terms);  j++ )
      {
	term = HaArray(line->terms, j);
	CQTermCheck(ii, line, term, 1);
	factor = HaArrayFirst(term->factors);
	CQFactorCheck(ii, line, term, factor, 1);
	if( !sscanf(factor->val1, "%d", &day_index) )
          FatalError(ii->file_name, line->line_num, "invalid day index %s",
	    factor->val1);
	if( day_index < 0 || day_index >= NrcInstanceCycleDayCount(ii->ins) )
          FatalError(ii->file_name, line->line_num,
	    "day index %d out of range (0 .. %d)", day_index,
	    NrcInstanceCycleDayCount(ii->ins) - 1);
	d = NrcInstanceCycleDay(ii->ins, day_index);
	NrcWorkerAddDayOff(w, d, p);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift on requests"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void ShiftOnOrOffOneLine(CQ_INFO ii, CQ_LINE line, NRC_WORKER *w,        */
/*    NRC_DAY *d, NRC_SHIFT_TYPE *st, int *weight)                           */
/*                                                                           */
/*  Read one line from a shift off or shift on section, and return its       */
/*  worker, day, shift type, and weight fields.                              */
/*                                                                           */
/*****************************************************************************/

static void ShiftOnOrOffOneLine(CQ_INFO ii, CQ_LINE line, NRC_WORKER *w,
  NRC_DAY *d, NRC_SHIFT_TYPE *st, int *weight)
{
  CQ_TERM term;  CQ_FACTOR factor;  int day_index;

  /* first term - nurse id */
  term = HaArray(line->terms, 0);
  CQTermCheck(ii, line, term, 1);
  factor = HaArrayFirst(term->factors);
  CQFactorCheck(ii, line, term, factor, 1);
  if( !NrcInstanceStaffingRetrieveWorker(ii->ins, factor->val1, w) )
    FatalError(ii->file_name, line->line_num, "unknown resource %s",
      factor->val1);

  /* second term - day index */
  term = HaArray(line->terms, 1);
  CQTermCheck(ii, line, term, 1);
  factor = HaArrayFirst(term->factors);
  CQFactorCheck(ii, line, term, factor, 1);
  if( !sscanf(factor->val1, "%d", &day_index) )
    FatalError(ii->file_name, line->line_num, "invalid day index %s",
      factor->val1);
  if( day_index < 0 || day_index >= NrcInstanceCycleDayCount(ii->ins) )
    FatalError(ii->file_name, line->line_num,
      "day index %d out of range (0 .. %d)", day_index,
      NrcInstanceCycleDayCount(ii->ins) - 1);
  *d = NrcInstanceCycleDay(ii->ins, day_index);

  /* third term - shift type */
  term = HaArray(line->terms, 2);
  CQTermCheck(ii, line, term, 1);
  factor = HaArrayFirst(term->factors);
  CQFactorCheck(ii, line, term, factor, 1);
  if( !NrcInstanceRetrieveShiftType(ii->ins, factor->val1, st) )
    FatalError(ii->file_name, line->line_num, "unknown shift type %s",
      factor->val1);

  /* fourth term - weight */
  term = HaArray(line->terms, 3);
  CQTermCheck(ii, line, term, 1);
  factor = HaArrayFirst(term->factors);
  CQFactorCheck(ii, line, term, factor, 1);
  if( !sscanf(factor->val1, "%d", weight) || *weight <= 0 )
    FatalError(ii->file_name, line->line_num, "invalid weight %s",
      factor->val1);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddSectionShiftOn(CQ_INFO ii)                                       */
/*                                                                           */
/*  Add the SECTION_SHIFT_ON_REQUESTS sections (shift on requests).          */
/*                                                                           */
/*****************************************************************************/

static void AddSectionShiftOn(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  NRC_PENALTY p;
  int i, weight;  NRC_WORKER w;  NRC_DAY d;  NRC_SHIFT_TYPE st;
  if( CQFileRetrieveSection(ii->cq_file, "SECTION_SHIFT_ON_REQUESTS",&section) )
    HaArrayForEach(section->lines, line, i)
    {
      /* read one line and add the corresponding shift on request */
      ShiftOnOrOffOneLine(ii, line, &w, &d, &st, &weight);
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddShiftOn(w, NrcDayShiftFromShiftType(d, st), p);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift off requests"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSectionShiftOff(CQ_INFO ii)                                      */
/*                                                                           */
/*  Add the SECTION_SHIFT_OFF_REQUESTS sections (shift off requests).        */
/*                                                                           */
/*****************************************************************************/

static void AddSectionShiftOff(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  NRC_PENALTY p;
  int i, weight;  NRC_WORKER w;  NRC_DAY d;  NRC_SHIFT_TYPE st;
  if( CQFileRetrieveSection(ii->cq_file,"SECTION_SHIFT_OFF_REQUESTS",&section) )
    HaArrayForEach(section->lines, line, i)
    {
      /* read one line and add the corresponding shift off request */
      ShiftOnOrOffOneLine(ii, line, &w, &d, &st, &weight);
      p = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ii->ins);
      NrcWorkerAddShiftOff(w, NrcDayShiftFromShiftType(d, st), p);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Coverage"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void AddSectionCover(CQ_INFO ii)                                         */
/*                                                                           */
/*  Add the SECTION_COVER section (cover requirements).                      */
/*                                                                           */
/*****************************************************************************/

static void AddSectionCover(CQ_INFO ii)
{
  CQ_SECTION section;  CQ_LINE line;  CQ_TERM term;  CQ_FACTOR factor;
  int i, day_index, opt_cover, weight_under, weight_over;  NRC_SHIFT_TYPE st;
  NRC_DAY d;  NRC_DEMAND_SET dms;  NRC_PENALTY p1, p2, p3;  NRC_BOUND b;
  NRC_WORKER_SET preferred_ws;
  if( !CQFileRetrieveSection(ii->cq_file, "SECTION_COVER", &section) )
    FatalError(ii->file_name, 0, "missing SECTION_COVER");
  p1 = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  /* p1 = NrcInstanceZeroPenalty(ii->ins); */
  HaArrayForEach(section->lines, line, i)
  {
    CQLineCheck(ii, line, 5);

    /* first term - day index (counting from 0) */
    term = HaArray(line->terms, 0);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    if( sscanf(factor->val1, "%d", &day_index) != 1 )
      FatalError(ii->file_name, line->line_num,
	"integer day index expected in first term of line\n");
    if( day_index < 0 || day_index >= NrcInstanceCycleDayCount(ii->ins) )
      FatalError(ii->file_name, line->line_num,
	"day index %d out of range (0 .. %d)\n",
	day_index, NrcInstanceCycleDayCount(ii->ins) - 1);
    d = NrcInstanceCycleDay(ii->ins, day_index);

    /* second term - shift type (and hence preferred_ws) */
    term = HaArray(line->terms, 1);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    if( !NrcInstanceRetrieveShiftType(ii->ins, factor->val1, &st) )
      FatalError(ii->file_name, line->line_num, "unknown shift type %s",
	factor->val1);
    preferred_ws = NrcWorkerSetSetWorkerSet(ii->preferred_worker_sets,
      NrcShiftTypeIndex(st));

    /* third term - number of nurses required */
    term = HaArray(line->terms, 2);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    if( sscanf(factor->val1, "%d", &opt_cover) != 1 )
      FatalError(ii->file_name, line->line_num,
	"integer requirement expected in third term of line\n");
    if( opt_cover < 0 )
      FatalError(ii->file_name, line->line_num, "negative requirement %d\n",
	opt_cover);

    /* fourth term - weight for under */
    term = HaArray(line->terms, 3);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    if( sscanf(factor->val1, "%d", &weight_under) != 1 )
      FatalError(ii->file_name, line->line_num,
	"integer weight for under expected in fourth term of line\n");
    if( weight_under < 0 )
      FatalError(ii->file_name, line->line_num,
	"negative weight for under %d\n", weight_under);

    /* fifth term - weight for over */
    term = HaArray(line->terms, 4);
    CQTermCheck(ii, line, term, 1);
    factor = HaArrayFirst(term->factors);
    CQFactorCheck(ii, line, term, factor, 1);
    if( sscanf(factor->val1, "%d", &weight_over) != 1 )
      FatalError(ii->file_name, line->line_num,
	"integer weight for over expected in fourth term of line\n");
    if( weight_over < 0 )
      FatalError(ii->file_name, line->line_num,
	"negative weight for over %d\n", weight_over);

    /* build a suitable demand-set */
    p2 = NrcPenalty(false, weight_under, NRC_COST_FUNCTION_LINEAR, ii->ins);
    p3 = NrcPenalty(false, weight_over, NRC_COST_FUNCTION_LINEAR, ii->ins);
    b = NrcBoundMakePreferred(opt_cover, p2, p3, ii->ins);
    dms = NrcDemandSetMakeFromBound(ii->ins, b, 2 * opt_cover + 5,
      preferred_ws, p1);
    /* ***
    dms = NrcDemandSetMake(ii->ins);
    dm = NrcDemandMake(ii->ins, p2, p1, NULL, p1);
    NrcDemandSetAddDemandMulti(dms, dm, opt_cover);
    dm = NrcDemandMake(ii->ins, p1, p3, NULL, p1);
    NrcDemandSetAddDemandMulti(dms, dm, opt_cover + 5);
    *** */
    NrcShiftAddDemandSet(NrcDayShiftFromShiftType(d, st), dms);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraints implicit in instance"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void MaxShiftsPerDay(CQ_INFO ii)                                         */
/*                                                                           */
/*  Add single shift per day constraints for all resources.                  */
/*                                                                           */
/*****************************************************************************/

static void MaxShiftsPerDay(CQ_INFO ii)
{
  NRC_CONSTRAINT c;  NRC_SHIFT_SET starting_ss;  NRC_PENALTY p;
  NRC_SHIFT_SET_SET day_sss;
  starting_ss = NrcInstanceDailyStartingShiftSet(ii->ins);
  p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins);
  c = NrcConstraintMake(ii->ins, "MaxShiftsPerDay",
    NrcInstanceStaffing(ii->ins), NRC_CONSTRAINT_ACTIVE,
    NrcBoundMakeMax(1, p, ii->ins), starting_ss);
  /* ***
  c = NrcConstraintMake(ii->ins, "MaxShiftsPerDay",
    NrcInstanceStaffing(ii->ins), p, NrcLimit(NRC_LIMIT_MAX, 1), starting_ss);
  *** */
  day_sss = NrcDayShiftSetSet(NrcInstanceCycleDay(ii->ins, 0));
  NrcConstraintAddShiftSetSet(c, day_sss, NRC_POSITIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *CQ14InstanceName(char *str, HA_ARENA a)                            */
/*                                                                           */
/*  Find the name of an instance within str, and return that name in         */
/*  arena memory.                                                            */
/*                                                                           */
/*****************************************************************************/

char *CQ14InstanceName(char *str, HA_ARENA a)
{
  char *tmp, *p;  int num;
  tmp = HnStringCopy(str, a);
  p = strstr(tmp, ".txt");
  if( p != NULL && p != tmp )
    *p = '\0';
  p = strstr(tmp, ".ros");
  if( p != NULL && p != tmp )
    *p = '\0';
  for( p = &tmp[strlen(tmp) - 1];  p != tmp && is_digit(*p);  p-- );
  p++;
  sscanf(p, "%d", &num);
  /* ***
  for( p = &tmp[strlen(tmp) - 1];  p != tmp && *p != '/' && *p != '\\';  p-- );
  if( *p == '/' || *p == '\\' )  p++;
  *** */
  return HnStringMake(a, "CQ14-%02d", num);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE CQ14ConvertInstance(INSTANCE_MODEL ins_model,               */
/*    char *instance_file_name, HA_ARENA_SET as)                             */
/*                                                                           */
/*  Read instance_file_name, which is a file in the Curtois and Qu 2014      */
/*  format, and add the resulting instance to archive.                       */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE CQ14ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  CQ_INFO ii;  char *ins_id;  HA_ARENA a;  NRC_INSTANCE res;

  /* make an instance info object, initially more or less empty */
  a = HaArenaMake(as);
  HaMake(ii, a);
  ii->arena = a;
  ii->file_name = instance_file_name;
  ii->cq_file = CQFileRead(ii->file_name, a);
  ii->ins = NULL;
  ii->weekends = NULL;

  /* build instance id from file name, and make instance */
  ins_id = CQ14InstanceName(instance_file_name, a);
  /* ***
  md = InstanceModelMetaData(ins_model, ins_id, NULL, NULL, NULL,
    MStringMake("Based on Curtois-Qu 2014 text file %s", instance_file_name),
    NULL);
  *** */
  ii->ins = NrcInstanceMakeBegin(ins_id, /* "Nurses", */ "Nurse", as);
  ii->preferred_worker_sets = NrcWorkerSetSetMake(ii->ins);
  /* p = NrcPenalty(true, 1, NRC_COST_FUNCTION_LINEAR, ii->ins); */
  NrcInstanceSetMetaData(ii->ins, ins_id, NULL, NULL, NULL,
    HnStringMake(a, "Based on Curtois-Qu 2014 text file %s",instance_file_name),
    NULL);

  /* add the sections of the instance file */
  AddShiftTypes(ii);
  AddSectionHorizon(ii);  /* must come after shift types */
  AddPatternsAndUnwantedPatternConstraints(ii);
  AddWeekends(ii);  /* must come after days and shift types are added */
  AddSectionStaff(ii);  /* this includes bulding the preferred_worker_sets */
  AddSectionDaysOff(ii);
  AddSectionShiftOn(ii);
  AddSectionShiftOff(ii);
  AddSectionCover(ii);

  /* add the one shift per day constraint, implicit in the instance */
  MaxShiftsPerDay(ii);

  /* all done */
  NrcInstanceMakeEnd(ii->ins);
  res = ii->ins;
  HaArenaDelete(a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int CQ14ExtractPenaltyFromFileName(char *file_name)                      */
/*                                                                           */
/*  Extract the penalty value from a file name like                          */
/*  Instance19.Solution.4032_1.roster or Instance19.Solution.4032.roster.    */
/*                                                                           */
/*****************************************************************************/

static int CQ14ExtractPenaltyFromFileName(char *file_name)
{
  char *p;  int res;
  if( DEBUG10 )
    fprintf(stderr, "[ CQ14ExtractPenaltyFromFileName(%s)\n", file_name);
  p = strstr(file_name, ".ros");
  if( p == NULL )
    FatalError(file_name, -1, "cannot extract penalty from soln file name");
  for( p--;  p >= file_name && (is_digit(*p) || *p == '_');  p-- );
  if( sscanf(p+1, "%d", &res) != 1 )
    FatalError(file_name, -1, "cannot extract penalty from soln file name");
  if( DEBUG10 )
    fprintf(stderr, "] CQ14ExtractPenaltyFromFileName returning %d\n", res);
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
/*  void ShiftOverloadAdd(ARRAY_SHIFT_OVERLOAD *overloads, NRC_SHIFT shift,  */
/*    HA_ARENA a)                                                            */
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
/*  void CQ14ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,        */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)                */
/*                                                                           */
/*  Convert soln_file_name into a solution of ins and add it to a solution   */
/*  group of archive, which is known to have at least one solution group.    */
/*  This code assumes further that it has solution groups called EC10,       */
/*  EC30, BP, and GR.                                                        */
/*                                                                           */
/*  The penalty is taken from the file name; not all solutions contain a     */
/*  <Penalty> category, but all solution file names contain a penalty, e.g.  */
/*  Instance19.Solution.4032_1.roster or Instance19.Solution.4032.roster.    */
/*                                                                           */
/*****************************************************************************/

void CQ14ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  FILE *fp;  NRC_SHIFT_TYPE st;  NRC_SHIFT s;  char *ins_id, *id;  NRC_DAY d;
  NRC_SOLN soln;  KML_ERROR ke;  NRC_WORKER w;  int i, j, k, day_num;
  KML_ELT soln_elt, employee_elt, assign_elt, day_elt, shift_type_elt;
  KML_ELT ins_elt, alg_elt, time_elt;  float secs;  HA_ARENA a;
  ARRAY_SHIFT_OVERLOAD overloads;  SHIFT_OVERLOAD max_so;
  if( DEBUG8 )
    fprintf(stderr, "[ CQ14ConvertSoln(%s, %s, archive)\n",
      soln_file_name, ins != NULL ? NrcInstanceId(ins) : "NULL");

  /* open the file and return if can't */
  fp = fopen(soln_file_name, "r");
  if( fp == NULL )
  {
    Warning(soln_file_name, -1, "ignoring solution file (cannot open)");
    if( DEBUG8 )
      fprintf(stderr, "] CQ14ConvertSoln failed (cannot open file)\n");
    return;
  }

  /* read XML from the file, and exit if can't */
  a = HaArenaMake(as);
  if( !KmlReadFile(fp, NULL, &soln_elt, &ke, a) )
  {
    KmlWarning(ke, soln_file_name);
    fclose(fp);
    if( DEBUG8 )
      fprintf(stderr, "] CQ14ConvertSoln failed (cannot read XML from file)\n");
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
    if( DEBUG5 )
      fprintf(stderr, "CQ14ConvertSoln failing at 1\n");
    KmlFatalError(ke, soln_file_name);
  }

  /* work out which instance this is for */
  KmlContainsChild(soln_elt, "SchedulingPeriodFile", &ins_elt);
  ins_id = CQ14InstanceName(KmlText(ins_elt), a);
  if( ins == NULL )
  {
    if( !NrcArchiveRetrieveInstance(archive, ins_id, &ins) )
    {
      KmlEltWarning(ins_elt, soln_file_name,
	"ignoring solution file; instance %s unknown", ins_id);
      if( DEBUG8 )
	fprintf(stderr, "] CQ14ConvertSoln failed (cannot find instance)\n");
      return;
    }
    if( DEBUG8 )
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
    CQ14ExtractPenaltyFromFileName(soln_file_name)));

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
    if( !NrcInstanceStaffingRetrieveWorker(ins, id, &w) )
      KmlEltFatalError(employee_elt, soln_file_name, "unknown Employee %s",
	KmlText(employee_elt));

    /* get and make the assignments */
    if( DEBUG8 )
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
      for( k = 0;  k < NrcShiftDemandCount(s);  k++ )
	if( NrcSolnAssignment(soln, s, k) == NULL )
	  break;
      if( k < NrcShiftDemandCount(s) )
	NrcSolnAddAssignment(soln, s, k, w);
      else
        ShiftOverloadAdd(&overloads, s, a);
    }
  }

  /* add description */
  NrcSolnSetDescription(soln,
    HnStringMake(a, "From %s.", soln_file_name));

  /* add running time, if available */
  secs = 0.0;
  if( KmlContainsChild(soln_elt, "CpuTime", &time_elt) )
  {
    int days, hours, mins, seconds, milliseconds;
    if( sscanf(KmlText(time_elt), "Time Elapsed = %d days, %d hours, "
	"%d minutes, %d seconds, %d milliseconds", &days, &hours, &mins,
	&seconds, &milliseconds) == 5 )
    {
      secs = days * 24 * 60 * 60 + hours * 60 * 60 +
	mins * 60 + seconds + milliseconds * 0.001;
      NrcSolnSetRunningTime(soln, secs);
    }
    else
      KmlEltWarning(time_elt, soln_file_name, "cannot parse CpuTime \"%s\"",
	KmlText(time_elt));
  }

  /* work out the algorithm, thence the solution group */
  if( KmlContainsChild(soln_elt, "Algorithm", &alg_elt) )
    id = KmlText(alg_elt);
  else
    id = NULL;

  /* fail with debug print if overloads */
  if( HaArrayCount(overloads) > 0 )
  {
    max_so = ShiftOverloadMax(&overloads);
    Warning(soln_file_name, -1, "ignoring solution (overloaded shifts, "
      "worst is %s:%s with demand %d and overload %d)",
      NrcDayShortName(NrcShiftDay(max_so->shift)),
      NrcShiftTypeName(NrcShiftType(max_so->shift)),
      NrcShiftDemandCount(max_so->shift), max_so->overload);
    if( DEBUG8 )
    {
      ShiftOverloadDebug(&overloads, 2, stderr);
      fprintf(stderr, "] CQ14ConvertSoln failed (shift overload)\n");
    }
    return;
  }

  /* add soln */
   SolnModelAddSoln(soln_model, archive, id, secs, soln);
  if( DEBUG12 )
    Warning(soln_file_name, -1, "including solution");
  HaArenaDelete(a);
  /* KmlFree(soln_elt); */
  if( DEBUG8 )
    fprintf(stderr, "] CQ14ConvertSoln returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP CQ14SolnGroupFn(NRC_ARCHIVE archive,                      */
/*    char *algorithm, float runtime)                                        */
/*                                                                           */
/*  Return the appropriate soln group for CQ14.                              */
/*                                                                           */
/*****************************************************************************/

NRC_SOLN_GROUP CQ14SolnGroupFn(SOLN_MODEL soln_model, NRC_ARCHIVE archive,
  char *algorithm, float runtime)
{
  char *sg_name;  NRC_SOLN_GROUP res;

  /* find the name of the soln group */
  if( algorithm == NULL )
  {
    /* no algorithm, this must be Gurobi, according to Curtois */
    sg_name = "CQ-GR";
  }
  else if( strstr(algorithm, "Branch and Price") != NULL )
  {
    /* algorithm, is Branch and Price */
    sg_name = "CQ-BP";
  }
  else if( runtime < 10.5 * 60.0 )  /* less than 10.5 minutes */
  {
    sg_name = "CQ-EJ10";
  }
  else
  {
    sg_name = "CQ-EJ60";
  }

  /* retrieve the soln group from the archive and return it */
  if( !NrcArchiveRetrieveSolnGroup(archive, sg_name, &res) )
    FatalError(NULL, 0, "no %s solution group to store solution in", sg_name);
  return res;
}

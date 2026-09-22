
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
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
/*  FILE:         main.c                                                     */
/*  MODULE:       Main module, handles reading and operation dispatch        */
/*                                                                           */
/*****************************************************************************/
#define _POSIX_SOURCE
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "externs.h"

#define DEBUG1 0
/* #define DEBUG_AVAIL 1 */
#define BannerFoot "HSEval Software Copyright Jeffrey H. Kingston 2012"


/*****************************************************************************/
/*                                                                           */
/*  HTML PageBegin()                                                         */
/*                                                                           */
/*  Begin an HSEVAL output page, returning the HTML handle where subsequent  */
/*  writes to that page should go.                                           */
/*                                                                           */
/*****************************************************************************/

HTML PageBegin(char *title)
{
  HTML html = NULL;
  html = HTMLPageBegin("./hseval.cgi", "HSEval home page", stdout);
  HTMLHeadBegin(html);
  if( title != NULL )
  {
    HTMLTitleBegin(html);
    HTMLText(html, title);
    HTMLTitleEnd(html);
  }
  HTMLHeadEnd(html);
  HTMLBodyBegin(html);
  HTMLSmallBegin(html);
  HTMLText(html, "HSEval");
  HTMLText(html, KheVersionBanner());
  HTMLSmallEnd(html);
  HTMLHorizontalRule(html);
  return html;
}


/*****************************************************************************/
/*                                                                           */
/*  void PageEnd(html)                                                       */
/*                                                                           */
/*  End an HSEVAL output page.                                               */
/*                                                                           */
/*****************************************************************************/

void PageEnd(HTML html)
{
  HTMLHorizontalRule(html);
  HTMLSmallBegin(html);
  HTMLText(html, BannerFoot);
  HTMLSmallEnd(html);
  HTMLBodyEnd(html);
  HTMLPageEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void Error(CGI cgi, char *text)                                          */
/*                                                                           */
/*  Report an error and exit.                                                */
/*                                                                           */
/*  The error text is to be taken literally, so any < and > chars in it      */
/*  must be encoded.                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***** replaced by CommandError
void Error(CGI cgi, char *text)
{
  HTML html;

  html = PageBegin("HSEval: Error in uploaded XML file");
  HTMLBigHeading(html, "HSEval: Error in uploaded XML file");

  HTMLParagraphBegin(html);
  HTMLText(html, "The XML file you just uploaded has at least one error:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLColouredBoxBegin(html, LightR ed);
  HTMLLiteralText(html, text);
  HTMLColouredBoxEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  PageEnd(html);
  CgiEnd(cgi);
  exit(0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void TimeOut(int sig)                                                    */
/*                                                                           */
/*  Signal handler function for when HSEval times out.                       */
/*                                                                           */
/*****************************************************************************/

static void TimeOut(int sig)
{
  HTML html;

  html = PageBegin("HSEval: Timeout");
  HTMLBigHeading(html, "HSEval: Timeout");

  HTMLParagraphBegin(html);
  HTMLText(html, "The call you have just made on HSEval was taking too");
  HTMLText(html, "long and has timed out.  Please do not use HSEval to");
  HTMLText(html, "evaluate very large archives, like the solutions to");
  HTMLText(html, "Part 2 of ITC2011.  Consult Jeff Kingston if you need");
  HTMLText(html, "to evaluate a very large archive.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  PageEnd(html);
  exit(0);
}


/*****************************************************************************/
/*                                                                           */
/*  void DisplayFrontPage(void)                                              */
/*                                                                           */
/*  Display the front page.                                                  */
/*                                                                           */
/*****************************************************************************/

static void DisplayFrontPage(void)
{
  HTML html;
  /* struct rlimit rlim_rec;  int status; */

  html = PageBegin("The HSEval High School Timetable Evaluator");
  HTMLBigHeading(html, "The HSEval High School Timetable Evaluator");
  HTMLFormBeginUpload(html, true);

  /* ***
  HTMLParagraphBegin(html);
  HTMLColouredBoxBeginFullWidth(html, LightRed);
  HTMLText(html, "This is a new version of HSEval running on a new server.");
  HTMLText(html, "Please report problems by email to jeff@it.usyd.edu.au.");
  HTMLColouredBoxEnd(html);
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "This is the HSEval High School Timetable Evaluator.  It");
  HTMLText(html, "accepts an XML file in");
  HTMLJump1(html, "op", "spec", NULL, "XHSTT format");
  HTMLText(html, ", containing any number");
  HTMLText(html, "of instances of high school timetabling problems and groups");
  HTMLText(html, "of solutions to the instances, and evaluates the solutions.");
  HTMLText(html, "It also handles the");
  HTMLJump2(html, "op", "spec", "part", "employee", NULL,
    "XESTT employee scheduling format");
  HTMLText(html, ".  HSEval will time out if it consumes more than 5 minutes");
  HTMLText(html, "of CPU time, as is possible if you try to use it to");
  HTMLText(html, "evaluate very large archives (please don't).");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "No data are stored on this site.  All operations are");
  HTMLText(html, "applied to a single XML file that you upload.  Specify");
  HTMLText(html, "the operation here:");
  HTMLRadioBoxesTabularBegin(html, "op", "summary_html");

  HTMLRadioBoxTabular(html, "report",
    "<b>XML Report.</b>\n"
    "Return a copy of the XML file with a report added to each solution,\n"
    "replacing any existing reports.");

  HTMLRadioBoxTabular(html, "summary_html",
    "<b>HTML Summary.</b>\n"
    "Return an HTML page containing tables summarizing the instances and\n"
    "solutions in the file.");

  HTMLRadioBoxTabular(html, "summary_latex",
    "<b>LaTeX Summary.</b>  Like the previous operation except that the\n"
    "tables are returned in LaTeX format.");

  HTMLRadioBoxTabular(html, "ranking_html",
    "<b>HTML Ranking.</b>\n"
    "Return an HTML page which ranks the solution groups in the file\n"
    "following the rules of the Third International Timetabling Competition.");

  HTMLRadioBoxTabular(html, "ranking_latex",
    "<b>LaTeX Ranking.</b>  Like the previous operation except that the\n"
    "tables are returned in LaTeX format.");

  HTMLRadioBoxTabular(html, "report_html",
    "<b>HTML Report.</b>\n"
    "Return an HTML page containing a detailed report on each solution.\n"
    "This is mainly useful when there are only a few solutions in the\n"
    "archive, since otherwise the page returned can be very long.");

  HTMLRadioBoxTabular(html, "timetables_html",
    "<b>HTML Timetables.</b>\n"
    "Return an HTML page containing, for each solution, a timetable for\n"
    "each resource, each with a table of violated constraints that apply\n"
    "to that resource.  Again, this page is mainly useful when there are\n"
    "only a few solutions; and it only works on instances that contain Day\n"
    "time subgroups.");

  /* ***
  HTMLRadioBoxTabular(html, "timetables_html_long",
    "<b>HTML Timetables (Long).</b>\n"
    "Like the previous operation, except that timetables for all event\n"
    "groups that have constraints are added.  This page is very long\n"
    "and is useful mainly for checking HSEval's report on a single solution.");
  *** */

  HTMLRadioBoxTabular(html, "planning_html",
    "<b>HTML Planning Timetables.</b>\n"
    "Return an HTML page containing, for each solution, one planning\n"
    "timetable for each resource type of its instance.  A planning\n"
    "timetable is a single large table with one column per time and\n"
    "one row per resource.");

  /* ***
  HTMLRadioBoxTabularWithTextBox(html, "planning_html",
    "<b>HTML Planning Timetables.</b>\n"
    "Return an HTML page containing, for each solution, one planning\n"
    "timetable for each resource type of its instance.  A planning\n"
    "timetable is a single large table with one column per time and\n"
    "one row per resource.  You can optionally highlight the parts of\n"
    "the timetable that concern some constraints by typing any substring\n"
    "of the constraints' names here:", "defects", "");
  *** */

  HTMLRadioBoxTabular(html, "planning_html_highlit",
    "<b>HTML Planning Timetables (Highlighted).</b>\n"
    "Like the previous operation, except that clashes and split and partial\n"
    "resource assignments are highlighted.");

  HTMLRadioBoxTabular(html, "avail_html",
    "<b>HTML Availability Report.</b>\n"
    "Return an HTML page showing, for each solution, how the availability\n"
    "of each resource (as presented in planning timetables) is calculated.");

  HTMLRadioBoxesTabularEnd(html);
  HTMLText(html, "If you have selected one of the timetable display options,");
  HTMLText(html, "you may choose to highlight the parts monitored by certain");
  HTMLText(html, "constraints, by typing a substring of the name or Id of the");
  HTMLText(html, "constraints in this box: ");
  HTMLTextBoxMedium(html, "constraints", "");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Specify the XML file here, then press Submit.  If the file");
  HTMLText(html, "is not in the right format, an HTML page will be returned");
  HTMLText(html, "giving the point of the first error.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLFileUploadBox(html, "file");
  HTMLParagraphEnd(html);

  HTMLSubmitButton(html, "Submit");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "HSEval is part of a wider project devoted to making");
  HTMLText(html, "instances and solutions of high school timetabling");
  HTMLText(html, "problems from around the world available in a standard");
  HTMLText(html, "format.  For further information about this project,");
  HTMLText(html, "consult");
  HTMLJumpToDocument(html, "http://www.utwente.nl/ewi/dmmp/gerhardpost/",
    NULL, "Gerhard Post");
  HTMLText(html, "'s");
  HTMLJumpToDocument(html, "http://www.utwente.nl/ctit/hstt/",
    NULL, "XHSTT web page");
  HTMLText(html, ".  HSEval itself was written and is maintained by");
  HTMLJumpToDocument(html, "http://jeffreykingston.id.au", NULL,
    "Jeffrey H. Kingston");
  HTMLText(html, ".  It uses his ");
  HTMLJumpToDocument(html, "http://jeffreykingston.id.au/khe",
    NULL, "KHE high school timetabing engine");
  HTMLText(html, ", a free, open source C library for reading and solving");
  HTMLText(html, "high school timetabling and employee scheduling problems.");
  HTMLText(html, "This web site is hosted on a server maintained by a web");
  HTMLText(html, "hosting company and paid for by Jeffrey H. Kingston.");
  HTMLJump1(html, "op", "timeout", NULL, "[timeout test]");
  HTMLParagraphEnd(html);

  HTMLFormEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  getrlimit(RLIMIT_CORE, &rlim_rec);
  rlim_rec.rlim_cur = rlim_rec.rlim_max;
  status = setrlimit(RLIMIT_CORE, &rlim_rec);
  HTMLTextWithInt(html, "Core rlim_cur: %lu.", rlim_rec.rlim_cur);
  HTMLTextWithInt(html, "Core rlim_max: %lu.", rlim_rec.rlim_max);
  HTMLTextWithInt(html, "RLIM_INFINITY: %llu.", RLIM_INFINITY);
  HTMLTextWithInt(html, "sizeof(rlim_rec.rlim_max): %d.",
    sizeof(rlim_rec.rlim_max));
  HTMLTextWithInt(html, "status: %d.", status);
  HTMLParagraphEnd(html);
  *** */

  PageEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void Spec(COMMAND c)                                                     */
/*                                                                           */
/*  Print one section of the specification of the XML file format.           */
/*                                                                           */
/*****************************************************************************/

static void Spec(COMMAND c)
{
  char *op, *val;  FILE *junk;
  /* op = CommandNextOp(c); */
  if( !CommandNextPair(c, &op, &val, &junk) || strcmp(op, "part") != 0 )
  /* if( op == NULL || strcmp(op, "part") != 0 ) */
    SpecMain();
  else
  {
    /* val = CommandNextOneLineVal(c); */
    if( val == NULL || strcmp(val, "main") == 0 )
      SpecMain();
    else if( strcmp(val, "constraints") == 0 )
      ConstraintSpecFull();
    else if( strcmp(val, "employee") == 0 )
      EmployeeSpecFull();
    else if( strcmp(val, "glossary") == 0 )
      Glossary();
    /* ***
    else if( strcmp(val, "availab ility") == 0 )
      Availability();
    *** */
    else
      CommandError(c, "request to display unknown part of the specification");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CGIVerify(void)                                                     */
/*                                                                           */
/*  Execute the Verify CGI operation.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void CGIVerify(CGI cgi)
{
  char *str;  XML_ELT archive_elt;  ARCHIVE archive;

  ** read and verify the file **
  if( !CGIRetrieve(cgi, "file", &str) || str[0] == '\0' )
    Error(cgi, "uploaded file was empty");
  XMLSetErrorHandler(&Error);
  archive_elt = XMLReadString(str);
  archive = BuildArchive(archive_elt, "hseval");

  ** if we get this far, everything must be all right **
  CGISuccess();
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool SolnGroupContainsInvalidSolns(KHE_SOLN_GROUP soln_group)            */
/*                                                                           */
/*  Return true if soln_group contains at least one invalid solution.        */
/*                                                                           */
/*****************************************************************************/

static bool SolnGroupContainsInvalidSolns(KHE_SOLN_GROUP soln_group)
{
  int i;  KHE_SOLN soln;
  for( i = 0;  i < KheSolnGroupSolnCount(soln_group);  i++ )
  {
    soln = KheSolnGroupSoln(soln_group, i);
    if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ArchiveContainsInvalidSolns(KHE_ARCHIVE archive)                    */
/*                                                                           */
/*  Return true if archive contains at least one invalid solution.           */
/*                                                                           */
/*****************************************************************************/

bool ArchiveContainsInvalidSolns(KHE_ARCHIVE archive)
{
  KHE_SOLN_GROUP soln_group;  int i;
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    if( SolnGroupContainsInvalidSolns(soln_group) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveReportInvalidSolns(KHE_ARCHIVE archive, HTML html)           */
/*                                                                           */
/*  Report on invalid solutions.                                             */
/*                                                                           */
/*****************************************************************************/

void ArchiveReportInvalidSolns(KHE_ARCHIVE archive, HTML html)
{
  KHE_SOLN_GROUP soln_group;  KHE_SOLN soln;  int i, j;  char buff[500];
  KML_ERROR ke;
  if( ArchiveContainsInvalidSolns(archive) )
  {
    /* header and explanatory paragraph */
    HTMLParagraphBegin(html);
    HTMLHeading(html, "Invalid solutions");
    HTMLParagraphEnd(html);

    HTMLParagraphBegin(html);
    HTMLText(html, "For each invalid solution, this listing shows its");
    HTMLText(html, "index within its solution group and the position and");
    HTMLText(html, "nature of its first error.");
    HTMLParagraphEnd(html);

    /* one paragraph for each solution group with an invalid soln */
    for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
    {
      soln_group = KheArchiveSolnGroup(archive, i);
      if( SolnGroupContainsInvalidSolns(soln_group) )
      {
	/* soln group header */
	HTMLParagraphBegin(html);
	HTMLBoldBegin(html);
	HTMLText(html, "In solution group");
	HTMLTextNoBreak(html, KheSolnGroupId(soln_group));
	HTMLText(html, ":");
	HTMLBoldEnd(html);

	/* one line for each invalid soln */
	for( j = 0;  j < KheSolnGroupSolnCount(soln_group);  j++ )
	{
	  soln = KheSolnGroupSoln(soln_group, j);
	  if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
	  {
	    HTMLNewLine(html);
	    ke = KheSolnInvalidError(soln);
	    snprintf(buff, 500, "Solution %d (line %d col %d: %s)", j + 1,
	      KmlErrorLineNum(ke), KmlErrorColNum(ke), KmlErrorString(ke));
	    HTMLText(html, buff);
	  }
	}
	HTMLParagraphEnd(html);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE ReadAndVerifyArchive(CGI cgi, bool allow_invalid_solns,      */
/*    KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)                              */
/*                                                                           */
/*  If this function returns at all, its return value is the archive         */
/*  stored in cgi, assumed to be the next op and value.                      */
/*                                                                           */
/*  If allow_invalid_solns is true, some of the solutions may be invalid.    */
/*  If placeholder_solns is true, all solutions will be placeholder solns.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void soln_fn(KHE_SOLN soln, void *impl)
{
  if( !KheSolnIsPlaceholder(soln) )
    KheSolnReduceToPlaceholder(soln);
}
*** */

KHE_ARCHIVE ReadAndVerifyArchive(COMMAND c, bool allow_invalid_solns,
  bool resource_type_partitions, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)
{
  char *op, *val;  KHE_ARCHIVE archive;  KML_ERROR ke;  FILE *fp;
  if( !CommandNextPair(c, &op, &val, &fp) || strcmp(op, "file") )
  /* ***
  op = CommandNextOp(c);
  if( op == NULL || strcmp(op, "file") != 0 )
  *** */
    CommandError(c, "uploaded file expected but missing");
  /* ***
  if( !KheArch iveReadIncremental(fp, as, &archive, &ke, false, false, false,
	false, allow_invalid_solns, CommandRerunFile(c), NULL, NULL, NULL,
	NULL, placeholder_solns ? soln_fn : NULL, NULL) )
  *** */
  if( !KheArchiveRead(fp, as, &archive, &ke, false, resource_type_partitions,
	false, allow_invalid_solns, soln_type, CommandRerunFile(c)) )
    CommandError(c, "line %d col %d: %s", KmlErrorLineNum(ke),
      KmlErrorColNum(ke), KmlErrorString(ke));
  return archive;
}


/*****************************************************************************/
/*                                                                           */
/*  void Report(COMMAND c, bool txt_suffix, HA_ARENA_SET as)                 */
/*                                                                           */
/*  Execute the Report command.  The result file is to have ".xml" for       */
/*  its suffix unless txt_suffix is true, in which case it has ".txt".       */
/*                                                                           */
/*****************************************************************************/

static void Report(COMMAND c, bool txt_suffix, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;
  archive = ReadAndVerifyArchive(c, false, false, KHE_SOLN_ORDINARY, as);
  fprintf(stdout, "Content-Type: text/%s\n", txt_suffix ? "plain" : "xml");
  fprintf(stdout, "Content-Disposition: attachment; filename=hseval.%s\n\n",
    txt_suffix ? "txt" : "xml");
  KheArchiveWrite(archive, true, stdout);
}


/*****************************************************************************/
/*                                                                           */
/*  void Summary(COMMAND c, PRINT_FORMAT print_format, HA_ARENA_SET as)      */
/*                                                                           */
/*  Execute the Summary operation, making a table in format print_format.    */
/*                                                                           */
/*****************************************************************************/

static void Summary(COMMAND c, PRINT_FORMAT print_format, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;
  archive = ReadAndVerifyArchive(c, true, false, KHE_SOLN_BASIC_PLACEHOLDER,as);
  SummaryEvalPrint(archive, print_format, as);
}


/*****************************************************************************/
/*                                                                           */
/*  void Ranking(COMMAND c, PRINT_FORMAT print_format, HA_ARENA_SET as)      */
/*                                                                           */
/*  Execute the Ranking operation, with the result in print_format.          */
/*                                                                           */
/*****************************************************************************/

static void Ranking(COMMAND c, PRINT_FORMAT print_format, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;
  archive = ReadAndVerifyArchive(c, true, false, KHE_SOLN_BASIC_PLACEHOLDER,as);
  RankingEvalPrint(archive, print_format, as);
}


/*****************************************************************************/
/*                                                                           */
/*  void ReportHTML(COMMAND c, HA_ARENA_SET as)                              */
/*                                                                           */
/*  Return a HTML page containing a detailed report on each solution.        */
/*                                                                           */
/*****************************************************************************/

static void ReportHTML(COMMAND c, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;  HTML html;  char buff[100];
  archive = ReadAndVerifyArchive(c, true, false, KHE_SOLN_ORDINARY, as);
  snprintf(buff, 100, "%s Evaluations Summary",
    KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 60 ?
    KheArchiveId(archive) : "HSeval");
  html = PageBegin(buff);
  HTMLBigHeading(html, buff);

  HTMLParagraphBegin(html);
  HTMLText(html, "This page contains detailed evaluations of the");
  HTMLText(html, "solutions in the uploaded XML archive.");
  HTMLParagraphEnd(html);

  ArchiveReportHTML(archive, html, as);

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void TimetablesHTML(COMMAND c, bool with_event_groups, HA_ARENA_SET as)  */
/*                                                                           */
/*  Return an HTML page containing timetables for each solution.             */
/*  There is always a timetable for each resource; if with_event_groups      */
/*  is true, there is a timetable for each constrained event group as well.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void TimetablesHTML(COMMAND c, bool with_event_groups, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;  HTML html;  char buff[100];
  archive = ReadAndVer ifyArchive(c, true, KHE_SOLN_ORDINARY, as);
  snprintf(buff, 100, "%s Solution Timetables",
    KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 60 ?
    KheArchiveId(archive) : "HSeval");
  html = PageBegin(buff);
  HTMLBigHeading(html, buff);

  ArchiveTimetablesHTML(archive, with_event_groups, html, as);

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CGIPlanning TimetablesHTML(CGI cgi, bool highlight_splits)          */
/*                                                                           */
/*  Print planning timetables for the solutions of the archive.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void CGIPlann ingTimetablesHTML(CGI cgi, bool highlight_splits)
{
  KHE_ARCHIVE archive;  HTML html;  char buff[100];
  archive = CGIReadAndV erifyArchive(cgi, true, false);
  snprintf(buff, 100, "%s Planning Timetables",
    KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 60 ?
    KheArchiveId(archive) : "HSeval");
  html = PageBegin(buff);
  HTMLBigHeading(html, buff);

  HTMLParagraphBegin(html);
  HTMLText(html, "For each solution in the uploaded XML archive, this page");
  HTMLText(html, "contains one planning timetable for each resource type");
  HTMLText(html, "of the corresponding instance.  The Avai l column, where");
  HTMLText(html, "present, shows the remaining unused workload, when the");
  HTMLText(html, "the resource has a required limit workload constraint.");
  HTMLText(html, "In Unassigned rows it shows the unassigned workload.");
  HTMLText(html, "Gre y boxes denote required unavailable times.");
  HTMLText(html, "Italic font denotes a missing, partial, or split");
  HTMLText(html, "assignment, whether or not an avoid split assignments");
  HTMLText(html, "constraint applies.  The times are reordered heuristically");
  HTMLText(html, "to bring split events together.");
  HTMLParagraphEnd(html);

  ArchivePlanningTimetablesHTML(archive, highlight_splits, html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CommandRun(COMMAND c, HA_ARENA_SET as, HA_ARENA a)                  */
/*                                                                           */
/*  Execute c.                                                               */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  COMMAND_DISPLAY,
  COMMAND_SPEC,
  COMMAND_TIMEOUT,
  COMMAND_REPORT,
  COMMAND_SUMMARY_HTML,
  COMMAND_SUMMARY_LATEX,
  COMMAND_RANKING_HTML,
  COMMAND_RANKING_LATEX,
  COMMAND_REPORT_HTML,
  COMMAND_TIMETABLES_HTML,
  COMMAND_TIMETABLES_HTML_LONG,
  COMMAND_PLANNING_HTML,
  COMMAND_PLANNING_HTML_HIGHLIT,
  COMMAND_AVAIL_HTML
} COMMAND_TYPE;

static void CommandRun(COMMAND c, HA_ARENA_SET as)
{
  char *op, *val, *constraints_str;  FILE *junk;  COMMAND_TYPE ct;
  if( DEBUG1 )
    fprintf(stderr, "[ CommandRun()\n");

  /* get the first command and set ct to its value */
  if( !CommandNextPair(c, &op, &val, &junk) || strcmp(op, "op") != 0 )
  {
    DisplayFrontPage();
    return;
  }

  if( val == NULL || strcmp(val, "display") == 0 )
    ct = COMMAND_DISPLAY;
  else if( strcmp(val, "spec") == 0 )
    ct = COMMAND_SPEC;
  else if( strcmp(val, "timeout") == 0 )
    ct = COMMAND_TIMEOUT;
  else if( strcmp(val, "report") == 0 )
    ct = COMMAND_REPORT;
  else if( strcmp(val, "summary_html") == 0 )
    ct = COMMAND_SUMMARY_HTML;
  else if( strcmp(val, "summary_latex") == 0 )
    ct = COMMAND_SUMMARY_LATEX;
  else if( strcmp(val, "ranking_html") == 0 )
    ct = COMMAND_RANKING_HTML;
  else if( strcmp(val, "ranking_latex") == 0 )
    ct = COMMAND_RANKING_LATEX;
  else if( strcmp(val, "report_html") == 0 )
    ct = COMMAND_REPORT_HTML;
  else if( strcmp(val, "timetables_html") == 0 )
    ct = COMMAND_TIMETABLES_HTML;
  /* ***
  else if( strcmp(val, "timetables_html_long") == 0 )
    ct = COMMAND_TIMETABLES_HTML_LONG;
  *** */
  else if( strcmp(val, "planning_html") == 0 )
    ct = COMMAND_PLANNING_HTML;
  else if( strcmp(val, "planning_html_highlit") == 0 )
    ct = COMMAND_PLANNING_HTML_HIGHLIT;
  else if( strcmp(val, "avail_html") == 0 )
    ct = COMMAND_AVAIL_HTML;
  else
  {
    CommandError(c, "unknown CGI operation \"%s\" requested", val);
    ct = COMMAND_DISPLAY;  /* keep compiler happy */
  }

  /* get the constraints op */
  if( ct >= COMMAND_REPORT )
  {
    if( !CommandNextPair(c, &op, &constraints_str, &junk) ||
	strcmp(op, "constraints") )
      CommandError(c, "constraints string expected but missing");
  }
  else
    constraints_str = NULL;

  /* execute the command */
  switch( ct )
  {
    case COMMAND_DISPLAY:

      DisplayFrontPage();
      break;

    case COMMAND_SPEC:

      Spec(c);
      break;

    case COMMAND_TIMEOUT:

      raise(SIGXCPU);
      break;

    case COMMAND_REPORT:

      Report(c, false, as);
      break;

    case COMMAND_SUMMARY_HTML:

      Summary(c, PRINT_HTML, as);
      break;

    case COMMAND_SUMMARY_LATEX:

      Summary(c, PRINT_LATEX, as);
      break;

    case COMMAND_RANKING_HTML:

      Ranking(c, PRINT_HTML, as);
      break;

    case COMMAND_RANKING_LATEX:

      Ranking(c, PRINT_LATEX, as);
      break;

    case COMMAND_REPORT_HTML:

      ReportHTML(c, as);
      break;

    case COMMAND_TIMETABLES_HTML:

      Timetables(c, false, false, false, constraints_str, as);
      break;

    case COMMAND_TIMETABLES_HTML_LONG:

      Timetables(c, false, false, true, constraints_str, as);
      break;

      /* *** I haven't been able to work out why these two do not terminate
	 *** on large archives.  They are fine on small archives, and fine
	 *** on large archives when run from the command line, and when I
	 *** do a rerun.  So there seems to be no way forward here.
      else if( strcmp(val, "plannin g_html") == 0 )
	PlanningTimetablesHTML(c, false, as);
      else if( strcmp(val, "plan ning_html_highlit") == 0 )
	PlanningTimetablesHTML(c, true, as);
      *** */

    case COMMAND_PLANNING_HTML:

      Timetables(c, true, false, false, constraints_str, as);
      /* PlanningTimetablesHTML2(c, false, as); */
      break;

    case COMMAND_PLANNING_HTML_HIGHLIT:

      Timetables(c, true, true, false, constraints_str, as);
      /* PlanningTimetablesHTML2(c, true, as); */
      break;

    case COMMAND_AVAIL_HTML:

      AvailabilityReportHTML(c, as);
      break;

    default:

      CommandError(c, "unknown command");
      break;
  }
  CommandEnd(c);
  if( DEBUG1 )
    fprintf(stderr, "] CommandRun()\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void Run(void)                                                           */
/*                                                                           */
/*  Run this program as a CGI script.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void Run(HA_ARENA_SET as, HA_ARENA a)
{
  COMMAND c;  char *op, *val;  FILE *junk;
  if( DEBUG1 )
    fprintf(stderr, "[ Run()\n");
  c = Command BeginCGI(HSEVAL_RERUN, &PageBegin, &PageEnd, a);
  if( !CommandNextPair(c, &op, &val, &junk) || strcmp(op, "op") != 0 )
    DisplayFrontPage();
  ** ***
  op = CommandNextOp(c);
  if( op == NULL || strcmp(op, "op") != 0 )
  *** **
  else
  {
    ** val = CommandNextOneLineVal(c); **
    if( val == NULL || strcmp(val, "display") == 0 )
      DisplayFrontPage();
    else if( strcmp(val, "spec") == 0 )
      Spec(c);
    else if( strcmp(val, "timeout") == 0 )
      raise(SIGXCPU);

    ** these are the main options from the bullet list on the front page **
    else if( strcmp(val, "report") == 0 )
      Report(c, false, as);
    else if( strcmp(val, "summary_html") == 0 )
      Summary(c, PRINT_HTML, as);
    else if( strcmp(val, "summary_latex") == 0 )
      Summary(c, PRINT_LATEX, as);
    else if( strcmp(val, "ranking_html") == 0 )
      Ranking(c, PRINT_HTML, as);
    else if( strcmp(val, "ranking_latex") == 0 )
      Ranking(c, PRINT_LATEX, as);
    else if( strcmp(val, "report_html") == 0 )
      ReportHTML(c, as);
    else if( strcmp(val, "timetables_html") == 0 )
      TimetablesHTML(c, false, as);
    else if( strcmp(val, "timetables_html_long") == 0 )
      TimetablesHTML(c, true, as);
    else if( strcmp(val, "planning_html") == 0 )
      PlanningTimetablesHTML2(c, false, as);
    else if( strcmp(val, "planning_html_highlit") == 0 )
      PlanningTimetablesHTML2(c, true, as);
    else
      CommandError(c, "unknown CGI operation \"%s\" requested", val);
  }
  CommandEnd(c);
  if( DEBUG1 )
    fprintf(stderr, "] Run()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void Help(int indent, FILE *fp)                                          */
/*                                                                           */
/*  Print a help message with the given indent onto fp.                      */
/*                                                                           */
/*****************************************************************************/
#define p(str) fprintf(fp, "%*s%s\n", indent, "", str)
#define p2(str1, str2) fprintf(fp, "%*s%s%s\n", indent, "", str1, str2)

static void Help(int indent, FILE *fp)
{
  p("");
  p("HSEval Usage");
  p("");
  p("With no command line arguments this program runs as a CGI script,");
  p("reading from stdin or the QUERY_STRING environment variable in");
  p("the usual way, and printing an XML file or HTML page to stdout as");
  p("appropriate.  With command line arguments it behaves like this:");
  p("");
  p("  hseval -c ...");
  p("");
  p("    Mimic CGI script behaviour.  The details are not documented,");
  p("    but, for example, \"hseval -c op:spec\" prints the specification");
  p("    page on stdout, in HTML as usual.");
  p("");
  p("  hseval -r <archive>");
  p("");
  p("    Read <archive>, check that it is legal XML, delete any");
  p("    existing reports, add a report generated by HSEval to");
  p("    each solution, and write the resulting modified version");
  p("    of the original XML file to standard output.");
  p("");
  p("  hseval -t<options> <archive>");
  p("");
  p("    Read <archive>, check that it is legal XML, and produce a");
  p("    tabular summary of its contents, depending on <options>,");
  p("    which is a sequence of these characters in any order:");
  p("");
  p("    h     Produce a table in HTML format");
  p("    l     Produce a table in LaTeX format");
  p("    c     Include solution cost columns in the table");
  p("    r     Include relative solution cost columns in the table");
  p("    t     Include running time columns in the table");
  p("    s     Show costs as integer soft costs only");
  p("    a     Add an average row at the foot of the table");
  p("    A     Interpret the rest of <options> as an instance name, and");
  p("            add an average row after that instance");
  p("    b     Add a number of best solutions row at the foot of the table");
  p("");
  p("  hseval -u");
  p("");
  p("    Print this usage message.");
  p("");
  p("  hseval -v");
  p("");
  p("    Print version, copyright, author, and license information.");
  p("");
  p("Here <archive> stands for one or more command line arguments.  The");
  p("first is the name of an XML timetable archive file, beginning with");
  p("either <HighSchoolTimetableArchive> or <EmployeeScheduleArchive>.");
  p("This may be followed by these command line arguments, in any order:");
  p("");
  p("  -x<id>{,<id>}       Delete instances with these Ids");
  p("  -i<id>{,<id>}       Include only instances with these Ids");
  p("  -X<id>{,<id>}       Delete solution groups with these Ids");
  p("  -I<id>{,<id>}       Include only solution groups with these Ids");
  p("");
  p("When an instance is omitted, all its solutions are omitted too.");
  p("The -x and -i arguments may not be used together.  The -X and -I");
  p("arguments may not be used together.  As a special case, -X without");
  p("ids deletes all solution groups.  These changes occur only in the");
  p("in-memory copy, not in the file.  They are done before the archive");
  p("is used for anything else (reported on, tabularized, etc.).");
}


/*****************************************************************************/
/*                                                                           */
/*  void Version(int indent, FILE *fp)                                       */
/*                                                                           */
/*  Print version, copright, author, and license information.                */
/*                                                                           */
/*****************************************************************************/

static void Version(int indent, FILE *fp)
{
  p("");
  p2("HSEval ", KheVersionBanner());
  p("COPYRIGHT (C) 2009, Jeffrey H. Kingston");
  p("");
  p("Jeffrey H. Kingston (jeff@it.usyd.edu.au)");
  p("School of Information Technologies");
  p("The University of Sydney 2006");
  p("Australia");
  p("");
  p("This program is free software; you can redistribute it and/or");
  p("modify it under the terms of the GNU General Public License as");
  p("published by the Free Software Foundation; either Version 3, or");
  p("(at your option) any later version.");
  p("");
  p("This program is distributed in the hope that it will be useful,");
  p("but WITHOUT ANY WARRANTY; without even the implied warranty of");
  p("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  p("GNU General Public License for more details.");
  p("");
  p("You should have received a copy of the GNU General Public License");
  p("with this program; if not, write to the Free Software Foundation,");
  p("Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA.");
  p("");
}


/*****************************************************************************/
/*                                                                           */
/*  FILE *OpenFile(char *fname)                                              */
/*                                                                           */
/*  Open file, possibly use stdin, possibly abort.                           */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static FILE *OpenFile(char *fname)
{
  FILE *fp;
  if( fname == NULL )
  {
    ** no file name means read from stdin **
    fp = stdin;
  }
  else
  {
    ** read from fname **
    fp = fopen(fname, "r");
    if( fp == NULL )
    {
      fprintf(stderr, "hseval: cannot open file \"%s\" for reading\n", fname);
      exit(1);
    }
  }
  return fp;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void WriteTable(KHE_ARCHIVE archive, char *options, HA_ARENA_SET as)     */
/*                                                                           */
/*  Write a tabular summary of archive according to options.                 */
/*                                                                           */
/*****************************************************************************/

static void WriteTable(KHE_ARCHIVE archive, char *options, HA_ARENA_SET as)
{
  PRINT_FORMAT fmt;
  bool show_costs, show_relative, show_times, show_averages, show_best_count;
  bool non_empty, soft_costs_only, average_costs;  TABLE table;  HTML html;
  char *p; HA_ARENA a;  char *instance_id;

  /* get the options */
  fmt = PRINT_LATEX;
  show_costs = show_times = soft_costs_only = average_costs = false;
  show_relative = show_averages = show_best_count = false;
  instance_id = NULL;
  for( p = options;  *p != '\0';  p++ ) switch( *p )
  {
    case 'h':

      fmt = PRINT_HTML;
      break;

    case 'l':

      fmt = PRINT_LATEX;
      break;

    case 'c':

      show_costs = true;
      break;

    case 'r':

      show_relative = true;
      break;

    case 't':

      show_times = true;
      break;

    case 's':

      soft_costs_only = true;
      break;

    case 'd':

      average_costs = true;
      break;

    case 'a':

      show_averages = true;
      break;

    case 'A':

      show_averages = true;
      instance_id = p + 1;
      do p++; while( *p != '\0' );
      p--;
      break;

    case 'b':

      show_best_count = true;
      break;

    default:

      HnFatal("HSEval:  unknown -t option \'%c\'", *p);
      break;
  }

  /* generate the table */
  a = HaArenaMake(as);
  /* a = HaAren aMake(); */
  table = EvalTableBuild(archive, show_costs, soft_costs_only, average_costs,
    show_relative, show_times, show_averages, instance_id, show_best_count,
    &non_empty, a);
  /* ***
  if( show_times )
    table = RunningTimeTableBuild(archive, show_averages, &non_empty);
  else if( show_costs )
    table = EvalTable Build(archive, soft_costs_only, show_averages);
  else
    HnFatal("HSEval:  in -t, neither c nor t option specified");
  *** */

  /* and print it */
  switch( fmt )
  {
    case PRINT_HTML:

      html = PageBegin("");
      TablePrintHTML(table, html);
      break;

    case PRINT_LATEX:

      TablePrintLaTeX(table, stdout);
      break;

    default:

      HnAbort("HSEval internal error (fmt = %d)", fmt);
      break;
  }
  HaArenaDelete(a);
  /* HaArenaD elete(a); */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE ReadArchive(char *file_name)                                 */
/*                                                                           */
/*  Read archive file file_name and return the archive.  Abort on error.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_ARCHIVE ReadArchive(char *file_name)
{
  FILE *fp;  KHE_ARCHIVE res;  KML_ERROR ke;
  fp = OpenFile(file_name);
  if( fp == NULL )
    HnFatal("khe: cannot open archive file %s", file_name);
  if( !KheArchi veRead(fp, &res, &ke, false, false, false, NULL) )
    HnFatal("%s:%d:%d: %s", file_name, KmlErrorLineNum(ke),
      KmlErrorColNum(ke), KmlErrorString(ke));
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Main program                                                             */
/*                                                                           */
/*****************************************************************************/

int main(int argc, char *argv[])
{
  KHE_ARCHIVE archive;  struct sigaction sig_act;  struct rlimit rlim_rec;
  int pos;  HA_ARENA a;  HA_ARENA_SET as;  COMMAND c;

  if( DEBUG1 )
    fprintf(stderr, "[ main(%d)\n", argc);

  /* set signal handler for timeout */
  sigaction(SIGXCPU, NULL, &sig_act);
  sig_act.sa_handler = &TimeOut;
  sigaction(SIGXCPU, &sig_act, NULL);

  /* set time limit */
  getrlimit(RLIMIT_CPU, &rlim_rec);
  rlim_rec.rlim_cur = HSEVAL_TIME_LIMIT;
  setrlimit(RLIMIT_CPU, &rlim_rec);

  /* allow core dump */
  getrlimit(RLIMIT_CORE, &rlim_rec);
  rlim_rec.rlim_cur = rlim_rec.rlim_max;
  setrlimit(RLIMIT_CORE, &rlim_rec);

  /* make an arena and an arena set */
  as = HaArenaSetMake();
  a = HaArenaMake(as);

  /* do what the command line arguments request */
  if( argc == 1 )
  {
    /* no command line arguments means run as CGI script */
    c = CommandBeginCGI(HSEVAL_RERUN, &PageBegin, &PageEnd, a);
    CommandRun(c, as);
  }
  else
  {
    /* the first command line argument gives the operation (-c, -r etc.) */
    if( argv[1][0] != '-' )
      HnFatal("hseval: first command line argument \"%s\" does not begin "
	"with \"-\"", argv[1]);
    switch( argv[1][1] )
    {
      case 'c':

	c = CommandBeginCommandLine(argv, argc, 2, a);
	CommandRun(c, as);
	break;

      case 'r':

	if( argc == 2 )
	  HnFatal("hseval: missing archive file name after -r");
	pos = 2;
        archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as, false,
	  false, false, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
	if( pos != argc )
          HnFatal("hseval: unexpected command line argument \"%s\"", argv[pos]);
	KheArchiveWrite(archive, true, stdout);
	/* *** withdrawn
	if( DEBUG_AVAIL )
	{
	  int i, j, k, avail_times;  KHE_SOLN_GROUP sg;  KHE_SOLN soln;
	  KHE_INSTANCE ins;  KHE_RESOURCE r;  float avail_workload;
	  KHE_FRAME frame;
	  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
	  {
	    sg = KheArchiveSolnGroup(archive, i);
	    for( j = 0;  j < KheSolnGroupSolnCount(sg);  j++ )
	    {
	      soln = KheSolnGroupSoln(sg, j);
	      frame = KheFrameMakeCommon(soln);
	      if( frame == NULL )
		frame = KheFrameMakeSingletons(soln);
	      ins = KheSolnInstance(soln);
	      for( k = 0;  k < KheInstanceResourceCount(ins);  k++ )
	      {
		r = KheInstanceResource(ins, k);
		ResourceAvail(soln, r, frame, NULL, &avail_times,
		  &avail_workload);
	      }
	    }
	  }
	}
	*** */
	break;

      case 't':

	if( argc == 2 )
	  HnFatal("hseval: missing archive file name after -t");
	pos = 2;
        archive = KheArchiveReadFromCommandLine(argc, argv, &pos,
	  as, false, false, false, false, KHE_SOLN_ORDINARY, NULL);
	if( pos != argc )
          HnFatal("hseval: unexpected command line argument \"%s\"", argv[pos]);
	WriteTable(archive, &argv[1][2], as);
	break;

      case 'u':

	Help(0, stderr);
	break;

      case 'v':

	Version(0, stderr);
	break;

      default:

	Help(0, stderr);
	fprintf(stderr, "\n");
	HnFatal("hseval: unknown first command line argument \"%s\"", argv[1]);
	break;
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] main()\n");
  exit(0);
}

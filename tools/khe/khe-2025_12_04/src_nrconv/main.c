
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
/*  FILE:         main.c                                                     */
/*  MODULE:       Main module, handles reading and operation dispatch        */
/*                                                                           */
/*****************************************************************************/
#define _POSIX_SOURCE
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include "externs.h"

#define DEBUG1 0
#define DEBUG2 0


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
  p("NRConv: Jeff Kingston's Nurse Rostering Format Converter");
  p("");
  p("  nrconv <options> <instances_or_solns> ... <instances_or_solns>");
  p("");
  p("    Read the files specified by each <instances_or_solns> part, and");
  p("    write to stdout one XESTT archive containing the instances and");
  p("    solutions read, converted into XESTT and stored in the archive");
  p("    in the order they were read.");
  p("");
  p("    For help with the (effectively separate) text file mapping part");
  p("    of NRConv, type \"nrconv -m\"");
  p("");
  p("    The <options> part holds the following optional flags, in any order.");
  p("");
  p("      -v         Print version information and exit");
  p("      -h         Print this help message and exit");
  p("      -o<file>   Write the archive to <file> instead of to stdout");
  p("      -n<name>   Set the archive id and name to <name>");
  p("");
  p("    Each <instances_or_solns> part either specifies some instances to");
  p("    be read, or some solutions.  To specify instances the arrangement is");
  p("");
  p("      -i<model_file> <instance_file> ... <instance_file>");
  p("");
  p("    Here <model_file> is the name of an \"instance model file\"");
  p("    containing information about the instances, including their format.");
  p("    For full details, consult the documentation distributed with NRConv.");
  p("");
  p("    To specify solutions the arrangement is");
  p("");
  p("      -s<model_file> <solution_file> ... <solution_file>");
  p("");
  p("    Here <model_file> is the name of a \"solution model file\"");
  p("    containing information about the solutions, including their format.");
  p("    For full details, consult the documentation distributed with NRConv.");
  p("");
  p("    In some cases, it is not possible to determine from a solution file");
  p("    the instance for which it is supposed to be a solution.  In those");
  p("    cases, use this arrangement:");
  p("");
  p("      -s<model_file>:<instance_file> <solution_file> ... <solution_file>");
  p("");
  p("    The -s flag's value ends with a colon and the name of an instance");
  p("    file (note: the file name, not the instance name) that must have");
  p("    occurred earlier.  Then the following solution files are taken to");
  p("    be solutions of the instance from that file.");
  p("");
  p("    For model INRC2 only, an <instance_file> is actually a sequence of");
  p("    three file names separated by commas without spaces, for example");
  p("");
  p("      <scenario_file>,<history_file>,<week_file>");
  p("");
  p("    As indicated, these name the scenario file, the history data file,");
  p("    and the week data file.  They are combined into a single XESTT");
  p("    instance for one week.");
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
  p2("NRConv ", NrcVersionBanner());
  p("COPYRIGHT (C) 2016, Jeffrey H. Kingston");
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
/*  NRC_ARCHIVE MakeArchive(char *archive_name, char *date_str,              */
/*    HA_ARENA_SET as)                                                       */
/*                                                                           */
/*  Make a new archive with these attributes.                                */
/*                                                                           */
/*****************************************************************************/

static NRC_ARCHIVE MakeArchive(char *archive_name, char *date_str,
  HA_ARENA_SET as)
{
  NRC_ARCHIVE res;
  /* ***
  NRC_ARCHIVE_METADATA archive_md;
  archive_md = NrcArchiveMetaDataMake(archive_name, "Jeffrey H. Kingston",
    date_str, "Converted from other formats by NRConv", NULL);
  *** */
  res = NrcArchiveMake(archive_name, as);
  NrcArchiveSetMetaData(res, archive_name, "Jeffrey H. Kingston",
    date_str, "Converted from other formats by NRConv", NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *CurrentDate(void)                                                  */
/*                                                                           */
/*  Return the current date in static memory.                                */
/*                                                                           */
/*****************************************************************************/

char *CurrentDate(void)
{
  time_t tm;  int i;  static char buff[50];
  time(&tm);
  ctime_r(&tm, buff);
  for( i = 0;  buff[i] != '\n' && buff[i] != '\0';  i++ );
  buff[i] = '\0';
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  int SolnReportedCost(NRC_SOLN soln)                                      */
/*                                                                           */
/*  Return the reported cost of soln, or 0 if none.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static int SolnReportedCost(NRC_SOLN soln)
{
  char *str;  int res;
  str = NrcSolnDescription(soln);
  if( str != NULL && sscanf(str, "Reported cost %d", &res) == 1 )
    return res;
  else
    return 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void ReduceToBestSolns(NRC_SOLN_GROUP sg)                                */
/*                                                                           */
/*  Reduce sg so that it holds at most one solution for each instance.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void ReduceToBestSolns(NRC_SOLN_GROUP sg)
{
  NRC_ARCHIVE archive;   NRC_INSTANCE ins;  NRC_SOLN soln, best_soln;  int i, j;
  if( DEBUG2 )
    fprintf(stderr, "[ ReduceToBestSolns(%s)\n", NrcSolnGroupId(sg));
  archive = NrcSolnGroupArchive(sg);
  for( i = 0;  i < NrcArchiveInstanceCount(archive);  i++ )
  {
    ins = NrcArchiveInstance(archive, i);
    if( DEBUG2 )
      fprintf(stderr, "  solns to instance %s:\n", NrcInstanceId(ins));
    best_soln = NULL;
    for( j = 0;  j < NrcSolnGroupSolnCount(sg);  j++ )
    {
      soln = NrcSolnGroupSoln(sg, j);
      if( NrcSolnInstance(soln) == ins )
      {
	if( best_soln == NULL )
	{
	  best_soln = soln;
	  if( DEBUG2 )
	    fprintf(stderr, "    init (%d)\n", SolnReportedCost(best_soln));
	}
	else if( SolnReportedCost(soln) < SolnReportedCost(best_soln) )
	{
	  NrcSolnGroupDeleteSoln(sg, best_soln);
	  j--;
	  best_soln = soln;
	  if( DEBUG2 )
	    fprintf(stderr, "    best (%d)\n", SolnReportedCost(best_soln));
	}
	else
	{
	  NrcSolnGroupDeleteSoln(sg, soln);
	  j--;
	  if( DEBUG2 )
	    fprintf(stderr, "    drop (%d)\n", SolnReportedCost(soln));
	}
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] ReduceToBestSolns returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Main program                                                             */
/*                                                                           */
/*****************************************************************************/

typedef HN_TABLE(NRC_INSTANCE) TABLE_NRC_INSTANCE;

int main(int argc, char *argv[])
{
  int i, pos;  FILE *out_fp;  TABLE_NRC_INSTANCE instances;  NRC_INSTANCE ins;
  NRC_ARCHIVE archive;  INSTANCE_MODEL ins_model;  SOLN_MODEL soln_model;
  char *p;  HA_ARENA a;  HA_ARENA_SET as;

  if( DEBUG1 )
  {
    fprintf(stderr, "NRConv [");
    for( i = 0;  i < argc;  i++ )
      fprintf(stderr, " %s", argv[i]);
    fprintf(stderr, "\n");
  }

  /* exit with help message if no command options */
  if( argc == 1 )
  {
    Help(0, stderr);
    exit(1);
  }

  /* initialize various state variables */
  as = HaArenaSetMake();
  a = HaArenaMake(as);
  HnTableInit(instances, a);
  archive = NULL;
  out_fp = NULL;

  /* read the initial <options> */
  i = 1;
  while( i < argc )
  {
    if( argv[i][0] != '-' )
      FatalError(NULL, 0, "%s command option out of place", argv[i]);
    switch( argv[i][1] )
    {
      case 'v':

	/* -v:  print version information and exit */
	Version(0, stderr);
	exit(0);
	break;

      case 'h':

	/* -h:  print help information and exit */
	Help(0, stderr);
	exit(0);
	break;

      case 'o':

	/* -o<file>:  Write the archive to <file> instead of to stdout */
	if( out_fp != NULL )
	  FatalError(NULL, 0, "-o<file> command option given twice");
	if( argv[i][2] == '\0' )
	  FatalError(NULL, 0, "-o<file> command option has no <file>");
	out_fp = fopen(&argv[i][2], "w");
	if( out_fp == NULL )
	  FatalError(NULL, 0, "cannot write to %s", argv[i][2]);
	i++;
	break;

      case 'n':

	/* -n<name>:  Set the archive id and name to <name> */
	if( archive != NULL )
	  FatalError(NULL, 0, "-n<name> command option given twice");
	if( argv[i][2] == '\0' )
	  FatalError(NULL, 0, "-n<name> command option has no <name>");
	archive = MakeArchive(&argv[i][2], CurrentDate(), as);
	i++;
	break;

      case 'm':

	/* -m<map_file_name>:  */
	if( strlen(argv[i]) == 2 )
	  HelpMap(0, stderr);
	else
	  Map(&argv[i][2], as);
	exit(0);
	break;

      default:

	goto CONT;
	break;
    }
  }
  CONT:

  /* read the <instances_or_solns> parts */
  while( i < argc )
  {
    if( argv[i][0] != '-' )
      FatalError(NULL, 0, "expected -i, or -s where %s appears", argv[i]);
    switch( argv[i][1] )
    {
      case 'i':

	/* -i<model_file> <instance_file> ... <instance_file> */
	if( argv[i][2] == '\0' )
	  FatalError(NULL, 0, "-i<model_file> option has no <model_file>");
	ins_model = InstanceModelMake(&argv[i][2], a);
	for( i++ ;  i < argc && argv[i][0] != '-';  i++ )
	{
	  if( HnTableRetrieve(instances, argv[i], ins, pos) )
	    FatalError(NULL, 0, "instance file name %s appears twice", argv[i]);
	  if( archive == NULL )
	    archive = MakeArchive("NRConvArchive", CurrentDate(), as);
	  ins = InstanceModelConvertInstance(ins_model, argv[i], as);
	  if( ins != NULL )
	  {
	    HnTableAdd(instances, argv[i], ins);
	    if( !NrcArchiveAddInstance(archive, ins) )
	      FatalError(NULL, 0, "two instances with the same Id (%s)",
		NrcInstanceId(ins));
	  }
	}
	break;

      case 's':

	/* -s<model_file>[:<instance_file>] <soln_file> ... <soln_file> */
	if( argv[i][2] == '\0' )
	  FatalError(NULL, 0, "-s<model_file> option has no <model_file>");
	if( archive == NULL )
          FatalError(NULL, 0, "-s option precedes first -i option");

	/* sort out the optional <instance_file> */
	p = strstr(&argv[i][2], ":");
	if( p == NULL )
	  ins = NULL;
	else
	{
	  *(p++) = '\0';
	  if( *p == '\0' )
	    FatalError(NULL, 0, "missing instance file name in option %s:",
	      argv[i]);
	  if( !HnTableRetrieve(instances, p, ins, pos) )
	    FatalError(NULL, 0, "instance file name %s is new in option %s:%s",
	      p, argv[i], p);
	}

	/* sort out the solution model and convert the solutions */
	soln_model = SolnModelMake(&argv[i][2], archive, a);
        for( i++ ;  i < argc && argv[i][0] != '-';  i++ )
	  SolnModelConvertSoln(soln_model, argv[i], ins, archive, as);
	break;

      default:

	FatalError(NULL, 0, "command option %s unknown or out of place",
	  argv[i]);
	break;
    }
  }

  /* wrapup; reduce soln groups to hold only the best solns, and write */
  if( archive != NULL && NrcArchiveInstanceCount(archive) > 0 )
  {
    /* ***
    for( i = 0;  i < NrcArchiveSolnGroupCount(archive);  i++ )
      ReduceToBestSolns(NrcArchiveSolnGroup(archive, i));
    *** */
    if( DEBUG1 )
      fprintf(stderr, "    calling NrcArchiveWrite:\n");
    NrcArchiveWrite(archive, false, out_fp != NULL ? out_fp : stdout);
  }
  else
    FatalError(NULL, 0, "no instances to convert");
  if( DEBUG1 )
    fprintf(stderr, "]\n");
  exit(0);
}


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
/*  FILE:         khe_main.c                                                 */
/*  DESCRIPTION:  Tests of KHE                                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_partition.h"
#include "sset.h"
#include <malloc.h>

#define DEBUG_MALLOC 0
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG_STATS 0
#define KHE_SOLE_DIVERSIFIER 0

typedef HA_ARRAY(KHE_INSTANCE) ARRAY_KHE_INSTANCE;
typedef HA_ARRAY(KHE_SOLN_GROUP) ARRAY_KHE_SOLN_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  bool KheDivSameFn(int (*fn)(int, int), int d, int *other_d)              */
/*                                                                           */
/*  If fn(d, *) is the same as fn(a, *) for some a < c, return true with     */
/*  *other_d set to the first such a.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheDivSameFn(int (*fn)(int, int), int cols, int d, int *other_d)
{
  int k, c, same_count;
  for( k = 0;  k < d;  k++ )
  {
    same_count = 0;
    for( c = 1;  c <= cols;  c++ )
      if( fn(d, c) == fn(k, c) )
	same_count++;
    if( same_count == cols )
    {
      *other_d = k;
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDivColTest(int (*fn)(int, int), int cols, int indent, FILE *fp)  */
/*                                                                           */
/*  Print a test of fn onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void KheDivColTest(int (*fn)(int, int), int cols, int *same_count,
  int indent, FILE *fp)
{
  int c, d, other_d, rows;
  fprintf(fp, "%*s[ %d cols\n", indent, "", cols);
  fprintf(fp, "%*s  %3s |", indent, "", "d");
  for( c = 1;  c <= cols;  c++ )
    fprintf(fp, "%3d", c);
  fprintf(fp, "\n");
  fprintf(fp, "%*s  ----+", indent, "");
  rows = 1;
  for( c = 1;  c <= cols;  c++ )
  {
    rows *= c;
    fprintf(fp, "---");
  }
  fprintf(fp, "\n");
  for( d = 0;  d < rows;  d++ )
  {
    fprintf(fp, "%*s  %3d |", indent, "", d);
    for( c = 1;  c <= cols;  c++ )
      fprintf(fp, "%3d", fn(d, c));
    if( KheDivSameFn(fn, cols, d, &other_d) )
    {
      fprintf(fp, "  (same as %d)", other_d);
      (*same_count)++;
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "%*s  ----+", indent, "");
  for( c = 1;  c <= cols;  c++ )
    fprintf(fp, "---");
  fprintf(fp, "\n");
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDivTest(int (*fn)(int, int), char *label, int indent, FILE *fp)  */
/*                                                                           */
/*****************************************************************************/

static void KheDivTest(int (*fn)(int, int), char *label, int max_cols,
  int indent, FILE *fp)
{
  int cols, same_count;
  fprintf(fp, "%*s[ Function %s\n", indent, "", label);
  same_count = 0;
  for( cols = 1;  cols <= max_cols;  cols++ )
  {
    if( cols > 1 )
      fprintf(fp, "\n");
    KheDivColTest(fn, cols, &same_count, indent + 2, fp);
  }
  fprintf(fp, "%*s] (same_count %d)\n", indent, "", same_count);
}


/*****************************************************************************/
/*                                                                           */
/*  Various diversification functions, for testing                           */
/*                                                                           */
/*****************************************************************************/

static int KheMod(int d, int c) { return d % c; }
static int KheOffset3Mod(int d, int c) { return (d + ((c >= 4) && (d >= 12))) % c; }
static int KheFact(int d, int c)
{
  int i, c1f;
  c1f = 1;
  for( i = 1;  i < c && c1f <= d;  i++ )
    c1f *= i;
  return (d / c1f) % c;
}
static int KheFactMod(int d, int c)
{
  int i, c1f;
  c1f = 1;
  for( i = 1;  i < c && c1f <= d;  i++ )
    c1f *= i;
  return ((d / c1f) + (d % c1f)) % c;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDiversificationTest(FILE *fp)                                    */
/*                                                                           */
/*  Test some diversification functions, and show the results on fp.         */
/*                                                                           */
/*****************************************************************************/

static void KheDiversificationTest(int max_cols, FILE *fp)
{
  KheDivTest(&KheMod, "d % c", max_cols, 2, fp);
  fprintf(fp, "\n");
  KheDivTest(&KheOffset3Mod, "(d + ((c >= 4) && (d >= 12))) % c",
    max_cols, 2,fp);
  KheDivTest(&KheFact, "(d / fact(c-1)) % c", max_cols, 2,fp);
  KheDivTest(&KheFactMod, "((d / fact(c - 1)) + (d % fact(c - 1))) % c",
    max_cols, 2,fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUsageMessageAndExit(void)                                        */
/*                                                                           */
/*  Print a usage message and exit.                                          */
/*                                                                           */
/*****************************************************************************/
#define p(s) fprintf(stderr, s "\n")

static void KheUsageMessageAndExit(void)
{
  p("Operations for manipulating archives");
  p("");
  p("  khe -a <archive>           Audit and fix archive, write to stdout");
  p("  khe -e <archive>           Explode archive into one per instance");
  p("  khe -E <archive>           Explode archive into one per country");
  p("  khe -m <id> <archive> ...  Merge archives into one with the given <id>");
  p("  khe -w <archive>           Write archive to stdout, neatly formatted");
  p("");
  p("Operations for solving archives");
  p("");
  p("  khe -r <archive> ...       Solve archive multiple times (see below)");
  p("  khe -R <archive> ...       Solve archive multiple times (see below)");
  p("  khe -s <archive> <opts>    Solve archive once (see below)");
  p("  khe -S <archive> <opts>    Solve archive once (see below)");
  p("");
  p("Operations for testing aspects of KHE");
  p("");
  p("  khe -b                     Test exponential backoff");
  p("  khe -c <archive>           Test KheSolnCopy, KheSolnDelete on archive");
  p("  khe -d                     Test diversification functions");
  p("  khe -n                     Test the SSet module");
  p("  khe -g                     Test the multiset generator module");
  p("  khe [ -u ]                 Write this usage message to stderr");
  p("  khe -v                     Write version number to stderr");
  p("");
  p("where <archive> is an XML timetable archive file, beginning with");
  p("either <HighSchoolTimetableArchive> or <EmployeeScheduleArchive>,");
  p("optionally followed by these options:");
  p("");
  p("  -x<id>{,<id>}              Delete instances with these Ids");
  p("  -i<id>{,<id>}              Include only instances with these Ids");
  p("  -n<int>                    Include only the first <int> instances");
  p("  -X<id>{,<id>}              Delete solution groups with these Ids");
  p("  -I<id>{,<id>}              Include only solution groups with these Ids");
  p("");
  p("Deleting a solution group includes deleting its solutions.  Options");
  p("-x, -i, and -n may not be used together.  Options -X and -I may");
  p("not be used together.  As a special case, -X without ids deletes all");
  p("solution groups.  These modifications occur only in the in-memory copy;");
  p("they do not affect the original file.  They are done before the archive");
  p("is used for anything else (exploded, written, solved, or whatever).");
  p("");
  p("For -s, the <opts> part consists of any number of elements separated");
  p("by spaces.  Each element has the form <key>=<value>, or just <key>,");
  p("which is equivalent to <key>=true.  These pairs go into the options");
  p("table passed to solvers when solving <archive>.  For example,");
  p("");
  p("  khe -s <archive> ps_soln_group=test ps_threads=4 ps_make=8 ps_keep=1");
  p("");
  p("causes the output archive to have a solution group with name test,");
  p("created using 4 threads running in parallel.  For each instance,");
  p("8 solutions are made and the best 1 of those is kept.  Consult the");
  p("User's Guide for the full list of options used by KHE's solvers.");
  p("");
  p("The -r flag allows an archive to be solved multiple times:");
  p("");
  p("  khe -r <archive> <ropts> -s <opts> -s <opts> ...");
  p("");
  p("Each -s causes one solve to occur, with options defined by the union");
  p("of <ropts> with the following <opts>.  If an option appears in both,");
  p("the second occurrence applies.  This form allows several combinations");
  p("of options to be tried, each producing its own solution group.");
  p("");
  p("One can also use -S and -R instead of -s and -r.  The only difference");
  p("is that the resource_type_partitions parameter of the read is set to");
  p("true.  This is recommended when solving.");
  p("");
  p("Furthermore, -s, -S, -r, and -R can be immediately followed by an o,");
  p("as in -Ro for example.  This means that the solutions of the archive");
  p("are to be read as ordinary solutions, suitable for detailed analysis");
  p("and further solving, not merely as writable placeholders.");
  /* ***
  p("");
  p("In all cases one can insert a -o option immediately after the command:");
  p("");
  p("  khe -o <file_name> -r ...");
  p("");
  p("for example.  This causes any output to go to a new file called");
  p("<file_name> rather than to stdout.  But when solving it has a");
  p("further meaning.  After each instance is solved, the result archive");
  p("is written to <file_name>; and before the solve begins, if <file_name>");
  p("exists it replaces <archive> as the archive being read, and any solves");
  p("for which <file_name> already contains at least one solution are not");
  p("done.  In this way, one can interrupt a long solve of many instances");
  p("and restart it without losing very much, unless the interrupt happens");
  p("while the archive is being written.");
  *** */
  exit(1);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP KheTestMakeSolnGroup(KHE_ARCHIVE archive,                 */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Make a new, empty soln group for archive to hold the solns produced      */
/*  by this test run.                                                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_parallel_solve.c ***
static KHE_SOLN_GROUP KheTestMakeSolnGroup(KHE_ARCHIVE archive,
  KHE_OPTIONS options)
{
#if KHE_USE_TIMING
  struct timeval tv;
#endif
  static char id[200], date_str[200], version_str[200];  int i;  char *sg_name;
  KHE_SOLN_GROUP res;

  ** construct an Id that is not already in use **
  sg_name = KheOptionsGet(options, "soln_group", "$$");
  if( strcmp(sg_name, "$$") == 0 )
  {
    sprintf(id, "KHE_%s", KheVersionNumber());
    for( i = 1;  KheArchiveRetrieveSolnGroup(archive, id, &res);  i++ )
      sprintf(id, "KHE_%s_%d", KheVersionNumber(), i);
  }
  else
  {
    sprintf(id, "%s", sg_name);
    for( i = 1;  KheArchiveRetrieveSolnGroup(archive, id, &res);  i++ )
      sprintf(id, "%s_%d", sg_name, i);
  }

  ** construct metadata **
#if KHE_USE_TIMING
  gettimeofday(&tv, NULL);
  strftime(date_str, 100, "%d %B %Y", localtime(&(tv.tv_sec)));
#else
  strcpy(date_str, "no date");
#endif
  sprintf(version_str, "Produced by KHE Version %s", KheVersionNumber());

  ** make and return a new soln group **
  if( !KheSolnGroupMake(archive, id, &res) )
    HnAbort("KheTestMakeSolnGroup internal error");
  KheSolnGroupSetMetaData(res, "Jeffrey H. Kingston",
    date_str[0] == '0' ? &date_str[1] : date_str,
    version_str, NULL, NULL);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheExplodeArchive(KHE_ARCHIVE archive, HA_ARENA_SET as)             */
/*                                                                           */
/*  Write one archive for each instance of archive, containing that          */
/*  instance.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheExplodeArchive(KHE_ARCHIVE archive, HA_ARENA_SET as)
{
  KHE_INSTANCE ins;  KHE_ARCHIVE archive2;
  KHE_SOLN_GROUP soln_group, soln_group2;  KHE_SOLN soln;  int i, j, k;
  FILE *fp;  char fname[500];
  char *a_name, *a_contributor, *a_date, *a_description, *a_remarks;
  char *sg_contributor, *sg_date, *sg_description, *sg_publication, *sg_remarks;
  KheArchiveMetaData(archive, &a_name, &a_contributor, &a_date,
    &a_description, &a_remarks);
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    ins = KheArchiveInstance(archive, i);

    /* make a new archive corresponding to ins and add ins to it */
    archive2 = KheArchiveMake(KheInstanceId(ins), KheArchiveModel(archive), as);
    KheArchiveSetMetaData(archive2, a_name, a_contributor, a_date,
      a_description, a_remarks);
    /* KheArchiveDeleteInstance(archive, ins); */
    KheArchiveAddInstance(archive2, ins);

    /* add each of ins's solutions to the archive */
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      soln_group2 = NULL;
      for( k = 0;  k < KheSolnGroupSolnCount(soln_group);  k++ )
      {
	soln = KheSolnGroupSoln(soln_group, k);
	if( KheSolnInstance(soln) == ins )
	{
	  /* make a soln group to hold this soln if not done already */
	  if( soln_group2 == NULL )
	  {
	    if( !KheSolnGroupMake(archive2, KheSolnGroupId(soln_group),
		  &soln_group2) )
	      HnAbort("KheExplodeArchive internal error");
            KheSolnGroupMetaData(soln_group, &sg_contributor, &sg_date,
	      &sg_description, &sg_publication, &sg_remarks);
            KheSolnGroupSetMetaData(soln_group2, sg_contributor, sg_date,
	      sg_description, sg_publication, sg_remarks);
	    /* ***
	    if( !KheSolnGroupMake(archive2, KheSolnGroupId(soln_group),
		  KheSolnGroupMetaData(soln_group), &soln_group2) )
	      HnAbort("KheExplodeArchive internal error");
	    *** */
	  }

	  /* add soln to this soln group */
	  KheSolnGroupAddSoln(soln_group2, soln);
	}
      }
    }

    /* write the new archive */
    snprintf(fname, 500, "%s.xml", KheInstanceId(ins));
    fp = fopen(fname, "w");
    if( fp == NULL )
    {
      fprintf(stderr, "KheExplodeArchive: cannot write to file %s\n",fname);
      exit(1);
    }
    KheArchiveWrite(archive2, false, fp);
    fclose(fp);

    /* delete the new archive */
    KheArchiveDelete(archive2);   /* added JeffK 11/25 */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheCountryPrefix(char *str)                                        */
/*                                                                           */
/*  Return the prefix of str which denotes the country; concretely, it       */
/*  is that part of the string before the first '-' or '.'.  A new string.   */
/*                                                                           */
/*****************************************************************************/

static char *KheCountryPrefix(char *str, HA_ARENA a)
{
  char *p, *res;
  res = HnStringCopy(str, a);
  for( p = res;  *p != '\0' && *p != '-' && *p != '.';  p++ );
  *p = '\0';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWriteArchiveToName(KHE_ARCHIVE archive, char *name_prefix)       */
/*                                                                           */
/*  Write archive to name_prefix.xml.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheWriteArchiveToName(KHE_ARCHIVE archive, char *name_prefix)
{
  FILE *fp;  char fname[500];
  snprintf(fname, 500, "%s.xml", name_prefix);
  fp = fopen(fname, "w");
  HnAssert(fp != NULL,
    "KheExplodeArchiveByCountry: cannot write to file %s\n", fname);
  KheArchiveWrite(archive, false, fp);
  fclose(fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheExplodeArchiveByCountry(KHE_ARCHIVE archive)                     */
/*                                                                           */
/*  Similar to KheExplodeArchive except that there is one archive            */
/*  per country, not one per instance.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheExplodeArchiveByCountry(KHE_ARCHIVE archive, HA_ARENA_SET as)
{
  KHE_INSTANCE ins;  char *prefix, *prev_prefix;  KHE_ARCHIVE curr_archive;
  char *a_name, *a_contributor, *a_date, *a_description, *a_remarks;
  HA_ARENA a;
  curr_archive = NULL;  prev_prefix = NULL;
  KheArchiveMetaData(archive, &a_name, &a_contributor, &a_date,
    &a_description, &a_remarks);
  a = HaArenaMake(as);
  while( KheArchiveInstanceCount(archive) > 0 )
  {
    ins = KheArchiveInstance(archive, 0);
    KheArchiveDeleteInstance(archive, ins);
    if( KheInstanceId(ins) != NULL )
    {
      prefix = KheCountryPrefix(KheInstanceId(ins), a);

      /* do what needs to be done if prefix is incompatible with prev_prefix */
      if( prev_prefix == NULL || strcmp(prev_prefix, prefix) != 0 )
      {
	/* write curr_archive, if any */
	if( curr_archive != NULL )
	{
          KheWriteArchiveToName(curr_archive, prev_prefix);
	  KheArchiveDelete(curr_archive);   /* added JeffK 11/25 */
	}

	/* start off a new archive */
	/* ***
	md2 = (md == NULL ? NULL : KheArchiveMetaDataMake(prefix,
	  KheArchiveMetaDataContributor(md), KheArchiveMetaDataDate(md),
	  KheArchiveMetaDataDescription(md), KheArchiveMetaDataRemarks(md)));
	*** */
	curr_archive = KheArchiveMake(prefix, KheArchiveModel(archive), as);
	KheArchiveSetMetaData(curr_archive, prefix, a_contributor, a_date,
	  a_description, a_remarks);
      }

      /* add ins to curr_archive */
      KheArchiveAddInstance(curr_archive, ins);
      prev_prefix = prefix;
    }
  }
  if( curr_archive != NULL )
  {
    KheWriteArchiveToName(curr_archive, prev_prefix);
    KheArchiveDelete(curr_archive);   /* added JeffK 11/25 */
  }
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  void GetSolveOptions(KHE_OPTIONS options, int argc,                      */
/*    char *argv[], int start, int *stop)                                    */
/*                                                                           */
/*  Get options from the command-line, starting at start and ending at       */
/*  the first command-line item beginning with -, or else at the end,        */
/*  and add them to options.  Set *stop to the first unprocessed command     */
/*  line option.                                                             */
/*                                                                           */
/*****************************************************************************/

static void GetSolveOptions(KHE_OPTIONS options, int argc,
  char *argv[], int *pos)
{
  char *opt, *p;
  for( ;  *pos < argc && argv[*pos][0] != '-';  (*pos)++ )
  {
    opt = HnStringCopy(argv[*pos], KheOptionsArena(options));
    p = strstr(opt, "=");
    if( p == NULL )
      KheOptionsSet(options, opt, "true");
    else
    {
      *p = '\0';
      KheOptionsSet(options, opt, p + 1);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  FILE *WriteFile(char *file_name)                                         */
/*                                                                           */
/*  Return a file open for writing, either file_name if non-NULL, or         */
/*  else stdout.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static FILE *WriteFile(char *file_name)
{
  FILE *fp;
  if( file_name != NULL )
  {
    fp = fopen(file_name, "w");
    HnAssert(fp != NULL, "khe: cannot open \"%s\" for writing", file_name);
  }
  else
    fp = stdout;
  return fp;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SkipArchiveOnCommandLine(int argc, char *argv[], int *pos)          */
/*                                                                           */
/*  Move *pos over an <archive> on the command line.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void SkipArchiveOnCommandLine(int argc, char *argv[], int *pos)
{
  ** skip <archive> **
  (*pos)++;

  ** skip -x, -i, -n, -X, and -I options **
  while( argv[*pos][0] == '-' && (argv[*pos][1] == 'x' || argv[*pos][1] == 'i'
      || argv[*pos][1] == 'n' || argv[*pos][1] == 'X' || argv[*pos][1] == 'I') )
    (*pos)++;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE GetArchive(int argc, char *argv[], int *pos,                 */
/*    HA_ARENA_SET as, bool audit_and_fix,                                   */
/*    bool resource_type_partitions, bool infer_resource_partitions,         */
/*    bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,      */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Read an archive from the command line with optional timing.              */
/*                                                                           */
/*****************************************************************************/

static KHE_ARCHIVE GetArchive(int argc, char *argv[], int *pos,
  HA_ARENA_SET as, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,
  /* char *out_file_name, */ HA_ARENA a)
{
  KHE_TIMER timer;  /* FILE *fp; */  KHE_ARCHIVE archive;  /* KML_ERROR ke; */

  if( DEBUG4 )
    timer = KheTimerMake("archive_read", KHE_NO_TIME, a);
  /* ***
  if( out_file_name != NULL && (fp = fopen(out_file_name, "r")) != NULL )
  {
    ** use out_file_name as the initial archive **
    if( !KheArchiveRead(fp, as, &archive, &ke, audit_and_fix,
	resource_type_partitions, infer_resource_partitions,
	allow_invalid_solns, soln_type, echo_fp) )
      HnFatal("%s:%d:%d: %s\n", out_file_name, KmlErrorLineNum(ke),
	KmlErrorColNum(ke), KmlErrorString(ke));
    fclose(fp);

    ** skip <archive> and its -n etc. options **
    SkipArchiveOnCommandLine(argc, argv, pos);
  }
  else
  {
  *** */

  /* use the <archive> command line option as the initial archive */
  archive = KheArchiveReadFromCommandLine(argc, argv, pos, as, audit_and_fix,
    resource_type_partitions, infer_resource_partitions, allow_invalid_solns,
    soln_type, echo_fp);
  /* } */
  if( DEBUG4 )
    fprintf(stderr, "  archive read time: %.1f secs\n",
      KheTimerElapsedTime(timer));
  return archive;
}


/*****************************************************************************/
/*                                                                           */
/*  int main(int argc, char *argv[])                                         */
/*                                                                           */
/*  Main program:  read a file, solve each instance, then write.             */
/*                                                                           */
/*****************************************************************************/

int main(int argc, char *argv[])
{
  KHE_ARCHIVE res_archive, archive;  KHE_SOLN soln;  char op, op2;
  bool no_print;  KHE_SOLN_GROUP soln_group;  int i, j, pos /* , s */;
  KHE_OPTIONS options, roptions;  HA_ARENA a;  HA_ARENA_SET as;  char *id;
  /* char *results[20], *out_file_name */;
  KHE_SOLN_TYPE soln_type;

  if( DEBUG_MALLOC )
    mallopt(M_CHECK_ACTION, 3);

  /* get the optional -o argument and set pos */
  /* ***
  if( argc >= 3 && strcmp(argv[1], "-o") == 0 )
  {
    ** khe -o <file_name> **
    out_file_name = argv[2];
    pos = 3;
  }
  else
  {
    out_file_name = NULL;
    pos = 1;
  }
  *** */
  pos = 1;

  /* decide which operation is wanted */
  if( argc == 1 )
    op = 'u', op2 = '\0';
  else if( argc >= 2 && argv[pos][0] == '-' && argv[pos][1] != '\0' )
    op = argv[pos][1], op2 = argv[pos][2];
  else
  {
    fprintf(stderr, "khe: unexpected first option %s\n", argv[pos]);
    op = 'u', op2 = '\0';
  }
  if( DEBUG1 )
    fprintf(stderr, "khe -%c (argc = %d)\n", op, argc);

  /* do the operation */
  as = HaArenaSetMake();
  a = HaArenaMake(as);
  switch( op )
  {
    case 'a':

      /* khe -a <archive> */
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as, true,
	false, false, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
      if( pos != argc )
	HnAbort("khe -a:  unexpected options on command line after <archive>");
      KheArchiveWrite(archive, true, stdout /*WriteFile(out_file_name)*/);
      KheArchiveDelete(archive);  /* added by JeffK 11/25, suggested by Ortiz */
      break;

    case 'b':

      /* khe -b */
      if( argc != 2 )
        KheUsageMessageAndExit();
      KheBackoffTest(stdout /*WriteFile(out_file_name)*/);
      break;

    case 'c':

      /* khe -c <archive> */
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos,
	as, false, false, false, false, KHE_SOLN_ORDINARY, NULL);
      if( pos != argc )
	HnAbort("khe -c:  unexpected options on command line after <archive>");
      for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
      {
	soln_group = KheArchiveSolnGroup(archive, i);
	for( j = 0;  j < KheSolnGroupSolnCount(soln_group);  j++ )
	{
	  soln = KheSolnGroupSoln(soln_group, j);
	  soln = KheSolnCopy(soln, as);
	  KheSolnDelete(soln);  /* added by JeffK 11/25 */
	}
      }
      KheArchiveDelete(archive);  /* added by JeffK 11/25, suggested by Ortiz */
      break;

    case 'd':

      /* khe -d */
      if( argc != 2 )
        KheUsageMessageAndExit();
      KheDiversificationTest(3, stdout /*WriteFile(out_file_name)*/);
      break;

    case 'e':

      /* khe -e <archive> */
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as, false,
	false, false, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
      if( pos != argc )
	HnAbort("khe -e:  unexpected options on command line after <archive>");
      KheExplodeArchive(archive, as);
      KheArchiveDelete(archive);  /* added by JeffK 11/25, suggested by Ortiz */
      break;

    case 'E':

      /* khe -E <archive> */
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as, false,
	false, false, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
      if( pos != argc )
	HnAbort("khe -E:  unexpected options on command line after <archive>");
      KheExplodeArchiveByCountry(archive, as);
      KheArchiveDelete(archive);  /* added by JeffK 11/25, suggested by Ortiz */
      break;

    case 'g':

      /* khe -g */
      KheMulisetGeneratorTest(a);
      break;

    case 'm':

      /* khe -m <id> <archive> ... <archive> */
      if( argc < 4 )
	HnAbort("khe -m:  too few command line arguments");
      pos++;
      id = argv[pos];
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as,
	false, false, false, true, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
      res_archive = KheArchiveMake(id, KheArchiveModel(archive), as);
      KheArchiveSetMetaData(res_archive, id, "khe", KheDateToday(),
	"Meld of other archives", NULL);
      KheArchiveMeld(res_archive, archive);
      KheArchiveDelete(archive);
      while( pos < argc )
      {
	archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as,
	  false, false, false, true, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
	KheArchiveMeld(res_archive, archive);
	KheArchiveDelete(archive);
      }
      KheArchiveWrite(res_archive, false, stdout);
      KheArchiveDelete(res_archive);
      /* ***
      ** khe -m: malloc test **
      fprintf(stderr, "[ malloc test; sizeof(void *) = %ld\n",
	sizeof(void *));
      for( i = 0, s = 1;  i < 20;  i++, s *= 2 )
      {
	results[i] = malloc(s);
	if( i > 0 )
	  fprintf(stderr, "    %p - %p = %ld (%ld words), overhead %ld bytes\n",
	    results[i], results[i-1], results[i] - results[i-1],
	    (results[i] - results[i-1]) / sizeof(void *),
	    (results[i] - results[i-1]) - s/2);
	fprintf(stderr, "  malloc(%d bytes, %ld words) = %p\n", s,
	  s / sizeof(void *), results[i]);
      }
      fprintf(stderr, "]\n");
      exit(0);
      *** */
      break;

    case 'n':

      /* khe -n */
      SSetTest(stdout /*WriteFile(out_file_name)*/);
      break;

    case 'r':
    case 'R':

      /* khe -r <archive> <options> -s <options> -s <options> ... */
      pos++;
      soln_type =(op2=='o' ? KHE_SOLN_ORDINARY : KHE_SOLN_WRITABLE_PLACEHOLDER);
      archive = GetArchive(argc, argv, &pos, as, false, op == 'R', true,
	false, soln_type, NULL, /* out_file_name, */ a);
      roptions = KheOptionsMake(a);
      GetSolveOptions(roptions, argc, argv, &pos);
      no_print = KheOptionsGetBool(roptions, "no_print", false);
      while( pos < argc )
      {
	if( strcmp(argv[pos], "-s") != 0 )
	  HnAbort("-s expected in %s -r command where %s appears",
            argv[0], argv[pos]);
	pos++;
	options = KheOptionsCopy(roptions, a);
	GetSolveOptions(options, argc, argv, &pos);
	/* soln_group = KheTestMakeSolnGroup(archive, options); */
	KheArchiveParallelSolve(archive, &KheGeneralSolve2024,
	  options, KHE_SOLN_WRITABLE_PLACEHOLDER, /* out_file_name, */ as);
      }
      if( !no_print /* && out_file_name == NULL */ )
      {
	/* ***
	if( DEBUG4 )
	  KheTimerResetStartTime(timer);
	*** */
	KheArchiveWrite(archive, false, stdout /*WriteFile(out_file_name)*/ );
	/* ***
	if( DEBUG4 )
	  fprintf(stderr, "  archive write time: %.1f secs\n",
	    KheTimerElapsedTime(timer));
	*** */
      }
      KheArchiveDelete(archive);
      break;

    case 's':
    case 'S':

      /* khe -s <archive> <options> */
      pos++;
      soln_type =(op2=='o' ? KHE_SOLN_ORDINARY : KHE_SOLN_WRITABLE_PLACEHOLDER);
      archive = GetArchive(argc, argv, &pos, as, false, op == 'R', true,
	false, soln_type, NULL, /* out_file_name, */ a);
      options = KheOptionsMake(a);
      GetSolveOptions(options, argc, argv, &pos);
      if( pos != argc )
	HnAbort("khe -w:  unexpected options on command line after <archive>");
      no_print = KheOptionsGetBool(options, "no_print", false);
      /* soln_group = KheTestMakeSolnGroup(archive, options); */
      KheArchiveParallelSolve(archive, &KheGeneralSolve2024, options,
	KHE_SOLN_WRITABLE_PLACEHOLDER, /* out_file_name, */ as);
      if( !no_print /* && out_file_name == NULL */ )
      {
	/* ***
	if( DEBUG4 )
	  KheTimerResetStartTime(timer);
	*** */
	KheArchiveWrite(archive, false, stdout /*WriteFile(out_file_name)*/);
	/* ***
	if( DEBUG4 )
	  fprintf(stderr, "  archive write time: %.1f secs\n",
	    KheTimerElapsedTime(timer));
	*** */
      }
      KheArchiveDelete(archive);
      break;

    case 'u':

      KheUsageMessageAndExit();
      break;

    case 'v':

      fprintf(stderr, "KHE %s\n", KheVersionBanner());
      break;

    case 'w':

      /* khe -w <archive> */
      pos++;
      archive = KheArchiveReadFromCommandLine(argc, argv, &pos, as, false,
	false, false, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL);
      if( pos != argc )
	HnAbort("khe -w:  unexpected options on command line after <archive>");
      KheArchiveWrite(archive, false, stdout /*WriteFile(out_file_name)*/);
      KheArchiveDelete(archive);
      break;

    default:

      fprintf(stderr, "khe: unknown first option %s\n", argv[1]);
      KheUsageMessageAndExit();
  }
  HaArenaDelete(a);
  HaArenaSetDelete(as);  /* added by JeffK 11/25, suggested by Ortiz */
}


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
/*  FILE:         command.c                                                  */
/*  MODULE:       Reading CGI or command line data                           */
/*                                                                           */
/*****************************************************************************/
#include "howard_a.h"
#include "howard_n.h"
#include "command.h"
#include "url.h"
#include <sys/stat.h>
#include <stdarg.h>

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  COMMAND - one command, whether from CGI Post, CGI Get, or command line   */
/*                                                                           */
/*****************************************************************************/
#define MAX_LINE 200

typedef enum {
  COMMAND_CGI_GET,
  COMMAND_CGI_POST,
  COMMAND_LINE,
} COMMAND_TYPE;

struct command_rec
{
  HA_ARENA		arena;
  COMMAND_TYPE		type;
  COMMAND_PAGE_BEGIN_FN	page_begin_fn;
  COMMAND_PAGE_END_FN	page_end_fn;
  union {
    struct {
      char		null_query_string[1];
      char		*query_string;
    } get;
    struct {
      char		mime_sep[MAX_LINE];
      FILE		*rerun;
    } post;
    struct {
      char		**argv;
      int		argc;
      int		pos;
    } line;
  } u;
};


/*****************************************************************************/
/*                                                                           */
/*  void CommandError(COMMAND c, char *fmt, ...)                             */
/*                                                                           */
/*  Report an error and exit.                                                */
/*                                                                           */
/*****************************************************************************/

void CommandError(COMMAND c, char *fmt, ...)
{
  HTML html;  char buff[500];  va_list args;

  /* first get the text into buff */
  va_start(args, fmt);
  vsnprintf(buff, 500, fmt, args);
  va_end(args);

  switch( c->type )
  {
    case COMMAND_CGI_GET:
    case COMMAND_CGI_POST:

      if( c->page_begin_fn != NULL )
        html = c->page_begin_fn(buff);
      else
      {
	html = HTMLPageBegin("./hseval.cgi", "HSEval home page", stdout);
	HTMLHeadBegin(html);
	HTMLTitleBegin(html);
	HTMLLiteralText(html, buff);
	HTMLTitleEnd(html);
	HTMLHeadEnd(html);
	HTMLBodyBegin(html);
      }

      HTMLBigHeading(html, "HSEval: format problem in uploaded file");

      HTMLParagraphBegin(html);
      HTMLText(html, "The uploaded file has at least one error:");
      HTMLParagraphEnd(html);

      HTMLParagraphBegin(html);
      HTMLColouredBoxBegin(html, LightRed);
      HTMLLiteralText(html, buff);
      HTMLColouredBoxEnd(html);
      HTMLParagraphEnd(html);

      HTMLParagraphBegin(html);
      HTMLText(html, "Return to the ");
      HTMLJumpFront(html);
      HTMLText(html, ".");
      HTMLParagraphEnd(html);

      if( c->page_end_fn != NULL )
        c->page_end_fn(html);
      else
      {
	HTMLBodyEnd(html);
	HTMLPageEnd(html);
      }
      break;

    case COMMAND_LINE:

      fprintf(stderr, "The uploaded file uploaded has at least one error:\n");
      fprintf(stderr, "  %s\n", buff);
  }
  CommandEnd(c);
  exit(0);
}


/*****************************************************************************/
/*                                                                           */
/*  void CommandInitRerun(COMMAND c, bool rerun)                             */
/*                                                                           */
/*  Initialize the rerun field of c.                                         */
/*                                                                           */
/*****************************************************************************/

static void CommandInitRerun(COMMAND c, bool rerun)
{
  if( rerun )
  {
    c->u.post.rerun = fopen("./rerun", "w");
    if( c->u.post.rerun == NULL )
      CommandError(c, "cannot open debug file \"./rerun\"");
    fprintf(c->u.post.rerun, "rm -f core*\n");
    fprintf(c->u.post.rerun, "MALLOC_CHECK_=2 exec ./hseval.cgi <<EOF\n");
    chmod("./rerun", S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    fflush(c->u.post.rerun);
  }
  else
    c->u.post.rerun = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  COMMAND CommandBeginCGI(bool rerun, COMMAND_PAGE_BEGIN_FN page_begin_fn, */
/*    COMMAND_PAGE_END_FN page_end_fn, HA_ARENA a)                           */
/*                                                                           */
/*  Make a command object with an optional rerun file (which will eventually */
/*  hold a copy of the input), which is primed to read CGI.                  */
/*                                                                           */
/*****************************************************************************/

COMMAND CommandBeginCGI(bool rerun, COMMAND_PAGE_BEGIN_FN page_begin_fn,
  COMMAND_PAGE_END_FN page_end_fn, HA_ARENA a)
{
  COMMAND res;  int ch;
  if( DEBUG1 )
    fprintf(stderr, "[ CommandBeginCGI(rerun %s)\n", rerun ? "true" : "false");
  HaMake(res, a);
  res->page_begin_fn = page_begin_fn;
  res->page_end_fn = page_end_fn;
  ch = getc(stdin);
  if( ch != EOF )
  {
    ungetc(ch, stdin);
    res->type = COMMAND_CGI_POST;
    res->u.post.mime_sep[0] = '\0';
    CommandInitRerun(res, rerun);
  }
  else
  {
    res->type = COMMAND_CGI_GET;
    res->u.get.null_query_string[0] = '\0';
    res->u.get.query_string = getenv("QUERY_STRING");
    if( res->u.get.query_string == NULL )
      res->u.get.query_string = res->u.get.null_query_string;
    if( DEBUG1 )
      fprintf(stderr, "  query_string: \"%s\"\n", res->u.get.query_string);
  }
  if( DEBUG1 )
    fprintf(stderr, "] CommandBeginCGI returning (type %d)\n", res->type);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  COMMAND CommandBeginCommandLine(char *argv[], int argc, int start_pos,   */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a command object which will read the command line from start_pos.   */
/*                                                                           */
/*****************************************************************************/

COMMAND CommandBeginCommandLine(char *argv[], int argc, int start_pos,
  HA_ARENA a)
{
  COMMAND res;
  if( DEBUG1 )
    fprintf(stderr, "[ CommandBeginCommandLine(argv, argc, a)\n");
  HaMake(res, a);
  res->type = COMMAND_LINE;
  res->page_begin_fn = NULL;
  res->page_end_fn = NULL;
  res->u.line.argv = argv;
  res->u.line.argc = argc;
  res->u.line.pos = start_pos - 1;  /* just *before* the first operation */
  if( DEBUG1 )
    fprintf(stderr, "] CommandBeginCommandLine\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void CgiEnd(CGI cgi)                                                     */
/*                                                                           */
/*  End the processing of CGI.                                               */
/*                                                                           */
/*****************************************************************************/

void CommandEnd(COMMAND c)
{
  int ch;
  if( c->type == COMMAND_CGI_POST && c->u.post.rerun != NULL )
  {
    while( (ch = getc(stdin)) != EOF )
      putc(ch, c->u.post.rerun);
    fprintf(c->u.post.rerun, "EOF\n");
    fclose(c->u.post.rerun);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  FILE *CommandRerunFile(COMMAND c)                                        */
/*                                                                           */
/*  Return the rerun file of c, or NULL if rerun not requested.              */
/*                                                                           */
/*****************************************************************************/

FILE *CommandRerunFile(COMMAND c)
{
  return c->type == COMMAND_CGI_POST ? c->u.post.rerun : NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool str_begins_with(char *str, char *pat)                               */
/*                                                                           */
/*  true if str begins with pat.                                             */
/*                                                                           */
/*****************************************************************************/

static bool str_begins_with(char *str, char *pat)
{
  return strstr(str, pat) == str;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CommandNextPair(COMMAND c, char **key, char **val, FILE **file_val) */
/*                                                                           */
/*  If there is another command pair, set (*key, *val) or (*key, *file_val)  */
/*  to it and return true.  Otherwise return false.                          */
/*                                                                           */
/*****************************************************************************/

bool CommandNextPair(COMMAND c, char **key, char **val, FILE **file_val)
{
  char line[MAX_LINE], *name, *name_end, *p, *q, last;
  if( DEBUG1 )
    fprintf(stderr, "[ CommandNextPair(type %d)\n", c->type);
  switch( c->type )
  {
    case COMMAND_CGI_GET:

      /* read the key */
      *key = URLDecodePartial(c->u.get.query_string, "=&",
	&c->u.get.query_string);
      if( *c->u.get.query_string == '&' )
	CommandError(c, "unexpected & in CGI string");
      if( *c->u.get.query_string != '\0' )
	c->u.get.query_string++;

      /* read the value from the query string */
      if( strcmp(*key, "file") == 0 )
	CommandError(c, "unexpected file in CGI string");
      *val = URLDecodePartial(c->u.get.query_string, "=&",
	&c->u.get.query_string);
      if( *c->u.get.query_string == '=' )
	CommandError(c, "unexpected = in CGI string");
      if( *c->u.get.query_string != '\0' )
	c->u.get.query_string++;
      *file_val = NULL;
      break;

    case COMMAND_CGI_POST:

      /* read one mime separator into sep, exiting if not present */
      p = fgets(line, MAX_LINE, stdin);  /* p unused, keep compiler happy */
      if( p == NULL )
	line[0] = '\0';
      if( DEBUG2 )
	fprintf(stderr, "  CommandNextPair at (a)\n");
      if( c->u.post.rerun != NULL )
	fputs(line, c->u.post.rerun);
      if( line[0] == '\n' || line[0] == '\0' )
      {
	if( DEBUG1 )
	  fprintf(stderr, "] CommandNextPair returning false");
	return false;
      }
      last = line[strlen(line) - 1];
      if( last != '\n' && last != '\r' )
	CommandError(c, "missing newline at end of first mime separator");

      /* either save the mime separator or make sure it hasn't changed */
      if( c->u.post.mime_sep[0] == '\0' )
	strcpy(c->u.post.mime_sep, line);
      else if( strcmp(line, c->u.post.mime_sep) != 0 )
	CommandError(c, "mime separator changed or not found where expected");
      if( DEBUG2 )
	fprintf(stderr, "  CommandNextPair at (b)\n");

      /* read header lines */
      *key = NULL;
      p = fgets(line, MAX_LINE, stdin);
      if( p == NULL )
	line[0] = '\0';
      if( c->u.post.rerun != NULL )
	fputs(line, c->u.post.rerun);
      while( line[0] != '\n' && line[0] != '\r' )
      {
	if( str_begins_with(line, "Content-Disposition:") &&
	    (name = strstr(line, "name=\"")) != NULL )
	{
	  if( *key != NULL )
	    CommandError(c, "redundant form-data Content-Disposition line");
	  name += 6;
	  if( (name_end = strstr(name, "\"")) == NULL || name_end == name + 1 )
	    CommandError(c, "error on form-data Content-Disposition line");
	  *key = malloc(sizeof(char) * (name_end - name + 1));
	  for( p = name, q = *key;  p != name_end;  p++, q++ )
	    *q = *p;
	  *q = '\0';
	}
	p = fgets(line, MAX_LINE, stdin);
	if( p == NULL )
	  line[0] = '\0';
	if( c->u.post.rerun != NULL )
	  fputs(line, c->u.post.rerun);
      }
      if( DEBUG2 )
	fprintf(stderr, "  CommandNextPair at (c)\n");

      /* read the next line from stdin and make that the value */
      if( strcmp(*key, "file") == 0 )
      {
	/* this one is a file, value is stdin */
	*val = NULL;
	*file_val = stdin;
      }
      else
      {
	/* this one is not a file, value is the next line */
	p = fgets(line, MAX_LINE, stdin);
	if( p == NULL )
	  line[0] = '\0';
	if( DEBUG2 )
	  fprintf(stderr, "  CommandNextPair at (e): \"%s\"\n", line);
	if( c->u.post.rerun != NULL )
	  fputs(line, c->u.post.rerun);
	if( DEBUG2 )
	  fprintf(stderr, "  CommandNextPair at (f)\n");
	for(p = &line[strlen(line)-1]; p >= line && (*p=='\n' || *p=='\r'); p--)
	  *p = '\0';
	if( DEBUG2 )
	  fprintf(stderr, "  CommandNextPair p - line + 2 = %ld: \"%s\"\n",
	    p - line + 2, line);
	*val = malloc(sizeof(char) * (p - line + 2)); /* bug fix JeffK 7/1/25 */
	strcpy(*val, line);
	*file_val = NULL;
      }
      if( DEBUG2 )
	fprintf(stderr, "  CommandNextPair at (z)\n");
      break;

    case COMMAND_LINE:

      c->u.line.pos++;
      if( c->u.line.pos >= c->u.line.argc )
      {
	/* off end */
	return false;
      }
      else
      {
	p = c->u.line.argv[c->u.line.pos];
	q = strstr(p, ":");
	if( q == NULL )
	{
	  /* file name */
	  *key = "file";
	  *val = NULL;
	  *file_val = fopen(p, "r");
	  if( *file_val == NULL )
	    CommandError(c, "cannot open file \"%s\"", p);
	}
	else
	{
	  /* <key>:<value> */
	  *q = '\0';
	  *key = p;
	  *val = q + 1;
	  *file_val = NULL;
	}
      }
      break;

    default:

      HnAbort("CommandNextPair internal error");
      break;

  }
  if( DEBUG1 )
    fprintf(stderr, "] CommandNextPair returning true(key %s, val %s, fp %s)\n",
      *key == NULL ? "(null)" : *key, *val == NULL ? "(null)" : *val,
      *file_val == NULL ? "null fp" : "fp");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *CommandNextOp(COMMAND c)                                           */
/*                                                                           */
/*  Assuming we're ready for it, return the next operator, or NULL if none.  */
/*  A non-NULL result may have any length and will lie in malloced memory.   */
/*                                                                           */
/*****************************************************************************/

/* ***
char *CommandNextOp(COMMAND c)
{
  char line[MAX_LINE], *name, *name_end, *p, *q, *res, last;
  switch( c->type )
  {
    case COMMAND_CGI_GET:

      ** read the key into op **
      res = URLDecodePartial(c->u.get.query_string, "=&",
	&c->u.get.query_string);
      if( *c->u.get.query_string == '&' )
	CommandError(c, "unexpected & in CGI string");
      if( *c->u.get.query_string != '\0' )
	c->u.get.query_string++;
      break;

    case COMMAND_CGI_POST:

      ** read one mime separator into sep, exiting if not present **
      p = fgets(line, MAX_LINE, stdin);  ** p unused, keep compiler happy **
      if( c->u.post.rerun != NULL )
	fputs(line, c->u.post.rerun);
      if( line[0] == '\n' || line[0] == '\0' )
      {
	if( DEBUG1 )
	  fprintf(stderr, "CgiNextOp returning NULL");
	return NULL;
      }
      last = line[strlen(line) - 1];
      if( last != '\n' && last != '\r' )
	CommandError(c, "missing newline at end of first mime separator");

      ** either save the mime separator or make sure it hasn't changed **
      if( c->u.post.mime_sep[0] == '\0' )
	strcpy(c->u.post.mime_sep, line);
      else if( strcmp(line, c->u.post.mime_sep) != 0 )
	CommandError(c, "mime separator changed or not found where expected");

      ** read header lines **
      res = NULL;
      p = fgets(line, MAX_LINE, stdin);  ** p unused, keep compiler happy **
      if( c->u.post.rerun != NULL )
	fputs(line, c->u.post.rerun);
      while( line[0] != '\n' && line[0] != '\r' )
      {
	if( str_begins_with(line, "Content-Disposition:") &&
	    (name = strstr(line, "name=\"")) != NULL )
	{
	  if( res != NULL )
	    CommandError(c, "redundant form-data Content-Disposition line");
	  name += 6;
	  if( (name_end = strstr(name, "\"")) == NULL || name_end == name + 1 )
	    CommandError(c, "error on form-data Content-Disposition line");
	  res = malloc(sizeof(char) * (name_end - name + 1));
	  for( p = name, q = res;  p != name_end;  p++, q++ )
	    *q = *p;
	  *q = '\0';
	}
	p = fgets(line, MAX_LINE, stdin);  ** p unused, keep compiler happy **
	if( c->u.post.rerun != NULL )
	  fputs(line, c->u.post.rerun);
      }
      break;

    case COMMAND_LINE:

      c->u.line.pos++;
      if( c->u.line.pos >= c->u.line.argc )
      {
	** off end **
	res = NULL;
      }
      else
      {
	p = c->u.line.argv[c->u.line.pos];
	if( p[0] != '-' )
	{
	  ** file name **
	  res = c->u.line.curr_op = "file";
	  c->u.line.curr_val = p;
	}
	else
	{
	  ** -c<key>:<value> **
	  if( p[1] != 'c' )
	    CommandError(c, "error on command line:  -c expected");
	  q = strstr(&p[2], ":");
	  if( q == NULL )
	    CommandError(c, "error on command line: no colon in -c option");
	  *q = '\0';
	  res = c->u.line.curr_op = &p[2];
	  c->u.line.curr_val = q + 1;
	}
      }
      break;

    default:

      HnAbort("CommandNextOp internal error");
      res = NULL;  ** keep compiler happy **
      break;

  }
  if( DEBUG1 )
    fprintf(stderr, "CgiNextOp returning %s\n", res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *CommandNextOneLineVal(COMMAND c)                                   */
/*                                                                           */
/*  Assuming that the file is positioned at the start of a value, and that   */
/*  the value is a simple one-line string, get that value.  Strip any        */
/*  trailing newline etc. characters.                                        */
/*                                                                           */
/*  A non-NULL result may have any length and will lie in malloced memory.   */
/*                                                                           */
/*****************************************************************************/

/* ***
char *CommandNextOneLineVal(COMMAND c)
{
  char line[MAX_LINE], *p, *res;
  switch( c->type )
  {
    case COMMAND_CGI_GET:

      ** read the value from the query string **
      res = URLDecodePartial(c->u.get.query_string, "=&",
	&c->u.get.query_string);
      if( *c->u.get.query_string == '=' )
	CommandError(c, "unexpected = in CGI string");
      if( *c->u.get.query_string != '\0' )
	c->u.get.query_string++;
      break;

    case COMMAND_CGI_POST:

      ** read the next line from stdin and make that the value **
      p = fgets(line, MAX_LINE, stdin);  ** p unused; keep compiler happy **
      if( c->u.post.rerun != NULL )
	fputs(line, c->u.post.rerun);
      for( p = &line[strlen(line)-1]; p >= line && (*p=='\n' || *p=='\r'); p-- )
	*p = '\0';
      res = malloc(sizeof(char) * (p - line + 1));
      strcpy(res, line);
      break;

    case COMMAND_LINE:

      res = c->u.line.curr_val;
      HnAssert(res != NULL, "CgiNextOneLineVal internal error");
      break;

    default:

      HnAbort("CommandNextOneLineVal internal error");
      res = NULL;  ** keep compiler happy **
      break;

  }
  if( DEBUG1 )
    fprintf(stderr, "CgiNextOneLineVal returning %s\n", res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  FILE *CommandFile(COMMAND c)                                             */
/*                                                                           */
/*  Return the file associated with c, or NULL if none.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
FILE *CommandFile(COMMAND c)
{
  FILE *res;
  switch( c->type )
  {
    case COMMAND_CGI_GET:

      ** no files here **
      res = NULL;
      break;

    case COMMAND_CGI_POST:

      ** stdin is the file hre **
      res = stdin;
      break;

    case COMMAND_LINE:

      ** file is named as the current op value **
      res = fopen(c->u.line.curr_val, "r");
      if( res == NULL )
	CommandError(c, "cannot open file \"%s\"", c->u.line.curr_val);
      break;

    default:

      HnAbort("CommandFile internal error");
      res = NULL;  ** keep compiler happy **
      break;

  }
  return res;
}
*** */

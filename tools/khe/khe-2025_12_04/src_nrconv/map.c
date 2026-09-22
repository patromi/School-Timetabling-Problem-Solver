
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
/*  FILE:         nrmap.c                                                    */
/*  MODULE:       NRMap - map files by replacing multiple lines              */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  MAP - a map from one set of lines to another                             */
/*                                                                           */
/*****************************************************************************/

typedef struct map_rec {
  char			*label;
  HA_ARRAY_NSTRING	lines;
} *MAP;


/*****************************************************************************/
/*                                                                           */
/*  MAP_SET - a set of maps with the same first line                         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(MAP) ARRAY_MAP;

typedef struct map_set_rec {
  ARRAY_MAP		maps;
  int			max_lines;
  HA_ARRAY_BOOL		alive;
  int			alive_count;
} *MAP_SET;

typedef HN_TABLE(MAP_SET) TABLE_MAP_SET;


/*****************************************************************************/
/*                                                                           */
/*  char *LineGet(FILE *fp, HA_ARENA a)                                      */
/*                                                                           */
/*  Return the next line of fp, or NULL if no more lines.                    */
/*                                                                           */
/*  For sanity, remove any white space at the end of the line.               */
/*                                                                           */
/*****************************************************************************/
#define is_white(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\r')

static char *LineGet(FILE *fp, HA_ARENA a)
{
  int val;  HA_ARRAY_NCHAR res;
  val = fgetc(fp);
  if( val == EOF )
    return NULL;
  HnStringBegin(res, a);
  while( val != '\n' && val != EOF )
  {
    HaArrayAddLast(res, (char) val);
    val = fgetc(fp);
  }
  while( HaArrayCount(res) > 0 && is_white(HaArrayLast(res)) )
    HaArrayDeleteLast(res);
  return HnStringEnd(res);
}


/*****************************************************************************/
/*                                                                           */
/*  void LineFree(char *line)                                                */
/*                                                                           */
/*  Free a line returned by LineGet.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void LineFree(char *line)
{
  free(line);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  MAP MapMake(char *label, HA_ARENA a)                                     */
/*                                                                           */
/*  Make a new map with this label.                                          */
/*                                                                           */
/*****************************************************************************/

static MAP MapMake(char *label, HA_ARENA a)
{
  MAP res;
  HaMake(res, a);
  HaArrayInit(res->lines, a);
  res->label = label;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void MapAddLine(MAP map, char *line)                                     */
/*                                                                           */
/*  Add line to map.                                                         */
/*                                                                           */
/*****************************************************************************/

static void MapAddLine(MAP map, char *line)
{
  HaArrayAddLast(map->lines, line);
}


/*****************************************************************************/
/*                                                                           */
/*  bool MapGet(FILE *fp, MAP *map, HA_ARENA a)                              */
/*                                                                           */
/*  If fp contains a map, read it and set *map to it.                        */
/*                                                                           */
/*****************************************************************************/

static bool MapGet(FILE *fp, MAP *map, HA_ARENA a)
{
  char *line;  char label[50];

  /* get first line, with format "begin <label>" */
  line = LineGet(fp, a);
  if( line == NULL )
  {
    *map = NULL;
    return false;
  }
  if( sscanf(line, "begin %s", label) != 1 )
    HnAbort("MapGet:  format error on line \"%s\"\n", line);
  /* LineFree(line); */

  /* make the map and add source lines to it */
  *map = MapMake(HnStringCopy(label, a), a);
  line = LineGet(fp, a);
  while( line != NULL )
  {
    if( strstr(line, "end ") == line )
    {
      /* LineFree(line); */
      break;
    }
    MapAddLine(*map, line);
    line = LineGet(fp, a);
  }
  HnAssert(line != NULL, "MapGet: unexpected end of map %s\n", label);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void MapDebug(MAP map, int indent, FILE *fp)                             */
/*                                                                           */
/*  Debug print of map onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void MapDebug(MAP map, int indent, FILE *fp)
{
  char *line;  int i;
  fprintf(fp, "%*s[ Map %s\n", indent, "", map->label);
  HaArrayForEach(map->lines, line, i)
    fprintf(fp, "%*s%s\n", indent + 2, "", line);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  MAP_SET MapSetMake(HA_ARENA a)                                           */
/*                                                                           */
/*  Make a new, empty map set.                                               */
/*                                                                           */
/*****************************************************************************/

static MAP_SET MapSetMake(HA_ARENA a)
{
  MAP_SET res;
  HaMake(res, a);
  HaArrayInit(res->maps, a);
  res->max_lines = 0;
  HaArrayInit(res->alive, a);
  res->alive_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void MapSetAddMap(MAP_SET map_set, MAP map)                              */
/*                                                                           */
/*  Add map to map_set.                                                      */
/*                                                                           */
/*****************************************************************************/

static void MapSetAddMap(MAP_SET map_set, MAP map)
{
  HaArrayAddLast(map_set->maps, map);
  if( HaArrayCount(map->lines) > map_set->max_lines )
    map_set->max_lines = HaArrayCount(map->lines);
  HaArrayAddLast(map_set->alive, false);
}


/*****************************************************************************/
/*                                                                           */
/*  void MapSetMatchBegin(MAP_SET map_set)                                   */
/*                                                                           */
/*  Begin matching map_set.                                                  */
/*                                                                           */
/*****************************************************************************/

static void MapSetMatchBegin(MAP_SET map_set)
{
  int i;
  for( i = 0;  i < HaArrayCount(map_set->alive);  i++ )
    HaArrayPut(map_set->alive, i, true);
  map_set->alive_count = HaArrayCount(map_set->alive);
}


/*****************************************************************************/
/*                                                                           */
/*  bool MapSetMatchIsAlive(MAP_SET map_set)                                 */
/*                                                                           */
/*  Return true if map_set is still alive.                                   */
/*                                                                           */
/*****************************************************************************/

static bool MapSetMatchIsAlive(MAP_SET map_set)
{
  return map_set->alive_count > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MapSetMatchTry(MAP_SET map_set, int pos, char *line, MAP *map)      */
/*                                                                           */
/*  Match line against map_set at position pos.  Return true if any map      */
/*  matches, setting *map to one such map if so.                             */
/*                                                                           */
/*****************************************************************************/

static bool MapSetMatchTry(MAP_SET map_set, int pos, char *line, MAP *map)
{
  bool alive;  int i;  MAP m;
  HaArrayForEach(map_set->alive, alive, i)
    if( alive )
    {
      m = HaArray(map_set->maps, i);
      if( pos < HaArrayCount(m->lines) &&
	  strcmp(line, HaArray(m->lines, pos)) == 0 )
      {
	if( pos == HaArrayCount(m->lines) - 1 )
	{
	  *map = m;
	  return true;
	}
      }
      else
      {
	HaArrayPut(map_set->alive, i, false);
	map_set->alive_count--;
      }
    }
  *map = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void MapSetDebug(MAP_SET map_set, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of map_set with the given indent.                            */
/*                                                                           */
/*****************************************************************************/

static void MapSetDebug(MAP_SET map_set, int indent, FILE *fp)
{
  MAP map;  int i;
  fprintf(fp, "%*s[ MapSet (%d maps, %d max_lines, %d alive)\n", indent, "",
    HaArrayCount(map_set->maps), map_set->max_lines, map_set->alive_count);
  HaArrayForEach(map_set->maps, map, i)
    MapDebug(map, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void GetLines(HA_ARRAY_NSTRING *lines, int n, FILE *fp, HA_ARENA a)      */
/*                                                                           */
/*  Ensure that *lines contains at least n lines, by adding lines from fp.   */
/*  If fp ends there may be fewer lines.                                     */
/*                                                                           */
/*****************************************************************************/

static void GetLines(HA_ARRAY_NSTRING *lines, int n, FILE *fp, HA_ARENA a)
{
  char *line;
  while( HaArrayCount(*lines) < n )
  {
    line = LineGet(fp, a);
    if( line == NULL )
      return;
    HaArrayAddLast(*lines, line);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void Usage(FILE *fp)                                                     */
/*                                                                           */
/*  Print a usage message.                                                   */
/*                                                                           */
/*****************************************************************************/
#define p(str) fprintf(fp, "%*s%s\n", indent, "", str)

void HelpMap(int indent, FILE *fp)
{
  p("");
  p("nrconv -m: the text file mapping part of nrconv");
  p("");
  p("  nrconv -m<map_file_name>");
  p("");
  p("    Read standard input and write it to standard output, modified");
  p("    according to the instructions in file <map_file_name>.");
  p("");
  p("    The map file contains zero or more entries of the form");
  p("");
  p("      begin <label>");
  p("      line");
  p("      ...");
  p("      line");
  p("      end <label>");
  p("");
  p("    where <label> is a string not containing white space characters,");
  p("    and line ... line stands for one or more lines of text, each");
  p("    not beginning with \"end \".");
  p("");
  p("    Whenever this sequence of lines is found in the input, it is");
  p("    replaced by a single line containing <label> only.  As a sanity");
  p("    measure, white space at the ends of all lines is ignored.");
  p("");
  p("    Where multiple matches are possible, ambiguity is resolved");
  p("    by these three rules, which are applied in order:");
  p("");
  p("      * A match starting on an earlier line takes priority over");
  p("        a match starting on a later line;");
  p("");
  p("      * When two matches start on the same line, a shorter");
  p("        match takes priority over a longer match;");
  p("");
  p("      * When two matches start on the same line and have the");
  p("        same length, the match defined earlier in the map");
  p("        file takes priority.");
  p("");
  p("    This mapping functionality is used to replace sections of input");
  p("    files that the main part of NRConv cannot interpret, by labels");
  p("    that NRConv takes as instructions to execute special case code.");
}


/*****************************************************************************/
/*                                                                           */
/*  void Map(char *map_file_name)                                            */
/*                                                                           */
/*  Map stdin to stdout according to instructions in map_file_name.          */
/*                                                                           */
/*****************************************************************************/

void Map(char *map_file_name, HA_ARENA_SET as)
{
  FILE *map_fp;  ARRAY_MAP maps;  MAP map;  char *first_val, *str;
  TABLE_MAP_SET table;  char *line;  int pos, i, line_num;
  HA_ARRAY_NSTRING lines;  bool match;  MAP_SET map_set;  HA_ARENA a;

  /* read the maps into the map table */
  a = HaArenaMake(as);
  map_fp = fopen(map_file_name, "r");
  if( map_fp == NULL )
    HnAbort("nrconv: cannot open map file \"%s\"", map_file_name);
  HaArrayInit(maps, a);
  HnTableInit(table, a);
  while( MapGet(map_fp, &map, a) )
  {
    HaArrayAddLast(maps, map);
    if( HaArrayCount(map->lines) > 0 )
    {
      first_val = HaArrayFirst(map->lines);
      if( !HnTableRetrieve(table, first_val, map_set, pos) )
      {
	map_set = MapSetMake(a);
	HnTableAdd(table, first_val, map_set);
      }
      MapSetAddMap(map_set, map);
      if( DEBUG1 )
	MapSetDebug(map_set, 0, stderr);
    }
  }
  /* ***
  HaArrayForEach(maps, map, i)
    MapDebug(map, stdout);
  *** */

  /* process the lines */
  HaArrayInit(lines, a);
  GetLines(&lines, 1, stdin, a);
  line_num = 1;
  map = NULL;
  while( HaArrayCount(lines) > 0 )
  {
    line = HaArrayFirst(lines);
    match = false;
    if( DEBUG1 )
    {
      fprintf(stderr, "line %d: %s\n", line_num, line);
      if( line_num == 102 )
      {
	fprintf(stderr, "table:\n");
	HnTableForEach(table, str, map_set, pos)
	{
	  fprintf(stderr, "  %s (%d):\n", str, pos);
	  MapSetDebug(map_set, 2, stderr);
	}
      }
    }
    if( HnTableRetrieve(table, line, map_set, pos) )
    {
      /* get enough lines for map_set, and check for a match */
      GetLines(&lines, map_set->max_lines, stdin, a);
      if( DEBUG1 )
      {
	fprintf(stderr, "  line %d matches first line of:\n", line_num);
	MapSetDebug(map_set, 2, stderr);
      }
      MapSetMatchBegin(map_set);
      for( i = 0;  MapSetMatchIsAlive(map_set);  i++ )
      {
	if( MapSetMatchTry(map_set, i, HaArray(lines, i), &map) )
	{
	  match = true;
	  break;
	}
      }
      if( DEBUG1 )
	fprintf(stderr, "  match ending with i = %d, match = %s\n",
	  i, match ? "true" : "false");
    }
    if( match )
    {
      /* lines match, so remove from lines and print label */
      if( DEBUG2 )
	fprintf(stderr, "  match printing label \"%s\"\n", map->label);
      fprintf(stdout, "%s\n", map->label);
      if( DEBUG1 )
      {
	fprintf(stderr, "  line %d matches:\n", line_num);
	MapDebug(map, 2, stderr);
      }
      for( i = 0;  i < HaArrayCount(map->lines);  i++ )
      {
	line = HaArrayFirst(lines);
	HaArrayDeleteAndShift(lines, 0);
	/* LineFree(line); */
      }
      line_num += HaArrayCount(map->lines);
    }
    else
    {
      /* no match, so print current line and move to next */
      line = HaArrayFirst(lines);
      HaArrayDeleteAndShift(lines, 0);
      if( DEBUG2 )
	fprintf(stderr, "  non-match printing line \"%s\"\n", line);
      fprintf(stdout, "%s\n", line);
      /* LineFree(line); */
      line_num += 1;
    }
    GetLines(&lines, 1, stdin, a);
  }
  HaArenaDelete(a);
}

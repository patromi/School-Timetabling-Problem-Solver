
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
/*  FILE:         khe_sm_stats.c                                             */
/*  DESCRIPTION:  Tables of statistics                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE_ENTRY - one entry of a table                                   */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_TABLE_ENTRY_NONE,
  KHE_TABLE_ENTRY_STRING,
  KHE_TABLE_ENTRY_COST,
  KHE_TABLE_ENTRY_TIME,
  KHE_TABLE_ENTRY_INT
} KHE_TABLE_ENTRY_TYPE;

typedef struct khe_table_entry_rec {
  KHE_TABLE_ENTRY_TYPE	entry_type;		/* type of value assigned    */
  bool			rule_after;		/* used in first row only    */
  bool			highlight;		/* highlight this entry      */
  union {
    char		*string_val;		/* if KHE_TABLE_ENTRY_STRING */
    KHE_COST		cost_val;		/* if KHE_TABLE_ENTRY_COST   */
    float		time_val;		/* if KHE_TABLE_ENTRY_TIME   */
    int			int_val;		/* if KHE_TABLE_ENTRY_INT    */
  } v;
} *KHE_TABLE_ENTRY;

typedef HA_ARRAY(KHE_TABLE_ENTRY) ARRAY_KHE_TABLE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ROW - one row of a table                                             */
/*                                                                           */
/*  The label for this row is the value of its first entry.                  */
/*                                                                           */
/*****************************************************************************/

struct khe_row_rec {
  bool			rule_below;		/* true if rule below here   */
  ARRAY_KHE_TABLE_ENTRY	entries;		/* entries for this row      */
};

typedef HA_ARRAY(KHE_ROW) ARRAY_KHE_ROW;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE - one table                                                    */
/*                                                                           */
/*****************************************************************************/

struct khe_table_rec {
  KHE_FILE		file;			/* enclosing file            */
  int			col_width;		/* column width (plain text) */
  bool			with_average_row;	/* with average row          */
  bool			with_total_row;		/* with total row            */
  bool			highlight_cost_minima;	/* highlight cost minima     */
  bool			highlight_time_minima;	/* highlight time minima     */
  bool			highlight_int_minima;	/* highlight int minima      */
  ARRAY_KHE_ROW		rows;			/* the rows                  */
  HA_ARRAY_NSTRING	caption_lines;		/* the lines of the caption  */
};

typedef HA_ARRAY(KHE_TABLE) ARRAY_KHE_TABLE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_POINT - one point in a graph dataset                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct {
  float			x;
  float			y;
} KHE_POINT;

typedef HA_ARRAY(KHE_POINT) ARRAY_KHE_POINT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DATASET - one dataset in a graph                                     */
/*                                                                           */
/*****************************************************************************/

struct khe_dataset_rec {
  KHE_DATASET_POINTS_TYPE	points_type;	/* to print at each point    */
  KHE_DATASET_PAIRS_TYPE	pairs_type;	/* to print between each pair*/
  char				*label;		/* label for dataset         */
  ARRAY_KHE_POINT		points;		/* points of the dataset     */
};

typedef HA_ARRAY(KHE_DATASET) ARRAY_KHE_DATASET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_GRAPH - one graph                                                    */
/*                                                                           */
/*****************************************************************************/

struct khe_graph_rec {
  KHE_FILE			file;		/* enclosing file            */
  float				width;		/* width of graph            */
  float				height;		/* height of graph           */
  float				xmax;		/* xmax of graph             */
  float				ymax;		/* ymax of graph             */
  char				*above_caption;	/* above caption of graph    */
  char				*below_caption;	/* below caption of graph    */
  char				*left_caption;	/* left caption of graph     */
  char				*left_gap;	/* left_gap option of graph  */
  char				*right_caption;	/* right caption of graph    */
  char				*right_gap;	/* right_gap option of graph */
  char				*key_label;	/* at start of graph key     */
  ARRAY_KHE_DATASET		datasets;	/* the datasets              */
  HA_ARRAY_NSTRING		caption_lines;	/* the lines of the caption  */
};

typedef HA_ARRAY(KHE_GRAPH) ARRAY_KHE_GRAPH;


/*****************************************************************************/
/*                                                                           */
/*  KHE_FILE - one file                                                      */
/*                                                                           */
/*****************************************************************************/

struct khe_file_rec {
  HA_ARENA		arena;			/* arena                     */
  char			*file_name;		/* file name                 */
  FILE			*fp;			/* file (when open)          */
  KHE_FILE_FORMAT	format;			/* file format               */
  int			thing_count;		/* no. of things so far      */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "timing"                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
#if KHE_USE_TIM ING
#include <time.h>
#include <sys/time.h>
#endif
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheDateToday(void)                                                 */
/*                                                                           */
/*  Return today's date as a string in static memory.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_timer.c
static char *KheMonthName(int tm_mon)
{
  char *months[] = { "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December" };
  HnAssert(tm_mon >= 0 && tm_mon <= 11, "KheMonthName internal error");
  return months[tm_mon];
}

char *KheDateToday(void)
{
#if KHE_USE_TI MING
  static char buff[100];
  time_t t;
  struct tm *time_now;
  time(&t);
  time_now = localtime(&t);
  snprintf(buff, 100, "%d %s %d", time_now->tm_mday,
    KheMonthName(time_now->tm_mon), 1900 + time_now->tm_year);
  return buff;
#else
  return "(no date)";
#endif
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table entries" (private)                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE_ENTRY KheEntryMake(bool rule_after, HA_ARENA a)                */
/*                                                                           */
/*  Make a new stats table entry, initially with no value.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_TABLE_ENTRY KheEntryMake(bool rule_after, HA_ARENA a)
{
  KHE_TABLE_ENTRY res;
  HaMake(res, a);
  res->entry_type = KHE_TABLE_ENTRY_NONE;
  res->highlight = false;
  res->rule_after = rule_after;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEntryAssignString(KHE_TABLE_ENTRY entry, char *string_val)       */
/*  void KheEntryAssignCost(KHE_TABLE_ENTRY entry, KHE_COST cost_val)        */
/*  void KheEntryAssignTime(KHE_TABLE_ENTRY entry, float time_val)           */
/*  void KheEntryAssignInt(KHE_TABLE_ENTRY entry, int int_val)               */
/*                                                                           */
/*  Assign the given value to entry, irrespective of any existing value.     */
/*                                                                           */
/*****************************************************************************/

static void KheEntryAssignString(KHE_TABLE_ENTRY entry, char *string_val,
  HA_ARENA a)
{
  entry->entry_type = KHE_TABLE_ENTRY_STRING;
  entry->v.string_val = HnStringCopy(string_val, a);
}

static void KheEntryAssignCost(KHE_TABLE_ENTRY entry, KHE_COST cost_val)
{
  entry->entry_type = KHE_TABLE_ENTRY_COST;
  entry->v.cost_val = cost_val;
}

static void KheEntryAssignTime(KHE_TABLE_ENTRY entry, float time_val)
{
  entry->entry_type = KHE_TABLE_ENTRY_TIME;
  entry->v.time_val = time_val;
}

static void KheEntryAssignInt(KHE_TABLE_ENTRY entry, int int_val)
{
  entry->entry_type = KHE_TABLE_ENTRY_INT;
  entry->v.int_val = int_val;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEntryWrite(KHE_TABLE_ENTRY entry, KHE_TABLE tbl,                 */
/*    int col_num, bool first_col, bool last_col, FILE *fp)                  */
/*                                                                           */
/*  Write one entry of tbl onto fp.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheEntryWrite(KHE_TABLE_ENTRY entry, KHE_TABLE tbl,
  int col_num, bool first_col, bool last_col, FILE *fp)
{
  char buff[100];

  /* get the string value of what is to be written */
  switch( entry->entry_type )
  {
    case KHE_TABLE_ENTRY_NONE:

      snprintf(buff, 100, "%s", "");
      break;

    case KHE_TABLE_ENTRY_STRING:

      snprintf(buff, 100, "%s", entry->v.string_val);
      break;

    case KHE_TABLE_ENTRY_COST:

      snprintf(buff, 100, "%.5f", KheCostShow(entry->v.cost_val));
      break;

    case KHE_TABLE_ENTRY_TIME:

      snprintf(buff, 100, "%.1f", entry->v.time_val);
      break;

    case KHE_TABLE_ENTRY_INT:

      snprintf(buff, 100, "%d", entry->v.int_val);
      break;

    default:

      HnAbort("KheEntryWrite internal error");
      snprintf(buff, 100, "??");  /* keep compiler happy */
      break;
  }

  /* write the string */
  switch( tbl->file->format )
  {
    case KHE_FILE_PLAIN:

      if( entry->highlight )
	fprintf(fp, first_col ? "*%-*s" : "%*s*", tbl->col_width - 1, buff);
      else
	fprintf(fp, first_col ? "%-*s" : "%*s", tbl->col_width, buff);
      break;

    case KHE_FILE_LOUT:
    case KHE_FILE_LOUT_STANDALONE:

      HnAssert(col_num < 12, "KheEntry: too many columns in Lout table");
      if( entry->highlight )
	fprintf(fp, "  %c { @B { %s } }\n", (char) ('A' + col_num), buff);
      else
	fprintf(fp, "  %c { %s }\n", (char) ('A' + col_num), buff);
      break;

    case KHE_FILE_LATEX:

      if( !first_col )
	fprintf(fp, " & ");
      if( entry->highlight )
	fprintf(fp, "{\\bf %s}", buff);
      else
	fprintf(fp, "%s", buff);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table rows" (private)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ROW KheRowMake(char *row_label, bool rule_below, HA_ARENA a)         */
/*                                                                           */
/*  Make a new row with these attributes.  The row label goes into the       */
/*  first entry of the row, which must exist.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_ROW KheRowMake(char *row_label, bool rule_below, HA_ARENA a)
{
  KHE_ROW res;  KHE_TABLE_ENTRY entry;

  /* make the object */
  HaMake(res, a);
  res->rule_below = rule_below;
  HaArrayInit(res->entries, a);

  /* make and add its first entry, holding row_label */
  entry = KheEntryMake(false, a);
  KheEntryAssignString(entry, row_label, a);
  HaArrayAddLast(res->entries, entry);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowFill(KHE_ROW row, int col_index)                              */
/*                                                                           */
/*  Fill row so that col_index has an entry.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheRowFill(KHE_ROW row, int col_index, HA_ARENA a)
{
  KHE_TABLE_ENTRY entry;
  while( HaArrayCount(row->entries) <= col_index )
  {
    entry = KheEntryMake(false, a);
    HaArrayAddLast(row->entries, entry);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheRowLabel(KHE_ROW row)                                           */
/*                                                                           */
/*  Return the label of row.                                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheRowLabel(KHE_ROW row)
{
  KHE_TABLE_ENTRY entry;
  HnAssert(HaArrayCount(row->entries) > 0, "KheRowLabel internal error 1");
  entry = HaArrayFirst(row->entries);
  HnAssert(entry->entry_type == KHE_TABLE_ENTRY_STRING,
    "KheRowLabel internal error 2");
  return entry->v.string_val;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowHighlightMinimaCost(KHE_ROW row)                              */
/*                                                                           */
/*  Highlight minimal entries of cost type in row, if any.                   */
/*                                                                           */
/*****************************************************************************/

static void KheRowHighlightMinimaCost(KHE_ROW row)
{
  KHE_TABLE_ENTRY entry;  int i;  bool min_found;  KHE_COST min_cost_val;

  /* find a minimum value, if any suitable values are present */
  min_found = false;
  min_cost_val = 0;  /* keep compiler happy; undefined really */
  HaArrayForEach(row->entries, entry, i)
    if( entry->entry_type == KHE_TABLE_ENTRY_COST &&
        (!min_found || entry->v.cost_val < min_cost_val) )
      min_found = true, min_cost_val = entry->v.cost_val;

  /* if a minimum value was found, highlight all occurrences of it */
  if( min_found )
    HaArrayForEach(row->entries, entry, i)
      if( entry->entry_type == KHE_TABLE_ENTRY_COST &&
	  entry->v.cost_val == min_cost_val )
	entry->highlight = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowHighlightMinimaTime(KHE_ROW row)                              */
/*                                                                           */
/*  Highlight minimal entries of time type in row, if any.                   */
/*                                                                           */
/*****************************************************************************/

static void KheRowHighlightMinimaTime(KHE_ROW row)
{
  KHE_TABLE_ENTRY entry;  int i;  bool min_found;  float min_time_val;

  /* find a minimum value, if any suitable values are present */
  min_found = false;
  min_time_val = 0.0;  /* keep compiler happy; undefined really */
  HaArrayForEach(row->entries, entry, i)
    if( entry->entry_type == KHE_TABLE_ENTRY_TIME &&
        (!min_found || entry->v.time_val < min_time_val) )
      min_found = true, min_time_val = entry->v.time_val;

  /* if a minimum value was found, highlight all occurrences of it */
  if( min_found )
    HaArrayForEach(row->entries, entry, i)
      if( entry->entry_type == KHE_TABLE_ENTRY_TIME &&
	  entry->v.time_val == min_time_val )
	entry->highlight = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowHighlightMinimaInt(KHE_ROW row)                               */
/*                                                                           */
/*  Highlight minimal entries of int type in row, if any.                    */
/*                                                                           */
/*****************************************************************************/

static void KheRowHighlightMinimaInt(KHE_ROW row)
{
  KHE_TABLE_ENTRY entry;  int i;  bool min_found;  int min_int_val;

  /* find a minimum value, if any suitable values are present */
  min_found = false;
  min_int_val = 0;  /* keep compiler happy; undefined really */
  HaArrayForEach(row->entries, entry, i)
    if( entry->entry_type == KHE_TABLE_ENTRY_INT &&
        (!min_found || entry->v.int_val < min_int_val) )
      min_found = true, min_int_val = entry->v.int_val;

  /* if a minimum value was found, highlight all occurrences of it */
  if( min_found )
    HaArrayForEach(row->entries, entry, i)
      if( entry->entry_type == KHE_TABLE_ENTRY_INT &&
	  entry->v.int_val == min_int_val )
	entry->highlight = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowWriteBegin(KHE_ROW row, bool first_row, bool last_row,        */
/*    KHE_TABLE tbl, FILE *fp)                                               */
/*                                                                           */
/*  Begin writing a row onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheRowWriteBegin(KHE_ROW row, bool first_row, bool last_row,
  KHE_TABLE tbl, FILE *fp)
{
  switch( tbl->file->format )
  {
    case KHE_FILE_PLAIN:

      /* nothing to do */
      break;

    case KHE_FILE_LOUT:
    case KHE_FILE_LOUT_STANDALONE:

      fprintf(fp, first_row ? "@Rowb\n" : "@Rowa\n");
      break;

    case KHE_FILE_LATEX:

      /* nothing to do */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowWriteEnd(KHE_ROW row, bool first_row, bool last_row,          */
/*    KHE_TABLE tbl, FILE *fp)                                               */
/*                                                                           */
/*  End writing a row of tbl onto fp.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheRowWriteEnd(KHE_ROW row, bool first_row, bool last_row,
  KHE_TABLE tbl, FILE *fp)
{
  int i, plain_total_width;  KHE_ROW first_r;
  switch( tbl->file->format )
  {
    case KHE_FILE_PLAIN:

      fprintf(fp, "\n");
      if( row->rule_below )
      {
	first_r = HaArrayFirst(tbl->rows);
	plain_total_width = tbl->col_width * HaArrayCount(first_r->entries);
	for( i = 0;  i < plain_total_width;  i++ )
	  fprintf(fp, "-");
	fprintf(fp, "\n");
      }
      break;

    case KHE_FILE_LOUT:
    case KHE_FILE_LOUT_STANDALONE:

      if( row->rule_below )
	fprintf(fp, "  rb { yes }\n");
      break;

    case KHE_FILE_LATEX:

      if( !last_row || row->rule_below )
	fprintf(fp, " \\\\");
      fprintf(fp, "\n");
      if( row->rule_below )
	fprintf(fp, "\\noalign{}\\hline\\noalign{}\n");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRowWrite(KHE_ROW row, bool first_row, bool last_row,             */
/*    KHE_TABLE tbl, FILE *fp)                                               */
/*                                                                           */
/*  Write a row of tbl onto fp.   If first_row is true, this is the first    */
/*  row.  If last_row is true, this is the last row.  Both could be true.    */
/*                                                                           */
/*****************************************************************************/

static void KheRowWrite(KHE_ROW row, bool first_row, bool last_row,
  KHE_TABLE tbl, FILE *fp)
{
  KHE_TABLE_ENTRY entry;  int i;
  KheRowWriteBegin(row, first_row, last_row, tbl, fp);
  HaArrayForEach(row->entries, entry, i)
    KheEntryWrite(entry, tbl, i, i == 0, i == HaArrayCount(row->entries)-1, fp);
  KheRowWriteEnd(row, first_row, last_row, tbl, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tables" (private)                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE KheTableMake(KHE_FILE file, int col_width, char *corner,       */
/*    bool with_average_row, bool with_total_row, bool highlight_cost_minima,*/
/*    bool highlight_time_minima, bool highlight_int_minima, HA_ARENA a)     */
/*                                                                           */
/*  Make a new stats_table object with these attributes and one row          */
/*  with one entry.                                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TABLE KheTableMake(KHE_FILE file, int col_width, char *corner,
  bool with_average_row, bool with_total_row, bool highlight_cost_minima,
  bool highlight_time_minima, bool highlight_int_minima, HA_ARENA a)
{
  KHE_TABLE res;  KHE_ROW row;

  /* check reasonable */
  if( file->format == KHE_FILE_PLAIN )
    HnAssert(col_width >= 1, "KheTableMake: col_width out of range (%d)",
      col_width);
  HnAssert(corner != NULL, "KheTableMake: corner == NULL");

  /* build the object */
  HaMake(res, a);
  res->file = file;
  res->col_width = col_width;
  res->with_average_row = with_average_row;
  res->with_total_row = with_total_row;
  res->highlight_cost_minima = highlight_cost_minima;
  res->highlight_time_minima = highlight_time_minima;
  res->highlight_int_minima = highlight_int_minima;
  HaArrayInit(res->rows, a);
  HaArrayInit(res->caption_lines, a);

  /* add the compulsory first row (and column), and return */
  row = KheRowMake(corner, true, a);
  HaArrayAddLast(res->rows, row);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTableContainsRow(KHE_TABLE tbl, char *row_label, KHE_ROW *row)   */
/*                                                                           */
/*  If tbl contains a row with this label, return true and set *row to       */
/*  the row, otherwise return false.  Ignore the first row.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTableContainsRow(KHE_TABLE tbl, char *row_label, KHE_ROW *row)
{
  int i;  KHE_ROW row2;
  for( i = 1;  i < HaArrayCount(tbl->rows);  i++ )
  {
    row2 = HaArray(tbl->rows, i);
    if( strcmp(row_label, KheRowLabel(row2)) == 0 )
    {
      *row = row2;
      return true;
    }
  }
  *row = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTableContainsCol(KHE_TABLE tbl, char *col_label, int *col_index) */
/*                                                                           */
/*  If tbl contains a col with this label, return true and set *col_index    */
/*  to its index, otherwise return false.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheTableContainsCol(KHE_TABLE tbl, char *col_label, int *col_index)
{
  int i;  KHE_ROW first_row;  KHE_TABLE_ENTRY entry;
  HnAssert(HaArrayCount(tbl->rows) > 0, "KheTableContainsCol internal error 1");
  first_row = HaArrayFirst(tbl->rows);
  for( i = 1;  i < HaArrayCount(first_row->entries);  i++ )
  {
    entry = HaArray(first_row->entries, i);
    HnAssert(entry->entry_type == KHE_TABLE_ENTRY_STRING,
      "KheTableContainsCol internal error 2");
    if( strcmp(col_label, entry->v.string_val) == 0 )
    {
      *col_index = i;
      return true;
    }
  }
  *col_index = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE_ENTRY_TYPE KheTableSoleNumericEntryType(KHE_TABLE tbl,         */
/*    int col_index)                                                         */
/*                                                                           */
/*  Return the unique entry type of column col_index of tbl, ignoring        */
/*  NONE and STRING, or NONE if none.                                        */
/*                                                                           */
/*****************************************************************************/
#define EntryTypeIsNumeric(et) (et >= KHE_TABLE_ENTRY_COST)

static KHE_TABLE_ENTRY_TYPE KheTableSoleNumericEntryType(KHE_TABLE tbl,
  int col_index)
{
  int i; KHE_ROW row;  KHE_TABLE_ENTRY entry;
  KHE_TABLE_ENTRY_TYPE res;
  res = KHE_TABLE_ENTRY_NONE;
  HaArrayForEach(tbl->rows, row, i)
  {
    KheRowFill(row, col_index, tbl->file->arena);
    entry = HaArray(row->entries, col_index);
    if( EntryTypeIsNumeric(entry->entry_type) )
    {
      if( res == KHE_TABLE_ENTRY_NONE )
	res = entry->entry_type;
      else if( res != entry->entry_type )
	return KHE_TABLE_ENTRY_NONE;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddAverageCost(KHE_TABLE tbl, int col_index,                */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the average cost of column col_index, if any.            */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddAverageCost(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  int hard_total, soft_total, count, i;
  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  hard_total = soft_total = count = 0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_COST )
    {
      hard_total += KheHardCost(entry2->v.cost_val);
      soft_total += KheSoftCost(entry2->v.cost_val);
      count += 1;
    }
  }
  if( count >= 1 )
    KheEntryAssignCost(entry, KheCost(hard_total/count, soft_total/count));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddAverageTime(KHE_TABLE tbl, int col_index,                */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the average time of column col_index of tbl, if any.     */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddAverageTime(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  int count, i;  float total_time;
  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  count = 0;  total_time = 0.0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_TIME )
    {
      total_time += entry2->v.time_val;
      count += 1;
    }
  }
  if( count >= 1 )
    KheEntryAssignTime(entry, total_time / count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddAverageInt(KHE_TABLE tbl, int col_index,                 */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the average int_val of column col_index of tbl, if any.  */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddAverageInt(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  int total, count, i;
  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  total = count = 0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_INT )
    {
      total += entry2->v.int_val;
      count += 1;
    }
  }
  if( count >= 1 )
    KheEntryAssignInt(entry, total / count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddAverageRow(KHE_TABLE tbl)                                */
/*                                                                           */
/*  Add an average row to tbl.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddAverageRow(KHE_TABLE tbl)
{
  KHE_ROW first_row, average_row;  KHE_TABLE_ENTRY entry;  int i;

  /* get the first row */
  HnAssert(HaArrayCount(tbl->rows) > 0,
    "KheTableAddAverageRow internal error 1");
  first_row = HaArrayFirst(tbl->rows);

  /* make and add an average row */
  average_row = KheRowMake("Average", false, tbl->file->arena);
  HaArrayAddLast(tbl->rows, average_row);

  /* for every column of first_row except the first, add an average entry */
  for( i = 1;  i < HaArrayCount(first_row->entries);  i++ )
  {
    entry = KheEntryMake(false, tbl->file->arena);
    HaArrayAddLast(average_row->entries, entry);
    switch( KheTableSoleNumericEntryType(tbl, i) )
    {
      case KHE_TABLE_ENTRY_NONE:
      case KHE_TABLE_ENTRY_STRING:

	/* no average, so change nothing here */
	break;

      case KHE_TABLE_ENTRY_COST:

        KheTableAddAverageCost(tbl, i, entry);
	break;

      case KHE_TABLE_ENTRY_TIME:

        KheTableAddAverageTime(tbl, i, entry);
	break;

      case KHE_TABLE_ENTRY_INT:

        KheTableAddAverageInt(tbl, i, entry);
	break;

      default:

	HnAbort("KheTableAddAverageRow internal error 2");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddTotalCost(KHE_TABLE tbl, int col_index,                  */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the total cost of column col_index, if any.              */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddTotalCost(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  KHE_COST cost_total;  int i;  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  cost_total = 0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_COST )
      cost_total += entry2->v.cost_val;
  }
  KheEntryAssignCost(entry, cost_total);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddTotalTime(KHE_TABLE tbl, int col_index,                  */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the total time of column col_index of tbl, if any.       */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddTotalTime(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  int i;  float total_time;  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  total_time = 0.0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_TIME )
      total_time += entry2->v.time_val;
  }
  KheEntryAssignTime(entry, total_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddTotalInt(KHE_TABLE tbl, int col_index,                   */
/*    KHE_TABLE_ENTRY entry)                                                 */
/*                                                                           */
/*  Assign to entry the total int_val of column col_index of tbl, if any.    */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddTotalInt(KHE_TABLE tbl, int col_index,
  KHE_TABLE_ENTRY entry)
{
  int total, i;  KHE_ROW row;  KHE_TABLE_ENTRY entry2;
  total = 0;
  HaArrayForEach(tbl->rows, row, i)
  {
    entry2 = HaArray(row->entries, col_index);
    if( entry2->entry_type == KHE_TABLE_ENTRY_INT )
      total += entry2->v.int_val;
  }
  KheEntryAssignInt(entry, total);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableAddTotalRow(KHE_TABLE tbl)                                  */
/*                                                                           */
/*  Add a total row to tbl.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheTableAddTotalRow(KHE_TABLE tbl)
{
  KHE_ROW first_row, total_row;  KHE_TABLE_ENTRY entry;  int i;

  /* get the first row */
  HnAssert(HaArrayCount(tbl->rows) > 0,
    "KheTableAddTotalRow internal error 1");
  first_row = HaArrayFirst(tbl->rows);

  /* make and add an average row */
  total_row = KheRowMake("Total", false, tbl->file->arena);
  HaArrayAddLast(tbl->rows, total_row);

  /* for every column of first_row except the first, add an average entry */
  for( i = 1;  i < HaArrayCount(first_row->entries);  i++ )
  {
    entry = KheEntryMake(false, tbl->file->arena);
    HaArrayAddLast(total_row->entries, entry);
    switch( KheTableSoleNumericEntryType(tbl, i) )
    {
      case KHE_TABLE_ENTRY_NONE:
      case KHE_TABLE_ENTRY_STRING:

	/* no average, so change nothing here */
	break;

      case KHE_TABLE_ENTRY_COST:

        KheTableAddTotalCost(tbl, i, entry);
	break;

      case KHE_TABLE_ENTRY_TIME:

        KheTableAddTotalTime(tbl, i, entry);
	break;

      case KHE_TABLE_ENTRY_INT:

        KheTableAddTotalInt(tbl, i, entry);
	break;

      default:

	HnAbort("KheTableAddTotalRow internal error 2");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableHighlightMinima(KHE_TABLE tbl)                              */
/*                                                                           */
/*  Highlight minima in the rows of tbl (except the first row).              */
/*                                                                           */
/*****************************************************************************/

static void KheTableHighlightMinima(KHE_TABLE tbl)
{
  int i;  KHE_ROW row;
  for( i = 1;  i < HaArrayCount(tbl->rows);  i++ )
  {
    row = HaArray(tbl->rows, i);
    if( tbl->highlight_cost_minima )
      KheRowHighlightMinimaCost(row);
    if( tbl->highlight_time_minima )
      KheRowHighlightMinimaTime(row);
    if( tbl->highlight_int_minima )
      KheRowHighlightMinimaInt(row);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableWriteBegin(KHE_TABLE tbl)                                   */
/*                                                                           */
/*  Begin writing tbl onto tbl->file.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheTableWriteBegin(KHE_TABLE tbl)
{
  int i;  char *str;  KHE_ROW row;  KHE_TABLE_ENTRY entry;  FILE *fp;
  KHE_FILE file;
  HnAssert(HaArrayCount(tbl->rows) > 0, "KheTableWriteBegin internal error");
  file = tbl->file;
  fp = file->fp;
  row = HaArrayFirst(tbl->rows);
  switch( tbl->file->format )
  {
    case KHE_FILE_PLAIN:

      /* nothing to do except optionally write a caption */
      if( HaArrayCount(tbl->caption_lines) > 0 )
      {
	HaArrayForEach(tbl->caption_lines, str, i)
	  fprintf(fp, "%s", str);
	fprintf(fp, "\n");
      }
      break;

    case KHE_FILE_LOUT:
    case KHE_FILE_LOUT_STANDALONE:

      HnAssert(HaArrayCount(row->entries) <= 12,
	"KheTableWriteBegin: too many cols for table in file \"%s\"",
	file->file_name);
      if( tbl->file->format == KHE_FILE_LOUT )
      {
	fprintf(fp, "@Table\n");
	fprintf(fp, "  @OnePage { Yes }\n");
	if( file->thing_count == 1 )
	  fprintf(fp, "  @Tag { %s }\n", file->file_name);
	else
	  fprintf(fp, "  @Tag { %s_%d }\n", file->file_name,
	    file->thing_count);
	if( HaArrayCount(tbl->caption_lines) > 0 )
	{
	  fprintf(fp, "  @Caption {\n");
	  HaArrayForEach(tbl->caption_lines, str, i)
	    fprintf(fp, "%s", str);
	  fprintf(fp, "  }\n");
	}
      }
      fprintf(fp, "-2p @Font -2px @Break @OneRow @Tbl\n");
      fprintf(fp, "  mv { 0.5vx }\n");
      fprintf(fp, "  aformat { ");
      HaArrayForEach(row->entries, entry, i)
	fprintf(fp, "%s@Cell %s%s%s %c", i == 0 ? "" : " | ",
	  i == 0 ? "ml { 0i }" : "i { right }",
	  entry->rule_after ? " rr { yes }" : "",
	  i == HaArrayCount(row->entries) - 1 ? " mr { 0i }" : "",
	  (char) ('A' + i));
      fprintf(fp, " }\n");
      fprintf(fp, "  bformat { ");
      HaArrayForEach(row->entries, entry, i)
	fprintf(fp, "%s@Cell %s%s%s %c", i == 0 ? "" : " | ",
	  i == 0 ? "ml { 0i }" : "i { ctr }",
	  entry->rule_after ? " rr { yes }" : "",
	  i == HaArrayCount(row->entries) - 1 ? " mr { 0i }" : "",
	  (char) ('A' + i));
      fprintf(fp, " }\n");
      fprintf(fp, "{\n");
      break;

    case KHE_FILE_LATEX:

      fprintf(fp, "\\begin{table}\n");
      fprintf(fp, "\\caption{");
      HaArrayForEach(tbl->caption_lines, str, i)
	fprintf(fp, "%s", str);
      fprintf(fp, "}\n");
      if( file->thing_count == 1 )
	fprintf(fp, "\\label{%s}\n", file->file_name);
      else
	fprintf(fp, "\\label{%s:%d}\n", file->file_name,
	  file->thing_count);
      fprintf(fp, "\\center\\begin{tabular}{");
      HaArrayForEach(row->entries, entry, i)
	fprintf(fp, "%c%s", i == 0 ? 'l' : 'r',
	  entry->rule_after ? "|" : "");
      fprintf(fp, "}\n");
      /* fprintf(file->fp, "\\hline\\noalign{}\n"); */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableWriteEnd(KHE_TABLE tbl)                                     */
/*                                                                           */
/*  End writing tbl onto tbl->file.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheTableWriteEnd(KHE_TABLE tbl)
{
  FILE *fp;
  fp = tbl->file->fp;
  switch( tbl->file->format )
  {
    case KHE_FILE_PLAIN:

      /* nothing to do */
      break;

    case KHE_FILE_LOUT:
    case KHE_FILE_LOUT_STANDALONE:

      fprintf(fp, "}\n");
      break;

    case KHE_FILE_LATEX:

      fprintf(fp, "\\end{tabular}\n");
      fprintf(fp, "\\end{table}\n");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableWrite(KHE_TABLE tbl)                                        */
/*                                                                           */
/*  Write tbl onto tbl->file.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheTableWrite(KHE_TABLE tbl)
{
  KHE_ROW row;  int i;
  KheTableWriteBegin(tbl);
  HaArrayForEach(tbl->rows, row, i)
    KheRowWrite(row, i==0, i == HaArrayCount(tbl->rows)-1, tbl, tbl->file->fp);
  KheTableWriteEnd(tbl);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions - files"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FILE KheFileBegin(char *file_name, KHE_FILE_FORMAT fmt)              */
/*                                                                           */
/*  Begin a new file with these attributes.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_FILE KheFileBegin(char *file_name, KHE_FILE_FORMAT fmt)
{
  KHE_FILE res;  char buff[200];  HA_ARENA a;  HA_ARENA_SET as;
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* we've decided not to remove this call */
  HaMake(res, a);
  res->arena = a;
  res->file_name = HnStringCopy(file_name, a);
  snprintf(buff, 200, "stats/%s", file_name);
  res->fp = fopen(buff, "w");
  HnAssert(res->fp != NULL, "KheFileBegin: cannot open file \"%s\"\n", buff);
  HnAssert(fmt >= KHE_FILE_PLAIN && fmt <= KHE_FILE_LATEX,
    "KheFileBegin: unknown format %d\n", fmt);
  if( fmt == KHE_FILE_LOUT_STANDALONE )
  {
    fprintf(res->fp, "@SysInclude { tbl }\n");
    fprintf(res->fp, "@SysInclude { graph }\n");
    fprintf(res->fp, "@SysInclude { picture }\n");
    fprintf(res->fp, "@Illustration {\n");
  }
  res->format = fmt;
  res->thing_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFileEnd(KHE_FILE kf)                                             */
/*                                                                           */
/*  End kf.                                                                  */
/*                                                                           */
/*****************************************************************************/

void KheFileEnd(KHE_FILE kf)
{
  if( kf->format == KHE_FILE_LOUT_STANDALONE )
    fprintf(kf->fp, "}\n");
  fclose(kf->fp);
  HaArenaDelete(kf->arena);  /* we've decided not to remove this call */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions - tables"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE KheTableBegin(KHE_FILE kf, int col_width, char *corner,        */
/*    bool with_average_row, bool with_total_row, bool highlight_cost_minima,*/
/*    bool highlight_time_minima, bool highlight_int_minima)                 */
/*                                                                           */
/*  Begin a table in kf with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TABLE KheTableBegin(KHE_FILE kf, int col_width, char *corner,
  bool with_average_row, bool with_total_row, bool highlight_cost_minima,
  bool highlight_time_minima, bool highlight_int_minima)
{
  KHE_TABLE res;
  res = KheTableMake(kf, col_width, corner,
    with_average_row, with_total_row, highlight_cost_minima,
    highlight_time_minima, highlight_int_minima, kf->arena);
  kf->thing_count++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableEnd(KHE_TABLE kt)                                           */
/*                                                                           */
/*  End a table.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheTableEnd(KHE_TABLE kt)
{
  KHE_ROW row;

  /* the last row (not counting any average row) has a rule below it */
  row = HaArrayLast(kt->rows);
  row->rule_below = true;

  /* add an average row if requested */
  if( kt->with_average_row )
    KheTableAddAverageRow(kt);

  /* add a total row if requested */
  if( kt->with_total_row )
    KheTableAddTotalRow(kt);

  /* highlight minima, as requested */
  KheTableHighlightMinima(kt);

  /* write the table */
  KheTableWrite(kt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableCaptionAdd(KHE_TABLE kt, char *fmt, ...)                    */
/*                                                                           */
/*  Add one caption line to kt.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTableCaptionAdd(KHE_TABLE kt, char *fmt, ...)
{
  va_list args;  HA_ARRAY_NCHAR ac;  char *str;

  /* get a malloced copy of the line */
  HnStringBegin(ac, kt->file->arena);
  va_start(args, fmt);
  HnStringVAdd(&ac, fmt, args);
  va_end(args);
  str = HnStringEnd(ac);

  /* add it to the table or graph */
  HaArrayAddLast(kt->caption_lines, str);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableRowAdd(KHE_TABLE kt, char *row_label, bool rule_below)      */
/*                                                                           */
/*  Make a new row of kt with these attributes.                              */
/*                                                                           */
/*****************************************************************************/

void KheTableRowAdd(KHE_TABLE kt, char *row_label, bool rule_below)
{
  KHE_ROW row;
  if( KheTableContainsRow(kt, row_label, &row) )
    HnAbort("KheTableRowAdd: row \"%s\" of table already exists", row_label);
  row = KheRowMake(row_label, rule_below, kt->file->arena);
  HaArrayAddLast(kt->rows, row);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableColAdd(KHE_TABLE kt, char *col_label, bool rule_after)      */
/*                                                                           */
/*  Make a new column of kt with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

void KheTableColAdd(KHE_TABLE kt, char *col_label, bool rule_after)
{
  int pos;
  KHE_TABLE_ENTRY entry;  KHE_ROW first_row;
  if( KheTableContainsCol(kt, col_label, &pos) )
    HnAbort("KheTableColAdd: col \"%s\" of table already exists", col_label);
  entry = KheEntryMake(rule_after, kt->file->arena);
  KheEntryAssignString(entry, col_label, kt->file->arena);
  first_row = HaArrayFirst(kt->rows);
  HaArrayAddLast(first_row->entries, entry);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TABLE_ENTRY KheFindEntry(KHE_TABLE kt, char *row_label,              */
/*    char *col_label)                                                       */
/*                                                                           */
/*  Return the entry with these labels, or abort if none.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_TABLE_ENTRY KheFindEntry(KHE_TABLE kt, char *row_label,
  char *col_label)
{
  int col_index;  KHE_ROW row;  KHE_TABLE_ENTRY entry;

  /* make sure there is an entry */
  if( !KheTableContainsRow(kt, row_label, &row) )
    HnAbort("KheFindEntry: no row \"%s\" in table", row_label);
  if( !KheTableContainsCol(kt, col_label, &col_index) )
    HnAbort("KheFindEntry: no col \"%s\" in table", col_label);

  /* make sure the row has an entry at col_index */
  KheRowFill(row, col_index, kt->file->arena);

  /* make sure the entry has not been assigned a value already, and return it */
  entry = HaArray(row->entries, col_index);
  HnAssert(entry->entry_type == KHE_TABLE_ENTRY_NONE,
    "KheFindEntry(kt, \"%s\", \"%s\") already has a value",
    row_label, col_label);
  return entry;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableEntryAddString(KHE_TABLE kt, char *row_label,               */
/*    char *col_label, char *str)                                            */
/*                                                                           */
/*  Set the value of the indicated entry to str.                             */
/*                                                                           */
/*****************************************************************************/

void KheTableEntryAddString(KHE_TABLE kt, char *row_label,
  char *col_label, char *str)
{
  KHE_TABLE_ENTRY entry;
  entry = KheFindEntry(kt, row_label, col_label);
  KheEntryAssignString(entry, str, kt->file->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableEntryAddCost(KHE_TABLE kt, char *row_label,                 */
/*    char *col_label, KHE_COST cost)                                        */
/*                                                                           */
/*  Set the value of the indicated entry to cost.                            */
/*                                                                           */
/*****************************************************************************/

void KheTableEntryAddCost(KHE_TABLE kt, char *row_label,
  char *col_label, KHE_COST cost)
{
  KHE_TABLE_ENTRY entry;
  entry = KheFindEntry(kt, row_label, col_label);
  KheEntryAssignCost(entry, cost);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableEntryAddTime(KHE_TABLE kt, char *row_label,                 */
/*    char *col_label, float time)                                           */
/*                                                                           */
/*  Set the value of the indicated entry to time.                            */
/*                                                                           */
/*****************************************************************************/

void KheTableEntryAddTime(KHE_TABLE kt, char *row_label,
  char *col_label, float time)
{
  KHE_TABLE_ENTRY entry;
  entry = KheFindEntry(kt, row_label, col_label);
  KheEntryAssignTime(entry, time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTableEntryAddInt(KHE_TABLE kt, char *row_label,                  */
/*    char *col_label, int val)                                              */
/*                                                                           */
/*  Set the value of the indicated entry to val.                             */
/*                                                                           */
/*****************************************************************************/

void KheTableEntryAddInt(KHE_TABLE kt, char *row_label,
  char *col_label, int val)
{
  KHE_TABLE_ENTRY entry;
  entry = KheFindEntry(kt, row_label, col_label);
  KheEntryAssignInt(entry, val);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "datasets"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DATASET KheDataSetMake(KHE_DATASET_POINTS_TYPE points_type,          */
/*    KHE_DATASET_PAIRS_TYPE pairs_type, char *label, HA_ARENA a)            */
/*                                                                           */
/*  Make a new dataset with these attributes.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_DATASET KheDataSetMake(KHE_DATASET_POINTS_TYPE points_type,
  KHE_DATASET_PAIRS_TYPE pairs_type, char *label, HA_ARENA a)
{
  KHE_DATASET res;
  HaMake(res, a);
  res->points_type = points_type;
  res->pairs_type = pairs_type;
  res->label = HnStringCopy(label, a);
  HaArrayInit(res->points, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePointsSym(KHE_DATASET_POINTS_TYPE points_type)                  */
/*                                                                           */
/*  Return the Lout stand-alone symbol for points_type.                      */
/*                                                                           */
/*****************************************************************************/

static char *KhePointsSym(KHE_DATASET_POINTS_TYPE points_type)
{
  switch( points_type )
  {
    case KHE_DATASET_POINTS_NONE:		return "";
    case KHE_DATASET_POINTS_CROSS:		return "@GraphCross";
    case KHE_DATASET_POINTS_SQUARE:		return "@GraphSquare";
    case KHE_DATASET_POINTS_DIAMOND:		return "@GraphDiamond";
    case KHE_DATASET_POINTS_CIRCLE:		return "@GraphCircle";
    case KHE_DATASET_POINTS_TRIANGLE:		return "@GraphTriangle";
    case KHE_DATASET_POINTS_PLUS:		return "@GraphPlus";
    case KHE_DATASET_POINTS_FILLED_SQUARE:	return "@GraphFilledSquare";
    case KHE_DATASET_POINTS_FILLED_DIAMOND:	return "@GraphFilledDiamond";
    case KHE_DATASET_POINTS_FILLED_CIRCLE:	return "@GraphFilledCircle";
    case KHE_DATASET_POINTS_FILLED_TRIANGLE:	return "@GraphFilledTriangle";

    default:

      HnAbort("KhePointsSym: unknown points_type (%d)", points_type);
      return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePairsSym(KHE_DATASET_PAIRS_TYPE pairs_type)                     */
/*                                                                           */
/*  Return the Lout stand-alone symbol for pairs_type.                       */
/*                                                                           */
/*****************************************************************************/

static char *KhePairsSym(KHE_DATASET_PAIRS_TYPE pairs_type)
{
  switch( pairs_type )
  {
    case KHE_DATASET_PAIRS_NONE:		return "@GraphNoLine";
    case KHE_DATASET_PAIRS_SOLID:		return "@GraphSolid";
    case KHE_DATASET_PAIRS_DASHED:		return "@GraphDashed";
    case KHE_DATASET_PAIRS_DOTTED:		return "@GraphDotted";
    case KHE_DATASET_PAIRS_DOT_DASHED:		return "@GraphDotDashed";
    case KHE_DATASET_PAIRS_DOT_DOT_DASHED:	return "@GraphDotDotDashed";
    case KHE_DATASET_PAIRS_DOT_DOT_DOT_DASHED:	return "@GraphDotDotDotDashed";

    case KHE_DATASET_PAIRS_YHISTO:
    case KHE_DATASET_PAIRS_SURFACE_YHISTO:
    case KHE_DATASET_PAIRS_FILLED_YHISTO:
    case KHE_DATASET_PAIRS_XHISTO:
    case KHE_DATASET_PAIRS_SURFACE_XHISTO:
    case KHE_DATASET_PAIRS_FILLED_XHISTO:

      HnAbort("KhePairsSym: no pairs symbol for histo type (%d)", pairs_type);
      return NULL;

    default:

      HnAbort("KhePairsSym: unknown pairs_type (%d)", pairs_type);
      return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDataSetWritePointsRegular(KHE_DATASET ds, KHE_FILE file)         */
/*                                                                           */
/*  Write the data points of ds in the regular way.                          */
/*                                                                           */
/*****************************************************************************/

static void KheDataSetWritePointsRegular(KHE_DATASET ds, KHE_FILE file)
{
  KHE_POINT point;  int i;
  fprintf(file->fp, "  {\n");
  HaArrayForEach(ds->points, point, i)
    fprintf(file->fp, "    %.4f %.4f\n", point.x, point.y);
  fprintf(file->fp, "  }\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDataSetWritePointsYHisto(KHE_DATASET ds, KHE_FILE file)          */
/*                                                                           */
/*  Write the data points of ds in a way suited to Y histograms.             */
/*                                                                           */
/*****************************************************************************/

static void KheDataSetWritePointsYHisto(KHE_DATASET ds, KHE_FILE file)
{
  KHE_POINT point;  int i;
  fprintf(file->fp, "  {\n");
  HaArrayForEach(ds->points, point, i)
    fprintf(file->fp, "    %.4f %.4f\n", point.x - 0.5, point.y);
  if( HaArrayCount(ds->points) > 0 )
  {
    point = HaArrayLast(ds->points);
    fprintf(file->fp, "    %.4f %.4f\n", point.x + 0.5, 0.0);
  }
  fprintf(file->fp, "  }\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDataSetWrite(KHE_DATASET ds, KHE_FILE file)                      */
/*                                                                           */
/*  Write ds to file.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheDataSetWrite(KHE_DATASET ds, KHE_FILE file)
{
  /* print @Data and points option */
  switch( ds->points_type )
  {
    case KHE_DATASET_POINTS_NONE:

      fprintf(file->fp, "  @Data");
      break;

    case KHE_DATASET_POINTS_CROSS:

      fprintf(file->fp, "  @Data points { cross }");
      break;

    case KHE_DATASET_POINTS_SQUARE:

      fprintf(file->fp, "  @Data points { square }");
      break;

    case KHE_DATASET_POINTS_DIAMOND:

      fprintf(file->fp, "  @Data points { diamond }");
      break;

    case KHE_DATASET_POINTS_CIRCLE:

      fprintf(file->fp, "  @Data points { circle }");
      break;

    case KHE_DATASET_POINTS_TRIANGLE:

      fprintf(file->fp, "  @Data points { triangle }");
      break;

    case KHE_DATASET_POINTS_PLUS:

      fprintf(file->fp, "  @Data points { plus }");
      break;

    case KHE_DATASET_POINTS_FILLED_SQUARE:

      fprintf(file->fp, "  @Data points { filledsquare }");
      break;

    case KHE_DATASET_POINTS_FILLED_DIAMOND:

      fprintf(file->fp, "  @Data points { filleddiamond }");
      break;

    case KHE_DATASET_POINTS_FILLED_CIRCLE:

      fprintf(file->fp, "  @Data points { filledcircle }");
      break;

    case KHE_DATASET_POINTS_FILLED_TRIANGLE:

      fprintf(file->fp, "  @Data points { filledtriangle }");
      break;

    default:

      HnAbort("KheDataSetWrite: illegal value of points_type (%d)",
	ds->points_type);
      break;
  }

  /* print pairs option and data points */
  switch( ds->pairs_type )
  {
    case KHE_DATASET_PAIRS_NONE:

      fprintf(file->fp, "\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_SOLID:

      fprintf(file->fp, " pairs { solid }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_DASHED:

      fprintf(file->fp, " pairs { dashed }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_DOTTED:

      fprintf(file->fp, " pairs { dotted }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_DOT_DASHED:

      fprintf(file->fp, " pairs { dotdashed }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_DOT_DOT_DASHED:

      fprintf(file->fp, " pairs { dotdotdashed }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_DOT_DOT_DOT_DASHED:

      fprintf(file->fp, " pairs { dotdotdotdashed }\n");
      KheDataSetWritePointsRegular(ds, file);
      break;

    case KHE_DATASET_PAIRS_YHISTO:

      fprintf(file->fp, " pairs { yhisto }\n");
      KheDataSetWritePointsYHisto(ds, file);
      break;

    case KHE_DATASET_PAIRS_SURFACE_YHISTO:

      fprintf(file->fp, " pairs { surfaceyhisto }\n");
      KheDataSetWritePointsYHisto(ds, file);
      break;

    case KHE_DATASET_PAIRS_FILLED_YHISTO:

      fprintf(file->fp, " pairs { filledyhisto }\n");
      KheDataSetWritePointsYHisto(ds, file);
      break;

    case KHE_DATASET_PAIRS_XHISTO:

      fprintf(file->fp, " pairs { xhisto }\n");
      HnAbort("KheDataSetWrite: PAIRS_XHISTO not implemented");
      break;

    case KHE_DATASET_PAIRS_SURFACE_XHISTO:

      fprintf(file->fp, " pairs { surfacexhisto }\n");
      HnAbort("KheDataSetWrite: PAIRS_SURFACE_XHISTO not implemented");
      break;

    case KHE_DATASET_PAIRS_FILLED_XHISTO:

      fprintf(file->fp, " pairs { filledyhisto }\n");
      HnAbort("KheDataSetWrite: FILLED_XHISTO not implemented");
      break;

    default:

      HnAbort("KheDataSetWrite: illegal value of pairs_type (%d)",
	ds->pairs_type);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "graphs"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGraphWrite(KHE_GRAPH graph, KHE_FILE file)                       */
/*                                                                           */
/*  Write graph to file.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheGraphWrite(KHE_GRAPH graph, KHE_FILE file)
{
  KHE_DATASET ds;  int i;  char *str;  bool objects_started;

  HnAssert(file->format == KHE_FILE_LOUT ||
    file->format == KHE_FILE_LOUT_STANDALONE,
    "KheGraphWrite: cannot write graph in non-Lout format");
  if( file->format == KHE_FILE_LOUT )
  {
    /* write @Figure (if not standalone) */
    fprintf(file->fp, "@Figure\n");
    fprintf(file->fp, "  @OnePage { Yes }\n");
    if( file->thing_count == 1 )
      fprintf(file->fp, "  @Tag { %s }\n", file->file_name);
    else
      fprintf(file->fp, "  @Tag { %s_%d }\n", file->file_name,
	file->thing_count);
    if( HaArrayCount(graph->caption_lines) > 0 )
    {
      fprintf(file->fp, "  @Caption {\n");
      HaArrayForEach(graph->caption_lines, str, i)
	fprintf(file->fp, "%s", str);
      fprintf(file->fp, "  }\n");
    }
  }
  else
  {
    /* add some surrounding space (if standalone) */
    fprintf(file->fp, "//0.5f ||0.5c ");
  }

  /* write @Graph and its options */
  fprintf(file->fp,
    "-2p @Font -2px @Break @OneRow @Graph hidecaptions { no }\n");
  fprintf(file->fp,
    "  xextra { 0c } yextra { 0c }");
  if( graph->width > 0 )
    fprintf(file->fp, " width { %.2fc }", graph->width);
  if( graph->height > 0 )
    fprintf(file->fp, " height { %.2fc }", graph->height);
  if( graph->xmax > 0 )
    fprintf(file->fp, " xmax { %.2f }", graph->xmax);
  if( graph->ymax > 0 )
    fprintf(file->fp, " ymax { %.2f }", graph->ymax);
  fprintf(file->fp, "\n");

  if( graph->above_caption != NULL )
    fprintf(file->fp, "  abovecaption { %s }\n", graph->above_caption);
  if( graph->below_caption != NULL )
  {
    fprintf(file->fp, "  belowgap { 0.2c }\n");
    fprintf(file->fp, "  belowcaption { %s }\n", graph->below_caption);
  }
  if( graph->left_caption != NULL )
    fprintf(file->fp, "  leftcaption { %s }\n", graph->left_caption);
  if( graph->left_gap != NULL )
    fprintf(file->fp, "  leftgap { %s }\n", graph->left_gap);
  if( graph->right_caption != NULL )
    fprintf(file->fp, "  rightcaption { %s }\n", graph->right_caption);
  if( graph->right_gap != NULL )
    fprintf(file->fp, "  rightgap { %s }\n", graph->right_gap);

  /* print objects option, if there are any non-NULL labels */
  objects_started = false;
  if( graph->key_label != NULL )
  {
    fprintf(file->fp, "  objects { @SE at {0 ymax} @OneRow {\n");
    fprintf(file->fp, "    %s\n", graph->key_label);
    objects_started = true;
  }
  HaArrayForEach(graph->datasets, ds, i)
    if( ds->label != NULL )
    {
      if( objects_started )
	fprintf(file->fp, "    @LLP\n");
      else
	fprintf(file->fp, "  objects { @SE at {0 ymax} @OneRow {\n");
      fprintf(file->fp, "    %s %s %s  %s\n", KhePointsSym(ds->points_type),
	KhePairsSym(ds->pairs_type), KhePointsSym(ds->points_type), ds->label);
      objects_started = true;
    }
  if( objects_started )
    fprintf(file->fp, "  } }\n");

  /* write datasets */
  fprintf(file->fp, "{\n");
  HaArrayForEach(graph->datasets, ds, i)
    KheDataSetWrite(ds, file);

  /* end @Graph */
  fprintf(file->fp, "}\n");

  if( file->format == KHE_FILE_LOUT_STANDALONE )
  {
    /* a bit more surrounding space */
    fprintf(file->fp, "||0.5c\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions - graphs"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheStatsGraphBegin(char *file_name)                                 */
/*                                                                           */
/*  Begin a graph.                                                           */
/*                                                                           */
/*****************************************************************************/

KHE_GRAPH KheGraphBegin(KHE_FILE kf)
{
  KHE_GRAPH res;
  HaMake(res, kf->arena);
  res->file = kf;
  res->width = -1.0;
  res->height = -1.0;
  res->xmax = -1.0;
  res->ymax = -1.0;
  res->above_caption = NULL;
  res->below_caption = NULL;
  res->left_caption = NULL;
  res->left_gap = NULL;
  res->right_caption = NULL;
  res->right_gap = NULL;
  res->key_label = NULL;
  HaArrayInit(res->datasets, kf->arena);
  HaArrayInit(res->caption_lines, kf->arena);
  kf->thing_count++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheStatsGraphEnd(char *file_name)                                   */
/*                                                                           */
/*  End a graph, including printing it.                                      */
/*                                                                           */
/*****************************************************************************/

void KheGraphEnd(KHE_GRAPH kg)
{
  KheGraphWrite(kg, kg->file);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheStatsGraphSetWidth(char *file_name, float width)                 */
/*  void KheStatsGraphSetHeight(char *file_name, float height)               */
/*  void KheStatsGraphSetXMax(char *file_name, float xmax)                   */
/*  void KheStatsGraphSetYMax(char *file_name, float ymax)                   */
/*  void KheStatsGraphSetAboveCaption(char *file_name, char *val)            */
/*  void KheStatsGraphSetBelowCaption(char *file_name, char *val)            */
/*  void KheStatsGraphSetLeftCaption(char *file_name, char *val)             */
/*  void KheStatsGraphSetRightCaption(char *file_name, char *val)            */
/*                                                                           */
/*                                                                           */
/*  Set these various global attributes of the graph.                        */
/*                                                                           */
/*****************************************************************************/

void KheGraphSetWidth(KHE_GRAPH kg, float width)
{
  HnAssert(width > 0, "KheGraphSetWidth: width out of range");
  kg->width = width;
}

void KheGraphSetHeight(KHE_GRAPH kg, float height)
{
  HnAssert(height > 0, "KheGraphSetHeight: height out of range");
  kg->height = height;
}

void KheGraphSetXMax(KHE_GRAPH kg, float xmax)
{
  HnAssert(xmax > 0, "KheGraphSetXMax: xmax out of range");
  kg->xmax = xmax;
}

void KheGraphSetYMax(KHE_GRAPH kg, float ymax)
{
  HnAssert(ymax > 0, "KheGraphSetYMax: ymax out of range");
  kg->ymax = ymax;
}

void KheGraphSetAboveCaption(KHE_GRAPH kg, char *val)
{
  HnAssert(kg->above_caption == NULL,
    "KheGraphSetAboveCaption: above_caption already set");
  kg->above_caption = HnStringCopy(val, kg->file->arena);
}

void KheGraphSetBelowCaption(KHE_GRAPH kg, char *val)
{
  HnAssert(kg->below_caption == NULL,
    "KheGraphSetBelowCaption: below_caption already set");
  kg->below_caption = HnStringCopy(val, kg->file->arena);
}

void KheGraphSetLeftCaptionAndGap(KHE_GRAPH kg, char *val, char *gap)
{
  HnAssert(kg->left_caption == NULL,
    "KheGraphSetLeftCaption: left_caption already set");
  kg->left_caption = HnStringCopy(val, kg->file->arena);
  kg->left_gap = HnStringCopy(gap, kg->file->arena);
}

void KheGraphSetRightCaptionAndGap(KHE_GRAPH kg, char *val, char *gap)
{
  HnAssert(kg->right_caption == NULL,
    "KheGraphSetRightCaption: right_caption already set");
  kg->right_caption = HnStringCopy(val, kg->file->arena);
  kg->right_gap = HnStringCopy(gap, kg->file->arena);
}

void KheGraphSetKeyLabel(KHE_GRAPH kg, char *val)
{
  HnAssert(kg->key_label == NULL, "KheGraphSetKeyLabel: key_label already set");
  kg->key_label = HnStringCopy(val, kg->file->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DATASET KheDataSetAdd(KHE_GRAPH kg,                                  */
/*    KHE_DATASET_POINTS_TYPE points_type,                                   */
/*    KHE_DATASET_PAIRS_TYPE pairs_type, char *label)                        */
/*                                                                           */
/*  Add a new dataset with these attributes to kg.                           */
/*                                                                           */
/*****************************************************************************/

KHE_DATASET KheDataSetAdd(KHE_GRAPH kg,
  KHE_DATASET_POINTS_TYPE points_type,
  KHE_DATASET_PAIRS_TYPE pairs_type, char *label)
{
  KHE_DATASET res;
  res = KheDataSetMake(points_type, pairs_type, label, kg->file->arena);
  HaArrayAddLast(kg->datasets, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePointAdd(KHE_DATASET kd, float x, float y)                       */
/*                                                                           */
/*  Add point (x, y) to ds.                                                  */
/*                                                                           */
/*****************************************************************************/

void KhePointAdd(KHE_DATASET kd, float x, float y)
{
  KHE_POINT point;
  point.x = x;
  point.y = y;
  HaArrayAddLast(kd->points, point);
}

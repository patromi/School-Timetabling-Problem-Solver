
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
/*  FILE:         table.c                                                    */
/*  MODULE:       Building and printing tables.                              */
/*                                                                           */
/*****************************************************************************/
#include <stdlib.h>
#include "table.h"
#include "howard_n.h"

/*****************************************************************************/
/*                                                                           */
/*  TABLE_ENTRY - one entry in a table.                                      */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  TABLE_TEXT,
  TABLE_INT,
  TABLE_DOUBLE
} TABLE_TYPE;

struct table_entry_rec {
  TABLE_JUST		just;		/* justification to use              */
  TABLE_FONT		font;		/* font to use                       */
  TABLE_TYPE		type;		/* type                              */
  char			*text;		/* text to print, or format          */
  int			int_num;	/* formatted by text                 */
  double		double_num;	/* formatted by text                 */
  int			hspan;		/* no. of columns spanned            */
  bool			underline;	/* underline this entry              */
};

typedef HA_ARRAY(TABLE_ENTRY) ARRAY_TABLE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  TABLE_ROW - one row in a table.                                          */
/*                                                                           */
/*****************************************************************************/

struct table_row_rec {
  ARRAY_TABLE_ENTRY	entries;
  bool			overline;
  bool			underline;
};

typedef HA_ARRAY(TABLE_ROW) ARRAY_TABLE_ROW;


/*****************************************************************************/
/*                                                                           */
/*  TABLE - one table, with header and caption                               */
/*                                                                           */
/*****************************************************************************/

struct table_rec {
  char			*header;		/* table header              */
  ARRAY_TABLE_ROW	rows;			/* table rows                */
  HA_ARRAY_NSTRING	caption;		/* table caption             */
  bool			hline_at_top;		/* line across top of table  */
  bool			hline_at_foot;		/* line across foot of table */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table entries - creation"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  TABLE_ENTRY TableEntryMakeText(TABLE_JUST just, TABLE_FONT font,         */
/*    char *text, HA_ARENA a)                                                */
/*                                                                           */
/*  Make a table entry containing text.                                      */
/*                                                                           */
/*****************************************************************************/

TABLE_ENTRY TableEntryMakeText(TABLE_JUST just, TABLE_FONT font,
  char *text, HA_ARENA a)
{
  TABLE_ENTRY res;
  HaMake(res, a);
  res->just = just;
  res->font = font;
  res->type = TABLE_TEXT;
  res->text = text;
  res->int_num = 0;
  res->double_num = 0.0;
  res->hspan = 1;
  res->underline = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE_ENTRY TableEntryMakeInt(TABLE_JUST just, TABLE_FONT font,          */
/*    char *fmt, int num, HA_ARENA a)                                        */
/*                                                                           */
/*  Make a table entry containing integer num.                               */
/*                                                                           */
/*****************************************************************************/

TABLE_ENTRY TableEntryMakeInt(TABLE_JUST just, TABLE_FONT font,
  char *fmt, int num, HA_ARENA a)
{
  TABLE_ENTRY res;
  HaMake(res, a);
  res->just = just;
  res->font = font;
  res->type = TABLE_INT;
  res->text = fmt;
  res->int_num = num;
  res->double_num = 0.0;
  res->hspan = 1;
  res->underline = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE_ENTRY TableEntryMakeDouble(TABLE_JUST just, TABLE_FONT font,       */
/*    char *fmt, double num, HA_ARENA a)                                     */
/*                                                                           */
/*  Make a table entry containing double num.                                */
/*                                                                           */
/*****************************************************************************/

TABLE_ENTRY TableEntryMakeDouble(TABLE_JUST just, TABLE_FONT font,
  char *fmt, double num, HA_ARENA a)
{
  TABLE_ENTRY res;
  HaMake(res, a);
  res->just = just;
  res->font = font;
  res->type = TABLE_DOUBLE;
  res->text = fmt;
  res->int_num = 0;
  res->double_num = num;
  res->hspan = 1;
  res->underline = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table entries - setting rarely used attributes"               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void TableEntrySetHSpan(TABLE_ENTRY entry, int hspan)                    */
/*                                                                           */
/*  Set the hspan attribute of entry.                                        */
/*                                                                           */
/*****************************************************************************/

void TableEntrySetHSpan(TABLE_ENTRY entry, int hspan)
{
  HnAssert(hspan >= 1, "TableEntrySetHSpan: hspan < 1");
  entry->hspan = hspan;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableEntrySetUnderline(TABLE_ENTRY entry, bool underline)           */
/*                                                                           */
/*  Set the underline attribute of entry.                                    */
/*                                                                           */
/*****************************************************************************/

void TableEntrySetUnderline(TABLE_ENTRY entry, bool underline)
{
  entry->underline = underline;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table entries - query"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  TABLE_JUST TableEntryJust(TABLE_ENTRY entry)                             */
/*                                                                           */
/*  Return the just attribute of entry.                                      */
/*                                                                           */
/*****************************************************************************/

TABLE_JUST TableEntryJust(TABLE_ENTRY entry)
{
  return entry->just;
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE_FONT TableEntryFont(TABLE_ENTRY entry)                             */
/*                                                                           */
/*  Return the font attribute of entry.                                      */
/*                                                                           */
/*****************************************************************************/

TABLE_FONT TableEntryFont(TABLE_ENTRY entry)
{
  return entry->font;
}


/*****************************************************************************/
/*                                                                           */
/*  int TableEntryHSpan(TABLE_ENTRY entry)                                   */
/*                                                                           */
/*  Return the hspan attribute of entry.                                     */
/*                                                                           */
/*****************************************************************************/

int TableEntryHSpan(TABLE_ENTRY entry)
{
  return entry->hspan;
}


/*****************************************************************************/
/*                                                                           */
/*  char *TableEntryText(TABLE_ENTRY entry)                                  */
/*                                                                           */
/*  Return the text attribute of entry.                                      */
/*                                                                           */
/*****************************************************************************/

char *TableEntryText(TABLE_ENTRY entry)
{
  return entry->text;
}


/*****************************************************************************/
/*                                                                           */
/*  int TableEntryInt(TABLE_ENTRY entry)                                     */
/*                                                                           */
/*  Return the int_num attribute of entry.                                   */
/*                                                                           */
/*****************************************************************************/

int TableEntryInt(TABLE_ENTRY entry)
{
  return entry->int_num;
}


/*****************************************************************************/
/*                                                                           */
/*  double TableEntryDouble(TABLE_ENTRY entry)                               */
/*                                                                           */
/*  Return the double_num attribute of entry.                                */
/*                                                                           */
/*****************************************************************************/

double TableEntryDouble(TABLE_ENTRY entry)
{
  return entry->double_num;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableEntrySetJust(TABLE_ENTRY entry, TABLE_JUST just)               */
/*                                                                           */
/*  Set the just attribute of entry.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void TableEntrySetJust(TABLE_ENTRY entry, TABLE_JUST just)
{
  entry->just = just;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void TableEntrySetVal(TABLE_ENTRY entry, char *text, double num)         */
/*                                                                           */
/*  Set the text and num attributes of entry.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void TableEntrySetVal(TABLE_ENTRY entry, bool numeric, char *text, double num)
{
  entry->numeric = numeric;
  entry->text = text;
  entry->num = num;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "table rows"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  TABLE_ROW TableRowMake(HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new, empty table row.                                             */
/*                                                                           */
/*****************************************************************************/

TABLE_ROW TableRowMake(HA_ARENA a)
{
  TABLE_ROW res;
  HaMake(res, a);
  HaArrayInit(res->entries, a);
  res->overline = false;
  res->underline = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableRowAddEntry(TABLE_ROW row, TABLE_ENTRY entry)                  */
/*                                                                           */
/*  Add entry to row.                                                        */
/*                                                                           */
/*****************************************************************************/

void TableRowAddEntry(TABLE_ROW row, TABLE_ENTRY entry)
{
  HaArrayAddLast(row->entries, entry);
}


/*****************************************************************************/
/*                                                                           */
/*  void TableRowSetOverline(TABLE_ROW row, bool overline)                   */
/*                                                                           */
/*  Set the overline attribute of row.                                       */
/*                                                                           */
/*****************************************************************************/

void TableRowSetOverline(TABLE_ROW row, bool overline)
{
  row->overline = overline;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableRowSetUnderline(TABLE_ROW row, bool underline)                 */
/*                                                                           */
/*  Set the underline attribute of row.                                      */
/*                                                                           */
/*****************************************************************************/

void TableRowSetUnderline(TABLE_ROW row, bool underline)
{
  row->underline = underline;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tables"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  TABLE TableMake(char *header, HA_ARENA a)                                */
/*                                                                           */
/*  Make a new, empty table with this header.                                */
/*                                                                           */
/*****************************************************************************/

TABLE TableMake(char *header, HA_ARENA a)
{
  TABLE res;
  HaMake(res, a);
  res->header = header;
  HaArrayInit(res->rows, a);
  HaArrayInit(res->caption, a);
  res->hline_at_top = true;
  res->hline_at_foot = true;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableSetHLineAtTop(TABLE table, bool hline_at_top)                  */
/*                                                                           */
/*  Set the hline_at_top attribute of table.                                 */
/*                                                                           */
/*****************************************************************************/

void TableSetHLineAtTop(TABLE table, bool hline_at_top)
{
  table->hline_at_top = hline_at_top;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableSetHLineAtFoot(TABLE table, bool hline_at_foot)                */
/*                                                                           */
/*  Set the hline_at_foot attribute of table.                                */
/*                                                                           */
/*****************************************************************************/

void TableSetHLineAtFoot(TABLE table, bool hline_at_foot)
{
  table->hline_at_foot = hline_at_foot;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableSetHeader(TABLE table, char *header)                           */
/*                                                                           */
/*  Reset the header of table.                                               */
/*                                                                           */
/*****************************************************************************/

void TableSetHeader(TABLE table, char *header)
{
  table->header = header;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableAddRow(TABLE table, TABLE_ROW row)                             */
/*                                                                           */
/*  Add row to table.                                                        */
/*                                                                           */
/*****************************************************************************/

void TableAddRow(TABLE table, TABLE_ROW row)
{
  HaArrayAddLast(table->rows, row);
}


/*****************************************************************************/
/*                                                                           */
/*  void TableAddA erageRow(TABLE table, char *fmt)                          */
/*                                                                           */
/*  Add an average row to table.                                             */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void TableAddAve rageRow(TABLE table, TABLE_JUST just, TABLE_FONT font,
  char *fmt)
{
  TABLE_ROW first_row, average_row, row;  TABLE_ENTRY entry;
  int col_count, i, j;  double col_value;
  if( HaArrayCount(table->rows) > 0 )
  {
    average_row = TableRowMake();
    TableRowAddEntry(average_row,
      TableEntryMakeText(TABLE_INDENT, TABLE_BOLD, "A erage"));
    first_row = MArrayFirst(table->rows);
    for( i = 1;  i < HaArrayCount(first_row->entries);  i++ )
    {
      col_count = 0;  col_value = 0.0;
      for( j = 0;  j < HaArrayCount(table->rows);  j++ )
      {
	row = HaArray(table->rows, j);
	if( i < HaArrayCount(row->entries) )
	{
	  entry = HaArray(row->entries, i);
	  switch( entry->type )
	  {
	    case TABLE_TEXT:

	      break;

	    case TABLE_INT:

	      col_count++, col_value += (double) entry->int_num;
	      break;

	    case TABLE_DOUBLE:

	      col_count++, col_value += entry->double_num;
	      break;

	    default:

	      HnAbort("TableAddAv erageRow internal error (entry type %d)",
                entry->type);
	      break;
	  }
	}
      }
      if( col_count > 0 )
	TableRowAddEntry(average_row,
	  TableEntryMakeDouble(just, font, fmt, col_value / col_count));
      else
	TableRowAddEntry(average_row, TableEntryMakeText(just, font, ""));
    }
    TableAddRow(table, average_row);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void TableClearCaption(TABLE table)                                      */
/*                                                                           */
/*  Clear the caption of table.                                              */
/*                                                                           */
/*****************************************************************************/

void TableClearCaption(TABLE table)
{
  HaArrayClear(table->caption);
}


/*****************************************************************************/
/*                                                                           */
/*  void TableAddCaptionLine(TABLE table, char *str)                         */
/*                                                                           */
/*  Add one line to the caption of table.                                    */
/*                                                                           */
/*****************************************************************************/

void TableAddCaptionLine(TABLE table, char *str)
{
  HaArrayAddLast(table->caption, str);
}


/*****************************************************************************/
/*                                                                           */
/*  int TableRowCount(TABLE table)                                           */
/*                                                                           */
/*  Return the number of rows in table.                                      */
/*                                                                           */
/*****************************************************************************/

int TableRowCount(TABLE table)
{
  return HaArrayCount(table->rows);
}


/*****************************************************************************/
/*                                                                           */
/*  int TableColCount(TABLE table)                                           */
/*                                                                           */
/*  Return the number of columns in table.                                   */
/*                                                                           */
/*****************************************************************************/

int TableColCount(TABLE table)
{
  int i, res;  TABLE_ROW row;
  res = 0;
  HaArrayForEach(table->rows, row, i)
    if( HaArrayCount(row->entries) > res )
      res = HaArrayCount(row->entries);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableDeleteColumn(TABLE table, int col_index)                       */
/*                                                                           */
/*  Remove the column with index col_index from table.                       */
/*                                                                           */
/*****************************************************************************/

static void TableDeleteColumn(TABLE table, int col_index)
{
  int i; TABLE_ROW row;
  HaArrayForEach(table->rows, row, i)
    if( col_index < HaArrayCount(row->entries) )
      HaArrayDeleteAndShift(row->entries, col_index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool TableColHasBoldNonHeader(TABLE table, int col_index)                */
/*                                                                           */
/*  Return true if the column with this index has a bold entry other than    */
/*  in the first row.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool TableColHasBoldNonHeader(TABLE table, int col_index)
{
  TABLE_ROW row;  int i;  TABLE_ENTRY entry;
  for( i = 1;  i < HaArrayCount(table->rows);  i++ )
  {
    row = HaArray(table->rows, i);
    entry = HaArray(row->entries, col_index);
    if( entry->font == TABLE_BOLD )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void TableMakeBestEntriesBold(TABLE table)                               */
/*                                                                           */
/*  Make the best entries in each row of table bold.                         */
/*                                                                           */
/*****************************************************************************/

void TableMakeBestEntriesBold(TABLE table)
{
  TABLE_ROW row;  int i, j;  TABLE_ENTRY entry;  double best_cost;
  for( i = 1;  i < HaArrayCount(table->rows);  i++ )
  {
    row = HaArray(table->rows, i);
    best_cost = -1.0;
    for( j = 1;  j < HaArrayCount(row->entries);  j++ )
    {
      entry = HaArray(row->entries, j);
      switch( entry->type )
      {
	case TABLE_TEXT:

	  break;

	case TABLE_INT:

	  if( best_cost == -1.0 || (double) entry->int_num < best_cost )
	    best_cost = (double) entry->int_num;
	  break;

	case TABLE_DOUBLE:

	  if( best_cost == -1.0 || entry->double_num < best_cost )
	    best_cost = entry->double_num;
	  break;

	default:

	  HnAbort("TableMakeBestEntriesBold internal error (type %d)",
            entry->type);
	  break;
      }
    }
    if( best_cost != -1.0 )
    {
      for( j = 1;  j < HaArrayCount(row->entries);  j++ )
      {
	entry = HaArray(row->entries, j);
	switch( entry->type )
	{
	  case TABLE_TEXT:

	    break;

	  case TABLE_INT:

	    if( (double) entry->int_num <= best_cost )
	      entry->font = TABLE_BOLD;
	    break;

	  case TABLE_DOUBLE:

	    if( entry->double_num <= best_cost )
	      entry->font = TABLE_BOLD;
	    break;

	  default:

	    HnAbort("TableMakeBestEntriesBold internal error (type %d)",
	      entry->type);
	    break;
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int TableRemoveUncompetitiveColumns(ARRAY_ARRAY_ENTRY table)             */
/*                                                                           */
/*  Remove the uncompetitive columns of table, and return a count of         */
/*  how many those were.                                                     */
/*                                                                           */
/*****************************************************************************/

int TableRemoveNonBoldColumns(TABLE table)
{
  int col_index, col_count, res;
  col_count = TableColCount(table);
  res = 0;
  for( col_index = 1;  col_index < col_count;  col_index++ )
    if( !TableColHasBoldNonHeader(table, col_index) )
    {
      TableDeleteColumn(table, col_index);
      col_index--;
      col_count--;
      res++;
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "print"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool TableEntryHasNoText(TABLE_ENTRY entry)                              */
/*                                                                           */
/*  Return true if entry has no visible text.                                */
/*                                                                           */
/*****************************************************************************/

static bool TableEntryHasNoText(TABLE_ENTRY entry)
{
  char *p;
  if( entry->text == NULL )
    return true;
  for( p = entry->text;  *p != '\0';  p++ )
    if( *p != ' ' )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void TablePrintHTML(TABLE table, HTML html)                              */
/*                                                                           */
/*  Print table, including its header and caption, onto html.                */
/*                                                                           */
/*****************************************************************************/

void TablePrintHTML(TABLE table, HTML html)
{
  int i, j;  TABLE_ROW row;  TABLE_ENTRY entry;  char *str;

  /* print the header, if any */
  if( table->header != NULL )
  {
    HTMLParagraphBegin(html);
    HTMLHeading(html, table->header);
    HTMLParagraphEnd(html);
  }

  /* print the table proper */
  HTMLParagraphBegin(html);
  HTMLTableBeginAttributed(html, 3, 0, 1, LightGreen);
  HaArrayForEach(table->rows, row, i)
  {
    HTMLTableRowVAlignBegin(html, "top");
    HaArrayForEach(row->entries, entry, j)
    {
      /* begin table entry, with justification and font */
      if( entry->hspan > 1 )
      {
	if( entry->just == TABLE_CENTRE )
	  HTMLTableEntrySpanCentredBegin(html, entry->hspan, NULL);
	else
	  HTMLTableEntrySpanBegin(html, entry->hspan, NULL);
      }
      else switch( entry->just )
      {
	case TABLE_LEFT:

	  HTMLTableEntryBegin(html);
	  break;

	case TABLE_INDENT:

	  HTMLTableEntryBegin(html);
	  HTMLHSpace(html, 4);
	  break;

	case TABLE_CENTRE:

	  HTMLTableEntryCentredBegin(html);
	  break;

	case TABLE_RIGHT:

	  HTMLTableEntryRightBegin(html);
	  break;

	default:

	  HnAbort("unknown TABLE_JUST (%d)", entry->just);
	  break;
      }

      if( entry->font == TABLE_ITALIC )
	HTMLItalicBegin(html);
      else if( entry->font == TABLE_BOLD )
	HTMLBoldBegin(html);

      /* text of table entry */
      if( TableEntryHasNoText(entry) )
	HTMLHSpace(html, 2);
      else switch( entry->type )
      {
	case TABLE_TEXT:

	  HTMLText(html, entry->text);
	  break;

	case TABLE_INT:

	  HTMLText(html, entry->text, entry->int_num);
	  break;

	case TABLE_DOUBLE:

	  HTMLText(html, entry->text, entry->double_num);
	  break;

	default:

	  HnAbort("TablePrintHTML internal error (entry type %d)", entry->type);
	  break;
      }

      /* end table entry, with justification and font */
      if( entry->font == TABLE_ITALIC )
	HTMLItalicEnd(html);
      else if( entry->font == TABLE_BOLD )
	HTMLBoldEnd(html);
      HTMLTableEntryEnd(html);
    }
    HTMLTableRowEnd(html);
  }
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);

  /* print the caption, if any */
  if( HaArrayCount(table->caption) > 0 )
  {
    HTMLParagraphBegin(html);
    HaArrayForEach(table->caption, str, i)
    {
      if( str == NULL )
      {
	HTMLParagraphEnd(html);
	HTMLParagraphBegin(html);
      }
      HTMLText(html, str);
    }
    HTMLParagraphEnd(html);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void TablePrintLaTeX(TABLE table, FILE *fp)                              */
/*                                                                           */
/*  Print table in LaTeX format onto fp.                                     */
/*                                                                           */
/*****************************************************************************/

void TablePrintLaTeX(TABLE table, FILE *fp)
{
  int i, j, max_cols, col;  TABLE_ROW row;  TABLE_ENTRY entry;  char *str;
  bool need_strut;

  /* find the maximum number of columns */
  max_cols = 0;
  HaArrayForEach(table->rows, row, i)
    if( max_cols < HaArrayCount(row->entries) )
      max_cols = HaArrayCount(row->entries);

  /* print an initial paragraph break */
  fprintf(fp, "\n");

  /* print the header, if any */
  if( table->header != NULL )
    fprintf(fp, "%%%s\n\n", table->header);

  /* print the table proper */
  fprintf(fp, "%s", "{\\renewcommand{\\tabcolsep}{0cm}\n");
  fprintf(fp, "%s", "\\begin{tabular}{");
  for( j = 0;  j < max_cols;  j++ )
  {
    fprintf(fp, "%c", 'r');
    if( j == 0 )
      fprintf(fp, "@{\\extracolsep{%s}}", max_cols <= 6 ? "0.5cm" :
	max_cols <= 8 ? "0.4cm" : "0.35cm");
  }
  /* ***
  i = HaArrayCount(table->rows) == 1 ? 0 : 1;
  row = HaArray(table->rows, i);
  HaArrayForEach(row->entries, entry, j)
  {
    if( entry->just == TABLE_LEFT )
      fprintf(fp, "%c", 'l');
    else if( entry->just == TABLE_CENTRE )
      fprintf(fp, "%c", 'c');
    else if( entry->just == TABLE_RIGHT )
      fprintf(fp, "%c", 'r');
    if( j == 0 )
      fprintf(fp, "%s", "@{\\extracolsep{0.4cm}}");
  }
  *** */
  fprintf(fp, "%s", "}\n");
  if( table->hline_at_top )
    fprintf(fp, "%s", "\\hline");
  fprintf(fp, "%s", "\\noalign{\\smallskip}\n");

  /* rows and columns */
  need_strut = false;
  HaArrayForEach(table->rows, row, i)
  {
    /* print overline, if requested */
    if( row->overline )
      fprintf(fp, "\\hline ");

    /* print the entries of row */
    HaArrayForEach(row->entries, entry, j)
    {
      /* move to new column if not first */
      if( j > 0 )
	fprintf(fp, "%s", " & ");

      /* multicolumn if needed by span or just */
      if( entry->hspan > 1 || entry->just != TABLE_RIGHT )
	fprintf(fp, "\\multicolumn{%d}{%c}{", entry->hspan,
          entry->just == TABLE_CENTRE ? 'c' : 'l');

      /* change font if not roman */
      if( entry->font == TABLE_BOLD )
	fprintf(fp, "%s", "{\\bf ");
      else if( entry->font == TABLE_ITALIC )
	fprintf(fp, "%s", "{\\em ");

      /* print indent if required */
      if( entry->just == TABLE_INDENT )
	fprintf(fp, "%s", "\\quad ");

      /* print strut if required */
      if( need_strut )
      {
	fprintf(fp, "%s", "\\rule{0pt}{2.6ex}");
	need_strut = false;
      }

      /* print entry content */
      switch( entry->type )
      {
	case TABLE_TEXT:

	  fprintf(fp, "%s", entry->text);
	  break;

	case TABLE_INT:

	  fprintf(fp, entry->text, entry->int_num);
	  break;

	case TABLE_DOUBLE:

	  fprintf(fp, entry->text, entry->double_num);
	  break;

	default:

	  HnAbort("TablePrintLaTeX internal error (entry type %d)",entry->type);
	  break;
      }

      /* finish off any font change */
      if( entry->font == TABLE_BOLD || entry->font == TABLE_ITALIC )
	fprintf(fp, "%s", "}");

      /* finish off any multicolumn */
      if( entry->hspan > 1 || entry->just != TABLE_RIGHT )
	fprintf(fp, "%s", "}");
    }

    /* terminate the row and print any underlines */
    need_strut = false;
    fprintf(fp, "%s", " \\\\\n");
    col = 1;
    HaArrayForEach(row->entries, entry, j)
    {
      if( entry->underline )
      {
	fprintf(fp, "\\cline{%d-%d} ", col, col + entry->hspan - 1);
	need_strut = true;
      }
      col += entry->hspan;
    }

    if( row->underline )
      fprintf(fp, "\\hline ");
  }

  /* footer */
  fprintf(fp, "%s", "\\noalign{\\smallskip}");
  if( table->hline_at_foot )
    fprintf(fp, "%s", "\\hline");
  fprintf(fp, "%s", "\n\\end{tabular}}\n");

  /* print the caption, if any */
  if( HaArrayCount(table->caption) > 0 )
  {
    fprintf(fp, "\n");
    HaArrayForEach(table->caption, str, i)
      fprintf(fp, "%%%s\n", str == NULL ? "" : str);
  }
}

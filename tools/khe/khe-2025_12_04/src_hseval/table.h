
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
/*  FILE:         table.h                                                    */
/*  MODULE:       Building and printing tables.                              */
/*                                                                           */
/*****************************************************************************/
#ifndef TABLE_HEADER_FILE
#define TABLE_HEADER_FILE
#include "html.h"
#include "howard_a.h"

typedef struct table_entry_rec *TABLE_ENTRY;
typedef struct table_row_rec *TABLE_ROW;
typedef struct table_rec *TABLE;

typedef enum {
  TABLE_ROMAN,
  TABLE_ITALIC,
  TABLE_BOLD
} TABLE_FONT;

typedef enum {
  TABLE_LEFT,
  TABLE_INDENT,
  TABLE_CENTRE,
  TABLE_RIGHT,
} TABLE_JUST;

/* table entries - creation */
extern TABLE_ENTRY TableEntryMakeText(TABLE_JUST just, TABLE_FONT font,
  char *text, HA_ARENA a);
extern TABLE_ENTRY TableEntryMakeInt(TABLE_JUST just, TABLE_FONT font,
  char *fmt, int num, HA_ARENA a);
extern TABLE_ENTRY TableEntryMakeDouble(TABLE_JUST just, TABLE_FONT font,
  char *fmt, double num, HA_ARENA a);

/* table entries - setting rarely used attributes */
extern void TableEntrySetHSpan(TABLE_ENTRY entry, int hspan);
extern void TableEntrySetUnderline(TABLE_ENTRY entry, bool underline);

/* table entries - query */
extern TABLE_JUST TableEntryJust(TABLE_ENTRY entry);
extern TABLE_FONT TableEntryFont(TABLE_ENTRY entry);
extern int TableEntryHSpan(TABLE_ENTRY entry);
extern char *TableEntryText(TABLE_ENTRY entry);
extern int TableEntryInt(TABLE_ENTRY entry);
extern double TableEntryDouble(TABLE_ENTRY entry);
/* ***
extern void TableEntrySetJust(TABLE_ENTRY entry, TABLE_JUST just);
extern void TableEntrySetVal(TABLE_ENTRY entry, bool numeric, char *text,
  double num);
*** */

/* table rows */
extern TABLE_ROW TableRowMake(HA_ARENA a);
extern void TableRowAddEntry(TABLE_ROW row, TABLE_ENTRY entry);
extern void TableRowSetOverline(TABLE_ROW row, bool overline);
extern void TableRowSetUnderline(TABLE_ROW row, bool underline);

/* tables */
extern TABLE TableMake(char *header, HA_ARENA a);
extern void TableSetHLineAtTop(TABLE table, bool hline_at_top);
extern void TableSetHLineAtFoot(TABLE table, bool hline_at_foot);
extern void TableSetHeader(TABLE table, char *header);
extern void TableAddRow(TABLE table, TABLE_ROW row);
/* ***
extern void TableAddAverageRow(TABLE table, TABLE_JUST just, TABLE_FONT font,
  char *fmt);
*** */
extern void TableClearCaption(TABLE table);
extern void TableAddCaptionLine(TABLE table, char *str);
extern int TableRowCount(TABLE table);
extern int TableColCount(TABLE table);
extern void TableMakeBestEntriesBold(TABLE table);
extern int TableRemoveNonBoldColumns(TABLE table);

/* print */
void TablePrintHTML(TABLE table, HTML html);
void TablePrintLaTeX(TABLE table, FILE *fp);

#endif

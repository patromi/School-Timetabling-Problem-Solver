
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
/*  FILE:         html.c                                                     */
/*  MODULE:       HTML generation                                            */
/*                                                                           */
/*****************************************************************************/
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include "url.h"
#include "html.h"
#include "howard_n.h"
#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  html_rec - the HTML structure.                                           */
/*                                                                           */
/*****************************************************************************/

struct html_rec {
  FILE			*fp;			/* file to write HTML to     */
  char			*front_page;		/* front page of system      */
  char			*front_page_text;	/* label of front page       */
  int			table_spacing;		/* table spacing width       */
  int			table_padding;		/* table padding width       */
  int			table_border;		/* table border width        */
  char			*table_bgcolor;		/* table background colour   */
  int			indent_width;		/* display indent width      */
  char			*radio_name;		/* current radio button name */
  char			*radio_checked;		/* current radio checked name*/
  bool			selecting;		/* true if inside a Select   */
  char			*selected;		/* selected value of select  */
  char			*checkbox_prefix;	/* prefix of checkbox names  */
  FILE			*fp2;
};


static void HTMLCheck(HTML html)
{
  if( DEBUG1 && html->fp != stdout )
  {
    fprintf(stderr, "HTMLCheck found non-stdout html object");
    exit(1);
  }
  if( DEBUG1 && html->fp != html->fp2 )
  {
    fprintf(stderr, "HTMLCheck found corrupt html object");
    exit(1);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  HTML HTMLMake(char *front_page, char *front_page_text, FILE *fp)         */
/*                                                                           */
/*  Make a new HTML object with this file pointer and suitable default       */
/*  values for the other attributes.                                         */
/*                                                                           */
/*****************************************************************************/

static HTML HTMLMake(char *front_page, char *front_page_text, FILE *fp)
{
  HTML res;
  res = (HTML) malloc(sizeof(struct html_rec));
  res->front_page = front_page;
  res->front_page_text = front_page_text;
  res->fp = fp;
  res->table_spacing = 2;
  res->table_padding = 1;
  res->table_border = 1;
  res->table_bgcolor = NULL;  /* i.e. no colour */
  res->indent_width = 12;
  res->radio_name = NULL;
  res->radio_checked = "";
  res->selecting = false;
  res->checkbox_prefix = NULL;
  res->fp2 = fp;
  HTMLCheck(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void Tag0(HTML html, char *name)                                         */
/*  void Tag1(HTML html, char *name, char *str1, char *val1)                 */
/*  void Tag2(HTML html, char *name, char *str1, char *val1,                 */
/*    char *str2, char *val2)                                                */
/*  void Tag3(HTML html, char *name, char *str1, char *val1,                 */
/*    char *str2, char *val2, char *str3, char *val3)                        */
/*  void Tag4(HTML html, char *name, char *str1, char *val1,                 */
/*    char *str2, char *val2, char *str3, char *val3,                        */
/*    char *str4, char *val4)                                                */
/*  void Tag5(HTML html, char *name, char *str1, char *val1,                 */
/*    char *str2, char *val2, char *str3, char *val3,                        */
/*    char *str4, char *val4, char *str5, char *val5)                        */
/*                                                                           */
/*  Print an HTML tag with 0, 1, 2, 3, or 4 associated options.              */
/*  If any of the values are NULL, it means to print the string with         */
/*  no associated value.                                                     */
/*                                                                           */
/*****************************************************************************/

static void one_pair(FILE *fp, char *str, char *val)
{
  if( DEBUG1 )
    fprintf(stderr, "  one_pair(fp, %s, %s)\n",
      str == NULL ? "(null)" : str, val == NULL ? "(null)" : val);
  if( str == NULL )
  {
    /* print nothing */
  }
  else if( val == NULL )
    fprintf(fp, " %s", str);
  else
    fprintf(fp, " %s=\"%s\"", str, val);
}

static void Tag0(HTML html, char *name)
{
  HTMLCheck(html);
  fprintf(html->fp, "<%s>\n", name);
}

static void Tag1(HTML html, char *name, char *str1, char *val1)
{
  HTMLCheck(html);
  fprintf(html->fp, "<%s", name);
  one_pair(html->fp, str1, val1);
  fprintf(html->fp, ">\n");
}

static void Tag2(HTML html, char *name, char *str1, char *val1,
  char *str2, char *val2)
{
  HTMLCheck(html);
  if( DEBUG1 )
    fprintf(stderr, "  Tag2(html, %s, %s, %s, %s, %s)\n", name, str1, val1,
      str2, val2);
  fprintf(html->fp, "<%s", name);
  one_pair(html->fp, str1, val1);
  one_pair(html->fp, str2, val2);
  fprintf(html->fp, ">\n");
}

static void Tag3(HTML html, char *name, char *str1, char *val1,
  char *str2, char *val2, char *str3, char *val3)
{
  HTMLCheck(html);
  fprintf(html->fp, "<%s", name);
  one_pair(html->fp, str1, val1);
  one_pair(html->fp, str2, val2);
  one_pair(html->fp, str3, val3);
  fprintf(html->fp, ">\n");
}

static void Tag4(HTML html, char *name, char *str1, char *val1,
  char *str2, char *val2, char *str3, char *val3,
  char *str4, char *val4)
{
  HTMLCheck(html);
  fprintf(html->fp, "<%s", name);
  one_pair(html->fp, str1, val1);
  one_pair(html->fp, str2, val2);
  one_pair(html->fp, str3, val3);
  one_pair(html->fp, str4, val4);
  fprintf(html->fp, ">\n");
}

static void Tag5(HTML html, char *name, char *str1, char *val1,
  char *str2, char *val2, char *str3, char *val3,
  char *str4, char *val4, char *str5, char *val5)
{
  HTMLCheck(html);
  fprintf(html->fp, "<%s", name);
  one_pair(html->fp, str1, val1);
  one_pair(html->fp, str2, val2);
  one_pair(html->fp, str3, val3);
  one_pair(html->fp, str4, val4);
  one_pair(html->fp, str5, val5);
  fprintf(html->fp, ">\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void TagEnd(HTML html, char *name)                                       */
/*                                                                           */
/*  Print an HTML closing tag.                                               */
/*                                                                           */
/*****************************************************************************/

static void TagEnd(HTML html, char *name)
{
  HTMLCheck(html);
  fprintf(html->fp, "</%s>\n", name);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Basic Text Elements"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLLiteralText(HTML html, char *text)                              */
/*                                                                           */
/*  Print some literal text; that is, encode <, >, and &.                    */
/*                                                                           */
/*****************************************************************************/

void HTMLLiteralText(HTML html, char *text)
{
  char *p;
  for( p = text;  *p != '\0';  p++ ) switch( *p )
  {
    case '&': fprintf(html->fp, "&amp");
	      break;

    case '<': fprintf(html->fp, "&lt;");
	      break;

    case '>': fprintf(html->fp, "&gt;");
	      break;

    default:  fputc((unsigned char) *p, html->fp);
	      break;
  }
  fputc('\n', html->fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLLiteralTextIndented(HTML html, int indent, char *text)          */
/*                                                                           */
/*  Print text literally, on a line by itself with the given indent.         */
/*                                                                           */
/*****************************************************************************/

void HTMLLiteralTextIndented(HTML html, int indent, char *text)
{
  HTMLHSpace(html, indent);
  HTMLLiteralText(html, text);
  HTMLNewLine(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLText(HTML html, char *fmt, ...)                                 */
/*                                                                           */
/*  Print some text.  Don't do anything about <, >  etc. inside it.          */
/*                                                                           */
/*****************************************************************************/

void HTMLText(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  fprintf(html->fp, "\n");
  va_end(args);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextNoBreak(HTML html, char *fmt, ...)                          */
/*                                                                           */
/*  Print some text without the following line break.                        */
/*                                                                           */
/*****************************************************************************/

void HTMLTextNoBreak(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextItalic(HTML html, char *fmt, ...)                           */
/*                                                                           */
/*  Print some italic text.                                                  */
/*                                                                           */
/*****************************************************************************/

void HTMLTextItalic(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<i>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</i>\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextItalicNoBreak(HTML html, char *fmt, ...)                    */
/*                                                                           */
/*  Print some italic text with no following break.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTextItalicNoBreak(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<i>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</i>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBold(HTML html, char *fmt, ...)                             */
/*                                                                           */
/*  Print some bold text.                                                    */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBold(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<b>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</b>\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBoldNoBreak(HTML html, char *fmt, ...)                      */
/*                                                                           */
/*  Print some bold text with no following break.                            */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBoldNoBreak(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<b>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</b>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBoldItalic(HTML html, char *fmt, ...)                       */
/*                                                                           */
/*  Print some bold italic text.                                             */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBoldItalic(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<b><i>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</i></b>\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBoldItalicNoBreak(HTML html, char *fmt, ...)                */
/*                                                                           */
/*  Print some bold italic text with no following break.                     */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBoldItalicNoBreak(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<b><i>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</i></b>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextMono(HTML html, char *fmt, ...)                             */
/*                                                                           */
/*  Print some monospaced text.                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLTextMono(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<tt>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</tt>\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextMonoNoBreak(HTML html, char *fmt, ...)                      */
/*                                                                           */
/*  Print some monospaced text with no following break.                      */
/*                                                                           */
/*****************************************************************************/

void HTMLTextMonoNoBreak(HTML html, char *fmt, ...)
{
  va_list args;
  HTMLCheck(html);
  fprintf(html->fp, "<tt>");
  va_start(args, fmt);
  vfprintf(html->fp, fmt, args);
  va_end(args);
  fprintf(html->fp, "</tt>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLFontBegin(HTML html, HTML_FONT font)                            */
/*                                                                           */
/*  Begin font.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLFontBegin(HTML html, HTML_FONT font)
{
  switch( font )
  {
    case HTML_ROMAN:

      fprintf(html->fp, "<r>");
      break;

    case HTML_ITALIC:

      fprintf(html->fp, "<i>");
      break;

    case HTML_BOLD:

      fprintf(html->fp, "<b>");
      break;

    case HTML_BOLD_ITALIC:

      fprintf(html->fp, "<b><i>");
      break;

    case HTML_MONO:

      fprintf(html->fp, "<tt>");
      break;

    default:

      HnAbort("HTMLFontBegin: unknown font %s", font);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLFontEnd(HTML html, HTML_FONT font)                              */
/*                                                                           */
/*  End font.  This must match the corresponding HTMLBeginFont.              */
/*                                                                           */
/*****************************************************************************/

void HTMLFontEnd(HTML html, HTML_FONT font)
{
  switch( font )
  {
    case HTML_ROMAN:

      fprintf(html->fp, "</r>");
      break;

    case HTML_ITALIC:

      fprintf(html->fp, "</i>");
      break;

    case HTML_BOLD:

      fprintf(html->fp, "</b>");
      break;

    case HTML_BOLD_ITALIC:

      fprintf(html->fp, "</i></b>");
      break;

    case HTML_MONO:

      fprintf(html->fp, "</tt>");
      break;

    default:

      HnAbort("HTMLFontEnd: unknown font %s", font);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSmallBegin(HTML html)                                           */
/*                                                                           */
/*  Begin small text.                                                        */
/*                                                                           */
/*****************************************************************************/

void HTMLSmallBegin(HTML html)
{
  Tag0(html, "small");
  Tag0(html, "small");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSmallEnd(HTML html)                                             */
/*                                                                           */
/*  End small text.                                                          */
/*                                                                           */
/*****************************************************************************/

void HTMLSmallEnd(HTML html)
{
  TagEnd(html, "small");
  TagEnd(html, "small");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBoldBegin(HTML html)                                            */
/*                                                                           */
/*  Begin bold.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLBoldBegin(HTML html)
{
  Tag0(html, "b");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBoldEnd(HTML html)                                              */
/*                                                                           */
/*  End bold.                                                                */
/*                                                                           */
/*****************************************************************************/

void HTMLBoldEnd(HTML html)
{
  TagEnd(html, "b");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLItalicBegin(HTML html)                                          */
/*                                                                           */
/*  Begin italic.                                                            */
/*                                                                           */
/*****************************************************************************/

void HTMLItalicBegin(HTML html)
{
  Tag0(html, "i");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLItalicEnd(HTML html)                                            */
/*                                                                           */
/*  End italic.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLItalicEnd(HTML html)
{
  TagEnd(html, "i");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLMonoBegin(HTML html)                                            */
/*                                                                           */
/*  Begin monospaced text.                                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLMonoBegin(HTML html)
{
  Tag0(html, "tt");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLMonoEnd(HTML html)                                              */
/*                                                                           */
/*  End monospaced text.                                                     */
/*                                                                           */
/*****************************************************************************/

void HTMLMonoEnd(HTML html)
{
  TagEnd(html, "tt");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHSpace(HTML html, int spaces)                                   */
/*                                                                           */
/*  Print spaces horizontal spaces.                                          */
/*                                                                           */
/*****************************************************************************/

void HTMLHSpace(HTML html, int spaces)
{
  int i;
  HTMLCheck(html);
  for( i = 0;  i < spaces;  i++ )
    fprintf(html->fp, "%s", "&nbsp;");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLVSpace(HTML html, int spaces)                                   */
/*                                                                           */
/*  Leave spaces paragraph spaces.                                           */
/*                                                                           */
/*****************************************************************************/

void HTMLVSpace(HTML html, int spaces)
{
  int i;
  for( i = 0;  i < spaces;  i++ )
    fprintf(html->fp, "<p>&nbsp;</p>\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLNewLine(HTML html)                                              */
/*                                                                           */
/*  Start a new line.                                                        */
/*                                                                           */
/*****************************************************************************/

void HTMLNewLine(HTML html)
{
  Tag0(html, "br");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLParagraphBegin(HTML html)                                       */
/*                                                                           */
/*  Begin a new paragraph.                                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLParagraphBegin(HTML html)
{
  Tag0(html, "p");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLParagraphEnd(HTML html)                                         */
/*                                                                           */
/*  End a paragraph.                                                         */
/*                                                                           */
/*****************************************************************************/

void HTMLParagraphEnd(HTML html)
{
  TagEnd(html, "p");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBigHeading(HTML html, char *heading)                            */
/*                                                                           */
/*  Make a large, centred heading.                                           */
/*                                                                           */
/*****************************************************************************/

void HTMLBigHeading(HTML html, char *heading)
{
  Tag1(html, "h1", "align", "center");
  fprintf(html->fp, "%s", heading);
  TagEnd(html, "h1");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHeading(HTML html, char *heading)                               */
/*                                                                           */
/*  Make a smaller heading.                                                  */
/*                                                                           */
/*****************************************************************************/

void HTMLHeading(HTML html, char *heading)
{
  Tag0(html, "h2");
  fprintf(html->fp, "%s", heading);
  TagEnd(html, "h2");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHeadingBegin(HTML html)                                         */
/*                                                                           */
/*  Begin an html heading.                                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLHeadingBegin(HTML html)
{
  Tag0(html, "h2");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHeadingEnd(HTML html)                                           */
/*                                                                           */
/*  End an HTML heading.                                                     */
/*                                                                           */
/*****************************************************************************/

void HTMLHeadingEnd(HTML html)
{
  TagEnd(html, "h2");
}



/*****************************************************************************/
/*                                                                           */
/*  void HTMLHorizontalRule(HTML html)                                       */
/*                                                                           */
/*  Make a horizontal rule.                                                  */
/*                                                                           */
/*****************************************************************************/

void HTMLHorizontalRule(HTML html)
{
  Tag0(html, "hr");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLIndentBegin(HTML html)                                          */
/*                                                                           */
/*  Begin an indented region.  Sadly, ideological hangups have prevented     */
/*  the HTML people from allowing this to be done in a rational manner.      */
/*                                                                           */
/*****************************************************************************/

void HTMLIndentBegin(HTML html)
{
  HTMLParagraphBegin(html);
  HTMLTableBeginAttributed(html, 0, 0, 0, NULL);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
  HTMLHSpace(html, html->indent_width);
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLIndentEnd(HTML html)                                            */
/*                                                                           */
/*  End an indented region.                                                  */
/*                                                                           */
/*****************************************************************************/

void HTMLIndentEnd(HTML html)
{
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Displays"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLCentredDisplayBegin(HTML html)                                  */
/*                                                                           */
/*  Begin a centred display.                                                 */
/*                                                                           */
/*****************************************************************************/

void HTMLCentredDisplayBegin(HTML html)
{
  Tag1(html, "div", "align", "center");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLCentredDisplayEnd(HTML html)                                    */
/*                                                                           */
/*  End a centred display.                                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLCentredDisplayEnd(HTML html)
{
  TagEnd(html, "div");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Lists"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLNumberedListBegin(HTML html)                                    */
/*  void HTMLNumberedListEnd(HTML html)                                      */
/*                                                                           */
/*  Begin/end a numbered list.                                               */
/*                                                                           */
/*****************************************************************************/

void HTMLNumberedListBegin(HTML html)
{
  Tag0(html, "ol");
}


void HTMLNumberedListEnd(HTML html)
{
  TagEnd(html, "ol");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBulletListBegin(HTML html)                                      */
/*  void HTMLBulletListEnd(HTML html)                                        */
/*                                                                           */
/*  Begin/end a bullet list.                                                 */
/*                                                                           */
/*****************************************************************************/

void HTMLBulletListBegin(HTML html)
{
  Tag0(html, "ul");
}


void HTMLBulletListEnd(HTML html)
{
  TagEnd(html, "ul");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLListItemBegin(HTML html)                                        */
/*  void HTMLListItemEnd(HTML html)                                          */
/*                                                                           */
/*  Begin/end a list item of either type.                                    */
/*                                                                           */
/*****************************************************************************/

void HTMLListItemBegin(HTML html)
{
  Tag0(html, "li");
}


void HTMLListItemEnd(HTML html)
{
  TagEnd(html, "li");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Tables"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLSetTableOptions(HTML html, int spacing, int padding, int border)*/
/*                                                                           */
/*  Set the options that control the appearance of tables.                   */
/*                                                                           */
/*****************************************************************************/

void HTMLSetTableOptions(HTML html, int spacing, int padding, int border,
  char *bgcolor)
{
  html->table_spacing = spacing;
  html->table_padding = padding;
  html->table_border = border;
  html->table_bgcolor = bgcolor;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBeginAttributed(HTML html, int cellpadding,                */
/*    int cellspacing, int border)                                           */
/*                                                                           */
/*  Begin a table using the given attributes rather than the usual ones.     */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBeginAttributed(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor)
{
  char buff1[10], buff2[10], buff3[10];
  snprintf(buff1, 10, "%d", cellpadding);
  snprintf(buff2, 10, "%d", cellspacing);
  snprintf(buff3, 10, "%d", border);
  if( bgcolor == NULL )
    Tag3(html, "table", "cellpadding", buff1, "cellspacing", buff2,
      "border", buff3);
  else
    Tag4(html, "table", "cellpadding", buff1, "cellspacing", buff2,
      "border", buff3, "bgcolor", bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBeginAttributedFractionalWidth(HTML html, int cellpadding, */
/*    int cellspacing, int border, char *bgcolor, int percent)               */
/*                                                                           */
/*  Begin a fractional-width table with the given attributes.                */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBeginAttributedFractionalWidth(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor, int percent)
{
  char buff1[10], buff2[10], buff3[10], buff4[10];
  snprintf(buff1, 10, "%d", cellpadding);
  snprintf(buff2, 10, "%d", cellspacing);
  snprintf(buff3, 10, "%d", border);
  snprintf(buff4, 10, "%d%%", percent);
  if( bgcolor == NULL )
    Tag4(html, "table", "cellpadding", buff1, "cellspacing", buff2,
      "border", buff3, "width", buff4);
  else
    Tag5(html, "table", "cellpadding", buff1, "cellspacing", buff2,
      "border", buff3, "bgcolor", bgcolor, "width", buff4);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBeginAttributedFullWidth(HTML html, int cellpadding,       */
/*    int cellspacing, int border)                                           */
/*                                                                           */
/*  Begin a full-width table using the given attributes as above.            */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBeginAttributedFullWidth(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor)
{
  HTMLTableBeginAttributedFractionalWidth(html, cellpadding,
    cellspacing, border, bgcolor, 100);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBegin(HTML html, char *bgcolor)                            */
/*                                                                           */
/*  Begin an HTML table using preassigned attributes and bgcolor.            */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBegin(HTML html, char *bgcolor)
{
  HTMLTableBeginAttributed(html, html->table_padding,
    html->table_spacing, html->table_border, bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBeginFullWidth(HTML html, char *bgcolor)                   */
/*                                                                           */
/*  Begin a full-width table.                                                */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBeginFullWidth(HTML html, char *bgcolor)
{
  HTMLTableBeginAttributedFullWidth(html, html->table_padding,
    html->table_spacing, html->table_border, bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableBeginFractionalWidth(HTML html, char *bgcolor, int percent)*/
/*                                                                           */
/*  Begin a fractional width table with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

void HTMLTableBeginFractionalWidth(HTML html, char *bgcolor, int percent)
{
  HTMLTableBeginAttributedFractionalWidth(html, html->table_padding,
    html->table_spacing, html->table_border, bgcolor, percent);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEnd(HTML html)                                             */
/*                                                                           */
/*  End an HTML table.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEnd(HTML html)
{
  TagEnd(html, "table");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableRowBegin(HTML html)                                        */
/*                                                                           */
/*  Begin a table row.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTableRowBegin(HTML html)
{
  Tag0(html, "tr");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableRowVAlignBegin(HTML html, char *valign)                    */
/*                                                                           */
/*  Begin a top-aligned table row.                                           */
/*                                                                           */
/*****************************************************************************/

void HTMLTableRowVAlignBegin(HTML html, char *valign)
{
  Tag1(html, "tr", "valign", valign);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableRowColouredVAlignBegin(HTML html, char *bgcolor,           */
/*    char *valign)                                                          */
/*                                                                           */
/*  Begin an aligned table row with a background colour.                     */
/*                                                                           */
/*****************************************************************************/

void HTMLTableRowColouredVAlignBegin(HTML html, char *bgcolor, char *valign)
{
  if( bgcolor == NULL )
  {
    if( valign == NULL )
      Tag0(html, "tr");
    else
      Tag1(html, "tr", "valign", valign);
  }
  else
  {
    if( valign == NULL )
      Tag1(html, "tr", "bgcolor", bgcolor);
    else
      Tag2(html, "tr", "bgcolor", bgcolor, "valign", valign);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableRowEnd(HTML html)                                          */
/*                                                                           */
/*  End a table row.                                                         */
/*                                                                           */
/*****************************************************************************/

void HTMLTableRowEnd(HTML html)
{
  TagEnd(html, "tr");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryBegin(HTML html)                                      */
/*                                                                           */
/*  Begin a table entry.                                                     */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryBegin(HTML html)
{
  fprintf(html->fp, "<td>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryCentredBegin(HTML html)                               */
/*                                                                           */
/*  Begin a centred entry.                                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryCentredBegin(HTML html)
{
  fprintf(html->fp, "<td align=center>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryRightBegin(HTML html)                                 */
/*                                                                           */
/*  Begin a right-justified entry.                                           */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryRightBegin(HTML html)
{
  fprintf(html->fp, "<td align=right>");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryBeginFractionalWidth(HTML html, int percentage,       */
/*    char *bgcolor)                                                         */
/*                                                                           */
/*  Begin a table entry that is to occupy the given fraction of the          */
/*  full width.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryBeginFractionalWidth(HTML html, int percentage,
  char *bgcolor)
{
  if( bgcolor == NULL )
    fprintf(html->fp, "<td width=\"%d%%\">", percentage);
  else
    fprintf(html->fp, "<td width=\"%d%%\" bgcolor=\"%s\">",
      percentage, bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryBeginCentredFractionalWidth(HTML html,                */
/*    int percentage, char *bgcolor)                                         */
/*                                                                           */
/*  Begin a table entry that is to occupy the given fraction of the          */
/*  full width.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryBeginCentredFractionalWidth(HTML html, int percentage,
  char *bgcolor)
{
  if( bgcolor == NULL )
    fprintf(html->fp, "<td align=center width=\"%d%%\">", percentage);
  else
    fprintf(html->fp, "<td align=center width=\"%d%%\" bgcolor=\"%s\">",
      percentage, bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryColouredBegin(HTML html, char *bgcolor)               */
/*                                                                           */
/*  Begin a table entry with the given (optional) background colour.         */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryColouredBegin(HTML html, char *bgcolor)
{
  if( bgcolor == NULL )
    fprintf(html->fp, "<td>");
  else
    fprintf(html->fp, "<td bgcolor=\"%s\">", bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryColouredCentredBegin(HTML html, char *bgcolor)        */
/*                                                                           */
/*  Begin a centred table entry with the given (optional) background colour. */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryColouredCentredBegin(HTML html, char *bgcolor)
{
  if( bgcolor == NULL )
    fprintf(html->fp, "<td align=center>");
  else
    fprintf(html->fp, "<td align=center bgcolor=\"%s\">", bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryEnd(HTML html)                                        */
/*                                                                           */
/*  End a table entry.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryEnd(HTML html)
{
  if( DEBUG1 )
    fprintf(stderr, "[ HTMLTableEntryEnd(html)\n");
  HTMLCheck(html);
  fprintf(html->fp, "</td>\n");
  if( DEBUG1 )
    fprintf(stderr, "] HTMLTableEntryEnd(html)\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryText(HTML html, char *text)                           */
/*                                                                           */
/*  Make a table entry containing some text.                                 */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryText(HTML html, char *text)
{
  HTMLTableEntryBegin(html);
  HTMLText(html, text);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryTextBold(HTML html, char *text)                       */
/*                                                                           */
/*  Make a table entry containing some bold text.                            */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryTextBold(HTML html, char *text)
{
  HTMLTableEntryBegin(html);
  HTMLTextBold(html, text);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryTextItalic(HTML html, char *text)                     */
/*                                                                           */
/*  Make a table entry containing some italic text.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryTextItalic(HTML html, char *text)
{
  HTMLTableEntryBegin(html);
  HTMLTextItalic(html, text);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanText(HTML html, int columns, char *bgcolor,       */
/*    char *text)                                                            */
/*                                                                           */
/*  Make a spanning, coloured table entry containing some text.              */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanText(HTML html, int columns, char *bgcolor, char *text)
{
  HTMLTableEntrySpanBegin(html, columns, bgcolor);
  HTMLText(html, text);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSpacer(HTML html, int height)                                   */
/*                                                                           */
/*  Add a Netscape spacer item to keep open a table cell, as suggested       */
/*  by Jennifer Niederst, "Web Design in a Nutshell", 2nd edition, O'Reilly. */
/*  As explained there, it is a good solution even though spacer is          */
/*  specific to Netscape.                                                    */
/*                                                                           */
/*****************************************************************************/

void HTMLSpacer(HTML html, int height)
{
  fprintf(html->fp, "<spacer type=block width=1 height=%d>", height);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryTextBox(HTML html, char *name, char *initial_value)   */
/*                                                                           */
/*  Make a table entry containing a text box.                                */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryTextBox(HTML html, char *name, char *initial_value)
{
  HTMLTableEntryBegin(html);
  HTMLTextBox(html, name, initial_value);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntryTextBoxShort(HTML html, char *name,                   */
/*    char *initial_value)                                                   */
/*                                                                           */
/*  Make a table entry containing a short text box.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntryTextBoxShort(HTML html, char *name, char *initial_value)
{
  HTMLTableEntryBegin(html);
  HTMLTextBoxShort(html, name, initial_value);
  HTMLTableEntryEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanBegin(HTML html, int columns, char *bgcolor)      */
/*                                                                           */
/*  Begin a table entry that spans columns columns and has the given         */
/*  background colour.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanBegin(HTML html, int columns, char *bgcolor)
{
  char buff[10];
  if( DEBUG1 )
    fprintf(stderr, "[ HTMLTableEntrySpanBegin(html, %d, %s)\n",
      columns, bgcolor == NULL ? "(null)" : bgcolor);
  snprintf(buff, 10, "%d", columns);
  if( bgcolor == NULL )
    Tag1(html, "td", "colspan", buff);
  else
    Tag2(html, "td", "colspan", buff, "bgcolor", bgcolor);
  if( DEBUG1 )
    fprintf(stderr, "] HTMLTableEntrySpanBegin\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanCentredBegin(HTML html, int columns,              */
/*    char *bgcolor)                                                         */
/*                                                                           */
/*  Begin a centred table entry that spans columns columns and has the given */
/*  background colour.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanCentredBegin(HTML html, int columns, char *bgcolor)
{
  char buff[10];
  snprintf(buff, 10, "%d", columns);
  if( bgcolor == NULL )
    Tag2(html, "td", "align", "center", "colspan", buff);
  else
    Tag3(html, "td", "align", "center", "colspan", buff, "bgcolor", bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanFractionalWidthBegin(HTML html, int columns,      */
/*    int percent, char *bgcolor)                                            */
/*                                                                           */
/*  Combines the effects of SpanBegin and FractionalWidth.                   */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanFractionalWidthBegin(HTML html, int columns,
  int percent, char *bgcolor)
{
  char buff1[12], buff2[12];
  assert(columns >= 1);
  snprintf(buff1, 12, "%d", columns);
  snprintf(buff2, 12, "%d%%", percent);
  if( bgcolor == NULL )
    Tag2(html, "td", "colspan", buff1, "width", buff2);
  else
    Tag3(html, "td", "colspan", buff1, "width", buff2, "bgcolor", bgcolor);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanSpanBegin(HTML html, int columns, int rows,       */
/*    char *bgcolor)                                                         */
/*                                                                           */
/*  Begin a table entry that spans columns columns and rows rows.            */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanSpanBegin(HTML html, int columns, int rows,
  char *bgcolor)
{
  char buff1[12], buff2[12];
  assert(columns >= 1);
  assert(rows >= 1);
  snprintf(buff1, 12, "%d", columns);
  snprintf(buff2, 12, "%d", rows);
  if( bgcolor == NULL )
  {
    if( columns == 1 )
    {
      if( rows == 1 )
	Tag0(html, "td");
      else
	Tag1(html, "td", "rowspan", buff2);
    }
    else
    {
      if( rows == 1 )
	Tag1(html, "td", "colspan", buff1);
      else
	Tag2(html, "td", "colspan", buff1, "rowspan", buff2);
    }
  }
  else
  {
    if( columns == 1 )
    {
      if( rows == 1 )
	Tag1(html, "td", "bgcolor", bgcolor);
      else
	Tag2(html, "td", "rowspan", buff2, "bgcolor", bgcolor);
    }
    else
    {
      if( rows == 1 )
	Tag2(html, "td", "colspan", buff1, "bgcolor", bgcolor);
      else
	Tag3(html, "td", "colspan", buff1, "rowspan", buff2,"bgcolor",bgcolor);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTableEntrySpanSpanFractionalWidthBegin(HTML html,               */
/*    int columns, int rows, int percent, char *bgcolor)                     */
/*                                                                           */
/*  Like HTMLTableEntrySpanFractionalWidthBegin but spanning rows as         */
/*  well as columns.  As a print optimization we omit span values of 1.      */
/*                                                                           */
/*****************************************************************************/

void HTMLTableEntrySpanSpanFractionalWidthBegin(HTML html,
  int columns, int rows, int percent, char *bgcolor)
{
  char buff1[15], buff2[15], buff3[15];
  assert(columns >= 1);
  assert(rows >= 1);
  snprintf(buff1, 15, "%d", columns);
  snprintf(buff2, 15, "%d", rows);
  snprintf(buff3, 15, "%d%%", percent);
  if( columns == 1 )
  {
    if( rows == 1 )
      Tag2(html, "td", "width", buff3, "bgcolor", bgcolor);
    else
      Tag3(html, "td", "rowspan", buff2, "width", buff3, "bgcolor", bgcolor);
  }
  else
  {
    if( rows == 1 )
      Tag3(html, "td", "colspan", buff1, "width", buff3, "bgcolor", bgcolor);
    else
      Tag4(html, "td", "colspan", buff1, "rowspan", buff2, "width", buff3,
	"bgcolor", bgcolor);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Boxes"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLBoxBegin(HTML html, int outline_width)                          */
/*  void HTMLBoxBeginFullWidth(HTML html, int outline_width)                 */
/*  void HTMLBoxEnd(HTML html)                                               */
/*                                                                           */
/*  Begin/end a box with the given outline width.                            */
/*                                                                           */
/*****************************************************************************/

void HTMLBoxBegin(HTML html, int outline_width)
{
  HTMLTableBeginAttributed(html, 0, 0, outline_width, NULL);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
}

void HTMLBoxBeginFullWidth(HTML html, int outline_width)
{
  HTMLTableBeginAttributedFullWidth(html, 5, 0, outline_width, NULL);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
}

void HTMLBoxEnd(HTML html)
{
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
  HTMLTableEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLOutlinedBoxBegin(HTML html, char *bgcolor)                      */
/*  void HTMLOutlinedBoxEnd(HTML html)                                       */
/*                                                                           */
/*  Begin/end a coloured box with the given background colour.               */
/*                                                                           */
/*****************************************************************************/

void HTMLColouredBoxBegin(HTML html, char *bgcolor)
{
  HTMLTableBeginAttributed(html, 0, 2, 0, bgcolor);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
}

void HTMLColouredBoxBeginFullWidth(HTML html, char *bgcolor)
{
  HTMLTableBeginAttributedFullWidth(html, 0, 2, 0, bgcolor);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
}

void HTMLColouredOutlineBoxBeginFullWidth(HTML html,
  int outline_width, char *bgcolor)
{
  HTMLTableBeginAttributedFullWidth(html, 0, 0, outline_width, bgcolor);
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
}

void HTMLColouredBoxEnd(HTML html)
{
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
  HTMLTableEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Pages, page segments, and jumps".                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  HTML HTMLPageBegin(char *front_page, char *front_page_text, FILE *fp)    */
/*                                                                           */
/*  Begin an HTML page.  front_page is the address of the binary,            */
/*  e.g. "./thing.cgi", and front_page_text is a label for that page.        */
/*                                                                           */
/*****************************************************************************/

HTML HTMLPageBegin(char *front_page, char *front_page_text, FILE *fp)
{
  HTML html = HTMLMake(front_page, front_page_text, fp);
  fprintf(html->fp, "Content-Type: text/html\n\n");
  Tag0(html, "html");
  return html;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLPageEnd()                                                       */
/*                                                                           */
/*  End an HTML page.                                                        */
/*                                                                           */
/*****************************************************************************/

void HTMLPageEnd(HTML html)
{
  TagEnd(html, "html");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHeadBegin(HTML html)                                            */
/*                                                                           */
/*  Begin the head of an HTML document.                                      */
/*                                                                           */
/*****************************************************************************/

void HTMLHeadBegin(HTML html)
{
  Tag0(html, "head");
  Tag2(html, "meta", "http-equiv", "content-type", "content",
    "text/html; charset=utf-8");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHeadEnd(HTML html)                                              */
/*                                                                           */
/*  End the head of an HTML document.                                        */
/*                                                                           */
/*****************************************************************************/

void HTMLHeadEnd(HTML html)
{
  TagEnd(html, "head");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTitleBegin(HTML html)                                           */
/*                                                                           */
/*  Begin the title of an HTML document.                                     */
/*                                                                           */
/*****************************************************************************/

void HTMLTitleBegin(HTML html)
{
  Tag0(html, "title");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTitleEnd(HTML html)                                             */
/*                                                                           */
/*  End the title of an HTML document.                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLTitleEnd(HTML html)
{
  TagEnd(html, "title");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBodyBegin(HTML html)                                            */
/*                                                                           */
/*  Begin the body of an HTML document.                                      */
/*                                                                           */
/*****************************************************************************/

void HTMLBodyBegin(HTML html)
{
  Tag3(html, "body", "bgcolor", "bisque", "link", "darkblue",
    "vlink", "darkgreen");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLBodyEnd(HTML html)                                              */
/*                                                                           */
/*  End the body of the document.                                            */
/*                                                                           */
/*****************************************************************************/

void HTMLBodyEnd(HTML html)
{
  TagEnd(html, "body");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLJump1(HTML html, char *text, char *key1, char *val1)            */
/*                                                                           */
/*  void HTMLJump2(HTML html, char *text, char *key1, char *val1,            */
/*    char *key2, char *val2)                                                */
/*                                                                           */
/*  Jump to a point outside this document with the given attributes.         */
/*                                                                           */
/*****************************************************************************/

static void jump_start(HTML html, char *key1, char *val1)
{
  fprintf(html->fp, "<a href=\"%s?%s=%s", html->front_page,
    URLEncode(key1), URLEncode(val1));
}

static void jump_end(HTML html, char *part, char *text)
{
  if( part != NULL )
    fprintf(html->fp, "#%s\">%s</a>", part, text);
  else
    fprintf(html->fp, "\">%s</a>", text);
}

/* *** unused
static void jump_end_with_target(HTML html, char *text, char *window_name)
{
  fprintf(html->fp, "\" target=\"%s\">%s</a>", window_name, text);
}
*** */

static void jump_pair(HTML html, char *key, char *value)
{
  fprintf(html->fp, "&%s=%s", URLEncode(key), URLEncode(value));
}

void HTMLJumpFront(HTML html)
{
  fprintf(html->fp, "<a href=\"%s\">%s</a>", html->front_page,
    html->front_page_text);
}

void HTMLJump1(HTML html, char *key1, char *val1, char *part, char *text)
{
  jump_start(html, key1, val1);
  jump_end(html, part, text);
}

void HTMLJump2(HTML html, char *key1, char *val1, char *key2, char *val2,
  char *part, char *text)
{
  jump_start(html, key1, val1);
  jump_pair(html, key2, val2);
  jump_end(html, part, text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLJumpToDocument(HTML html, char *url, char *part, char *text)    */
/*                                                                           */
/*  Jump to an external URL, optionally part-way through.                    */
/*                                                                           */
/*****************************************************************************/

void HTMLJumpToDocument(HTML html, char *url, char *part, char *text)
{
  if( part != NULL )
    fprintf(html->fp, "<a href=\"%s#%s\">%s</a>", url, part, text);
  else
    fprintf(html->fp, "<a href=\"%s\">%s</a>", url, text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLJumpInternal(HTML html, char *name, char *text)                 */
/*                                                                           */
/*  Jump to a point inside the current document somewhere.                   */
/*                                                                           */
/*****************************************************************************/

void HTMLJumpInternal(HTML html, char *name, char *text)
{
  fprintf(html->fp, "<a href=\"#%s\">%s</a>", name, text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLJumpInternalIndented(HTML html, int indent, char *name,         */
/*    char *text)                                                            */
/*                                                                           */
/*  Like HTMLJumpInternal except that the jump appears on a line by          */
/*  itself with the given indent.                                            */
/*                                                                           */
/*****************************************************************************/

void HTMLJumpInternalIndented(HTML html, int indent, char *name, char *text)
{
  HTMLHSpace(html, indent);
  HTMLJumpInternal(html, name, text);
  HTMLNewLine(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLJumpTarget(HTML html, char *name)                               */
/*                                                                           */
/*  Mark this spot as the target of an internal jump.                        */
/*                                                                           */
/*****************************************************************************/

static void HTMLJumpTarget(HTML html, char *name)
{
  fprintf(html->fp, "<a name=\"%s\"></a>\n", name);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSegmentBegin(HTML html, char *name, char *title)                */
/*                                                                           */
/*  Start off a page segment with this name and title.                       */
/*                                                                           */
/*****************************************************************************/

void HTMLSegmentBegin(HTML html, char *name, char *title)
{
  HTMLParagraphBegin(html);
  HTMLParagraphEnd(html);
  HTMLJumpTarget(html, name);
  HTMLHeading(html, title);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSegmentEnd(HTML html)                                           */
/*                                                                           */
/*  End a segment by leaving some space and a horizontal rule.               */
/*                                                                           */
/*****************************************************************************/

void HTMLSegmentEnd(HTML html)
{
  HTMLParagraphBegin(html);
  HTMLParagraphEnd(html);
  HTMLHorizontalRule(html);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Forms".                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HTMLFormBegin(HTML html)                                            */
/*                                                                           */
/*  Begin a form.                                                            */
/*                                                                           */
/*****************************************************************************/

void HTMLFormBegin(HTML html)
{
  Tag1(html, "form", "method", "post");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLFormBeginUpload(HTML html, bool upload)                         */
/*                                                                           */
/*  Begin a form, with the option of setting the enctype attribute           */
/*  appropriately for uploading a file.                                      */
/*                                                                           */
/*****************************************************************************/

void HTMLFormBeginUpload(HTML html, bool upload)
{
  if( upload )
    Tag2(html, "form", "method", "post", "enctype", "multipart/form-data");
  else
    Tag1(html, "form", "method", "post");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLFormEnd(HTML html)                                              */
/*                                                                           */
/*  End a form.                                                              */
/*                                                                           */
/*****************************************************************************/

void HTMLFormEnd(HTML html)
{
  TagEnd(html, "form");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLHidden(HTML html, char *name, char *value)                      */
/*                                                                           */
/*  Hidden attribute with the given name and value.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLHidden(HTML html, char *name, char *value)
{
  Tag3(html, "input", "type", "hidden", "name", name, "value", value);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSubmitButton(HTML html, char *text)                             */
/*                                                                           */
/*  Add a submit button with the given text.                                 */
/*                                                                           */
/*****************************************************************************/

void HTMLSubmitButton(HTML html, char *text)
{
  Tag2(html, "input", "type", "submit", "value", text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBox(HTML html, char *name, char *initial_value)             */
/*                                                                           */
/*  Text box with the given name and initial value.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBox(HTML html, char *name, char *initial_value)
{
  Tag4(html, "input", "type", "text", "name", name,
    "value", initial_value, "size", "70");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBoxMedium(HTML html, char *name, char *initial_value)       */
/*                                                                           */
/*  Text box with the given name and initial value.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBoxMedium(HTML html, char *name, char *initial_value)
{
  Tag4(html, "input", "type", "text", "name", name,
    "value", initial_value, "size", "25");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextBoxShort(HTML html, char *name, char *initial_value)        */
/*                                                                           */
/*  Text box with the given name and initial value.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLTextBoxShort(HTML html, char *name, char *initial_value)
{
  Tag4(html, "input", "type", "text", "name", name,
    "value", initial_value, "size", "15");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLPasswordBox(HTML html, char *name)                              */
/*                                                                           */
/*  Text box with the given name and initial value.                          */
/*                                                                           */
/*****************************************************************************/

void HTMLPasswordBox(HTML html, char *name)
{
  Tag4(html, "input", "type", "password", "name", name,
    "value", "", "size", "70");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextAreaBegin(HTML html, char *name, int rows)                  */
/*                                                                           */
/*  Begin a text area with the given name.                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLTextAreaBegin(HTML html, char *name, int rows)
{
  char buff[10];
  snprintf(buff, 10, "%d", rows);
  Tag3(html, "textarea", "name", name, "cols", "70", "rows", buff);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextArea2Begin(HTML html, char *name, int rows, int cols)       */
/*                                                                           */
/*  Begin a text area with the given name.                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLTextArea2Begin(HTML html, char *name, int rows, int cols)
{
  char buff1[10];
  char buff2[10];
  snprintf(buff1, 10, "%d", rows);
  snprintf(buff2, 10, "%d", cols);
  Tag3(html, "textarea", "name", name, "rows", buff1, "cols", buff2);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLTextAreaEnd(HTML html)                                          */
/*                                                                           */
/*  End a text area.                                                         */
/*                                                                           */
/*****************************************************************************/

void HTMLTextAreaEnd(HTML html)
{
  TagEnd(html, "textarea");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxesBegin(HTML html, char *name, char *checked)           */
/*                                                                           */
/*  Begin a set of radio boxes.  The one equal to checked will be            */
/*  checked initially.                                                       */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBoxesBegin(HTML html, char *name, char *checked)
{
  assert(html->radio_name == NULL);
  html->radio_name = name;
  html->radio_checked = checked;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBox(HTML html, char *value)                                */
/*                                                                           */
/*  Print one radio box with the given name and value.  Compare its          */
/*  value with html->radio_checked, and mark it checked if equal.            */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBox(HTML html, char *value, char *text)
{
  assert(html->radio_name != NULL);
  Tag4(html, "input", "type", "radio",
    "name", html->radio_name, "value", value,
    html->radio_checked != NULL && strcmp(value, html->radio_checked) == 0 ?
    "checked" : NULL, NULL);
  if( text != NULL )
    HTMLText(html, text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxesEnd(HTML html)                                        */
/*                                                                           */
/*  End a sequence of radio boxes.                                           */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBoxesEnd(HTML html)
{
  assert(html->radio_name != NULL);
  html->radio_name = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxesTabularBegin(HTML html, char *name, char *checked)    */
/*                                                                           */
/*  Begin a set of radio boxes, using a tabular style with one alternative   */
/*  per row, the first column containing the box, and the second the text.   */
/*  The one whose name is equal to checked will be checked initially.        */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBoxesTabularBegin(HTML html, char *name, char *checked)
{
  assert(html->radio_name == NULL);
  html->radio_name = name;
  html->radio_checked = checked;
  /* HTMLParagraphBegin(html); */
  HTMLTableBeginAttributed(html, 0, 10, 0, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxTabular(HTML html, char *value)                         */
/*                                                                           */
/*  Print one tabular radio box with the given name and value.  Compare its  */
/*  value with html->radio_checked, and mark it checked if equal.            */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBoxTabular(HTML html, char *value, char *text)
{
  assert(html->radio_name != NULL);
  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  Tag4(html, "input", "type", "radio", "name", html->radio_name, "value", value,
    html->radio_checked != NULL && strcmp(value, html->radio_checked) == 0 ?
    "checked" : NULL, NULL);
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, text);
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxTabularWithTextBox(HTML html, char *value, char *text,  */
/*    char *text_box_name, char *text_box_initial_value)                     */
/*                                                                           */
/*  Like HTMLRadioBoxTabular only endiing with a text box with the given     */
/*  name and initial value.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
void HTMLRadioBoxTabularWithTextBox(HTML html, char *value, char *text,
  char *text_box_name, char *text_box_initial_value)
{
  assert(html->radio_name != NULL);
  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  Tag4(html, "input", "type", "radio", "name", html->radio_name, "value", value,
    html->radio_checked != NULL && strcmp(value, html->radio_checked) == 0 ?
    "checked" : NULL, NULL);
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, text);
  HTMLTextBoxMedium(html, text_box_name, text_box_initial_value);
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void HTMLRadioBoxesTabularEnd(HTML html)                                 */
/*                                                                           */
/*  End a sequence of tabular radio boxes.                                   */
/*                                                                           */
/*****************************************************************************/

void HTMLRadioBoxesTabularEnd(HTML html)
{
  assert(html->radio_name != NULL);
  html->radio_name = NULL;
  HTMLTableEnd(html);
  /* HTMLParagraphEnd(html); */
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLCheckBoxesBegin(HTML html, char *names_prefix)                  */
/*                                                                           */
/*  Begin a set of checkboxes.                                               */
/*                                                                           */
/*****************************************************************************/

void HTMLCheckBoxesBegin(HTML html, char *names_prefix)
{
  assert(html->checkbox_prefix == NULL);
  html->checkbox_prefix = names_prefix;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLCheckBox(HTML html, char *value, char *text, bool checked)      */
/*                                                                           */
/*  Add a check box.                                                         */
/*                                                                           */
/*****************************************************************************/

void HTMLCheckBox(HTML html, char *value, char *text, bool checked)
{
  char buff[200];
  if( html->checkbox_prefix == NULL )
    snprintf(buff, 200, "%s", value);
  else
    snprintf(buff, 200, "%s_%s", html->checkbox_prefix, value);
  Tag4(html, "input", "type", "checkbox", "name", buff, "value", "Yes",
    checked ? "checked" : NULL, NULL);
  if( text != NULL )
    HTMLText(html, text);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLCheckBoxesEnd(HTML html)                                        */
/*                                                                           */
/*  End a set of checkboxes.                                                 */
/*                                                                           */
/*****************************************************************************/

void HTMLCheckBoxesEnd(HTML html)
{
  html->checkbox_prefix = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSelectBegin(HTML html, char *name, char *selected_value)        */
/*                                                                           */
/*  Begin an HTML selection.  If selected_value is not NULL, the element     */
/*  with that value will be selected initially.                              */
/*                                                                           */
/*****************************************************************************/

void HTMLSelectBegin(HTML html, char *name, char *selected_value)
{
  assert(!html->selecting);
  html->selecting = true;
  html->selected = selected_value;
  Tag1(html, "select", "name", name);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSelect(HTML html, char *value)                                  */
/*                                                                           */
/*  Add this value to the current select, possibly pre-selected.             */
/*                                                                           */
/*****************************************************************************/

void HTMLSelect(HTML html, char *value)
{
  if( html->selected != NULL && strcmp(value, html->selected) == 0 )
    Tag2(html, "option", "value", value, "selected", NULL);
  else
    Tag1(html, "option", "value", value);
  HTMLText(html, value);
  TagEnd(html, "option");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSelectIndented(HTML html, char *value, int indent_level)        */
/*                                                                           */
/*  Like HTMLSelect but allows the option of the menu item to be indented.   */
/*                                                                           */
/*****************************************************************************/

void HTMLSelectIndented(HTML html, char *value, int indent_level)
{
  if( html->selected != NULL && strcmp(value, html->selected) == 0 )
    Tag2(html, "option", "value", value, "selected", NULL);
  else
    Tag1(html, "option", "value", value);
  HTMLHSpace(html, indent_level * 4);
  HTMLText(html, value);
  TagEnd(html, "option");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSelectEnd(HTML html)                                            */
/*                                                                           */
/*  End an HTML select.                                                      */
/*                                                                           */
/*****************************************************************************/

void HTMLSelectEnd(HTML html)
{
  assert(html->selecting);
  html->selecting = false;
  TagEnd(html, "select");
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLSelectYesOrNo(HTML html, char *name, char *selected_value)      */
/*                                                                           */
/*  A complete selection, offering choices "Yes" and "No"                    */
/*                                                                           */
/*****************************************************************************/

void HTMLSelectYesOrNo(HTML html, char *name, char *selected_value)
{
  HTMLSelectBegin(html, name, selected_value);
  HTMLSelect(html, "Yes");
  HTMLSelect(html, "No");
  HTMLSelectEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void HTMLFileUploadBox(HTML html, char *name)                            */
/*                                                                           */
/*  File upload box with the given name.                                     */
/*                                                                           */
/*****************************************************************************/

void HTMLFileUploadBox(HTML html, char *name)
{
  Tag3(html, "input", "type", "file", "name", name, "size", "70");
}

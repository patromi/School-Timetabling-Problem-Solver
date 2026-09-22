
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
/*  FILE:         html.h                                                     */
/*  MODULE:       HTML generation                                            */
/*                                                                           */
/*****************************************************************************/
#ifndef HTML_HEADER_FILE
#define HTML_HEADER_FILE
#include <stdbool.h>
#include <stdio.h>

typedef struct html_rec *HTML;

/* colours */
#define BrightGreen	"#7CFC00"
#define LightGreen	"#99FF99"
#define LightYellow	"#FFFF66"
#define Red		"#FF0000"
#define LightRed	"#FF9999"
#define LightPink	"#FFEEEE"
#define LightBlue	"#99FFFF"
#define Blue		"#0080FF"
#define White		"#FFFFFF"
#define VeryLightGrey	"#EEEEEE"
#define LightGrey	"#CCCCCC"
#define Grey		"#999999"
#define Black		"#000000"

/* fonts */
typedef enum {
  HTML_ROMAN,
  HTML_ITALIC,
  HTML_BOLD,
  HTML_BOLD_ITALIC,
  HTML_MONO
} HTML_FONT;

/* basic text elements */
extern void HTMLLiteralText(HTML html, char *text);
extern void HTMLLiteralTextIndented(HTML html, int indent, char *text);
extern void HTMLText(HTML html, char *fmt, ...);
extern void HTMLTextNoBreak(HTML html, char *fmt, ...);
extern void HTMLTextItalic(HTML html, char *fmt, ...);
extern void HTMLTextItalicNoBreak(HTML html, char *fmt, ...);
extern void HTMLTextBold(HTML html, char *fmt, ...);
extern void HTMLTextBoldNoBreak(HTML html, char *fmt, ...);
extern void HTMLTextBoldItalic(HTML html, char *fmt, ...);
extern void HTMLTextBoldItalicNoBreak(HTML html, char *fmt, ...);
extern void HTMLTextMono(HTML html, char *fmt, ...);
extern void HTMLTextMonoNoBreak(HTML html, char *fmt, ...);

extern void HTMLFontBegin(HTML html, HTML_FONT font);
extern void HTMLFontEnd(HTML html, HTML_FONT font);

/* *** old versions without varargs
extern void HTMLText(HTML html, char *text);
extern void HTMLLiteralText(HTML html, char *text);
extern void HTMLTextNoBreak(HTML html, char *text);
extern void HTMLTextWithInt(HTML html, char *fmt, int i);
extern void HTMLTextWithIntNoBreak(HTML html, char *fmt, int i);
extern void HTMLTextWithDouble(HTML html, char *fmt, double x);
extern void HTMLTextItalicNoBreak(HTML html, char *text);
extern void HTMLTextBoldItalic(HTML html, char *text);
extern void HTMLTextBold(HTML html, char *text);
extern void HTMLTextBoldWithInt(HTML html, char *fmt, int i);
extern void HTMLTextItalic(HTML html, char *text);
extern void HTMLTextMono(HTML html, char *text);
extern void HTMLTextMonoNoBreak(HTML html, char *text);
extern void HTMLTextIndented(HTML html, int indent, char *text);
*** */

extern void HTMLSmallBegin(HTML html);
extern void HTMLSmallEnd(HTML html);
extern void HTMLBoldBegin(HTML html);
extern void HTMLBoldEnd(HTML html);
extern void HTMLItalicBegin(HTML html);
extern void HTMLItalicEnd(HTML html);
extern void HTMLMonoBegin(HTML html);
extern void HTMLMonoEnd(HTML html);
extern void HTMLHSpace(HTML html, int spaces);
extern void HTMLVSpace(HTML html, int spaces);
extern void HTMLNewLine(HTML html);
extern void HTMLParagraphBegin(HTML html);
extern void HTMLParagraphEnd(HTML html);
extern void HTMLBigHeading(HTML html, char *heading);
extern void HTMLHeading(HTML html, char *heading);
extern void HTMLHeadingBegin(HTML html);
extern void HTMLHeadingEnd(HTML html);
extern void HTMLHorizontalRule(HTML html);
extern void HTMLIndentBegin(HTML html);
extern void HTMLIndentEnd(HTML html);

/* displays */
extern void HTMLCentredDisplayBegin(HTML html);
extern void HTMLCentredDisplayEnd(HTML html);

/* lists */
extern void HTMLNumberedListBegin(HTML html);
extern void HTMLNumberedListEnd(HTML html);
extern void HTMLBulletListBegin(HTML html);
extern void HTMLBulletListEnd(HTML html);
extern void HTMLListItemBegin(HTML html);
extern void HTMLListItemEnd(HTML html);

/* tables */
extern void HTMLSetTableOptions(HTML html, int spacing, int padding,
  int border, char *bgcolor);
extern void HTMLTableBeginAttributed(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor);
extern void HTMLTableBeginAttributedFractionalWidth(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor, int percent);
extern void HTMLTableBeginAttributedFullWidth(HTML html, int cellpadding,
  int cellspacing, int border, char *bgcolor);
extern void HTMLTableBegin(HTML html, char *bgcolor);
extern void HTMLTableBeginFullWidth(HTML html, char *bgcolor);
extern void HTMLTableBeginFractionalWidth(HTML html, char *bgcolor,int percent);
extern void HTMLTableEnd(HTML html);
extern void HTMLTableRowBegin(HTML html);
extern void HTMLTableRowVAlignBegin(HTML html, char *valign);
extern void HTMLTableRowColouredVAlignBegin(HTML html, char *bgcolor,
  char *valign);
extern void HTMLTableRowEnd(HTML html);
extern void HTMLTableEntryBegin(HTML html);
extern void HTMLTableEntryCentredBegin(HTML html);
extern void HTMLTableEntryRightBegin(HTML html);
extern void HTMLTableEntryBeginFractionalWidth(HTML html, int percentage,
  char *bgcolor);
extern void HTMLTableEntryBeginCentredFractionalWidth(HTML html, int percentage,
  char *bgcolor);
extern void HTMLTableEntryColouredBegin(HTML html, char *bgcolor);
extern void HTMLTableEntryColouredCentredBegin(HTML html, char *bgcolor);
extern void HTMLTableEntrySpanBegin(HTML html, int columns, char *bgcolor);
extern void HTMLTableEntrySpanCentredBegin(HTML html, int columns,
  char *bgcolor);
extern void HTMLTableEntrySpanFractionalWidthBegin(HTML html, int columns,
  int percent, char *bgcolor);
extern void HTMLTableEntrySpanSpanBegin(HTML html,
  int columns, int rows, char *bgcolor);
extern void HTMLTableEntrySpanSpanFractionalWidthBegin(HTML html,
  int columns, int rows, int percent, char *bgcolor);
extern void HTMLTableEntryEnd(HTML html);
extern void HTMLTableEntryText(HTML html, char *text);
extern void HTMLTableEntryTextBold(HTML html, char *text);
extern void HTMLTableEntryTextItalic(HTML html, char *text);
extern void HTMLTableEntryTextBox(HTML html, char *name, char *initial_value);
extern void HTMLTableEntryTextBoxShort(HTML html, char *name,
  char *initial_value);
extern void HTMLTableEntrySpanText(HTML html, int columns,
  char *bgcolor, char *text);
extern void HTMLSpacer(HTML html, int height);

/* boxes */
extern void HTMLBoxBegin(HTML html, int outline_width);
extern void HTMLBoxBeginFullWidth(HTML html, int outline_width);
extern void HTMLBoxEnd(HTML html);
extern void HTMLColouredBoxBegin(HTML html, char *bgcolor);
extern void HTMLColouredBoxBeginFullWidth(HTML html, char *bgcolor);
extern void HTMLColouredOutlineBoxBeginFullWidth(HTML html,
  int outline_width, char *bgcolor);
extern void HTMLColouredBoxEnd(HTML html);

/* pages, page segments, and jumps */
extern HTML HTMLPageBegin(char *front_page, char *front_page_text, FILE *fp);
extern void HTMLPageEnd(HTML html);
extern void HTMLHeadBegin(HTML html);
extern void HTMLHeadEnd(HTML html);
extern void HTMLTitleBegin(HTML html);
extern void HTMLTitleEnd(HTML html);
extern void HTMLBodyBegin(HTML html);
extern void HTMLBodyEnd(HTML html);
extern void HTMLJumpFront(HTML html);
extern void HTMLJump1(HTML html, char *key1, char *val1,
  char *part, char *text);
extern void HTMLJump2(HTML html, char *key1, char *val1,
  char *key2, char *val2, char *part, char *text);
extern void HTMLJumpToDocument(HTML html, char *url, char *part, char *text);

extern void HTMLJumpInternal(HTML html, char *name, char *text);
extern void HTMLJumpInternalIndented(HTML html, int indent, char *name,
  char *text);
extern void HTMLSegmentBegin(HTML html, char *name, char *title);
extern void HTMLSegmentEnd(HTML html);

/* forms */
extern void HTMLFormBegin(HTML html);
extern void HTMLFormBeginUpload(HTML html, bool upload);
extern void HTMLFormEnd(HTML html);
extern void HTMLHidden(HTML html, char *name, char *value);
extern void HTMLSubmitButton(HTML html, char *text);
extern void HTMLTextBox(HTML html, char *name, char *initial_value);
extern void HTMLTextBoxMedium(HTML html, char *name, char *initial_value);
extern void HTMLTextBoxShort(HTML html, char *name, char *initial_value);
extern void HTMLPasswordBox(HTML html, char *name);
extern void HTMLTextAreaBegin(HTML html, char *name, int rows);
extern void HTMLTextArea2Begin(HTML html, char *name, int rows, int cols);
extern void HTMLTextAreaEnd(HTML html);
extern void HTMLRadioBoxesBegin(HTML html, char *name, char *checked);
extern void HTMLRadioBox(HTML html, char *value, char *text);
extern void HTMLRadioBoxesEnd(HTML html);
extern void HTMLRadioBoxesTabularBegin(HTML html, char *name, char *checked);
extern void HTMLRadioBoxTabular(HTML html, char *value, char *text);
/* ***
extern void HTMLRadioBoxTabularWithTextBox(HTML html, char *value, char *text,
  char *text_box_name, char *text_box_initial_value);
*** */
extern void HTMLRadioBoxesTabularEnd(HTML html);
extern void HTMLCheckBoxesBegin(HTML html, char *names_prefix);
extern void HTMLCheckBox(HTML html, char *value, char *text, bool checked);
extern void HTMLCheckBoxesEnd(HTML html);
extern void HTMLSelectBegin(HTML html, char *name, char *selected_value);
extern void HTMLSelect(HTML html, char *value);
extern void HTMLSelectIndented(HTML html, char *value, int indent_level);
extern void HTMLSelectEnd(HTML html);
extern void HTMLSelectYesOrNo(HTML html, char *name, char *selected_value);
extern void HTMLFileUploadBox(HTML html, char *name);

#endif

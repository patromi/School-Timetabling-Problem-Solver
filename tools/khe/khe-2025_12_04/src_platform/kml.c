
/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2008 Jeffrey H. Kingston                             */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  FILE:         kml.c                                                      */
/*  MODULE:       XML reading and writing                                    */
/*                                                                           */
/*****************************************************************************/
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "kml.h"
#include "howard_a.h"
#include "howard_n.h"

#define KML_MAX_STR 200
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KML_FILE - an XML file open for writing (not reading)                    */
/*                                                                           */
/*****************************************************************************/

struct kml_file_rec {
  FILE			*fp;			/* file to write XML to      */
  int			curr_indent;		/* current indent            */
  int			indent_step;		/* indent step               */
  bool			attribute_allowed;	/* state of print            */
};


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT - an XML element                                                 */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KML_ELT) ARRAY_KML_ELT;

struct kml_elt_rec {
  int			line_num;		/* line number of element    */
  int			col_num;		/* column number of element  */
  char			*label;			/* label of element          */
  KML_ELT		parent;			/* parent of element         */
  HA_ARRAY_NSTRING	attribute_names;	/* attribute names           */
  HA_ARRAY_NSTRING	attribute_values;	/* attribute values          */
  ARRAY_KML_ELT		children;		/* children                  */
  HA_ARRAY_NCHAR	text;			/* text                      */
};


/*****************************************************************************/
/*                                                                           */
/*  KML_ERROR                                                                */
/*                                                                           */
/*****************************************************************************/

struct kml_error_rec {
  int			line_num;		/* line number of error      */
  int			col_num;		/* column number of error    */
  char			string[KML_MAX_STR];	/* error string              */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "string handling (private)"                                    */
/*                                                                           */
/*****************************************************************************/
#define is_space(ch) ((ch)=='\n' || (ch)=='\r' || (ch)=='\t' || (ch)==' ')
#define is_digit(ch) ((ch) >= '0' && ch <= '9')


/*****************************************************************************/
/*                                                                           */
/*  bool KmlStringHasNonWhite(const char *str, int len)                      */
/*                                                                           */
/*  Return true if str has non-white spaces in it.                           */
/*                                                                           */
/*****************************************************************************/

static bool KmlStringHasNonWhite(char *str)
{
  char *p;
  for( p = str;  *p != '\0';  p++ )
    if( !is_space(*p) )
      return true;
  return false;
}

/* obsolete version ***
static bool KmlStringHasNonWhite(const char *str, int len)
{
  int i;
  for( i = 0;  i < len;  i++ )
    if( !is_space(str[i]) )
      return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KmlStringContainsOneIntegerOnly(char *s)                            */
/*                                                                           */
/*  Return true if s contains exactly one non-negative integer, possibly     */
/*  with white space on either side.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KmlStringContainsOneIntegerOnly(char *s)
{
  char *p;
  if( s == NULL )
    return false;

  /* skip zero or more preceding spaces */
  for( p = s;  is_space(*p);  p++ );

  /* skip one or more digits */
  if( !is_digit(*p) )
    return false;
  do { p++; } while( is_digit(*p) );

  /* skip zero or more following spaces */
  while( is_space(*p) )  p++;

  /* that must be the end */
  return *p == '\0';
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlStringContainsOneFloatOnly(char *s)                              */
/*                                                                           */
/*  Return true if s contains exactly one non-negative float, possibly       */
/*  with white space on either side.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KmlStringContainsOneFloatOnly(char *s)
{
  char *p;
  if( s == NULL )
    return false;

  /* skip zero or more preceding spaces */
  for( p = s;  is_space(*p);  p++ );

  if( *p == '.' )
  {
    /* grammar for this case is .[0-9]+ */
    p++;
    if( !is_digit(*p) )
      return false;
    do { p++; } while( is_digit(*p) );
  }
  else
  {
    /* grammar for this case is [0-9]+[.[0-9]*] */
    if( !is_digit(*p) )
      return false;
    do { p++; } while( is_digit(*p) );
    if( *p == '.' )
    {
      p++;
      while( is_digit(*p) )
	p++;
    }
  }

  /* skip zero or more following spaces */
  while( is_space(*p) )  p++;

  /* that must be the end */
  return *p == '\0';
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "A.4.1 Representing XML in memory"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KmlArena(KML_ELT elt)                                           */
/*                                                                           */
/*  Return the arena holding elt.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
HA_ARENA KmlArena(KML_ELT elt)
{
  return HaArrayArena(elt->children);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KmlLineNum(KML_ELT elt)                                              */
/*                                                                           */
/*  Return the line number of elt.                                           */
/*                                                                           */
/*****************************************************************************/

int KmlLineNum(KML_ELT elt)
{
  return elt->line_num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KmlColNum(KML_ELT elt)                                               */
/*                                                                           */
/*  Return the column number of elt.                                         */
/*                                                                           */
/*****************************************************************************/

int KmlColNum(KML_ELT elt)
{
  return elt->col_num;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KmlLabel(KML_ELT elt)                                              */
/*                                                                           */
/*  Return the label of elt.                                                 */
/*                                                                           */
/*****************************************************************************/

char *KmlLabel(KML_ELT elt)
{
  return elt->label;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT KmlParent(KML_ELT elt)                                           */
/*                                                                           */
/*  Return the parent of elt.  This may be NULL.                             */
/*                                                                           */
/*****************************************************************************/

KML_ELT KmlParent(KML_ELT elt)
{
  return elt->parent;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KmlText(KML_ELT elt)                                               */
/*                                                                           */
/*  Return the text of elt.                                                  */
/*                                                                           */
/*****************************************************************************/

char *KmlText(KML_ELT elt)
{
  return HnStringEnd(elt->text);
}


/*****************************************************************************/
/*                                                                           */
/*  int KmlAttributeCount(KML_ELT elt)                                       */
/*                                                                           */
/*  Return the number of attributes of elt.                                  */
/*                                                                           */
/*****************************************************************************/

int KmlAttributeCount(KML_ELT elt)
{
  return HaArrayCount(elt->attribute_names);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KmlAttributeName(KML_ELT elt, int index)                           */
/*                                                                           */
/*  Return the name of the index'th attribute of elt, where                  */
/*  0 <= index <= KmlAttributeCount(elt).                                    */
/*                                                                           */
/*****************************************************************************/

char *KmlAttributeName(KML_ELT elt, int index)
{
  if( index < 0 )
    index = KmlAttributeCount(elt) + index;
  assert(index < HaArrayCount(elt->attribute_names));
  return HaArray(elt->attribute_names, index);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KmlAttributeValue(KML_ELT elt, int index)                          */
/*                                                                           */
/*  Return the value of the index'th attribute of elt, where                 */
/*  0 <= index <= KmlAttributeCount(elt).                                    */
/*                                                                           */
/*****************************************************************************/

char *KmlAttributeValue(KML_ELT elt, int index)
{
  if( index < 0 )
    index = KmlAttributeCount(elt) + index;
  assert(index < HaArrayCount(elt->attribute_values));
  return HaArray(elt->attribute_values, index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlContainsAttribute(KML_ELT elt, char *name, int *index)           */
/*                                                                           */
/*  If elt contains an attribute with the given name, return true and set    */
/*  *index to its index.  Otherwise return false and set *index to -1.       */
/*                                                                           */
/*****************************************************************************/

bool KmlContainsAttributePos(KML_ELT elt, char *name, int *index)
{
  int i;
  for( i = 0;  i < KmlAttributeCount(elt);  i++ )
    if( strcmp(KmlAttributeName(elt, i), name) == 0 )
    {
      *index = i;
      return true;
    }
  *index = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlContainsAttribute(KML_ELT elt, char *name, char **value)         */
/*                                                                           */
/*  If elt contains an attribute with the given name, return true and set    */
/*  *value to its value.  Otherwise return false and set *value to NULL.     */
/*                                                                           */
/*****************************************************************************/

bool KmlContainsAttribute(KML_ELT elt, char *name, char **value)
{
  int i;
  if( KmlContainsAttributePos(elt, name, &i) )
  {
    *value = KmlAttributeValue(elt, i);
    return true;
  }
  else
  {
    *value = NULL;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KmlChildCount(KML_ELT elt)                                           */
/*                                                                           */
/*  Return the number of children of elt.                                    */
/*                                                                           */
/*****************************************************************************/

int KmlChildCount(KML_ELT elt)
{
  return HaArrayCount(elt->children);
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT KmlChild(KML_ELT elt, int index)                                 */
/*                                                                           */
/*  Return the index'th child of elt, where 0 <= index < KmlChildCount(elt), */
/*  or if index is negative, from the back (-1 means last, etc.).            */
/*                                                                           */
/*****************************************************************************/

KML_ELT KmlChild(KML_ELT elt, int index)
{
  if( index < 0 )
    index = KmlChildCount(elt) + index;
  assert(index < HaArrayCount(elt->children));
  return HaArray(elt->children, index);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlContainsChildPos(KML_ELT elt, char *label, int *index)           */
/*                                                                           */
/*  If elt contains at least one child with this label, return true and      */
/*  set *index to the index of the first such child.  Otherwise return       */
/*  false and set *index to -1.                                              */
/*                                                                           */
/*****************************************************************************/

bool KmlContainsChildPos(KML_ELT elt, char *label, int *index)
{
  KML_ELT e;  int i;
  HaArrayForEach(elt->children, e, i)
    if( strcmp(e->label, label) == 0 )
    {
      *index = i;
      return true;
    }
  *index = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlContainsChild(KML_ELT elt, char *label, KML_ELT *child_elt)      */
/*                                                                           */
/*  If elt contains at least one child with this label, return true and      */
/*  set *child_elt to the first such child.  Otherwise return false and      */
/*  set *child_elt to NULL.                                                  */
/*                                                                           */
/*****************************************************************************/

bool KmlContainsChild(KML_ELT elt, char *label, KML_ELT *child_elt)
{
  int i;
  if( KmlContainsChildPos(elt, label, &i) )
  {
    *child_elt = KmlChild(elt, i);
    return true;
  }
  else
  {
    *child_elt = NULL;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT KmlMakeElt(int line_num, int col_num, char *label, HA_ARENA a)   */
/*                                                                           */
/*  Make a new XML element with these attributes.  Initially the node        */
/*  has no parent, but if it is added as a child by KmlAddChild it will      */
/*  get a parent then.                                                       */
/*                                                                           */
/*****************************************************************************/

KML_ELT KmlMakeElt(int line_num, int col_num, char *label, HA_ARENA a)
{
  KML_ELT res;
  HaMake(res, a);
  res->line_num = line_num;
  res->col_num = col_num;
  res->label = HnStringCopy(label, a);
  res->parent = NULL;
  HaArrayInit(res->attribute_names, a);
  HaArrayInit(res->attribute_values, a);
  HaArrayInit(res->children, a);
  HnStringBegin(res->text, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlAddAttribute(KML_ELT elt, char *name, char *value)               */
/*                                                                           */
/*  Add an attribute with this name and value to elt.                        */
/*                                                                           */
/*****************************************************************************/

void KmlAddAttribute(KML_ELT elt, char *name, char *value)
{
  HA_ARENA a;
  assert(name != NULL && strlen(name) > 0 && value != NULL);
  a = HaArrayArena(elt->attribute_names);
  HaArrayAddLast(elt->attribute_names, HnStringCopy(name, a));
  HaArrayAddLast(elt->attribute_values, HnStringCopy(value, a));
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlAddChild(KML_ELT elt, KML_ELT child)                             */
/*                                                                           */
/*  Add this child to elt.                                                   */
/*                                                                           */
/*****************************************************************************/

void KmlAddChild(KML_ELT elt, KML_ELT child)
{
  if( DEBUG2 )
  {
    int i;  KML_ELT c;
    fprintf(stderr, "[ KmlAddChild(%d:%d:<%s>, %d:%d:<%s>)\n",
      elt->line_num, elt->col_num, elt->label,
      child->line_num, child->col_num, child->label);
    fprintf(stderr, "  %d children:\n", HaArrayCount(elt->children));
    HaArrayForEach(elt->children, c, i)
      fprintf(stderr, "    %d: %s\n", i, c->label);
  }
  /* allowing text now, but will be an error later
  assert(elt->text == NULL);
  *** */
  HnAssert(child->parent == NULL,
    "KmlAddChild: attempt to give a child two parents");
  HaArrayAddLast(elt->children, child);
  child->parent = elt;
  if( DEBUG2 )
    fprintf(stderr, "] KmlAddChild returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlDeleteChild(KML_ELT elt, KML_ELT child)                          */
/*                                                                           */
/*  Delete child from elt.  It must be present.                              */
/*                                                                           */
/*****************************************************************************/

void KmlDeleteChild(KML_ELT elt, KML_ELT child)
{
  int pos;
  if( !HaArrayContains(elt->children, child, &pos) )
    HnAbort("KmlDeleteChild:  child not present");
  HaArrayDeleteAndShift(elt->children, pos);
  child->parent = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlAddTextLen(KML_ELT elt, const char *text, int len)               */
/*                                                                           */
/*  Add text, which is known to have the given length, to any text that      */
/*  is already present.  There may be no children.                           */
/*                                                                           */
/*  If the string to be added contains nothing but white space, then it      */
/*  is not added at all.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KmlAddTextLen(KML_ELT elt, const char *text, int len)
{
  int curr_len;
  if( KmlStringHasNonWhite(text, len) )
  {
    assert(HaArrayCount(elt->children) == 0);
    curr_len = (elt->text != NULL ? strlen(elt->text) : 0);
    elt->text = (char *) reall oc(elt->text, (curr_len+len+1) * sizeof(char));
    strncpy(&elt->text[curr_len], text, len);
    elt->text[curr_len + len] = '\0';
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KmlAddText(KML_ELT elt, char *text)                                 */
/*                                                                           */
/*  Add text to elt, appending it to any text already present.  There may    */
/*  not be any children beforehand.                                          */
/*                                                                           */
/*****************************************************************************/

void KmlAddText(KML_ELT elt, char *text)
{
  if( HaArrayCount(elt->text) > 0 && HaArrayLast(elt->text) == '\0' )
    HaArrayDeleteLast(elt->text);
  HnStringAdd(&elt->text, "%s", text);
  /* KmlAddTextLen(elt, text, strlen(text)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlAddFmtText(KML_ELT elt, char *fmt, ...)                          */
/*                                                                           */
/*  Add formatted text to elt.                                               */
/*                                                                           */
/*****************************************************************************/

void KmlAddFmtText(KML_ELT elt, char *fmt, ...)
{
  va_list ap;
  if( HaArrayCount(elt->text) > 0 && HaArrayLast(elt->text) == '\0' )
    HaArrayDeleteLast(elt->text);
  va_start(ap, fmt);
  HnStringVAdd(&elt->text, fmt, ap);
  va_end(ap);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlFree(KML_ELT elt)                                                */
/*                                                                           */
/*  Free the memory occuped by elt and its descendants, including all        */
/*  strings accessible from elt.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
void KmlFree(KML_ELT elt)
{
  KML_ELT child;  char *str;  int i;
  if( DEBUG3 )
    fprintf(stderr, "[ KmlFree(%p %s)\n", (void *) elt, elt->label);

  ** free the attribute names array and the strings it contains **
  HaArrayForEach(elt->attribute_names, str, i)
    MFree(str);
  MArrayFree(elt->attribute_names);

  ** free the attribute values array and the strings it contains **
  HaArrayForEach(elt->attribute_values, str, i)
    MFree(str);
  MArrayFree(elt->attribute_values);

  ** free the children **
  HaArrayForEach(elt->children, &child, &i)
  {
    HnAssert(child->parent == elt, "KmlFree internal error");
    KmlFree(child);
  }
  MArrayFree(elt->children);

  ** free the text and elt itself **
  MFree(elt->text);
  MFree(elt);
  if( DEBUG3 )
    fprintf(stderr, "] KmlFree\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "A.4.2 Error handing and format checking"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KmlErrorLineNum(KML_ERROR ke)                                        */
/*                                                                           */
/*  Return the line number attribute of error ke.                            */
/*                                                                           */
/*****************************************************************************/

int KmlErrorLineNum(KML_ERROR ke)
{
  return ke->line_num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KmlErrorColNum(KML_ERROR ke)                                         */
/*                                                                           */
/*  Return the column number attribute of error ke.                          */
/*                                                                           */
/*****************************************************************************/

int KmlErrorColNum(KML_ERROR ke)
{
  return ke->col_num;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KmlErrorString(KML_ERROR ke)                                       */
/*                                                                           */
/*  Return the string attribute of error ke.                                 */
/*                                                                           */
/*****************************************************************************/

char *KmlErrorString(KML_ERROR ke)
{
  return ke->string;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlErrorMake(KML_ERROR *ke, int line_num, int col_num,              */
/*    char *fmt, ...)                                                        */
/*                                                                           */
/*  Make a KML error object unless ke is NULL.  For convenience when using   */
/*  this function, *ke is set to the new object, and false is returned.      */
/*                                                                           */
/*****************************************************************************/

KML_ERROR KmlErrorMake(HA_ARENA a, int line_num, int col_num, char *fmt, ...)
{
  KML_ERROR res;  va_list ap;
  HaMake(res, a);
  /* MMake(res); */
  res->line_num = line_num;
  res->col_num = col_num;
  va_start(ap, fmt);
  vsnprintf(res->string, KML_MAX_STR, fmt, ap);
  va_end(ap);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ERROR KmlVErrorMake(HA_ARENA a, int line_num, int col_num,           */
/*    char *fmt, va_list ap)                                                 */
/*                                                                           */
/*  Make a KML error object.                                                 */
/*                                                                           */
/*****************************************************************************/

KML_ERROR KmlVErrorMake(HA_ARENA a, int line_num, int col_num,
  char *fmt, va_list ap)
{
  KML_ERROR res;
  HaMake(res, a);
  /* MMake(res); */
  res->line_num = line_num;
  res->col_num = col_num;
  vsnprintf(res->string, KML_MAX_STR, fmt, ap);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlError(KML_ERROR *ke, HA_ARENA a, int line_num, int col_num,      */
/*    char *fmt, ...)                                                        */
/*                                                                           */
/*  Make a KML error object.  For convenience when using this function,      */
/*  *ke is set to the new object, and false is returned.                     */
/*                                                                           */
/*****************************************************************************/

bool KmlError(KML_ERROR *ke, HA_ARENA a, int line_num, int col_num,
  char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  *ke = KmlVErrorMake(a, line_num, col_num, fmt, ap);
  va_end(ap);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ERROR KmlErrorCopy(KML_ERROR ke, HA_ARENA a)                         */
/*                                                                           */
/*  Return a fresh copy of ke in arena a.                                    */
/*                                                                           */
/*****************************************************************************/

KML_ERROR KmlErrorCopy(KML_ERROR ke, HA_ARENA a)
{
  return KmlErrorMake(a, KmlErrorLineNum(ke), KmlErrorColNum(ke),
    "%s", KmlErrorString(ke));
}


/*****************************************************************************/
/*                                                                           */
/*  bool NextTerm(char **fmt, char *name, TERM_TYPE *et, FACTOR_TYPE *ft)    */
/*                                                                           */
/*  If *fmt contains a next term, set name, *et, and *ft to its              */
/*  attributes, move *fmt past the term ready to read the next term.        */
/*  and return true.  If there is no next term, return false.                */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  TERM_COLON,
  TERM_COMPULSORY,
  TERM_OPTIONAL,
  TERM_SEQUENCE
} TERM_TYPE;

typedef enum {
  FACTOR_NO_TEXT,
  FACTOR_OPTIONAL_TEXT,
  FACTOR_INTEGER_TEXT,
  FACTOR_FLOAT_TEXT
} FACTOR_TYPE;

static bool NextTerm(char **fmt, char *name, TERM_TYPE *et, FACTOR_TYPE *ft)
{
  /* skip initial spaces and return false if exhausted */
  while( **fmt == ' ')
    (*fmt)++;
  if( **fmt == '\0' )
    return false;

  /* colon is special case */
  if( **fmt == ':' )
  {
    *et = TERM_COLON;
    (*fmt)++;
    return true;
  }

  /* optional + or * */
  if( **fmt == '+' )
  {
    *et = TERM_OPTIONAL;
    (*fmt)++;
  }
  else if( **fmt == '*' )
  {
    *et = TERM_SEQUENCE;
    (*fmt)++;
  }
  else
    *et = TERM_COMPULSORY;

  /* optional $ or # */
  if( **fmt == '$' )
  {
    *ft = FACTOR_OPTIONAL_TEXT;
    (*fmt)++;
  }
  else if( **fmt == '#' )
  {
    *ft = FACTOR_INTEGER_TEXT;
    (*fmt)++;
  }
  else if( **fmt == '%' )
  {
    *ft = FACTOR_FLOAT_TEXT;
    (*fmt)++;
  }
  else
    *ft = FACTOR_NO_TEXT;

  /* label proper */
  sscanf(*fmt, "%s", name);
  *fmt += strlen(name);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NextAttribute(KML_ELT elt, int *index, char *name, char *val)       */
/*                                                                           */
/*  If elt contains a next attribute (at *index, that is) with the given     */
/*  name, set *val to its value, move *index past it ready for the next      */
/*  attribute, and return true.  Otherwise return false.                     */
/*                                                                           */
/*****************************************************************************/

static bool NextAttribute(KML_ELT elt, int *index, char *name, char **val)
{
  if( *index < KmlAttributeCount(elt) &&
      strcmp(HaArray(elt->attribute_names, *index), name) == 0 )
  {
    *val = HaArray(elt->attribute_values, *index);
    (*index)++;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NextChild(KML_ELT elt, int *index, char *name, KML_ELT *child)      */
/*                                                                           */
/*  If elt contains a next child (at *index, that is) with the given         */
/*  name, set *child to that child, move *index past it ready for the next   */
/*  child, and return true.  Otherwise return false.                         */
/*                                                                           */
/*****************************************************************************/

static bool NextChild(KML_ELT elt, int *index, char *name, KML_ELT *child)
{
  if( *index < KmlChildCount(elt) &&
      /* KmlLabelMatches(KmlLabel(HaArray(elt->children, *index)), name) ) */
      strcmp(KmlLabel(HaArray(elt->children, *index)), name) == 0 )
  {
    *child = HaArray(elt->children, *index);
    (*index)++;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlCheck(KML_ELT elt, char *fmt, KML_ERROR *ke)                     */
/*                                                                           */
/*  If the attributes and children of elt conform to fmt, set *ke to NULL    */
/*  and return true.  Otherwise, set *ke to an error object describing the   */
/*  first problem encountered and return false.                              */
/*                                                                           */
/*  Each fmt string is a sequences of zero or more terms separated by        */
/*  one or more space characters:                                            */
/*                                                                           */
/*    fmt ::= { term } [ ":" { term } ]                                      */
/*                                                                           */
/*  Terms before the colon (if any) describe attributes; terms after the     */
/*  colon describe children.  Each term may be any one of                    */
/*                                                                           */
/*    term ::= factor               Exactly one must be present              */
/*    term ::= "+" factor           Zero or one must be present              */
/*    term ::= "*" factor           Zero or more must be present             */
/*                                                                           */
/*  Each factor may be any one of                                            */
/*                                                                           */
/*    factor ::= label              No text allowed                          */
/*    factor ::= "$" label          Text may be present, children may not    */
/*    factor ::= "#" label          Text denoting integer must be present    */
/*    factor ::= "%" label          Text denoting float must be present      */
/*                                                                           */
/*  There may be no spaces within terms and factors.                         */
/*                                                                           */
/*****************************************************************************/

#define test_attribute()						\
{									\
  if( ft == FACTOR_INTEGER_TEXT &&					\
      !KmlStringContainsOneIntegerOnly(val) )				\
    return KmlError(ke, a, elt->line_num, elt->col_num,			\
      "in <%s>, attribute %s does not have int value",			\
      elt->label, name);						\
  if( ft == FACTOR_FLOAT_TEXT &&					\
      !KmlStringContainsOneFloatOnly(val) )				\
    return KmlError(ke, a, elt->line_num, elt->col_num,			\
      "in <%s>, attribute %s does not have float value",		\
      elt->label, name);						\
}

#define test_child()							\
{									\
  if( ft != FACTOR_NO_TEXT && KmlChildCount(child) > 0 )		\
    return KmlError(ke, a, child->line_num, child->col_num,		\
      "child <%s> of <%s> has unexpected (out of order?) children",	\
      child->label, elt->label);					\
  text = HnStringEnd(child->text);					\
  if( ft == FACTOR_INTEGER_TEXT &&					\
      !KmlStringContainsOneIntegerOnly(text) )				\
    return KmlError(ke, a, child->line_num, child->col_num,		\
      "child <%s> of <%s> does not have integer value",			\
      child->label, elt->label, text);					\
  if( ft == FACTOR_FLOAT_TEXT &&					\
      !KmlStringContainsOneFloatOnly(text) )				\
    return KmlError(ke, a, child->line_num, child->col_num,		\
      "child <%s> of <%s> does not have float value",			\
      child->label, elt->label, text);					\
}

static HA_ARENA KmlEltArena(KML_ELT elt)
{
  return HaArrayArena(elt->children);
}

bool KmlCheck(KML_ELT elt, char *fmt, KML_ERROR *ke)
{
  int index;  char *p, *val, *text;  HA_ARENA a;
  char name[200];  TERM_TYPE et;  FACTOR_TYPE ft;  KML_ELT child;
  if( DEBUG1 )
    fprintf(stderr, "[ KmlCheck(elt, \"%s\")\n", fmt);

  /* check attributes */
  a = KmlEltArena(elt);
  p = fmt;  index = 0;
  while( NextTerm(&p, name, &et, &ft) && et != TERM_COLON ) switch( et )
  {
    case TERM_COMPULSORY:

      if( !NextAttribute(elt, &index, name, &val) )
	return KmlError(ke, a, elt->line_num, elt->col_num,
	  "in <%s>, attribute %s missing or out of order", elt->label, name);
      test_attribute();
      break;

    case TERM_OPTIONAL:

      if( NextAttribute(elt, &index, name, &val) )
	test_attribute();
      break;

    case TERM_SEQUENCE:

      while( NextAttribute(elt, &index, name, &val) )
	test_attribute();
      break;

    default:

      assert(false);
  }
  if( index < KmlAttributeCount(elt) )
    return KmlError(ke, a, elt->line_num, elt->col_num,
      "in <%s>, unexpected (out of order?) attribute %s", elt->label,
      HaArray(elt->attribute_names, index));

  /* check children */
  index = 0;
  while( NextTerm(&p, name, &et, &ft) )  switch( et )
  {
    case TERM_COMPULSORY:

      if( !NextChild(elt, &index, name, &child) )
	return KmlError(ke, a, elt->line_num, elt->col_num,
	  "in <%s>, child <%s> missing or out of order", elt->label, name);
      test_child();
      break;

    case TERM_OPTIONAL:

      if( NextChild(elt, &index, name, &child) )
	test_child();
      break;

    case TERM_SEQUENCE:

      while( NextChild(elt, &index, name, &child) )
	test_child();
      break;

    default:

      assert(false);
  }
  if( index < KmlChildCount(elt) )
  {
    child = KmlChild(elt, index);
    return KmlError(ke, a, child->line_num, child->col_num,
      "unexpected (out of order?) child <%s> in <%s>",
      child->label, elt->label);
  }
  /* ***
  if( KmlChildCount(elt) > 0 && elt->text != NULL )
    return KmlError(ke, a, elt->line_num, elt->col_num,
      "element <%s> contains both children and text", elt->label);
  *** */

  /* all fine */
  *ke = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "A.4.4 Writing XML files"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KML_FILE KmlMakeFile(FILE *fp, int initial_indent, int indent_step,      */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a KML file object with these attributes.  The arena parameter       */
/*  was added by JeffK in 11/25, following a suggestion by Ortiz.            */
/*                                                                           */
/*****************************************************************************/

KML_FILE KmlMakeFile(FILE *fp, int initial_indent, int indent_step,
  HA_ARENA a)
{
  KML_FILE res;
  HaMake(res, a);
  /* res = (KML_FILE) malloc(sizeof(struct kml_file_rec)); */
  res->fp = fp;
  res->curr_indent = initial_indent;
  res->indent_step = indent_step;
  res->attribute_allowed = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlWrite(KML_ELT elt, KML_FILE kf)                                  */
/*                                                                           */
/*  Write elt to kf.                                                         */
/*                                                                           */
/*****************************************************************************/
static void KmlEndNoIndent(KML_FILE kf, char *label);

void KmlWrite(KML_ELT elt, KML_FILE kf)
{
  int i;  char *str;  KML_ELT child;
  assert(elt != NULL);
  KmlBegin(kf, elt->label);
  HaArrayForEach(elt->attribute_names, str, i)
    KmlAttribute(kf, str, HaArray(elt->attribute_values, i));
  if( KmlStringHasNonWhite(KmlText(elt)) )
  {
    KmlPlainText(kf, KmlText(elt));
    KmlEndNoIndent(kf, elt->label);
  }
  else
  {
    HaArrayForEach(elt->children, child, i)
      KmlWrite(child, kf);
    KmlEnd(kf, elt->label);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlPrintText(FILE *fp, char *text)                                  */
/*                                                                           */
/*  Print text onto fp, taking care to handle KML predefined entities.       */
/*                                                                           */
/*****************************************************************************/

static void KmlPrintText(FILE *fp, char *text)
{
  char *p;
  for( p = text;  *p != '\0';  p++ ) switch( *p )
  {
    case '&':

      fputs("&amp;", fp);
      break;

    case '<':

      fputs("&lt;", fp);
      break;

    case '>':

      fputs("&gt;", fp);
      break;

    case '\'':

      fputs("&apos;", fp);
      break;

    case '"':

      fputs("&quot;", fp);
      break;

    default:

      putc(*p, fp);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlBegin(KML_FILE kf, char *label)                                  */
/*                                                                           */
/*  Begin printing an element with this label.                               */
/*                                                                           */
/*****************************************************************************/

void KmlBegin(KML_FILE kf, char *label)
{
  if( kf->attribute_allowed )
    fprintf(kf->fp, ">\n");
  fprintf(kf->fp, "%*s<%s", kf->curr_indent, "", label);
  kf->curr_indent += kf->indent_step;
  kf->attribute_allowed = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlAttribute(KML_FILE kf, char *name, char *text)                   */
/*                                                                           */
/*  Add an attribute to kf.                                                  */
/*                                                                           */
/*****************************************************************************/

void KmlAttribute(KML_FILE kf, char *name, char *text)
{
  assert(kf->attribute_allowed);
  fprintf(kf->fp, " %s=\"", name);
  KmlPrintText(kf->fp, text);
  fprintf(kf->fp, "\"");
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlPlainText(KML_FILE kf, char *text)                               */
/*                                                                           */
/*  Print unformatted text, possibly with funny characters.                  */
/*                                                                           */
/*****************************************************************************/

void KmlPlainText(KML_FILE kf, char *text)
{
  if( kf->attribute_allowed )
    fprintf(kf->fp, ">");
  KmlPrintText(kf->fp, text);
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlFmtText(KML_FILE kf, char *fmt, ...)                             */
/*                                                                           */
/*  Like printf but for kf.  The text may not have any funny characters.     */
/*                                                                           */
/*****************************************************************************/

void KmlFmtText(KML_FILE kf, char *fmt, ...)
{
  va_list args;
  if( kf->attribute_allowed )
    fprintf(kf->fp, ">");
  va_start(args, fmt);
  vfprintf(kf->fp, fmt, args);
  va_end(args);
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEnd(KML_FILE kf, char *label)                                    */
/*                                                                           */
/*  End print of category label.                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEnd(KML_FILE kf, char *label)
{
  kf->curr_indent -= kf->indent_step;
  assert(kf->curr_indent >= 0);
  if( kf->attribute_allowed )
    fprintf(kf->fp, "/>\n");
  else
    fprintf(kf->fp, "%*s</%s>\n", kf->curr_indent, "", label);
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEndNoIndent(KML_FILE kf, char *label)                            */
/*                                                                           */
/*  Like KmlEnd but with no indent.                                          */
/*                                                                           */
/*****************************************************************************/

static void KmlEndNoIndent(KML_FILE kf, char *label)
{
  kf->curr_indent -= kf->indent_step;
  if( kf->attribute_allowed )
    fprintf(kf->fp, "/>\n");
  else
    fprintf(kf->fp, "</%s>\n", label);
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltAttribute(KML_FILE kf, char *label, char *name, char *value)  */
/*                                                                           */
/*  Print category label with one attribute and no children or text.         */
/*                                                                           */
/*****************************************************************************/

void KmlEltAttribute(KML_FILE kf, char *label, char *name, char *value)
{
  KmlBegin(kf, label);
  KmlAttribute(kf, name, value);
  KmlEnd(kf, label);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltPlainText(KML_FILE kf, char *label, char *text)               */
/*                                                                           */
/*  Print the equivalent of                                                  */
/*                                                                           */
/*    KmlBegin(kf, label);                                                   */
/*    KmlPlainText(kf, text);                                                */
/*    KmlEnd(kf, label);                                                     */
/*                                                                           */
/*  on one line.                                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEltPlainText(KML_FILE kf, char *label, char *text)
{
  KmlBegin(kf, label);
  if( kf->attribute_allowed )
    fprintf(kf->fp, ">");
  KmlPrintText(kf->fp, text);
  kf->attribute_allowed = false;
  KmlEndNoIndent(kf, label);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltFmtText(KML_FILE kf, char *label, char *fmt, ...)             */
/*                                                                           */
/*  Print the equivalent of                                                  */
/*                                                                           */
/*    KmlBegin(kf, label);                                                   */
/*    KmlFmtText(kf, fmt, ...);                                              */
/*    KmlEnd(kf, label);                                                     */
/*                                                                           */
/*  on one line.                                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEltFmtText(KML_FILE kf, char *label, char *fmt, ...)
{
  va_list args;
  KmlBegin(kf, label);
  if( kf->attribute_allowed )
    fprintf(kf->fp, ">");
  va_start(args, fmt);
  vfprintf(kf->fp, fmt, args);
  va_end(args);
  kf->attribute_allowed = false;
  KmlEndNoIndent(kf, label);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltAttributeEltPlainText(KML_FILE kf, char *label, char *name,   */
/*    char *value, char *label2, char *text)                                 */
/*                                                                           */
/*  Equivalent to                                                            */
/*                                                                           */
/*    KmlEltAttribute(kf, label, name, value);                               */
/*    KmlEltPlainText(kf, label2, text);                                     */
/*                                                                           */
/*  on one line.                                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEltAttributeEltPlainText(KML_FILE kf, char *label, char *name,
  char *value, char *label2, char *text)
{
  KmlBegin(kf, label);
  KmlAttribute(kf, name, value);
  fprintf(kf->fp, "><%s>", label2);
  KmlPrintText(kf->fp, text);
  fprintf(kf->fp, "</%s></%s>\n", label2, label);
  kf->curr_indent -= kf->indent_step;
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltAttributeEltFmtText(KML_FILE kf, char *label, char *name,     */
/*    char *value, char *label2, char *fmt, ...)                             */
/*                                                                           */
/*  Equivalent to                                                            */
/*                                                                           */
/*    KmlEltAttribute(kf, label, name, value);                               */
/*    KmlEltFmtText(kf, label2, fmt, ...);                                   */
/*                                                                           */
/*  on one line.                                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEltAttributeFmtText(KML_FILE kf, char *label, char *name,
  char *value, char *fmt, ...)
{
  va_list args;
  KmlBegin(kf, label);
  KmlAttribute(kf, name, value);
  fprintf(kf->fp, ">");
  va_start(args, fmt);
  vfprintf(kf->fp, fmt, args);
  va_end(args);
  fprintf(kf->fp, "</%s>\n", label);
  kf->curr_indent -= kf->indent_step;
  kf->attribute_allowed = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltAttributeEltFmtText(KML_FILE kf, char *label, char *name,     */
/*    char *value, char *label2, char *fmt, ...)                             */
/*                                                                           */
/*  Equivalent to                                                            */
/*                                                                           */
/*    KmlEltAttribute(kf, label, name, value);                               */
/*    KmlEltFmtText(kf, label2, fmt, ...);                                   */
/*                                                                           */
/*  on one line.                                                             */
/*                                                                           */
/*****************************************************************************/

void KmlEltAttributeEltFmtText(KML_FILE kf, char *label, char *name,
  char *value, char *label2, char *fmt, ...)
{
  va_list args;
  KmlBegin(kf, label);
  KmlAttribute(kf, name, value);
  fprintf(kf->fp, "><%s>", label2);
  va_start(args, fmt);
  vfprintf(kf->fp, fmt, args);
  va_end(args);
  fprintf(kf->fp, "</%s></%s>\n", label2, label);
  kf->curr_indent -= kf->indent_step;
  kf->attribute_allowed = false;
}

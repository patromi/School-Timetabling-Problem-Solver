
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
/*  FILE:         kml_read.c                                                 */
/*  MODULE:       XML reading                                                */
/*                                                                           */
/*  This module parses an XML file using this grammar ($ denotes simplified) */
/*                                                                           */
/*    <document>  ::=  <prolog> <element> { <misc> }                         */
/*    <prolog>    ::=  { <xmldecl> | <dtdecl> | <misc> }                  $  */
/*    <xmldecl>   ::=  <pi>                                               $  */
/*    <dtdecl>    ::=  "<!" ... ">"            (... is anything but ">")  $  */
/*    <misc>      ::=  <comment> | <pi> | <s>                                */
/*    <comment>   ::=  "<!--" ... "-->"       (... is anything but "--")  $  */
/*    <pi>        ::=  "<?" ... "?>"          (... is anything but "?>")  $  */
/*    <s>         ::=  <os> { <os> }                                         */
/*    <os>        ::=  " "  |  "\t"  |  "\r"  |  "\n"                        */
/*                                                                           */
/*    <element>   ::=  <emptyelt>  |  <stag> <content> <etag>                */
/*    <emptyelt>  ::=  "<" <name> { <s> <attribute> } [ <s> ] "/>"           */
/*    <stag>      ::=  "<" <name> { <s> <attribute> } [ <s> ] ">"            */
/*    <etag>      ::=  "</" <name> [ <s> ] ">"                               */
/*    <attribute> ::=  <name> [ <s> ] "=" [ <s> ] <attvalue>                 */
/*    <attvalue>  ::=  "\"" { <any1> | <reference> } "\""                    */
/*    <attvalue>  ::=  "'"  { <any2> | <reference> } "'"                     */
/*    <any1>      ::=  (any character except "<", "&", and "\"")             */
/*    <any2>      ::=  (any character except "<", "&", and "'")              */
/*    <reference> ::=  "&" <name> ";"                                     $  */
/*                                                                           */
/*    <content>   ::=  [ <chardata> ] { <cintern> [ <chardata> ] }           */
/*    <chardata>  ::=  sequence of chars not containing "<", "&", or "]]>"   */
/*    <cintern>   ::=  <element> | <reference> | <cdsect> | <pi> | <comment> */
/*    <cdsect>    ::=  "<![CDATA[" ... "]]>"     (... is anything but "]]>"  */
/*                                                                           */
/*  Implementation note.  The implementation uses a recursive descent        */
/*  parser.  There is no lexical analyser; instead the parser operates       */
/*  directly on characters, with one character of lookahead.                 */
/*                                                                           */
/*****************************************************************************/
#include <setjmp.h>
#include <string.h>
#include "kml.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

#define is_space(ch) ((ch)==' ' || (ch)=='\t' || (ch)=='\n' || (ch)=='\r')
#define is_letter(ch) (((ch)>='A' && (ch)<='Z') || ((ch)>='a' && (ch)<='z'))
#define is_digit(ch) ((ch) >= '0' && (ch) <= '9')
#define is_other_name(ch) ((ch)=='_' || (ch)==':' || (ch)=='-' || (ch)=='.')


/*****************************************************************************/
/*                                                                           */
/*  KML_SEGMENT_DECL - a segment declaration                                 */
/*                                                                           */
/*  Implementation note.  When a segment declaration has a path name with    */
/*  two or more components, segment declaration objects are generated that   */
/*  do not given rise to segments.  For example, if the path name is         */
/*                                                                           */
/*     Instances/Instance                                                    */
/*                                                                           */
/*  then there will be an Instances segment declaration that does not        */
/*  give rise to a segment, under which lies an Instance declaration         */
/*  that does give rise to a segment.                                        */
/*                                                                           */
/*****************************************************************************/

typedef struct kml_segment_decl_rec *KML_SEGMENT_DECL;
typedef HA_ARRAY(KML_SEGMENT_DECL) ARRAY_KML_SEGMENT_DECL;

struct kml_segment_decl_rec {
  KML_SEGMENT_FN	segment_begin_fn;	/* called at segment begin   */
  KML_SEGMENT_FN	segment_end_fn;		/* called at segment end     */
  HA_ARRAY_NSTRING	child_labels;		/* child labels              */
  ARRAY_KML_SEGMENT_DECL child_decls;		/* child decls               */
  char			*path_name;		/* path name from decl       */
};


/*****************************************************************************/
/*                                                                           */
/*  KML_SEGMENT - a segment                                                  */
/*                                                                           */
/*****************************************************************************/

struct kml_segment_rec {
  HA_ARENA		arena;			/* the segment's own arena   */
  KML_READER		reader;			/* the enclosing ri object   */
  KML_ELT		root;			/* the root element          */
  KML_SEGMENT		parent;			/* the parent segment        */
  KML_SEGMENT_DECL	decl;			/* corresponding decl        */
  bool			begin_fn_called;	/* true after begin fn called*/
};


/*****************************************************************************/
/*                                                                           */
/*  KML_READER - a file reader object                                        */
/*                                                                           */
/*****************************************************************************/

struct kml_reader_rec {
  HA_ARENA		arena;			/* holds this object, etc.   */
  void			*impl;			/* implementation pointer    */
  HA_ARENA_SET		arena_set;		/* arena set                 */
  FILE			*fp;			/* file being read           */
  FILE			*echo_fp;		/* echo to file, if present  */
  int			line_num;		/* line number of curr_ch    */
  int			col_num;		/* column number of curr_ch  */
  int			curr_ch;		/* first unconsumed char     */
  int			curr_depth;		/* current depth of parse    */
  ARRAY_KML_SEGMENT_DECL curr_decl_stack;	/* current segment decl stack*/
  KML_SEGMENT		curr_seg;		/* the current segment       */
  KML_ERROR		ke;			/* error object              */
  jmp_buf		jmp_env;		/* early return              */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "segment declarations"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KML_SEGMENT_DECL KmlSegmentDeclMake(HA_ARENA a)                          */
/*                                                                           */
/*  Make a new segment declaration.                                          */
/*                                                                           */
/*****************************************************************************/

static KML_SEGMENT_DECL KmlSegmentDeclMake(HA_ARENA a)
{
  KML_SEGMENT_DECL res;
  HaMake(res, a);
  res->segment_begin_fn = NULL;
  res->segment_end_fn = NULL;
  HaArrayInit(res->child_labels, a);
  HaArrayInit(res->child_decls, a);
  res->path_name = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlSegmentDeclGivesRiseToSegments(KML_SEGMENT_DECL decl)            */
/*                                                                           */
/*  Only some segment declarations give rise to segments; this function      */
/*  returns true if decl does so.                                            */
/*                                                                           */
/*  Implementation note.  Semi-coincidentally, the segment declarations      */
/*  that give rise to segments have non-NULL path names.                     */
/*                                                                           */
/*****************************************************************************/

static bool KmlSegmentDeclGivesRiseToSegments(KML_SEGMENT_DECL decl)
{
  return decl->path_name != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSegmentDeclContainsName(KML_SEGMENT_DECL decl, char *name,       */
/*    KML_SEGMENT_DECL *child_decl)                                          */
/*                                                                           */
/*  If decl contains an outgoing edge labelled with this name, return true   */
/*  with *child_decl set to the child decl.  Otherwise return false.         */
/*  The special label "*" matches with any name.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheSegmentDeclContainsName(KML_SEGMENT_DECL decl, char *name,
  KML_SEGMENT_DECL *child_decl)
{
  char *label;  int i;
  HaArrayForEach(decl->child_labels, label, i)
    if( strcmp(name, label) == 0 || strcmp(label, "*") == 0 )
      return *child_decl = HaArray(decl->child_decls, i), true;
  return *child_decl = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSegmentDeclRetrieveOrInsert(KML_SEGMENT_DECL root,               */
/*    char *path_name, KML_SEGMENT_DECL *res, HA_ARENA a)                    */
/*                                                                           */
/*  Extend the tree of segment declarations rooted at root so that it        */
/*  includes a node for path_name.                                           */
/*                                                                           */
/*  If *res != NULL, make *res the node for path_name.  If this is not       */
/*  possible it is an error.  If *res == NULL, make a new node for           */
/*  path_name and set *res to that new node.                                 */
/*                                                                           */
/*****************************************************************************/

static void KmlSegmentDeclRetrieveOrInsert(KML_SEGMENT_DECL root,
  char *path_name, KML_SEGMENT_DECL *res, HA_ARENA a)
{
  char *p;  KML_SEGMENT_DECL child_decl;

  /* separate any initial part of path_name from the rest */
  p = strstr(path_name, "/");
  if( p != NULL )
    *p = '\0';

  /* make sure there is a child_decl for the initial part of path_name */
  if( !KheSegmentDeclContainsName(root, path_name, &child_decl) )
  {
    if( p == NULL && *res != NULL )
      child_decl = *res;
    else
      child_decl = KmlSegmentDeclMake(a);
    HaArrayAddLast(root->child_labels, HnStringCopy(path_name, a));
    HaArrayAddLast(root->child_decls, child_decl);
  }

  /* return child_decl if the path is ending, else keep searching */
  if( p == NULL )
  {
    /* all done, except that *res and child_decl must be the same */
    if( *res == NULL )
      *res = child_decl;
    else
      HnAssert(*res == child_decl,
	"KmlSegmentDeclRetrieveOrInsert: inconsistent declarations");
  }
  else
  {
    *p = '/';
    KmlSegmentDeclRetrieveOrInsert(child_decl, p + 1, res, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSegmentDeclDebug(KML_SEGMENT_DECL decl, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug a decl and its descendants.                                        */
/*                                                                           */
/*****************************************************************************/

static void KmlSegmentDeclDebug(KML_SEGMENT_DECL decl, int verbosity,
  int indent, FILE *fp)
{
  int i;  char *child_label;  KML_SEGMENT_DECL child_decl;
  fprintf(fp, "%*s[ SegmentDecl(%s, %s, %s)\n", indent, "",
    decl->segment_begin_fn != NULL ? "begin_fn" : "-",
    decl->segment_end_fn != NULL ? "end_fn" : "-",
    decl->path_name != NULL ? decl->path_name : "-");
  for( i = 0;  i < HaArrayCount(decl->child_labels);  i++ )
  {
    child_label = HaArray(decl->child_labels, i);
    child_decl = HaArray(decl->child_decls, i);
    fprintf(stderr, "%*s\"%s\":\n", indent + 2, "", child_label);
    KmlSegmentDeclDebug(child_decl, verbosity, indent + 2, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "segments"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KML_SEGMENT KmlSegmentMake(KML_READER ri, KML_ELT root,                  */
/*    KML_SEGMENT parent)                                                    */
/*                                                                           */
/*  Make a new segment with these attributes.                                */
/*                                                                           */
/*****************************************************************************/

static KML_SEGMENT KmlSegmentMake(KML_READER ri,
  KML_ELT root, KML_SEGMENT parent, KML_SEGMENT_DECL decl)
{
  HA_ARENA a;  KML_SEGMENT res;
  HnAssert(KmlSegmentDeclGivesRiseToSegments(decl),
    "KmlSegmentMake internal error 1");
  a = (parent == NULL ? ri->arena : HaArenaMake(ri->arena_set));
  HaMake(res, a);
  res->arena = a;
  res->reader = ri;
  res->root = root;
  res->parent = parent;
  res->decl = decl;
  res->begin_fn_called = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT KmlSegmentRoot(KML_SEGMENT ks)                                   */
/*                                                                           */
/*  Return the root element of ks.                                           */
/*                                                                           */
/*****************************************************************************/

KML_ELT KmlSegmentRoot(KML_SEGMENT ks)
{
  return ks->root;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_SEGMENT KmlSegmentParent(KML_SEGMENT ks)                             */
/*                                                                           */
/*  Return the parent segment of ks, or NULL if ks is the root segment.      */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn owing to lack of interest
KML_SEGMENT KmlSegmentParent(KML_SEGMENT ks)
{
  return ks->parent;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KML_READER KmlSegmentReader(KML_SEGMENT ks)                              */
/*                                                                           */
/*  Return the enclosing reader.                                             */
/*                                                                           */
/*****************************************************************************/

KML_READER KmlSegmentReader(KML_SEGMENT ks)
{
  return ks->reader;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSegmentFail(KML_SEGMENT ks, KML_ERROR ke)                        */
/*                                                                           */
/*  Fail now with the given error object.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KmlSegmentFail(KML_SEGMENT ks, KML_ERROR ke)
{
  KmlReaderFail(ks->reader, ke);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KmlSegmentArena(KML_SEGMENT ks)                                 */
/*                                                                           */
/*  Return ks's arena.                                                       */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KmlSegmentArena(KML_SEGMENT ks)
{
  return ks->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSegmentFree(KML_SEGMENT ks)                                      */
/*                                                                           */
/*  Free ks.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KmlSegmentFree(KML_SEGMENT ks)
{
  HaArenaDelete(ks->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "readers"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KML_READER KmlReaderMake(void *impl, HA_ARENA_SET as, HA_ARENA a)        */
/*                                                                           */
/*  Make a new reader object with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

KML_READER KmlReaderMake(void *impl, HA_ARENA_SET as, HA_ARENA a)
{
  KML_READER res;  KML_SEGMENT_DECL decl;
  HaMake(res, a);
  res->arena = a;
  res->impl = impl;
  res->arena_set = as;
  res->fp = NULL;
  res->echo_fp = NULL;
  res->line_num = -1;
  res->col_num = -1;
  res->curr_ch = '\0';
  res->curr_depth = -1;
  HaArrayInit(res->curr_decl_stack, a);
  decl = KmlSegmentDeclMake(a);
  decl->path_name = "/";
  HaArrayAddLast(res->curr_decl_stack, decl);
  res->curr_seg = NULL;
  res->ke = NULL;
  /* can't initialize jmp_env until we get to it */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KmlReaderImpl(KML_READER kr)                                       */
/*                                                                           */
/*  Return the implementation pointer attribute of kr.                       */
/*                                                                           */
/*****************************************************************************/

void *KmlReaderImpl(KML_READER kr)
{
  return kr->impl;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA_SET KmlReaderArenaSet(KML_READER kr)                            */
/*                                                                           */
/*  Return the arena_set attribute of kr.                                    */
/*                                                                           */
/*****************************************************************************/

HA_ARENA_SET KmlReaderArenaSet(KML_READER kr)
{
  return kr->arena_set;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KmlReaderArena(KML_READER kr)                                   */
/*                                                                           */
/*  Return the arena attribute of kr.                                        */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KmlReaderArena(KML_READER kr)
{
  return kr->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderFail(KML_READER ri, KML_ERROR ke)                          */
/*                                                                           */
/*  Make an immediate fail exit from the current incremental read, with      */
/*  the given ke.                                                            */
/*                                                                           */
/*****************************************************************************/

void KmlReaderFail(KML_READER ri, KML_ERROR ke)
{
  ri->ke = ke;
  longjmp(ri->jmp_env, 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderDeclareSegmentBegin(KML_READER kr, char *path_name,        */
/*    KML_SEGMENT_FN segment_begin_fn)                                       */
/*                                                                           */
/*  Begin the declaration of a segment.                                      */
/*                                                                           */
/*****************************************************************************/
char *strtok_r(char *str, const char *delim, char **saveptr);

void KmlReaderDeclareSegmentBegin(KML_READER kr, char *path_name,
  KML_SEGMENT_FN segment_begin_fn)
{
  KML_SEGMENT_DECL prnt_decl, decl;  char *p, *xp;

  /* prnt_dec is the one enclosing this decl; there must be one */
  prnt_decl = HaArrayLast(kr->curr_decl_stack);

  /* visit each colon-separated path in path_name */
  path_name = HnStringCopy(path_name, kr->arena);
  decl = NULL;
  for( p = strtok_r(path_name, ":", &xp); p != NULL;
       p = strtok_r(NULL, ":", &xp) )
    KmlSegmentDeclRetrieveOrInsert(prnt_decl, p, &decl, kr->arena);

  /* tidy up the new decl */
  HnAssert(decl != NULL, "KmlReaderDeclareSegmentBegin internal error");
  decl->segment_begin_fn = segment_begin_fn;
  decl->path_name = HnStringCopy(path_name, kr->arena);
  HaArrayAddLast(kr->curr_decl_stack, decl);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderDeclareSegmentEnd(KML_READER kr, char *path_name,          */
/*    KML_SEGMENT_FN segment_end_fn)                                         */
/*                                                                           */
/*  End the declaration of a segment.                                        */
/*                                                                           */
/*****************************************************************************/

void KmlReaderDeclareSegmentEnd(KML_READER kr, KML_SEGMENT_FN segment_end_fn)
{
  KML_SEGMENT_DECL decl;

  /* make sure that this call matches a corresponding begin */
  HnAssert(HaArrayCount(kr->curr_decl_stack) > 0, "KmlReaderDeclareSegmentEnd"
    ": no matching KmlReaderDeclareSegmentBegin");
    /* "(\"%s\"): no matching KmlReaderDeclareSegmentBegin", path_name); */
  decl = HaArrayLast(kr->curr_decl_stack);
  /* ***
  HnAssert(strcmp(path_name, decl->path_name) == 0,
    "KmlReaderDeclareSegmentEnd(\"%s\") does not match with "
    "KmlReaderDeclareSegmentBegin(\"%s\")", path_name, decl->path_name);
  *** */

  /* save segment_end_fn and pop the declaration stack */
  decl->segment_end_fn = segment_end_fn;
  HaArrayDeleteLast(kr->curr_decl_stack);
  decl = HaArrayLast(kr->curr_decl_stack);
  while( decl->path_name == NULL )
  {
    HaArrayDeleteLast(kr->curr_decl_stack);
    decl = HaArrayLast(kr->curr_decl_stack);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderDeclareSegment(KML_READER kr, char *path_name,             */
/*    KML_SEGMENT_FN segment_fn)                                             */
/*                                                                           */
/*  Begin and end the declaration of a segment.                              */
/*                                                                           */
/*****************************************************************************/

void KmlReaderDeclareSegment(KML_READER kr, char *path_name,
  KML_SEGMENT_FN segment_fn)
{
  KmlReaderDeclareSegmentBegin(kr, path_name, NULL);
  KmlReaderDeclareSegmentEnd(kr, segment_fn);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlReaderBeginSegment(KML_READER ri, char *label)                   */
/*                                                                           */
/*  An element with this label is beginning.  Update the decl stack and      */
/*  make any required segment callbacks.  Return true if a new segment       */
/*  was begun.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KmlReaderBeginSegment(KML_READER ri, char *label)
{
  KML_SEGMENT_DECL top_decl, child_decl;  KML_SEGMENT encl_seg, seg;
  top_decl = HaArrayLast(ri->curr_decl_stack);
  if( top_decl != NULL &&
      KheSegmentDeclContainsName(top_decl, label, &child_decl) )
  {
    HaArrayAddLast(ri->curr_decl_stack, child_decl);
    if( KmlSegmentDeclGivesRiseToSegments(child_decl) )
    {
      /* call begin fun of enclosing segment, if any and not done yet */
      encl_seg = ri->curr_seg;
      if( encl_seg != NULL && !encl_seg->begin_fn_called )
      {
	if( encl_seg->decl->segment_begin_fn != NULL )
	  encl_seg->decl->segment_begin_fn(encl_seg);
	encl_seg->begin_fn_called = true;
      }

      /* create the new segment corresponding to child_decl */
      /* but leave root unassigned for now */
      seg = KmlSegmentMake(ri, NULL, encl_seg, child_decl);
      ri->curr_seg = seg;
      return true;
    }
    else
      return false;
  }
  else
  {
    HaArrayAddLast(ri->curr_decl_stack, NULL);
    return false;
  }
}

/* old version that passes root but root is in the wrong arena
static bool KmlReaderBeginSegment(KML_READER ri, KML_ELT root)
{
  KML_SEGMENT_DECL top_decl, child_decl;  KML_SEGMENT encl_seg, seg;
  top_decl = HaArrayLast(ri->curr_decl_stack);
  if( top_decl != NULL &&
      KheSegmentDeclContainsName(top_decl, KmlLabel(root), &child_decl) )
  {
    HaArrayAddLast(ri->curr_decl_stack, child_decl);
    if( KmlSegmentDeclGivesRiseToSegments(child_decl) )
    {
      ** call begin fun of enclosing segment, if any and not done yet **
      encl_seg = ri->curr_seg;
      if( encl_seg != NULL && !encl_seg->begin_fn_called )
      {
	if( encl_seg->decl->segment_begin_fn != NULL )
	  encl_seg->decl->segment_begin_fn(encl_seg);
	encl_seg->begin_fn_called = true;
      }

      ** create the new segment corresponding to child_decl **
      seg = KmlSegmentMake(ri, root, encl_seg, child_decl);
      ri->curr_seg = seg;
      return true;
    }
    else
      return false;
  }
  else
  {
    HaArrayAddLast(ri->curr_decl_stack, NULL);
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderEndSegment(KML_READER ri)                                  */
/*                                                                           */
/*  An element is ending.  Pop the decl stack.                               */
/*                                                                           */
/*****************************************************************************/

static void KmlReaderEndSegment(KML_READER ri)
{
  KML_SEGMENT_DECL top_decl;  KML_SEGMENT seg, encl_seg;
  top_decl = HaArrayLast(ri->curr_decl_stack);
  HaArrayDeleteLast(ri->curr_decl_stack);
  if( top_decl != NULL && KmlSegmentDeclGivesRiseToSegments(top_decl) )
  {
    /* segment is ending */
    seg = ri->curr_seg;
    encl_seg = seg->parent;
    HnAssert(seg->decl == top_decl, "KmlReaderEndSegment internal error");

    /* call begin fun of current segment, if not done yet */
    if( !seg->begin_fn_called && top_decl->segment_begin_fn != NULL )
      top_decl->segment_begin_fn(seg);

    /* call end fun of current segment */
    if( top_decl->segment_end_fn != NULL )
      top_decl->segment_end_fn(seg);

    /* free seg if it is not the root */
    if( seg->parent != NULL )
      HaArenaDelete(seg->arena);

    /* the current seg is now seg's parent */
    ri->curr_seg = encl_seg;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlReaderDebug(KML_READER kr, int verbosity, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of kr.                                                       */
/*                                                                           */
/*****************************************************************************/

void KmlReaderDebug(KML_READER kr, int verbosity, int indent, FILE *fp)
{
  fprintf(fp, "%*s[ KmlReader\n", indent, "");
  if( HaArrayCount(kr->curr_decl_stack) > 0 )
    KmlSegmentDeclDebug(HaArrayFirst(kr->curr_decl_stack),
      verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "lexical analyser" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KmlCurrChar(KML_READER ri)                                           */
/*                                                                           */
/*  Return the next unconsumed character, possibly EOF.                      */
/*                                                                           */
/*****************************************************************************/

static int KmlCurrChar(KML_READER ri)
{
  return ri->curr_ch;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlNextChar(KML_READER ri)                                          */
/*                                                                           */
/*  Read the next character from fp, and increment ri's line number and      */
/*  column number attributes as appropriate.  The result is not '\r'.        */
/*                                                                           */
/*    <ch> ::= "\n" | "\n\r" | "\r" | "\r\n" | ...                           */
/*                                                                           */
/*  where ... stands for any other single character.  The sequences of       */
/*  two newline characters read here are converted into single characters.   */
/*                                                                           */
/*****************************************************************************/

static void KmlNextChar(KML_READER ri)
{
  int ch;
  RESTART:
  ch = getc(ri->fp);
  if( DEBUG4 )
    fprintf(stderr, "KmlNextChar(ri) read '%c' (hex %x)\n", (char) ch, ch);
  if( ch == 0xef || ch == 0xbb || ch == 0xbf )
    goto RESTART;
  else if( ch == '\n' )
  {
    /* "\n" | "\n\r" */
    ch = getc(ri->fp);
    if( ch != '\r' )
      ungetc(ch, ri->fp);
    ri->line_num++, ri->col_num = 0;
    ri->curr_ch = '\n';
  }
  else if( ch == '\r' )
  {
    /* "\r" | "\r\n" */
    ch = getc(ri->fp);
    if( ch != '\n' )
      ungetc(ch, ri->fp);
    ri->line_num++, ri->col_num = 0;
    ri->curr_ch = '\n';
  }
  else
  {
    /* ... */
    ri->col_num++;
    ri->curr_ch = ch;
  }
  if( ri->echo_fp != NULL && ri->curr_ch != EOF )
    putc(ri->curr_ch, ri->echo_fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parser - character-skipping functions" (private)              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KmlSkipChars(KML_READER ri, char *start_str, char *end_str)         */
/*                                                                           */
/*  Skip chars until KmlCurrChar is the first character after end_str:       */
/*                                                                           */
/*    <start_str> ... <end_str>    (where ... is anything except <end_str>)  */
/*                                                                           */
/*****************************************************************************/

static void KmlSkipChars(KML_READER ri, char *start_str, char *end_str)
{
  char buff[5];  int i, len, line_num, col_num;
  len = strlen(end_str);
  HnAssert(len > 0 && len < 5, "KmlSkipChars internal error");
  for( i = 0;  i <= len;  i++ )
    buff[i] = '\0';
  line_num = ri->line_num, col_num = ri->col_num;
  for( ;; )
  {
    /* shift buff and add current char to it */
    for( i = 1;  i < len;  i++ )
      buff[i-1] = buff[i];
    buff[i-1] = KmlCurrChar(ri);

    /* move to next char */
    KmlNextChar(ri);

    /* exit if match */
    if( strcmp(end_str, buff) == 0 )
      return;

    /* fail if current char is EOF */
    if( KmlCurrChar(ri) == EOF )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, line_num, col_num,
	"\"%s\" has no matching \"%s\"", start_str, end_str));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSkipZeroOrMoreSpaces(KML_READER ri)                              */
/*                                                                           */
/*  Skip zero or more white space characters.                                */
/*                                                                           */
/*      [ <s> ]  aka  { <os> }                                               */
/*                                                                           */
/*****************************************************************************/

static void KmlSkipZeroOrMoreSpaces(KML_READER ri)
{
  while( is_space(KmlCurrChar(ri)) )
    KmlNextChar(ri);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlSkipOneOrMoreSpaces(KML_READER ri)                               */
/*                                                                           */
/*  Skip one or more white space characters.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** unused owing to various grammar transformations
static void KmlSkipOneOrMoreSpaces(KML_READER ri)
{
  if( !is_space(KmlCurrChar(ri)) )
    KmlReaderFail(ri, KmlErrorMake(ri->line_num, ri->col_num,
      "expected white space character here"));
  do
  {
    KmlNextChar(ri);
  }
  while( is_space(KmlCurrChar(ri)) );
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parser - names and references" (private)                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KmlCharIsNameChar(int ch)                                           */
/*                                                                           */
/*  Return true if ch may occur within a name.  This is only a rough stab    */
/*  at the XML specification.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KmlCharIsNameChar(int ch)
{
  return is_letter(ch) || is_digit(ch) || is_other_name(ch);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseName(KML_READER ri, char buff[501])                         */
/*                                                                           */
/*  Parse a name.  The result is in buff.                                    */
/*                                                                           */
/*****************************************************************************/

static void KmlParseName(KML_READER ri, char buff[501])
{
  int i;
  for( i = 0;  i < 500 && KmlCharIsNameChar(KmlCurrChar(ri));  i++ )
  {
    buff[i] = KmlCurrChar(ri);
    KmlNextChar(ri);
  }
  buff[i] = '\0';
  if( i >= 500 )
    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
      "name %s... too long", buff));
}


/*****************************************************************************/
/*                                                                           */
/*  char KmlParseReference(KML_READER ri)                                    */
/*                                                                           */
/*  Parse one reference, returning the character it refers to.               */
/*                                                                           */
/*    <reference> ::=  "&" <name> ";"                                        */
/*                                                                           */
/*****************************************************************************/

static char KmlParseReference(KML_READER ri)
{
  char name[501];
  KmlNextChar(ri);  /* skip opening & */
  KmlParseName(ri, name);
  if( KmlCurrChar(ri) != ';' )
    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
      "missing ; after &%s", name));
  KmlNextChar(ri);
  if( strcmp(name, "lt") == 0 )
    return '<';
  else if( strcmp(name, "gt") == 0 )
    return '>';
  else if( strcmp(name, "amp") == 0 )
    return '&';
  else if( strcmp(name, "apos") == 0 )
    return '\'';
  else if( strcmp(name, "quot") == 0 )
    return '"';
  else
  {
    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
      "reference &%s; unknown", name));
    return '\0';  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parsing - main part" (private)                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KmlParseProlog(KML_READER ri)                                       */
/*                                                                           */
/*  Parse the prolog.  When this function returns, the first "<" of the      */
/*  root element has been read.  Officially, the syntax is                   */
/*                                                                           */
/*    <prolog>    ::=  [ <xmldecl> ] { <misc> } [ <dtdecl> { <misc> } ]      */
/*                                                                           */
/*  but the syntax actually parsed here is the more general and easier       */
/*                                                                           */
/*    <prolog>    ::= { <xmldecl> | <dtdecl> | <comment> | <pi> | <s> } "<"  */
/*                                                                           */
/*****************************************************************************/

static void KmlParseProlog(KML_READER ri)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KmlParseProlog(ri)\n");
  for( ;; ) switch( KmlCurrChar(ri) )
  {
    case '<':

      /* <xmldecl>, <dtdecl>, <comment>, or <pi> */
      KmlNextChar(ri);
      if( KmlCurrChar(ri) == '!' )
      {
	/* <dtdecl> or <comment> */
	KmlNextChar(ri);
	if( KmlCurrChar(ri) == '-' )
	{
	  /* <comment> */
	  KmlSkipChars(ri, "<!-", "-->");
	}
	else
	{
	  /* <dtdecl> */
	  KmlSkipChars(ri, "<!", ">");
	}
      }
      else if( KmlCurrChar(ri) == '?' )
      {
	/* <xmldecl> or <pi> */
	KmlSkipChars(ri, "<?", "?>");
      }
      else
      {
	/* the only return; curr char is char following "<" */
	if( DEBUG1 )
	  fprintf(stderr, "] KmlParseProlog\n");
	return;
      }
      break;

    /* <s> */
    case '\n':
    case '\t':
    case ' ':

      KmlNextChar(ri);
      break;

    default:

      if( KmlCurrChar(ri) == EOF )
	KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	  "unexpected end of file in prolog"));
      else
	KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	  "unexpected character '%c' in prolog", KmlCurrChar(ri)));
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseAttributeValue(KML_READER ri, char *attribute_name,         */
/*    char buff[501])                                                        */
/*                                                                           */
/*  Parse one attribute value, including the opening and closing delimiter,  */
/*  which is KmlCurrChar(ri).  The result is in buff.                        */
/*                                                                           */
/*    <attvalue>  ::=  "\"" { <any1> | <reference> } "\""                    */
/*    <attvalue>  ::=  "'" { <any2> | <reference> } "'"                      */
/*    <any1>      ::=  (any character except "<", "&", and "\"")             */
/*    <any2>      ::=  (any character except "<", "&", and "'")              */
/*                                                                           */
/*****************************************************************************/

static void KmlParseAttributeValue(KML_READER ri, char *attribute_name,
  char buff[501])
{
  int i, delim_ch, line_num, col_num;
  delim_ch = KmlCurrChar(ri);
  line_num = ri->line_num, col_num = ri->col_num;
  KmlNextChar(ri);  /* skip opening delimiter */
  for( i = 0;  KmlCurrChar(ri) != delim_ch;  i++ )
  {
    if( i >= 500 )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, line_num, col_num,
	"value of attribute %s is too long", attribute_name));
    else if( KmlCurrChar(ri) == '<' )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, line_num, col_num,
	"'<' character within value of attribute %s", attribute_name));
    else if( KmlCurrChar(ri) == EOF )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, line_num, col_num,
	"end of input within value of attribute %s", attribute_name));
    else if( KmlCurrChar(ri) == '&' )
      buff[i] = KmlParseReference(ri);
    else
    {
      buff[i] = KmlCurrChar(ri);
      KmlNextChar(ri);
    }
  }
  buff[i] = '\0';
  KmlNextChar(ri);  /* skip closing delimiter */
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseAttribute(KML_READER ri, KML_ELT prnt)                      */
/*                                                                           */
/*  Parse an attribute and add it to prnt.  The syntax is                    */
/*                                                                           */
/*    <attribute> ::=  <name> [ <s> ] "=" [ <s> ] <attvalue>                 */
/*                                                                           */
/*****************************************************************************/

static void KmlParseAttribute(KML_READER ri, KML_ELT prnt)
{
  char name[501], value[501];
  KmlParseName(ri, name);
  KmlSkipZeroOrMoreSpaces(ri);
  if( KmlCurrChar(ri) != '=' )
    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
      "expected = after attribute name %s", name));
  KmlNextChar(ri);
  KmlSkipZeroOrMoreSpaces(ri);
  if( KmlCurrChar(ri) != '"' && KmlCurrChar(ri) != '\'' )
    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
      "expected ' or \" at the start of value of attribute %s", name));
  KmlParseAttributeValue(ri, name, value);
  KmlAddAttribute(prnt, name, value);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseContent(KML_READER ri, KML_ELT prnt)                        */
/*                                                                           */
/*  Parse content and add it to prnt.  The syntax is                         */
/*                                                                           */
/*    <content>   ::=  [ <chardata> ] { <cintern> [ <chardata> ] }           */
/*    <chardata>  ::=  sequence of chars not containing "<", "&", or "]]>"   */
/*    <cintern>   ::=  <element> | <reference> | <cdsect> | <pi> | <comment> */
/*    <cdsect>    ::=  "<![CDATA[" ... "]]>"     (... is anything but "]]>"  */
/*                                                                           */
/*  Actually this function parses the equivalent grammar                     */
/*                                                                           */
/*    <content>   ::=  { <reference> | <cstuff> | <chardata> }               */
/*    <cstuff>    ::=  <cdsect> | <comment> | <pi> | <element>               */
/*                                                                           */
/*  When it returns it has already consumed the initial "<" of the <etag>.   */
/*                                                                           */
/*****************************************************************************/
static void KmlParseElement(KML_READER ri, KML_ELT prnt);

static void KmlParseContent(KML_READER ri, KML_ELT prnt)
{
  int i;  char buff[501];
  if( DEBUG3 )
    fprintf(stderr, "%*s[ ParseContent(...)\n", ri->curr_depth * 2, "");
  i = 0;
  for( ;; )
  {
    switch( KmlCurrChar(ri) )
    {
      case '&':

	/* <reference> */
	if( DEBUG3 )
	  fprintf(stderr, "%*s  ParseContent (2)\n", ri->curr_depth * 2, "");
        buff[i++] = KmlParseReference(ri);
	if( i >= 500 )
	{
	  buff[i] = '\0';
	  KmlAddText(prnt, buff);
	  i = 0;
	}
	break;

      case '<':

	/* <cstuff> or start of <etag> */
	if( DEBUG3 )
	  fprintf(stderr, "%*s  ParseContent (3)\n", ri->curr_depth * 2, "");
	KmlNextChar(ri);
	switch( KmlCurrChar(ri) )
	{
	  case '/':

	    /* <etag> ::=  "</" <name> [ <s> ] ">" */
	    if( DEBUG3 )
	      fprintf(stderr, "%*s  ParseContent (4)\n", ri->curr_depth*2, "");
	    if( i > 0 )
	    {
	      buff[i] = '\0';
	      KmlAddText(prnt, buff);
	    }
	    if( DEBUG3 )
	      fprintf(stderr, "%*s] ParseContent returning\n",
		ri->curr_depth * 2, "");
	    return;  /* only way out */

	  case '!':

	    /* <cdsect> | <comment> */
	    if( DEBUG3 )
	      fprintf(stderr, "%*s  ParseContent (5)\n", ri->curr_depth*2, "");
	    KmlNextChar(ri);
	    if( KmlCurrChar(ri) == '-' )
	    {
	      /* <comment> */
	      KmlSkipChars(ri, "<!-", "-->");
	    }
	    else
	    {
	      /* <cdsect> */
	      KmlSkipChars(ri, "<!", "]]>");
	    }
	    break;

	  case '?':

	    /* <pi> */
	    if( DEBUG3 )
	      fprintf(stderr, "%*s  ParseContent (6)\n", ri->curr_depth*2, "");
            KmlSkipChars(ri, "<?", "?>");
	    break;

	  default:

	    /* <element> */
	    if( DEBUG3 )
	      fprintf(stderr, "%*s  ParseContent (7)\n", ri->curr_depth*2, "");
            KmlParseElement(ri, prnt);
	    break;
	}
	break;

      case EOF:

	/* unexpected end of file */
        KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
          "unexpected end of file (within %s element at line %d, col %d)",
	  KmlLabel(prnt), KmlLineNum(prnt), KmlLineNum(prnt)));
	break;

      default:

        /* <chardata> */
	buff[i++] = KmlCurrChar(ri);
	if( i >= 500 )
	{
	  buff[i] = '\0';
	  KmlAddText(prnt, buff);
	  i = 0;
	}
	KmlNextChar(ri);
	break;
    }
  }
  HnAbort("KmlParseContent internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseElement(KML_READER ri, KML_ELT prnt)                        */
/*                                                                           */
/*  Parse an <element> according to syntax                                   */
/*                                                                           */
/*    <element>   ::=  <emptyelt>  |  <stag> <content> <etag>                */
/*    <emptyelt>  ::=  "<" <name> { <s> <attribute> } [ <s> ] "/>"           */
/*    <stag>      ::=  "<" <name> { <s> <attribute> } [ <s> ] ">"            */
/*    <etag>      ::=  "</" <name> [ <s> ] ">"                               */
/*                                                                           */
/*  Actually the initial "<" has already been read when this is called,      */
/*  and there are LL difficulties in the grammar as given.  So the grammar   */
/*  actually parsed is                                                       */
/*                                                                           */
/*    <element>   ::=  <elem_init> <elem_rest>                               */
/*    <elem_init> ::=  <name> [ <s> ] { <attribute> [ <s> ] }                */
/*    <elem_rest> ::=  "/>"                                                  */
/*    <elem_rest> ::=  ">" <content> <etag>                                  */
/*    <etag>      ::=  "</" <name> [ <s> ] ">"                               */
/*                                                                           */
/*  The resulting KML_ELT is not returned.  It is handed off to a callback   */
/*  function if it has been declared to be a segment, or else added to       */
/*  prnt if prnt is non-NULL.                                                */
/*                                                                           */
/*****************************************************************************/

static void KmlParseElement(KML_READER ri, KML_ELT prnt)
{
  KML_ELT res;  char name[501], end_name[501];  bool is_segment;
  int line_num, col_num, end_line_num, end_col_num;

  /* <name> */
  line_num = ri->line_num;  col_num = ri->col_num - 1;
  KmlParseName(ri, name);
  ri->curr_depth++;
  if( DEBUG2 )
    fprintf(stderr, "%*s[ KmlParseElement(), line %d col %d name %s depth %d\n",
      ri->curr_depth * 2, "", ri->line_num, ri->col_num, name, ri->curr_depth);

  /* decide whether this is a segment (includes pushing stack if so) */
  is_segment = KmlReaderBeginSegment(ri, name);
  res = KmlMakeElt(line_num, col_num, name, ri->curr_seg->arena);
  if( !is_segment && prnt != NULL )
    KmlAddChild(prnt, res);
  else
    ri->curr_seg->root = res;

  /* [ <s> ] { <attribute> [ <s> ] } */
  KmlSkipZeroOrMoreSpaces(ri);
  while( KmlCurrChar(ri) != '>' && KmlCurrChar(ri) != '/' )
  {
    KmlParseAttribute(ri, res);
    KmlSkipZeroOrMoreSpaces(ri);
  }
  if( DEBUG2 )
    fprintf(stderr, "%*s  KmlParseElement (2)\n", ri->curr_depth * 2, "");

  /* <elem_rest> */
  if( KmlCurrChar(ri) == '>' )
  {
    /* ">" */
    KmlNextChar(ri);

    /* <content> */
    if( DEBUG2 )
      fprintf(stderr, "%*s  KmlParseElement (3)\n", ri->curr_depth * 2, "");
    KmlParseContent(ri, res);
    if( DEBUG2 )
      fprintf(stderr, "%*s  KmlParseElement (4)\n", ri->curr_depth * 2, "");

    /* <etag> ::= "</" <name> [ <s> ] ">"   (except "<" is already read) */
    end_line_num = ri->line_num;  end_col_num = ri->col_num - 1;
    if( KmlCurrChar(ri) != '/' )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	"expected / of </%s> here", name));
    KmlNextChar(ri);
    KmlParseName(ri, end_name);
    if( strcmp(end_name, name) != 0 )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, end_line_num, end_col_num,
	"</%s> failed to match <%s> at line %d, column %d", end_name,
	name, line_num, col_num));
    KmlSkipZeroOrMoreSpaces(ri);
    if( KmlCurrChar(ri) != '>' )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	"expected > of </%s> here", name));
    KmlNextChar(ri);
  }
  else /* KmlCurrChar(ri) == '/' */
  {
    KmlNextChar(ri);
    if( KmlCurrChar(ri) != '>' )
      KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	"/ not followed by >"));
    KmlNextChar(ri);
  }

  /* optionally pop the segment stack */
  KmlReaderEndSegment(ri);

  if( DEBUG2 )
    fprintf(stderr, "%*s] KmlParseElement()\n", ri->curr_depth * 2, "");
  ri->curr_depth--;
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlParseDocument(KML_READER ri)                                     */
/*                                                                           */
/*  Parse an XML document according to the grammar for <document> above:     */
/*                                                                           */
/*    <document>  ::=  <prolog> <element> { <misc> }                         */
/*    <misc>      ::=  <comment> | <pi> | <s>                                */
/*                                                                           */
/*  The result is handed off to a callback function, not returned.           */
/*                                                                           */
/*****************************************************************************/

static void KmlParseDocument(KML_READER ri)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KmlParseDocument(ri)\n");

  /* <prolog> */
  KmlParseProlog(ri);

  /* <element> */
  KmlParseElement(ri, NULL);

  /* { <comment> | <pi> | <s> } */
  switch( KmlCurrChar(ri) )
  {
    case '<':

      /* <comment> | <pi> */
      KmlNextChar(ri);
      switch( KmlCurrChar(ri) )
      {
	case '!':

	  /* <comment> */
          KmlNextChar(ri);
	  if( KmlCurrChar(ri) != '-' )
	    KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	      "in epilogue, <! not followed by -"));
          KmlSkipChars(ri, "<!-", "-->");
	  break;

	case '?':

	  /* <pi> */
          KmlSkipChars(ri, "<?", "?>");
	  break;

	default:

	  KmlReaderFail(ri, KmlErrorMake(ri->arena, ri->line_num, ri->col_num,
	    "in epilogue, < not followed by ! or ?"));
	  break;
      }
      break;

    case ' ':
    case '\t':
    case '\n':

      /* <s> */
      break;

    case EOF:

      /* normal exit */
      return;

    default:

      /* following non-XML, preserved for whoever wants it */
      ungetc(KmlCurrChar(ri), ri->fp);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KmlParseDocument returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KmlReaderReadFileSegmented(KML_READER kr, FILE *fp, FILE *echo_fp,  */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Incrementally read fp, calling back before and after each segment.       */
/*                                                                           */
/*****************************************************************************/

bool KmlReaderReadFileSegmented(KML_READER kr, FILE *fp, FILE *echo_fp,
  KML_ERROR *ke)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KmlReaderReadFileSegmented(...)\n");

  /* initialize all the fields that vary with each file parsed */
  kr->fp = fp;
  kr->echo_fp = echo_fp;
  kr->line_num = 1;
  kr->col_num = 0;
  KmlNextChar(kr);
  kr->curr_depth = -1;
  while( HaArrayCount(kr->curr_decl_stack) > 1 )
    HaArrayDeleteLast(kr->curr_decl_stack);
  kr->ke = NULL;

  /* find a document within fp - or not */
  if( setjmp(kr->jmp_env) == 0 )
  {
    KmlParseDocument(kr);
    *ke = NULL;
    if( DEBUG1 )
      fprintf(stderr, "] KmlReaderReadFileSegmented returning true\n");
    return true;
  }
  else
  {
    *ke = kr->ke;
    if( DEBUG1 )
      fprintf(stderr, "] KmlReaderReadFileSegmented returning false\n");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlReadFile(FILE *fp, KML_ELT *res, KML_ERROR *ke, FILE *echo_fp,   */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Read an XML element from fp.  If successful, set *res to the element     */
/*  and return true.  Otherwise set *ke to an error object describing the    */
/*  first problem encountered, and return false.                             */
/*                                                                           */
/*****************************************************************************/

static void end_fn(KML_SEGMENT ks)
{
  KML_ELT *res = (KML_ELT *) KmlReaderImpl(KmlSegmentReader(ks));
  *res = KmlSegmentRoot(ks);
}

bool KmlReadFile(FILE *fp, FILE *echo_fp, KML_ELT *res, KML_ERROR *ke,
  HA_ARENA a)
{
  KML_READER kr;
  kr = KmlReaderMake((void *) res, NULL, a);
  KmlReaderDeclareSegmentBegin(kr, "*", NULL);
  KmlReaderDeclareSegmentEnd(kr, /* "*", */ &end_fn);
  return KmlReaderReadFileSegmented(kr, fp, echo_fp, ke);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlReadString(char *str, char *stop_str, KML_ELT *res,KML_ERROR *ke)*/
/*                                                                           */
/*  Like KmlReadFile just above, but reading from a string, not from a file. */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
bool KmlReadString(char *str, KML_ELT *res, KML_ERROR *ke, HA_ARENA a)
{
  HnAbort("KmlReadString stil l to do");
  return false;
}
*** */

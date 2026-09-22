
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
/*  FILE:         khe_sm_solver.c                                            */
/*  DESCRIPTION:  Syntax-directed solver                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0		/* KheSolverRun */
#define DEBUG2	0		/* stats */
#define DEBUG3	0		/* logging of solve items */
#define DEBUG4	0		/* checking Sequential */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "types"                                                        */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* documented for general solvers */
  KHE_SOLVER_DO,
  KHE_SOLVER_SKIP,
  KHE_SOLVER_TS,
  KHE_SOLVER_RS,
  KHE_SOLVER_GDL,
  KHE_SOLVER_GTS,
  KHE_SOLVER_GTI,
  KHE_SOLVER_GEM,
  KHE_SOLVER_GTP,
  /* KHE_SOLVER_GUX, */
  KHE_SOLVER_GPU,

  /* documented for time solvers */
  KHE_SOLVER_TCL,
  KHE_SOLVER_TBR,
  KHE_SOLVER_TRT,
  KHE_SOLVER_TPA,
  KHE_SOLVER_TNP,
  KHE_SOLVER_TTP,
  KHE_SOLVER_TMD,
  KHE_SOLVER_TNL,
  KHE_SOLVER_TEC,
  KHE_SOLVER_TNF,
  KHE_SOLVER_TDZ,

  /* documented for resource structural */
  KHE_SOLVER_RT,
  KHE_SOLVER_RTC,	/* converted version of RT, done when reading */
  KHE_SOLVER_RRD,
  KHE_SOLVER_REM,
  KHE_SOLVER_RWP,
  KHE_SOLVER_RSM,
  KHE_SOLVER_RCM,
  KHE_SOLVER_RBW,
  KHE_SOLVER_RGC,
  KHE_SOLVER_RGR,
  KHE_SOLVER_RED,

  /* documented for resource solvers */
  KHE_SOLVER_RIN,
  KHE_SOLVER_RRQ,
  KHE_SOLVER_RAH,
  KHE_SOLVER_RMC,
  KHE_SOLVER_RPK,
  KHE_SOLVER_RCX,
  KHE_SOLVER_RFS,
  KHE_SOLVER_RDV,
  KHE_SOLVER_RDT,
  KHE_SOLVER_RDS,
  KHE_SOLVER_RTS,
  KHE_SOLVER_RRM,
  KHE_SOLVER_RRH,
  KHE_SOLVER_RMU,
  KHE_SOLVER_REC,
  KHE_SOLVER_RGL,
  KHE_SOLVER_RRS,
  KHE_SOLVER_RRP,

  /* this value just counts the number of elements */
  KHE_SOLVER_COUNT
} KHE_SOLVER_ID;

typedef struct khe_solver_item_rec *KHE_SOLVER_ITEM;
typedef HA_ARRAY(KHE_SOLVER_ITEM) ARRAY_KHE_SOLVER_ITEM;

typedef struct khe_solver_rec *KHE_SOLVER;

struct khe_solver_item_rec {
  int			time_weight;
  KHE_SOLVER_ID		id;
  KHE_SOLVER		solver;
  KHE_TASKING		rtc_tasking;   /* used only by RTC type */
};

struct khe_solver_rec {
  ARRAY_KHE_SOLVER_ITEM	items;
};

typedef struct khe_parser_rec {
  HA_ARENA		arena;
  KHE_SOLN		soln;
  char			*name;
  char			*val;
  int			pos;
} *KHE_PARSER;

typedef struct khe_solver_stats_rec {
  KHE_SOLVER_ID		solver_id;
  int			no_of_calls;
  KHE_COST		total_cost_drop;
  float			total_running_time;
} *KHE_SOLVER_STATS;

typedef HA_ARRAY(KHE_SOLVER_STATS) ARRAY_KHE_SOLVER_STATS;

typedef struct khe_stats_rec {
  char				*label;
  ARRAY_KHE_SOLVER_STATS	stats;
  KHE_TIMER			timer;
} *KHE_STATS;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PARSER"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PARSER KheParserMake(KHE_SOLN soln, char *name, char *val,           */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a parser object with these attributes.                              */
/*                                                                           */
/*****************************************************************************/
static void KheParserSkipWhiteSpace(KHE_PARSER kp);

static KHE_PARSER KheParserMake(KHE_SOLN soln, char *name, char *val,
  HA_ARENA a)
{
  KHE_PARSER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->name = name;
  res->val = val;
  res->pos = 0;
  KheParserSkipWhiteSpace(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheParserAbort(KHE_PARSER kp, char *fmt, ...)                       */
/*                                                                           */
/*  Abort a parse with an error message.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheParserAbort(KHE_PARSER kp, char *fmt, ...)
{
  va_list args;
  fprintf(stderr, "khe: in %s option, ", kp->name);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, ":\n");
  va_end(args);
  fprintf(stderr, "  %s=\"%s\"\n", kp->name, kp->val);
  HnAbort("  %*s  %*s^ the error is here or just before here\n",
    strlen(kp->name), "", kp->pos, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "lexing"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char KheParserCurrCh(KHE_PARSER kp)                                      */
/*                                                                           */
/*  Return the character that kp is currently up to.                         */
/*                                                                           */
/*****************************************************************************/

static char KheParserCurrCh(KHE_PARSER kp)
{
  return kp->val[kp->pos];
}


/*****************************************************************************/
/*                                                                           */
/*  void KheParserNextCh(KHE_PARSER kp)                                      */
/*                                                                           */
/*  Move kp on to the next character.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheParserNextCh(KHE_PARSER kp)
{
  (kp->pos)++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheParserSkipWhiteSpace(KHE_PARSER kp)                              */
/*                                                                           */
/*  Skip kp past any white space.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheParserSkipWhiteSpace(KHE_PARSER kp)
{
  char ch;
  ch = KheParserCurrCh(kp);
  while( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' )
  {
    KheParserNextCh(kp);
    ch = KheParserCurrCh(kp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSkipWhiteSpace(char **str)                                       */
/*                                                                           */
/*  Move *str past any initial white space.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSkipWhiteSpace(char **str)
{
  char ch;
  ch = **str;
  while( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' )
  {
    *str += 1;
    ch = **str;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSkipChar(char **str, char ch)                                    */
/*                                                                           */
/*  Skip ch and any following white space.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSkipChar(char **str, char ch)
{
  if( **str != ch )
    HnAbort("expected but did not find `%c` in rs option string", ch);
  *str += 1;
  KheSkipWhiteSpace(str);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheParserSkipChar(KHE_PARSER kp, char ch)                           */
/*                                                                           */
/*  Skip ch (which must be there) and any following white space.             */
/*                                                                           */
/*****************************************************************************/

static void KheParserSkipChar(KHE_PARSER kp, char ch)
{
  if( KheParserCurrCh(kp) != ch )
    KheParserAbort(kp, "'%c' expected but found '%c'", ch, kp->val[kp->pos]);
  KheParserNextCh(kp);
  KheParserSkipWhiteSpace(kp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheParserParseInt(KHE_PARSER kp, int *val)                          */
/*                                                                           */
/*  Parse an integer and any following white space.                          */
/*                                                                           */
/*****************************************************************************/

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static void KheParserParseInt(KHE_PARSER kp, int *val)
{
  if( !is_digit(KheParserCurrCh(kp)) )
    KheParserAbort(kp, "integer expected but found '%c'", KheParserCurrCh(kp));
  sscanf(&kp->val[kp->pos], "%d", val);
  while( is_digit(KheParserCurrCh(kp)) )
    KheParserNextCh(kp);
  KheParserSkipWhiteSpace(kp);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseId(char **str, char buff[20])                               */
/*                                                                           */
/*  Parse an identifier and any following white space.                       */
/*                                                                           */
/*****************************************************************************/

#define is_letter(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

/* ***
static bool KheParseId(char **str, char buff[20])
{
  int i;
  i = 0;
  while( is_letter(**str) && i < 20 )
  {
    buff[i++] = **str;
    *str += 1;
  }
  buff[i++] = '\0';
  KheSkipWhiteSpace(str);
  return i >= 2;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheParserParseId(KHE_PARSER kp, char buff[20])                      */
/*                                                                           */
/*  Parse an identifier and any following white space.                       */
/*                                                                           */
/*****************************************************************************/

static void KheParserParseId(KHE_PARSER kp, char buff[20])
{
  int i;  char ch;
  i = 0;
  ch = KheParserCurrCh(kp);
  if( !is_letter(ch) )
    KheParserAbort(kp, "identifier expected but found '%c'", ch);
  while( is_letter(ch) && i < 20 )
  {
    buff[i++] = ch;
    KheParserNextCh(kp);
    ch = KheParserCurrCh(kp);
  }
  buff[i++] = '\0';
  KheParserSkipWhiteSpace(kp);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_ID KheParserConvertIdToSolverId(KHE_PARSER kp, char buff[20]) */
/*                                                                           */
/*  Convert buff into a solver Id, or else abort.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_ID KheParserConvertIdToSolverId(KHE_PARSER kp, char buff[20])
{
  switch( buff[0] )
  {
    case 'd':
    case 's':
    case 'g':

      /* general solvers */
      if( strcmp(buff, "do") == 0 )
	return KHE_SOLVER_DO;
      else if( strcmp(buff, "skip") == 0 )
	return KHE_SOLVER_SKIP;
      else if( strcmp(buff, "gdl") == 0 )
	return KHE_SOLVER_GDL;
      else if( strcmp(buff, "gts") == 0 )
	return KHE_SOLVER_GTS;
      else if( strcmp(buff, "gti") == 0 )
	return KHE_SOLVER_GTI;
      else if( strcmp(buff, "gem") == 0 )
	return KHE_SOLVER_GEM;
      else if( strcmp(buff, "gtp") == 0 )
	return KHE_SOLVER_GTP;
      else if( strcmp(buff, "gpu") == 0 )
	return KHE_SOLVER_GPU;
      break;

    case 't':

      /* time solvers */
      if( strcmp(buff, "ts") == 0 )
	return KHE_SOLVER_TS;
      else if( strcmp(buff, "tcl") == 0 )
	return KHE_SOLVER_TCL;
      else if( strcmp(buff, "tbr") == 0 )
	return KHE_SOLVER_TBR;
      else if( strcmp(buff, "trt") == 0 )
	return KHE_SOLVER_TRT;
      else if( strcmp(buff, "tpa") == 0 )
	return KHE_SOLVER_TPA;
      else if( strcmp(buff, "tnp") == 0 )
	return KHE_SOLVER_TNP;
      else if( strcmp(buff, "ttp") == 0 )
	return KHE_SOLVER_TTP;
      else if( strcmp(buff, "tmd") == 0 )
	return KHE_SOLVER_TMD;
      else if( strcmp(buff, "tnl") == 0 )
	return KHE_SOLVER_TNL;
      else if( strcmp(buff, "tec") == 0 )
	return KHE_SOLVER_TEC;
      else if( strcmp(buff, "tnf") == 0 )
	return KHE_SOLVER_TNF;
      else if( strcmp(buff, "tdz") == 0 )
	return KHE_SOLVER_TDZ;
      break;

    case 'r':

      /* resource structural */
      if( strcmp(buff, "rs") == 0 )
	return KHE_SOLVER_RS;
      else if( strcmp(buff, "rt") == 0 )
	return KHE_SOLVER_RT;
      else if( strcmp(buff, "rrd") == 0 )
	return KHE_SOLVER_RRD;
      else if( strcmp(buff, "rem") == 0 )
	return KHE_SOLVER_REM;
      else if( strcmp(buff, "rwp") == 0 )
	return KHE_SOLVER_RWP;
      else if( strcmp(buff, "rsm") == 0 )
	return KHE_SOLVER_RSM;
      else if( strcmp(buff, "rcm") == 0 )
	return KHE_SOLVER_RCM;
      else if( strcmp(buff, "rbw") == 0 )
	return KHE_SOLVER_RBW;
      else if( strcmp(buff, "rgc") == 0 )
	return KHE_SOLVER_RGC;
      else if( strcmp(buff, "rgr") == 0 )
	return KHE_SOLVER_RGR;
      else if( strcmp(buff, "red") == 0 )
	return KHE_SOLVER_RED;

      /* resource solvers */
      else if( strcmp(buff, "rin") == 0 )
	return KHE_SOLVER_RIN;
      else if( strcmp(buff, "rrq") == 0 )
	return KHE_SOLVER_RRQ;
      else if( strcmp(buff, "rah") == 0 )
	return KHE_SOLVER_RAH;
      else if( strcmp(buff, "rmc") == 0 )
	return KHE_SOLVER_RMC;
      else if( strcmp(buff, "rpk") == 0 )
	return KHE_SOLVER_RPK;
      else if( strcmp(buff, "rcx") == 0 )
	return KHE_SOLVER_RCX;
      else if( strcmp(buff, "rfs") == 0 )
	return KHE_SOLVER_RFS;
      else if( strcmp(buff, "rdv") == 0 )
	return KHE_SOLVER_RDV;
      else if( strcmp(buff, "rdt") == 0 )
	return KHE_SOLVER_RDT;
      else if( strcmp(buff, "rds") == 0 )
	return KHE_SOLVER_RDS;
      else if( strcmp(buff, "rts") == 0 )
	return KHE_SOLVER_RTS;
      else if( strcmp(buff, "rrm") == 0 )
	return KHE_SOLVER_RRM;
      else if( strcmp(buff, "rrh") == 0 )
	return KHE_SOLVER_RRH;
      else if( strcmp(buff, "rmu") == 0 )
	return KHE_SOLVER_RMU;
      else if( strcmp(buff, "rec") == 0 )
	return KHE_SOLVER_REC;
      else if( strcmp(buff, "rgl") == 0 )
	return KHE_SOLVER_RGL;
      else if( strcmp(buff, "rrs") == 0 )
	return KHE_SOLVER_RRS;
      else if( strcmp(buff, "rrp") == 0 )
	return KHE_SOLVER_RRP;
      break;
  }

  /* if we reach this point, it's an error */
  KheParserAbort(kp, "undefined identifier \"%s\"", buff);
  return KHE_SOLVER_RS;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSolverIdShow(KHE_SOLVER_ID id)                                  */
/*                                                                           */
/*  Return a string representation of id.                                    */
/*                                                                           */
/*****************************************************************************/

static char *KheSolverIdShow(KHE_SOLVER_ID id)
{
  switch( id )
  {
    /* general solvers */
    case KHE_SOLVER_DO:		return "do";
    case KHE_SOLVER_SKIP:	return "skip";
    case KHE_SOLVER_GDL:	return "gdl";
    case KHE_SOLVER_GTS:	return "gts";
    case KHE_SOLVER_GTI:	return "gti";
    case KHE_SOLVER_GEM:	return "gem";
    case KHE_SOLVER_GTP:	return "gtp";
    case KHE_SOLVER_GPU:	return "gpu";

    /* time solvers */
    case KHE_SOLVER_TS:		return "ts";
    case KHE_SOLVER_TCL:	return "tcl";
    case KHE_SOLVER_TBR:	return "tbr";
    case KHE_SOLVER_TRT:	return "trt";
    case KHE_SOLVER_TPA:	return "tpa";
    case KHE_SOLVER_TNP:	return "tnp";
    case KHE_SOLVER_TTP:	return "ttp";
    case KHE_SOLVER_TMD:	return "tmd";
    case KHE_SOLVER_TNL:	return "tnl";
    case KHE_SOLVER_TEC:	return "tec";
    case KHE_SOLVER_TNF:	return "tnf";
    case KHE_SOLVER_TDZ:	return "tdz";

    /* resource structural */
    case KHE_SOLVER_RS:		return "rs";
    case KHE_SOLVER_RT:		return "rt";
    case KHE_SOLVER_RTC:	return "rtc";
    case KHE_SOLVER_RRD:	return "rrd";
    case KHE_SOLVER_REM:	return "rem";
    case KHE_SOLVER_RWP:	return "rwp";
    case KHE_SOLVER_RSM:	return "rsm";
    case KHE_SOLVER_RCM:	return "rcm";
    case KHE_SOLVER_RBW:	return "rbw";
    case KHE_SOLVER_RGC:	return "rgc";
    case KHE_SOLVER_RGR:	return "rgr";
    case KHE_SOLVER_RED:	return "red";

    /* resource solvers */
    case KHE_SOLVER_RIN:	return "rin";
    case KHE_SOLVER_RRQ:	return "rrq";
    case KHE_SOLVER_RAH:	return "rah";
    case KHE_SOLVER_RMC:	return "rmc";
    case KHE_SOLVER_RPK:	return "rpk";
    case KHE_SOLVER_RCX:	return "rcx";
    case KHE_SOLVER_RFS:	return "rfs";
    case KHE_SOLVER_RDV:	return "rdv";
    case KHE_SOLVER_RDT:	return "rdt";
    case KHE_SOLVER_RDS:	return "rds";
    case KHE_SOLVER_RTS:	return "rts";
    case KHE_SOLVER_RRM:	return "rrm";
    case KHE_SOLVER_RRH:	return "rrh";
    case KHE_SOLVER_RMU:	return "rmu";
    case KHE_SOLVER_REC:	return "rec";
    case KHE_SOLVER_RGL:	return "rgl";
    case KHE_SOLVER_RRS:	return "rrs";
    case KHE_SOLVER_RRP:	return "rrp";
    default:			return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parsing"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER KheSolverMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Make and return a new solver.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER KheSolverMake(HA_ARENA a)
{
  KHE_SOLVER res;
  HaMake(res, a);
  HaArrayInit(res->items, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_ITEM KheSolverItemMake(int time_weight, KHE_SOLVER_ID id,     */
/*    KHE_SOLVER solver, KHE_TASKING rtc_tasking, HA_ARENA a)                */
/*                                                                           */
/*  Make and return a new solver item.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_ITEM KheSolverItemMake(int time_weight, KHE_SOLVER_ID id,
  KHE_SOLVER solver, KHE_TASKING rtc_tasking, HA_ARENA a)
{
  KHE_SOLVER_ITEM res;
  HaMake(res, a);
  res->time_weight = time_weight;
  res->id = id;
  res->solver = solver;
  res->rtc_tasking = rtc_tasking;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolverItemTimeWeightCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Comparison function for sorting solver items by increasing time weight.  */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static int KheSolverItemTimeWeightCmp(const void *t1, const void *t2)
{
  KHE_SOLVER_ITEM si1 = * (KHE_SOLVER_ITEM *) t1;
  KHE_SOLVER_ITEM si2 = * (KHE_SOLVER_ITEM *) t2;
  return si1->time_weight - si2->time_weight;
}

static bool KheParseSolver(char **str, KHE_SOLN soln, HA_ARENA a,
  KHE_SOLVER *res);
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheParserParseItem(KHE_PARSER kp, KHE_SOLVER_ITEM *res)             */
/*                                                                           */
/*  Parse one item and any following white space, according to grammar       */
/*                                                                           */
/*    <item>    ::=  [ <int> ":" ] <name> [ "(" <solver> ")" ]               */
/*    <name>    ::=  <id> [ "$" <id> ]                                       */
/*                                                                           */
/*****************************************************************************/
static void KheOldParserParseSolver(KHE_PARSER kp, KHE_SOLVER *res);

static void KheOldParserParseItem(KHE_PARSER kp, KHE_SOLVER_ITEM *res)
{
  int i, time_weight;  KHE_SOLVER_ID id1, id2, id;  KHE_TASKING rtc_tasking;
  KHE_RESOURCE_TYPE rt;  KHE_SOLVER solver, rdo_solver;  KHE_SOLVER_ITEM item;
  char buff[20];

  /* [ <int> : ] */
  if( is_digit(KheParserCurrCh(kp)) )
  {
    KheParserParseInt(kp, &time_weight);
    KheParserSkipChar(kp, ':');
  }
  else
    time_weight = 1;

  /* <id> [ "!" <id> ] */
  KheParserParseId(kp, buff);
  id1 = KheParserConvertIdToSolverId(kp, buff);
  if( KheParserCurrCh(kp) == '!' )
  {
    KheParserSkipChar(kp, '!');
    KheParserParseId(kp, buff);
    id2 = KheParserConvertIdToSolverId(kp, buff);
  }
  else
    id2 = id1;

  /* select id depending on model */
  switch( KheInstanceModel(KheSolnInstance(kp->soln)) )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE:

      id = id1;
      break;

    case KHE_MODEL_EMPLOYEE_SCHEDULE:

      id = id2;
      break;

    default:

      KheParserAbort(kp, "unknown model (%d)",
	KheInstanceModel(KheSolnInstance(kp->soln)));
      id = id1;  /* keep compiler happy */
  }

  /* [ "(" <solver> ")" ] */
  if( KheParserCurrCh(kp) == '(' )
  {
    KheParserSkipChar(kp, '(');
    KheOldParserParseSolver(kp, &solver);
    KheParserSkipChar(kp, ')');
  }
  else
    solver = NULL;

  /* create result and return true */
  if( id == KHE_SOLVER_RT )
  {
    /* convert rt(<solver>) into do(rtc(<solver>), ..., rtc(<solver>)) */
    rdo_solver = KheSolverMake(kp->arena);
    for( i = 0;  i < KheSolnTaskingCount(kp->soln);  i++ )
    {
      rtc_tasking = KheSolnTasking(kp->soln, i);
      rt = KheTaskingResourceType(rtc_tasking);
      if( KheResourceTypeResourceCount(rt) > 0 )
      {
	item = KheSolverItemMake(KheResourceTypeResourceCount(rt),
	  KHE_SOLVER_RTC, solver, rtc_tasking, kp->arena);
	HaArrayAddLast(rdo_solver->items, item);
      }
    }
    *res = KheSolverItemMake(time_weight, KHE_SOLVER_DO, rdo_solver, NULL,
      kp->arena);
  }
  else
    *res = KheSolverItemMake(time_weight, id, solver, NULL, kp->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOldParseSolver(char **str, HA_ARENA a, KHE_SOLVER *res)          */
/*                                                                           */
/*  Parse one <solver> and any following white space, according to grammar   */
/*                                                                           */
/*    <solver>  ::=  <item> { "," <item> }                                   */
/*                                                                           */
/*****************************************************************************/

static void KheOldParserParseSolver(KHE_PARSER kp, KHE_SOLVER *res)
{
  KHE_SOLVER_ITEM item;
  *res = KheSolverMake(kp->arena);
  /* KheParserSkipWhiteSpace(kp); */
  KheOldParserParseItem(kp, &item);
  HaArrayAddLast((*res)->items, item);
  while( KheParserCurrCh(kp) == ',' )
  {
    KheParserSkipChar(kp, ',');
    KheOldParserParseItem(kp, &item);
    HaArrayAddLast((*res)->items, item);
  }
}

/* ***
static bool KheParseSolver(char **str, KHE_SOLN soln, HA_ARENA a,
  KHE_SOLVER *res)
{
  KHE_SOLVER_ITEM item;
  *res = KheSolverMake(a);
  KheSkipWhiteSpace(str);
  if( !KheParseItem(str, soln, a, &item) )
    return false;
  HaArrayAddLast((*res)->items, item);
  while( **str == ',' )
  {
    KheSkipChar(str, ',');
    if( !KheParseItem(str, soln, a, &item) )
      return false;
    HaArrayAddLast((*res)->items, item);
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheRemainingTimeWeight(KHE_SOLVER solver, int i)                     */
/*                                                                           */
/*  Return the remaining time weight for items from i inclusive to the end.  */
/*                                                                           */
/*****************************************************************************/

static int KheRemainingTimeWeight(KHE_SOLVER solver, int i)
{
  int j, res;  KHE_SOLVER_ITEM item;
  res = 0;
  for( j = i;  j < HaArrayCount(solver->items);  j++ )
  {
    item = HaArray(solver->items, j);
    res += item->time_weight;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVER_STATS"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_STATS KheSolverStatsMake(KHE_SOLVER_ID solver_id, HA_ARENA a) */
/*                                                                           */
/*  Make a solver stats object.                                              */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_STATS KheSolverStatsMake(KHE_SOLVER_ID solver_id, HA_ARENA a)
{
  KHE_SOLVER_STATS res;
  HaMake(res, a);
  res->solver_id = solver_id;
  res->no_of_calls = 0;
  res->total_cost_drop = KheCost(0, 0);
  res->total_running_time = 0.0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverStatsAddStat(KHE_SOLVER_STATS ss,                          */
/*    KHE_COST cost_drop, float running_time)                                */
/*                                                                           */
/*  Add one statistic to ss.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheSolverStatsAddStat(KHE_SOLVER_STATS ss,
  KHE_COST cost_drop, float running_time)
{
  ss->no_of_calls++;
  ss->total_cost_drop += cost_drop;
  ss->total_running_time += running_time;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverStatsDebug(KHE_SOLVER_STATS ss, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of sss onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheSolverStatsDebug(KHE_SOLVER_STATS ss, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%-5s %5d %10.5f %10.1f", KheSolverIdShow(ss->solver_id),
    ss->no_of_calls, KheCostShow(ss->total_cost_drop), ss->total_running_time);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_STATS"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_STATS KheStatsMake(char *label, HA_ARENA a)                          */
/*                                                                           */
/*  Make a new stats object.                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_STATS KheStatsMake(char *label, HA_ARENA a)
{
  int i;  KHE_STATS res;
  HaMake(res, a);
  res->label = label;
  HaArrayInit(res->stats, a);
  for( i = 0;  i < KHE_SOLVER_COUNT;  i++ )
    HaArrayAddLast(res->stats, KheSolverStatsMake((KHE_SOLVER_ID) i, a));
  res->timer = KheTimerMake("yourself", KHE_NO_TIME, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheStatsAddStat(KHE_STATS stats, KHE_SOLVER_ID id,                  */
/*    KHE_COST cost_drop, float running_time)                                */
/*                                                                           */
/*  Add one statistic to stats.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheStatsAddStat(KHE_STATS stats, KHE_SOLVER_ID id,
  KHE_COST cost_drop, float running_time)
{
  KHE_SOLVER_STATS ss;
  ss = HaArray(stats->stats, id);
  KheSolverStatsAddStat(ss, cost_drop, running_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheStatsDebug(KHE_STATS stats, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of stats onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheStatsDebug(KHE_STATS stats, int verbosity, int indent, FILE *fp)
{
  KHE_SOLVER_STATS ss;  int i;  bool header_printed;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ DoItYourself %s:\n", indent, "", stats->label);
    header_printed = false;
    HaArrayForEach(stats->stats, ss, i)
      if( ss->solver_id >= KHE_SOLVER_TCL && ss->no_of_calls > 0 &&
	  (ss->total_cost_drop > 0 || ss->total_running_time >= 0.1) )
      {
	if( !header_printed )
	{
	  fprintf(fp, "%*s%-5s %5s %10s %10s\n", indent + 2, "", "Item",
	    "Calls", "Cost", "Secs");
	  header_printed = true;
	}
	KheSolverStatsDebug(ss, verbosity, indent + 2, fp);
      }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "running"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverItemRun(KHE_SOLVER_ITEM item, KHE_RESOURCE_TYPE rt,        */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)    */
/*                                                                           */
/*  Run item.                                                                */
/*                                                                           */
/*****************************************************************************/
static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, int depth, KHE_STATS stats, KHE_COST *cost_drop,
  float *running_time);
static void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp);

static bool KheSolverItemRun(KHE_SOLVER_ITEM item, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, int depth, KHE_STATS stats)
{
  bool res;  KHE_TASK_SET task_set, ts;  char *rs_multiplier;
  char mult_str[101];  int mult_val, i;  HA_ARENA a;  KHE_SOLN_ADJUSTER sa;
  KHE_CLUSTER_MINIMUM_SOLVER cms;  KHE_MEET_BOUND_GROUP cluster_mbg;
  /* KHE_GROUP_MONITOR low_cost_gm; */  KHE_TASK_BOUND_GROUP tighten_tbg;
  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_INSTANCE ins;
  float time_before, time_after, inner_running_time;
  KHE_COST cost_before, cost_after, inner_cost_drop;
  KHE_BALANCE_SOLVER bs;
  inner_cost_drop = 0;
  inner_running_time = 0.0;
  time_before = KheTimerElapsedTime(stats->timer);
  cost_before = KheSolnCost(soln);
  switch( item->id )
  {
    /***********************************/
    /*                                 */
    /* documented for general solvers  */
    /*                                 */
    /***********************************/

    case KHE_SOLVER_DO:

      /* do */
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      break;

    case KHE_SOLVER_SKIP:

      /* skip */
      res = false;
      break;

    case KHE_SOLVER_TS:

      /* KheCycleNodeAssignTimes (redoing this still to do) */
      res = KheCombinedTimeAssign(cycle_node, options);
      break;

    case KHE_SOLVER_RS:

      /* KheCombinedResourceAssign */
      res = KheCombinedResourceAssign(soln, options);
      break;

    case KHE_SOLVER_GDL:

      /* KheDetachLowCostMonitors */
      sa = KheSolnAdjusterMake(soln);
      KheDetachLowCostMonitors(sa, KheCost(1, 0));
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterDelete(sa);
      /* ***
      low_cost_gm = KheGroupMonitorMake(soln, 55, "low-cost");
      KheDetachLowCostMonitors(soln, KheCost(1, 0), low_cost_gm);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheAttachLowCostMonitors(low_cost_gm);
      *** */
      break;

    case KHE_SOLVER_GTS:

      /* KheSolnMatchingBegin */
      HnAssert(!KheSolnHasMatching(soln),
        "KheSolverItemRun: gts called where matching already present");
      KheMatchingBegin(soln, false);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheMatchingEnd(soln);
      break;

    case KHE_SOLVER_GTI:

      /* KheSolnMatchingBegin */
      HnAssert(!KheSolnHasMatching(soln),
        "KheSolverItemRun: gti called where matching already present");
      KheMatchingBegin(soln, true);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheMatchingEnd(soln);
      break;

    case KHE_SOLVER_GEM:

      /* KheSolnEvennessBegin */
      KheSolnEvennessBegin(soln);
      KheSolnSetAllEvennessMonitorWeights(soln, KheCost(0, 5));
      KheSolnAttachAllEvennessMonitors(soln);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnEvennessEnd(soln);
      break;

    case KHE_SOLVER_GTP:

      /* KheTiltPlateau */
      sa = KheSolnAdjusterMake(soln);
      KheTiltPlateau(sa);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterDelete(sa);
      break;

    case KHE_SOLVER_GPU:

      /* KhePropagateUnavailableTimes */
      res = KhePropagateUnavailableTimes(soln, NULL);
      break;


    /***********************************/
    /*                                 */
    /* documented for time solvers     */
    /*                                 */
    /***********************************/

    case KHE_SOLVER_TCL:

      /* KheCoordinateLayers */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tcl given no cycle node");
      KheCoordinateLayers(cycle_node, true);
      res = true;
      break;

    case KHE_SOLVER_TBR:

      /* KheBuildRunarounds */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tbr given no cycle node");
      KheBuildRunarounds(cycle_node, &KheNodeSimpleAssignTimes,
	options, &KheRunaroundNodeAssignTimes, options);
      res = true;
      break;

    case KHE_SOLVER_TRT:

      /* KheNodeRecursiveAssignTimes */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: trt given no cycle node");
      for( i = 0;  i < KheNodeChildCount(cycle_node);  i++ )
	KheNodeRecursiveAssignTimes(KheNodeChild(cycle_node, i),
	  &KheRunaroundNodeAssignTimes, options);
      res = false;
      break;

    case KHE_SOLVER_TPA:

      /* KheNodePreassignedAssignTimes */
      res = KheNodePreassignedAssignTimes(cycle_node, options);
      break;

    case KHE_SOLVER_TNP:

      /* if !KheInstanceAllEventsHavePreassignedTimes */
      if( !KheInstanceAllEventsHavePreassignedTimes(KheSolnInstance(soln)) )
	res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	  options, depth, stats, &inner_cost_drop, &inner_running_time);
      else
	res = true;
      break;

    case KHE_SOLVER_TTP:

      /* KheTaskingTightenToPartition */
      tighten_tbg = KheTaskBoundGroupMake(soln);
      for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
	KheTaskingTightenToPartition(KheSolnTasking(soln, i),
	  tighten_tbg, options);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheTaskBoundGroupDelete(tighten_tbg);
      break;

    case KHE_SOLVER_TMD:

      /* KheSolnClusterAndLimitMeetDomains */
      cluster_mbg = KheMeetBoundGroupMake(soln);
      KheSolnAddUnavailableBounds(soln, KheCost(0, 0), cluster_mbg);
      KheSolnClusterAndLimitMeetDomains(soln, 0, KheCost(1, 0), 1.8,
	cluster_mbg, options);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheMeetBoundGroupDelete(cluster_mbg);
      break;

    case KHE_SOLVER_TNL:

      /* KheNodeLayeredAssignTimes */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tnl given no cycle node");
      res = KheNodeLayeredAssignTimes(cycle_node, options);
      break;

    case KHE_SOLVER_TEC:

      HnAssert(cycle_node != NULL, "KheSolverItemRun: tec given no cycle node");
      res = KheEjectionChainNodeRepairTimes(cycle_node, options);
      break;

    case KHE_SOLVER_TNF:

      /* KheNodeFlatten */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tnf given no cycle node");
      KheNodeFlatten(cycle_node);
      res = true;
      break;

    case KHE_SOLVER_TDZ:

      /* KheNodeDeleteZones */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tdz given no cycle node");
      KheNodeDeleteZones(cycle_node);
      res = true;
      break;


    /***********************************/
    /*                                 */
    /* documented for resource struct  */
    /*                                 */
    /***********************************/

    case KHE_SOLVER_RT:

      /* resource type (should have been compiled away) */
      HnAbort("KheSolverItemRun: unexpected type KHE_SOLVER_RT");
      res = false;
      break;

    case KHE_SOLVER_RTC:

      /* resource type */
      tasking = item->rtc_tasking;
      rt = KheTaskingResourceType(tasking);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      break;

    case KHE_SOLVER_RRD:

      a = KheSolnArenaBegin(soln);
      bs = KheBalanceSolverMake(soln, options, rt, a);
      if( KheBalanceSolverTotalSupply(bs) <= KheBalanceSolverTotalDemand(bs) )
      {
	KheSolnRedundancyBegin(soln,rt,KHE_LINEAR_COST_FUNCTION, KheCost(0, 1));
	res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	  options, depth, stats, &inner_cost_drop, &inner_running_time);
	KheSolnRedundancyEnd(soln);
      }
      else
	res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	  options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_REM:

      /* event timetable monitor */
      ins = KheSolnInstance(soln);
      etm = KheEventTimetableMonitorMake(soln, KheInstanceFullEventGroup(ins));
      KheMonitorAttachToSoln((KHE_MONITOR) etm);
      KheOptionsSetObject(options, "gs_event_timetable_monitor", (void *) etm);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      HnAssert(KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL)
	== (void *) etm, "KheSolverItemRun internal error (etm)");
      KheOptionsSetObject(options, "gs_event_timetable_monitor", NULL);
      break;

    case KHE_SOLVER_RWP:

      /* KheWorkloadPack */
      HnAssert(rt != NULL, "rwp item in rs option is outside rt()");
      KheWorkloadPack(soln, options, rt, &tighten_tbg);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      if( tighten_tbg != NULL )
	KheTaskBoundGroupDelete(tighten_tbg);
      break;

    case KHE_SOLVER_RSM:

      /* KheSetMonitorMultipliers */
      rs_multiplier = KheOptionsGet(options, "rs_multiplier", NULL);
      HnAssert(rs_multiplier != NULL, "rsm item in rs option has no "
	"corresponding rs_multiplier option");
      if( sscanf(rs_multiplier, "%d:%100s", &mult_val, mult_str) != 2 )
	HnAbort("syntax error in rs_multiplier value \"%s\"", rs_multiplier);
      sa = KheSolnAdjusterMake(soln);
      KheSetClusterMonitorMultipliers(sa, mult_str, mult_val);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterDelete(sa);
      /* ***
      KheSetClusterMonitorMultipliers(soln, mult_str, mult_val);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheResetClusterMonitorMultipliers(soln, mult_str);
      *** */
      break;

    case KHE_SOLVER_RCM:

      /* KheClusterMinimumSolverSolve */
      HnAssert(rt != NULL, "rcm item in rs option is outside rt()");
      a = KheSolnArenaBegin(soln);
      cms = KheClusterMinimumSolverMake(a);
      KheClusterMinimumSolverSolve(cms, soln, options, rt);
      KheClusterMinimumSolverSetBegin(cms);
      KheClusterMinimumSolverSetMulti(cms,
	KheResourceTypeFullResourceGroup(rt));
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheClusterMinimumSolverSetEnd(cms, true);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RBW:

      /* KheBalanceWeekends */
      sa = KheSolnAdjusterMake(soln);
      KheBalanceWeekends(sa, options, rt);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterDelete(sa);
      break;

    case KHE_SOLVER_RGC:

      /* KheGroupByResourceConstraints */
      ts = KheTaskSetMake(soln);
      KheGroupByResourceConstraints(soln, rt, options, ts);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheTaskSetUnGroup(ts);
      KheTaskSetDelete(ts);
      break;

    case KHE_SOLVER_RGR:

      /* KheTaskingGroupByResource */
      task_set = KheTaskSetMake(soln);
      if( KheTaskingGroupByResource(tasking, options, task_set) )
      {
	res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	  options, depth, stats, &inner_cost_drop, &inner_running_time);
	/* KheTaskingRepairResources(tasking, options, 0.5, 1); */
	KheTaskSetUnGroup(task_set);
	KheTaskSetDelete(task_set);
      }
      else
	res = false;
      break;

    case KHE_SOLVER_RED:

      /* KheTaskingEnlargeDomains */
      KheTaskingEnlargeDomains(tasking, false);
      res = true;
      break;


    /***********************************/
    /*                                 */
    /* documented for resource solvers */
    /*                                 */
    /***********************************/

    case KHE_SOLVER_RIN:

      /* resource invariant */
      KheOptionsSetBool(options, "rs_invariant", true);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheOptionsSetBool(options, "rs_invariant", false);
      break;

    case KHE_SOLVER_RRQ:

      /* KheSolnAssignRequestedResources */
      res = KheSolnAssignRequestedResources(soln, rt, options);
      break;

    case KHE_SOLVER_RAH:

      /* KheAssignByHistory */
      sa = KheSolnAdjusterMake(soln);
      res = KheAssignByHistory(soln, rt, options, sa);
      res = KheSolverRun(item->solver, rt, tasking, soln, cycle_node,
	options, depth, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterDelete(sa);
      break;

    case KHE_SOLVER_RMC:

      /* KheMostConstrainedFirstAssignResources */
      res = KheMostConstrainedFirstAssignResources(tasking, options);
      break;

    case KHE_SOLVER_RPK:

      /* KheResourcePackAssignResources */
      res = KheResourcePackAssignResources(tasking, options);
      break;

    case KHE_SOLVER_RCX:

      /* resource assignment choice */
      if( rt != NULL && KheResourceTypeAvoidSplitAssignmentsCount(rt)==0 )
	res = KheMostConstrainedFirstAssignResources(tasking, options);
      else
	res = KheResourcePackAssignResources(tasking, options);
      break;

    case KHE_SOLVER_RFS:

      /* KheFindSplitResourceAssignments */
      if( KheResourceTypeAvoidSplitAssignmentsCount(rt) > 0 )
      {
	res = KheFindSplitResourceAssignments(tasking, options);
	KheTaskingAllowSplitAssignments(tasking, false);
      }
      else
	res = true;
      break;

    case KHE_SOLVER_RDV:

      /* KheDynamicResourceVLSNSolve */
      res = KheDynamicResourceVLSNSolve(soln, rt, options);
      break;

    case KHE_SOLVER_RDT:

      /* KheDynamicResourceVLSNTest */
      KheDynamicResourceVLSNTest(soln, rt, options);
      res = false;
      break;

    case KHE_SOLVER_RDS:

      /* KheDynamicResourceSequentialSolve */
      if( DEBUG4 )
      {
	fprintf(stderr, "[ before KheDynamicResourceSequentialSolve:\n");
	KheGroupMonitorDebug((KHE_GROUP_MONITOR) soln, 2, 2, stderr);
      }
      res = KheDynamicResourceSequentialSolve(soln, rt, options);
      if( DEBUG4 )
      {
	fprintf(stderr, "  after KheDynamicResourceSequentialSolve:\n");
	KheGroupMonitorDebug((KHE_GROUP_MONITOR) soln, 2, 2, stderr);
	fprintf(stderr, "]\n");
      }
      break;

    case KHE_SOLVER_RTS:

      /* KheTimeSweepAssignResources */
      res = KheTimeSweepAssignResources(soln,
	KheResourceTypeFullResourceGroup(rt), options);
      break;

    case KHE_SOLVER_RRM:

      /* KheResourceRematch */
      res = KheResourceRematch(soln, KheResourceTypeFullResourceGroup(rt),
        options, 0);
      break;

    case KHE_SOLVER_RRH:

      /* KheRunHomogenize */
      KheRunHomogenize(soln, rt, options);
      res = true;
      break;

    case KHE_SOLVER_RMU:

      /* KheMoveUnnecessaryAssignments */
      KheMoveUnnecessaryAssignments(soln, rt, options);
      res = true;
      break;

    case KHE_SOLVER_REC:

      /* KheEjectionChainRepairResources */
      res = KheEjectionChainRepairResources(tasking, options);
      break;

    case KHE_SOLVER_RGL:

      /* KheGlobalLoadBalance */
      res = KheGlobalLoadBalance(soln, rt, options);
      break;

    case KHE_SOLVER_RRS:

      /* KheResourcePairRepair */
      res = KheResourcePairRepair(tasking, options);
      break;

    case KHE_SOLVER_RRP:

      /* KheReassign1Repair */
      res = KheReassign1Repair(soln, rt, options);
      break;

    default:

      HnAbort("KheSolverItemRun: internal error (id %d)", item->id);
      res = false;
      break;
  }
  time_after = KheTimerElapsedTime(stats->timer);
  cost_after = KheSolnCost(soln);
  KheStatsAddStat(stats, item->id, cost_before - cost_after - inner_cost_drop,
    time_after - time_before - inner_running_time);
  if( DEBUG3 )
  {
    /* ***
    fprintf(stderr, "[queries %6d, cost_after %10.5f] ",
      KheOptionsTimeLimitReachedQueryCount(options), KheCostShow(cost_after));
    fprintf(stderr, "[queries %6d, time %6.1f, cost_after %10.5f] ",
      KheOptionsTimeLimitReachedQueryCount(options), time_after,
      KheCostShow(cost_after));
    *** */
    KheSolverItemDebug(item, stderr);
    fprintf(stderr, "\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverRun(KHE_SOLVER solver, KHE_RESOURCE_TYPE rt,               */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)    */
/*                                                                           */
/*  Run solver on these attributes.                                          */
/*                                                                           */
/*****************************************************************************/
static void KheSolverDebug(KHE_SOLVER solver, FILE *fp);

static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, int depth, KHE_STATS stats, KHE_COST *cost_drop,
  float *running_time)
{
  KHE_SOLVER_ITEM item;  int i, rem_time_weight;  float rem_time, avail_time;
  char buff[20], buff2[20];  bool with_timer, res;  KHE_TIMER timer;
  KHE_COST cost, cost_before, cost_after;  float time_before, time_after;

  /* quit now if there is no solver */
  if( s == NULL )
  {
    *cost_drop = 0;
    *running_time = 0.0;
    return false;
  }

  /* decide whether a timer is needed or not */
  with_timer = (KheOptionsRemainingTime(options) != KHE_NO_TIME &&
    HaArrayCount(s->items) >= 2);

  /* run each item with a non-zero time weight */
  cost_before = KheSolnCost(soln);
  time_before = KheTimerElapsedTime(stats->timer);
  timer = NULL;  avail_time = -1.0;  /* these two to keep compiler happy */
  res = false;
  HaArrayForEach(s->items, item, i)
    if( item->time_weight > 0 )
    {
      if( with_timer )
      {
	rem_time = KheOptionsRemainingTime(options);
	if( rem_time == 0.0 )
	  break;
	rem_time_weight = KheRemainingTimeWeight(s, i);
	avail_time = (rem_time * item->time_weight) / rem_time_weight;
	snprintf(buff, 20, "diy%d_%s", depth, KheSolverIdShow(item->id));
	timer = KheOptionsAddTimer(options, buff, avail_time);
      }
      if( DEBUG1 )
      {
	fprintf(stderr, "%*s[ ", depth * 2, "");
        KheSolverItemDebug(item, stderr);
	if( with_timer )
	  fprintf(stderr, ", %s available", KheTimeShow(avail_time, buff2));
	fprintf(stderr, "\n");
      }
      cost = KheSolnCost(soln);
      if( KheSolverItemRun(item, rt, tasking, soln, cycle_node, options,
	    depth + 1, stats) )
	res = true;
      if( DEBUG1 )
      {
	fprintf(stderr, "%*s] %s end", depth*2, "", KheSolverIdShow(item->id));
	if( with_timer )
	  fprintf(stderr, ", %s used",
	    KheTimeShow(KheTimerElapsedTime(timer), buff2));
	if( KheSolnCost(soln) < cost && item->id >= KHE_SOLVER_RRQ )
	  fprintf(stderr, ", improved (%.5f < %.5f)",
	    KheCostShow(KheSolnCost(soln)), KheCostShow(cost));
	fprintf(stderr, "\n");
      }
      if( with_timer )
	KheOptionsDeleteTimer(options, timer);
    }
  cost_after = KheSolnCost(soln);
  time_after = KheTimerElapsedTime(stats->timer);
  *cost_drop = cost_before - cost_after;
  *running_time = time_after - time_before;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)                  */
/*                                                                           */
/*  Debug print of item onto fp, in the same format as it was read in.       */
/*                                                                           */
/*****************************************************************************/

static void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)
{
  KHE_RESOURCE_TYPE rt;
  if( item->time_weight != 1 )
    fprintf(fp, "%d: ", item->time_weight);
  fprintf(fp, "%s", KheSolverIdShow(item->id));
  if( item->rtc_tasking != NULL )
  {
    rt = KheTaskingResourceType(item->rtc_tasking);
    fprintf(fp, " %s", KheResourceTypeId(rt));
  }
  if( item->solver != NULL )
  {
    fprintf(fp, "(");
    KheSolverDebug(item->solver, fp);
    fprintf(fp, ")");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverDebug(KHE_SOLVER solver, FILE *fp)                         */
/*                                                                           */
/*  Debug print of solver onto fp, in the same format as it was read in.     */
/*                                                                           */
/*****************************************************************************/

static void KheSolverDebug(KHE_SOLVER solver, FILE *fp)
{
  KHE_SOLVER_ITEM item;  int i;
  HaArrayForEach(solver->items, item, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheSolverItemDebug(item, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoItYourselfSolverParseAndRun(KHE_SOLN soln, KHE_NODE cycle_node,*/
/*    KHE_OPTIONS options, char *solver_option_name, char *solver_option_val)*/
/*                                                                           */
/*  Parse solver_option_val (the value of option solver_option_name) and     */
/*  run the solver it describes.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheDoItYourselfSolverParseAndRun(KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, char *solver_option_name, char *solver_option_val)
{
  KHE_SOLVER solver;  HA_ARENA a;  bool res;  KHE_PARSER kp;
  KHE_STATS stats;  KHE_COST cost_drop;  float running_time;

  /* parse */
  a = KheSolnArenaBegin(soln);
  kp = KheParserMake(soln, solver_option_name, solver_option_val, a);
  KheOldParserParseSolver(kp, &solver);
  if( KheParserCurrCh(kp) != '\0' )
    KheParserAbort(kp, "unexpected character '%c'", KheParserCurrCh(kp));
  /* ***
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombinedResourceAssign(soln of %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)));
    fprintf(stderr, "  rs=\"");
    KheSolverDebug(solver, stderr);
    fprintf(stderr, "\"\n");
    fprintf(stderr, "  rs_time_ limit=\"%s\"\n", str);
  }
  *** */

  /* run */
  stats = KheStatsMake(solver_option_name, a);
  res = KheSolverRun(solver, NULL, NULL, soln, cycle_node, options, 1, stats,
    &cost_drop, &running_time);
  if( DEBUG2 )
    KheStatsDebug(stats, 2, 0, stderr);
  KheSolnArenaEnd(soln, a);
  return res;
}

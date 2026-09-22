
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

#define NO_TIME_WEIGHT -1

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0		/* KheSolverRun */
#define DEBUG2	0		/* stats */
#define DEBUG3	0		/* logging of solve items */
#define DEBUG4	0		/* checking Sequential */
#define DEBUG5	1		/* checking for unassigned preassigned meets */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "types"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_PARSER                                                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_parser_rec {
  HA_ARENA		arena;
  KHE_SOLN		soln;
  char			*name;
  char			*val;
  int			pos;
} *KHE_PARSER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVER_ID                                                       */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* documented for general solvers */
  KHE_SOLVER_DO,
  KHE_SOLVER_SKIP,
  KHE_SOLVER_EMPTY,
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
  KHE_SOLVER_RCG,
  KHE_SOLVER_RWG,
  KHE_SOLVER_RIG,
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
  KHE_SOLVER_RDW,
  KHE_SOLVER_RTS,
  KHE_SOLVER_RRM,
  KHE_SOLVER_RRH,
  KHE_SOLVER_RMU,
  KHE_SOLVER_REC,
  KHE_SOLVER_RE2,
  KHE_SOLVER_RGL,
  KHE_SOLVER_RRS,
  KHE_SOLVER_RRP,

  /* this value just counts the number of elements */
  KHE_SOLVER_COUNT
} KHE_SOLVER_ID;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVER_ITEM                                                     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_solver_rec *KHE_SOLVER;

typedef struct khe_solver_item_rec {
  int			time_weight;
  KHE_SOLVER		solver;
} *KHE_SOLVER_ITEM;

typedef HA_ARRAY(KHE_SOLVER_ITEM) ARRAY_KHE_SOLVER_ITEM;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVER_MULTIPLE                                                 */
/*                                                                           */
/*****************************************************************************/

#define INHERIT_SOLVER						\
  bool			single;

typedef struct khe_solver_multiple_rec {
  INHERIT_SOLVER
  bool			is_derived_from_rt;
  ARRAY_KHE_SOLVER_ITEM	items;
} *KHE_SOLVER_MULTIPLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVER_SINGLE                                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_solver_single_rec {
  INHERIT_SOLVER
  KHE_SOLVER_ID		id;
  /* KHE_SOLN		soln; */	/* used only by RTC type */
  KHE_RESOURCE_TYPE	resource_type;	/* used only by RTC type */
  /* KHE_TASKING	rtc_tasking; */	/* used only by RTC type */
  KHE_SOLVER		solver;
} *KHE_SOLVER_SINGLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVER                                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_solver_rec {
  INHERIT_SOLVER
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_STATS                                                           */
/*                                                                           */
/*****************************************************************************/

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
/*  void KheParserParseId(KHE_PARSER kp, char buff[20])                      */
/*                                                                           */
/*  Parse an identifier and any following white space.                       */
/*                                                                           */
/*****************************************************************************/
#define is_letter(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

static void KheParserParseId(KHE_PARSER kp, char buff[20])
{
  int i;  char ch;
  i = 0;
  ch = KheParserCurrCh(kp);
  if( !is_letter(ch) )
    KheParserAbort(kp, "identifier expected but found '%c'", ch);
  while( (is_letter(ch) || is_digit(ch)) && i < 20 )
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
/*  Submodule "KHE_SOLVER_ID"                                                */
/*                                                                           */
/*****************************************************************************/

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
    case 'e':
    case 'd':
    case 's':
    case 'g':

      /* general solvers */
      if( strcmp(buff, "do") == 0 )
	return KHE_SOLVER_DO;
      else if( strcmp(buff, "skip") == 0 )
	return KHE_SOLVER_SKIP;
      else if( strcmp(buff, "empty") == 0 )
	return KHE_SOLVER_EMPTY;
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
      else if( strcmp(buff, "rcg") == 0 )
	return KHE_SOLVER_RCG;
      else if( strcmp(buff, "rwg") == 0 )
	return KHE_SOLVER_RWG;
      else if( strcmp(buff, "rig") == 0 )
	return KHE_SOLVER_RIG;
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
      else if( strcmp(buff, "rdw") == 0 )
	return KHE_SOLVER_RDW;
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
      else if( strcmp(buff, "re2") == 0 )
	return KHE_SOLVER_RE2;
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
/*  bool KheSolverIdIsEnclosing(KHE_SOLVER_ID id)                            */
/*                                                                           */
/*  Return true if Id represents an enclosing solver.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheSolverIdIsEnclosing(KHE_SOLVER_ID id)
{
  switch( id )
  {
    /* general solvers */
    case KHE_SOLVER_DO:		return true;
    case KHE_SOLVER_SKIP:	return true;
    case KHE_SOLVER_EMPTY:	return false;
    case KHE_SOLVER_GDL:	return true;
    case KHE_SOLVER_GTS:	return true;
    case KHE_SOLVER_GTI:	return true;
    case KHE_SOLVER_GEM:	return true;
    case KHE_SOLVER_GTP:	return true;
    case KHE_SOLVER_GPU:	return false;

    /* time solvers */
    case KHE_SOLVER_TS:		return false;
    case KHE_SOLVER_TCL:	return false;
    case KHE_SOLVER_TBR:	return false;
    case KHE_SOLVER_TRT:	return false;
    case KHE_SOLVER_TPA:	return false;
    case KHE_SOLVER_TNP:	return true;
    case KHE_SOLVER_TTP:	return true;
    case KHE_SOLVER_TMD:	return true;
    case KHE_SOLVER_TNL:	return false;
    case KHE_SOLVER_TEC:	return false;
    case KHE_SOLVER_TNF:	return false;
    case KHE_SOLVER_TDZ:	return false;

    /* resource structural */
    case KHE_SOLVER_RS:		return false;
    case KHE_SOLVER_RT:		return true;
    case KHE_SOLVER_RTC:	return true;
    case KHE_SOLVER_RRD:	return true;
    case KHE_SOLVER_REM:	return true;
    case KHE_SOLVER_RWP:	return true;
    case KHE_SOLVER_RSM:	return true;
    case KHE_SOLVER_RCM:	return true;
    case KHE_SOLVER_RCG:	return true;
    case KHE_SOLVER_RWG:	return true;
    case KHE_SOLVER_RIG:	return true;
    case KHE_SOLVER_RGR:	return true;
    case KHE_SOLVER_RED:	return false;

    /* resource solvers */
    case KHE_SOLVER_RIN:	return true;
    case KHE_SOLVER_RRQ:	return false;
    case KHE_SOLVER_RAH:	return true;
    case KHE_SOLVER_RMC:	return false;
    case KHE_SOLVER_RPK:	return false;
    case KHE_SOLVER_RCX:	return false;
    case KHE_SOLVER_RFS:	return false;
    case KHE_SOLVER_RDV:	return false;
    case KHE_SOLVER_RDT:	return false;
    case KHE_SOLVER_RDS:	return false;
    case KHE_SOLVER_RDW:	return false;
    case KHE_SOLVER_RTS:	return false;
    case KHE_SOLVER_RRM:	return false;
    case KHE_SOLVER_RRH:	return false;
    case KHE_SOLVER_RMU:	return false;
    case KHE_SOLVER_REC:	return false;
    case KHE_SOLVER_RE2:	return false;
    case KHE_SOLVER_RGL:	return false;
    case KHE_SOLVER_RRS:	return false;
    case KHE_SOLVER_RRP:	return false;
    default:			return "??";
  }
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
    case KHE_SOLVER_EMPTY:	return "empty";
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
    case KHE_SOLVER_RCG:	return "rcg";
    case KHE_SOLVER_RWG:	return "rwg";
    case KHE_SOLVER_RIG:	return "rig";
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
    case KHE_SOLVER_RDW:	return "rdw";
    case KHE_SOLVER_RTS:	return "rts";
    case KHE_SOLVER_RRM:	return "rrm";
    case KHE_SOLVER_RRH:	return "rrh";
    case KHE_SOLVER_RMU:	return "rmu";
    case KHE_SOLVER_REC:	return "rec";
    case KHE_SOLVER_RE2:	return "re2";
    case KHE_SOLVER_RGL:	return "rgl";
    case KHE_SOLVER_RRS:	return "rrs";
    case KHE_SOLVER_RRP:	return "rrp";
    default:			return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVER_ITEM"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_ITEM KheSolverItemMake(int time_weight, KHE_SOLVER_ID id,     */
/*    KHE_SOLVER solver, KHE_TASKING rtc_tasking, HA_ARENA a)                */
/*                                                                           */
/*  Make and return a new solver item.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_ITEM KheSolverItemMake(int time_weight,
  KHE_SOLVER solver, HA_ARENA a)
{
  KHE_SOLVER_ITEM res;
  HaMake(res, a);
  res->time_weight = time_weight;
  res->solver = solver;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverItemParse(KHE_PARSER kp, KHE_SOLVER_ITEM *res)             */
/*                                                                           */
/*  Parse one item and any following white space, according to grammar       */
/*                                                                           */
/*    <item>    ::=  [ <int> ":" ] <solver>                                  */
/*                                                                           */
/*****************************************************************************/
static void KheSolverParse(KHE_PARSER kp, KHE_SOLVER *res);

static void KheSolverItemParse(KHE_PARSER kp, KHE_SOLVER_ITEM *res)
{
  int time_weight;  KHE_SOLVER solver;

  /* [ <time_weight> : ] */
  if( KheParserCurrCh(kp) == '*' )
  {
    KheParserSkipChar(kp, '*');
    KheParserSkipChar(kp, ':');
    time_weight = NO_TIME_WEIGHT;
  }
  else if( is_digit(KheParserCurrCh(kp)) )
  {
    KheParserParseInt(kp, &time_weight);
    KheParserSkipChar(kp, ':');
  }
  else
    time_weight = 1;

  /* <solver> */
  KheSolverParse(kp, &solver);

  /* make result */
  *res = KheSolverItemMake(time_weight, solver, kp->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)                  */
/*                                                                           */
/*  Debug print of item onto fp, in the same format as it was read in.       */
/*                                                                           */
/*****************************************************************************/
static void KheSolverDebug(KHE_SOLVER solver, FILE *fp);

static void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)
{
  if( item->time_weight == NO_TIME_WEIGHT )
    fprintf(fp, "*: ");
  else if( item->time_weight != 1 )
    fprintf(fp, "%d: ", item->time_weight);
  KheSolverDebug(item->solver, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVER_MULTIPLE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_MULTIPLE KheSolverMultipleMake(bool is_derived_from_rt,       */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make and return a new multiple solver with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_MULTIPLE KheSolverMultipleMake(bool is_derived_from_rt,
  HA_ARENA a)
{
  KHE_SOLVER_MULTIPLE res;
  HaMake(res, a);
  res->single = false;
  res->is_derived_from_rt = is_derived_from_rt;
  HaArrayInit(res->items, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverMultipleParse(KHE_PARSER kp, KHE_SOLVER *res)              */
/*                                                                           */
/*  Parse one multiple solver according to grammar                           */
/*                                                                           */
/*    <solver>  ::=  "(" <item> "," <item> { "," <item> } ")"                */
/*    <item>    ::=  [ <int> ":" ] <solver>                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSolverMultipleParse(KHE_PARSER kp, KHE_SOLVER *res)
{
  KHE_SOLVER_ITEM item;  KHE_SOLVER_MULTIPLE sm;

  /* "(" <item> "," <item> { "," <item> } ")" */
  sm = KheSolverMultipleMake(false, kp->arena);
  KheParserSkipChar(kp, '(');
  KheSolverItemParse(kp, &item);
  HaArrayAddLast(sm->items, item);
  do
  {
    KheParserSkipChar(kp, ',');
    KheSolverItemParse(kp, &item);
    HaArrayAddLast(sm->items, item);
  } while( KheParserCurrCh(kp) == ',' );
  KheParserSkipChar(kp, ')');
  *res = (KHE_SOLVER) sm;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRemainingTimeWeight(KHE_SOLVER_MULTIPLE sm, int i)                */
/*                                                                           */
/*  Return the remaining time weight for items from i inclusive to the end.  */
/*                                                                           */
/*****************************************************************************/

static int KheRemainingTimeWeight(KHE_SOLVER_MULTIPLE sm, int i)
{
  int j, res;  KHE_SOLVER_ITEM item;
  res = 0;
  for( j = i;  j < HaArrayCount(sm->items);  j++ )
  {
    item = HaArray(sm->items, j);
    if( item->time_weight != NO_TIME_WEIGHT )
      res += item->time_weight;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverMultipleRun(KHE_SOLVER_MULTIPLE sm,                        */
/*    KHE_RESOURCE_TYPE rt, KHE_TASKING tasking, KHE_SOLN soln,              */
/*    KHE_NODE cycle_node, KHE_OPTIONS options, int depth,                   */
/*    KHE_STATS stats, KHE_COST *cost_drop, float *running_time)             */
/*                                                                           */
/*  Run sm on these attributes.  Set *cost_drop to the change in             */
/*  cost (before - after), and set *running_time to the running time.        */
/*                                                                           */
/*****************************************************************************/
static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt, KHE_SOLN soln,
  KHE_NODE cycle_node, KHE_OPTIONS options, int depth,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_STATS stats,
  KHE_COST *cost_drop, float *running_time);
static void KheSolverMultipleDebug(KHE_SOLVER_MULTIPLE sm, FILE *fp);

static bool KheSolverMultipleRun(KHE_SOLVER_MULTIPLE sm,
  KHE_RESOURCE_TYPE rt, KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, int depth, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_STATS stats, KHE_COST *cost_drop, float *running_time)
{
  KHE_SOLVER_ITEM item;  int i, rem_time_weight;  float rem_time, avail_time;
  char buff[20];  bool res;  KHE_TIMER item_timer;
  KHE_COST cost_before, cost_after;  float time_before, time_after;

  /* work out cost and running time at the start */
  cost_before = KheSolnCost(soln);
  time_before = KheTimerElapsedTime(stats->timer);

  /* run each item */
  res = false;
  HaArrayForEach(sm->items, item, i)
    if( item->time_weight == NO_TIME_WEIGHT )
    {
      /* weight is *:  ignore time limits and run the solver */
      if( KheSolverRun(item->solver, rt, soln, cycle_node, options,
	    depth + 1, tgdf, stats, cost_drop, running_time) )
	res = true;

    }
    else if( item->time_weight == 0 )
    {
      /* weight is zero:  skip this solver */
    }
    else if( (rem_time = KheOptionsRemainingTime(options)) == KHE_NO_TIME )
    {
      /* no time limit:  run the solver with no time limit */
      if( KheSolverRun(item->solver, rt, soln, cycle_node, options,
	    depth + 1, tgdf, stats, cost_drop, running_time) )
	res = true;
    }
    else if( rem_time == 0.0 )
    {
      /* time limit, and no time is left:  skip this solver */
    }
    else
    {
      /* time limit, and time is available:  make a timer and run the solver */
      rem_time_weight = KheRemainingTimeWeight(sm, i);
      HnAssert(rem_time_weight > 0,  /* because includes item->time_weight */
	"KheSolverMultipleRun internal error");
      avail_time = (rem_time * item->time_weight) / rem_time_weight;
      snprintf(buff, 20, "diy%d", depth);
      item_timer = KheOptionsAddTimer(options, buff, avail_time);

      /* run the solver */
      if( KheSolverRun(item->solver, rt, soln, cycle_node, options,
	    depth + 1, tgdf, stats, cost_drop, running_time) )
	res = true;

      /* delete any timer made earlier */
      KheOptionsDeleteTimer(options, item_timer);
    }

  /* work out cost and running time at the end */
  cost_after = KheSolnCost(soln);
  time_after = KheTimerElapsedTime(stats->timer);

  /* set *cost_drop and *running_time as required by caller, and return */
  *cost_drop = cost_before - cost_after;
  *running_time = time_after - time_before;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverMultipleDebug(KHE_SOLVER_MULTIPLE sm, FILE *fp)            */
/*                                                                           */
/*  Debug print of sm onto fp, in the same format as it was read in.         */
/*                                                                           */
/*****************************************************************************/

static void KheSolverMultipleDebug(KHE_SOLVER_MULTIPLE sm, FILE *fp)
{
  KHE_SOLVER_ITEM item;  int i;  KHE_SOLVER_SINGLE ss;
  if( sm->is_derived_from_rt && HaArrayCount(sm->items) > 0 )
  {
    /* debug print that returns to the underived form */
    fprintf(fp, "%s", "rt ");
    item = HaArrayFirst(sm->items);
    HnAssert(item->solver->single, "KheSolverMultipleDebug internal error 1");
    ss = (KHE_SOLVER_SINGLE) item->solver;
    HnAssert(ss->solver != NULL, "KheSolverMultipleDebug internal error 2");
    KheSolverDebug(ss->solver, fp);
  }
  else
  {
    /* regular debug print that shows the sequence of items */
    fprintf(fp, "(");
    HaArrayForEach(sm->items, item, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheSolverItemDebug(item, fp);
    }
    fprintf(fp, ")");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVER_SINGLE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_SINGLE KheSolverSingleMake(KHE_SOLVER_ID id,                  */
/*    KHE_TASKING rtc_tasking, KHE_SOLVER solver, HA_ARENA a)                */
/*                                                                           */
/*  Make and return a new single solver.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_SINGLE KheSolverSingleMake(KHE_SOLVER_ID id,
  KHE_RESOURCE_TYPE rt, KHE_SOLVER solver, HA_ARENA a)
{
  KHE_SOLVER_SINGLE res;
  HaMake(res, a);
  res->single = true;
  res->id = id;
  res->resource_type = rt;
  res->solver = solver;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverSingleParse(KHE_PARSER kp, KHE_SOLVER *res)                */
/*                                                                           */
/*  Parse one single solver, according to grammar                            */
/*                                                                           */
/*    <solver>  ::=  <name> [ <solver> ]                                     */
/*    <name>    ::=  <id> [ "!" <id> ]                                       */
/*                                                                           */
/*  Note.  The result will be a multiple solver when the input is            */
/*  syntactically single but its id is rt.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSolverSingleParse(KHE_PARSER kp, KHE_SOLVER *res)
{
  KHE_SOLVER_ITEM item;  KHE_SOLVER_SINGLE ss;  char buff[20];
  KHE_SOLVER_ID id1, id2, id;  /* KHE_TASKING rtc_tasking; */
  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  int i;  KHE_SOLVER_MULTIPLE sm;  KHE_SOLVER s;

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

  /* [ <solver> ] (depending on whether id is enclosing or non-enclosing) */
  if( KheSolverIdIsEnclosing(id) )
  {
    if( KheParserCurrCh(kp) == '(' || is_letter(KheParserCurrCh(kp)) )
      KheSolverParse(kp, &s);
    else
      KheParserAbort(kp, "no <solver> following enclosing solver %s",
	KheSolverIdShow(id));
  }
  else
  {
    if( KheParserCurrCh(kp) == '(' || is_letter(KheParserCurrCh(kp)) )
      KheParserAbort(kp, "<solver> follows non-enclosing solver %s",
	KheSolverIdShow(id));
    s = NULL;
  }

  /* create result and return true */
  if( id == KHE_SOLVER_RT )
  {
    /* convert rt <solver> into (rtc <solver>, ..., rtc <solver>) */
    sm = KheSolverMultipleMake(true, kp->arena);
    ins = KheSolnInstance(kp->soln);
    /* ***
    for( i = 0;  i < KheSolnTaskingCount(kp->soln);  i++ )
    {
      rtc_tasking = KheSolnTasking(kp->soln, i);
      rt = KheTaskingResourceType(rtc_tasking);
    }
    *** */
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      if( KheResourceTypeResourceCount(rt) > 0 )
      {
	ss = KheSolverSingleMake(KHE_SOLVER_RTC, rt, s, kp->arena);
	item = KheSolverItemMake(KheResourceTypeResourceCount(rt),
	  (KHE_SOLVER) ss, kp->arena);
	HaArrayAddLast(sm->items, item);
      }
    }
    *res = (KHE_SOLVER) sm;
  }
  else
    *res = (KHE_SOLVER) KheSolverSingleMake(id, NULL, s, kp->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverSingleRun(KHE_SOLVER_SINGLE ss, KHE_RESOURCE_TYPE rt,      */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_NODE cycle_node,               */
/*    KHE_OPTIONS options, int depth, KHE_TIMER timer, KHE_STATS stats,      */
/*    KHE_COST *cost_drop, float *running_time)                              */
/*                                                                           */
/*  Run ss.                                                                  */
/*                                                                           */
/*****************************************************************************/
static void KheStatsAddStat(KHE_STATS stats, KHE_SOLVER_ID id,
  KHE_COST cost_drop, float running_time);

static bool KheSolverSingleRun(KHE_SOLVER_SINGLE ss, KHE_RESOURCE_TYPE rt,
  KHE_SOLN soln, KHE_NODE cycle_node, KHE_OPTIONS options, int depth,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_STATS stats,
  KHE_COST *cost_drop, float *running_time)
{
  bool res;  /* KHE_TASK_SET task_set; */  char *rs_multiplier;
  char mult_str[101];  int mult_val, i;  HA_ARENA a;  KHE_SOLN_ADJUSTER sa;
  KHE_CLUSTER_MINIMUM_SOLVER cms;  KHE_MEET_BOUND_GROUP cluster_mbg;
  /* KHE_GROUP_MONITOR low_cost_gm; */  /* KHE_TASK_BOUND_GROUP tighten_tbg; */
  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_INSTANCE ins;
  float time_before, time_after, inner_running_time;
  KHE_COST cost_before, cost_after, inner_cost_drop;
  /* KHE_BALANCE_SOLVER bs; */  KHE_FRAME days_frame;
  int demand, supply;  KHE_COST task_cost, resource_cost;
  KHE_SIMPLE_GROUPER sg;

  /* work out cost and running time at the start */
  time_before = KheTimerElapsedTime(stats->timer);
  cost_before = KheSolnCost(soln);

  /* run the solve and find inner_cost_drop and inner_running time */
  inner_cost_drop = 0;
  inner_running_time = 0.0;
  switch( ss->id )
  {
    /***********************************/
    /*                                 */
    /* documented for general solvers  */
    /*                                 */
    /***********************************/

    case KHE_SOLVER_DO:

      /* do */
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      break;

    case KHE_SOLVER_SKIP:
    case KHE_SOLVER_EMPTY:

      /* skip and empty */
      res = false;
      break;

    case KHE_SOLVER_TS:

      /* KheCombinedTimeAssign */
      res = KheCombinedTimeAssign(cycle_node, options);
      break;

    case KHE_SOLVER_RS:

      /* KheCombinedResourceAssign */
      res = KheCombinedResourceAssign(soln, options);
      break;

    case KHE_SOLVER_GDL:

      /* KheDetachLowCostMonitors */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheDetachLowCostMonitors(soln, KheCost(1, 0), sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_GTS:

      /* KheSolnMatchingBegin */
      HnAssert(!KheSolnHasMatching(soln),
        "KheSolverItemRun: gts called where matching already present");
      KheMatchingBegin(soln, false);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheMatchingEnd(soln);
      break;

    case KHE_SOLVER_GTI:

      /* KheSolnMatchingBegin */
      HnAssert(!KheSolnHasMatching(soln),
        "KheSolverItemRun: gti called where matching already present");
      KheMatchingBegin(soln, true);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheMatchingEnd(soln);
      break;

    case KHE_SOLVER_GEM:

      /* KheSolnEvennessBegin */
      KheSolnEvennessBegin(soln);
      KheSolnSetAllEvennessMonitorWeights(soln, KheCost(0, 5));
      KheSolnAttachAllEvennessMonitors(soln);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnEvennessEnd(soln);
      break;

    case KHE_SOLVER_GTP:

      /* KheTiltPlateau */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheTiltPlateau(soln, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
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
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln,
	  "after KheNodePreassignedAssignTimes");
      break;

    case KHE_SOLVER_TNP:

      /* if !KheInstanceAllEventsHavePreassignedTimes */
      if( !KheInstanceAllEventsHavePreassignedTimes(KheSolnInstance(soln)) )
      {
	res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	  depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
	if( DEBUG5 )
	  KheCheckForUnassignedPreassignedMeets(soln, "after \"tnp\"");
      }
      else
	res = true;
      break;

    case KHE_SOLVER_TTP:

      /* KheTaskingTightenToPartition */
      /* tighten_tbg = KheTaskBoundGroupMake(soln); */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      ins = KheSolnInstance(soln);
      for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
      {
	rt = KheInstanceResourceType(ins, i);
	KheTightenToPartition(soln, rt, sa, options);
      }
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln, "after \"ttp\"");
      /* KheTaskBoundGroupDelete(tighten_tbg); */
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_TMD:

      /* KheSolnClusterAndLimitMeetDomains */
      cluster_mbg = KheMeetBoundGroupMake(soln);
      KheSolnAddUnavailableBounds(soln, KheCost(0, 0), cluster_mbg);
      KheSolnClusterAndLimitMeetDomains(soln, 0, KheCost(1, 0), 1.8,
	cluster_mbg, options);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheMeetBoundGroupDelete(cluster_mbg);
      break;

    case KHE_SOLVER_TNL:

      /* KheNodeLayeredAssignTimes */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tnl given no cycle node");
      res = KheNodeLayeredAssignTimes(cycle_node, options);
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln,
	  "after KheNodeLayeredAssignTimes");
      break;

    case KHE_SOLVER_TEC:

      HnAssert(cycle_node != NULL, "KheSolverItemRun: tec given no cycle node");
      res = KheEjectionChainNodeRepairTimes(cycle_node, options, NULL);
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln,
	  "after KheEjectionChainNodeRepairTimes");
      break;

    case KHE_SOLVER_TNF:

      /* KheNodeFlatten */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tnf given no cycle node");
      KheNodeFlatten(cycle_node);
      res = true;
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln,
	  "after KheNodeFlatten");
      break;

    case KHE_SOLVER_TDZ:

      /* KheNodeDeleteZones */
      HnAssert(cycle_node != NULL, "KheSolverItemRun: tdz given no cycle node");
      KheNodeDeleteZones(cycle_node);
      res = true;
      if( DEBUG5 )
	KheCheckForUnassignedPreassignedMeets(soln, "after KheNodeDeleteZones");
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
      res = KheSolverRun(ss->solver, ss->resource_type, soln, cycle_node,
	options, depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      break;

    case KHE_SOLVER_RRD:

      a = KheSolnArenaBegin(soln);
      days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
      /* ***
      bs = KheBalanceSolverMake(soln, rt, days_frame, a);
      if( KheBalanceSolverTotalSupply(bs) <= KheBalanceSolverTotalDemand(bs) )
      *** */
      if( KheResourceDemandExceedsSupply(soln, rt, &demand, &supply,
	    &task_cost, &resource_cost) )
      {
	/* ***
	KheSolnRedundancyBegin(soln,rt,KHE_LINEAR_COST_FUNCTION, KheCost(1, 0));
	*** */
	KheSolnRedundancyBegin(soln,rt,KHE_LINEAR_COST_FUNCTION, KheCost(0, 1));
	res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	  depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
	KheSolnRedundancyEnd(soln);
      }
      else
	res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	  depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_REM:

      /* event timetable monitor */
      ins = KheSolnInstance(soln);
      etm = KheEventTimetableMonitorMake(soln, KheInstanceFullEventGroup(ins));
      KheMonitorAttachToSoln((KHE_MONITOR) etm);
      KheOptionsSetObject(options, "gs_event_timetable_monitor", (void *) etm);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      HnAssert(KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL)
	== (void *) etm, "KheSolverItemRun internal error (etm)");
      KheOptionsSetObject(options, "gs_event_timetable_monitor", NULL);
      break;

    case KHE_SOLVER_RWP:

      /* KheWorkloadPack */
      HnAssert(rt != NULL, "rwp item in rs option is outside rt()");
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheWorkloadPack(soln, options, rt, sa /* &tighten_tbg */);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      /* ***
      if( tighten_tbg != NULL )
	KheTaskBoundGroupDelete(tighten_tbg);
      *** */
      break;

    case KHE_SOLVER_RSM:

      /* KheSetMonitorMultipliers */
      rs_multiplier = KheOptionsGet(options, "rs_multiplier", NULL);
      HnAssert(rs_multiplier != NULL, "rsm item in rs option has no "
	"corresponding rs_multiplier option");
      if( sscanf(rs_multiplier, "%d:%100s", &mult_val, mult_str) != 2 )
	HnAbort("syntax error in rs_multiplier value \"%s\"", rs_multiplier);
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheSetClusterMonitorMultipliers(soln, sa, mult_str, mult_val);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
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
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheClusterMinimumSolverSetEnd(cms, true);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RCG:

      /* KheCombinatorialGrouping */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheCombinatorialGrouping(soln, rt, options, tgdf, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RWG:

      /* KheWeekendGrouping */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheWeekendGrouping(soln, rt, options, tgdf, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RIG:

      /* KheIntervalGrouping */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      KheIntervalGrouping(soln, rt, options, tgdf, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RGR:

      /* KheGroupByResource */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
      sg = KheSimpleGrouperMake(soln, days_frame, tgdf, a);
      KheSimpleGrouperAddResourceTypeTasks(sg, rt);
      KheSimpleGrouperMakeGroups(sg,
	KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE_CONSECUTIVE, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      /* *** old version from before KheSimpleGrouper
      sa = KheSolnAdjusterMake(soln);
      if( KheGroupByResource(soln, rt, options, sa) )
      {
	res = KheSolverRun(ss->solver, rt, tasking, soln, cycle_node, options,
	  depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      }
      else
	res = false;
      KheSolnAdjusterUndo(sa);
      KheSolnAdjusterDelete(sa);
      *** */
      /* *** old version that uses KheTaskSetUnGroup
      task_set = KheTaskSetMake(soln);
      if( KheTaski ngGroupByResource(tasking, options, task_set) )
      {
	res = KheSolverRun(ss->solver, rt, tasking, soln, cycle_node, options,
	  depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
	KheTaskSetUnGr oup(task_set);
	KheTaskSetDelete(task_set);
      }
      else
	res = false;
      *** */
      break;

    case KHE_SOLVER_RED:

      /* KheTaskingEnlargeDomains */
      KheEnlargeDomains(soln, rt, false);
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
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheOptionsSetBool(options, "rs_invariant", false);
      break;

    case KHE_SOLVER_RRQ:

      /* KheSolnAssignRequestedResources */
      res = KheSolnAssignRequestedResources(soln, rt, options);
      break;

    case KHE_SOLVER_RAH:

      /* KheAssignByHistory */
      a = KheSolnArenaBegin(soln);
      sa = KheSolnAdjusterMake(soln, a);
      res = KheAssignByHistory(soln, rt, options, sa);
      res = KheSolverRun(ss->solver, rt, soln, cycle_node, options,
	depth + 1, tgdf, stats, &inner_cost_drop, &inner_running_time);
      KheSolnAdjusterUndo(sa);
      KheSolnArenaEnd(soln, a);
      break;

    case KHE_SOLVER_RMC:

      /* KheMostConstrainedFirstAssignResources */
      res = KheMostConstrainedFirstAssignResources(soln, rt, options);
      break;

    case KHE_SOLVER_RPK:

      /* KheResourcePackAssignResources */
      res = KheResourcePackAssignResources(soln, rt, options);
      break;

    case KHE_SOLVER_RCX:

      /* resource assignment choice */
      if( rt != NULL && KheResourceTypeAvoidSplitAssignmentsCount(rt)==0 )
	res = KheMostConstrainedFirstAssignResources(soln, rt, options);
      else
	res = KheResourcePackAssignResources(soln, rt, options);
      break;

    case KHE_SOLVER_RFS:

      /* KheFindSplitResourceAssignments */
      if( KheResourceTypeAvoidSplitAssignmentsCount(rt) > 0 )
      {
	res = KheFindSplitResourceAssignments(soln, rt, options);
	KheAllowSplitAssignments(soln, rt, false);
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

    case KHE_SOLVER_RDW:

      res = KheDynamicResourceBalanceWorkloads(soln, rt, options);
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
      res = KheEjectionChainRepairResources(soln, rt, options, NULL);
      break;

    case KHE_SOLVER_RE2:

      /* KheEjectionChainRepairResources */
      res = KheEjectionChainRepairResources(soln, rt, options, "2+");
      break;

    case KHE_SOLVER_RGL:

      /* KheGlobalLoadBalance */
      res = KheGlobalLoadBalance(soln, rt, options);
      break;

    case KHE_SOLVER_RRS:

      /* KheResourcePairRepair */
      res = KheResourcePairRepair(soln, rt, options);
      break;

    case KHE_SOLVER_RRP:

      /* KheReassign1Repair */
      res = KheReassign1Repair(soln, rt, options);
      break;

    default:

      HnAbort("KheSolverItemRun: internal error (id %d)", ss->id);
      res = false;
      break;
  }

  /* work out cost and running time at the end */
  time_after = KheTimerElapsedTime(stats->timer);
  cost_after = KheSolnCost(soln);

  /* work out change in cost and running time for the caller */
  *cost_drop = cost_before - cost_after;
  *running_time = time_after - time_before;

  /* save cost and running time, but subtract out the inner values */
  KheStatsAddStat(stats, ss->id, *cost_drop - inner_cost_drop,
    *running_time - inner_running_time);
  /* ***
  if( DEBUG3 )
  {
    fprintf(stderr, "[queries %6d, cost_after %10.5f] ",
      KheOptionsTimeLimitReachedQueryCount(options), KheCostShow(cost_after));
    fprintf(stderr, "[queries %6d, time %6.1f, cost_after %10.5f] ",
      KheOptionsTimeLimitReachedQueryCount(options), time_after,
      KheCostShow(cost_after));
    KheSolverSingleDebug(ss, stderr);
    fprintf(stderr, "\n");
  }
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverSingleDebug(KHE_SOLVER_SINGLE ss, FILE *fp)                */
/*                                                                           */
/*  Debug print of ss onto fp, in the same format as it was read in.         */
/*                                                                           */
/*****************************************************************************/

static void KheSolverSingleDebug(KHE_SOLVER_SINGLE ss, FILE *fp)
{
  fprintf(fp, "%s", KheSolverIdShow(ss->id));
  if( ss->resource_type != NULL )
    fprintf(fp, "-%s", KheResourceTypeId(ss->resource_type));
  if( ss->solver != NULL )
  {
    fprintf(fp, " ");
    KheSolverDebug(ss->solver, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVER"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolverParse(KHE_PARSER kp, KHE_SOLVER *res)                      */
/*                                                                           */
/*  Parse a solver following grammar                                         */
/*                                                                           */
/*    <solver>  ::=  <name> [ <solver> ]                                     */
/*              ::=  "(" <item> "," <item> { "," <item> } ")"                */
/*    <name>    ::=  <id> [ "!" <id> ]                                       */
/*    <item>    ::=  [ <int> ":" ] <solver>                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSolverParse(KHE_PARSER kp, KHE_SOLVER *res)
{
  if( KheParserCurrCh(kp) == '(' )
    KheSolverMultipleParse(kp, res);
  else
    KheSolverSingleParse(kp, res);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,                    */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_NODE cycle_node,               */
/*    KHE_OPTIONS options, int depth, KHE_STATS stats,                       */
/*    KHE_COST *cost_drop, float *running_time)                              */
/*                                                                           */
/*  Run solver on these attributes.  Update stats, and set *cost_drop        */
/*  to the amount that cost has dropped over this call, and *running_time    */
/*  to the running time of this call.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_SOLN soln, KHE_NODE cycle_node, KHE_OPTIONS options, int depth,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_STATS stats,
  KHE_COST *cost_drop, float *running_time)
{
  char buff[20];  KHE_COST cost_before, cost_after;  float rem_time;
  bool res;

  /* debug the start of the run */
  HnAssert(s != NULL, "KheSolverRun internal error (s == NULL)");
  if( DEBUG1 )
  {
    cost_before = KheSolnCost(soln);
    fprintf(stderr, "%*s[ ", depth * 2, "");
    KheSolverDebug(s, stderr);
    rem_time = KheOptionsRemainingTime(options);
    if( rem_time != KHE_NO_TIME )
      fprintf(stderr, ", avail %s", KheTimeShow(rem_time, buff));
    fprintf(stderr, "\n");
  }

  /* do the actual run */
  if( s->single )
  {
    /* run a single */
    res = KheSolverSingleRun((KHE_SOLVER_SINGLE) s, rt, soln,
      cycle_node, options, depth, tgdf, stats, cost_drop, running_time);
  }
  else
  {
    /* run a multiple */
    res = KheSolverMultipleRun((KHE_SOLVER_MULTIPLE) s, rt, soln,
      cycle_node, options, depth, tgdf, stats, cost_drop, running_time);
  }

  /* debug the end of the run */
  if( DEBUG1 )
  {
    cost_after = KheSolnCost(soln);
    fprintf(stderr, "%*s] %s", depth * 2, "", bool_show(res));
    if( cost_after < cost_before )
      fprintf(stderr, ", improved from %.5f to %.5f", KheCostShow(cost_before),
	KheCostShow(cost_after));
    else if( cost_after > cost_before )
      fprintf(stderr, ", worsened from %.5f to %.5f", KheCostShow(cost_before),
	KheCostShow(cost_after));
    fprintf(stderr, ", used %s\n", KheTimeShow(*running_time, buff));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverDebug(KHE_SOLVER solver, FILE *fp)                         */
/*                                                                           */
/*  Debug print of s onto fp, in the same format as it was read in.          */
/*                                                                           */
/*****************************************************************************/

static void KheSolverDebug(KHE_SOLVER s, FILE *fp)
{
  HnAssert(s != NULL, "KheSolverDebug internal error:  s == NULL");
  if( s->single )
    KheSolverSingleDebug((KHE_SOLVER_SINGLE) s, fp);
  else
    KheSolverMultipleDebug((KHE_SOLVER_MULTIPLE) s, fp);
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
    fprintf(fp, "%*s[ DoItYourself %s stats:\n", indent, "", stats->label);
    header_printed = false;
    HaArrayForEach(stats->stats, ss, i)
      if( ss->no_of_calls > 0 )
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
  /* ***
  if( ss->solver_id >= KHE_SOLVER_TCL && ss->no_of_calls > 0 &&
      (ss->total_cost_drop > 0 || ss->total_running_time >= 0.1) )
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "macro expansion"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMatchingRightBrace(char *str, int i, int *res)               */
/*                                                                           */
/*  Here str[i] contains '{'.  If str has a matching right brace, set        */
/*  *res to its position and return true.  Otherwise return false.           */
/*                                                                           */
/*****************************************************************************/

static bool KheFindMatchingRightBrace(char *str, int i, int *res)
{
  int depth;
  HnAssert(str[i] == '{', "KheFindMatchingRightBrace internal error");
  for( i++, depth = 1;  str[i] != '\0';  i++ )
  {
    switch( str[i] )
    {
      case '{':
	depth++;
	break;

      case '}':
	depth--;
	if( depth == 0 )
	  return *res = i, true;
	break;

      default:
	break;
    }
  }
  return *res = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMacroExpand(char *str, KHE_OPTIONS options, HA_ARENA a)         */
/*                                                                           */
/*  Carry out a simple macro expansion of str.                               */
/*                                                                           */
/*  Implementation note.  This function's first step is to copy str.  This   */
/*  is to ensure thread-safety.                                              */
/*                                                                           */
/*****************************************************************************/

static char *KheMacroExpand(char *str, KHE_OPTIONS options, HA_ARENA a)
{
  HA_ARRAY_NCHAR res;
  int pos, i, amp_pos, id_pos, left_brace_pos, right_brace_pos;
  char *val;
  str = HnStringCopy(str, a);
  pos = 0;
  RESTART:
  for( ;  str[pos] != '\0';  pos++ )
  {
    if( str[pos] == '@' )
    {
      /* find @<id>{ ... } */
      amp_pos = pos;
      id_pos = pos + 1;
      for( i = id_pos;  is_letter(str[i]);  i++ );
      if( i == id_pos )
	HnAbort("in do-it-yourself string, @ not followed by <id>");
      if( str[i] != '{' )
	HnAbort("in do-it-yourself string, @<id> not followed by left brace: "
	  "id \"%s\", lb \"%s\"", &str[id_pos], &str[i]);
      left_brace_pos = i;
      if( !KheFindMatchingRightBrace(str, i, &right_brace_pos) )
	HnAbort("in do-it-yourself string, @<id>{ has no matching right brace");

      /* look up <id> and either use its value or not */
      str[amp_pos] = '\0';
      str[left_brace_pos] = '\0';
      str[right_brace_pos] = '\0';
      val = KheOptionsGet(options, &str[id_pos], &str[left_brace_pos + 1]);
      HnStringBegin(res, a);
      HnStringAdd(&res, "%s%s%s", str, val, &str[right_brace_pos + 1]);
      str[amp_pos] = '@';
      str[left_brace_pos] = '{';
      str[right_brace_pos] = '}';
      str = HnStringEnd(res);
      pos = amp_pos;
      goto RESTART;
    }
  }
  return str;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "running"                                                      */
/*                                                                           */
/*****************************************************************************/

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
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf;

  /* macro expansion */
  a = KheSolnArenaBegin(soln);
  solver_option_val = KheMacroExpand(solver_option_val, options, a);

  /* parse */
  kp = KheParserMake(soln, solver_option_name, solver_option_val, a);
  KheSolverParse(kp, &solver);
  if( KheParserCurrCh(kp) != '\0' )
    KheParserAbort(kp, "unexpected character '%c'", KheParserCurrCh(kp));

  /* run */
  tgdf = KheTaskGroupDomainFinderMake(soln, a);
  stats = KheStatsMake(solver_option_name, a);
  res = KheSolverRun(solver, NULL, soln, cycle_node, options, 1,
    tgdf, stats, &cost_drop, &running_time);
  if( DEBUG2 )
    KheStatsDebug(stats, 2, 0, stderr);
  KheSolnArenaEnd(soln, a);
  return res;
}


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
/*  FILE:         khe_sm_backoff.c                                           */
/*  DESCRIPTION:  Exponential backoff                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_BACKOFF                                                              */
/*                                                                           */
/*****************************************************************************/

struct khe_backoff_rec
{
  KHE_BACKOFF_TYPE backoff_type;
  int	curr_declined;		/* opportunities declined since last accept  */
  int	max_declined;		/* maximum allowed value of curr_declined    */
  bool	expecting_result;	/* next call must be to KheBackoffResult     */
  int	debug_successful;	/* successful opps so far (debug only)       */
  int	debug_failed;		/* failed opps so far (debug only)           */
  int	debug_declined;		/* declined opps so far (debug only)         */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_BACKOFF KheBackoffBegin(KHE_BACKOFF_TYPE backoff_type)               */
/*                                                                           */
/*  Create, initialize, and return a new backoff object.                     */
/*                                                                           */
/*****************************************************************************/

KHE_BACKOFF KheBackoffBegin(KHE_BACKOFF_TYPE backoff_type, HA_ARENA a)
{
  KHE_BACKOFF res;
  HaMake(res, a);
  res->backoff_type = backoff_type;
  res->curr_declined = 0;
  res->max_declined = 0;
  res->expecting_result = false;
  res->debug_successful = 0;
  res->debug_failed = 0;
  res->debug_declined = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheBackoffAcceptOpportunity(KHE_BACKOFF bk)                         */
/*                                                                           */
/*  Return true if this opportunity should be accepted.                      */
/*                                                                           */
/*****************************************************************************/

bool KheBackoffAcceptOpportunity(KHE_BACKOFF bk)
{
  HnAssert(!bk->expecting_result, "KheBackoffAcceptOpportunity called"
    "when KheBackoffResult expected");
  switch( bk->backoff_type )
  {

    case KHE_BACKOFF_NONE:

      /* always accept */
      bk->expecting_result = true;
      return true;

    case KHE_BACKOFF_EXPONENTIAL:

      if( bk->curr_declined < bk->max_declined )
      {
	/* decline */
	bk->curr_declined++;
	bk->debug_declined++;
	return false;
      }
      else
      {
	/* accept */
	bk->curr_declined = 0;
	bk->expecting_result = true;
	return true;
      }

    default:

      HnAbort("KheBackoffAcceptOpportunity: invalid backoff_type");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBackoffResult(KHE_BACKOFF bk, bool success)                      */
/*                                                                           */
/*  Let bk know whether the opportunity last accepted was a success.         */
/*                                                                           */
/*****************************************************************************/

void KheBackoffResult(KHE_BACKOFF bk, bool success)
{
  HnAssert(bk->expecting_result, "KheBackoffResult called out of order");
  switch( bk->backoff_type )
  {
    case KHE_BACKOFF_NONE:

      if( success )
        bk->debug_successful++;
      else
        bk->debug_failed++;
      break;

    case KHE_BACKOFF_EXPONENTIAL:

      if( success )
      {
	/* successful */
	bk->max_declined = 0;
	bk->debug_successful++;
      }
      else
      {
	/* failed */
	if( bk->max_declined == 0 )
	  bk->max_declined = 1;
	else if( bk->max_declined <= INT_MAX / 2 )
	  bk->max_declined *= 2;
	bk->debug_failed++;
      }
      break;

    default:

      HnAbort("KheBackoffResult: invalid backoff_type");
  }
  bk->expecting_result = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBackoffEnd(KHE_BACKOFF bk)                                       */
/*                                                                           */
/*  Reclaim the memory used by bk.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheBackoffEnd(KHE_BACKOFF bk)
{
  HnAssert(!bk->expecting_result, "KheBackoffEnd called"
    "when KheBackoffResult expected");
  MFree(bk);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheBackoffShowNextDecision(KHE_BACKOFF bk)                         */
/*                                                                           */
/*  Return a string indicating bk's decision about the next opportunity.     */
/*                                                                           */
/*****************************************************************************/

char *KheBackoffShowNextDecision(KHE_BACKOFF bk)
{
  switch( bk->backoff_type )
  {
    case KHE_BACKOFF_NONE:

      return "ACCEPT";

    case KHE_BACKOFF_EXPONENTIAL:

      return (bk->curr_declined < bk->max_declined ? "DECLINE" : "ACCEPT");

    default:

      HnAbort("KheBackoffShowNextDecision: invalid backoff_type");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBackoffSingleTest(KHE_BACKOFF_TYPE backoff_type,                 */
/*    bool *results, int rcount, int indent, FILE *fp)                       */
/*                                                                           */
/*  Carry out one test of exponential backoff, using results[0..rcount-1].   */
/*                                                                           */
/*****************************************************************************/

static void KheBackoffSingleTest(KHE_BACKOFF_TYPE backoff_type,
  bool *results, int rcount, int indent, FILE *fp)
{
  KHE_BACKOFF bk;  int i;  HA_ARENA a;  HA_ARENA_SET as;
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  bk = KheBackoffBegin(backoff_type, a);
  fprintf(fp, "%*s", indent, "");
  for( i = 0;  i < rcount;  )
    if( KheBackoffAcceptOpportunity(bk) )
    {
      fprintf(fp, "%c", results[i] ? 'S' : 'F');
      KheBackoffResult(bk, results[i++]);
    }
    else
      fprintf(fp, "%c", '.');
  fprintf(fp, "\n");
  KheBackoffDebug(bk, 2, indent + 2, fp);
  HaArenaDelete(a);  /* this arena is used only for testing */
  /* KheBackoffEnd(bk); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBackoffTest(FILE *fp)                                            */
/*                                                                           */
/*  Test this module.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheBackoffTest(FILE *fp)
{
  bool test1[] = {true, true, true, false, false, true};
  bool test2[] = {false, true, false, false, false, false, false, true};
  fprintf(fp, "[ KheBackoffTest()\n");
  fprintf(fp, "  KHE_BACKOFF_EXPONENTIAL:\n");
  KheBackoffSingleTest(KHE_BACKOFF_EXPONENTIAL, test1, 6, 4, fp);
  KheBackoffSingleTest(KHE_BACKOFF_EXPONENTIAL, test2, 8, 4, fp);
  fprintf(fp, "  KHE_BACKOFF_NONE:\n");
  KheBackoffSingleTest(KHE_BACKOFF_NONE, test1, 6, 4, fp);
  KheBackoffSingleTest(KHE_BACKOFF_NONE, test2, 8, 4, fp);
  fprintf(fp, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBackoffDebug(KHE_BACKOFF bk, int verbosity, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of bk to fp with the given verbosity and indent.             */
/*                                                                           */
/*****************************************************************************/

void KheBackoffDebug(KHE_BACKOFF bk, int verbosity, int indent, FILE *fp)
{
  if( verbosity == 1 || indent < 0 )
  {
    /* brief one-line display, no statistics */
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    switch( bk->backoff_type )
    {
      case KHE_BACKOFF_NONE:

	fprintf(fp, "[NONE%s]", bk->expecting_result ? "!" : "");
	break;

      case KHE_BACKOFF_EXPONENTIAL:

	fprintf(fp, "[C%dM%d%s]", bk->curr_declined, bk->max_declined,
	  bk->expecting_result ? "!" : "");
	break;

      default:

	HnAbort("KheBackoffDebug: invalid backoff_type");
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
  else
  {
    /* multi-line display (same for all types) */
    fprintf(fp, "%*s[ %d opportunities ", indent, "",
      bk->debug_successful + bk->debug_failed + bk->debug_declined);
    fprintf(fp, "(%d successful, %d failed, %d declined) ]\n",
      bk->debug_successful, bk->debug_failed, bk->debug_declined);
  }
}

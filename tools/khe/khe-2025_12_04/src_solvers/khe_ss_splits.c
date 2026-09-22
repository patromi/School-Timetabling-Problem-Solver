
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
/*  FILE:         khe_ss_splits.c                                            */
/*  DESCRIPTION:  Analysing split defects                                    */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_SUGGESTION - one suggestion for splitting meets                */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_split_suggestion_rec {
  int	merged_durn;
  int	split1_durn;
} *KHE_SPLIT_SUGGESTION;

typedef HA_ARRAY(KHE_SPLIT_SUGGESTION) ARRAY_KHE_SPLIT_SUGGESTION;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MERGE_SUGGESTION - one suggestion for merging meets                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_merge_suggestion_rec {
  int	split1_durn;
  int	split2_durn;
} *KHE_MERGE_SUGGESTION;

typedef HA_ARRAY(KHE_MERGE_SUGGESTION) ARRAY_KHE_MERGE_SUGGESTION;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DURN_INFO - information about one duration                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_durn_info_rec {
  int	duration;
  int	min_amount;
  int	max_amount;
  int	curr_amount;
} *KHE_DURN_INFO;

typedef HA_ARRAY(KHE_DURN_INFO) ARRAY_KHE_DURN_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_ANALYSER - object which analyses split defects                 */
/*                                                                           */
/*****************************************************************************/

struct khe_split_analyser_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_EVENT			event;
  ARRAY_KHE_SPLIT_SUGGESTION	split_suggestions;
  ARRAY_KHE_SPLIT_SUGGESTION	free_split_suggestions;
  ARRAY_KHE_MERGE_SUGGESTION	merge_suggestions;
  ARRAY_KHE_MERGE_SUGGESTION	free_merge_suggestions;
  ARRAY_KHE_DURN_INFO		durn_info;
  ARRAY_KHE_DURN_INFO		free_durn_info;
  int				min_amount;
  int				max_amount;
  int				curr_amount;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "split suggestions"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_SUGGESTION KheSplitSuggestionMake(KHE_SPLIT_ANALYSER sa,       */
/*    int merged_durn, int split1_durn)                                      */
/*                                                                           */
/*  Make a split suggestion object with these attributes.  Get the memory    */
/*  either from sa's free list or from HaMake.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_SPLIT_SUGGESTION KheSplitSuggestionMake(KHE_SPLIT_ANALYSER sa,
  int merged_durn, int split1_durn)
{
  KHE_SPLIT_SUGGESTION res;
  if( HaArrayCount(sa->free_split_suggestions) > 0 )
    res = HaArrayLastAndDelete(sa->free_split_suggestions);
  else
    HaMake(res, sa->arena);
  res->merged_durn = merged_durn;
  res->split1_durn = split1_durn;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitSuggestionFree(KHE_SPLIT_SUGGESTION ss)                     */
/*                                                                           */
/*  Free ss.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitSuggestionFree(KHE_SPLIT_SUGGESTION ss)
{
  MFree(ss);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitSuggestionCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Comparison function for sorting and uniquifying split suggestions.       */
/*                                                                           */
/*****************************************************************************/

static int KheSplitSuggestionCmp(const void *t1, const void *t2)
{
  KHE_SPLIT_SUGGESTION ss1 = * (KHE_SPLIT_SUGGESTION *) t1;
  KHE_SPLIT_SUGGESTION ss2 = * (KHE_SPLIT_SUGGESTION *) t2;
  if( ss1->merged_durn != ss2->merged_durn )
    return ss1->merged_durn - ss2->merged_durn;
  if( ss1->split1_durn != ss2->split1_durn )
    return ss1->split1_durn - ss2->split1_durn;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "merge suggestions"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MERGE_SUGGESTION KheMergeSuggestionMake(KHE_SPLIT_ANALYSER sa,       */
/*    int split1_durn, int split2_durn)                                      */
/*                                                                           */
/*  Make a new merge suggestion object.  Get the memory either from sa's     */
/*  free list or from HaMake.                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_MERGE_SUGGESTION KheMergeSuggestionMake(KHE_SPLIT_ANALYSER sa,
  int split1_durn, int split2_durn)
{
  KHE_MERGE_SUGGESTION res;
  if( HaArrayCount(sa->free_merge_suggestions) > 0 )
    res = HaArrayLastAndDelete(sa->free_merge_suggestions);
  else
    HaMake(res, sa->arena);
  res->split1_durn = split1_durn;
  res->split2_durn = split2_durn;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMergeSuggestionFree(KHE_MERGE_SUGGESTION ms)                     */
/*                                                                           */
/*  Free ms.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMergeSuggestionFree(KHE_MERGE_SUGGESTION ms)
{
  MFree(ms);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMergeSuggestionCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Comparison function for sorting and uniquifying merge suggestions.       */
/*                                                                           */
/*****************************************************************************/

static int KheMergeSuggestionCmp(const void *t1, const void *t2)
{
  KHE_MERGE_SUGGESTION ms1 = * (KHE_MERGE_SUGGESTION *) t1;
  KHE_MERGE_SUGGESTION ms2 = * (KHE_MERGE_SUGGESTION *) t2;
  if( ms1->split1_durn != ms2->split1_durn )
    return ms1->split1_durn - ms2->split1_durn;
  if( ms1->split2_durn != ms2->split2_durn )
    return ms1->split2_durn - ms2->split2_durn;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "duration info"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DURN_INFO KheDurnInfoMake(KHE_SPLIT_ANALYSER sa, int duration)       */
/*                                                                           */
/*  Make a durn info object with these attributes.  Get the memory either    */
/*  from sa's free list or from HaMake.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_DURN_INFO KheDurnInfoMake(KHE_SPLIT_ANALYSER sa, int duration)
{
  KHE_DURN_INFO res;
  if( HaArrayCount(sa->free_durn_info) > 0 )
    res = HaArrayLastAndDelete(sa->free_durn_info);
  else
    HaMake(res, sa->arena);
  res->duration = duration;
  res->min_amount = 0;
  res->max_amount = INT_MAX;
  res->curr_amount = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDurnInfoFree(KHE_DURN_INFO di)                                   */
/*                                                                           */
/*  Free di.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheDurnInfoFree(KHE_DURN_INFO di)
{
  MFree(di);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "split analyser"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_ANALYSER KheSplitAnalyserMake(KHE_SOLN soln)                   */
/*                                                                           */
/*  Make a split analyser object.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_ANALYSER KheSplitAnalyserMake(KHE_SOLN soln)
{
  KHE_SPLIT_ANALYSER res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->event = NULL;
  HaArrayInit(res->split_suggestions, a);
  HaArrayInit(res->free_split_suggestions, a);
  HaArrayInit(res->merge_suggestions, a);
  HaArrayInit(res->free_merge_suggestions, a);
  HaArrayInit(res->durn_info, a);
  HaArrayInit(res->free_durn_info, a);
  res->min_amount = 0;
  res->max_amount = 0;
  res->curr_amount = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAnalyserDelete(KHE_SPLIT_ANALYSER sa)                       */
/*                                                                           */
/*  Delete a split analyser object.                                          */
/*                                                                           */
/*****************************************************************************/

void KheSplitAnalyserDelete(KHE_SPLIT_ANALYSER sa)
{
  KheSolnArenaEnd(sa->soln, sa->arena);
  /* HaAre naDelete(sa->arena); */
  /* ***
  while( HaArrayCount(sa->split_suggestions) > 0 )
    KheSplitSuggestionFree(HaArrayLastAndDelete(sa->split_suggestions));
  MArrayFree(sa->split_suggestions);
  while( HaArrayCount(sa->free_split_suggestions) > 0 )
    KheSplitSuggestionFree(HaArrayLastAndDelete(sa->free_split_suggestions));
  MArrayFree(sa->free_split_suggestions);
  while( HaArrayCount(sa->merge_suggestions) > 0 )
    KheMergeSuggestionFree(HaArrayLastAndDelete(sa->merge_suggestions));
  MArrayFree(sa->merge_suggestions);
  while( HaArrayCount(sa->free_merge_suggestions) > 0 )
    KheMergeSuggestionFree(HaArrayLastAndDelete(sa->free_merge_suggestions));
  MArrayFree(sa->free_merge_suggestions);
  while( HaArrayCount(sa->durn_info) > 0 )
    KheDurnInfoFree(HaArrayLastAndDelete(sa->durn_info));
  MArrayFree(sa->durn_info);
  while( HaArrayCount(sa->free_durn_info) > 0 )
    KheDurnInfoFree(HaArrayLastAndDelete(sa->free_durn_info));
  MArrayFree(sa->free_durn_info);
  MFree(sa);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_ANALYSER KheSplitAnalyserOption(KHE_OPTIONS options, char *key)*/
/*                                                                           */
/*  Return a split analyser object from options, creating it if necessary.   */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_ANALYSER KheSplitAnalyserOption(KHE_OPTIONS options, char *key,
  KHE_SOLN soln)
{
  void *value;  KHE_SPLIT_ANALYSER res;
  value = KheOptionsGetObject(options, key, NULL);
  if( value != NULL )
    res = (KHE_SPLIT_ANALYSER) value;
  else
  {
    res = KheSplitAnalyserMake(soln);
    KheOptionsSetObject(options, key, (void *) res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitIsPossible(KHE_SPLIT_ANALYSER sa,                           */
/*    int merged_durn, int split1_durn)                                      */
/*                                                                           */
/*  Return true if a repair which splits a meet of duration merged_durn      */
/*  so that the first fragment has duration split1_durn is possible.         */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitIsPossible(KHE_SPLIT_ANALYSER sa,
  int merged_durn, int split1_durn)
{
  KHE_DURN_INFO di;  int split2_durn;

  /* return false if more meets are not acceptable */
  if( sa->curr_amount >= sa->max_amount )
    return false;

  /* return false if removing size merged_durn is bad */
  di = HaArray(sa->durn_info, merged_durn - 1);
  if( di->curr_amount <= di->min_amount )
    return false;

  /* return false if adding sizes split1_durn and split2_durn is bad */
  split2_durn = merged_durn - split1_durn;
  if( split2_durn == split1_durn )
  {
    /* fragments have same duration */
    di = HaArray(sa->durn_info, split1_durn - 1);
    if( di->curr_amount >= di->max_amount - 1 )
      return false;
  }
  else
  {
    /* fragments have different durations */
    di = HaArray(sa->durn_info, split1_durn - 1);
    if( di->curr_amount >= di->max_amount )
      return false;
    di = HaArray(sa->durn_info, split2_durn - 1);
    if( di->curr_amount >= di->max_amount )
      return false;
  }

  /* all OK so return true */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTrySplit(KHE_SPLIT_ANALYSER sa, int merged_durn, int split1_durn)*/
/*                                                                           */
/*  If a split with these attributes is possible, add a suggestion for it.   */
/*                                                                           */
/*****************************************************************************/

static void KheTrySplit(KHE_SPLIT_ANALYSER sa, int merged_durn, int split1_durn)
{
  if( KheSplitIsPossible(sa, merged_durn, split1_durn) )
    HaArrayAddLast(sa->split_suggestions,
      KheSplitSuggestionMake(sa, merged_durn, split1_durn));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMergeIsPossible(KHE_SPLIT_ANALYSER sa,                           */
/*    int split1_durn, int split2_durn)                                      */
/*                                                                           */
/*  Return true if a repair which merges meets of durations split1_durn      */
/*  and split2_durn is possible.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMergeIsPossible(KHE_SPLIT_ANALYSER sa,
  int split1_durn, int split2_durn)
{
  KHE_DURN_INFO di;  int merged_durn;

  /* return false if fewer meets are not acceptable */
  if( sa->curr_amount <= sa->min_amount )
    return false;

  /* return false if adding size merged_durn is bad */
  merged_durn = split1_durn + split2_durn;
  di = HaArray(sa->durn_info, merged_durn - 1);
  if( di->curr_amount >= di->max_amount )
    return false;

  /* return false if removing sizes split1_durn and split2_durn is bad */
  if( split2_durn == split1_durn )
  {
    /* fragments have same duration */
    di = HaArray(sa->durn_info, split1_durn - 1);
    if( di->curr_amount <= di->min_amount + 1 )
      return false;
  }
  else
  {
    /* fragments have different durations */
    di = HaArray(sa->durn_info, split1_durn - 1);
    if( di->curr_amount <= di->min_amount )
      return false;
    di = HaArray(sa->durn_info, split2_durn - 1);
    if( di->curr_amount <= di->min_amount )
      return false;
  }

  /* all OK so return true */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTryMerge(KHE_SPLIT_ANALYSER sa, int split1_durn, int split2_durn)*/
/*                                                                           */
/*  If a merge with these attributes is possible, add a suggestion for it.   */
/*                                                                           */
/*****************************************************************************/

static void KheTryMerge(KHE_SPLIT_ANALYSER sa, int split1_durn, int split2_durn)
{
  if( KheMergeIsPossible(sa, split1_durn, split2_durn) )
    HaArrayAddLast(sa->merge_suggestions,
      KheMergeSuggestionMake(sa, split1_durn, split2_durn));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAnalyserAnalyse(KHE_SPLIT_ANALYSER sa, KHE_EVENT e)         */
/*                                                                           */
/*  Carry out the analysis of e.                                             */
/*                                                                           */
/*****************************************************************************/

void KheSplitAnalyserAnalyse(KHE_SPLIT_ANALYSER sa, KHE_EVENT e)
{
  int i, durn, durn2, max_durn, min_d, max_d, min_a, max_a, meet_count;
  KHE_DURN_INFO di;  KHE_MONITOR m;

  /* clear out old values */
  HaArrayAppend(sa->free_split_suggestions, sa->split_suggestions, i);
  HaArrayClear(sa->split_suggestions);
  HaArrayAppend(sa->free_merge_suggestions, sa->merge_suggestions, i);
  HaArrayClear(sa->merge_suggestions);
  HaArrayAppend(sa->free_durn_info, sa->durn_info, i);
  HaArrayClear(sa->durn_info);
  sa->event = e;

  /* initialize durn_info, min_amount, and max_amount */
  max_durn = KheEventDuration(e);
  for( durn = 1;  durn <= max_durn;  durn++ )
    HaArrayAddLast(sa->durn_info, KheDurnInfoMake(sa, durn));
  sa->min_amount = 1;
  sa->max_amount = max_durn;

  /* accumulate limit info into durn_info, min_amount, and max_amount */
  for( i = 0;  i < KheSolnEventMonitorCount(sa->soln, e);  i++ )
  {
    m = KheSolnEventMonitor(sa->soln, e, i);
    if( KheMonitorCombinedWeight(m) > 0 )
      switch( KheMonitorTag(m) )
      {
	case KHE_SPLIT_EVENTS_MONITOR_TAG:

	  KheSplitEventsMonitorLimits((KHE_SPLIT_EVENTS_MONITOR) m,
	    &min_d, &max_d, &min_a, &max_a);
	  for( durn = 1;  durn < min_d && durn <= max_durn;  durn++ )
	  {
	    di = HaArray(sa->durn_info, durn - 1);
	    di->max_amount = 0;
	  }
	  for( durn = max_d + 1;  durn <= max_durn;  durn++ )
	  {
	    di = HaArray(sa->durn_info, durn - 1);
	    di->max_amount = 0;
	  }
	  if( min_a > sa->min_amount )
	    sa->min_amount = min_a;
	  if( max_a < sa->max_amount )
	    sa->max_amount = max_a;
	  break;

	case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

	  KheDistributeEventsMonitorLimits(
	    (KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m,
	    &durn, &min_a, &max_a, &meet_count);
	  if( durn >= 1 && durn <= max_durn )
	  {
	    di = HaArray(sa->durn_info, durn - 1);
	    if( min_a > di->min_amount )
              di->min_amount = min_a;
	    if( max_a < di->max_amount )
              di->max_amount = max_a;
	  }
	  break;

	default:

	  break;
      }
  }

  /* accumulate curr_amount info into durn_info records */
  for( i = 0;  i < KheEventMeetCount(sa->soln, e);  i++ )
  {
    durn = KheMeetDuration(KheEventMeet(sa->soln, e, i));
    di = HaArray(sa->durn_info, durn - 1);
    di->curr_amount++;
  }
  sa->curr_amount = KheEventMeetCount(sa->soln, e);

  /* make suggestions concerning specific durations */
  HaArrayForEach(sa->durn_info, di, i)
  {
    if( di->curr_amount < di->min_amount )
    {
      /* suggestions for more meets of duration di->duration */
      for( durn = di->duration + 1;  durn <= max_durn;  durn++ )
        KheTrySplit(sa, durn, di->duration);
      for( durn = 1;  durn < di->duration;  durn++ )
        KheTryMerge(sa, durn, di->duration - durn);
    }
    if( di->curr_amount > di->max_amount )
    {
      /* suggestions for fewer meets of duration di->duration */
      for( durn = 1;  durn < di->duration;  durn++ )
        KheTrySplit(sa, di->duration, durn);
      for( durn = di->duration + 1;  durn <= max_durn;  durn++ )
        KheTryMerge(sa, di->duration, durn - di->duration);
    }
  }

  if( sa->curr_amount < sa->min_amount )
  {
    /* suggestions for more meets of any duration (all possible splits) */
    for( durn = 2;  durn <= max_durn;  durn++ )
    {
      di = HaArray(sa->durn_info, durn - 1);
      if( di->curr_amount > di->min_amount )
	for( durn2 = 1;  durn2 < durn;  durn2++ )
	  KheTrySplit(sa, durn, durn2);
    }
  }
  if( sa->curr_amount > sa->max_amount )
  {
    /* suggestions for fewer meets of any duration (all possible merges) */
    for( durn = 1;  durn < max_durn;  durn++ )
      for( durn2 = durn;  durn2 <= max_durn - durn;  durn2++ )
	KheTryMerge(sa, durn, durn2);
  }

  /* sort and uniqueify the split and merge suggestions */
  HaArraySortUnique(sa->split_suggestions, &KheSplitSuggestionCmp);
  HaArraySortUnique(sa->merge_suggestions, &KheMergeSuggestionCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitAnalyserSplitSuggestionCount(KHE_SPLIT_ANALYSER sa)          */
/*                                                                           */
/*  Return the number of split suggestions.                                  */
/*                                                                           */
/*****************************************************************************/

int KheSplitAnalyserSplitSuggestionCount(KHE_SPLIT_ANALYSER sa)
{
  return HaArrayCount(sa->split_suggestions);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAnalyserSplitSuggestion(KHE_SPLIT_ANALYSER sa, int i,       */
/*    int *merged_durn, int *split1_durn)                                    */
/*                                                                           */
/*  Return the i'th split suggestion.                                        */
/*                                                                           */
/*****************************************************************************/

void KheSplitAnalyserSplitSuggestion(KHE_SPLIT_ANALYSER sa, int i,
  int *merged_durn, int *split1_durn)
{
  KHE_SPLIT_SUGGESTION ss;
  ss = HaArray(sa->split_suggestions, i);
  *merged_durn = ss->merged_durn;
  *split1_durn = ss->split1_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitAnalyserMergeSuggestionCount(KHE_SPLIT_ANALYSER sa)          */
/*                                                                           */
/*  Return the number of merge suggestions.                                  */
/*                                                                           */
/*****************************************************************************/

int KheSplitAnalyserMergeSuggestionCount(KHE_SPLIT_ANALYSER sa)
{
  return HaArrayCount(sa->merge_suggestions);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAnalyserMergeSuggestion(KHE_SPLIT_ANALYSER sa, int i,       */
/*    int *min_split1_durn, int *max_split1_durn,                            */
/*    int *min_split2_durn, int *max_split2_durn,                            */
/*    int *min_merged_durn, int *max_merged_durn)                            */
/*                                                                           */
/*  Return the i'th merge suggestion.                                        */
/*                                                                           */
/*****************************************************************************/

void KheSplitAnalyserMergeSuggestion(KHE_SPLIT_ANALYSER sa, int i,
  int *split1_durn, int *split2_durn)
{
  KHE_MERGE_SUGGESTION ms;
  ms = HaArray(sa->merge_suggestions, i);
  *split1_durn = ms->split1_durn;
  *split2_durn = ms->split2_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAnalyserDebug(KHE_SPLIT_ANALYSER sa, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug printf of sa onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheSplitAnalyserDebug(KHE_SPLIT_ANALYSER sa, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_SPLIT_SUGGESTION ss;  KHE_MERGE_SUGGESTION ms;  KHE_MEET meet;
  if( indent >= 0 && verbosity >= 1 )
  {
    if( sa->event == NULL )
      fprintf(fp, "%*s[ SplitAnalyser(no event) ]\n", indent, "");
    else
    {
      fprintf(fp, "%*s[ SplitAnalyser(%s)\n", indent, "",
	KheEventName(sa->event));
      for( i = 0;  i < KheEventMeetCount(sa->soln, sa->event);  i++ )
      {
	meet = KheEventMeet(sa->soln, sa->event, i);
	KheMeetDebug(meet, 2, indent + 2, fp);
      }
      HaArrayForEach(sa->split_suggestions, ss, i)
	fprintf(fp, "%*s  Split(%d -> %d + %d)\n", indent, "", ss->merged_durn,
	  ss->split1_durn, ss->merged_durn - ss->split1_durn);
      HaArrayForEach(sa->merge_suggestions, ms, i)
	fprintf(fp, "%*s  Merge(%d + %d -> %d)\n", indent, "", ms->split1_durn,
	  ms->split2_durn, ms->split1_durn + ms->split2_durn);
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

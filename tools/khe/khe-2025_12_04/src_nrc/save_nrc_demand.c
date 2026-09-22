
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         nrc_demand.c                                               */
/*  MODULE:       Demands                                                    */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 1


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY_GROUP                                                        */
/*                                                                           */
/*  A group of workers who should *not* be penalized by p;                   */
/*  these become the preferred workers in a prefer resources constraint.     */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct nrc_penalty_group_rec {
  NRC_PENALTY		penalty;
  NRC_WORKER_SET	worker_set;
} *NRC_PENALTY_GROUP;

typedef HA_ARRAY(NRC_PENALTY_GROUP) ARRAY_NRC_PENALTY_GROUP;
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PREFERENCE                                                           */
/*                                                                           */
/*  A record of one call saying how assignments to a demand should be        */
/*  penalized.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct nrc_preference_rec {
  NRC_WORKER_SET		prefer_ws;		** prefer ws         **
  bool				prefer_nonasst;		** prefer non-asst   **
  NRC_PENALTY			penalty;		** penalty           **
} *NRC_PREFERENCE;

typedef HA_ARRAY(NRC_PREFERENCE) ARRAY_NRC_PREFERENCE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_PENALTY                                                       */
/*                                                                           */
/*  The penalty for assigning a given worker (or non-assignment)             */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_worker_penalty_rec {
  NRC_WORKER			worker;			/* the worker        */
  NRC_PENALTY			penalty;		/* the penalty       */
} *NRC_WORKER_PENALTY;

typedef HA_ARRAY(NRC_WORKER_PENALTY) ARRAY_NRC_WORKER_PENALTY;


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALIZER                                                             */
/*                                                                           */
/*  A record of one call saying how assignments to a demand should be        */
/*  penalized.                                                               */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_penalizer_rec {
  NRC_PENALIZER_TYPE		type;			/* penalizer type    */
  NRC_WORKER_SET		worker_set;		/* worker set        */
  NRC_PENALTY_TYPE		penalty_type;		/* penalty type      */
  NRC_PENALTY			penalty;		/* penalty           */
} *NRC_PENALIZER;

typedef HA_ARRAY(NRC_PENALIZER) ARRAY_NRC_PENALIZER;


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND - a demand for one worker                                     */
/*                                                                           */
/*****************************************************************************/

struct nrc_demand_rec {
  NRC_INSTANCE			instance;
  char				*name;
  /* ARRAY_NRC_PREFERENCE	preferences; */
  ARRAY_NRC_PENALIZER		penalizers;
  NRC_SHIFT			prev_shift;
  int				prev_shift_index;
  int				max_shift_index;
  NRC_SHIFT_SET			shift_set;
  ARRAY_NRC_WORKER_PENALTY	worker_penalties;
};

/* ***
struct nrc_demand_rec {
  NRC_INSTANCE			instance;
  char				*name;
  NRC_PENALTY			not_assigned_penalty;
  NRC_PENALTY			assigned_penalty;
  NRC_WORKER_SET		preferred_ws;
  NRC_PENALTY			not_preferred_penalty;
  NRC_SHIFT			prev_shift;
  int				prev_shift_index;
  int				max_shift_index;
  NRC_SHIFT_SET			shift_set;
  ARRAY_NRC_PENALTY_GROUP	penalty_groups;
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "penalizers"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALIZER NrcPenalizerMake(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,      */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Make a new penalizer object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static NRC_PENALIZER NrcPenalizerMake(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  NRC_PENALIZER res;
  HaMake(res, NrcInstanceArena(d->instance));
  res->type = pt;
  res->worker_set = ws;
  res->penalty_type = ptype;
  res->penalty = p;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preferences"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_PREFERENCE NrcPreferenceMake(NRC_DEMAND d,                           */
/*    NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)          */
/*                                                                           */
/*  Make a new preference object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static NRC_PREFERENCE NrcPreferenceMake(NRC_DEMAND d,
  NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)
{
  NRC_PREFERENCE res;
  HaMake(res, NrcInstanceArena(d->instance));
  res->prefer_ws = prefer_ws;
  res->prefer_nonasst = prefer_nonasst;
  res->penalty = p;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPreferenceIsEffective(NRC_WORKER_SET prefer_ws,                  */
/*    bool prefer_nonasst, NRC_PENALTY p)                                    */
/*                                                                           */
/*  Return true when a preference with these attributes is effective,        */
/*  that is, makes a difference.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcPreferenceIsEffective(NRC_WORKER_SET prefer_ws,
  bool prefer_nonasst, NRC_PENALTY p)
{
  NRC_INSTANCE ins;

  ** ineffective if penalty weight is 0 **
  if( NrcPenaltyWeight(p) == 0 )
    return false;

  ** ineffective if there is no case where the penalty applies **
  ins = NrcWorkerSetInstance(prefer_ws);
  if( NrcWorkerSetWorkerCount(prefer_ws) == NrcInstanceStaffingWorkerCount(ins)
      && prefer_nonasst )
    return false;

  ** otherwise effective **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcPreferenceTypedCmp(NRC_PREFERENCE p1, NRC_PREFERENCE p2)          */
/*                                                                           */
/*  Typed comparison function for sorting preferences into canonical order.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static int NrcPreferenceTypedCmp(NRC_PREFERENCE p1, NRC_PREFERENCE p2)
{
  char *name1, *name2;  int cmp;
  name1 = NrcWorkerSetName(p1->prefer_ws);
  name2 = NrcWorkerSetName(p2->prefer_ws);
  cmp = strcmp(name1, name2);
  if( cmp != 0 )
    return cmp;
  else if( p1->prefer_nonasst != p2->prefer_nonasst )
    return (int) p1->prefer_nonasst - (int) p2->prefer_nonasst;
  else
    return NrcPenaltyTypedCmp(p1->penalty, p2->penalty);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcPreferenceCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Untyped comparison function for sorting preferences.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static int NrcPreferenceCmp(const void *t1, const void *t2)
{
  NRC_PREFERENCE p1 = * (NRC_PREFERENCE *) t1;
  NRC_PREFERENCE p2 = * (NRC_PREFERENCE *) t2;
  return NrcPreferenceTypedCmp(p1, p2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandAddPreference(NRC_DEMAND d,                                */
/*    NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)          */
/*                                                                           */
/*  Add a preference with these attributes to d, unless ineffective.         */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcDemandAddPreference(NRC_DEMAND d,
  NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p)
{
  if( NrcPreferenceIsEffective(prefer_ws, prefer_nonasst, p) )
    HaArrayAddLast(d->preferences,
      NrcPreferenceMake(d, prefer_ws, prefer_nonasst, p));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandAddPenalize(NRC_DEMAND d, NRC_PENALIZER_TYPE type,         */
/*    NRC_WORKER_SET ws, NRC_PENALTY p)                                      */
/*                                                                           */
/*  Add a penalize with these attributes to d.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcDemandAddPenalize(NRC_DEMAND d, NRC_PENALIZER_TYPE type,
  NRC_WORKER_SET ws, NRC_PENALTY p)
{
  NRC_PENALIZER res;

  ** make the new object **
  HaMake(res, NrcInstanceArena(d->instance));
  res->type = type;
  res->worker_set = ws;
  res->penalty = p;

  ** add it to d **
  HaArrayAddLast(d->preferences, res);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandAddPenalizer(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,          */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Add a penalizer with these attributes to d.                              */
/*                                                                           */
/*****************************************************************************/

void NrcDemandAddPenalizer(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  NRC_PENALIZER pr;
  pr = NrcPenalizerMake(d, pt, ws, ptype, p);
  HaArrayAddLast(d->penalizers, pr);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcDemandMake(NRC_INSTANCE ins)                               */
/*                                                                           */
/*  Return a new demand object with no preferences.                          */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND NrcDemandMake(NRC_INSTANCE ins)
{
  NRC_DEMAND res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = NULL;
  HaArrayInit(res->penalizers, a);
  res->prev_shift = NULL;
  res->prev_shift_index = 0;
  res->max_shift_index = 0;
  res->shift_set = NrcShiftSetMake(ins);
  HaArrayInit(res->worker_penalties, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_PENALTY NrcWorkerPenaltyMake(NRC_WORKER w)                    */
/*                                                                           */
/*  Make a new worker penalty object, with initial penalty zero.             */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_PENALTY NrcWorkerPenaltyMake(NRC_WORKER w, NRC_INSTANCE ins)
{
  NRC_WORKER_PENALTY res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->worker = w;
  res->penalty = NrcInstanceZeroPenalty(ins);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerPenaltyAddPenalty(NRC_WORKER_PENALTY wp, NRC_PENALTY p)    */
/*                                                                           */
/*  Add p to wp.  NB penalty objects are immutable after creation.           */
/*                                                                           */
/*****************************************************************************/

/* *** using NrcPenaltyAdd now
static void NrcWorkerPenaltyAddPenalty(NRC_WORKER_PENALTY wp, NRC_PENALTY p,
  NRC_INSTANCE ins)
{
  int weight;
  if( NrcPenaltyWeight(wp->penalty) == 0 )
  {
    ** wp has no penalty yet, so use p **
    wp->penalty = p;
  }
  else if( NrcPenaltyHard(wp->penalty) )
  {
    if( NrcPenaltyHard(p) )
    {
      ** both are hard, so add their weights **
      weight = NrcPenaltyWeight(wp->penalty) + NrcPenaltyWeight(p);
      wp->penalty = NrcPenalty(true, weight, NRC_COST_FUNCTION_LINEAR, ins);
    }
    else
    {
      ** wp is hard and p is soft, so no change **
      ** do nothing here **
    }
  }
  else
  {
    if( NrcPenaltyHard(p) )
    {
      ** wp is soft and p is hard, so use p **
      wp->penalty = p;
    }
    else
    {
      ** both are soft, so add their weights **
      weight = NrcPenaltyWeight(wp->penalty) + NrcPenaltyWeight(p);
      wp->penalty = NrcPenalty(false, weight, NRC_COST_FUNCTION_LINEAR, ins);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerPenaltyTypedCmp(NRC_WORKER_PENALTY wp1,                     */
/*    NRC_WORKER_PENALTY wp2)                                                */
/*                                                                           */
/*  Comparison function for bringing worker penalties with equal             */
/*  penalties together, except that non-assignment is always separate.       */
/*                                                                           */
/*****************************************************************************/

static int NrcWorkerPenaltyTypedCmp(NRC_WORKER_PENALTY wp1,
  NRC_WORKER_PENALTY wp2)
{
  int cmp;
  cmp = (int) (wp1->worker != NULL) - (int) (wp2->worker != NULL);
  if( cmp != 0 )  return cmp;
  return NrcPenaltyTypedCmp(wp1->penalty, wp2->penalty);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerPenaltyCmp(const void *p1, const void *p2)                  */
/*                                                                           */
/*  Untyped version of NrcWorkerPenaltyTypedCmp.                             */
/*                                                                           */
/*****************************************************************************/

static int NrcWorkerPenaltyCmp(const void *p1, const void *p2)
{
  NRC_WORKER_PENALTY wp1 = * (NRC_WORKER_PENALTY *) p1;
  NRC_WORKER_PENALTY wp2 = * (NRC_WORKER_PENALTY *) p2;
  return NrcWorkerPenaltyTypedCmp(wp1, wp2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerPenaltyDebug(NRC_WORKER_PENALTY wp, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of wp onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void NrcWorkerPenaltyDebug(NRC_WORKER_PENALTY wp, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "(%s, %c%d)",
    wp->worker == NULL ? "-" : NrcWorkerName(wp->worker),
    NrcPenaltyHard(wp->penalty) ? 'h' : 's', NrcPenaltyWeight(wp->penalty));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "penalty groups (private)"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY_GROUP NrcPenaltyGroupMake(NRC_DEMAND d, NRC_PENALTY p)       */
/*                                                                           */
/*  Make, add, and return a penalty group object with these attributes.      */
/*                                                                           */
/*****************************************************************************/

/* ***
static NRC_PENALTY_GROUP NrcPenaltyGroupMake(NRC_DEMAND d, NRC_PENALTY p)
{
  NRC_PENALTY_GROUP res;  char *name;  HA_ARENA a;
  a = NrcInstanceArena(d->instance);
  if( DEBUG2 )
    fprintf(stderr, "[ NrcPenaltyGroupMake(%s, %s)\n", d->name,
      NrcPenaltyShow(p, a));
  HaMake(res, a);
  res->penalty = p;
  name = HnStringMake(a, "%s-Or-UnpreferredNot=%s",
    NrcWorkerSetName(d->preferred_ws), NrcPenaltyId(p));
  if( !NrcInstanceRetrieveWorkerSet(d->instance, name, &res->worker_set) )
    res->worker_set = NrcWorkerSetMake(d->instance, name);
  HaArrayAddLast(d->penalty_groups, res);
  if( DEBUG2 )
    fprintf(stderr, "] res = %p, res->worker_set = %p\n",
      (void *) res, (void *) res->worker_set);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY_GROUP NrcPenaltyGroupDefaultMake(NRC_DEMAND d, NRC_PENALTY p)*/
/*                                                                           */
/*  Make, add, and return a default penalty weight object with these         */
/*  attributes.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static NRC_PENALTY_GROUP NrcPenaltyGroupDefaultMake(NRC_DEMAND d, NRC_PENALTY p)
{
  NRC_PENALTY_GROUP res;  HA_ARENA a;
  a = NrcInstanceArena(d->instance);
  if( DEBUG2 )
    fprintf(stderr, "[ NrcPenaltyGroupDefaultMake(%s, %s)\n", d->name,
      NrcPenaltyShow(p, a));
  HaMake(res, a);
  res->penalty = p;
  res->worker_set = d->preferred_ws;
  HaArrayAddLast(d->penalty_groups, res);
  if( DEBUG2 )
    fprintf(stderr, "] res = %p, res->worker_set = %p\n",
      (void *) res, (void *) res->worker_set);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPenaltyGroupRetrieve(NRC_DEMAND d, NRC_PENALTY p,                */
/*    NRC_PENALTY_GROUP *pg)                                                 */
/*                                                                           */
/*  Retrieve a penalty group with this penalty from d, if present.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcPenaltyGroupRetrieve(NRC_DEMAND d, NRC_PENALTY p,
  NRC_PENALTY_GROUP *pg)
{
  int i;  NRC_PENALTY_GROUP pg2;
  HaArrayForEach(d->penalty_groups, pg2, i)
    if( NrcPenaltyEqual(pg2->penalty, p) )
    {
      *pg = pg2;
      return true;
    }
  *pg = NULL;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcPenaltyGroupDebug(NRC_PENALTY_GROUP pg, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of pg onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void NrcPenaltyGroupDebug(NRC_PENALTY_GROUP pg, int indent, FILE *fp)
{
  int i;  NRC_WORKER w;  HA_ARENA a;
  a = NrcInstanceArena(NrcWorkerSetInstance(pg->worker_set));
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ PenaltyGroup(%s)", indent, "",
      NrcPenaltyShow(pg->penalty, a));
    for( i = 0;  i < NrcWorkerSetWorkerCount(pg->worker_set);  i++ )
    {
      w = NrcWorkerSetWorker(pg->worker_set, i);
      if( i % 5 == 0 )
	fprintf(fp, "%s\n%*s", i == 0 ? "" : ",", indent + 4, "");
      else
	fprintf(fp, ", ");
      fprintf(fp, "%s", NrcWorkerConvertedName(w));
    }
    fprintf(fp, "\n%*s]\n", indent, "");
  }
  else
    fprintf(fp, "PenaltyGroup(%s, %s)", NrcPenaltyShow(pg->penalty, a),
      NrcWorkerSetName(pg->worker_set));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandBuildPenaltyGroups(NRC_DEMAND d)                           */
/*                                                                           */
/*  Build the penalty groups for d.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void NrcDemandBuildPenaltyGroups(NRC_DEMAND d)
{
  bool ok;  int i, j;  NRC_WORKER w, w2;  NRC_PENALTY_GROUP pg;
  NRC_PENALTY p;

  if( DEBUG1 )
    fprintf(stderr, "[ NrcDemandBuildPenaltyGroups(%s)\n", d->name);

  if( NrcInstanceUniqueSkillPenalty(d->instance, &p) )
  {
    ** all workers have the same skill penalty, so we need one penalty group **
    if( p == NULL )
      p = d->not_preferred_penalty;
    pg = NrcPenaltyGroupDefaultMake(d, p);
  }
  else
  {
    ** build one penalty group for each distinct non-zero penalty **
    for( i = 0;  i < NrcInstanceStaffingWorkerCount(d->instance);  i++ )
    {
      ** get one worker w and its skill penalty, possibly the default **
      w = NrcInstanceStaffingWorker(d->instance, i);
      p = NrcWorkerSki llPenalty(w);
      if( p == NULL )
	p = d->not_preferred_penalty;
      if( DEBUG1 )
      {
	HA_ARENA a = NrcInstanceArena(d->instance);
	fprintf(stderr, "  worker %s: %s\n", NrcWorkerConvertedName(w),
	  NrcPenaltyShow(p, a));
      }

      ** make sure there is a penalty group with this penalty, if non-zero **
      if( NrcPenaltyWeight(p) > 0 && !NrcPenaltyGroupRetrieve(d, p, &pg) )
	pg = NrcPenaltyGroupMake(d, p);
    }

    ** add each worker to each penalty group as required **
    for( i = 0;  i < NrcInstanceStaffingWorkerCount(d->instance);  i++ )
    {
      ** get one worker w and its skill penalty, possibly the default **
      w = NrcInstanceStaffingWorker(d->instance, i);
      p = NrcWorkerSkil lPenalty(w);
      if( p == NULL )
	p = d->not_preferred_penalty;

      ** add w to each penalty group it qualifies for, either because it **
      ** is in preferred_ws, or because its penalty differs from the group's **
      ok = NrcWorkerSetRetrieveWorker(d->preferred_ws, NrcWorkerName(w), &w2);
      HaArrayForEach(d->penalty_groups, pg, j)
	if( ok || (pg != NULL && !NrcPenaltyEqual(p, pg->penalty)) )
	  if( !NrcWorkerSetContainsWorker(pg->worker_set, w) )
	    NrcWorkerSetAddWorker(pg->worker_set, w);
    }
  }
  if( DEBUG1 )
  {
    HaArrayForEach(d->penalty_groups, pg, j)
      NrcPenaltyGroupDebug(pg, 2, stderr);
    fprintf(stderr, "]\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandName(NRC_DEMAND d)                                        */
/*                                                                           */
/*  Make the name of d based on d's preferences.  Cache it in d.              */
/*                                                                           */
/*****************************************************************************/

char *NrcDemandName(NRC_DEMAND d)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  NRC_PENALIZER pr;  int i, wcount;
  char *name;
  if( d->name == NULL )
  {
    a = NrcInstanceArena(d->instance);
    HnStringBegin(anc, a);
    if( HaArrayCount(d->penalizers) == 0 )
      HnStringAdd(&anc, "X");
    else
    {
      wcount = NrcInstanceStaffingWorkerCount(d->instance);
      /* HaArraySort(d->penalizers, &NrcPreferenceCmp); */
      HaArrayForEach(d->penalizers, pr, i)
      {
	if( i > 0 )
	  HnStringAdd(&anc, ":");
	switch( pr->type )
	{
	  case NRC_PENALIZER_NON_ASSIGNMENT:

	    HnStringAdd(&anc, "NA=%s", NrcPenaltyId(pr->penalty));
	    break;

	  case NRC_PENALIZER_WORKER_SET:

	    if( NrcWorkerSetWorkerCount(pr->worker_set) == 0 )
	    {
	      /* no workers: ineffective */
	    }
	    else if( NrcWorkerSetWorkerCount(pr->worker_set) == wcount )
	    {
	      /* all workers */
	      HnStringAdd(&anc, "W=%s", NrcPenaltyId(pr->penalty));
	    }
	    else
	    {
	      /* some workers */
	      name = NrcWorkerSetName(pr->worker_set);
	      if( strstr(name, "Skill-") == name )
		name = &name[strlen("Skill-")];
	      HnStringAdd(&anc, "W%s=%s", name, NrcPenaltyId(pr->penalty));
	    }
	    break;

	  case NRC_PENALIZER_NOT_WORKER_SET:

	    if( NrcWorkerSetWorkerCount(pr->worker_set) == 0 )
	    {
	      /* no workers */
	      HnStringAdd(&anc, "NW=%s", NrcPenaltyId(pr->penalty));
	    }
	    else if( NrcWorkerSetWorkerCount(pr->worker_set) == wcount )
	    {
	      /* all workers: ineffective */
	    }
	    else
	    {
	      name = NrcWorkerSetName(pr->worker_set);
	      if( strstr(name, "Skill-") == name )
		name = &name[strlen("Skill-")];
	      HnStringAdd(&anc, "NW%s=%s", name, NrcPenaltyId(pr->penalty));
	    }
	    break;

	  default:

	    HnAbort("NrcDemandName: unknown penalizer type %d", pr->type);
	}
      }
    }
    d->name = HnStringEnd(anc);
  }
  return d->name;
}


/* ***
char *NrcDemandName(NRC_DEMAND d)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  NRC_PENALIZER dp;  int i;  char *name;
  if( d->name == NULL )
  {
    a = NrcInstanceArena(d->instance);
    HnStringBegin(anc, a);
    if( HaArrayCount(d->preferences) == 0 )
      HnStringAdd(&anc, "X");
    else
    {
      HaArrayForEach(d->preferences, dp, i)
      {
	if( i > 0 )
	  HnStringAdd(&anc, ":");
	switch( dp->type )
	{
	  case NRC_PENALIZER_WORKER_SET:

	    name = NrcWorkerSetName(dp->worker_set);
	    if( strstr(name, "Skill-") == name )
	      name = &name[strlen("Skill-")];
	    HnStringAdd(&anc, "A-%s=%s", name, NrcPenaltyId(dp->penalty));
	    break;

	  case NRC_PENALIZER_NOT_WORKER_SET:

	    name = NrcWorkerSetName(dp->worker_set);
	    if( strstr(name, "Skill-") == name )
	      name = &name[strlen("Skill-")];
	    HnStringAdd(&anc, "N-%s=%s", name, NrcPenaltyId(dp->penalty));
	    break;

	  case NRC_PENALIZER_ASSIGNMENT:

	    HnStringAdd(&anc, "A=%s", NrcPenaltyId(dp->penalty));
	    break;

	  case NRC_PENALIZER_NON_ASSIGNMENT:

	    HnStringAdd(&anc, "N=%s", NrcPenaltyId(dp->penalty));
	    break;

	  default:

	    HnAbort("NrcDemandName: unexpected penalizer type (%d)", dp->type);
	}
      }
    }
    d->name = HnStringEnd(anc);
  }
  return d->name;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *N rcDemandMakeName(NRC_PENALTY not_assigned_penalty,                */
/*    NRC_PENALTY assigned_penalty, NRC_WORKER_SET preferred_ws,             */
/*    NRC_PENALTY not_preferred_penalty)                                     */
/*                                                                           */
/*  Return the name of a demand with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *N rcDemandMakeName(NRC_PENALTY not_assigned_penalty,
  NRC_PENALTY assigned_penalty, NRC_WORKER_SET preferred_ws,
  NRC_PENALTY not_preferred_penalty, HA_ARENA a)
{
  char buff1[100], buff3[100], *buff2, *name, *p;  char *items[3];
  int item_count;
  item_count = 0;

  ** sort out the part of the name relating to being not assigned **
  if( NrcPenaltyWeight(not_assigned_penalty) > 0 )
  {
    sprintf(buff1, "A=%s", NrcPenaltyId(not_assigned_penalty));
    items[item_count++] = buff1;
  }

  ** sort out the part of the name relating to preferred workers **
  if( preferred_ws != NULL )
  {
    p = NrcWorkerSetName(preferred_ws);
    if( strstr(p, "Skill-") == p )
      name = &p[strlen("Skill-")];
    else
      name = p;
    buff2 = HnStringMake(a, "P-%s=%s", name,
      NrcPenaltyId(not_preferred_penalty));
    items[item_count++] = buff2;
  }

  ** sort out the part of the name relating to being assigned **
  if( NrcPenaltyWeight(assigned_penalty) > 0 )
  {
    sprintf(buff3, "U=%s", NrcPenaltyId(assigned_penalty));
    items[item_count++] = buff3;
  }

  ** return the name **
  switch( item_count )
  {
    case 0:	return HnStringMake(a, "X");
    case 1:	return HnStringMake(a, "%s", items[0]);
    case 2:	return HnStringMake(a, "%s:%s", items[0], items[1]);
    case 3:	return HnStringMake(a, "%s:%s:%s", items[0], items[1],items[2]);

    default:

      HnAbort("N rcDemandMakeName internal error");
      return NULL;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND N rcDemandMake(NRC_INSTANCE ins, bool not_assigned_reqd,       */
/*    int not_assigned_weight, bool assigned_reqd, int assigned_weight,      */
/*    NRC_WORKER_SET preferred_ws, bool not_preferred_reqd,                  */
/*    int not_preferred_weight)                                              */
/*                                                                           */
/*  Either make a new role or retrieve an existing role with these           */
/*  attributes.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_DEMAND Nr cDemandMake(NRC_INSTANCE ins,
  NRC_PENALTY not_assigned_penalty, NRC_PENALTY assigned_penalty,
  NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_penalty)
{
  char *name;  NRC_DEMAND res;  HA_ARENA a;

  ** make sure all penalties are non-NULL **
  HnAssert(not_assigned_penalty != NULL,
    "Nr cDemandMake: NULL not_assigned_penalty");
  HnAssert(assigned_penalty != NULL, "N rcDemandMake: NULL assigned_penalty");
  HnAssert(not_preferred_penalty != NULL || preferred_ws == NULL,
    "N rcDemandMake: NULL not_preferred_penalty");

  ** normalize the parameters **
  if( preferred_ws == NULL )
    not_preferred_penalty = NrcInstanceZeroPenalty(ins);

  ** make the name **
  a = NrcInstanceArena(ins);
  name = N rcDemandMakeName(not_assigned_penalty, assigned_penalty,
    preferred_ws, not_preferred_penalty, a);

  ** retrieve or make the demand object, and return it **
  if( !NrcInstanceRetrieveDemand(ins, name, &res) )
  {
    HaMake(res, a);
    res->instance = ins;
    res->name = name;
    res->not_assigned_penalty = not_assigned_penalty;
    res->assigned_penalty = assigned_penalty;
    res->preferred_ws = preferred_ws;
    res->not_preferred_penalty = not_preferred_penalty;
    res->prev_shift = NULL;
    res->prev_shift_index = 0;
    res->max_shift_index = 0;
    res->shift_set = NrcShiftSetMake(ins);
    HaArrayInit(res->penalty_groups, a);
    NrcInstanceAddDemand(ins, res);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDemandInstance(NRC_DEMAND d)                             */
/*                                                                           */
/*  Return the instance attribute of d.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDemandInstance(NRC_DEMAND d)
{
  return d->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandPreferenceCount(NRC_DEMAND d)                               */
/*                                                                           */
/*  Return the number of effective preferences of d.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
int NrcDemandPreferenceCount(NRC_DEMAND d)
{
  return HaArrayCount(d->preferences);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPreference(NRC_DEMAND d, int i,                            */
/*    NRC_WORKER_SET *prefer_ws, bool *prefer_nonasst, NRC_PENALTY *p)       */
/*                                                                           */
/*  Return the i'th effective preference of d.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcDemandPreference(NRC_DEMAND d, int i,
  NRC_WORKER_SET *prefer_ws, bool *prefer_nonasst, NRC_PENALTY *p)
{
  NRC_PREFERENCE dp;
  dp = HaArray(d->preferences, i);
  *prefer_ws = dp->prefer_ws;
  *prefer_nonasst = dp->prefer_nonasst;
  *p = dp->penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandPenalizerCount(NRC_DEMAND d)                                */
/*                                                                           */
/*  Return the number of preferences of d.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcDemandPenalizerCount(NRC_DEMAND d)
{
  return HaArrayCount(d->penalizers);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPenalizer(NRC_DEMAND d, int i, NRC_PENALIZER_TYPE *pt,     */
/*    NRC_WORKER_SET *ws, NRC_PENALTY_TYPE *ptype, NRC_PENALTY *p)           */
/*                                                                           */
/*  Return the fields of the i'th penalizer of d.                            */
/*                                                                           */
/*****************************************************************************/

void NrcDemandPenalizer(NRC_DEMAND d, int i, NRC_PENALIZER_TYPE *pt,
  NRC_WORKER_SET *ws, NRC_PENALTY_TYPE *ptype, NRC_PENALTY *p)
{
  NRC_PENALIZER pr;
  pr = HaArray(d->penalizers, i);
  *pt = pr->type;
  *ws = pr->worker_set;
  *ptype = pr->penalty_type;
  *p = pr->penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcDemandNotAssignedPenalty(NRC_DEMAND d)                    */
/*                                                                           */
/*  Return the not_assigned_penalty attribute of d.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY NrcDemandNotAssignedPenalty(NRC_DEMAND d)
{
  return d->not_assigned_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcDemandAssignedPenalty(NRC_DEMAND d)                       */
/*                                                                           */
/*  Return the assigned_penalty attribute of d.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY NrcDemandAssignedPenalty(NRC_DEMAND d)
{
  return d->assigned_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcDemandPreferredWorkerSet(NRC_DEMAND d)                 */
/*                                                                           */
/*  Return the preferred_ws attribute of d.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_WORKER_SET NrcDemandPreferredWorkerSet(NRC_DEMAND d)
{
  return d->preferred_ws;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcDemandNotPreferredPenalty(NRC_DEMAND d)                   */
/*                                                                           */
/*  Return the not_preferred_reqd attribute of d.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY NrcDemandNotPreferredPenalty(NRC_DEMAND d)
{
  return d->not_preferred_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandName(NRC_DEMAND d)                                        */
/*                                                                           */
/*  Return the name attribute of d.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
char *NrcDemandName(NRC_DEMAND d)
{
  return d->name;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandCanUseDemands(NRC_DEMAND d)                                */
/*                                                                           */
/*  Return true if d is convertible to a demand from a demand constraint.    */
/*                                                                           */
/*****************************************************************************/

/* ***
bool NrcDemandCanUseDemands(NRC_DEMAND d)
{
  return NrcPenaltyWeight(d->not_assigned_penalty) == 0 &&
    NrcPenaltyWeight(d->assigned_penalty) == 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandRoleMake(NRC_DEMAND d, int shift_index)                   */
/*                                                                           */
/*  Make a role name for demand d with index.                                */
/*                                                                           */
/*****************************************************************************/

static char *NrcDemandRoleMake(NRC_DEMAND d, int index)
{
  HA_ARENA a = NrcInstanceArena(d->instance);
  return HnStringMake(a, "%s:%d", d->name, index);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConvertToEventResource(NRC_DEMAND d, NRC_SHIFT s,          */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Convert d into an event resource for s, whose KHE event is e.            */
/*                                                                           */
/*  Implementation note.  This function assumes that all calls for a         */
/*  given shift s occur together, not separated by calls for other           */
/*  shifts, as is indeed the case.                                           */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConvertToEventResource(NRC_DEMAND d, NRC_SHIFT s,
  NRC_WORKER preasst, KHE_EVENT e)
{
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_RESOURCE er;  char *role;
  KHE_RESOURCE r;

  /* sort out prev_shift, prev_shift_index, and max_shift_index */
  if( d->prev_shift == s )
    d->prev_shift_index++;
  else
  {
    d->prev_shift = s;
    d->prev_shift_index = 1;
    NrcShiftSetAddShift(d->shift_set, s);
  }
  if( d->max_shift_index < d->prev_shift_index )
    d->max_shift_index = d->prev_shift_index;

  /* generate the event resource */
  rt = KheInstanceResourceType(KheEventInstance(e), 0);
  r = (preasst == NULL ? NULL : NrcWorkerResource(preasst));
  role = NrcDemandRoleMake(d, d->prev_shift_index);
  if( !KheEventResourceMake(e, rt, r, role, KheEventWorkload(e), &er) )
    HnAbort("NrcDemandConvertToEventResource internal error: %s", role);
}


/*****************************************************************************/
/*                                                                           */
/*  void WorkerPenaltyAddPenalizer(NRC_WORKER_PENALTY wp, NRC_PENALIZER pr)  */
/*                                                                           */
/*  Add pr to wp.                                                            */
/*                                                                           */
/*****************************************************************************/

static void WorkerPenaltyAddPenalizer(NRC_WORKER_PENALTY wp, NRC_PENALIZER pr,
  NRC_INSTANCE ins)
{
  switch( pr->penalty_type )
  {
    case NRC_PENALTY_REPLACE:

      wp->penalty = pr->penalty;
      break;

    case NRC_PENALTY_ADD:

      wp->penalty = NrcPenaltyAdd(wp->penalty, pr->penalty, ins);
      /* NrcWorkerPenaltyAddPenalty(wp, pr->penalty, ins); */
      break;

    case NRC_PENALTY_UNIQUE:

      HnAssert(NrcPenaltyWeight(wp->penalty) == 0,
	"WorkerPenaltyAddPenalizer: unique penalty is replacement");
      wp->penalty = pr->penalty;
      break;

    default:

      HnAbort("WorkerPenaltyAddPenalizer internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConvertToEventResourceConstraints(NRC_DEMAND d,            */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Convert d into all corresponding event resources constraints.            */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConvertToEventResourceConstraints(NRC_DEMAND d, KHE_INSTANCE ins)
{
  NRC_WORKER w;  NRC_PENALIZER pr;  NRC_WORKER_PENALTY wp, wp2;
  int i, j, k, index;  char *role, *id, *name;  HA_ARENA a;
  KHE_ASSIGN_RESOURCE_CONSTRAINT karc;  KHE_PREFER_RESOURCES_CONSTRAINT kprc;
  KHE_EVENT e;  KHE_EVENT_GROUP eg;  KHE_RESOURCE r, r2;

  if( DEBUG4 )
    fprintf(stderr, "[ NrcDemandConvertTo...(%s, %s)\n",
      NrcDemandName(d), KheInstanceId(ins));

  /* get the event group containing the shifts that contain d */
  a = NrcInstanceArena(d->instance);
  id = HnStringMake(a, "EG:%s", d->name);
  eg = NrcShiftSetEventGroup(d->shift_set, id, ins);

  /* build the array of worker penalties, including non-assignment */
  for( i = 0;  i < NrcInstanceStaffingWorkerCount(d->instance);  i++ )
  {
    w = NrcInstanceStaffingWorker(d->instance, i);
    HaArrayAddLast(d->worker_penalties, NrcWorkerPenaltyMake(w, d->instance));
  }
  HaArrayAddLast(d->worker_penalties, NrcWorkerPenaltyMake(NULL, d->instance));

  /* add in all the penalties from all the preferences */
  HaArrayForEach(d->penalizers, pr, i)
    switch( pr->type )
    {
      case NRC_PENALIZER_NON_ASSIGNMENT:

	wp = HaArrayLast(d->worker_penalties);
	WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	break;

      case NRC_PENALIZER_WORKER_SET:

	for( j = 0;  j < NrcWorkerSetWorkerCount(pr->worker_set);  j++ )
	{
	  w = NrcWorkerSetWorker(pr->worker_set, j);
	  wp = HaArray(d->worker_penalties, NrcWorkerIndex(w));
	  WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	}
	break;

      case NRC_PENALIZER_NOT_WORKER_SET:

	for( i = 0;  i < NrcInstanceStaffingWorkerCount(d->instance);  i++ )
	{
	  w = NrcInstanceStaffingWorker(d->instance, i);
	  if( !NrcWorkerSetContainsWorker(pr->worker_set, w) )
	  {
	    wp = HaArray(d->worker_penalties, NrcWorkerIndex(w));
	    WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	  }
	}
	break;

      default:

	HnAbort("NrcDemandConvertToEventResourceConstraints internal error");
	break;
    }

  /* sort the worker penalties to bring equal penalties together */
  HaArraySort(d->worker_penalties, &NrcWorkerPenaltyCmp);

  /* handle the worker penalties, starting by grouping them by equal penalty */
  for( i = 0;  i < HaArrayCount(d->worker_penalties);  i = j )
  {
    /* build a group of worker penalties with the same penalty as wp */
    wp = HaArray(d->worker_penalties, i);
    for( j = i + 1;  j < HaArrayCount(d->worker_penalties);  j++ )
    {
      wp2 = HaArray(d->worker_penalties, i);
      if( NrcWorkerPenaltyTypedCmp(wp, wp2) != 0 )
	break;
    }

    /* ignore this group if its penalty is 0 */
    if( wp->penalty == 0 )
      continue;

    /* here d->worker_penalties[i .. j-1] form one group */
    if( DEBUG4 )
    {
      fprintf(stderr, "  [ group %d .. %d:\n", i, j - 1);
      for( k = i;  k < j;  k++ )
      {
	wp2 = HaArray(d->worker_penalties, k);
	NrcWorkerPenaltyDebug(wp2, 4, stderr);
      }
    }

    /* one constraint for each role that d could take in any shift */
    for( index = 1;  index <= d->max_shift_index;  index++ )
    {
      role = NrcDemandRoleMake(d, index);
      if( wp->worker == NULL )
      {
	/* penalize non-assignment using an XESTT assign resource constraint */
	id = HnStringMake(a, "ARC:%s", role);
	name = "Assign resource";
	if( !KheAssignResourceConstraintMake(ins, id, name,
	    NrcPenaltyConvert(wp->penalty), role, &karc) )
	  HnAbort("cannot add %s constraint", id);
	KheAssignResourceConstraintAddEventGroup(karc, eg);
      }
      else if( i > 0 || j < HaArrayCount(d->worker_penalties) - 1 )
      {
	/* penalize unpreferred using an XESTT prefer resources constraint */
	r = NrcWorkerResource(wp->worker);
	id = HnStringMake(a, "PRC:%s:Not%s%s", role, KheResourceId(r),
	  i+1 == j ? "" : "Etc");
	name = HnStringMake(a, "Prefer not %s%s", KheResourceId(r),
	  i+1 == j ? "" : " (etc.)");
	if( !KhePreferResourcesConstraintMake(ins, id, name,
	    NrcPenaltyConvert(wp->penalty), role, &kprc) )
	  HnAbort("cannot add %s constraint", id);
	for( k = 0;  k < HaArrayCount(d->worker_penalties) - 1;  k++ )
	  if( k < i || k >= j )
	  {
	    wp2 = HaArray(d->worker_penalties, k);
	    r2 = NrcWorkerResource(wp2->worker);
	    if( !KhePreferResourcesConstraintAddResource(kprc, r2) )
	      HnAbort("cannot add resource %s to %s constraint", id,
		KheResourceId(r2));
	  }
	if( !KhePreferResourcesConstraintAddEventGroup(kprc, eg, &e) )
	  HnAbort("cannot add eg to %s constraint (problem is %s)",
	    id, KheEventId(e));
      }
    }
  }

  /* ***
  KHE_EVENT e;  KHE_ASSIGN_RESOURCE_CONSTRAINT karc;  HA_ARENA a;
  int index, i, staffing_count;  KHE_EVENT_GROUP eg;  NRC_PENALTY_GROUP pg;
  KHE_PREFER_RESOURCES_CONSTRAINT kprc;  char *role, *id, *name;
  a = NrcInstanceArena(d->instance);
  id = HnStringMake(a, "EG:%s", d->name);
  eg = NrcShiftSetEventGroup(d->shift_set, id, ins);
  for( index = 1;  index <= d->max_shift_index;  index++ )
  {
    ** not assigned **
    role = NrcDemandRoleMake(d, index);
    if( NrcPenaltyWeight(d->not_assigned_penalty) > 0 )
    {
      id = HnStringMake(a, "ARC:%s", role);
      name = "Assign resource";
      if( !KheAssignResourceConstraintMake(ins, id, name,
	  NrcPenaltyConvert(d->not_assigned_penalty), role, &karc) )
	HnAbort("cannot add %s constraint", id);
      KheAssignResourceConstraintAddEventGroup(karc, eg);
    }

    ** assigned **
    if( NrcPenaltyWeight(d->assigned_penalty) > 0 )
    {
      id = HnStringMake(a, "PRC:%s", role);
      name = "Do not assign resource";
      if( !KhePreferResourcesConstraintMake(ins, id, name,
          NrcPenaltyConvert(d->assigned_penalty), role, &kprc) )
	HnAbort("cannot add %s constraint", id);
      if( !KhePreferResourcesConstraintAddResourceGroup(kprc,
	  NrcInstanceEmptyResourceGroup(d->instance)) )
	HnAbort("cannot add empty rg to %s constraint", id);
      if( !KhePreferResourcesConstraintAddEventGroup(kprc, eg, &e) )
	HnAbort("cannot add all_eg to %s constraint (problem is %s)",
	  id, KheEventId(e));
    }

    ** not preferred **
    if( d->preferred_ws != NULL )
    {
      if( index == 1 )
	NrcDemandBuildPenaltyGroups(d);
      staffing_count = NrcInstanceStaffingWorkerCount(d->instance);
      HaArrayForEach(d->penalty_groups, pg, i)
	if( NrcWorkerSetWorkerCount(pg->worker_set) < staffing_count )
	{
	  id = HnStringMake(a, "PRC:%s:Not=%s", role,NrcPenaltyId(pg->penalty));
	  name = HnStringMake(a, "Prefer %s resource",
	    NrcWorkerSetName(pg->worker_set));
	  if( !KhePreferResourcesConstraintMake(ins, id, name,
	      NrcPenaltyConvert(pg->penalty), role, &kprc) )
	    HnAbort("cannot add %s constraint", id);
	  if( !KhePreferResourcesConstraintAddResourceGroup(kprc,
		NrcWorkerSetResourceGroup(pg->worker_set, ins)) )
	    HnAbort("cannot add rg to %s constraint", id);
	  if( !KhePreferResourcesConstraintAddEventGroup(kprc, eg, &e) )
	    HnAbort("cannot add all_eg to %s constraint (problem is %s)",
	      id, KheEventId(e));
	}
    }
  }
  *** */
  if( DEBUG4 )
    fprintf(stderr, "] NrcDemandConvertTo... returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandTypedCmp(NRC_DEMAND d1, NRC_DEMAND d2)                      */
/*                                                                           */
/*  Comparison function for sorting an array of demands by name.             */
/*                                                                           */
/*****************************************************************************/

int NrcDemandTypedCmp(NRC_DEMAND d1, NRC_DEMAND d2)
{
  return strcmp(d1->name, d2->name);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandCmp(const void *t1, const void *t2)                         */
/*                                                                           */
/*  Untyped demand comparison function.                                      */
/*                                                                           */
/*****************************************************************************/


int NrcDemandCmp(const void *t1, const void *t2)
{
  NRC_DEMAND d1 = * (NRC_DEMAND *) t1;
  NRC_DEMAND d2 = * (NRC_DEMAND *) t2;
  return NrcDemandTypedCmp(d1, d2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandDebug(NRC_DEMAND d, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of d onto fp.                                                */
/*                                                                           */
/*****************************************************************************/

void NrcDemandDebug(NRC_DEMAND d, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ Demand %s ]", NrcDemandName(d));
  if( indent >= 0 )
    fprintf(fp, "\n");
}

/* ***
void NrcDemandDebug(NRC_DEMAND d, int indent, FILE *fp)
{
  HA_ARENA a;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  a = NrcInstanceArena(d->instance);
  fprintf(fp, "%s(nap %s, ap %s, ws, npp %s)", d->name,
    NrcPenaltyShow(d->not_assigned_penalty, a),
    NrcPenaltyShow(d->assigned_penalty, a),
    NrcPenaltyShow(d->not_preferred_penalty, a));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */

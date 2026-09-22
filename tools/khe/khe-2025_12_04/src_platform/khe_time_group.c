
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
/*  FILE:         khe_time_group.c                                           */
/*  DESCRIPTION:  A set of times                                             */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#include "sset.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_NHOOD - a time group neighbourhood                        */
/*                                                                           */
/*****************************************************************************/

struct khe_time_group_nhood_rec {
  KHE_INSTANCE			instance;
  SSET				base_set;		/* the base set      */
  ARRAY_KHE_TIME_GROUP		time_groups;		/* the neighbours    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP - a set of times                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_time_group_rec {
  void			*back;			/* back pointer              */
  KHE_INSTANCE		instance;		/* enclosing instance        */
  KHE_TIME_GROUP_KIND	kind;			/* Weeks, Days, etc          */
  char			*id;			/* Id                        */
  char			*name;			/* Name                      */
  SSET			times;			/* times as sset of indexes  */
  KHE_TIME_GROUP_NHOOD	neighbourhood;		/* time group neighbourhood  */
  int			pos_in_nhood;		/* pos of this tg in nhood   */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time group neighbourhoods"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_NHOOD KheTimeGroupNHoodMakeEmpty(SSET *base_set,          */
/*    KHE_INSTANCE ins, KHE_SOLN soln)                                       */
/*                                                                           */
/*  Make a new, empty time group neighbourhood with the given base_set, and  */
/*  add it to ins, or to soln if soln is non-NULL.                           */
/*                                                                           */
/*  Implementation note.  It is the address of the copy of base_set stored   */
/*  in the new neighbourhood that is passed to the instance.  This address   */
/*  has a lifetime as long as the instance's.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_NHOOD KheTimeGroupNHoodMakeEmpty(SSET *base_set,
  KHE_INSTANCE ins, KHE_TIME_GROUP_KIND kind, /* KHE_SOLN soln */ HA_ARENA a)
{
  KHE_TIME_GROUP_NHOOD res;
  /* a = (soln != NULL ? KheSolnArena(soln) : KheInstanceArena(ins)); */
  HaMake(res, a);
  res->base_set = *base_set;
  res->instance = ins;
  HaArrayInit(res->time_groups, a);
  if( kind != KHE_TIME_GROUP_KIND_SOLN )
    KheInstanceTimeGroupNeighbourhoodTableInsert(ins, &res->base_set, res);
  /* *** arenas take care of this now
  else
    KheSolnAddTimeNHood(soln, res);
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_NHOOD KheTimeGroupNhoodMake(KHE_TIME_GROUP tg,            */
/*    KHE_SOLN soln, int *index_in_nhood)                                    */
/*                                                                           */
/*  Return the time group neighbourhood for tg, either freshly made or else  */
/*  retrieved from tg's instance.  Also set *index_in_nhood to the index in  */
/*  the neighbourhood of the time group with the same times as tg.           */
/*                                                                           */
/*  If soln != NULL, the time nhood is associated with soln, not with        */
/*  an instance, and needs to be enrolled in soln so that it can be          */
/*  deleted later (that is, if it is created fresh).                         */

/*****************************************************************************/

static KHE_TIME_GROUP_NHOOD KheTimeGroupNhoodMake(KHE_TIME_GROUP tg,
  /* KHE_TIME_GROUP_KIND kind, KHE_SOLN soln, */ HA_ARENA arena,
  int *index_in_nhood)
{
  KHE_TIME_GROUP_NHOOD res;  int a, b, count, max_index, delta;
  SSET *tg_set, shifted_set, base_set;  KHE_TIME ta, tb;
  KHE_TIME_GROUP shifted_tg;  KHE_INSTANCE ins;

  /* boilerplate */
  if( DEBUG1 || (DEBUG3 && KheTimeGroupTimeCount(tg) <= 1) )
  {
    fprintf(stderr, "[ KheTimeGroupNhoodMake(");
    KheTimeGroupDebug(tg, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  ins = KheTimeGroupInstance(tg);
  tg_set = KheTimeGroupTimeSet(tg);
  count = KheTimeGroupTimeCount(tg);

  if( count == 0 )
  {
    /* tg is empty; retrieve or make empty neighbourhood (special case) */
    *index_in_nhood = 0;
    if( !KheInstanceTimeGroupNeighbourhoodTableRetrieve(ins, tg_set, &res) )
      res = KheTimeGroupNHoodMakeEmpty(tg_set, ins, tg->kind, /* soln */ arena);
  }
  else
  {
    /* tg is non-empty; *index_in_nhood is the index of the last time of tg */
    ta = KheTimeGroupTime(tg, count - 1);
    *index_in_nhood = a = KheTimeIndex(ta);

    /* the "base set" is tg's set shifted so the first time has index 0 */
    tb = KheTimeGroupTime(tg, 0);
    SSetInitShifted(base_set, *tg_set, -KheTimeIndex(tb));

    if( !KheInstanceTimeGroupNeighbourhoodTableRetrieve(ins, &base_set, &res) )
    {
      /* no existing neighbourhood; make one, and add non-empty time groups */
      res = KheTimeGroupNHoodMakeEmpty(&base_set, ins, tg->kind, /*soln*/arena);
      max_index = KheInstanceTimeCount(ins) - 1;
      b = max_index - KheTimeIndex(tb);
      if( DEBUG1 )
	fprintf(stderr, "  %s: a = %d;  %s: b = %d\n",
	  KheTimeId(ta) == NULL ? "-" : KheTimeId(ta), a,
	  KheTimeId(tb) == NULL ? "-" : KheTimeId(tb), b);
      for( delta = -a;  delta <= b;  delta++ )
      {
	/* find the shifted time group and add it to res->time_groups */
	if( DEBUG1 )
	  fprintf(stderr, "  delta = %d:\n", delta);
	SSetInitShiftedAndSliced(shifted_set, *tg_set, delta, 0, max_index);
	shifted_tg = KheTimeGroupMakeAndFinalize(ins, KHE_TIME_GROUP_KIND_AUTO,
	  KheTimeGroupId(tg), KheTimeGroupName(tg), &shifted_set, false,
	  /* soln */ arena);
	if( shifted_tg->neighbourhood == NULL )
	{
	  shifted_tg->neighbourhood = res;
	  shifted_tg->pos_in_nhood = HaArrayCount(res->time_groups);
	}
	HaArrayAddLast(res->time_groups, shifted_tg);
      }
    }
  }
  if( DEBUG1 || (DEBUG3 && KheTimeGroupTimeCount(tg) <= 1) )
    fprintf(stderr, "] KheTimeGroupNhoodMake returning.\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupNHoodDelete(KHE_TIME_GROUP_NHOOD tgn, int pos)          */
/*                                                                           */
/*  Delete tgn, but not any of the time groups in it.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTimeGroupNHoodDelete(KHE_TIME_GROUP_NHOOD tgn)
{
  SSetFree(tgn->base_set);
  MArrayFree(tgn->time_groups);
  MFree(tgn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupNHoodNeighbour(KHE_TIME_GROUP_NHOOD tgn,      */
/*    int pos)                                                               */
/*                                                                           */
/*  Return the time group at this position in pos if there is one, or        */
/*  else the empty time group.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheTimeGroupNHoodNeighbour(KHE_TIME_GROUP_NHOOD tgn,
  int pos)
{
  if( pos >= 0 && pos < HaArrayCount(tgn->time_groups) )
    return HaArray(tgn->time_groups, pos);
  else
    return KheInstanceEmptyTimeGroup(tgn->instance);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "internal operations"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupMakeInternal(KHE_INSTANCE ins,                */
/*    KHE_SOLN soln, KHE_TIME_GROUP_KIND kind, char *id, char *name)         */
/*                                                                           */
/*  Make a time group of the given kind, but do not add it to ins.           */
/*                                                                           */
/*  If soln != NULL, the time group is associated with soln, not with        */
/*  an instance, and needs to be enrolled in soln so that it can be          */
/*  deleted later.                                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheTimeGroupMakeInternal(KHE_INSTANCE ins,
  /* KHE_SOLN soln, */ KHE_TIME_GROUP_KIND kind, char *id, char *name,
  HA_ARENA a)
{
  KHE_TIME_GROUP res;  static int count = 0;  /* HA_ARENA a; */
  /* a = (soln != NULL ? KheSolnArena(soln) : KheInstanceArena(ins)); */
  HaMake(res, a);
  res->back = NULL;
  res->instance = ins;
  res->kind = kind;
  res->id = HnStringCopy(id, a);
  res->name = HnStringCopy(name, a);
  SSetInit(res->times, a);
  res->neighbourhood = NULL;  /* done when finalizing */
  res->pos_in_nhood = -1;     /* done when finalizing */
  /* *** arenas take care of this now
  if( soln != NULL )
    KheSolnAddTimeGroup(soln, res);
  *** */
  if( DEBUG5 && ++count % 1000 == 0 )
    fprintf(stderr, "%d'th KheTimeGroupMakeInternal\n", count);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupCopy(KHE_TIME_GROUP tg, HA_ARENA a)           */
/*                                                                           */
/*  Make a copy of tg, or share it if it is safe to do so.                   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheTimeGroupCopy(KHE_TIME_GROUP tg, HA_ARENA a)
{
  KHE_TIME_GROUP res;
  if( tg->kind == KHE_TIME_GROUP_KIND_SOLN )
  {
    /* need a true copy */
    HaMake(res, a);
    res->back = tg->back;
    res->instance = tg->instance;
    res->kind = tg->kind;
    res->id = HnStringCopy(tg->id, a);
    res->name = HnStringCopy(tg->name, a);
    SSetCopy(res->times, tg->times, a);
    if( tg->neighbourhood != NULL )
      res->neighbourhood = KheTimeGroupNhoodMake(res, a, /*soln,*/
	&res->pos_in_nhood);
    res->neighbourhood = NULL;  /* still to do */
    res->pos_in_nhood = -1;	/* still to do */
    return res;
  }
  else
  {
    /* sharing is safe */
    return tg;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeGroupFinalName(KHE_TIME_GROUP tg, HA_ARENA a)               */
/*                                                                           */
/*  Return the final name of tg.                                             */
/*                                                                           */
/*****************************************************************************/

char *KheTimeGroupFinalName(KHE_TIME_GROUP tg, HA_ARENA a)
{
  if( tg->name == NULL || SSetShift(tg->times) == 0 )
    return tg->name;
  else
    return HnStringMake(a, "%s%+d", tg->name, SSetShift(tg->times));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeGroupId(KHE_TIME_GROUP tg)                                  */
/*                                                                           */
/*  Return the Id of tg.                                                     */
/*                                                                           */
/*****************************************************************************/

static char *KheTimeGroupFinalId(KHE_TIME_GROUP tg, HA_ARENA a)
{
  if( tg->id == NULL )
  {
    if( KheTimeGroupTimeCount(tg) == 0 )
      return "TG:Empty";
    else if( KheTimeGroupTimeCount(tg) == KheInstanceTimeCount(tg->instance) )
      return "TG:All";
    else if( KheTimeGroupTimeCount(tg) == 1 )
      return HnStringMake(a, "TG:%s", KheTimeId(KheTimeGroupTime(tg, 0)));
    else
      return "TG:NoId";
  }
  else if( SSetShift(tg->times) == 0 )
    return tg->id;
  else
    return HnStringMake(a, "%s%+d", tg->id, SSetShift(tg->times));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupFinalize(KHE_TIME_GROUP tg, KHE_SOLN soln,              */
/*    bool with_nhood)                                                       */
/*                                                                           */
/*  Finalize tg.  If with_neighbourhood is true, a neighbourhood is added.   */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupFinalize(KHE_TIME_GROUP tg, /* KHE_SOLN soln, */ HA_ARENA a,
  bool with_nhood)
{
  /* HA_ARENA a; */
  if( DEBUG4 )
    fprintf(stderr, "[ KheTimeGroupFinalize(%p %s)\n", (void *) tg,
      tg->id != NULL ? tg->id : "-");
  /* a = (soln!=NULL ? KheSolnArena(soln) : KheInstanceArena(tg->instance)); */
  tg->id = KheTimeGroupFinalId(tg, a);
  tg->name = KheTimeGroupFinalName(tg, a);
  if( !SSetIsFinalized(tg->times) )
    SSetFinalize(tg->times);
  if( with_nhood )
    tg->neighbourhood = KheTimeGroupNhoodMake(tg, a, /*soln,*/
      &tg->pos_in_nhood);
  if( DEBUG4 )
  {
    KheTimeGroupDebug(tg, 3, 0, stderr);
    fprintf(stderr, "] KheTimeGroupFinalize\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupMakeAndFinalize(KHE_INSTANCE ins,             */
/*    KHE_TIME_GROUP_KIND kind, char *id, char *name, SSET *times,           */
/*    KHE_SOLN soln, bool with_neighbourhood)                                */
/*                                                                           */
/*  Make and finalize a new time group with these attributes.                */
/*                                                                           */
/*  If there is a user-defined time group with these times, return that      */
/*  time group and free *times.  If not, use *times in the new time group.   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheTimeGroupMakeAndFinalize(KHE_INSTANCE ins,
  KHE_TIME_GROUP_KIND kind, char *id, char *name, SSET *times,
  /* KHE_SOLN soln, */ bool with_neighbourhood, HA_ARENA a)
{
  KHE_TIME_GROUP res;
  if( KheInstanceUserTimeGroupTableRetrieve(ins, times, &res) )
  {
    HnAssert(res != NULL, "KheTimeGroupMakeAndFinalize internal error 1");
    /* SSetFree(*times); */
  }
  else
  {
    res = KheTimeGroupMakeInternal(ins, /* soln, */ kind, id, name, a);
    res->times = *times;
    KheTimeGroupFinalize(res, a, /* soln, */ with_neighbourhood);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupDelete(KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Delete tg.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTimeGroupDelete(KHE_TIME_GROUP tg)
{
  SSetFree(tg->times);
  MFree(tg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupKindIsUser(KHE_TIME_GROUP_KIND kind)                    */
/*                                                                           */
/*  Return true if kind is a user-defined kind.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupKindIsUser(KHE_TIME_GROUP_KIND kind)
{
  return kind >= KHE_TIME_GROUP_KIND_ORDINARY &&
    kind <= KHE_TIME_GROUP_KIND_DAY;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupMake(KHE_INSTANCE ins, KHE_TIME_GROUP_KIND kind,        */
/*    char *id, char *name, KHE_TIME_GROUP *tg)                              */
/*                                                                           */
/*  Make a user-defined time group, initially with no times; add it to ins.  */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupMake(KHE_INSTANCE ins, KHE_TIME_GROUP_KIND kind,
  char *id, char *name, KHE_TIME_GROUP *tg)
{
  KHE_TIME_GROUP other;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheTimeGroupMake called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(kind), "KheTimeGroupMake given invalid kind");
  if( id != NULL && KheInstanceRetrieveTimeGroup(ins, id, &other) )
  {
    *tg = NULL;
    return false;
  }
  *tg = KheTimeGroupMakeInternal(ins, /* NULL, */ kind, id, name,
    KheInstanceArena(ins));
  KheInstanceAddTimeGroup(ins, *tg);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupSetBack(KHE_TIME_GROUP tg, void *back)                  */
/*                                                                           */
/*  Set the back pointer of tg.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupSetBack(KHE_TIME_GROUP tg, void *back)
{
  HnAssert(tg->kind == KHE_TIME_GROUP_KIND_SOLN ||
    KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupSetBack called after KheInstanceMakeEnd");
  tg->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheTimeGroupBack(KHE_TIME_GROUP tg)                                */
/*                                                                           */
/*  Return the back pointer of tg.                                           */
/*                                                                           */
/*****************************************************************************/

void *KheTimeGroupBack(KHE_TIME_GROUP tg)
{
  return tg->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheTimeGroupInstance(KHE_TIME_GROUP tg)                     */
/*                                                                           */
/*  Return the instance containing tg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheTimeGroupInstance(KHE_TIME_GROUP tg)
{
  return tg->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_KIND KheTimeGroupKind(KHE_TIME_GROUP tg)                  */
/*                                                                           */
/*  Return the kind of tg.                                                   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP_KIND KheTimeGroupKind(KHE_TIME_GROUP tg)
{
  return tg->kind;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeGroupId(KHE_TIME_GROUP tg)                                  */
/*                                                                           */
/*  Return the Id of tg.                                                     */
/*                                                                           */
/*****************************************************************************/

char *KheTimeGroupId(KHE_TIME_GROUP tg)
{
  return tg->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeGroupName(KHE_TIME_GROUP tg)                                */
/*                                                                           */
/*  Return the name of tg.                                                   */
/*                                                                           */
/*****************************************************************************/

char *KheTimeGroupName(KHE_TIME_GROUP tg)
{
  return tg->name;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times construction"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupAddTime(KHE_TIME_GROUP tg, KHE_TIME t)                  */
/*                                                                           */
/*  Add t to tg, first checking that tg is a user-defined time group.        */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupAddTime(KHE_TIME_GROUP tg, KHE_TIME t)
{
  HnAssert(KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupAddTime called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(tg->kind),
    "KheTimeGroupAddTime: given unchangeable time group");
  SSetInsert(tg->times, KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupSubTime(KHE_TIME_GROUP tg, KHE_TIME t)                  */
/*                                                                           */
/*  Remove t from tg, first checking that tg is user-defined.                */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupSubTime(KHE_TIME_GROUP tg, KHE_TIME t)
{
  HnAssert(KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupSubTime called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(tg->kind),
    "KheTimeGroupSubTime: given unchangeable time group");
  SSetDelete(tg->times, KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupUnion(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)            */
/*                                                                           */
/*  Set tg's set of times to its union with tg2's.                           */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupUnion(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)
{
  HnAssert(KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupUnion called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(tg->kind),
    "KheTimeGroupUnion: given unchangeable time group");
  SSetUnion(tg->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupIntersect(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)        */
/*                                                                           */
/*  Set tg's set of times to its intersection with tg2's.                    */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupIntersect(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)
{
  HnAssert(KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupIntersect called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(tg->kind),
    "KheTimeGroupIntersect: given unchangeable time group");
  SSetIntersect(tg->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupDifference(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)       */
/*                                                                           */
/*  Set tg's set of times to its difference with tg2's.                      */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupDifference(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2)
{
  HnAssert(KheInstanceFinalized(tg->instance) == KHE_FINALIZED_NONE,
    "KheTimeGroupDifference called after KheInstanceMakeEnd");
  HnAssert(KheTimeGroupKindIsUser(tg->kind),
    "KheTimeGroupDifference: given unchangeable time group");
  SSetDifference(tg->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times queries"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupTimeCount(KHE_TIME_GROUP tg)                             */
/*                                                                           */
/*  Return the number of times in tg.                                        */
/*                                                                           */
/*****************************************************************************/

int KheTimeGroupTimeCount(KHE_TIME_GROUP tg)
{
  return SSetCount(tg->times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTimeGroupTime(KHE_TIME_GROUP tg, int i)                      */
/*                                                                           */
/*  Return the i'th time of tg.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheTimeGroupTime(KHE_TIME_GROUP tg, int i)
{
  return KheInstanceTime(tg->instance, SSetGet(tg->times, i));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupContainsIndex(KHE_TIME_GROUP tg, int time_index,        */
/*    int *pos)                                                              */
/*                                                                           */
/*  Return true if tg contains the time with this index.                     */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupContainsIndex(KHE_TIME_GROUP tg, int time_index, int *pos)
{
  return SSetContains(tg->times, time_index, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupContains(KHE_TIME_GROUP tg, KHE_TIME t, int *pos)       */
/*                                                                           */
/*  Return true if tg contains t.  May be called at any time.                */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupContains(KHE_TIME_GROUP tg, KHE_TIME t, int *pos)
{
  return KheTimeGroupContainsIndex(tg, KheTimeIndex(t), pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupEqual(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)           */
/*                                                                           */
/*  Return true if these two time groups are equal.                          */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupEqual(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)
{
  return tg1 == tg2 || SSetEqual(tg1->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupSubset(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)          */
/*                                                                           */
/*  Return true if tg1 is a subset of tg2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupSubset(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)
{
  return SSetSubset(tg1->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupDisjoint(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)        */
/*                                                                           */
/*  Return true if tg1 and tg2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupDisjoint(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)
{
  return SSetDisjoint(tg1->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupTypedCmp(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)         */
/*                                                                           */
/*  Typed comparison function for sorting an array of time groups.           */
/*                                                                           */
/*****************************************************************************/

int KheTimeGroupTypedCmp(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2)
{
  return SSetTypedCmp(tg1->times, tg2->times);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Untyped comparison function for sorting an array of time groups.         */
/*                                                                           */
/*****************************************************************************/

int KheTimeGroupCmp(const void *t1, const void *t2)
{
  KHE_TIME_GROUP tg1 = * (KHE_TIME_GROUP *) t1;
  KHE_TIME_GROUP tg2 = * (KHE_TIME_GROUP *) t2;
  return KheTimeGroupTypedCmp(tg1, tg2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupIsCompact(KHE_TIME_GROUP tg)                            */
/*                                                                           */
/*  Return true if tg is compact, i.e. there are no gaps in the              */
/*  chronological ordering of its times.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupIsCompact(KHE_TIME_GROUP tg)
{
  KHE_TIME first_time, last_time;  int count;
  if( KheTimeGroupTimeCount(tg) <= 1 )
    return true;
  else
  {
    first_time = KheTimeGroupTime(tg, 0);
    last_time = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
    count = KheTimeIndex(last_time) - KheTimeIndex(first_time) + 1;
    return count == KheTimeGroupTimeCount(tg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupOverlap(KHE_TIME_GROUP tg, KHE_TIME time, int durn)      */
/*                                                                           */
/*  Return the number of times that a meet starting at time with duration    */
/*  durn would overlap with tg.                                              */
/*                                                                           */
/*****************************************************************************/

int KheTimeGroupOverlap(KHE_TIME_GROUP tg, KHE_TIME time, int durn)
{
  int start_index, i, res, pos;
  res = 0;
  start_index = KheTimeIndex(time);
  for( i = 0;  i < durn;  i++ )
    if( SSetContains(tg->times, start_index + i, &pos) )
      res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupNeighbourInternal(KHE_TIME_GROUP tg,          */
/*    int delta)                                                             */
/*                                                                           */
/*  Unchecked version of KheTimeGroupNeighbour.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheTimeGroupNeighbourInternal(KHE_TIME_GROUP tg,
  int delta)
{
  return KheTimeGroupNHoodNeighbour(tg->neighbourhood, tg->pos_in_nhood+delta);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheTimeGroupNeighbour(KHE_TIME_GROUP tg, int delta)       */
/*                                                                           */
/*  Return the time group obtained by shifting tg by delta.                  */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheTimeGroupNeighbour(KHE_TIME_GROUP tg, int delta)
{
  HnAssert(KheInstanceFinalized(tg->instance) >= KHE_FINALIZED_TIME_GROUPS,
    "KheTimeGroupNeighbour called before KheInstanceMakeEnd");
  HnAssert(tg->neighbourhood != NULL,
    "KheTimeGroupNeighbour called on a time group which has no neighbourhood");
  return KheTimeGroupNeighbourInternal(tg, delta);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupDomainsAllowAssignment(KHE_TIME_GROUP domain,           */
/*    KHE_TIME_GROUP target_domain, int target_offset)                       */
/*                                                                           */
/*  Return true if the domains allow the assignment of a meet with domain    */
/*  domain to a meet with domain target_domain at offset target_offset.      */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupDomainsAllowAssignment(KHE_TIME_GROUP domain,
  KHE_TIME_GROUP target_domain, int target_offset)
{
  return KheTimeGroupSubset(
    KheTimeGroupNeighbourInternal(target_domain, target_offset), domain);
}


/*****************************************************************************/
/*                                                                           */
/*  unsigned int KheTimeGroupTimePos(KHE_TIME_GROUP tg, int time_index)      */
/*                                                                           */
/*  Return the position in tg of the time whose index number is time_index.  */
/*  For example, the chronologically first time of tg has position 0, the    */
/*  chronologically second time has position 1, and so on.                   */
/*                                                                           */
/*  This function may only be called after tg has been finalized, but since  */
/*  it is private to KHE and efficiency is important here, this condition    */
/*  is not checked.  It should only be called when the indicated time is     */
/*  an element of tg; when called with other times its result is undefined.  */
/*                                                                           */
/*  Crucially, the implementation assumes that tg is compact.                */
/*                                                                           */
/*****************************************************************************/

int KheTimeGroupTimePos(KHE_TIME_GROUP tg, int time_index)
{
  return time_index - SSetMin(tg->times);
}


/*****************************************************************************/
/*                                                                           */
/*  SSET *KheTimeGroupTimeSet(KHE_TIME_GROUP tg)                             */
/*                                                                           */
/*  Return the time set defining tg, not necessarily finalized yet.          */
/*                                                                           */
/*****************************************************************************/

SSET *KheTimeGroupTimeSet(KHE_TIME_GROUP tg)
{
  return &tg->times;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupMakeFromKml(KML_ELT time_group_elt, KHE_INSTANCE ins,   */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Add a time group from time_group_elt to ins.                             */
/*                                                                           */
/*****************************************************************************/

bool KheTimeGroupMakeFromKml(KML_ELT time_group_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  KHE_TIME_GROUP_KIND tg_kind;
  KHE_TIME_GROUP tg;  char *id, *name;
  if( !KmlCheck(time_group_elt, "Id : $Name", ke) )
    return false;
  id = KmlAttributeValue(time_group_elt, 0);
  name = KmlText(KmlChild(time_group_elt, 0));
  tg_kind =
    strcmp(KmlLabel(time_group_elt), "Week") == 0 ? KHE_TIME_GROUP_KIND_WEEK :
    strcmp(KmlLabel(time_group_elt), "Day")  == 0 ? KHE_TIME_GROUP_KIND_DAY :
    KHE_TIME_GROUP_KIND_ORDINARY;
  if( !KheTimeGroupMake(ins, tg_kind, id, name, &tg) )
    return KmlError(ke, KheInstanceArena(ins), KmlLineNum(time_group_elt),
      KmlColNum(time_group_elt), "<TimeGroup> Id \"%s\" used previously", id);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupWrite(KHE_TIME_GROUP tg, KML_FILE kf)                   */
/*                                                                           */
/*  Write tg (just the name and Id) onto kf.                                 */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupWrite(KHE_TIME_GROUP tg, KML_FILE kf)
{
  char *kind_str;
  kind_str = tg->kind == KHE_TIME_GROUP_KIND_WEEK ? "Week" :
    tg->kind == KHE_TIME_GROUP_KIND_DAY ? "Day" : "TimeGroup";
  HnAssert(tg->id != NULL, "KheArchiveWrite: Id missing from %s", kind_str);
  HnAssert(tg->name != NULL, "KheArchiveWrite: Name missing from %s", kind_str);
  KmlEltAttributeEltPlainText(kf, kind_str, "Id", tg->id, "Name", tg->name);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupPrintTimes(KHE_TIME_GROUP tg, FILE *fp)                 */
/*                                                                           */
/*  Print the times of tg in the form of comma-separated runs.               */
/*                                                                           */
/*****************************************************************************/

static void KheTimeGroupPrintTimes(KHE_TIME_GROUP tg, FILE *fp)
{
  int start, stop;  KHE_TIME start_t, stop_t, next_stop_t;
  fprintf(stderr, "{");
  for( start = 0;  start < KheTimeGroupTimeCount(tg);  start = stop )
  {
    /* find start_t and stop_t of current run */
    start_t = KheTimeGroupTime(tg, start);
    stop_t = start_t;
    for( stop = start + 1;  stop < KheTimeGroupTimeCount(tg);  stop++ )
    {
      next_stop_t = KheTimeGroupTime(tg, stop);
      if( KheTimeIndex(next_stop_t) != KheTimeIndex(stop_t) + 1 )
	break;
      stop_t = next_stop_t;
    }

    /* print start_t and stop_t */
    if( start != 0 )
      fprintf(fp, ", ");
    if( start_t != stop_t )
      fprintf(fp, "%s..%s",
	KheTimeId(start_t) != NULL ? KheTimeId(start_t) : "-",
	KheTimeId(stop_t) != NULL ? KheTimeId(stop_t) : "-");
    else
      fprintf(fp, "%s", KheTimeId(start_t) != NULL ? KheTimeId(start_t) : "-");
  }
  fprintf(stderr, "}");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupDebug(KHE_TIME_GROUP tg, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of tg onto fp with the given verbosity and indent.           */
/*  This function accepts a NULL value for tg.                               */
/*                                                                           */
/*****************************************************************************/

void KheTimeGroupDebug(KHE_TIME_GROUP tg, int verbosity,
  int indent, FILE *fp)
{
  KHE_TIME t1;
  if( verbosity == 1 )
  {
    /* print as briefly as possible, just a name if that's available */
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( tg == NULL )
      fprintf(fp, "NULL");
    else
    {
      if( !SSetIsFinalized(tg->times) )
	fprintf(fp, "!");
      if( KheTimeGroupTimeCount(tg) == 0 )
	fprintf(fp, "{}");
      else if( KheTimeGroupTimeCount(tg) == 1 )
      {
	t1 = KheTimeGroupTime(tg, 0);
	if( KheTimeId(t1) != NULL )
	  fprintf(fp, "{%s}", KheTimeId(t1));
	else if( KheTimeGroupId(tg) != NULL )
	  fprintf(fp, "%s", KheTimeGroupId(tg));
	else
	  fprintf(fp, "{-}");
      }
      else if( KheTimeGroupId(tg) != NULL )
	fprintf(fp, "%s", KheTimeGroupId(tg));
      else
      {
	KheTimeGroupPrintTimes(tg, fp);
      }
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
  else if( verbosity >= 2 )
  {
    /* print the name (if available) and the full value */
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( tg == NULL )
      fprintf(fp, "NULL");
    else
    {
      if( !SSetIsFinalized(tg->times) )
	fprintf(fp, "!");
      if( KheTimeGroupId(tg) != NULL )
	fprintf(fp, "%s", KheTimeGroupId(tg));
      KheTimeGroupPrintTimes(tg, fp);
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

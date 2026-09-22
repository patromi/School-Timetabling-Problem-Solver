
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
/*  FILE:         nrc_demand_constraint.c                                    */
/*  MODULE:       A demand constraint                                        */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_CONSTRAINT - a demand constraint                              */
/*                                                                           */
/*****************************************************************************/

struct nrc_demand_constraint_rec {
  NRC_INSTANCE		instance;
  NRC_BOUND		bound;
  NRC_SHIFT_SET		shift_set;
  NRC_WORKER_SET	worker_set;
  char			*name;
  /* bool		defunct; */		/* ignore self when true     */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and query"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_CONSTRAINT NrcDemandConstraintMake(NRC_BOUND b,               */
/*    NRC_SHIFT_SET ss, NRC_WORKER_SET ws, char *name)                       */
/*                                                                           */
/*  Make and return a new demand constraint object with these attributes.    */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND_CONSTRAINT NrcDemandConstraintMake(NRC_BOUND b,
  NRC_SHIFT_SET ss, NRC_WORKER_SET ws, char *name)
{
  NRC_DEMAND_CONSTRAINT res;  int i;  HA_ARENA a;
  a = NrcInstanceArena(NrcShiftSetInstance(ss));
  HaMake(res, a);
  res->instance = NrcShiftSetInstance(ss);
  res->bound = b;
  res->shift_set = ss;
  res->worker_set = ws;
  res->name = HnStringCopy(name, a);
  /* res->defunct = false; */
  NrcInstanceAddDemandConstraint(res->instance, res);
  for( i = 0;  i < NrcShiftSetShiftCount(ss);  i++ )
    NrcShiftAddDemandConstraint(NrcShiftSetShift(ss, i), res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcDemandConstraintBound(NRC_DEMAND_CONSTRAINT dc)             */
/*                                                                           */
/*  Return the bound attribute of dc.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcDemandConstraintBound(NRC_DEMAND_CONSTRAINT dc)
{
  return dc->bound;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDemandConstraintShiftSet(NRC_DEMAND_CONSTRAINT dc)      */
/*                                                                           */
/*  Return the shift-set attribute of dc.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDemandConstraintShiftSet(NRC_DEMAND_CONSTRAINT dc)
{
  return dc->shift_set;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcDemandConstraintWorkerSet(NRC_DEMAND_CONSTRAINT dc)    */
/*                                                                           */
/*  Return the worker-set attribute of dc.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcDemandConstraintWorkerSet(NRC_DEMAND_CONSTRAINT dc)
{
  return dc->worker_set;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandConstraintName(NRC_DEMAND_CONSTRAINT dc)                  */
/*                                                                           */
/*  Return the name attribute of dc.                                         */
/*                                                                           */
/*****************************************************************************/

char *NrcDemandConstraintName(NRC_DEMAND_CONSTRAINT dc)
{
  return dc->name;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandConstraintTypedCmpBoundAndWorkerSets(                       */
/*    NRC_DEMAND_CONSTRAINT dc1, NRC_DEMAND_CONSTRAINT dc2)                  */
/*                                                                           */
/*  Typed comparison function for bringing similar demand constraints        */
/*  together.  The shift sets and name may differ.                           */
/*                                                                           */
/*****************************************************************************/

int NrcDemandConstraintTypedCmpBoundAndWorkerSets(NRC_DEMAND_CONSTRAINT dc1,
  NRC_DEMAND_CONSTRAINT dc2)
{
  int cmp;

  /* bound */
  cmp = NrcBoundTypedCmp(dc1->bound, dc2->bound);
  if( cmp != 0 )  return cmp;

  /* worker sets */
  cmp = NrcWorkerSetTypedCmp(dc1->worker_set, dc2->worker_set);
  if( cmp != 0 )  return cmp;

  /* all equal if we get this far */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandConstraintCmpBoundAndWorkerSets(const void *p1,             */
/*    const void *p2)                                                        */
/*                                                                           */
/*  Untyped comparison function for bringing similar demand constraints      */
/*  together.                                                                */
/*                                                                           */
/*****************************************************************************/

int NrcDemandConstraintCmpBoundAndWorkerSets(const void *p1, const void *p2)
{
  NRC_DEMAND_CONSTRAINT dc1 = * (NRC_DEMAND_CONSTRAINT *) p1;
  NRC_DEMAND_CONSTRAINT dc2 = * (NRC_DEMAND_CONSTRAINT *) p2;
  return NrcDemandConstraintTypedCmpBoundAndWorkerSets(dc1, dc2);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcStrCmp(const void *p1, const void *p2)                            */
/*                                                                           */
/*  Untyped comparison function for sorting an array of strings.             */
/*                                                                           */
/*****************************************************************************/

int NrcStrCmp(const void *p1, const void *p2)
{
  char *str1 = * (char **) p1;
  char *str2 = * (char **) p2;
  char *colon1, *colon2;  int val1, val2, cmp;
  colon1 = strstr(str1, ":");
  colon2 = strstr(str2, ":");
  if( colon1 != NULL && colon2 != NULL &&
      sscanf(colon1 + 1, "%d", &val1) == 1 &&
      sscanf(colon2 + 1, "%d", &val2) == 1 )
  {
    *colon1 = *colon2 = '\0';
    cmp = strcmp(str1, str2);
    if( cmp == 0 ) cmp = val1 - val2;
    *colon1 = *colon2 = ':';
    return cmp;
  }
  else
    return strcmp(str1, str2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintAddRoles(NRC_INSTANCE ins,                       */
/*    int first_index, int last_index, KHE_LIMIT_RESOURCES_CONSTRAINT c)     */
/*                                                                           */
/*  Add roles covering all of the slots of the non-defunct demand            */
/*  constraints between first_index and last_index to c.                     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DEMAND) ARRAY_NRC_DEMAND;

static void NrcDemandConstraintAddRoles(NRC_INSTANCE ins,
  int first_index, int last_index, KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  int i, j, k;  HA_ARRAY_NSTRING role_array;  char *role;  HA_ARENA a;
  NRC_DEMAND_CONSTRAINT dc;  KHE_EVENT e;  KHE_EVENT_RESOURCE er;

  /* accumulate the roles in role_array */
  a = NrcInstanceArena(ins);
  HaArrayInit(role_array, a);
  for( i = first_index;  i <= last_index;  i++ )
  {
    dc = NrcInstanceDemandConstraint(ins, i);
    for( j = 0;  j < NrcShiftSetShiftCount(dc->shift_set);  j++ )
    {
      e = NrcShiftEvent(NrcShiftSetShift(dc->shift_set, j));
      for( k = 0;  k < KheEventResourceCount(e);  k++ )
      {
	er = KheEventResource(e, k);
	HaArrayAddLast(role_array, KheEventResourceRole(er));
      }
    }
  }

  /* sort and uniqueify role_array */
  HaArraySortUnique(role_array, &NrcStrCmp);

  /* add the roles to c */
  HaArrayForEach(role_array, role, i)
    KheLimitResourcesConstraintAddRole(c, role);
  HaArrayFree(role_array);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintAddEventGroups(NRC_INSTANCE ins,                 */
/*    int first_index, int last_index, KHE_LIMIT_RESOURCES_CONSTRAINT c)     */
/*                                                                           */
/*  Add to c the resource groups corresponding to the worker-sets of the     */
/*  demand constraints from first_index to last_index in ins.                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT_SET) ARRAY_NRC_SHIFT_SET;

static void NrcDemandConstraintAddEventGroups(NRC_INSTANCE ins,
  int first_index, int last_index, KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  NRC_DEMAND_CONSTRAINT dc;  int i;  KHE_EVENT_GROUP eg;
  KHE_INSTANCE kins;  char name[20];  NRC_SHIFT s;  HA_ARENA a;
  ARRAY_NRC_SHIFT_SET shift_sets;  NRC_SHIFT_SET ss;
  kins = KheConstraintInstance((KHE_CONSTRAINT) c);

  /* sort the shift-sets from first_index to last_index */
  a = NrcInstanceArena(ins);
  HaArrayInit(shift_sets, a);
  for( i = first_index;  i <= last_index;  i++ )
  {
    dc = NrcInstanceDemandConstraint(ins, i);
    HaArrayAddLast(shift_sets, dc->shift_set);
  }
  HaArraySort(shift_sets, &NrcShiftSetCmp);

  /* add the shift-sets to c */
  HaArrayForEach(shift_sets, ss, i)
  {
    if( NrcShiftSetShiftCount(ss) == 1 )
    {
      /* use the sole shift's event's singleton event group */
      /* this will be converted back to an Event on writing */
      s = NrcShiftSetShift(ss, 0);
      eg = KheEventSingletonEventGroup(NrcShiftEvent(s));
    }
    else
    {
      /* have to manufacture an event group */
      sprintf(name, "EGDC:%d", first_index + i);
      eg = NrcShiftSetEventGroup(ss, name, kins);
    }
    KheLimitResourcesConstraintAddEventGroup(c, eg);
  }
  HaArrayFree(shift_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandConstraintGetId(NRC_DEMAND_CONSTRAINT dc, int name_index, */
/*    char *suffix)                                                          */
/*                                                                           */
/*  Generate a suitable name for dc, containing name_index and suffix.       */
/*                                                                           */
/*****************************************************************************/

char *NrcDemandConstraintGetId(NRC_DEMAND_CONSTRAINT dc, int name_index,
  char *suffix)
{
  HA_ARENA a = NrcInstanceArena(dc->instance);
  return HnStringMake(a, "DemandConstraint:%d%s", name_index, suffix);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintsConvertToLimitResources(                        */
/*    NRC_DEMAND_CONSTRAINT dc, int name_index, int first_index,             */
/*    int last_index, KHE_INSTANCE ins)                                      */
/*                                                                           */
/*  The demand constraints of the instance from index first_index to index   */
/*  last_index inclusive compare equal using NrcDemandConstraintTypedCmp so  */
/*  they can be generated together.  Do this generation, to limit resources  */
/*  constraints.  Constraint dc is the first of these constraints, the one   */
/*  at index first_index.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** moved back to NrcDemandConstraintsConvert
static void NrcDemandConstraintsConvertToLimitResources(
  NRC_DEMAND_CONSTRAINT dc, int name_index, int first_index,
  int last_index, KHE_INSTANCE ins)
{
  static char *suffixes[] = {"A", "B", "C", "D", "E", "F"};
  NRC_BOUND b;  NRC_PENALTY penalty;
  int i, min_value, max_value;  bool allow_zero;
  char *suffix, *id, *name;  KHE_LIMIT_RESOURCES_CONSTRAINT c;
  b = NrcDemandConstraintBound(dc);
  for( i = 0;  i < NrcBoundIte mCount(b);  i++ )
  {
    NrcBoundItem(b, i, &min_value, &allow_zero, &max_value, &penalty);
    HnAssert(!allow_zero,
      "NrcDemandConstraintsConvertToLimitResources internal error 1");

    ** get the id **
    suffix = NrcBoundI temCount(b) == 0 ? "" : suffixes[i];
    id = NrcDemandConstraintGetId(dc, name_index, suffix);

    ** get the name **
    if( min_value != 0 && max_value != INT_MAX )
      name = MStringMake("%s, min %d, max %d", dc->name, min_value, max_value);
    else if( min_value != 0 )
      name = MStringMake("%s, min %d", dc->name, min_value);
    else if( max_value != 0 )
      name = MStringMake("%s, max %d", dc->name, max_value);
    else
    {
      MAbort("NrcDemandConstraintsConvertToLimitResources internal error 2");
      name = NULL;  ** keep compiler happy **
    }

    ** add one KHE limit resources constraint **
    if( !KheLimitRe sourcesConstraintMake(ins, id, name,
	  NrcPenaltyConvert(penalty), min_value, max_value, &c) )
      MAbort("NrcDemandConstraintsConvertToLimitResources internal error 3");
    NrcDemandConstraintAddEventGroups(dc->instance, first_index, last_index, c);
    KheLimitResourcesConstraintAddResourceGroup(c,
      NrcWorkerSetResourceGroup(dc->worker_set, ins));
    NrcDemandCon straintAddRoles(dc->instance, first_index, last_index, c);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandConstraintCanUseDemands(NRC_DEMAND_CONSTRAINT dc)          */
/*                                                                           */
/*  Return true when dc can be represented by ordinary demands, because      */
/*  it applies to a single shift and to all workers.                         */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
bool NrcDemandConstraintCanUseDemands(NRC_DEMAND_CONSTRAINT dc)
{
  if( DEBUG1 )
    fprintf(stderr, "[ NrcDemandConstraintCanUseDemands(%s)\n",
      dc->name != NULL ? dc->name : "-");
  HnAssert(!dc->def unct, "NrcDemandConstraintCanUseDemands internal error");

  ** dc's worker-set must be the whole staff **
  if( NrcWorkerSetWorkerCount(dc->worker_set) !=
      NrcInstanceStaffingWorkerCount(dc->instance) )
  {
    if( DEBUG1 )
      fprintf(stderr,
	"] NrcDemandConstraintCanUseDemands returning false (%d workers)\n",
	NrcWorkerSetWorkerCount(dc->worker_set));
    return false;
  }

  ** dc must constrain a single shift **
  if( NrcShiftSetShiftCount(dc->shift_set) != 1 )
  {
    if( DEBUG1 )
      fprintf(stderr,
	"] NrcDemandConstraintCanUseDemands returning false (%d shifts)\n",
	NrcShiftSetShiftCount(dc->shift_set));
    return false;
  }

  ** all good **
  if( DEBUG1 )
    fprintf(stderr, "] NrcDemandConstraintCanUseDemands returning true\n");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandConstraintTryDemands(NRC_DEMAND_CONSTRAINT dc)             */
/*                                                                           */
/*  Try to convert dc into demands, and return true if successful.           */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete (using NrcDemandConstraintCanUseDemands now)
bool NrcDemandConstraintTryDemands(NRC_DEMAND_CONSTRAINT dc)
{
  bool res;
  if( !dc->def unct )
  {
    if( DEBUG1 )
      fprintf(stderr, "[ NrcDemandConstraintTryDemands(%s)\n",
	dc->name != NULL ? dc->name : "-");

    ** dc's worker-set must be the whole staff **
    if( NrcWorkerSetWorkerCount(dc->worker_set) !=
	NrcInstanceStaffingWorkerCount(dc->instance) )
    {
      if( DEBUG1 )
	fprintf(stderr,
	  "] NrcDemandConstraintTryDemands returning false (%d workers)\n",
	  NrcWorkerSetWorkerCount(dc->worker_set));
      return false;
    }

    ** dc must constrain a single shift, which must itself be convertible **
    if( NrcShiftSetShiftCount(dc->shift_set) != 1 )
    {
      if( DEBUG1 )
	fprintf(stderr,
	  "] NrcDemandConstraintTryDemands returning false (%d shifts)\n",
	  NrcShiftSetShiftCount(dc->shift_set));
      return false;
    }
    res = NrcShiftTryDemands(NrcShiftSetShift(dc->shift_set, 0) **,dc->bound**);
    if( DEBUG1 )
      fprintf(stderr, "] NrcDemandConstraintTryDemands returning %s\n",
	res ? "true" : "false");
    return res;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintSetDefunct(NRC_DEMAND_CONSTRAINT dc)             */
/*                                                                           */
/*  Make dc defunct.                                                         */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConstraintSetDefunct(NRC_DEMAND_CONSTRAINT dc)
{
  dc->shift_set = NULL;
  /* dc->defunct = true; */
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandConstraintsUseAssignResources(NRC_DEMAND_CONSTRAINT dc,    */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Return true if the constraints beginning with dc can be generated        */
/*  using assign resources constraints.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcDemandConstraintsUseAssignResources(NRC_DEMAND_CONSTRAINT dc,
  int first_index, int last_index)
{
  int i;  NRC_DEMAND_CONSTRAINT dc2;  NRC_SHIFT s;

  ** dc's worker-set must be the whole staff **
  if( NrcWorkerSetWorkerCount(dc->worker_set) !=
      NrcInstanceStaffingWorkerCount(dc->instance) )
    return false;

  ** each constraint must constrain a single shift, and that shift must **
  ** be subject to a single demand constraint **
  for( i = first_index;  i <= last_index;  i++ )
  {
    dc2 = NrcInstanceDemandConstraint(dc->instance, i);
    if( NrcShiftSetShiftCount(dc2->shift_set) != 1 )
      return false;
    s = NrcShiftSetShift(dc2->shift_set, 0);
    if( NrcShiftDemandCon straintCount(s) != 1 )
      return false;
  }

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintConvertToAssignResources(                        */
/*    NRC_DEMAND_CONSTRAINT dc, int name_index, int pos, KHE_INSTANCE ins)   */
/*                                                                           */
/*  Convert dc to assign resources constraints.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void NrcDemandConstraintConvertToAssignResources(
  NRC_DEMAND_CONSTRAINT dc, int name_index, int pos, KHE_INSTANCE ins)
{
  MAbort("NrcDemandConstraintConvertToAssignResources still to do");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstrai ntsConvertToAssignResources(                      */
/*    NRC_DEMAND_CONSTRAINT dc, int name_index, int first_index,             */
/*    int last_index, KHE_INSTANCE ins)                                      */
/*                                                                           */
/*  The demand constraints of the instance from index first_index to index   */
/*  last_index inclusive compare equal using NrcDemandConstraintTypedCmp so  */
/*  they can be generated together.  Do this generation, to assign resources */
/*  constraints.  Constraint dc is the first of these constraints, the one   */
/*  at index first_index.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** done separately now
static void NrcDemandConstraint sConvertToAssignResources(
  NRC_DEMAND_CONSTRAINT dc, int name_index, int first_index,
  int last_index, KHE_INSTANCE ins)
{
  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    dc = NrcInstanceDemandConstraint(dc->instance, i);
    NrcDemandConstraintConvertToAssignResources(dc, name_index,
      i - first_index, ins);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandConstraintsAllDefunct(NRC_INSTANCE ins,                    */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Return true if all of these demand constraints are defunct.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheDemandConstraintsAllDefunct(NRC_INSTANCE ins,
  int first_index, int last_index)
{
  int i;  NRC_DEMAND_CONSTRAINT dc;
  for( i = first_index;  i <= last_index;  i++ )
  {
    dc = NrcInstanceDemandConstraint(ins, i);
    if( !dc->defunct )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintsConvert(NRC_DEMAND_CONSTRAINT dc,               */
/*    int name_index, int first_index, int last_index, KHE_INSTANCE ins)     */
/*                                                                           */
/*  The demand constraints of the instance from index first_index to index   */
/*  last_index inclusive compare equal using NrcDemandConstraintTypedCmp     */
/*  so they can be generated together.  Do this generation.  Constraint dc   */
/*  is the first of these constraints, the one at index first_index.         */
/*                                                                           */
/*  Any constraints that are convertible into demands will have already      */
/*  been converted.  So all of these constraints are converted to limit      */
/*  resources constraints.                                                   */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConstraintsConvert(NRC_DEMAND_CONSTRAINT dc,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  static char *suffixes[] = {"A", "B", "C", "D", "E", "F"};
  NRC_BOUND b;  NRC_PENALTY penalty;  HA_ARENA a;
  int i, min_value, max_value;  bool allow_zero;
  char *suffix, *id, *name;  KHE_LIMIT_RESOURCES_CONSTRAINT c;

  a = NrcInstanceArena(dc->instance);
  b = NrcDemandConstraintBound(dc);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ NrcDemandConstraintsConvert(%s, %d, %d-%d, ins)\n",
      dc->name != NULL ? dc->name : "-", name_index, first_index, last_index);
    NrcShiftSetDebug(dc->shift_set, 2, stderr);
    fprintf(stderr, "  bound = %s\n", NrcBoundShow(b));
  }

  /* if all defunct, there is nothing to do */
  /* ***
  if( KheDemandConstraintsAllDefunct(dc->instance, first_index, last_index) )
  {
    if( DEBUG2 )
      fprintf(stderr,
	"] NrcDemandConstraintsConvert returning (all defunct)\n");
    return;
  }
  *** */

  for( i = 0;  i < NrcBoundItemCount(b);  i++ )
  {
    NrcBoundItem(b, i, &min_value, &allow_zero, &max_value, &penalty);
    HnAssert(!allow_zero, "NrcDemandConstraintsConvert internal error 1");
    if( DEBUG2 )
      fprintf(stderr, "  bound item %d of %d: min %d, max %d, penalty %s\n",
	i, NrcBoundItemCount(b), min_value,
	max_value == INT_MAX ? -1 : max_value, NrcPenaltyShow(penalty));

    /* get the id */
    suffix = NrcBoundItemCount(b) == 0 ? "" : suffixes[i];
    id = NrcDemandConstraintGetId(dc, name_index, suffix);

    /* get the name */
    if( min_value != 0 )
    {
      if( max_value != INT_MAX )
      {
	/* min, max */
	name = HnStringMake(a, "%s, min %d, max %d", dc->name, min_value,
	  max_value);
      }
      else
      {
	/* min, no max */
	name = HnStringMake(a, "%s, min %d", dc->name, min_value);
      }
    }
    else
    {
      if( max_value != INT_MAX )
      {
	/* no min, max */
	name = HnStringMake(a, "%s, max %d", dc->name, max_value);
      }
      else
      {
	/* no min, no max */
	HnAbort("NrcDemandConstraintsConvert internal error 2");
	name = NULL;  /* keep compiler happy */
      }
    }

    /* add one KHE limit resources constraint */
    if( !KheLimitResourcesConstraintMake(ins, id, name,
	  NrcPenaltyConvert(penalty), min_value, max_value, &c) )
      HnAbort("NrcDemandConstraintsConvert internal error 3 (name %s)", name);
    NrcDemandConstraintAddEventGroups(dc->instance, first_index, last_index, c);
    KheLimitResourcesConstraintAddResourceGroup(c,
      NrcWorkerSetResourceGroup(dc->worker_set, ins));
    NrcDemandConstraintAddRoles(dc->instance, first_index, last_index, c);
  }
  if( DEBUG2 )
    fprintf(stderr, "] NrcDemandConstraintsConvert returning\n");
  /* ***
  if( NrcDemandConstraintsUseAssignResources(dc, first_index, last_index) )
    NrcDemandConst raintsConvertToAssignResources(dc, name_index, first_index,
      last_index, ins);
  else
    NrcDemandConstraintsConvertToLimitResources(dc, name_index, first_index,
      last_index, ins);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConstraintDebug(NRC_DEMAND_CONSTRAINT dc,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of demand constraint dc onto fp with the given indent.       */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConstraintDebug(NRC_DEMAND_CONSTRAINT dc,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[Demand Constraint %s: %s, ", dc->name, NrcBoundShow(dc->bound));
  NrcShiftSetDebug(dc->shift_set, -1, stderr);
  fprintf(fp, ", ");
  NrcWorkerSetDebug(dc->worker_set, -1, stderr);
  fprintf(fp, "]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}

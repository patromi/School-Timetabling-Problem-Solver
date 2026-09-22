
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
/*  FILE:         khe_instance.c                                             */
/*  DESCRIPTION:  One instance of the high school timetabling problem        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE - one instance of the high school timetabling problem       */
/*                                                                           */
/*****************************************************************************/

struct khe_instance_rec {
  HA_ARENA			arena;			/* arena             */
  HA_ARENA_SET			arena_set;		/* arena set         */
  void				*back;			/* back pointer      */
  ARRAY_KHE_ARCHIVE		archives;		/* archives          */
  char				*id;			/* optional id       */
  char				*meta_name;		/* metadata name     */
  char				*meta_contributor;	/* " contributor     */
  char				*meta_date;		/* " date            */
  char				*meta_country;		/* " country         */
  char				*meta_description;	/* " description     */
  char				*meta_remarks;		/* " remarks         */
  /* KHE_INSTANCE_METADATA	meta_data; */		/* instance metadata */
  KHE_MODEL			model;			/* the model         */
  ARRAY_KHE_TIME_GROUP		time_group_array;	/* user's time groups*/
  TABLE_KHE_TIME_GROUP		time_group_table;	/* time groups       */
  KHE_TIME_GROUP		empty_time_group;	/* empty time group  */
  KHE_TIME_GROUP		full_time_group;	/* full time group   */
  ARRAY_KHE_TIME		time_array;		/* times             */
  TABLE_KHE_TIME		time_table;		/* times symtab      */
  ARRAY_KHE_RESOURCE_TYPE	resource_type_array;	/* resource types    */
  TABLE_KHE_RESOURCE_TYPE	resource_type_table;	/* resource types    */
  ARRAY_KHE_RESOURCE_GROUP	partition_array;	/* all partitions    */
  ARRAY_KHE_RESOURCE		resource_array;		/* all resources     */
  ARRAY_KHE_EVENT_GROUP		event_group_array;	/* event groups      */
  TABLE_KHE_EVENT_GROUP		event_group_table;	/* event groups      */
  KHE_EVENT_GROUP		full_event_group;	/* full event group  */
  KHE_EVENT_GROUP		empty_event_group;	/* empty event group */
  ARRAY_KHE_EVENT		event_array;		/* events            */
  TABLE_KHE_EVENT		event_table;		/* events            */
  bool				all_events_preassigned;	/* all preassigned   */
  int				max_event_duration;	/* max event durn    */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* event resources   */
  ARRAY_KHE_CONSTRAINT		constraint_array;	/* constraints       */
  TABLE_KHE_CONSTRAINT		constraint_table;	/* constraints       */
  int				constraint_counts[KHE_CONSTRAINT_TAG_COUNT];
  HA_ARRAY_INT			density_counts;         /* density counts    */
  HA_ARRAY_INT			density_totals;         /* density totals    */
  SSET_TABLE			user_time_group_table;	/* indexed by SSet   */
  SSET_TABLE			time_nhood_table;	/* indexed by SSet   */
  /* KHE_TIME_GROUP_NHOOD	singleton_tgn; */	/* singleton tgn     */
  /* LSET_TRIE			time_group_lset_trie;*/	/* time groups       */
  /* LSET_TRIE			time_nhood_lset_trie;*/	/* time nhoods       */
  /* LSET_TABLE			time_group_lset_table;*//* time groups       */
  /* LSET_TABLE			time_nhood_lset_table;*//* time nhoods       */
  KHE_FINALIZED			finalized;		/* how far done      */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheInstanceArena(KHE_INSTANCE ins)                              */
/*                                                                           */
/*  Return the arena in which ins is stored.  This is used internally only.  */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheInstanceArena(KHE_INSTANCE ins)
{
  return ins->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddArchive(KHE_INSTANCE ins, KHE_ARCHIVE archive)        */
/*                                                                           */
/*  Add archive to ins, assuming it is safe to do so.                        */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddArchive(KHE_INSTANCE ins, KHE_ARCHIVE archive)
{
  HaArrayAddLast(ins->archives, archive);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceDeleteArchive(KHE_INSTANCE ins, KHE_ARCHIVE archive)     */
/*                                                                           */
/*  Delete archive from ins (internal use only).                             */
/*                                                                           */
/*****************************************************************************/

void KheInstanceDeleteArchive(KHE_INSTANCE ins, KHE_ARCHIVE archive)
{
  int pos;
  if( !HaArrayContains(ins->archives, archive, &pos) )
    HnAbort("KheInstanceDeleteArchive internal error");
  HaArrayDeleteAndShift(ins->archives, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheInstanceMakeBegin(char *id, KHE_MODEL model,             */
/*    HA_ARENA_SET as)                                                       */
/*                                                                           */
/*  Make an initially empty instance with these attributes.                  */
/*                                                                           */
/*  Also add the `all' subgroups for time groups and event groups.           */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheInstanceMakeBegin(char *id, KHE_MODEL model, HA_ARENA_SET as)
{
  KHE_INSTANCE res;  HA_ARENA a;  int i;

  if( DEBUG6 )
    fprintf(stderr, "[ KheInstanceMakeBegin(%s, ...)\n", id);
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->arena_set = as;
  res->back = NULL;
  HaArrayInit(res->archives, a);
  res->id = HnStringCopy(id, a);
  res->meta_name = NULL;
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_country = NULL;
  res->meta_description = NULL;
  res->meta_remarks = NULL;
  /* res->meta_data = md; */
  res->model = model;
  HaArrayInit(res->time_group_array, a);
  HnTableInit(res->time_group_table, a);
  res->empty_time_group = NULL;
  res->full_time_group = NULL;
  /* *** defer these until the end
  res->empty_time_group = KheTimeGroup MakeInternal(res,
    ** KHE_TIME_GROUP_TYPE_EMPTY, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
    NULL, NULL ** , LSetNew() ** );
  res->full_time_group = KheTimeGroup MakeInternal(res,
    ** KHE_TIME_GROUP_TYPE_FULL, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
    NULL, NULL ** , LSetNew() **);
  *** */
  HaArrayInit(res->time_array, a);
  HnTableInit(res->time_table, a);
  HaArrayInit(res->resource_type_array, a);
  HnTableInit(res->resource_type_table, a);
  HaArrayInit(res->partition_array, a);
  HaArrayInit(res->resource_array, a);
  HaArrayInit(res->event_group_array, a);
  HnTableInit(res->event_group_table, a);
  res->full_event_group = KheEventGroupMakeInternal(res,
    KHE_EVENT_GROUP_TYPE_FULL, NULL, KHE_EVENT_GROUP_KIND_ORDINARY, NULL, NULL);
  res->empty_event_group = KheEventGroupMakeInternal(res,
    KHE_EVENT_GROUP_TYPE_EMPTY, NULL, KHE_EVENT_GROUP_KIND_ORDINARY, NULL,NULL);
  HaArrayInit(res->event_array, a);
  HnTableInit(res->event_table, a);
  res->all_events_preassigned = true;
  res->max_event_duration = 0;
  HaArrayInit(res->event_resources, a);
  HaArrayInit(res->constraint_array, a);
  for( i = 0;  i < KHE_CONSTRAINT_TAG_COUNT;  i++ )
    res->constraint_counts[i] = 0;
  HnTableInit(res->constraint_table, a);
  HaArrayInit(res->density_counts, a);
  HaArrayInit(res->density_totals, a);
  res->user_time_group_table = SSetTableMake(a);
  res->time_nhood_table = SSetTableMake(a);
  /* res->singleton_tgn = NULL;  initialized when finalizing */
  /* res->time_group_lset_trie = LSetTrieMake(); */
  /* res->time_nhood_lset_trie = LSetTrieMake(); */
  /* ***
  res->time_group_lset_table = LSetTableMake();
  res->time_nhood_lset_table = LSetTableMake();
  *** */
  res->finalized = KHE_FINALIZED_NONE;
  if( DEBUG6 )
    fprintf(stderr, "- KheInstanceMakeBegin(%s) returning\n", id);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceDelete(KHE_INSTANCE ins)                                 */
/*                                                                           */
/*  Delete ins.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheInstanceDelete(KHE_INSTANCE ins)
{
  /* delete ins from each of its archives */
  while( HaArrayCount(ins->archives) > 0 )
    KheArchiveDeleteInstance(HaArrayFirst(ins->archives), ins);

  /* delete ins itself */
  HaArenaDelete(ins->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheInstanceArenaBegin(KHE_INSTANCE ins, bool large)             */
/*                                                                           */
/*  Return a fresh or recycled arena for use in constructing ins.            */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheInstanceArenaBegin(KHE_INSTANCE ins)
{
  return HaArenaMake(ins->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceArenaEnd(KHE_INSTANCE ins, HA_ARENA a)                   */
/*                                                                           */
/*  Receive back an arena.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheInstanceArenaEnd(KHE_INSTANCE ins, HA_ARENA a)
{
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceSetBack(KHE_INSTANCE ins, void *back)                    */
/*                                                                           */
/*  Set the back pointer of ins.                                             */
/*                                                                           */
/*****************************************************************************/

void KheInstanceSetBack(KHE_INSTANCE ins, void *back)
{
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_ALL,
    "KheInstanceSetBack called after KheInstanceMakeEnd");
  ins->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheInstanceBack(KHE_INSTANCE ins)                                  */
/*                                                                           */
/*  Return the back pointer of ins.                                          */
/*                                                                           */
/*****************************************************************************/

void *KheInstanceBack(KHE_INSTANCE ins)
{
  return ins->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_NHOOD KheInstanceSingletonTimeGroupNeighbourhood(         */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Return the unique singleton time group neighbourhood.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TIME_GROUP_NHOOD KheInstanceSingletonTimeGroupNeighbourhood(
  KHE_INSTANCE ins)
{
  return ins->singleton_tgn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceTimeGroupNeighbourhoodLSetTableInsert(KHE_INSTANCE ins,  */
/*    LSET key, KHE_TIME_GROUP_NHOOD tgn)                                    */
/*                                                                           */
/*  Insert tgn into the instance time group neighbourhood table, with the    */
/*  given key.  It is up to the caller to make sure there is not already     */
/*  a time group neighbourhood with this key.                                */
/*                                                                           */
/*****************************************************************************/

void KheInstanceTimeGroupNeighbourhoodTableInsert(KHE_INSTANCE ins,
  SSET *key, KHE_TIME_GROUP_NHOOD tgn)
{
  SSetTableInsert(ins->time_nhood_table, *key, (void *) tgn);
}

/* ***
void KheInstanceTimeGroupNeighbourhoodLSetTrieInsert(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP_NHOOD tgn)
{
  LSetTrieInsert(ins->time_nhood_lset_trie, key, (void *) tgn);
}
*** */

/* ***
void KheInstanceTimeGroupNeighbourhoodLSetTableInsert(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP_NHOOD tgn)
{
  LSetTableInsert(ins->time_nhood_lset_table, key, (void *) tgn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceTimeGroupNeighbourhoodLSetTableRetrieve(KHE_INSTANCE ins,*/
/*    LSET key, KHE_TIME_GROUP_NHOOD *tgn)                                   */
/*                                                                           */
/*  If the instance time group neighbourhood table contains a time group     */
/*  neighbourhood with the given key, set *tgn to that neighbourhood and     */
/*  return true.  Otherwise set *tgn to NULL and return false.               */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceTimeGroupNeighbourhoodTableRetrieve(KHE_INSTANCE ins,
  SSET *key, KHE_TIME_GROUP_NHOOD *tgn)
{
  return SSetTableRetrieve(ins->time_nhood_table, *key, (void **) tgn);
}

/* ***
bool KheInstanceTimeGroupNeighbourhoodLSetTrieRetrieve(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP_NHOOD *tgn)
{
  return LSetTrieRetrieve(ins->time_nhood_lset_trie, key, (void **) tgn);
}
*** */

/* ***
bool KheInstanceTimeGroupNeighbourhoodLSetTableRetrieve(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP_NHOOD *tgn)
{
  return LSetTableRetrieve(ins->time_nhood_lset_table, key, (void **) tgn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceDoResourceTypePartitioning(KHE_INSTANCE ins)             */
/*                                                                           */
/*  Carry out resource type partitioning on ins.                             */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceDoResourceTypePartitioning(KHE_INSTANCE ins)
{
  KHE_RESOURCE_TYPE rt;  int i, j, count;  KHE_RESOURCE r, r1, r2;
  KHE_EVENT_RESOURCE er;  KHE_CONSTRAINT c;  KHE_RESOURCE_GROUP domain;

  if( DEBUG7 )
    fprintf(stderr, "[ KheInstanceDoResourceTypePartitioning(%s)\n",
      KheInstanceId(ins));

  /* decide which resource types can be partitioned */
  HaArrayForEach(ins->resource_type_array, rt, i)
    KheResourceTypeSetResourceTypePartitioningOn(rt, true);
  HaArrayForEach(ins->event_resources, er, i)
    if( KheEventResourcePreventsResourceTypePartitioning(er, &rt) )
      KheResourceTypeSetResourceTypePartitioningOn(rt, false);

  /* if none of the resource types can be partitioned, return now */
  count = 0;
  HaArrayForEach(ins->resource_type_array, rt, i)
    if( KheResourceTypeResourceTypePartitioningOn(rt) )
    {
      if( DEBUG7 )
	fprintf(stderr, "  partitioning on in resource type %s\n",
	  KheResourceTypeId(rt));
      count++;
    }
  if( count == 0 )
  {
    if( DEBUG7 )
      fprintf(stderr,
	"] KheInstanceDoResourceTypePartitioning returning (count == 0)\n");
    return;
  }

  /* at least one resource type can be partitioned */
  HaArrayForEach(ins->resource_array, r, i)
    KheResourceResourceTypePartitioningInit(r);
  HaArrayForEach(ins->constraint_array, c, i)
    if( KheConstraintAffectsResourceTypePartitioning(c, &domain) )
    {
      /* each pair of resources needs to be merged */
      for( j = 1;  j < KheResourceGroupResourceCount(domain);  j++ )
      {
	r1 = KheResourceGroupResource(domain, j - 1);
	r2 = KheResourceGroupResource(domain, j);
	KheResourceResourceTypePartitioningMerge(r1, r2);
      }
    }

  /* build the new resource types based on the merged sets */
  HaArrayForEach(ins->resource_array, r, i)
    KheResourceResourceTypePartitionMakeResourceType(r);

  /* reset the types of all user-defined resource groups */
  HaArrayForEach(ins->resource_type_array, rt, i)
    KheResourceTypeResetResourceGroupResourceTypes(rt);

  /* reset the resource types of all event resources */
  HaArrayForEach(ins->event_resources, er, i)
    KheEventResourceResetResourceType(er);

  /* reset the resource_of_type values of all constraints */
  HaArrayForEach(ins->constraint_array, c, i)
    KheConstraintResetResourceOfType(c);
  if( DEBUG7 )
    fprintf(stderr,
      "] KheInstanceDoResourceTypePartitioning returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceFinalize(KHE_INSTANCE ins, bool audit_and_fix,           */
/*    bool resource_type_partitions, bool infer_resource_partitions,         */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Internal version of KheInstanceMakeEnd which returns false and sets *ke  */
/*  when something is wrong.                                                 */
/*                                                                           */
/*  Resources must be finalized after constraints, because they calculate    */
/*  available and unavailable times using KhePreferTimesConstraintDomain.    */
/*                                                                           */
/*****************************************************************************/
static void KheInstanceUserTimeGroupTableInsert(KHE_INSTANCE ins,
  KHE_TIME_GROUP tg);

static bool KheInstanceFinalize(KHE_INSTANCE ins, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  KML_ERROR *ke)
{
  int i;  KHE_TIME_GROUP tg, tg2;  KHE_EVENT_GROUP eg;  KHE_RESOURCE r;
  KHE_RESOURCE_TYPE rt;  KHE_EVENT e;  KHE_CONSTRAINT c;
  SSET empty_time_set, full_time_set;  KHE_TIME t;
  if( DEBUG4 )
    fprintf(stderr, "[ KheInstanceFinalize(%s, %s)\n", KheInstanceId(ins),
      infer_resource_partitions ? "true" : "false");
  HnAssert(ins->finalized == KHE_FINALIZED_NONE,
    "KheInstanceMakeEnd called twice");

  /* audit and fix, if required */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceFinalize calling KheInstanceAuditAndFix\n");
  if( audit_and_fix )
    KheInstanceAuditAndFix(ins);
  ins->finalized = KHE_FINALIZED_AUDIT_AND_FIX;

  /* add user time groups to the SSet table; but don't add duplicates */
  if( DEBUG4 )
    fprintf(stderr,
      "  KheInstanceFinalize calling KheInstanceUserTimeGroupTableInsert\n");
  HaArrayForEach(ins->time_group_array, tg, i)
    if(!KheInstanceUserTimeGroupTableRetrieve(ins,KheTimeGroupTimeSet(tg),&tg2) )
      KheInstanceUserTimeGroupTableInsert(ins, tg);

  /* finalize times */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceFinalize calling KheTimeFinalize\n");
  HaArrayForEach(ins->time_array, t, i)
    KheTimeFinalize(t);
  ins->finalized = KHE_FINALIZED_TIMES;

  /* finalize time groups (this includes building their neighbourhoods) */
  /* other time groups will be built and finalized within the calls below */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceFinalize calling KheTimeGroupFinalize\n");
  HaArrayForEach(ins->time_group_array, tg, i)
  {
    if( DEBUG4 )
      fprintf(stderr, "    KheInstanceMakeEnd finalizing time group %s\n",
	KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "(null)");
    KheTimeGroupFinalize(tg, ins->arena, true);
  }
  SSetInit(empty_time_set, ins->arena);
  ins->empty_time_group = KheTimeGroupMakeAndFinalize(ins,
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &empty_time_set, /* NULL, */
    true, ins->arena);
  SSetInit(full_time_set, ins->arena);
  HaArrayForEach(ins->time_array, t, i)
    SSetInsert(full_time_set, KheTimeIndex(t));
  ins->full_time_group = KheTimeGroupMakeAndFinalize(ins,
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &full_time_set, /* NULL, */
    true, ins->arena);
  ins->finalized = KHE_FINALIZED_TIME_GROUPS;

  /* finalize resource types */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceMakeEnd finalizing resource types\n");
  HaArrayForEach(ins->resource_type_array, rt, i)
    KheResourceTypeFinalize(rt);
  ins->finalized = KHE_FINALIZED_RESOURCE_GROUPS;

  /* finalize event groups */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceMakeEnd finalizing event groups\n");
  HaArrayForEach(ins->event_group_array, eg, i)
    KheEventGroupFinalize(eg);
  KheEventGroupFinalize(ins->full_event_group);
  KheEventGroupFinalize(ins->empty_event_group);
  HaArrayForEach(ins->event_array, e, i)
    KheEventGroupFinalize(KheEventSingletonEventGroup(e));
  ins->finalized = KHE_FINALIZED_EVENT_GROUPS;

  /* finalize constraints (must be done after inferring time breaks) */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceMakeEnd finalizing constraints\n");
  HaArrayForEach(ins->constraint_array, c, i)
    if( !KheConstraintFinalize(c, ke) )
      return false;
  ins->finalized = KHE_FINALIZED_CONSTRAINTS;

  /* record completeness (KheEventFinalize assumes this flag is set) */

  /* finalize events (must be done after finalizing constraints) */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceMakeEnd finalizing events\n");
  HaArrayForEach(ins->event_array, e, i)
    KheEventFinalize(e);
  ins->finalized = KHE_FINALIZED_EVENTS;

  /* finalize resources (must be done after finalizing constraints) */
  if( DEBUG4 )
    fprintf(stderr, "  KheInstanceMakeEnd finalizing resources\n");
  HaArrayForEach(ins->resource_array, r, i)
    KheResourceFinalize(r);
  ins->finalized = KHE_FINALIZED_RESOURCES;

  /* resource type partitions, if required */
  if( resource_type_partitions )
    KheInstanceDoResourceTypePartitioning(ins);

  /* infer resource partitions, if required */
  if( infer_resource_partitions )
  {
    HaArrayForEach(ins->event_array, e, i)
      KheEventPartitionSetAdmissible(e);
    HaArrayForEach(ins->resource_type_array, rt, i)
      if( !KheResourceTypeHasPartitions(rt) )
	KheResourceTypeInferPartitions(rt);
  }

  *ke = NULL;
  ins->finalized = KHE_FINALIZED_ALL;
  if( DEBUG4 )
    fprintf(stderr, "] KheInstanceFinalize(%s) returning\n", ins->id);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceMakeEnd(KHE_INSTANCE ins, bool audit_and_fix,            */
/*    bool resource_type_partitions, bool infer_resource_partitions,         */
/*    char **error_message)                                                  */
/*                                                                           */
/*  End the construction of ins; initialize data structures.  If             */
/*  successful, set *error_message to NULL and return true, otherwise        */
/*  set *error_message to an error message describing the problem and        */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceMakeEnd(KHE_INSTANCE ins, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  char **error_message)
{
  KML_ERROR ke;  bool res;
  if( KheInstanceFinalize(ins, audit_and_fix, resource_type_partitions,
	infer_resource_partitions, &ke) )
  {
    *error_message = NULL;
    res = true;
  }
  else
  {
    *error_message = KmlErrorString(ke);
    res = false;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceArchiveCount(KHE_INSTANCE ins)                            */
/*                                                                           */
/*  Return the number of archives containing ins.                            */
/*                                                                           */
/*****************************************************************************/

int KheInstanceArchiveCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->archives);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE KheInstanceArchive(KHE_INSTANCE ins, int i)                  */
/*                                                                           */
/*  Return the i'th archive containing ins.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_ARCHIVE KheInstanceArchive(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->archives, i);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheInstanceId(KHE_INSTANCE ins)                                    */
/*                                                                           */
/*  Return the Id attribute of ins.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheInstanceId(KHE_INSTANCE ins)
{
  return ins->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheInstanceName(KHE_INSTANCE ins)                                  */
/*                                                                           */
/*  Convenience function which returns the name of ins.                      */
/*                                                                           */
/*****************************************************************************/

char *KheInstanceName(KHE_INSTANCE ins)
{
  return ins->meta_name;
  /* return KheInstanceMetaDataName(KheInstanceMetaData(ins)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceSetMetaData(KHE_INSTANCE ins, char *name,                */
/*    char *contributor, char *date, char *country, char *description,       */
/*    char *remarks)                                                         */
/*                                                                           */
/*  Set the metadata fields of ins.                                          */
/*                                                                           */
/*****************************************************************************/

void KheInstanceSetMetaData(KHE_INSTANCE ins, char *name, char *contributor,
  char *date, char *country, char *description, char *remarks)
{
  ins->meta_name = HnStringCopy(name, ins->arena);
  ins->meta_contributor = HnStringCopy(contributor, ins->arena);
  ins->meta_date = HnStringCopy(date, ins->arena);
  ins->meta_country = HnStringCopy(country, ins->arena);
  ins->meta_description = HnStringCopy(description, ins->arena);
  ins->meta_remarks = HnStringCopy(remarks, ins->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceMetaData(KHE_INSTANCE ins, char **name,                  */
/*    char **contributor, char **date, char **country, char **description,   */
/*    char **remarks)                                                        */
/*                                                                           */
/*  Retrieve the metadata fields of ins.                                     */
/*                                                                           */
/*****************************************************************************/

void KheInstanceMetaData(KHE_INSTANCE ins, char **name, char **contributor,
  char **date, char **country, char **description, char **remarks)
{
  *name = ins->meta_name;
  *contributor = ins->meta_contributor;
  *date = ins->meta_date;
  *country = ins->meta_country;
  *description = ins->meta_description;
  *remarks = ins->meta_remarks;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheInstanceMetaDataText(KHE_INSTANCE ins)                          */
/*                                                                           */
/*  Return the instance metadata as a paragraph of English text.             */
/*                                                                           */
/*****************************************************************************/

char *KheInstanceMetaDataText(KHE_INSTANCE ins)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  char *s;

  /* handle the case of no metadata */
  if( HnStringIsWhiteSpaceOnly(ins->meta_name) &&
      HnStringIsWhiteSpaceOnly(ins->meta_contributor) &&
      HnStringIsWhiteSpaceOnly(ins->meta_date) &&
      HnStringIsWhiteSpaceOnly(ins->meta_country) &&
      HnStringIsWhiteSpaceOnly(ins->meta_description) &&
      HnStringIsWhiteSpaceOnly(ins->meta_remarks) )
    return "This instance has no metadata.";

  /* name */
  a = ins->arena;
  HnStringBegin(anc, a);
  if( HnStringIsWhiteSpaceOnly(ins->meta_name) )
    HnStringAdd(&anc, "This instance has no name.\n");
  else
    HnStringAdd(&anc, "This is instance %s.\n",
      HnStringCopyStripped(ins->meta_name, a));

  /* contributor and date */
  if( !HnStringIsWhiteSpaceOnly(ins->meta_contributor) )
  {
    if( !HnStringIsWhiteSpaceOnly(ins->meta_date) )
    {
      /* contributor and date */
      HnStringAdd(&anc, "It was contributed by %s on %s.\n",
        HnStringCopyStripped(ins->meta_contributor, a),
        HnStringCopyStripped(ins->meta_date, a));
    }
    else
    {
      /* contributor only */
      HnStringAdd(&anc, "It was contributed by %s.\n",
        HnStringCopyStripped(ins->meta_contributor, a));
    }
  }
  else
  {
    if( !HnStringIsWhiteSpaceOnly(ins->meta_date) )
    {
      /* date only */
      HnStringAdd(&anc, "  It was contributed on %s.\n",
        HnStringCopyStripped(ins->meta_date, a));
    }
  }

  /* country */
  if( !HnStringIsWhiteSpaceOnly(ins->meta_country) )
  {
    s = HnStringCopyStripped(ins->meta_country, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "Its country of origin is %s\n", s);
    else
      HnStringAdd(&anc, "Its country of origin is %s.\n", s);
  }

  /* description */
  if( !HnStringIsWhiteSpaceOnly(ins->meta_description) )
  {
    s = HnStringCopyStripped(ins->meta_description, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "%s\n", s);
    else
      HnStringAdd(&anc, "%s.\n", s);
  }

  /* remarks */
  if( !HnStringIsWhiteSpaceOnly(ins->meta_remarks) )
  {
    s = HnStringCopyStripped(ins->meta_remarks, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "%s\n", s);
    else
      HnStringAdd(&anc, "%s.\n", s);
  }

  /* all done */
  return HnStringEnd(anc);
}



/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE_METADATA KheInstanceMetaData(KHE_INSTANCE ins)              */
/*                                                                           */
/*  Return the metadata attribute of ins.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_INSTANCE_METADATA KheInstanceMetaData(KHE_INSTANCE ins)
{
  return ins->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceSetMetaData(KHE_INSTANCE ins, KHE_INSTANCE_METADATA md)  */
/*                                                                           */
/*  Set the metadata attribute of ins.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheInstanceSetMetaData(KHE_INSTANCE ins, KHE_INSTANCE_METADATA md)
{
  ins->meta_data = md;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MODEL KheInstanceModel(KHE_INSTANCE ins)                             */
/*                                                                           */
/*  Return the model attribute of ins.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MODEL KheInstanceModel(KHE_INSTANCE ins)
{
  return ins->model;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FINALIZED KheInstanceFinalized(KHE_INSTANCE ins)                     */
/*                                                                           */
/*  Return the degree of finalization of ins                                 */
/*                                                                           */
/*****************************************************************************/

KHE_FINALIZED KheInstanceFinalized(KHE_INSTANCE ins)
{
  return ins->finalized;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceComplete(KHE_INSTANCE ins)                               */
/*                                                                           */
/*  Return true if ins is complete.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheInstanceFinalized
bool KheInstanceComplete(KHE_INSTANCE ins)
{
  return ins->compl ete;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "auditing instances"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventContainsConstraint(KHE_EVENT e, KHE_CONSTRAINT c)           */
/*                                                                           */
/*  Return true if e contains c.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheEventContainsConstraint(KHE_EVENT e, KHE_CONSTRAINT c)
{
  int i;  KHE_CONSTRAINT c2;
  for( i = 0;  i < KheEventConstraintCount(e);  i++ )
  {
    c2 = KheEventConstraint(e, i);
    if( c2 == c )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAuditLinkedEvents(KHE_EVENT e1, KHE_EVENT e2,            */
/*    bool *started, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Audit linked events e1 and e2.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceAuditLinkedEvents(KHE_EVENT e1, KHE_EVENT e2,
  bool *started, int indent)
{
  int i;  KHE_CONSTRAINT c;  KHE_CONSTRAINT_TAG tag;
  for( i = 0;  i < KheEventConstraintCount(e1);  i++ )
  {
    c = KheEventConstraint(e1, i);
    tag = KheConstraintTag(c);
    if( (tag == KHE_SPLIT_EVENTS_CONSTRAINT_TAG ||
         tag == KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG ||
	 tag == KHE_ASSIGN_TIME_CONSTRAINT_TAG ||
	 tag == KHE_PREFER_TIMES_CONSTRAINT_TAG ||
	 tag == KHE_SPREAD_EVENTS_CONSTRAINT_TAG)
          && !KheEventContainsConstraint(e2, c) )
    {
      /* debug report */
      if( !*started && DEBUG2 )
	fprintf(stderr, "%*slinked events %s and %s:\n", indent, "",
	  KheEventId(e1), KheEventId(e2));
      *started = true;
      if( DEBUG2 )
	fprintf(stderr, "%*s  %s has extra constraint %s\n",
	  indent, "", KheEventId(e1), KheConstraintId(c));

      /* fix */
      if( tag == KHE_SPLIT_EVENTS_CONSTRAINT_TAG )
	KheSplitEventsConstraintAddEvent((KHE_SPLIT_EVENTS_CONSTRAINT) c, e2);
      else if( tag == KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG )
	KheDistributeSplitEventsConstraintAddEvent(
	  (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, e2);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAuditLinkEventsConstraints(KHE_INSTANCE ins, int indent) */
/*                                                                           */
/*  Audit and fix the link events constraints of ins.                        */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceAuditLinkEventsConstraints(KHE_INSTANCE ins, int indent)
{
  int i, j, k, l;  KHE_CONSTRAINT c;  KHE_LINK_EVENTS_CONSTRAINT lec;
  KHE_EVENT_GROUP eg;  KHE_EVENT e1, e2;  bool started;
  if( DEBUG2 )
    fprintf(stderr, "%*slink events constraints:\n", indent, "");
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LINK_EVENTS_CONSTRAINT_TAG )
    {
      if( DEBUG2 )
	fprintf(stderr, "%*s%s:\n", indent + 2, "", KheConstraintId(c));
      lec = KheToLinkEventsConstraint(c);
      for( j = 0;  j < KheLinkEventsConstraintEventGroupCount(lec);  j++ )
      {
	eg = KheLinkEventsConstraintEventGroup(lec, j);
	for( k = 0;  k < KheEventGroupEventCount(eg);  k++ )
	{
	  e1 = KheEventGroupEvent(eg, k);
	  for( l = k + 1;  l <  KheEventGroupEventCount(eg);  l++ )
	  {
	    e2 = KheEventGroupEvent(eg, l);
	    started = false;
	    KheInstanceAuditLinkedEvents(e1, e2, &started, indent + 4);
	    KheInstanceAuditLinkedEvents(e2, e1, &started, indent + 4);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceDoAudit(KHE_INSTANCE ins, bool fix, int indent, FILE *fp)*/
/*                                                                           */
/*  Audit ins, and if fix is true, fix it.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheInstanceDoAudit(KHE_INSTANCE ins, bool fix, int indent, FILE *fp)
{
  fprintf(fp, "%*s[ audit %sof instance %s:\n", indent, "",
    fix ? "and fix " : "", ins->id != NULL ? ins->id : "(no id)");
  KheInstanceAuditLinkEventsConstraints(ins, fix, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAudit(KHE_INSTANCE ins, int indent, FILE *fp)            */
/*                                                                           */
/*  Audit ins, printing the results onto fp.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheInstanceAudit(KHE_INSTANCE ins, int indent, FILE *fp)
{
  KheInstanceDoAudit(ins, false, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAuditAndFix(KHE_INSTANCE ins, int indent, FILE *fp)      */
/*                                                                           */
/*  Audit ins, printing the results onto fp, and also fix the problems.      */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAuditAndFix(KHE_INSTANCE ins)
{
  if( DEBUG2 )
    fprintf(stderr, "  [ audit and fix of instance %s:\n",
      ins->id != NULL ? ins->id : "(no id)");
  KheInstanceAuditLinkEventsConstraints(ins, 4);
  if( DEBUG2 )
    fprintf(stderr, "  ]\n");
}

/* ***
void KheInstanceAuditAndFix(KHE_INSTANCE ins, int indent, FILE *fp)
{
  KheInstanceDoAudit(ins, true, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddTimeGroup(KHE_INSTANCE ins, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Add user-defined time group tg to ins, assuming it is safe to do so.     */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddTimeGroup(KHE_INSTANCE ins, KHE_TIME_GROUP tg)
{
  HaArrayAddLast(ins->time_group_array, tg);
  if( KheTimeGroupId(tg) != NULL )
    HnTableAdd(ins->time_group_table, KheTimeGroupId(tg), tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceTimeGroupCount(KHE_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of time groups in ins.                                 */
/*                                                                           */
/*****************************************************************************/

int KheInstanceTimeGroupCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->time_group_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheInstanceTimeGroup(KHE_INSTANCE ins, int i)             */
/*                                                                           */
/*  Return the i'th time group of ins.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheInstanceTimeGroup(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->time_group_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveTimeGroup(KHE_INSTANCE ins, char *id,            */
/*    KHE_TIME_GROUP *tg)                                                    */
/*                                                                           */
/*  Retrieve a time group by Id.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveTimeGroup(KHE_INSTANCE ins, char *id,
  KHE_TIME_GROUP *tg)
{
  int pos;
  return HnTableRetrieve(ins->time_group_table, id, *tg, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheInstanceEmptyTimeGroup(KHE_INSTANCE ins)               */
/*                                                                           */
/*  Return the `empty' time group of ins.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheInstanceEmptyTimeGroup(KHE_INSTANCE ins)
{
  return ins->empty_time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheInstanceFullTimeGroupInternal(KHE_INSTANCE ins)        */
/*                                                                           */
/*  Return the `all' time group of ins, even before instance is complete.    */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheInstanceFullTimeGroupInternal(KHE_INSTANCE ins)
{
  /* return MArrayFirst(ins->packing_time_groups); */
  return ins->full_time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheInstanceFullTimeGroup(KHE_INSTANCE ins)                 */
/*                                                                           */
/*  Return the `all' time group of ins.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheInstanceFullTimeGroup(KHE_INSTANCE ins)
{
  HnAssert(KheInstanceFinalized(ins) >= KHE_FINALIZED_TIME_GROUPS,
    "KheInstanceFullTimeGroup called before KheInstanceMakeEnd");
  return KheInstanceFullTimeGroupInternal(ins);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceTimeGroupLSetTableInsert(KHE_INSTANCE ins,               */
/*    LSET key, KHE_TIME_GROUP tg)                                           */
/*                                                                           */
/*  Insert tg into the instance time group lset table, with the given key.   */
/*  It is up to the caller to make sure there is not already a time group    */
/*  with this key.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceUserTimeGroupTableInsert(KHE_INSTANCE ins,
  /* SSET *key, */ KHE_TIME_GROUP tg)
{
  if( DEBUG5 )
    fprintf(stderr, "KheInstanceUserTimeGroupTableInsert(%p, %p)\n",
      (void *) ins, (void *) tg);
  SSetTableInsert(ins->user_time_group_table, *KheTimeGroupTimeSet(tg),
    (void *) tg);
}

/* ***
void KheInstanceTimeGroupLSetTrieInsert(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP tg)
{
  if( DEBUG5 )
    fprintf(stderr, "KheInstanceTimeGroupLSetTrieInsert(%p, %p, %p)\n",
      (void *) ins, (void *) key, (void *) tg);
  LSetTrieInsert(ins->time_group_lset_trie, key, (void *) tg);
}
*** */

/* ***
void KheInstanceTimeGroupLSetTableInsert(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP tg)
{
  LSetTableInsert(ins->time_group_lset_table, key, (void *) tg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceTimeGroupTableRetrieve(KHE_INSTANCE ins,                 */
/*    LSET key, KHE_TIME_GROUP *tg)                                          */
/*                                                                           */
/*  If the instance time group lset table contains a time group with the     */
/*  given key, set *tg to that time group and return true.  Otherwise set    */
/*  *tg to NULL and return false.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceUserTimeGroupTableRetrieve(KHE_INSTANCE ins,
  SSET *key, KHE_TIME_GROUP *tg)
{
  bool res;
  res = SSetTableRetrieve(ins->user_time_group_table, *key, (void **) tg);
  if( DEBUG5 )
    fprintf(stderr, "KheInstanceUserTimeGroupSSetRetrieve(%p, %p) %s: %p)\n",
      (void *) ins, (void *) key, res ? "true" : "false", (void *) *tg);
  return res;
}

/* ***
bool KheInstanceTimeGroupLSetTrieRetrieve(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP *tg)
{
  bool res;
  res = LSetTrieRetrieve(ins->time_group_lset_trie, key, (void **) tg);
  if( DEBUG5 )
    fprintf(stderr, "KheInstanceTimeGroupLSetTrieRetrieve(%p, %p) %s: %p)\n",
      (void *) ins, (void *) key, res ? "true" : "false", (void *) *tg);
  return res;
}
*** */

/* ***
bool KheInstanceTimeGroupLSetTableRetrieve(KHE_INSTANCE ins,
  LSET key, KHE_TIME_GROUP *tg)
{
  return LSetTableRetrieve(ins->time_group_lset_table, key, (void **) tg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddTime(KHE_INSTANCE ins, KHE_TIME t)                    */
/*                                                                           */
/*  Add t to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddTime(KHE_INSTANCE ins, KHE_TIME t)
{
  HaArrayAddLast(ins->time_array, t);
  if( KheTimeId(t) != NULL )
    HnTableAdd(ins->time_table, KheTimeId(t), t);
  /* KheTimeGroupAdd TimeInternal(ins->full_time_group, t); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceTimeCount(KHE_INSTANCE ins)                               */
/*                                                                           */
/*  Return the number of times in ins.                                       */
/*                                                                           */
/*****************************************************************************/

int KheInstanceTimeCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->time_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheInstanceTime(KHE_INSTANCE ins, int i)                        */
/*                                                                           */
/*  Return the i'th time of ins.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheInstanceTime(KHE_INSTANCE ins, int i)
{
  HnAssert(i >= 0 && i < HaArrayCount(ins->time_array),
    "KheInstanceTime: i (%d) out of range (0 - %d)", i,
    HaArrayCount(ins->time_array) - 1);
  return HaArray(ins->time_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveTime(KHE_INSTANCE ins, char *id, KHE_TIME *t)    */
/*                                                                           */
/*  Retrieve a time from ins.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveTime(KHE_INSTANCE ins, char *id, KHE_TIME *t)
{
  int pos;
  return HnTableRetrieve(ins->time_table, id, *t, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource types"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddResourceType(KHE_INSTANCE ins, KHE_RESOURCE_TYPE rt,  */
/*    int *index)                                                            */
/*                                                                           */
/*  Add rt to ins, setting *index to its index.                              */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddResourceType(KHE_INSTANCE ins, KHE_RESOURCE_TYPE rt,
  int *index)
{
  if( DEBUG3 )
    fprintf(stderr, "KheInstanceAddResourceType(%p, %p %s)\n",
      (void *) ins, (void *) rt, KheResourceTypeId(rt) != NULL ?
      KheResourceTypeId(rt) : "-");
  *index = HaArrayCount(ins->resource_type_array);
  HaArrayAddLast(ins->resource_type_array, rt);
  if( KheResourceTypeId(rt) != NULL )
  {
    HnTableAdd(ins->resource_type_table, KheResourceTypeId(rt), rt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceResourceTypeCount(KHE_INSTANCE ins)                       */
/*                                                                           */
/*  Return the number of resource types of ins.                              */
/*                                                                           */
/*****************************************************************************/

int KheInstanceResourceTypeCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->resource_type_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheInstanceResourceType(KHE_INSTANCE ins, int i)       */
/*                                                                           */
/*  Return the i'th resource type of ins.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheInstanceResourceType(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->resource_type_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveResourceType(KHE_INSTANCE ins, char *id,         */
/*    KHE_RESOURCE_TYPE *rt)                                                 */
/*                                                                           */
/*  Retrieve a resource type by Id.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveResourceType(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE_TYPE *rt)
{
  int pos;
  if( DEBUG3 )
    fprintf(stderr, "KheInstanceRetrieveResourceType(%p, %s)\n",
      (void *) ins, id);
  return HnTableRetrieve(ins->resource_type_table, id, *rt, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "partitions"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddPartition(KHE_INSTANCE ins, KHE_RESOURCE_GROUP rg,    */
/*    int *index)                                                            */
/*                                                                           */
/*  Add partition rg to ins, returning its index in *index.                  */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddPartition(KHE_INSTANCE ins, KHE_RESOURCE_GROUP rg,
  int *index)
{
  *index = HaArrayCount(ins->partition_array);
  HaArrayAddLast(ins->partition_array, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstancePartitionCount(KHE_INSTANCE ins)                          */
/*                                                                           */
/*  Return the number of partitions of ins.                                  */
/*                                                                           */
/*****************************************************************************/

int KheInstancePartitionCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->partition_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheInstancePartition(KHE_INSTANCE ins, int i)         */
/*                                                                           */
/*  Return the i'th partition of ins.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheInstancePartition(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->partition_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups and resources"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveResourceGroup(KHE_INSTANCE ins, char *id,        */
/*    KHE_RESOURCE_GROUP *rg)                                                */
/*                                                                           */
/*  Retrieve a resource group by Id from the resource types of ins.          */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveResourceGroup(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE_GROUP *rg)
{
  int i;  KHE_RESOURCE_TYPE rt;
  HaArrayForEach(ins->resource_type_array, rt, i)
    if( KheResourceTypeRetrieveResourceGroup(rt, id, rg) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveResource(KHE_INSTANCE ins, char *id,             */
/*    KHE_RESOURCE *r)                                                       */
/*                                                                           */
/*  Retrieve a resource by Id from the resource types of ins.                */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveResource(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE *r)
{
  int i;  KHE_RESOURCE_TYPE rt;
  HaArrayForEach(ins->resource_type_array, rt, i)
    if( KheResourceTypeRetrieveResource(rt, id, r) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceResourceCount(KHE_INSTANCE ins)                           */
/*                                                                           */
/*  Return the total number of resources in the instance.                    */
/*                                                                           */
/*****************************************************************************/

int KheInstanceResourceCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->resource_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheInstanceResource(KHE_INSTANCE ins, int i)                */
/*                                                                           */
/*  Return the i'th resource of ins.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheInstanceResource(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->resource_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddResource(KHE_INSTANCE ins, KHE_RESOURCE r)            */
/*                                                                           */
/*  Add r to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddResource(KHE_INSTANCE ins, KHE_RESOURCE r)
{
  HaArrayAddLast(ins->resource_array, r);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddEventGroup(KHE_INSTANCE ins, KHE_EVENT_GROUP eg)      */
/*                                                                           */
/*  Add eg to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddEventGroup(KHE_INSTANCE ins, KHE_EVENT_GROUP eg)
{
  if( DEBUG1 )
    fprintf(stderr, "KheInstanceAddEventGroup(ins, \"%s\")\n",
      KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "<null>");
  HaArrayAddLast(ins->event_group_array, eg);
  if( KheEventGroupId(eg) != NULL )
    HnTableAdd(ins->event_group_table, KheEventGroupId(eg), eg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceEventGroupCount(KHE_INSTANCE ins)                         */
/*                                                                           */
/*  Return the number of event groups of ins.                                */
/*                                                                           */
/*****************************************************************************/

int KheInstanceEventGroupCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->event_group_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheInstanceEventGroup(KHE_INSTANCE ins, int i)           */
/*                                                                           */
/*  Return the i'th event group of ins.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheInstanceEventGroup(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->event_group_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveEventGroup(KHE_INSTANCE ins, char *id,           */
/*    KHE_EVENT_GROUP *eg)                                                   */
/*                                                                           */
/*  Retrieve an event group from ins by id.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveEventGroup(KHE_INSTANCE ins, char *id,
  KHE_EVENT_GROUP *eg)
{
  int pos;
  if( DEBUG1 )
    fprintf(stderr, "KheInstanceRetrieveEventGroup(ins, \"%s\")\n", id);
  return HnTableRetrieve(ins->event_group_table, id, *eg, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheInstanceFullEventGroup(KHE_INSTANCE ins)              */
/*                                                                           */
/*  Return the `all' event group of ins.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheInstanceFullEventGroup(KHE_INSTANCE ins)
{
  return ins->full_event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheInstanceEmptyEventGroup(KHE_INSTANCE ins)             */
/*                                                                           */
/*  Return the empty event group of ins.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheInstanceEmptyEventGroup(KHE_INSTANCE ins)
{
  return ins->empty_event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddEvent(KHE_INSTANCE ins, KHE_EVENT e)                  */
/*                                                                           */
/*  Add e to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddEvent(KHE_INSTANCE ins, KHE_EVENT e)
{
  HaArrayAddLast(ins->event_array, e);
  if( KheEventId(e) != NULL )
    HnTableAdd(ins->event_table, KheEventId(e), e);
  if( KheEventPreassignedTime(e) == NULL )
    ins->all_events_preassigned = false;
  if( KheEventDuration(e) > ins->max_event_duration )
    ins->max_event_duration = KheEventDuration(e);
}


/*****************************************************************************/
/*                                                                           */
/*  ARRAY_KHE_EVENT KheInstanceEventsArray(KHE_INSTANCE ins)                 */
/*                                                                           */
/*  Return the event_array array of ins.                                     */
/*                                                                           */
/*****************************************************************************/

ARRAY_KHE_EVENT KheInstanceEventsArray(KHE_INSTANCE ins)
{
  return ins->event_array;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceEventCount(KHE_INSTANCE ins)                              */
/*                                                                           */
/*  Return the number of events in ins.                                      */
/*                                                                           */
/*****************************************************************************/

int KheInstanceEventCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->event_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheInstanceEvent(KHE_INSTANCE ins, int i)                      */
/*                                                                           */
/*  Return the i'th event of ins.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheInstanceEvent(KHE_INSTANCE ins, int i)
{
  HnAssert(i >= 0 && i < HaArrayCount(ins->event_array),
    "KheInstanceEvent: i out of range");
  return HaArray(ins->event_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveEvent(KHE_INSTANCE ins, char *id, KHE_EVENT *e)  */
/*                                                                           */
/*  Retrieve an event of ins by id.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveEvent(KHE_INSTANCE ins, char *id, KHE_EVENT *e)
{
  int pos;
  return HnTableRetrieve(ins->event_table, id, *e, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceAllEventsHavePreassignedTimes(KHE_INSTANCE ins)          */
/*                                                                           */
/*  Return true if all of the events of ins have preassigned times.          */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceAllEventsHavePreassignedTimes(KHE_INSTANCE ins)
{
  return ins->all_events_preassigned;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceMaximumEventDuration(KHE_INSTANCE ins)                    */
/*                                                                           */
/*  Return the maximum duration of any event of ins.                         */
/*                                                                           */
/*****************************************************************************/

int KheInstanceMaximumEventDuration(KHE_INSTANCE ins)
{
  return ins->max_event_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddEventResource(KHE_INSTANCE ins, KHE_EVENT_RESOURCE er)*/
/*                                                                           */
/*  Return a new index number for an event resource.                         */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddEventResource(KHE_INSTANCE ins, KHE_EVENT_RESOURCE er)
{
  HaArrayAddLast(ins->event_resources, er);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceEventResourceCount(KHE_INSTANCE ins)                      */
/*                                                                           */
/*  Return the number of event resources in ins.                             */
/*                                                                           */
/*****************************************************************************/

int KheInstanceEventResourceCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheInstanceEventResource(KHE_INSTANCE ins, int i)     */
/*                                                                           */
/*  Return the i'th event resource of ins.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheInstanceEventResource(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->event_resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraints"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceAddConstraint(KHE_INSTANCE ins, KHE_CONSTRAINT c)        */
/*                                                                           */
/*  Add c to ins.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheInstanceAddConstraint(KHE_INSTANCE ins, KHE_CONSTRAINT c)
{
  HaArrayAddLast(ins->constraint_array, c);
  if( KheConstraintId(c) != NULL )
    HnTableAdd(ins->constraint_table, KheConstraintId(c), c);
  ins->constraint_counts[KheConstraintTag(c)]++;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceConstraintCount(KHE_INSTANCE ins)                         */
/*                                                                           */
/*  Return the number of constraints of ins.                                 */
/*                                                                           */
/*****************************************************************************/

int KheInstanceConstraintCount(KHE_INSTANCE ins)
{
  return HaArrayCount(ins->constraint_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheInstanceConstraint(KHE_INSTANCE ins, int i)            */
/*                                                                           */
/*  Return the i'th constraint of ins.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheInstanceConstraint(KHE_INSTANCE ins, int i)
{
  return HaArray(ins->constraint_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceRetrieveConstraint(KHE_INSTANCE ins, char *id,           */
/*    KHE_CONSTRAINT *c)                                                     */
/*                                                                           */
/*  Retrieve a constraint from ins by id.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceRetrieveConstraint(KHE_INSTANCE ins, char *id,
  KHE_CONSTRAINT *c)
{
  int pos;
  return HnTableRetrieve(ins->constraint_table, id, *c, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceConstraintOfTypeCount(KHE_INSTANCE ins,                   */
/*    KHE_CONSTRAINT_TAG constraint_tag)                                     */
/*                                                                           */
/*  Return the number of constraints with the given constraint_tag.          */
/*                                                                           */
/*****************************************************************************/

int KheInstanceConstraintOfTypeCount(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag)
{
  return ins->constraint_counts[constraint_tag];
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceSetConstraintDensities(KHE_INSTANCE ins)                 */
/*                                                                           */
/*  Set the constraint densities of ins.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceSetConstraintDensities(KHE_INSTANCE ins)
{
  KHE_CONSTRAINT_TAG tag;  KHE_CONSTRAINT c;  KHE_EVENT_RESOURCE er;
  KHE_EVENT e;  int i, count, unpreass_event_resources, unpreass_events;

  /* find the number of unpreassigned event resources */
  unpreass_event_resources = 0;
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourcePreassignedResource(er) == NULL )
      unpreass_event_resources++;
  }

  /* find the number of unpreassigned events */
  unpreass_events = 0;
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) == NULL )
      unpreass_events++;
  }

  /* initialize the density_counts and density_totals arrays to 0 */
  for( tag = 0;  tag < KHE_CONSTRAINT_TAG_COUNT;  tag++ )
  {
    HaArrayAddLast(ins->density_counts, 0);
    HaArrayAddLast(ins->density_totals, 0);
  }

  /* set the density totals */
  HaArrayPut(ins->density_totals, KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG,
    unpreass_event_resources);
  HaArrayPut(ins->density_totals, KHE_ASSIGN_TIME_CONSTRAINT_TAG,
    unpreass_events);
  HaArrayPut(ins->density_totals, KHE_SPLIT_EVENTS_CONSTRAINT_TAG,
    KheInstanceEventCount(ins));
  HaArrayPut(ins->density_totals, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG,
    KheInstanceEventCount(ins));
  HaArrayPut(ins->density_totals, KHE_PREFER_RESOURCES_CONSTRAINT_TAG,
    unpreass_event_resources);
  HaArrayPut(ins->density_totals, KHE_PREFER_TIMES_CONSTRAINT_TAG,
    unpreass_events);
  HaArrayPut(ins->density_totals, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG,
    unpreass_event_resources);
  HaArrayPut(ins->density_totals, KHE_SPREAD_EVENTS_CONSTRAINT_TAG,
    KheInstanceEventCount(ins));
  HaArrayPut(ins->density_totals, KHE_LINK_EVENTS_CONSTRAINT_TAG,
    KheInstanceEventCount(ins));
  HaArrayPut(ins->density_totals, KHE_ORDER_EVENTS_CONSTRAINT_TAG,
    KheInstanceEventCount(ins));
  HaArrayPut(ins->density_totals, KHE_AVOID_CLASHES_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG,
    KheInstanceResourceCount(ins));
  HaArrayPut(ins->density_totals, KHE_LIMIT_RESOURCES_CONSTRAINT_TAG,
    unpreass_event_resources);

  /* accumulate the counts for each kind of constraint */
  HaArrayForEach(ins->constraint_array, c, i)
  {
    tag = KheConstraintTag(c);
    count = HaArray(ins->density_counts, tag);
    HaArrayPut(ins->density_counts, tag, count + KheConstraintDensityCount(c));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceConstraintDensityCount(KHE_INSTANCE ins,                  */
/*    KHE_CONSTRAINT_TAG constraint_tag)                                     */
/*                                                                           */
/*  Return the density count of constraints with this tag.                   */
/*                                                                           */
/*****************************************************************************/

int KheInstanceConstraintDensityCount(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag)
{
  if( HaArrayCount(ins->density_counts) == 0 )
    KheInstanceSetConstraintDensities(ins);
  return HaArray(ins->density_counts, constraint_tag);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInstanceConstraintDensityTotal(KHE_INSTANCE ins,                  */
/*    KHE_CONSTRAINT_TAG constraint_tag)                                     */
/*                                                                           */
/*  Return the density total of constraints with this tag.                   */
/*                                                                           */
/*****************************************************************************/

int KheInstanceConstraintDensityTotal(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag)
{
  if( HaArrayCount(ins->density_counts) == 0 )
    KheInstanceSetConstraintDensities(ins);
  return HaArray(ins->density_totals, constraint_tag);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool AddTimes(KML_ELT times_elt, KHE_INSTANCE ins, KML_ERROR *ke)        */
/*                                                                           */
/*  Add time groups and times to ins based on times_elt.                     */
/*                                                                           */
/*****************************************************************************/

static bool AddTimes(KML_ELT times_elt, KHE_INSTANCE ins, KML_ERROR *ke)
{
  KML_ELT time_groups_elt, time_group_elt, time_elt;
  int i, first_time_index;

  /* verify times_elt */
  if( !KmlCheck(times_elt, ": +TimeGroups *Time", ke) )
    return false;

  /* TimeGroups */
  first_time_index = 0;
  if( KmlContainsChild(times_elt, "TimeGroups", &time_groups_elt) )
  {
    first_time_index = 1;
    if( !KmlCheck(time_groups_elt, ": *Week *Day *TimeGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(time_groups_elt);  i++ )
    {
      time_group_elt = KmlChild(time_groups_elt, i);
      if( !KheTimeGroupMakeFromKml(time_group_elt, ins, ke) )
	return false;
    }
  }

  /* times */
  for( i = first_time_index;  i < KmlChildCount(times_elt);  i++ )
  {
    /* make and add one time */
    time_elt = KmlChild(times_elt, i);
    if( !KheTimeMakeFromKml(time_elt, ins, ke) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool AddResources(KML_ELT resources_elt, KHE_INSTANCE ins, KML_ERROR *ke)*/
/*                                                                           */
/*  Add resource types, resources groups, and resources to ins based on      */
/*  resources_elt.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool AddResources(KML_ELT resources_elt, KHE_INSTANCE ins, KML_ERROR *ke)
{
  KML_ELT resource_types_elt, resource_type_elt;
  KML_ELT resource_groups_elt, resource_group_elt;
  int i, first_resource_index;

  /* check */
  if( !KmlCheck(resources_elt, ": ResourceTypes +ResourceGroups *Resource",ke) )
    return false;

  /* resource types */
  resource_types_elt = KmlChild(resources_elt, 0);
  if( !KmlCheck(resource_types_elt, ": *ResourceType", ke) )
    return false;
  for( i = 0;  i < KmlChildCount(resource_types_elt);  i++ )
  {
    resource_type_elt = KmlChild(resource_types_elt, i);
    if( !KheResourceTypeMakeFromKml(resource_type_elt, ins, ke) )
      return false;
  }

  /* resource groups */
  first_resource_index = 1;
  if( KmlContainsChild(resources_elt, "ResourceGroups", &resource_groups_elt) )
  {
    if( !KmlCheck(resource_groups_elt, ": *ResourceGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(resource_groups_elt);  i++ )
    {
      resource_group_elt = KmlChild(resource_groups_elt, i);
      if( !KheResourceGroupMakeFromKml(resource_group_elt, ins, ke) )
	return false;
    }
    first_resource_index = 2;
  }

  /* resources */
  for( i = first_resource_index;  i < KmlChildCount(resources_elt);  i++ )
    if( !KheResourceMakeFromKml(KmlChild(resources_elt, i), ins, ke) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool AddEventGroups(KML_ELT event_groups_elt, KHE_INSTANCE ins,          */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Add event groups from event_groups_elt to ins.                           */
/*                                                                           */
/*****************************************************************************/

static bool AddEventGroups(KML_ELT event_groups_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  KML_ELT event_group_elt;  int i;
  if( !KmlCheck(event_groups_elt, ": *Course *EventGroup", ke) )
    return false;
  for( i = 0;  i < KmlChildCount(event_groups_elt);  i++ )
  {
    event_group_elt = KmlChild(event_groups_elt, i);
    if( !KheEventGroupMakeFromKml(event_group_elt, ins, ke) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool AddEvents(KML_ELT events_elt, KHE_INSTANCE ins, KML_ERROR *ke)      */
/*                                                                           */
/*  Add events from events_elt to ins.                                       */
/*                                                                           */
/*****************************************************************************/

static bool AddEvents(KML_ELT events_elt, KHE_INSTANCE ins, KML_ERROR *ke)
{
  int i, first_event_index;  KML_ELT event_groups_elt;
  if( !KmlCheck(events_elt, ": +EventGroups *Event", ke) )
    return false;
  first_event_index = 0;
  if( KmlContainsChild(events_elt, "EventGroups", &event_groups_elt) )
  {
    if( !AddEventGroups(event_groups_elt, ins, ke) )
      return false;
    first_event_index = 1;
  }
  for( i = first_event_index;  i < KmlChildCount(events_elt);  i++ )
    if( !KheEventMakeFromKml(KmlChild(events_elt, i), ins, ke) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool AddConstraints(KML_ELT constraints_elt, KHE_INSTANCE ins,           */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Add constraints from constraints_elt to ins.                             */
/*                                                                           */
/*****************************************************************************/

static bool AddConstraints(KML_ELT constraints_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  int i;
  if( !KmlCheck(constraints_elt, ":"
      " *AssignResourceConstraint"
      " *AssignTimeConstraint"
      " *SplitEventsConstraint"
      " *DistributeSplitEventsConstraint"
      " *PreferResourcesConstraint"
      " *PreferTimesConstraint"
      " *AvoidSplitAssignmentsConstraint"
      " *SpreadEventsConstraint"
      " *LinkEventsConstraint"
      " *OrderEventsConstraint"
      " *AvoidClashesConstraint"
      " *AvoidUnavailableTimesConstraint"
      " *LimitIdleTimesConstraint"
      " *ClusterBusyTimesConstraint"
      " *LimitBusyTimesConstraint"
      " *LimitWorkloadConstraint"
      " *LimitActiveIntervalsConstraint"
      " *LimitResourcesConstraint",
      ke) )
  {
    HnAssert(*ke != NULL, "AddConstraints internal error 1");
    return false;
  }
  for( i = 0;  i < KmlChildCount(constraints_elt);  i++ )
    if( !KheConstraintMakeFromKml(KmlChild(constraints_elt, i), ins, ke) )
    {
      HnAssert(*ke != NULL, "AddConstraints internal error 2");
      return false;
    }
  return true;
}

/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceMetaDataMakeFromKml(KML_ELT md_elt, KHE_INSTANCE ins,    */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Add instance metadata based on md_elt to ins.                            */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceMetaDataMakeFromKml(KML_ELT md_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  if( !KmlCheck(md_elt,
      ": $Name $Contributor $Date $Country $Description +$Remarks", ke) )
    return false;
  KheInstanceSetMetaData(ins,
    KmlText(KmlChild(md_elt, 0)),
    KmlText(KmlChild(md_elt, 1)),
    KmlText(KmlChild(md_elt, 2)),
    KmlText(KmlChild(md_elt, 3)),
    KmlText(KmlChild(md_elt, 4)),
    KmlChildCount(md_elt) == 6 ? KmlText(KmlChild(md_elt, 5)) : NULL);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceMakeFromKml(KML_ELT instance_elt, KHE_MODEL model,       */
/*    HA_ARENA_SET as, bool audit_and_fix, bool resource_type_partitions,    */
/*    bool infer_resource_partitions,                                        */
/*    KHE_INSTANCE *ins, KML_ERROR *ke)                                      */
/*                                                                           */
/*  Make an instance and add it to archive.  Pass infer_resource_partitions  */
/*  etc. to KheInstanceMakeEnd; and if audit_and_fix is true, before ending  */
/*  the instance, audit it an fix any problems.                              */
/*                                                                           */
/*****************************************************************************/

bool KheInstanceMakeFromKml(KML_ELT instance_elt, KHE_MODEL model,
  HA_ARENA_SET as, bool audit_and_fix, bool resource_type_partitions,
  bool infer_resource_partitions, KHE_INSTANCE *ins, KML_ERROR *ke)
{
  char *id;

  /* check instance_elt and make *ins */
  if( DEBUG6 )
    fprintf(stderr, "[ KheInstanceMakeFromKml\n");
  if( !KmlCheck(instance_elt,
      "Id : MetaData Times Resources Events Constraints", ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 1");
    return false;
  }
  id = KmlAttributeValue(instance_elt, 0);
  *ins = KheInstanceMakeBegin(id, model, as);

  /* add metadata, times, resources, events, and constraints */
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling Meta\n");
  if( !KheInstanceMetaDataMakeFromKml(KmlChild(instance_elt, 0), *ins, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 2");
    return false;
  }
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling AddTimes\n");
  if( !AddTimes(KmlChild(instance_elt, 1), *ins, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 3");
    return false;
  }
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling AddResources\n");
  if( !AddResources(KmlChild(instance_elt, 2), *ins, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 4");
    return false;
  }
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling AddEvents\n");
  if( !AddEvents(KmlChild(instance_elt, 3), *ins, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 5");
    return false;
  }
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling AddConstraints\n");
  if( !AddConstraints(KmlChild(instance_elt, 4), *ins, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 6");
    return false;
  }
  if( DEBUG6 )
    fprintf(stderr, "  KheInstanceMakeFromKml calling KheInstanceFinalize\n");
  if( !KheInstanceFinalize(*ins, audit_and_fix, resource_type_partitions,
	infer_resource_partitions, ke) )
  {
    HnAssert(*ke != NULL, "KheInstanceMakeFromKml internal error 7");
    return false;
  }
  /* KheInstanceMakeEnd(*ins, audit_and_fix, infer_resource_partitions); */
  if( DEBUG6 )
    fprintf(stderr, "] KheInstanceMakeFromKml\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceSortConstraints(KHE_INSTANCE ins)                        */
/*                                                                           */
/*  Sort the constraints of ins into increasing tag order.                   */
/*                                                                           */
/*  Implementation note.  We need a stable sort here, so we can't use        */
/*  qsort.  Instead, we use an insertion sort that will run in linear        */
/*  time when the constraints are already sorted.                            */
/*                                                                           */
/*****************************************************************************/

static void KheInstanceSortConstraints(KHE_INSTANCE ins)
{
  KHE_CONSTRAINT ci, cj;  int i, j;
  HaArrayForEach(ins->constraint_array, ci, i)
  {
    /* insert ci into its place among its predecessors */
    for( j = i - 1;  j >= 0;  j-- )
    {
      cj = HaArray(ins->constraint_array, j);
      if( KheConstraintTag(cj) <= KheConstraintTag(ci) )
	break;
      HaArrayPut(ins->constraint_array, j + 1, cj);
    }
    HaArrayPut(ins->constraint_array, j + 1, ci);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceMetaDataWrite(KHE_INSTANCE ins, KML_FILE kf)             */
/*                                                                           */
/*  Write instance metadata to xml.                                          */
/*                                                                           */
/*****************************************************************************/

static bool Has(char *val)
{
  return val != NULL && *val != '\0';
}

static char *Val(char *val, char *alt)
{
  return Has(val) ? val : alt;
}


void KheInstanceMetaDataWrite(KHE_INSTANCE ins, KML_FILE kf)
{
  if( Has(ins->meta_name) || Has(ins->meta_contributor) ||
      Has(ins->meta_date) || Has(ins->meta_country) ||
      Has(ins->meta_description) || Has(ins->meta_remarks) )
  {
    KmlBegin(kf, "MetaData");
    KmlEltPlainText(kf, "Name",
      Val(ins->meta_name, "No name"));
    KmlEltPlainText(kf, "Contributor",
      Val(ins->meta_contributor, "No contributor"));
    KmlEltPlainText(kf, "Date",
      Val(ins->meta_date, "No date"));
    KmlEltPlainText(kf, "Country",
      Val(ins->meta_country, "No country"));
    KmlEltPlainText(kf, "Description",
      Val(ins->meta_description, "No description"));
    if( Has(ins->meta_remarks) )
      KmlEltPlainText(kf, "Remarks", ins->meta_remarks);
    KmlEnd(kf, "MetaData");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheInstanceWrite(KHE_INSTANCE ins, KML_FILE kf)                     */
/*                                                                           */
/*  Write ins onto kf.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheInstanceWrite(KHE_INSTANCE ins, KML_FILE kf)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i;  KHE_RESOURCE r;
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_CONSTRAINT c;  HA_ARENA a;  bool rg_written;

  /* temporary memory while writing */
  if( DEBUG6 )
    fprintf(stderr, "[ KheInstanceWrite(%s)\n", ins->id);
  a = KheInstanceArenaBegin(ins);

  /* header and metadata */
  KmlBegin(kf, "Instance");
  HnAssert(ins->id != NULL,
    "KheArchiveWrite:  Id missing in Instance");
  KmlAttribute(kf, "Id", ins->id);
  /* *** NULL metadata is acceptable now
  HnAssert(ins->meta_data != NULL,
    "KheArchiveWrite:  MetaData missing in Instance");
  *** */
  KheInstanceMetaDataWrite(ins, kf);

  /* times */
  KmlBegin(kf, "Times");
  if( HaArrayCount(ins->time_group_array) > 0 )
  {
    KmlBegin(kf, "TimeGroups");
    /* patch suggested by Valentin Dreismann 9 Oct 2015 ***
    HaArrayForEach(ins->time_group_array, &tg, &i)
      if( !KheTimeGroupWrite(tg, kf) )
	return false;
    *** */
    HaArrayForEach(ins->time_group_array, tg, i)
      if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_WEEK )
	KheTimeGroupWrite(tg, kf);
    HaArrayForEach(ins->time_group_array, tg, i)
      if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_DAY )
	KheTimeGroupWrite(tg, kf);
    HaArrayForEach(ins->time_group_array, tg, i)
      if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_ORDINARY )
	KheTimeGroupWrite(tg, kf);
    KmlEnd(kf, "TimeGroups");
  }
  HaArrayForEach(ins->time_array, t, i)
    KheTimeWrite(t, kf, a);
  KmlEnd(kf, "Times");

  /* resource types */
  KmlBegin(kf, "Resources");
  KmlBegin(kf, "ResourceTypes");
  HaArrayForEach(ins->resource_type_array, rt, i)
    if( KheResourceTypeOriginalResourceType(rt) == rt )
      KheResourceTypeWrite(rt, kf);
  KmlEnd(kf, "ResourceTypes");

  /* resource groups */
  rg_written = false;
  /* KmlBegin(kf, "ResourceGroups"); */
  HaArrayForEach(ins->resource_type_array, rt, i)
    KheResourceTypeWriteResourceGroups(rt, kf, &rg_written);
  if( rg_written )
    KmlEnd(kf, "ResourceGroups");

  /* resources */
  HaArrayForEach(ins->resource_array, r, i)
    KheResourceWrite(r, kf);
  /* *** resource type partitioning might change the order here
  HaArrayForEach(ins->resource_type_array, rt, i)
    KheResourceTypeWriteResources(rt, kf);
  *** */
  KmlEnd(kf, "Resources");

  /* event groups */
  KmlBegin(kf, "Events");
  if( HaArrayCount(ins->event_group_array) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    /* patch corresponding to Valentin Dreismann's above, 9 Oct 2015 ***
    HaArrayForEach(ins->event_group_array, eg, i)
      if( !KheEventGroupWrite(eg, kf) )
	return false;
    *** */
    HaArrayForEach(ins->event_group_array, eg, i)
      if( KheEventGroupKind(eg) == KHE_EVENT_GROUP_KIND_COURSE )
	KheEventGroupWrite(eg, kf);
    HaArrayForEach(ins->event_group_array, eg, i)
      if( KheEventGroupKind(eg) == KHE_EVENT_GROUP_KIND_ORDINARY )
	KheEventGroupWrite(eg, kf);
    KmlEnd(kf, "EventGroups");
  }

  /* events */
  HaArrayForEach(ins->event_array, e, i)
    KheEventWrite(e, kf);
  KmlEnd(kf, "Events");

  /* constraints (sort them by tag first, using a stable sort) */
  KheInstanceSortConstraints(ins);
  KmlBegin(kf, "Constraints");
  HaArrayForEach(ins->constraint_array, c, i)
    KheConstraintWrite(c, kf);
  KmlEnd(kf, "Constraints");

  /* footer */
  KmlEnd(kf, "Instance");
  KheInstanceArenaEnd(ins, a);
  if( DEBUG6 )
    fprintf(stderr, "] KheInstanceWrite(%s) returning\n", ins->id);
}

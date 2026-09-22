
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
/*  FILE:         khe_sr_tgrc_comb.c                                         */
/*  DESCRIPTION:  Task grouping by resource constraints - comb. grouping     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG2 0
#define DEBUG3 0	/* recursive search */
#define DEBUG4 0	/* KheCombGrouperAddCombMTask */
#define DEBUG5 0	/* adding requirements */
#define DEBUG7 0	/* bases of recursion */
#define DEBUG8 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COVER_REQUIREMENT                                               */
/*                                                                           */
/*  One cover requirement (one time group or mtask requirement), including   */
/*  how many times it has been covered.                                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_cover_requirement_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_TIME_GROUP		time_group;	 /* if reqt is a time group  */
  KHE_MTASK			mtask;		 /* if reqt is am mtask      */
  KHE_TIME_SET			time_set;
  KHE_COMB_COVER_TYPE		cover_type;
  bool				last_cover_done; /* last mtask covering e set*/
  int				cover_count;
} *KHE_COVER_REQUIREMENT;

typedef HA_ARRAY(KHE_COVER_REQUIREMENT) ARRAY_KHE_COVER_REQUIREMENT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_MTASK                                                      */
/*                                                                           */
/*  Information about one mtask, including the cover_reqts it covers.        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_comb_mtask_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_MTASK			mtask;
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  ARRAY_KHE_COVER_REQUIREMENT	last_cover_reqts;
} *KHE_COMB_MTASK;

typedef HA_ARRAY(KHE_COMB_MTASK) ARRAY_KHE_COMB_MTASK;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_GROUPER - solver for combinatorial grouping                */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;
/* typedef HA_ARRAY(KHE_MTASK_GROUP) ARRAY_KHE_MTASK_GROUP; */

struct khe_comb_grouper_rec {

  /* free lists */
  /* ARRAY_KHE_MTASK_GROUP	mtask_group_free_list; */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqt_free_list;
  ARRAY_KHE_COMB_MTASK		comb_mtask_free_list;

  /* constant for the lifetime of the solver */
  HA_ARENA			arena;
  KHE_MTASK_FINDER		mtask_finder;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;

  /* requirements, may vary freely but always defined */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  bool				no_singles;
  KHE_RESOURCE_GROUP		preferred_domain;

  /* derived from the requirements; need resetting only when they are reset */
  bool				derived_info_up_to_date;
  ARRAY_KHE_MTASK		reqd_mtasks;
  ARRAY_KHE_COMB_MTASK		comb_mtasks;

  /* varying between and during solves */
  KHE_COMB_VARIANT_TYPE		variant;
  KHE_MTASK_GROUPER		curr_mtg;
  KHE_MTASK_GROUPER		best_mtg;	/* passed in by caller */
  KHE_COST			best_mtg_cost;
  int				zero_cost_groups_count;
  int				singles_tasks_count;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_COVER_TYPE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)                      */
/*                                                                           */
/*  Show a cover type.                                                       */
/*                                                                           */
/*****************************************************************************/

static char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)
{
  switch( cover_type )
  {
    case KHE_COMB_COVER_YES:	return "comb_cover_yes";
    case KHE_COMB_COVER_NO:	return "comb_cover_no";
    case KHE_COMB_COVER_PREV:	return "comb_cover_prev";
    case KHE_COMB_COVER_FREE:	return "comb_cover_free";

    default:

      HnAbort("CoverTypeShow: unexpected cover type (%d)", cover_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_VARIANT_TYPE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)                      */
/*                                                                           */
/*  Show a comb variant.                                                     */
/*                                                                           */
/*****************************************************************************/

static char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)
{
  switch( cgv )
  {
    case KHE_COMB_VARIANT_MIN:	 	return "comb_variant_min";
    case KHE_COMB_VARIANT_ZERO:	 	return "comb_variant_zero";
    case KHE_COMB_VARIANT_SOLE_ZERO:	return "comb_variant_sole_zero";
    case KHE_COMB_VARIANT_SINGLES:	return "comb_variant_singles";

    default:

      HnAbort("KheCombVariantShow: unexpected cost type (%d)", cgv);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COVER_REQUIREMENT"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)            */
/*                                                                           */
/*  Make a cover requirement object.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  if( HaArrayCount(cg->cover_reqt_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->cover_reqt_free_list);
    KheTimeSetClear(res->time_set);
  }
  else
  {
    HaMake(res, cg->arena);
    res->time_set = KheTimeSetMake(KheSolnInstance(cg->soln), cg->arena);
  }
  res->grouper = cg;
  res->time_group = tg;
  res->mtask = mt;
  if( tg != NULL )
    KheTimeSetAddTimeGroup(res->time_set, tg);
  res->cover_type = cover;
  res->last_cover_done = false;
  res->cover_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)  */
/*                                                                           */
/*  Return true if cr is covered by mt.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)
{
  if( cr->time_group != NULL )
    return !KheTimeSetDisjoint(cr->time_set, KheMTaskTimeSet(mt));
  else
    return cr->mtask == mt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr, FILE *fp)  */
/*                                                                           */
/*  Debug print of the header of a cover requirement.                        */
/*                                                                           */
/*****************************************************************************/

static void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr,
  FILE *fp)
{
  if( cr->time_group != NULL )
    fprintf(fp, "CoverRequirement(tg %s, %s, %d covers)",
      KheTimeGroupId(cr->time_group), CoverTypeShow(cr->cover_type),
      cr->cover_count);
  else
    fprintf(fp, "CoverRequirement(mt %p %s %s, %d tasks, %s, %d covers)",
      (void *) cr->mtask, KheMTaskId(cr->mtask),
      KheIntervalShow(KheMTaskInterval(cr->mtask), cr->grouper->days_frame),
      KheMTaskTaskCount(cr->mtask), CoverTypeShow(cr->cover_type),
      cr->cover_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp);

static void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCoverRequirementHeaderDebug(cr, fp);
    fprintf(fp, "\n");
  }
  else
    KheCoverRequirementHeaderDebug(cr, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_MTASK"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)       */
/*                                                                           */
/*  Make an mtask info object.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COMB_MTASK res;
  if( HaArrayCount(cg->comb_mtask_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->comb_mtask_free_list);
    HaArrayClear(res->cover_reqts);
    HaArrayClear(res->last_cover_reqts);
  }
  else
  {
    HaMake(res, cg->arena);
    HaArrayInit(res->cover_reqts, cg->arena);
    HaArrayInit(res->last_cover_reqts, cg->arena);
  }
  res->grouper = cg;
  res->mtask = mt;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)                           */
/*                                                                           */
/*  Return the "size" of cmt's domain, which is either the number of         */
/*  resources in cmt->mtask's domain, or INT_MAX if that does not suit.      */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)
{
  KHE_RESOURCE_GROUP domain;
  domain = KheMTaskDomain(cmt->mtask);
  if( KheResourceGroupSubset(cmt->grouper->preferred_domain, domain) )
    return KheResourceGroupResourceCount(domain);
  else
    return INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting an array of comb mtask objects.          */
/*                                                                           */
/*  The first priority is to sort the array so that mtasks that start on     */
/*  earlier days come before mtasks that start on later days.  This is       */
/*  vital because we use intervals during the solve and they assume that     */
/*  sets of mtasks have no gaps.                                             */
/*                                                                           */
/*  The second priority is to sort the array so that mtasks whose domains    */
/*  are a better fit for cg->preferred_domain (if present) come before       */
/*  mtasks whose fit is worse.  If the mtask's domain is not a superset      */
/*  of the preferred domain, it is maximally bad.  If it is a superset,      */
/*  then smaller domains are preferred to larger ones.                       */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskCmp(const void *t1, const void *t2)
{
  KHE_COMB_MTASK cmt1, cmt2;  KHE_INTERVAL in1, in2;  int cmp;
  cmt1 = * (KHE_COMB_MTASK *) t1;
  cmt2 = * (KHE_COMB_MTASK *) t2;

  /* first priority: chronologically earlier mtasks precede later ones */
  in1 = KheMTaskInterval(cmt1->mtask);
  in2 = KheMTaskInterval(cmt2->mtask);
  cmp = KheIntervalFirst(in1) - KheIntervalFirst(in2);
  if( cmp != 0 )
    return cmp;

  /* second priority: preferred domain, if requested */
  if( cmt1->grouper->preferred_domain != NULL )
  {
    cmp = KheCombMTaskDomainSize(cmt1) - KheCombMTaskDomainSize(cmt2);
    if( cmp != 0 )
      return cmp;
  }

  /* no difference */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)                      */
/*                                                                           */
/*  Return true if cmt is the last possible cover for a cover requirement    */
/*  that is not yet covered, so that consequently it must be included.       */
/*                                                                           */
/*****************************************************************************/

static bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->last_cover_reqts, cr, i)
    if( cr->cover_count == 0 )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header of cmt onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)
{
  fprintf(fp, "CombMTask(%p %s, %d tasks, %d unassigned, %d cover_reqts, "
    "%d last_cover)", (void *) cmt->mtask, KheMTaskId(cmt->mtask),
    KheMTaskTaskCount(cmt->mtask), KheMTaskUnassignedTaskCount(cmt->mtask),
    HaArrayCount(cmt->cover_reqts), HaArrayCount(cmt->last_cover_reqts));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cmt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCombMTaskHeaderDebug(cmt, fp);
    fprintf(fp, "\n");
  }
  else
    KheCombMTaskHeaderDebug(cmt, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - construction and query"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,                */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new comb grouper based on mtf.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_COMB_GROUPER res;

  /* free lists */
  HaMake(res, a);
  /* HaArrayInit(res->mtask_group_free_list, a); */
  HaArrayInit(res->cover_reqt_free_list, a);
  HaArrayInit(res->comb_mtask_free_list, a);

  /* constant for the lifetime of the solver */
  res->arena = a;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->resource_type = rt;  /* KheMTaskFinderResourceType(mtf); */
  res->days_frame = KheMTaskFinderDaysFrame(mtf);

  /* requirements, may vary freely but always defined */
  HaArrayInit(res->cover_reqts, a);
  res->no_singles = false;
  res->preferred_domain = NULL;

  /* derived from the requirements; need resetting only when they are reset */
  res->derived_info_up_to_date = false;
  HaArrayInit(res->reqd_mtasks, a);
  HaArrayInit(res->comb_mtasks, a);

  /* varying between and during solves */
  res->variant = KHE_COMB_VARIANT_SINGLES;
  res->curr_mtg = KheMTaskGrouperMake(a);
  res->best_mtg = NULL;   /* passed in by caller */
  res->best_mtg_cost = KheCost(INT_MAX, INT_MAX);
  res->zero_cost_groups_count = 0;
  res->singles_tasks_count = 0;

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Return the mtask finder attribute of cg.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)
{
  return cg->mtask_finder;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheCombGrouperSoln(KHE_COMB_GROUPER cg)                         */
/*                                                                           */
/*  Return cg's soln.                                                        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheCombGrouperSoln(KHE_COMB_GROUPER cg)
{
  return cg->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)        */
/*                                                                           */
/*  Return the resource type handled by cg.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)
{
  return cg->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheCombGrouperArena(KHE_COMB_GROUPER cg)                        */
/*                                                                           */
/*  Return cg's arena.                                                       */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheCombGrouperArena(KHE_COMB_GROUPER cg)
{
  return cg->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - mtask group free list"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUP KheCombGrouperGetMTaskGroup(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Get a free mtask group object from cg's free list; or return NULL        */
/*  if the free list is empty.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MTASK_GROUP KheCombGrouperGetMTaskGroup(KHE_COMB_GROUPER cg)
{
  if( HaArrayCount(cg->mtask_group_free_list) > 0 )
    return HaArrayLastAndDelete(cg->mtask_group_free_list);
  else
    return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperPutMTaskGroup(KHE_COMB_GROUPER cg, KHE_MTASK_GROUP mg)*/
/*                                                                           */
/*  Put mg onto cg's free list.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperPutMTaskGroup(KHE_COMB_GROUPER cg, KHE_MTASK_GROUP mg)
{
  HaArrayAddLast(cg->mtask_group_free_list, mg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - loading requirements"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Clear away all requirements.                                             */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)
{
  int i;
  HaArrayAppend(cg->cover_reqt_free_list, cg->cover_reqts, i);
  HaArrayClear(cg->cover_reqts);
  cg->no_singles = false;
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperClearRequirements(cg) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)                          */
/*                                                                           */
/*  Add a time group cover requirement to the solver.                        */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, tg, NULL, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddTimeGroupRequirement(cg, %s, %s) ]\n",
      KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-",
      CoverTypeShow(cover));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Undo the effect of a corresponding time group add.                       */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->time_group == tg )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteTimeGroupRequirement: tg not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,              */
/*    KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)                               */
/*                                                                           */
/*  Add an mtask cover requirement to the solver.                            */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, NULL, mt, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddMTaskRequirement(cg, mt) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,           */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Undo the effect of a corresponding mtask add.                            */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->mtask == mt )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteMTaskRequirement: mt not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Add a requirement that there be no singles.                              */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)       */
/*                                                                           */
/*  Undo the effect of a corresponding no singles add.                       */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,    */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add a preferred domain requirement (for rg) to cg.                       */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,
  KHE_RESOURCE_GROUP rg)
{
  cg->preferred_domain = rg;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg) */
/*                                                                           */
/*  Delete any preferred domain requirement from cg.                         */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg)
{
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - setting up for solving"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Set the required mtasks of cg.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayClear(cg->reqd_mtasks);
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_YES && cr->mtask != NULL )
      HaArrayAddLast(cg->reqd_mtasks, cr->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,    */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Return true if mt clashes with any of the required mtasks of cg,         */
/*  without being required itself.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_MTASK mt2;  int i, pos;
  if( !HaArrayContains(cg->reqd_mtasks, mt, &pos) )
  {
    HaArrayForEach(cg->reqd_mtasks, mt2, i)
      if( !KheIntervalDisjoint(KheMTaskInterval(mt), KheMTaskInterval(mt2)) )
	return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)      */
/*                                                                           */
/*  Return true if mt is wanted, because it has not already been chosen, it  */
/*  it does not cover any No requirement, it is not fixed and it needs       */
/*  assignment, and it does not clash with any required mtask.               */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;  KHE_COMB_MTASK cmt;

  /* if mt has already been chosen, we don't want it again */
  HaArrayForEach(cg->comb_mtasks, cmt, i)
    if( cmt->mtask == mt )
      return false;

  /* if mt covers any No requirement, we don't want it (2) */
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_NO &&
	KheCoverRequirementCovered(cr, mt) )
      return false;

  /* if mt is fixed or does not need assignment, we don't want it (3) */
  if( KheMTaskAssignIsFixed(mt) || !KheMTaskNeedsAssignment(mt) )
    return false;

  /* if mt is not in cg->reqd_mtasks but it does clash with them, no */
  if( KheCombGrouperMTaskClashesWithRequiredMTask(cg, mt) )
    return false;

  /* otherwise all good and we want mt */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperSetUpForSolving(KHE_COMB_GROUPER cg,                  */
/*    KHE_COMB_VARIANT_TYPE cg_variant)                                      */
/*                                                                           */
/*  Set up cg for solving.  Return true if it is worth doing the solve.      */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSetUpForSolving(KHE_COMB_GROUPER cg,
  KHE_COMB_VARIANT_TYPE cg_variant)
{
  KHE_COVER_REQUIREMENT cr;  int i, j;
  KHE_MTASK mt;  KHE_COMB_MTASK cmt;  KHE_MTASK_SET mts;

  /* derived from the requirements */
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSetUpForSolving(cg, cg_variant)\n");
  if( !cg->derived_info_up_to_date )
  {
    /* initialize reqd_mtasks */
    KheCombGrouperSetRequiredMTasks(cg);

    /* initialize comb_mtasks for this solve, first removing any old mtasks */
    HaArrayAppend(cg->comb_mtask_free_list, cg->comb_mtasks, i);
    HaArrayClear(cg->comb_mtasks);
    HaArrayForEach(cg->cover_reqts, cr, i)
      if( cr->cover_type != KHE_COMB_COVER_NO )
      {
	if( cr->time_group != NULL )
	{
	  /* cr is a time group */
	  mts = KheMTaskFinderMTasksInTimeGroup(cg->mtask_finder,
	    cg->resource_type, cr->time_group);
	  for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
	  {
	    mt = KheMTaskSetMTask(mts, j);
	    if( KheCombGrouperMTaskIsWanted(cg, mt) )
	      HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, mt));
	  }
	}
	else
	{
	  /* cr is an mtask */
	  if( KheCombGrouperMTaskIsWanted(cg, cr->mtask) )
	    HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, cr->mtask));
	}
      }

    /* sort comb_mtasks into chronological order */
    HaArraySort(cg->comb_mtasks, &KheCombMTaskCmp);

    /* clear out last_cover_done fields */
    HaArrayForEach(cg->cover_reqts, cr, j)
      cr->last_cover_done = false;

    /* set last_cover_done and last_cover_reqts fields */
    HaArrayForEachReverse(cg->comb_mtasks, cmt, i)
      HaArrayForEach(cg->cover_reqts, cr, j)
        if( KheCoverRequirementCovered(cr, cmt->mtask) )
	{
	  /* add cr to cmt->cover_reqts */
	  HaArrayAddLast(cmt->cover_reqts, cr);

	  /* sort out last_cover */
	  if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
	  {
	    /* cmt is e's last cover */
            cr->last_cover_done = true;
	    HaArrayAddLast(cmt->last_cover_reqts, cr);
	  }
	}

    /* if any Yes cover requirement has no last cover, we can quit now */
    HaArrayForEach(cg->cover_reqts, cr, j)
      if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
      {
	if( DEBUG2 )
	{
	  KheCombGrouperDebug(cg, 2, 2, stderr);
	  fprintf(stderr, "  not covered: ");
	  KheCoverRequirementDebug(cr, 2, 0, stderr);
	  fprintf(stderr, "] KheCombGrouperSetUpForSolving returning false\n");
	}
	return false;
      }

    /* derived info is up to date now */
    cg->derived_info_up_to_date = true;
  }

  /* varying between and during solves */
  cg->variant = cg_variant;
  KheMTaskGrouperClear(cg->curr_mtg);
  cg->zero_cost_groups_count = 0;
  cg->singles_tasks_count = 0;

  /* check that cover counts are all zero */
  HaArrayForEach(cg->cover_reqts, cr, i)
    HnAssert(cr->cover_count == 0,
      "KheCombGrouperSetUpForSolving internal error 1 (index %d)\n", i);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSetUpForSolving returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - solving and debug"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt) */
/*                                                                           */
/*  Add cmt and return true, or return false and change nothing if adding    */
/*  cmt would not work with the current group.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt,
  int curr_index)
{
  KHE_COVER_REQUIREMENT cr;  int i;

  /* make sure that cmt's mtask is acceptable to the current group */
  if( !KheMTaskGrouperAddMTask(cg->curr_mtg, cmt->mtask) )    /* (6), (7) */
  {
    if( DEBUG4 )
      fprintf(stderr, "%*s  add mtask info failed on group (%d mtasks)\n",
	2*curr_index, "", KheMTaskGrouperMTaskCount(cg->curr_mtg));
    return false;
  }

  /* all good, update the cover counts and return true */
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,                  */
/*    KHE_COMB_MTASK cmt)                                                    */
/*                                                                           */
/*  Undo the corresponding call to KheCombGrouperAddCombMTask.               */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count--;
  KheMTaskGrouperDeleteMTask(cg->curr_mtg, cmt->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperCurrGroupCoversRequirements(KHE_COMB_GROUPER cg)      */
/*                                                                           */
/*  Return true if cg's current mtask group has valid cover.  This is (11).  */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperCurrGroupCoversRequirements(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr, prev_cr;  int i;
  prev_cr = NULL;
  HaArrayForEach(cg->cover_reqts, cr, i)
  {
    switch( cr->cover_type )
    {
      case KHE_COMB_COVER_YES:

	/* should be covered at least once by now */
	if( DEBUG8 && cr->cover_count <= 0 )
	{
	  fprintf(stderr, "  KheCurrGroupCoversRequirements failing:\n");
          KheCombGrouperDebug(cg, 2, 2, stderr);
	}
	HnAssert(cr->cover_count > 0,
	  "KheCombGrouperCurrGroupCoversRequirements internal error 1");
	break;

      case KHE_COMB_COVER_NO:

	/* should be not covered, in fact no mtasks that cover it are tried */
	HnAssert(cr->cover_count == 0,
	  "KheCombGrouperCurrGroupCoversRequirements internal error 2");
	break;

      case KHE_COMB_COVER_PREV:

	/* should be covered if and only if prev is covered */
	if( prev_cr != NULL &&
	    (cr->cover_count > 0) != (prev_cr->cover_count > 0) )
	  return false;
	break;

      case KHE_COMB_COVER_FREE:

	/* any cover is acceptable here */
	break;

      default:

	HnAbort("KheCombGrouperCurrGroupCoversRequirements internal error"
	  " (%d)", cr->cover_type);
    }
    prev_cr = cr;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)            */
/*                                                                           */
/*  Return true if the current group satisfies cg's singles requirement.     */
/*                                                                           */
/*****************************************************************************/

static bool KheCurrGroupSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)
{
  return KheMTaskGrouperMTaskCount(cg->curr_mtg) >= (cg->no_singles ? 2 : 1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)                      */
/*                                                                           */
/*  Return true if singles requires all mtasks from now on to be excluded.   */
/*                                                                           */
/*****************************************************************************/

static bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)
{
  return cg->variant == KHE_COMB_VARIANT_SINGLES &&
    KheMTaskGrouperMTaskCount(cg->curr_mtg) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)        */
/*                                                                           */
/*  Solve cg from cg->comb_mtask[curr_index] onwards.  Return true if it's   */
/*  time to stop.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)
{
  KHE_MTASK mt;  KHE_COMB_MTASK curr_cmt;  bool stopping;  KHE_COST cost;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cg, %d):\n", 2*curr_index+2,"",curr_index);
  if( curr_index >= HaArrayCount(cg->comb_mtasks) )
  {
    /* end of recursion; if suitable mtasks, try them */
    if( KheCurrGroupSatisfiesSinglesRequirement(cg) &&        /* (9)  */
        KheCombGrouperCurrGroupCoversRequirements(cg) )       /* (11) */
    {
      switch( cg->variant )
      {
	case KHE_COMB_VARIANT_MIN:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  break;

	case KHE_COMB_VARIANT_ZERO:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  if( cost == 0 )
	  {
	    /* new zero cost; save group and stop */
	    cg->zero_cost_groups_count++;
	    if( DEBUG3 )
	      fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	    return true;
	  }
	  break;

	case KHE_COMB_VARIANT_SOLE_ZERO:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  if( cost == 0 )
	  {
	    /* new zero cost; record its presence */
	    cg->zero_cost_groups_count++;
	    if( cg->zero_cost_groups_count > 1 )
	    {
	      if( DEBUG3 )
		fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	      return true;
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SINGLES:

	  if( KheMTaskGrouperMTaskCount(cg->curr_mtg) == 1 )
	  {
	    mt = KheMTaskGrouperMTask(cg->curr_mtg, 0);
	    cg->singles_tasks_count += KheMTaskUnassignedTaskCount(mt);
	  }
	  break;

	default:

	  HnAbort("KheCombGrouperSolveFrom internal error (variant %d)",
	    cg->variant);
	  break;
      }
    }
  }
  else
  {
    /* try including curr_cmt */
    curr_cmt = HaArray(cg->comb_mtasks, curr_index);
    if( DEBUG3 )
      KheCombMTaskDebug(curr_cmt, 2, 2*curr_index + 4, stderr);
    if( KheCombGrouperAddCombMTask(cg, curr_cmt, curr_index) )
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      KheCombGrouperDeleteCombMTask(cg, curr_cmt);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }

    /* try excluding curr_cmt, unless it is the last cover for some reqt */
    if( !KheCombMTaskMustBeIncluded(curr_cmt) &&
        !KheSinglesRequiresExclude(cg) )              /* (8) */
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", 2*curr_index + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperSolve(KHE_COMB_GROUPER cg,                            */
/*    KHE_COMB_VARIANT_TYPE cg_variant, KHE_MTASK_GROUP mg)                  */
/*                                                                           */
/*  Solve cg for the previously loaded requirements.                         */
/*                                                                           */
/*****************************************************************************/

bool KheCombGrouperSolve(KHE_COMB_GROUPER cg,
  KHE_COMB_VARIANT_TYPE cg_variant, KHE_MTASK_GROUPER mtg)
{
  bool res;

  /* boilerplate */
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSolve(cg, %s)\n",
      KheCombVariantShow(cg_variant));
  HnAssert(cg_variant >= KHE_COMB_VARIANT_MIN &&
    cg_variant <= KHE_COMB_VARIANT_SOLE_ZERO,
    "KheCombGrouperSolve: invalid cg_variant");
  HnAssert(mtg != NULL, "KheCombGrouperSolve: mtg is NULL");

  /* do the solve */
  cg->best_mtg = mtg;
  cg->best_mtg_cost = KheCost(INT_MAX, INT_MAX);
  KheMTaskGrouperClear(cg->best_mtg);
  if( KheCombGrouperSetUpForSolving(cg, cg_variant) )
    KheCombGrouperSolveFrom(cg, 0);

  /* sort out the consequences */
  switch( cg_variant )
  {
    case KHE_COMB_VARIANT_MIN:

      /* successful if there is at least one solution */
      res = (KheMTaskGrouperMTaskCount(cg->best_mtg) > 0);
      break;

    case KHE_COMB_VARIANT_ZERO:

      /* successful if there is at least one zero cost solution */
      res = (cg->zero_cost_groups_count >= 1);
      break;

    case KHE_COMB_VARIANT_SOLE_ZERO:

      /* successful if there is exactly one zero cost solution */
      res = (cg->zero_cost_groups_count == 1);
      break;

    case KHE_COMB_VARIANT_SINGLES:
    default:

      HnAbort("KheCombGrouperSolve: unknown cg_variant (%d)", cg_variant);
      res = false;  /* keep compiler happy */
      break;
  }

  /* all done */
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSolve returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouperSolveSingles(KHE_COMB_GROUPER cg)                      */
/*                                                                           */
/*  Find the number of singles.                                              */
/*                                                                           */
/*****************************************************************************/

int KheCombGrouperSolveSingles(KHE_COMB_GROUPER cg)
{
  int res;

  /* boilerplate */
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSolveSingles(cg)\n");

  /* do the solve */
  if( KheCombGrouperSetUpForSolving(cg, KHE_COMB_VARIANT_SINGLES) )
    KheCombGrouperSolveFrom(cg, 0);
  res = cg->singles_tasks_count;

  /* all done */
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSolveSingles returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp)
{
  KHE_COVER_REQUIREMENT cr;  int i;  KHE_COMB_MTASK cmt;
  fprintf(fp, "%*s[ CombGrouper(%s %s, no_singles %s, domain %s)\n",
    indent, "", KheInstanceId(KheSolnInstance(cg->soln)),
    KheResourceTypeId(cg->resource_type),
    bool_show(cg->no_singles), cg->preferred_domain != NULL ?
      KheResourceGroupId(cg->preferred_domain) : "-");
  HaArrayForEach(cg->cover_reqts, cr, i)
    KheCoverRequirementDebug(cr, verbosity, indent + 2, fp);
  fprintf(fp, "%*s------------\n", indent + 2, "");
  HaArrayForEach(cg->comb_mtasks, cmt, i)
  {
    fprintf(fp, "%*s", indent + 2, "");
    if( KheMTaskGrouperContainsMTask(cg->curr_mtg, cmt->mtask) )
      fprintf(fp, "** ");
    else
      fprintf(fp, "   ");
    KheCombMTaskDebug(cmt, verbosity, 0, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


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
/*  FILE:         khe_sr_task_group_domain.c                                 */
/*  DESCRIPTION:  Task group domain finder                                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_set.h"
#include "khe_solvers.h"

#define TGD_MAGIC	4927

#define DEBUG1	0
#define DEBUG2	0

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TGDF_SIMPLE_CACHE - a simple cache (an unsorted list, or NULL)  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK_GROUP_DOMAIN) ARRAY_KHE_TASK_GROUP_DOMAIN;

typedef struct khe_tgdf_simple_cache_rec {
  ARRAY_KHE_TASK_GROUP_DOMAIN		entries;
} *KHE_TGDF_SIMPLE_CACHE;

typedef HA_ARRAY(KHE_TGDF_SIMPLE_CACHE) ARRAY_KHE_TGDF_SIMPLE_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TGDF_COMPLEX_CACHE - a complex cache (array of simples, or NULL)*/
/*                                                                           */
/*****************************************************************************/

typedef struct khe_tgdf_complex_cache_rec {
  KHE_TASK_GROUP_DOMAIN			empty_rg_element;
  ARRAY_KHE_TGDF_SIMPLE_CACHE		non_empty_rg_entries;
} *KHE_TGDF_COMPLEX_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TGDF_CACHE - a cache, either simple or complex                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_tgdf_cache_rec {
  KHE_TGDF_SIMPLE_CACHE			simple_cache;
  KHE_TGDF_COMPLEX_CACHE		complex_cache;
} *KHE_TGDF_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_GROUP_DOMAIN_FINDER                                        */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;

struct khe_task_group_domain_finder_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  struct khe_tgdf_cache_rec		cache;
  /* bool				task_domains_open; */
  ARRAY_KHE_TASK_GROUP_DOMAIN		all_task_group_domains;
  ARRAY_KHE_RESOURCE_GROUP		all_dominance_task_domains;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_GROUP_DOMAIN                                               */
/*                                                                           */
/*****************************************************************************/

struct khe_task_group_domain_rec {
  int					magic;
  KHE_TASK_GROUP_DOMAIN			prev;
  KHE_RESOURCE_GROUP			rg;
  KHE_RESOURCE_GROUP			result_rg;
  /* KHE_TASK_GROUP_DOMAIN_TYPE		type; */
  KHE_TASK_BOUND			task_bound;
  struct khe_tgdf_cache_rec		cache;
  KHE_SET				dominance_task_domains;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TGDF_SIMPLE_CACHE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TGDF_SIMPLE_CACHE KheTgdfSimpleCacheMake(HA_ARENA a)                 */
/*                                                                           */
/*  Make a new, empty simple cache.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TGDF_SIMPLE_CACHE KheTgdfSimpleCacheMake(HA_ARENA a)
{
  KHE_TGDF_SIMPLE_CACHE res;
  HaMake(res, a);
  HaArrayInit(res->entries, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfSimpleCacheInsert(KHE_TGDF_SIMPLE_CACHE cache,               */
/*    KHE_TASK_GROUP_DOMAIN val)                                             */
/*                                                                           */
/*  Insert val into cache.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTgdfSimpleCacheInsert(KHE_TGDF_SIMPLE_CACHE cache,
  KHE_TASK_GROUP_DOMAIN val)
{
  HnAssert(cache != NULL, "KheTgdfSimpleCacheInsert internal error");
  HaArrayAddLast(cache->entries, val);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTgdfSimpleCacheRetrieve(KHE_TGDF_SIMPLE_CACHE cache,             */
/*    KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)                     */
/*                                                                           */
/*  Retrieve a task group domain object with key rg from cache.  If          */
/*  successful, set *res to the result and return true, otherwise set        */
/*  *res to NULL and return false.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheTgdfSimpleCacheRetrieve(KHE_TGDF_SIMPLE_CACHE cache,
  KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)
{
  KHE_TASK_GROUP_DOMAIN tgd;  int i;
  if( cache != NULL )
    HaArrayForEach(cache->entries, tgd, i)
      if( KheResourceGroupEqual(tgd->rg, rg) )
	return *res = tgd, true;
  return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfSimpleCacheAddTaskDomain(KHE_TGDF_SIMPLE_CACHE cache,        */
/*    KHE_RESOURCE_GROUP rg, int rg_index)                                   */
/*                                                                           */
/*  Add task domain rg (whose index in task_domains is rg_index) to cache.   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheTgdfSimpleCacheAddTaskDomain(KHE_TGDF_SIMPLE_CACHE cache,
  KHE_RESOURCE_GROUP rg, int rg_index)
{
  KHE_TASK_GROUP_DOMAIN tgd;  int i;
  HaArrayForEach(cache->entries, tgd, i)
    KheTaskGroupDomainDominanceAddTaskDomain(tgd, rg, rg_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTgdfSimpleCacheReadyToConvert(KHE_TGDF_SIMPLE_CACHE cache)       */
/*                                                                           */
/*  Return true if cache is ready for conversion from simple to complex.     */
/*                                                                           */
/*****************************************************************************/

static bool KheTgdfSimpleCacheReadyToConvert(KHE_TGDF_SIMPLE_CACHE cache)
{
  return cache != NULL && HaArrayCount(cache->entries) > 10;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TGDF_COMPLEX_CACHE"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TGDF_COMPLEX_CACHE KheTgdfComplexCacheMake(HA_ARENA a)               */
/*                                                                           */
/*  Make a new, empty complex cache.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_TGDF_COMPLEX_CACHE KheTgdfComplexCacheMake(HA_ARENA a)
{
  KHE_TGDF_COMPLEX_CACHE res;
  HaMake(res, a);
  res->empty_rg_element = NULL;
  HaArrayInit(res->non_empty_rg_entries, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfComplexCacheInsert(KHE_TGDF_COMPLEX_CACHE cache,             */
/*    KHE_TASK_GROUP_DOMAIN val, HA_ARENA a)                                 */
/*                                                                           */
/*  Insert val into cache.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTgdfComplexCacheInsert(KHE_TGDF_COMPLEX_CACHE cache,
  KHE_TASK_GROUP_DOMAIN val, HA_ARENA a)
{
  KHE_RESOURCE r;  KHE_TGDF_SIMPLE_CACHE simple_cache;  int index;
  HnAssert(cache != NULL, "KheTgdfComplexCacheInsert internal error");
  if( KheResourceGroupResourceCount(val->rg) == 0 )
    cache->empty_rg_element = val;
  else
  {
    r = KheResourceGroupResource(val->rg, 0);
    index = KheResourceInstanceIndex(r);
    HaArrayFill(cache->non_empty_rg_entries, index + 1, NULL);
    simple_cache = HaArray(cache->non_empty_rg_entries, index);
    if( simple_cache == NULL )
    {
      simple_cache = KheTgdfSimpleCacheMake(a);
      HaArrayPut(cache->non_empty_rg_entries, index, simple_cache);
    }
    KheTgdfSimpleCacheInsert(simple_cache, val);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTgdfComplexCacheRetrieve(KHE_TGDF_COMPLEX_CACHE cache,           */
/*    KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)                     */
/*                                                                           */
/*  Retrieve a task group domain object with key rg from cache.  If          */
/*  successful, set *res to the result and return true, otherwise set        */
/*  *res to NULL and return false.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheTgdfComplexCacheRetrieve(KHE_TGDF_COMPLEX_CACHE cache,
  KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)
{
  KHE_RESOURCE r;  KHE_TGDF_SIMPLE_CACHE simple_cache;  int index;
  if( cache == NULL )
    return *res = NULL, false;
  else if( KheResourceGroupResourceCount(rg) == 0 )
    return *res = cache->empty_rg_element, *res != NULL;
  else
  {
    r = KheResourceGroupResource(rg, 0);
    index = KheResourceInstanceIndex(r);
    HaArrayFill(cache->non_empty_rg_entries, index + 1, NULL);
    simple_cache = HaArray(cache->non_empty_rg_entries, index);
    return KheTgdfSimpleCacheRetrieve(simple_cache, rg, res);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfComplexCacheAddTaskDomain(KHE_TGDF_COMPLEX_CACHE cache,      */
/*    KHE_RESOURCE_GROUP rg, int rg_index)                                   */
/*                                                                           */
/*  Add task domain rg (whose index in task_domains is rg_index) to cache.   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheTgdfComplexCacheAddTaskDomain(KHE_TGDF_COMPLEX_CACHE cache,
  KHE_RESOURCE_GROUP rg, int rg_index)
{
  KHE_TGDF_SIMPLE_CACHE simple_cache;  int i;
  if( cache != NULL )
  {
    if( cache->empty_rg_element != NULL )
      KheTaskGroupDomainDominanceAddTaskDomain(cache->empty_rg_element, rg, rg_index);
    HaArrayForEach(cache->non_empty_rg_entries, simple_cache, i)
      if( simple_cache != NULL )
	KheTgdfSimpleCacheAddTaskDomain(simple_cache, rg, rg_index);
  }
}
*** */

/* *** old version, seems erroneous
static void KheTgdfComplexCacheAddTaskDomain(KHE_TGDF_COMPLEX_CACHE cache,
  KHE_RESOURCE_GROUP rg, int rg_index)
{
  KHE_TGDF_SIMPLE_CACHE simple_cache;  int i;
  if( cache == NULL )
  {
    ** do nothing **
  }
  else if( cache->empty_rg_element != NULL )
    KheTaskGroupDomainDominanceAddTaskDomain(cache->empty_rg_element, rg, rg_index);
  else
  {
    HaArrayForEach(cache->non_empty_rg_entries, simple_cache, i)
      if( simple_cache != NULL )
        KheTgdfSimpleCacheAddTaskDomain(simple_cache, rg, rg_index);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TGDF_COMPLEX_CACHE KheTgdfComplexCacheBuild(                         */
/*    KHE_TGDF_SIMPLE_CACHE cache, HA_ARENA a)                               */
/*                                                                           */
/*  Build a complex cache holding the entries of cache, which is a           */
/*  non-empty simple cache.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_TGDF_COMPLEX_CACHE KheTgdfComplexCacheBuild(
  KHE_TGDF_SIMPLE_CACHE cache, HA_ARENA a)
{
  KHE_TGDF_COMPLEX_CACHE res;  KHE_TASK_GROUP_DOMAIN tgd;  int i;
  HnAssert(cache != NULL, "KheTgdfComplexCacheBuild internal error");
  res = KheTgdfComplexCacheMake(a);
  HaArrayForEach(cache->entries, tgd, i)
    KheTgdfComplexCacheInsert(res, tgd, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TGDF_CACHE"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfCacheInit(KHE_TGDF_CACHE cache)                              */
/*                                                                           */
/*  Initialize cache.  Since an empty simple cache can be represented by     */
/*  a NULL value, we start out with that.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheTgdfCacheInit(KHE_TGDF_CACHE cache)
{
  cache->simple_cache = NULL;
  cache->complex_cache = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfCacheInsert(KHE_TGDF_CACHE cache, KHE_TASK_GROUP_DOMAIN tgd, */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Insert tgd into cache.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTgdfCacheInsert(KHE_TGDF_CACHE cache, KHE_TASK_GROUP_DOMAIN tgd,
  HA_ARENA a)
{
  if( cache->complex_cache != NULL )
    KheTgdfComplexCacheInsert(cache->complex_cache, tgd, a);
  else
  {
    if( cache->simple_cache == NULL )
      cache->simple_cache = KheTgdfSimpleCacheMake(a);
    KheTgdfSimpleCacheInsert(cache->simple_cache, tgd);
    if( KheTgdfSimpleCacheReadyToConvert(cache->simple_cache) )
      cache->complex_cache = KheTgdfComplexCacheBuild(cache->simple_cache, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTgdfCacheRetrieve(KHE_TGDF_CACHE cache,                          */
/*    KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)                     */
/*                                                                           */
/*  Retrieve a task group domain object with key rg from cache.  If          */
/*  successful, set *res to the result and return true, otherwise set        */
/*  *res to NULL and return false.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheTgdfCacheRetrieve(KHE_TGDF_CACHE cache,
  KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN *res)
{
  if( cache->complex_cache != NULL )
    return KheTgdfComplexCacheRetrieve(cache->complex_cache, rg, res);
  else
    return KheTgdfSimpleCacheRetrieve(cache->simple_cache, rg, res);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTgdfCacheAddTaskDomain(KHE_TGDF_CACHE cache,                     */
/*    KHE_RESOURCE_GROUP rg, int rg_index)                                   */
/*                                                                           */
/*  Add task domain rg (whose index in task_domains is rg_index) to cache.   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheTgdfCacheAddTaskDomain(KHE_TGDF_CACHE cache,
  KHE_RESOURCE_GROUP rg, int rg_index)
{
  if( cache->complex_cache != NULL )
    KheTgdfComplexCacheAddTaskDomain(cache->complex_cache, rg, rg_index);
  else
    KheTgdfSimpleCacheAddTaskDomain(cache->simple_cache, rg, rg_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_GROUP_DOMAIN_FINDER"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN_FINDER KheTaskGroupDomainFinderMake(KHE_SOLN soln, */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a task group domain finder object.                                */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUP_DOMAIN_FINDER KheTaskGroupDomainFinderMake(KHE_SOLN soln,
  HA_ARENA a)
{
  KHE_TASK_GROUP_DOMAIN_FINDER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  KheTgdfCacheInit(&res->cache);
  HaArrayInit(res->all_task_group_domains, a);
  HaArrayInit(res->all_dominance_task_domains, a);
  if( DEBUG1 )
    fprintf(stderr, "%p = KheTaskGroupDomainFinderMake(soln to %s)\n",
      (void *) res, KheInstanceId(KheSolnInstance(soln)));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainFinderInsert(KHE_TASK_GROUP_DOMAIN_FINDER tgdf,   */
/*    KHE_TASK_GROUP_DOMAIN tgd)                                             */
/*                                                                           */
/*  Insert tgd into tgdf:  into its cache, and into all_task_group_domains.  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupDomainFinderInsert(KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_TASK_GROUP_DOMAIN tgd)
{
  HaArrayAddLast(tgdf->all_task_group_domains, tgd);
  if( tgd->prev == NULL )
    KheTgdfCacheInsert(&tgdf->cache, tgd, tgdf->arena);
  else
    KheTgdfCacheInsert(&tgd->prev->cache, tgd, tgdf->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupDomainFinderRetrieve(KHE_TASK_GROUP_DOMAIN_FINDER tgdf, */
/*    KHE_TASK_GROUP_DOMAIN prev, KHE_RESOURCE_GROUP rg,                     */
/*    KHE_TASK_GROUP_DOMAIN *res)                                            */
/*                                                                           */
/*  Retrieve a task group domain object with key (prev, rg) from tgdf.       */
/*  If successful, set *res to the result and return true.  Otherwise        */
/*  set *res to NULL and return false.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskGroupDomainFinderRetrieve(KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_TASK_GROUP_DOMAIN prev, KHE_RESOURCE_GROUP rg,
  KHE_TASK_GROUP_DOMAIN *res)
{
  if( prev == NULL )
    return KheTgdfCacheRetrieve(&tgdf->cache, rg, res);
  else
    return KheTgdfCacheRetrieve(&prev->cache, rg, res);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_GROUP_DOMAIN"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN KheTaskGroupDomainDoMake(                          */
/*    KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_TASK_GROUP_DOMAIN prev,         */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Make a KHE_TASK_GROUP_DOMAIN object for (prev, rg), or return prev       */
/*  if it is a subset of rg.                                                 */
/*                                                                           */
/*****************************************************************************/
static void KheTaskGroupDomainDominanceAddTaskDomain(KHE_TASK_GROUP_DOMAIN tgd,
  KHE_RESOURCE_GROUP rg, int rg_index);

static KHE_TASK_GROUP_DOMAIN KheTaskGroupDomainDoMake(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_TASK_GROUP_DOMAIN prev,
  KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE_GROUP result_rg, task_domain;  KHE_TASK_GROUP_DOMAIN res;
  int i;

  /* find result_rg and type */
  if( prev == NULL )
  {
    result_rg = rg;
  }
  else if( KheResourceGroupSubset(prev->result_rg, rg) )
  {
    result_rg = prev->result_rg;
    return prev;
  }
  else if( KheResourceGroupSubset(rg, prev->result_rg) )
  {
    result_rg = rg;
  }
  else
  {
    KheSolnResourceGroupBegin(tgdf->soln, KheResourceGroupResourceType(rg));
    KheSolnResourceGroupUnion(tgdf->soln, prev->result_rg);
    KheSolnResourceGroupIntersect(tgdf->soln, rg);
    result_rg = KheSolnResourceGroupEnd(tgdf->soln);
  }

  /* build a new result object and return it */
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheTaskGroupDomainDoMake(tgdf %p, ", (void *) tgdf);
    if( prev == NULL )
      fprintf(stderr, "NULL");
    else
      KheTaskGroupDomainDebug(prev, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheResourceGroupId(rg));
    if( prev != NULL )
    {
      fprintf(stderr, "  prev: ");
      KheResourceGroupDebug(prev->result_rg, 2, 0, stderr);
    }
    /* if( KheResourceGroupId(rg) == NULL ) */
    fprintf(stderr, "    rg: ");
    KheResourceGroupDebug(rg, 2, 0, stderr);
  }
  HaMake(res, tgdf->arena);
  res->magic = TGD_MAGIC;
  res->prev = prev;
  res->rg = rg;
  res->result_rg = result_rg;
  res->task_bound = KheTaskBoundMake(tgdf->soln, result_rg);
  KheTgdfCacheInit(&res->cache);

  /* find the set of compatible task domains */
  KheSetInit(res->dominance_task_domains, tgdf->arena);
  HaArrayForEach(tgdf->all_dominance_task_domains, task_domain, i)
    KheTaskGroupDomainDominanceAddTaskDomain(res, task_domain, i);

  /* all done, return res */
  if( DEBUG2 )
  {
    fprintf(stderr, "] KheTaskGroupDomainDoMake returning ");
    KheTaskGroupDomainDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN KheTaskGroupDomainMake(                            */
/*    KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_TASK_GROUP_DOMAIN prev,         */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Return an object representing the intersection of the resource           */
/*  groups represented by prev and rg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUP_DOMAIN KheTaskGroupDomainMake(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_TASK_GROUP_DOMAIN prev,
  KHE_RESOURCE_GROUP rg)
{
  KHE_TASK_GROUP_DOMAIN res;
  HnAssert(rg != NULL, "KheTaskGroupDomainMake internal error");
  if( !KheTaskGroupDomainFinderRetrieve(tgdf, prev, rg, &res) )
  {
    res = KheTaskGroupDomainDoMake(tgdf, prev, rg);
    KheTaskGroupDomainFinderInsert(tgdf, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainDominanceClear(KHE_TASK_GROUP_DOMAIN tgd)         */
/*                                                                           */
/*  Clear all dominance information from tgd.                                */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupDomainDominanceClear(KHE_TASK_GROUP_DOMAIN tgd)
{
  KheSetClear(tgd->dominance_task_domains);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainDominanceAddTaskDomain(KHE_TASK_GROUP_DOMAIN tgd, */
/*    KHE_RESOURCE_GROUP rg, int rg_index)                                   */
/*                                                                           */
/*  Add rg (whose index in task_domains is rg_index) to tgd.                 */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupDomainDominanceAddTaskDomain(KHE_TASK_GROUP_DOMAIN tgd,
  KHE_RESOURCE_GROUP rg, int rg_index)
{
  if( !KheResourceGroupDisjoint(rg, tgd->result_rg) )
    KheSetInsert(tgd->dominance_task_domains, rg_index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskGroupDomainValue(KHE_TASK_GROUP_DOMAIN tgd,    */
/*    KHE_TASK_GROUP_DOMAIN_TYPE *type)                                      */
/*                                                                           */
/*  Return the resource group represented by ri, as well as the type of      */
/*  calculation that produced that value.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskGroupDomainValue(KHE_TASK_GROUP_DOMAIN tgd)
  /* KHE_TASK_GROUP_DOMAIN_TYPE *type) */
{
  /* *type = tgd->type; */
  return tgd->result_rg;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskGroupDomainTaskBound(KHE_TASK_GROUP_DOMAIN tgd)    */
/*                                                                           */
/*  Return a task bound containing KheTaskGroupDomainValue.                  */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheTaskGroupDomainTaskBound(KHE_TASK_GROUP_DOMAIN tgd)
{
  return tgd->task_bound;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheShowResourceGroupId(KHE_RESOURCE_GROUP rg)                      */
/*                                                                           */
/*  Show the Id of rg, safely.                                               */
/*                                                                           */
/*****************************************************************************/

static char *KheShowResourceGroupId(KHE_RESOURCE_GROUP rg)
{
  char *id;
  if( rg == NULL )
    return "(null)";
  id = KheResourceGroupId(rg);
  if( id != NULL )
    return id;
  else if( KheResourceGroupResourceCount(rg) == 0 )
    return "{}";
  else if( KheResourceGroupResourceCount(rg) == 1 )
    return KheResourceId(KheResourceGroupResource(rg, 0));
  else
    return "?";
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainDebug(KHE_TASK_GROUP_DOMAIN tgd,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tgd onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupDomainDebug(KHE_TASK_GROUP_DOMAIN tgd,
  int verbosity, int indent, FILE *fp)
{
  KHE_TASK_GROUP_DOMAIN tgd2;  char buff[200];
  HnAssert(tgd->magic == TGD_MAGIC, "KheTaskGroupDomainDebug internal error");
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s", KheShowResourceGroupId(tgd->rg));
  for( tgd2 = tgd->prev;  tgd2 != NULL;  tgd2 = tgd2->prev )
    fprintf(fp, " * %s", KheShowResourceGroupId(tgd2->rg));
  /* if( !KheSetEmpty(tgd->dominance_task_domains) ) */
  fprintf(fp, " %s", KheSetShow(tgd->dominance_task_domains, buff));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "dominance testing"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainFinderDominanceClear(                             */
/*    KHE_TASK_GROUP_DOMAIN_FINDER tgdf)                                     */
/*                                                                           */
/*  Clear all previous dominance testing stuff, ready for a fresh round      */
/*  of dominance testing.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupDomainFinderDominanceClear(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf)
{
  KHE_TASK_GROUP_DOMAIN tgd;  int i;
  HaArrayForEach(tgdf->all_task_group_domains, tgd, i)
    KheTaskGroupDomainDominanceClear(tgd);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDomainFinderAddTaskDomain(                              */
/*    KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_RESOURCE_GROUP rg)              */
/*                                                                           */
/*  Add rg to the set of task domains stored in tgdf, unless already present.*/
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupDomainFinderDominanceAddTaskDomain(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE_GROUP rg2;  int i, count;  KHE_TASK_GROUP_DOMAIN tgd;

  /* if rg is already present, do nothing */
  HaArrayForEach(tgdf->all_dominance_task_domains, rg2, i)
    if( KheResourceGroupEqual(rg2, rg) )
    {
      /* ***
      if( DEBUG1 )
	fprintf(stderr, "] KheTaskGroupDomainFinderAddTaskDomain returning"
	  " (already present)\n");
      *** */
      return;
    }

  /* add rg */
  count = HaArrayCount(tgdf->all_dominance_task_domains);
  HaArrayForEach(tgdf->all_task_group_domains, tgd, i)
    KheTaskGroupDomainDominanceAddTaskDomain(tgd, rg, count);
  /* ***
  KheTgdfCacheAddTaskDomain(&tgdf->cache, rg,HaArrayCount(tgdf->t ask_domains));
  *** */
  HaArrayAddLast(tgdf->all_dominance_task_domains, rg);
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheTaskGroupDomainFinderAddTaskDomain(%p) returning"
      " (%s added)\n", (void *) tgdf, KheResourceGroupId(rg));
    if( KheResourceGroupId(rg) == NULL )
      KheResourceGroupDebug(rg, 2, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupDomainDominates(KHE_TASK_GROUP_DOMAIN tgd1,             */
/*    KHE_TASK_GROUP_DOMAIN tgd2)                                            */
/*                                                                           */
/*  Return true if tgd1 dominates tgd2, according to the compatibility       */
/*  set test.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGroupDomainDominanceTest(KHE_TASK_GROUP_DOMAIN tgd1,
  KHE_TASK_GROUP_DOMAIN tgd2, bool *proper)
{
  bool res;
  res = KheSetSubset(tgd2->dominance_task_domains,
    tgd1->dominance_task_domains);
  *proper = res && KheSetCount(tgd1->dominance_task_domains) >
    KheSetCount(tgd2->dominance_task_domains);
  return res;
  /* ***
  return KheSetSubset(tgd2->dominance_task_domains,
    tgd1->dominance_task_domains);
  *** */
}

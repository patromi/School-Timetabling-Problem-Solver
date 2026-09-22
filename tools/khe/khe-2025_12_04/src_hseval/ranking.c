
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
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
/*  FILE:         ranking.c                                                  */
/*  MODULE:       Building and printing tabular summaries.                   */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

#define DEBUG1 0
#define DEBUG2 0

typedef enum {
  RANKING_ENTRY_ABSENT,
  RANKING_ENTRY_DUPLICATE,
  RANKING_ENTRY_EXCLUDED,
  RANKING_ENTRY_ORDINARY,
  RANKING_ENTRY_INVALID,
  RANKING_ENTRY_VARYING,
} RANKING_ENTRY_TYPE;

typedef struct ranking_entry_rec {
  RANKING_ENTRY_TYPE	type;			/* type of entry             */
  long			hard_cost;		/* its cost, or max if none  */
  long			soft_cost;		/* its cost, or max if none  */
  double		rank;			/* its rank (poss. fraction) */
  int			count;			/* no of solns included      */
  /* bool		best_in_row; */		/* true if best in row       */
} *RANKING_ENTRY;

typedef HA_ARRAY(RANKING_ENTRY) ARRAY_RANKING_ENTRY;

typedef struct ranking_row_rec {
  KHE_INSTANCE		instance;		/* the instance              */
  int			pos_in_archive;		/* instance pos in archive   */
  char 			rand_seed[20];		/* the random seed           */
} *RANKING_ROW;

typedef HA_ARRAY(RANKING_ROW) ARRAY_RANKING_ROW;

typedef struct ranking_column_rec {
  KHE_SOLN_GROUP	soln_group;		/* solution group            */
  ARRAY_RANKING_ENTRY	entries;		/* one entry per trial       */
  int			rank_count;		/* number of rank2s          */
  double		rank_total;		/* total rank                */
  double		average_rank;		/* average rank              */
} *RANKING_COLUMN;

typedef HA_ARRAY(RANKING_COLUMN) ARRAY_RANKING_COLUMN;

typedef struct ranking_rec {
  KHE_ARCHIVE		archive;		/* the archive               */
  ARRAY_RANKING_ROW	rows;			/* rows of ranking           */
  ARRAY_RANKING_COLUMN	columns;		/* columns of ranking        */
} *RANKING;


/*****************************************************************************/
/*                                                                           */
/*  RANKING_ENTRY RankingAbsentEntryMake(HA_ARENA a)                         */
/*                                                                           */
/*  Make an `absent' ranking entry.  Do not assign a rank yet.               */
/*                                                                           */
/*****************************************************************************/

static RANKING_ENTRY RankingAbsentEntryMake(HA_ARENA a)
{
  RANKING_ENTRY res;
  HaMake(res, a);
  res->type = RANKING_ENTRY_ABSENT;
  /* res->solution = NULL; */
  res->hard_cost = INT_MAX;
  res->soft_cost = INT_MAX;
  res->rank = 0.0;
  res->count = 0;
  /* res->best_in_row = false; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void RankingEntryAddSoln(RANKING_ENTRY re, KHE_SOLN soln, bool excluded) */
/*                                                                           */
/*  A solution has been found that belongs in re, so add it.  But still      */
/*  do not assign a rank.                                                    */
/*                                                                           */
/*****************************************************************************/

static void RankingEntryAddSoln(RANKING_ENTRY re, KHE_SOLN soln, bool excluded)
{
  switch( re->type )
  {
    case RANKING_ENTRY_ABSENT:

      /* re->solution = soln; */
      if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
      {
	re->hard_cost = INT_MAX;
	re->soft_cost = INT_MAX;
	re->type = RANKING_ENTRY_INVALID;
      }
      else
      {
	re->hard_cost = KheHardCost(KheSolnCost(soln));
	re->soft_cost = KheSoftCost(KheSolnCost(soln));
	re->type = (excluded ? RANKING_ENTRY_EXCLUDED : RANKING_ENTRY_ORDINARY);
      }
      re->count = 1;
      break;

    case RANKING_ENTRY_EXCLUDED:
    case RANKING_ENTRY_DUPLICATE:
    case RANKING_ENTRY_ORDINARY:
    case RANKING_ENTRY_INVALID:
    case RANKING_ENTRY_VARYING:

      re->type = RANKING_ENTRY_DUPLICATE;
      re->count += 1;
      /* re->solution = NULL; */
      re->hard_cost = INT_MAX;
      re->soft_cost = INT_MAX;
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int RankingEntryCmp(const void *p1, const void *p2)                      */
/*                                                                           */
/*  Comparison function for sorting entries to obtain their ranks.           */
/*                                                                           */
/*****************************************************************************/

static int RankingEntryCmp(const void *p1, const void *p2)
{
  RANKING_ENTRY re1 = * (RANKING_ENTRY *) p1;
  RANKING_ENTRY re2 = * (RANKING_ENTRY *) p2;
  if( re1->hard_cost < re2->hard_cost )
    return -1;
  else if( re1->hard_cost > re2->hard_cost )
    return 1;
  else if( re1->soft_cost < re2->soft_cost )
    return -1;
  else if( re1->soft_cost > re2->soft_cost )
    return 1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  RANKING_ROW RankingRowMake(KHE_INSTANCE ins, int pos_in_archive,         */
/*    char *rand_seed, HA_ARENA a)                                           */
/*                                                                           */
/*  Make a ranking row with these attributes.                                */
/*                                                                           */
/*****************************************************************************/

static RANKING_ROW RankingRowMake(KHE_INSTANCE ins, int pos_in_archive,
  char *rand_seed, HA_ARENA a)
{
  RANKING_ROW res;
  HaMake(res, a);
  res->instance = ins;
  res->pos_in_archive = pos_in_archive;
  strcpy(res->rand_seed, rand_seed);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool RankingContainsRow(RANKING ranking, KHE_INSTANCE ins,               */
/*    char *rand_seed, int *pos)                                             */
/*                                                                           */
/*  If ranking contains a ranking row with the given ins and rand_seed, set  */
/*  *pos to its position and return true.  Otherwise return false.           */
/*                                                                           */
/*****************************************************************************/

static bool RankingContainsRow(RANKING ranking, KHE_INSTANCE ins,
  char *rand_seed, int *pos)
{
  int i;  RANKING_ROW row;
  for( i = 0;  i < HaArrayCount(ranking->rows);  i++ )
  {
    row = HaArray(ranking->rows, i);
    if( row->instance == ins && strcmp(row->rand_seed, rand_seed) == 0 )
      return *pos = i, true;
  }
  return *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  RANKING_COLUMN RankingColumnMake(KHE_SOLN_GROUP soln_group, int len,     */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a new ranking column for the given soln_group, with len           */
/*  ranking entries, all NULL initially.                                     */
/*                                                                           */
/*****************************************************************************/

static RANKING_COLUMN RankingColumnMake(KHE_SOLN_GROUP soln_group, int len,
  HA_ARENA a)
{
  RANKING_COLUMN res;  int i;
  HaMake(res, a);
  res->soln_group = soln_group;
  HaArrayInit(res->entries, a);
  for( i = 0;  i < len;  i++ )
    HaArrayAddLast(res->entries, RankingAbsentEntryMake(a));
  res->rank_count = 0;
  res->rank_total = 0.0;
  res->average_rank = 0.0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int RankingColumnCmp(const void *p1, const void *p2)                     */
/*                                                                           */
/*  Comparison function for sorting columns by average rank.                 */
/*                                                                           */
/*****************************************************************************/

static int RankingColumnCmp(const void *p1, const void *p2)
{
  RANKING_COLUMN rc1 = * (RANKING_COLUMN *) p1;
  RANKING_COLUMN rc2 = * (RANKING_COLUMN *) p2;
  if( rc1->average_rank < rc2->average_rank )
    return -1;
  else if( rc1->average_rank > rc2->average_rank )
    return 1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int ArchivePosOfInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)          */
/*                                                                           */
/*  Return the position of ins in archive.                                   */
/*                                                                           */
/*****************************************************************************/

static int ArchivePosOfInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)
{
  int i;
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
    if( KheArchiveInstance(archive, i) == ins )
      return i;
  HnAssert(false, "ArchivePosOfInstance: ins not in archive");
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int RankingRowCmp(const void *p1, const void *p2)                        */
/*                                                                           */
/*  Comparison function for sorting ranking rows.                            */
/*                                                                           */
/*****************************************************************************/

static int RankingRowCmp(const void *p1, const void *p2)
{
  RANKING_ROW rrow1 = * (RANKING_ROW *) p1;
  RANKING_ROW rrow2 = * (RANKING_ROW *) p2;
  if( rrow1->pos_in_archive != rrow2->pos_in_archive )
    return rrow1->pos_in_archive - rrow2->pos_in_archive;
  else
    return strcmp(rrow1->rand_seed, rrow2->rand_seed);
}


/*****************************************************************************/
/*                                                                           */
/*  RANKING RankingMake(KHE_ARCHIVE archive, HA_ARENA a)                     */
/*                                                                           */
/*  Make a ranking object for archive.                                       */
/*                                                                           */
/*****************************************************************************/

static RANKING RankingMake(KHE_ARCHIVE archive, HA_ARENA a)
{
  RANKING res;
  HaMake(res, a);
  res->archive = archive;
  HaArrayInit(res->rows, a);
  HaArrayInit(res->columns, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *RemovePercent(char *str)                                           */
/*                                                                           */
/*  Overwrite % characters in str with spaces.                               */
/*                                                                           */
/*****************************************************************************/

static char *RemovePercent(char *str)
{
  char *p;
  for( p = str;  *p != '\0';  p++ )
    if( *p == '%' )
      *p = ' ';
  return str;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnSetRandSeed(KHE_SOLN soln, char *val)                           */
/*                                                                           */
/*  Copy the randseed string of soln into *val.                              */
/*                                                                           */
/*****************************************************************************/

static void SolnSetRandSeed(KHE_SOLN soln, char *val)
{
  char *str;
  if( KheSolnDescription(soln) == NULL )
    strcpy(val, "none");
  else
  {
    str = strstr(KheSolnDescription(soln), "randseed=");
    if( str == NULL )
      strcpy(val, "none");
    else if( sscanf(str, "randseed=%20[0-9]", val) != 1 )
      strcpy(val, "none");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool SolnExcluded(KHE_SOLN soln)                                         */
/*                                                                           */
/*  Return true if the excluded flag of soln is set.                         */
/*                                                                           */
/*****************************************************************************/

static bool SolnExcluded(KHE_SOLN soln)
{
  return KheSolnDescription(soln) != NULL &&
    (strstr(KheSolnDescription(soln), "excluded") != NULL ||
     strstr(KheSolnDescription(soln), "EXCLUDED") != NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  RANKING RankingMakeFull(KHE_ARCHIVE archive, HA_ARENA a)                 */
/*                                                                           */
/*  Make a ranking object for archive.  This is a full ranking object, i.e.  */
/*  it contains one row for each (instance, randseed).                       */
/*                                                                           */
/*****************************************************************************/

static RANKING RankingMakeFull(KHE_ARCHIVE archive, HA_ARENA a)
{
  int i, j, k, pos;  KHE_INSTANCE ins;
  KHE_SOLN_GROUP soln_group;  KHE_SOLN soln;
  char rand_seed[20];  int rank_count;  double rank_total, rank;

  RANKING ranking;  RANKING_ROW rrow;  RANKING_ENTRY re, e;
  RANKING_COLUMN rcol;  ARRAY_RANKING_ENTRY entries;

  /* make an initially empty ranking object */
  ranking = RankingMake(archive, a);

  /* create and sort rows, one for each distinct (instance, rand_seed) */
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    for( j = 0;  j < KheSolnGroupSolnCount(soln_group);  j++ )
    {
      soln = KheSolnGroupSoln(soln_group, j);
      ins = KheSolnInstance(soln);
      SolnSetRandSeed(soln, rand_seed);
      if( !RankingContainsRow(ranking, ins, rand_seed, &pos) )
      {
	rrow = RankingRowMake(ins, ArchivePosOfInstance(archive,ins),
	  rand_seed, a);
	HaArrayAddLast(ranking->rows, rrow);
      }
    }
  }
  HaArraySort(ranking->rows, &RankingRowCmp);

  /* create columns, one for each solution group */
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    rcol = RankingColumnMake(soln_group, HaArrayCount(ranking->rows), a);
    HaArrayAddLast(ranking->columns, rcol);
    for( j = 0;  j < KheSolnGroupSolnCount(soln_group);  j++ )
    {
      /* get pos, the row index of this soln's (instance, description) */
      soln = KheSolnGroupSoln(soln_group, j);
      SolnSetRandSeed(soln, rand_seed);
      HnAssert(RankingContainsRow(ranking, KheSolnInstance(soln), rand_seed,
	&pos), "HSEval internal error (RankingTableBuild)");

      /* get the entry and add soln to it */
      re = HaArray(rcol->entries, pos);
      RankingEntryAddSoln(re, soln, SolnExcluded(soln));
    }
  }

  /* rank each row of the table and store ranks in the entries */
  HaArrayInit(entries, a);
  HaArrayForEach(ranking->rows, rrow, i)
  {
    /* get the entries of row i that need to be ranked */
    HaArrayClear(entries);
    HaArrayForEach(ranking->columns, rcol, j)
    {
      re = HaArray(rcol->entries, i);
      if( re->type != RANKING_ENTRY_EXCLUDED )
	HaArrayAddLast(entries, re);
    }

    /* sort the entries for row i */
    HaArraySort(entries, &RankingEntryCmp);

    /* assign ranks to the entries for row i */
    for( j = 0;  j < HaArrayCount(entries);  j = k )
    {
      re = HaArray(entries, j);
      rank_count = 1;
      rank_total = j + 1;
      for( k = j + 1;  k < HaArrayCount(entries);  k++ )
      {
	e = HaArray(entries, k);
	if( e->hard_cost != re->hard_cost || e->soft_cost != re->soft_cost )
	  break;
	rank_count++;
	rank_total += k + 1;
      }
      rank = (double) rank_total / rank_count;
      while( j < k )
      {
	re = HaArray(entries, j);
        re->rank = rank;
	j++;
      }
    }
  }

  /* find the total rank of each column, and the number of ranks */
  HaArrayForEach(ranking->columns, rcol, i)
  {
    rcol->rank_count = 0;
    rcol->rank_total = 0.0;
    HaArrayForEach(rcol->entries, re, j)
      if( re->type != RANKING_ENTRY_EXCLUDED )
      {
        rcol->rank_count++;
        rcol->rank_total += re->rank;
      }
    rcol->average_rank = (rcol->rank_count == 0 ? 0.0 :
      rcol->rank_total / rcol->rank_count);
  }

  /* sort the columns by increasing average rank */
  HaArraySort(ranking->columns, &RankingColumnCmp);
  return ranking;
}


/*****************************************************************************/
/*                                                                           */
/*  void RankingMergeEntry(RANKING_ENTRY target, RANKING_ENTRY source)       */
/*                                                                           */
/*  Merge source into target.                                                */
/*                                                                           */
/*****************************************************************************/

static void RankingMergeEntry(RANKING_ENTRY target, RANKING_ENTRY source)
{
  if( target->type == RANKING_ENTRY_ABSENT )
  {
    target->type = source->type;
    target->hard_cost = source->hard_cost;
    target->soft_cost = source->soft_cost;
    target->rank = source->rank;
    target->count = source->count;
  }
  else if( target->type != source->type )
    target->type = RANKING_ENTRY_VARYING;
  else if( target->type == RANKING_ENTRY_ORDINARY )
  {
    if( DEBUG2 )
      fprintf(stderr, "  %ld.%ld += %ld.%ld = %ld.%ld\n",
	target->hard_cost, target->soft_cost,
	source->hard_cost, source->soft_cost,
	target->hard_cost + source->hard_cost,
	target->soft_cost + source->soft_cost);
    target->hard_cost += source->hard_cost;
    target->soft_cost += source->soft_cost;
    target->rank += source->rank;
    target->count += source->count;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  RANKING RankingMergeRows(RANKING ranking, HA_ARENA a)                    */
/*                                                                           */
/*  Return a new ranking object whose rows are merged versions of the        */
/*  old rows.                                                                */
/*                                                                           */
/*****************************************************************************/

static RANKING RankingMergeRows(RANKING ranking, HA_ARENA a)
{
  RANKING res;  RANKING_ROW rrow, prev_rrow, res_rrow;  int i, j, k;
  RANKING_COLUMN rcol, res_rcol;  RANKING_ENTRY rentry;

  /* make an intially empty ranking object */
  res = RankingMake(ranking->archive, a);

  /* create rows, one for each distinct instance in ranking */
  prev_rrow = NULL;
  HaArrayForEach(ranking->rows, rrow, i)
  {
    if( prev_rrow == NULL || prev_rrow->instance != rrow->instance )
    {
      res_rrow = RankingRowMake(rrow->instance, rrow->pos_in_archive, "", a);
      HaArrayAddLast(res->rows, res_rrow);
    }
    prev_rrow = rrow;
  }

  /* create columns, one for each column in ranking */
  HaArrayForEach(ranking->columns, rcol, i)
  {
    res_rcol = RankingColumnMake(rcol->soln_group, HaArrayCount(res->rows), a);
    HaArrayAddLast(res->columns, res_rcol);
    res_rcol->rank_count = rcol->rank_count;
    res_rcol->rank_total = rcol->rank_total;
    res_rcol->average_rank = rcol->average_rank;

    /* one entry for each group of rows */
    prev_rrow = NULL;
    k = 0;
    rentry = NULL;
    HaArrayForEach(ranking->rows, rrow, j)
    {
      if( prev_rrow == NULL || prev_rrow->instance != rrow->instance )
      {
	res_rrow = HaArray(res->rows, k);
	rentry = HaArray(res_rcol->entries, k);
	k++;
      }
      RankingMergeEntry(rentry, HaArray(rcol->entries, j));
      prev_rrow = rrow;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST RankingEntryCost(RANKING_ENTRY re)                              */
/*                                                                           */
/*  Return the average cost of re.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_COST RankingEntryCost(RANKING_ENTRY re)
{
  int hard_cost, soft_cost;
  hard_cost = re->hard_cost / re->count;
  soft_cost = re->soft_cost / re->count;
  return KheCost(hard_cost, soft_cost);
}


/*****************************************************************************/
/*                                                                           */
/*  bool RankingEntryHasCost(RANKING_ENTRY re)                               */
/*                                                                           */
/*  Return true if re has a cost.                                            */
/*                                                                           */
/*****************************************************************************/

bool RankingEntryHasCost(RANKING_ENTRY re)
{
  return re->type==RANKING_ENTRY_EXCLUDED || re->type==RANKING_ENTRY_ORDINARY;
}


/*****************************************************************************/
/*                                                                           */
/*  void RankingSetBestInRow(RANKING ranking)                                */
/*                                                                           */
/*  Set the best in row fields of the entries of ranking.                    */
/*                                                                           */
/*****************************************************************************/

/* not using it now
static void RankingSetBestInRow(RANKING ranking)
{
  KHE_COST cost, best_cost;  RANKING_ROW rrow;
  RANKING_COLUMN rcol;  RANKING_ENTRY re;  int i, j;
  HaArrayForEach(ranking->rows, rrow, i)
  {
    ** find the best cost **
    best_cost = KheCostMax;
    HaArrayForEach(ranking->columns, rcol, j)
    {
      re = HaArray(rcol->entries, i);
      if( RankingEntryHasCost(re) )
      {
	cost = RankingEntryCost(re);
	if( cost < best_cost )
	  best_cost = cost;
      }
    }

    ** mark those entries that have that cost **
    HaArrayForEach(ranking->columns, rcol, j)
    {
      re = HaArray(rcol->entries, i);
      if( RankingEntryHasCost(re) && RankingEntryCost(re) <= best_cost )
	re->best_in_row = true;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  TABLE RankingTableBuild(KHE_ARCHIVE archive, bool full, bool costs,      */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Build an evaluation table from archive.                                  */
/*                                                                           */
/*  If full is true, make one line for each (instance, randseed).  Else      */
/*  make one line for each instance, containing averages of those lines.     */
/*                                                                           */
/*  If costs is true, include costs as well as ranks.                        */
/*                                                                           */
/*****************************************************************************/

static TABLE RankingTableBuild(KHE_ARCHIVE archive, bool full, bool costs,
  HA_ARENA a)
{
  TABLE res;  TABLE_ROW row;  int i, j;  TABLE_ENTRY entry;
  KHE_SOLN_GROUP soln_group;  char *str;

  RANKING ranking;  RANKING_ROW rrow;  RANKING_ENTRY re;
  RANKING_COLUMN rcol;

  /* make a full ranking object, then optionally convert it to a summary */
  ranking = RankingMakeFull(archive, a);
  if( !full )
    ranking = RankingMergeRows(ranking, a);
  /* RankingSetBestInRow(ranking); */

  /* table and header */
  if( costs )
    res = TableMake(full ? "Full list of all experiments" :
      "Experiments grouped by instance and averaged", a);
  else
    res = TableMake(full ? "Full list of all experiments (showing ranks only)" :
      "Experiments grouped by instance and averaged (showing ranks only)", a);

  /* first header row */
  row = TableRowMake(a);
  TableAddRow(res, row);
  if( full )
  {
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Test case", a);
    TableEntrySetHSpan(entry, 2);
    TableRowAddEntry(row, entry);
  }
  else
    TableRowAddEntry(row,
      TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Instance", a));
  HaArrayForEach(ranking->columns, rcol, i)
  {
    soln_group = rcol->soln_group;
    str = RemovePercent(KheSolnGroupId(soln_group));
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, str, a);
    if( costs )
      TableEntrySetHSpan(entry, 2);
    TableRowAddEntry(row, entry);
  }

  /* second header row */
  if( full || costs )
  {
    row = TableRowMake(a);
    TableAddRow(res, row);
    TableRowAddEntry(row, TableEntryMakeText(TABLE_LEFT,
      TABLE_BOLD, full ? "Instance" : "", a));
    if( full )
      TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	TABLE_BOLD, "RandSeed", a));
    HaArrayForEach(ranking->columns, rcol, i)
    {
      if( costs )
	TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	  TABLE_BOLD, full ? "Cost" : "Average Cost", a));
      TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	TABLE_BOLD, full ? "Rank" : "Average Rank", a));
    }
  }

  /* ranking rows */
  HaArrayForEach(ranking->rows, rrow, i)
  {
    /* header columns */
    row = TableRowMake(a);
    TableAddRow(res, row);
    TableRowAddEntry(row, TableEntryMakeText(TABLE_LEFT, TABLE_BOLD,
      KheInstanceId(rrow->instance), a));
    if( full )
      TableRowAddEntry(row, TableEntryMakeText(TABLE_RIGHT,
	  TABLE_BOLD, rrow->rand_seed, a));

    /* solution group columns */
    HaArrayForEach(ranking->columns, rcol, j)
    {
      re = HaArray(rcol->entries, i);
      switch( re->type )
      {
	case RANKING_ENTRY_ABSENT:

	  if( costs )
	  {
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "ABSENT", a));
	    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	      TABLE_ROMAN, "%.1f", (double) re->rank, a));
	  }
	  else
	  {
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "ABSENT", a));
	  }
	  break;

	case RANKING_ENTRY_DUPLICATE:

	  if( costs )
	  {
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "DUPLICATE", a));
	    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	      TABLE_ROMAN, "%.1f", (double) re->rank / re->count, a));
	  }
	  else
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "DUPLICATE", a));
	  break;

	case RANKING_ENTRY_EXCLUDED:

	  if( costs )
	  {
	    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	      /* re->best_in_row ? TABLE_BOLD : */ TABLE_ROMAN,
	      "%.5f", KheCostShow(RankingEntryCost(re)), a));
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "EXCLUDED", a));
	  }
	  else
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "EXCLUDED", a));
	  break;

	case RANKING_ENTRY_ORDINARY:

	  if( costs )
	    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	      /* re->best_in_row ? TABLE_BOLD : */ TABLE_ROMAN,
	      "%.5f", KheCostShow(RankingEntryCost(re)), a));
          TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	    /* re->best_in_row ? TABLE_BOLD : */ TABLE_ROMAN,
	    "%.1f", (double) re->rank / re->count, a));
	  break;

	case RANKING_ENTRY_INVALID:

	  if( costs )
	  {
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "INVALID", a));
	    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	      TABLE_ROMAN, "%.1f", (double) re->rank / re->count, a));
	  }
	  else
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "INVALID", a));
	  break;

	case RANKING_ENTRY_VARYING:

	  if( costs )
	  {
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "VARYING", a));
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_ROMAN, "", a));
	  }
	  else
	    TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE,
	      TABLE_BOLD, "VARYING", a));
	  break;

      }
    }
  }

  /* first footer row - sum of ranks */
  if( full )
  {
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Sum of ranks", a);
    if( full )
      TableEntrySetHSpan(entry, 2);
    TableRowAddEntry(row, entry);
    HaArrayForEach(ranking->columns, rcol, j)
    {
      if( costs )
	TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN,"", a));
      TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	TABLE_ROMAN, "%.1f", rcol->rank_total, a));
    }

    /* second footer row - number of ranks */
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Number of ranks", a);
    if( full )
      TableEntrySetHSpan(entry, 2);
    TableRowAddEntry(row, entry);
    HaArrayForEach(ranking->columns, rcol, j)
    {
      if( costs )
	TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN,"", a));
      TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
	TABLE_ROMAN, "%.0f", rcol->rank_count, a));
    }
  }

  /* third footer row - average rank */
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Average rank", a);
  if( full )
    TableEntrySetHSpan(entry, 2);
  TableRowAddEntry(row, entry);
  HaArrayForEach(ranking->columns, rcol, j)
  {
    if( costs )
      TableRowAddEntry(row, TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN, "", a));
    TableRowAddEntry(row, TableEntryMakeDouble(TABLE_CENTRE,
      TABLE_BOLD, rcol->rank_count == 0 ? "-" : "%.2f", rcol->average_rank, a));
  }

  /* caption */
  if( full )
  {
    if( costs )
    {
      TableAddCaptionLine(res, "In this table, each row represents one test");
      TableAddCaptionLine(res, "case, each double column represents one");
      TableAddCaptionLine(res, "solution group, and each entry represents the");
      TableAddCaptionLine(res, "result of the corresponding experiment.");
      TableAddCaptionLine(res, "The last three rows contain, for each");
      TableAddCaptionLine(res, "solution group, the sum of the ranks of its");
      TableAddCaptionLine(res, "non-excluded results, the number of those");
      TableAddCaptionLine(res, "results, and the quotient of those");
      TableAddCaptionLine(res, "quantities.  The solution groups appear");
      TableAddCaptionLine(res, "from left to right in order of increasing");
      TableAddCaptionLine(res, "value of this quotient.");
    }
    else
    {
      TableAddCaptionLine(res, "In this table, each row represents one test");
      TableAddCaptionLine(res, "case, each column represents one");
      TableAddCaptionLine(res, "solution group, and each entry represents the");
      TableAddCaptionLine(res, "result of the corresponding experiment.");
      TableAddCaptionLine(res, "The last three rows contain, for each");
      TableAddCaptionLine(res, "solution group, the sum of the ranks of its");
      TableAddCaptionLine(res, "non-excluded results, the number of those");
      TableAddCaptionLine(res, "results, and the quotient of those");
      TableAddCaptionLine(res, "quantities.  The solution groups appear");
      TableAddCaptionLine(res, "from left to right in order of increasing");
      TableAddCaptionLine(res, "value of this quotient.");
    }
  }
  else
  {
    if( costs )
    {
      TableAddCaptionLine(res, "In this table, each row represents one");
      TableAddCaptionLine(res, "instance, each double column represents one");
      TableAddCaptionLine(res, "solution group, and each entry contains an");
      TableAddCaptionLine(res, "average cost or rank over all random seeds of");
      TableAddCaptionLine(res, "the results for that instance, random seed,");
      TableAddCaptionLine(res, "and solution group.  VARYING indicates that");
      TableAddCaptionLine(res, "these results have varying classifications.");
      TableAddCaptionLine(res, "The last row contains, for each solution");
      TableAddCaptionLine(res, "group, its average rank.");
    }
    else
    {
      TableAddCaptionLine(res, "In this table, each row represents one");
      TableAddCaptionLine(res, "instance, each column represents one");
      TableAddCaptionLine(res, "solution group, and each entry contains the");
      TableAddCaptionLine(res, "average over all random seeds of");
      TableAddCaptionLine(res, "the rank for that instance, random seed,");
      TableAddCaptionLine(res, "and solution group.");
      TableAddCaptionLine(res, "The last row contains, for each solution");
      TableAddCaptionLine(res, "group, its average rank.");
    }
  }

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void RankingEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format)    */
/*                                                                           */
/*  Build and print a table of type table_type, summarising the results in   */
/*  archive, which has been evaluated.                                       */
/*                                                                           */
/*****************************************************************************/

void RankingEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format,
  HA_ARENA_SET as)
{
  HTML html;  TABLE ranking_table;  HA_ARENA a;
  /* KHE_ARCHIVE_METADATA md; */  char buff[100];
  a = HaArenaMake(as);
  switch( print_format )
  {
    case PRINT_HTML:

      /* page header */
      snprintf(buff, 100, "%s Rankings",
        KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 60 ?
        KheArchiveId(archive) : "HSEval");
      html = PageBegin(buff);
      HTMLBigHeading(html, buff);

      /* ***
      HTMLParagraphBegin(html);
      HTMLColouredBoxBeginFullWidth(html, LightRed);
      HTMLText(html, "The code for this page is all written but it has");
      HTMLText(html, "been only lightly tested so far.  Use with caution.");
      HTMLText(html, "Please report problems to Jeff Kingston as usual.");
      HTMLColouredBoxEnd(html);
      HTMLParagraphEnd(html);
      *** */

      /* metadata */
      HTMLParagraphBegin(html);
      HTMLText(html, KheArchiveMetaDataText(archive));
      HTMLParagraphEnd(html);

      /* ranking tables */
      if( KheArchiveInstanceCount(archive) == 0 )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "This archive contains no instances.");
	HTMLParagraphEnd(html);
      }
      else if( KheArchiveSolnGroupCount(archive) == 0 )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "This archive contains no solution groups.");
	HTMLParagraphEnd(html);
      }
      else
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "The solutions of the archive, with the solution");
	HTMLText(html, "groups sorted by increasing average rank, as");
	HTMLText(html, "defined by the rules of the Third International");
	HTMLText(html, "Timetabling Competition.");
	HTMLParagraphEnd(html);

	HTMLParagraphBegin(html);
	HTMLText(html, "A test case is one instance plus one random");
	HTMLText(html, "seed.  A solution group is a set of solutions");
	HTMLText(html, "produced by one solver.  An experiment is a");
	HTMLText(html, "test of one solver on one test case, and");
	HTMLText(html, "consists of a test case, a solution group,");
	HTMLText(html, "and a result, which is a set of zero or more");
	HTMLText(html, "solutions, the ones for the test case stored");
	HTMLText(html, "in the solution group.");
	HTMLParagraphEnd(html);

	HTMLParagraphBegin(html);
	HTMLText(html, "Each result is classified as follows.  It is");
	HTMLText(html, "<i>absent</i> if it contains no solutions; it");
	HTMLText(html, "is <i>invalid</i> if it contains exactly one");
	HTMLText(html, "solution, which is invalid; it is <i>excluded</i>");
	HTMLText(html, "if it contains exactly one solution, which is not");
	HTMLText(html, "invalid, but is required to be excluded; it is");
	HTMLText(html, "<i>duplicate</i> if it contains two or more");
	HTMLText(html, "solutions; and it is <i>ordinary</i> otherwise.");
	HTMLParagraphEnd(html);

	HTMLParagraphBegin(html);
	HTMLText(html, "Each excluded and ordinary result has a cost, the");
	HTMLText(html, "cost of its sole solution.  Each non-excluded");
	HTMLText(html, "result has a rank, which is the rank of its cost");
	HTMLText(html, "with respect to all results for the same test case,");
	HTMLText(html, "assigning fractional ranks in the case of ties,");
	HTMLText(html, "treating absent, invalid, and duplicate results");
	HTMLText(html, "as having very large equal costs, and ignoring");
	HTMLText(html, "excluded results.");
	HTMLParagraphEnd(html);

	ranking_table = RankingTableBuild(archive, false, false, a);
	TablePrintHTML(ranking_table, html);

	ranking_table = RankingTableBuild(archive, true, true, a);
	TablePrintHTML(ranking_table, html);
      }

      /* invalid solutions */
      ArchiveReportInvalidSolns(archive, html);

      /* page footer */
      HTMLParagraphBegin(html);
      HTMLText(html, "Return to the ");
      HTMLJumpFront(html);
      HTMLText(html, ".");
      HTMLParagraphEnd(html);
      PageEnd(html);
      break;

    case PRINT_LATEX:

      /* page header */
      fprintf(stdout, "Content-Type: text/latex\n");
      fprintf(stdout,
	"Content-Disposition: attachment; filename=hseval.tex\n\n");

      /* ranking tables */
      if( KheArchiveInstanceCount(archive) == 0 )
	printf("%%%s\n", "This archive contains no instances.");
      else if( KheArchiveSolnGroupCount(archive) == 0 )
	printf("%%%s\n", "This archive contains no solution groups.");
      else
      {
	ranking_table = RankingTableBuild(archive, false, false, a);
	TablePrintLaTeX(ranking_table, stdout);

	ranking_table = RankingTableBuild(archive, true, true, a);
	TablePrintLaTeX(ranking_table, stdout);
      }
      break;

    default:

      HnAssert(false, "hseval internal error (RankingEvalPrint)");
  }
  HaArenaDelete(a);
}

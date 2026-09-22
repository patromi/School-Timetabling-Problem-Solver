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
/*  FILE:         khe_sr_grouped_tasks_display.c                             */
/*  DESCRIPTION:  Grouped tasks display                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define KHE_COL_WIDTH 14
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GTD_GROUP - one task group                                      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

typedef struct khe_gtd_group_rec {
  ARRAY_KHE_TASK		tasks;
  KHE_INTERVAL			interval;
  /* KHE_TASK_GROUP_DOMAIN	domain; */
  KHE_RESOURCE			history_resource;
  int				history_durn;
  bool				optional;
  int				primary_durn;
  int				index_in_soln;
} *KHE_GTD_GROUP;

typedef HA_ARRAY(KHE_GTD_GROUP) ARRAY_KHE_GTD_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GTD_ENTRY - one entry of a grouped tasks display                */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_RULE_NONE,
  KHE_RULE_SMALL,
  KHE_RULE_FULL,
} KHE_RULE;

typedef struct khe_gtd_entry_rec {
  KHE_GTD_GROUP			group;
  bool				history;
  KHE_TASK			task;		/* the exact KHE task        */
  KHE_RULE			rule;		/* rule above this entry     */
  char				durn_char;	/* over or under durn        */
  int				index_in_soln;
  KHE_RESOURCE			history_resource;
  int				history_durn;
} *KHE_GTD_ENTRY;

typedef HA_ARRAY(KHE_GTD_ENTRY) ARRAY_KHE_GTD_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GTD_ROW - one row of a grouped tasks display                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_gtd_row_rec {
  bool				history;
  KHE_TIME_GROUP		time_group;	/* the time group of the row */
  ARRAY_KHE_GTD_ENTRY		entries;	/* the entries of the row    */
  int				non_empty_count;	/* non-empty entries */
  int				starting_count;	/* non-empty entries */
  int				ending_count;	/* non-empty entries */
} *KHE_GTD_ROW;

typedef HA_ARRAY(KHE_GTD_ROW) ARRAY_KHE_GTD_ROW;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GROUPED_TASKS_DISPLAY - a grouped tasks display                 */
/*                                                                           */
/*****************************************************************************/

struct khe_grouped_tasks_display_rec {

  /* free lists */
  ARRAY_KHE_GTD_ENTRY		entry_free_list;
  ARRAY_KHE_GTD_ROW		row_free_list;

  /* constant values */
  HA_ARENA			arena;
  KHE_SOLN			soln;
  /* KHE_TASK_GROUP_DOMAIN_FINDER domain_finder; sharing this now */
  char				*id;
  int				min_limit;
  int				max_limit;
  KHE_COST			cost;
  KHE_FRAME			days_frame;

  /* fields that change as groups are added */
  KHE_GTD_GROUP			curr_group;
  int				undersized_durn; /* summary value            */
  int				oversized_durn;  /* summary value            */
  ARRAY_KHE_GTD_GROUP		groups;		 /* groups to be displayed   */

  /* the display */
  bool				rows_up_to_date; /* up to date with groups   */
  ARRAY_KHE_GTD_ROW		rows;		 /* rows of the display      */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GROUP_SIGNATURE                                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_group_signature_rec {
  int				length;
  KHE_RESOURCE_GROUP		domain;
} *KHE_GROUP_SIGNATURE;

typedef HA_ARRAY(KHE_GROUP_SIGNATURE) ARRAY_KHE_GROUP_SIGNATURE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GROUPED_TASKS_SIGNATURE                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_grouped_tasks_signature_rec {
  ARRAY_KHE_GROUP_SIGNATURE	group_signatures;
} *KHE_GROUPED_TASKS_SIGNATURE;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GTD_GROUP"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GTD_GROUP KheGtdGroupMake(bool optional, int primary_durn,           */
/*    int index_in_soln, HA_ARENA a)                                         */
/*                                                                           */
/*  Make a new, initially empty gtd group.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_GTD_GROUP KheGtdGroupMake(bool optional, int primary_durn,
  int index_in_soln, HA_ARENA a)
{
  KHE_GTD_GROUP res;
  HaMake(res, a);
  HaArrayInit(res->tasks, a);
  res->interval = KheIntervalMake(1, 0);
  /* res->domain = NULL; */
  res->history_resource = NULL;
  res->history_durn = 0;
  res->optional = optional;
  res->primary_durn = primary_durn;
  res->index_in_soln = index_in_soln;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdGroupAddTask(KHE_GTD_GROUP gg, KHE_TASK task,                 */
/*    KHE_GROUPED_TASKS_DISPLAY gtd)                                         */
/*                                                                           */
/*  Add task to gg.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheGtdGroupAddTask(KHE_GTD_GROUP gg, KHE_TASK task,
  KHE_GROUPED_TASKS_DISPLAY gtd)
{
  KHE_INTERVAL in;  /* KHE_RESOURCE_GROUP rg; */
  HaArrayAddLast(gg->tasks, task);
  in = KheTaskInterval(task, gtd->days_frame);
  gg->interval = KheIntervalUnion(gg->interval, in);
  /* ***
  rg = KheTaskDomain(task);
  gg->domain = KheTaskGroupDomainMake(gtd->domain_finder, gg->domain, rg);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdGroupAddHistory(KHE_GTD_GROUP gg, KHE_RESOURCE r, int durn)   */
/*                                                                           */
/*  Add history to gg.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheGtdGroupAddHistory(KHE_GTD_GROUP gg, KHE_RESOURCE r, int durn)
{
  HnAssert(gg->history_resource == NULL,
    "KheGroupedTasksDisplayGroupAddHistory internal error "
    "(called twice on the same group)");
  gg->history_resource = r;
  gg->history_durn = durn;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGtdGroupAtEnd(KHE_GTD_GROUP gg, KHE_GROUPED_TASKS_DISPLAY gtd)   */
/*                                                                           */
/*  Return true if gtd's current group lies at the end of the days frame.    */
/*                                                                           */
/*****************************************************************************/

static bool KheGtdGroupAtEnd(KHE_GTD_GROUP gg, KHE_GROUPED_TASKS_DISPLAY gtd)
{
  return KheIntervalLast(gg->interval) >=
    KheFrameTimeGroupCount(gtd->days_frame) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  char KheGtdGroupDurnChar(KHE_GTD_GROUP gg, KHE_GROUPED_TASKS_DISPLAY gtd)*/
/*                                                                           */
/*  Find the duration character for gg.  Also update the undersized_durn     */
/*  and oversized_durn fields of gtd.                                        */
/*                                                                           */
/*****************************************************************************/

static char KheGtdGroupDurnChar(KHE_GTD_GROUP gg, KHE_GROUPED_TASKS_DISPLAY gtd)
{
  /* ***
  int durn;
  durn = KheIntervalLength(gg->interval) + gg->history_durn;
  *** */
  if( gg->optional )
  {
    return '*';
  }
  else if( gg->primary_durn < gtd->min_limit && !KheGtdGroupAtEnd(gg, gtd) )
  {
    gtd->undersized_durn += (gtd->min_limit - gg->primary_durn);
    return '<';
  }
  else if( gg->primary_durn > gtd->max_limit )
  {
    gtd->oversized_durn += (gg->primary_durn - gtd->min_limit);
    return '>';
  }
  else
    return ' ';
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsRunningDuringTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)   */
/*                                                                           */
/*  Return true if meet is running during tg.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetIsRunningDuringTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)
{
  KHE_TIME start_time;  int durn, i, pos;
  start_time = KheMeetAsstTime(meet);
  if( start_time != NULL )
  {
    durn = KheMeetDuration(meet);
    for( i = 0;  i < durn;  i++ )
      if( KheTimeGroupContains(tg, KheTimeNeighbour(start_time, i), &pos) )
	return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheDoTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)      */
/*                                                                           */
/*  Return a descendant of task running during tg, or NULL if none.          */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheDoTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TASK res, child_task;  int i;

  /* do it for task */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetIsRunningDuringTimeGroup(meet, tg) )
    return task;

  /* do it for task's proper descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    res = KheDoTaskDuringTimeGroup(child_task, tg);
    if( res != NULL )
      return res;
  }

  /* no luck, return NULL */
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheGtdGroupTaskDuringTimeGroup(KHE_GTD_GROUP gg,                */
/*    KHE_TIME_GROUP tg, KHE_TASK *proper_root_task)                         */
/*                                                                           */
/*  Return a task from gg which is running during tg, or NULL if none.  The  */
/*  result is the specific task, and *proper_root_task is its proper root.   */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheGtdGroupTaskDuringTimeGroup(KHE_GTD_GROUP gg,
  KHE_TIME_GROUP tg, KHE_TASK *proper_root_task)
{
  KHE_TASK task, res;  int i;
  HaArrayForEach(gg->tasks, task, i)
  {
    res = KheDoTaskDuringTimeGroup(task, tg);
    if( res != NULL )
      return *proper_root_task = task, res;
  }
  return *proper_root_task = NULL, NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGtdGroupFirstTaskDomainSize(KHE_GTD_GROUP gg)                     */
/*                                                                           */
/*  Return the size of the domain of the first task of gg, or 0 if none.     */
/*                                                                           */
/*****************************************************************************/

static int KheGtdGroupFirstTaskDomainSize(KHE_GTD_GROUP gg)
{
  KHE_TASK task;
  if( HaArrayCount(gg->tasks) == 0 )
    return 0;
  task = HaArrayFirst(gg->tasks);
  return KheResourceGroupResourceCount(KheTaskDomain(task));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGtdGroupTypedCmp(KHE_GTD_GROUP gg1, KHE_GTD_GROUP gg2)            */
/*                                                                           */
/*  Typed comparison function for sorting an array of gtd groups, first      */
/*  by first index, then by domain.                                          */
/*                                                                           */
/*****************************************************************************/

int KheGtdGroupTypedCmp(KHE_GTD_GROUP gg1, KHE_GTD_GROUP gg2)
{
  int first1, first2, rg_count1, rg_count2, cmp;

  /* groups with history come first, in resource index order */
  first1 = gg1->history_durn > 0 ? -1 : 0;
  first2 = gg2->history_durn > 0 ? -1 : 0;
  cmp = first1 - first2;
  if( cmp != 0 )
    return cmp;
  if( gg1->history_durn > 0 )
  {
    cmp = KheResourceInstanceIndex(gg1->history_resource) -
      KheResourceInstanceIndex(gg2->history_resource);
    if( cmp != 0 )
      return cmp;
  }

  /* otherwise, groups which start on earlier days come first */
  cmp = gg1->interval.first - gg2->interval.first;
  if( cmp != 0 )
    return cmp;

  /* otherwise, groups which end on earlier days come first */
  /* ***
  cmp = gg1->interval.last - gg2->interval.last;
  if( cmp != 0 )
    return cmp;
  *** */

  /* otherwise, groups whose first tasks have smaller domains come first */
  rg_count1 = KheGtdGroupFirstTaskDomainSize(gg1);
  rg_count2 = KheGtdGroupFirstTaskDomainSize(gg2);
  return rg_count1 - rg_count2;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGtdGroupCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Untyped comparison function for sorting an array of gtd groups as above. */
/*                                                                           */
/*****************************************************************************/

static int KheGtdGroupCmp(const void *t1, const void *t2)
{
  KHE_GTD_GROUP gg1 = * (KHE_GTD_GROUP *) t1;
  KHE_GTD_GROUP gg2 = * (KHE_GTD_GROUP *) t2;
  return KheGtdGroupTypedCmp(gg1, gg2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GTD_ENTRY"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GTD_ENTRY KheGtdEntryMake(KHE_GROUPED_TASKS_DISPLAY gtd)             */
/*                                                                           */
/*  Make an empty gtd entry.                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_GTD_ENTRY KheGtdEntryMake(KHE_GROUPED_TASKS_DISPLAY gtd)
{
  KHE_GTD_ENTRY res;
  if( HaArrayCount(gtd->entry_free_list) > 0 )
    res = HaArrayLastAndDelete(gtd->entry_free_list);
  else
    HaMake(res, gtd->arena);
  res->group = NULL;
  res->history = false;
  res->task = NULL;
  res->rule = KHE_RULE_FULL;
  res->durn_char = ' ';
  res->index_in_soln = -1;
  res->history_resource = NULL;
  res->history_durn = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdEntryFree(KHE_GTD_ENTRY gtde, KHE_GROUPED_TASKS_DISPLAY gtd)  */
/*                                                                           */
/*  Free gtde.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheGtdEntryFree(KHE_GTD_ENTRY gtde, KHE_GROUPED_TASKS_DISPLAY gtd)
{
  HaArrayAddLast(gtd->entry_free_list, gtde);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdEntrySet(KHE_GTD_ENTRY gtde, KHE_GTD_GROUP group,             */
/*    KHE_TASK task, KHE_RULE rule, char durn_char, int index_in_soln)       */
/*                                                                           */
/*  Set the attributes of gtde to these values.                              */
/*                                                                           */
/*****************************************************************************/

static void KheGtdEntrySet(KHE_GTD_ENTRY gtde, KHE_GTD_GROUP group,
  KHE_TASK task, KHE_RULE rule, char durn_char, int index_in_soln)
{
  HnAssert(task != NULL, "KheGtdEntrySet internal error 2");
  gtde->group = group;
  gtde->task = task;
  gtde->rule = rule;
  gtde->durn_char = durn_char;
  gtde->index_in_soln = index_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdEntrySetHistory(KHE_GTD_ENTRY gtde, KHE_GTD_GROUP group,      */
/*    KHE_RESOURCE r, int durn, char durn_char)                              */
/*                                                                           */
/*  Set gtde to be a history entry with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

static void KheGtdEntrySetHistory(KHE_GTD_ENTRY gtde, KHE_GTD_GROUP group,
  KHE_RESOURCE r, int durn, char durn_char)
{
  HnAssert(r != NULL, "KheGtdEntrySetHistory internal error 2");
  gtde->group = group;
  gtde->history = true;
  gtde->history_resource = r;
  gtde->history_durn = durn;
  gtde->rule = KHE_RULE_FULL;
  gtde->durn_char = durn_char;
  gtde->index_in_soln = -1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGtdEntryIsEmpty(KHE_GTD_ENTRY gtde)                              */
/*                                                                           */
/*  Return true if gtde is empty.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheGtdEntryIsEmpty(KHE_GTD_ENTRY gtde)
{
  return gtde->task == NULL && gtde->history_resource == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheGtdEntryShowFirstLine(KHE_GTD_ENTRY gtde,                       */
/*    char buff[KHE_COL_WIDTH + 1])                                          */
/*                                                                           */
/*  Show gtde's first line in buff and return buff.                          */
/*                                                                           */
/*****************************************************************************/

static char *KheGtdEntryShowFirstLine(KHE_GTD_ENTRY gtde,
  char buff[KHE_COL_WIDTH + 1])
{
  char *id, *id2;
  if( gtde->history )
    snprintf(buff, KHE_COL_WIDTH, "%s %d%c",
      KheResourceId(gtde->history_resource), gtde->history_durn,
      gtde->durn_char);
  else if( gtde->task == NULL )
    snprintf(buff, KHE_COL_WIDTH, "%s", "");
  else
  {
    id = KheTaskId(gtde->task);
    id2 = strstr(id, ":");
    id = (id2 != NULL ? id2 + 1 : id);
    if( gtde->index_in_soln == -1 )
      snprintf(buff, KHE_COL_WIDTH, "%s%c", id, gtde->durn_char);
    else
      snprintf(buff, KHE_COL_WIDTH, "%d: %s%c",
	gtde->index_in_soln, id, gtde->durn_char);
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheGtdEntryShowSecondLine(KHE_GTD_ENTRY gtde,                      */
/*    char buff[KHE_COL_WIDTH + 1])                                          */
/*                                                                           */
/*  Show gtde's second line in buff and return buff.                         */
/*                                                                           */
/*****************************************************************************/

static char *KheGtdEntryShowSecondLine(KHE_GTD_ENTRY gtde,
  bool show_asst, char buff[KHE_COL_WIDTH + 1])
{
  KHE_COST non_asst_cost, asst_cost;  KHE_RESOURCE_GROUP domain;
  if( gtde->history || gtde->task == NULL )
    snprintf(buff, KHE_COL_WIDTH, "%s", "");
  else if( show_asst && KheTaskAsstResource(gtde->task) != NULL )
  {
    /* print the assigned resource */
    snprintf(buff, KHE_COL_WIDTH, "%s%c",
      KheResourceId(KheTaskAsstResource(gtde->task)), gtde->durn_char);
  }
  else
  {
    domain = KheTaskDomain(gtde->task);
    KheTaskNonAsstAndAsstCost(gtde->task, &non_asst_cost, &asst_cost);
    if( non_asst_cost <= asst_cost )
    {
      /* print domain id (or just a count) and soft cost */
      if( KheResourceGroupId(domain) == NULL )
	snprintf(buff, KHE_COL_WIDTH, "(%d)@%d%c",
	  KheResourceGroupResourceCount(domain), KheSoftCost(asst_cost),
	  gtde->durn_char);
      else
	snprintf(buff, KHE_COL_WIDTH, "%s@%d%c", KheResourceGroupId(domain),
	  KheSoftCost(asst_cost), gtde->durn_char);
    }
    else if( non_asst_cost >= KheCost(1, 0) )
    {
      /* print domain and "H" for hard cost */
      snprintf(buff, KHE_COL_WIDTH, "%s/H%c", KheResourceGroupId(domain),
	gtde->durn_char);
    }
    else
    {
      /* print domain and soft cost */
      snprintf(buff, KHE_COL_WIDTH, "%s/%d%c", KheResourceGroupId(domain),
	KheSoftCost(non_asst_cost), gtde->durn_char);
    }
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GTD_ROW"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GTD_ROW KheGtdRowMake(KHE_TIME_GROUP tg,                             */
/*    KHE_GROUPED_TASKS_DISPLAY gtd)                                         */
/*                                                                           */
/*  Make a gtd row with these attributes.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_GTD_ROW KheGtdRowMake(KHE_TIME_GROUP tg,
  KHE_GROUPED_TASKS_DISPLAY gtd)
{
  KHE_GTD_ROW res;
  if( HaArrayCount(gtd->row_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(gtd->row_free_list);
    HaArrayClear(res->entries);
  }
  else
  {
    HaMake(res, gtd->arena);
    HaArrayInit(res->entries, gtd->arena);
  }
  res->history = (tg == NULL);
  res->time_group = tg;
  res->non_empty_count = 0;
  res->starting_count = 0;
  res->ending_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdRowFree(KHE_GTD_ROW gtdr, KHE_GROUPED_TASKS_DISPLAY gtd)      */
/*                                                                           */
/*  Free gtdr, including freeing its entries.                                */
/*                                                                           */
/*****************************************************************************/

static void KheGtdRowFree(KHE_GTD_ROW gtdr, KHE_GROUPED_TASKS_DISPLAY gtd)
{
  KHE_GTD_ENTRY gtde;  int i;
  HaArrayForEach(gtdr->entries, gtde, i)
    KheGtdEntryFree(gtde, gtd);
  HaArrayAddLast(gtd->row_free_list, gtdr);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGtdRowIsEmpty(KHE_GTD_ROW gtdr)                                  */
/*                                                                           */
/*  Return true if gtdr is empty.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheGtdRowIsEmpty(KHE_GTD_ROW gtdr)
{
  KHE_GTD_ENTRY gtde;  int i;
  HaArrayForEach(gtdr->entries, gtde, i)
    if( !KheGtdEntryIsEmpty(gtde) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "building the display"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayAddEmptyColumn(KHE_GROUPED_TASKS_DISPLAY gtd) */
/*                                                                           */
/*  Add an empty column to the end of gtd.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheGroupedTasksDisplayAddEmptyColumn(KHE_GROUPED_TASKS_DISPLAY gtd)
{
  KHE_GTD_ROW gtdr;  int i;  KHE_GTD_ENTRY gtde;
  HaArrayForEach(gtd->rows, gtdr, i)
  {
    gtde = KheGtdEntryMake(gtd);
    HaArrayAddLast(gtdr->entries, gtde);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupedTasksDisplayColumnAvail(KHE_GROUPED_TASKS_DISPLAY gtd,    */
/*    int col_index)                                                         */
/*                                                                           */
/*  If the column with index col_index is available for the tasks of group   */
/*  gg, then return true, else return false.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupedTasksDisplayColumnAvail(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_GTD_GROUP gg, int col_index)
{
  int i;  KHE_GTD_ROW gtdr;  KHE_GTD_ENTRY gtde;
  if( gg->history_resource != NULL )
  {
    gtdr = HaArray(gtd->rows, 0);
    gtde = HaArray(gtdr->entries, col_index);
    if( !KheGtdEntryIsEmpty(gtde) )
      return false;
  }
  for( i = gg->interval.first;  i <= gg->interval.last;  i++ )
  {
    gtdr = HaArray(gtd->rows, i + 1);
    gtde = HaArray(gtdr->entries, col_index);
    if( !KheGtdEntryIsEmpty(gtde) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupedTasksDisplayFindColumn(KHE_GROUPED_TASKS_DISPLAY gtd)      */
/*                                                                           */
/*  Find or make a column able to hold group gg.                             */
/*                                                                           */
/*****************************************************************************/

static int KheGroupedTasksDisplayFindColumn(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_GTD_GROUP gg)
{
  KHE_GTD_ROW gtdr;  int col_count, i;

  /* find out the current number of columns */
  gtdr = HaArrayFirst(gtd->rows);
  col_count = HaArrayCount(gtdr->entries);

  /* if an existing column will do the job, return its index */
  for( i = 0;  i < col_count;  i++ )
    if( KheGroupedTasksDisplayColumnAvail(gtd, gg, i) )
      return i;

  /* otherwise add an empty column and return its index */
  KheGroupedTasksDisplayAddEmptyColumn(gtd);
  return col_count;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetRunsBeforeTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Return true if meet is running before tg.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetRunsBeforeTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)
{
  KHE_TIME meet_first, tg_first;
  meet_first = KheMeetAsstTime(meet);
  if( meet_first != NULL && KheTimeGroupTimeCount(tg) > 0 )
  {
    tg_first = KheTimeGroupTime(tg, 0);
    return KheTimeIndex(meet_first) < KheTimeIndex(tg_first);
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskRunsBeforeTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Return true if task, or any task assigned directly or indirectly         */
/*  to task, runs at a time before tg.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskRunsBeforeTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TASK child_task;  int i;

  /* do it for task */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetRunsBeforeTimeGroup(meet, tg) )
    return true;

  /* do it for task's proper descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskRunsBeforeTimeGroup(child_task, tg) )
      return true;
  }

  /* no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddGroupToDisplay(KHE_GROUPED_TASKS_DISPLAY gtd,                 */
/*    KHE_GTD_GROUP gg)                                                      */
/*                                                                           */
/*  Add gg to gtd's display.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddGroupToDisplay(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_GTD_GROUP gg)
{
  char durn_char;  int i, col_index;  KHE_TIME_GROUP tg;
  KHE_GTD_ROW gtdr;  KHE_GTD_ENTRY gtde;  KHE_TASK task, proper_root_task;

  /* find the durn character and update undersized_durn and oversized_durn */
  durn_char = KheGtdGroupDurnChar(gg, gtd);

  /* find a column index for the group, making a new column if needed */
  col_index = KheGroupedTasksDisplayFindColumn(gtd, gg);

  /* add a history entry if required */
  if( gg->history_resource != NULL )
  {
    gtdr = HaArray(gtd->rows, 0);
    gtde = HaArray(gtdr->entries, col_index);
    KheGtdEntrySetHistory(gtde, gg, gg->history_resource, gg->history_durn,
      durn_char);
    gtdr->non_empty_count++;
    gtdr->starting_count++;
    if( KheIntervalLength(gg->interval) == 0 )
      gtdr->ending_count++;
  }

  /* add the current group at those indexes */
  for( i = gg->interval.first;  i <= gg->interval.last;  i++ )
  {
    tg = KheFrameTimeGroup(gtd->days_frame, i);
    task = KheGtdGroupTaskDuringTimeGroup(gg, tg, &proper_root_task);
    if( task != NULL )
    {
      gtdr = HaArray(gtd->rows, i + 1);
      gtde = HaArray(gtdr->entries, col_index);
      KheGtdEntrySet(gtde, gg, task, i == gg->interval.first &&
       	gg->history_resource == NULL ? KHE_RULE_FULL :
        KheTaskRunsBeforeTimeGroup(proper_root_task, tg) ?
	  KHE_RULE_SMALL : KHE_RULE_NONE, durn_char,
	  i == gg->interval.last ? gg->index_in_soln : -1 );
      gtdr->non_empty_count++;
      if( i == gg->interval.first && gg->history_resource == NULL )
	gtdr->starting_count++;
      if( i == gg->interval.last )
	gtdr->ending_count++;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GROUPED_TASKS_DISPLAY"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGtdClearDisplay(KHE_GROUPED_TASKS_DISPLAY gtd)                   */
/*                                                                           */
/*  Clear away any existing display, then add an empty display.              */
/*                                                                           */
/*****************************************************************************/

static void KheGtdClearDisplay(KHE_GROUPED_TASKS_DISPLAY gtd)
{
  int i;  KHE_TIME_GROUP tg;  KHE_GTD_ROW gtdr;

  /* clear away any existing display, recycling its memory */
  HaArrayForEach(gtd->rows, gtdr, i)
    KheGtdRowFree(gtdr, gtd);
  HaArrayClear(gtd->rows);

  /* make one row for each time group of the frame, plus one extra at first */
  gtdr = KheGtdRowMake(NULL, gtd);
  HaArrayAddLast(gtd->rows, gtdr);
  for( i = 0;  i < KheFrameTimeGroupCount(gtd->days_frame);  i++ )
  {
    tg = KheFrameTimeGroup(gtd->days_frame, i);
    gtdr = KheGtdRowMake(tg, gtd);
    HaArrayAddLast(gtd->rows, gtdr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGtdBuildDisplay(KHE_GROUPED_TASKS_DISPLAY gtd)                   */
/*                                                                           */
/*  Build the display of gtd.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheGtdBuildDisplay(KHE_GROUPED_TASKS_DISPLAY gtd)
{
  int i;  KHE_GTD_GROUP gg;
  KheGtdClearDisplay(gtd);
  HaArraySort(gtd->groups, &KheGtdGroupCmp);
  HaArrayForEach(gtd->groups, gg, i)
    KheAddGroupToDisplay(gtd, gg);
  gtd->rows_up_to_date = true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUPED_TASKS_DISPLAY KheGroupedTasksDisplayMake(KHE_SOLN soln,      */
/*    char *id, int min_limit, int max_limit, KHE_COST cost,                 */
/*    KHE_FRAME days_frame, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new grouped tasks display object for soln with these other        */
/*  attributes.  The cost is the cost of the groups, not of soln.            */
/*                                                                           */
/*****************************************************************************/

KHE_GROUPED_TASKS_DISPLAY KheGroupedTasksDisplayMake(KHE_SOLN soln,
  char *id, int min_limit, int max_limit, KHE_COST cost,
  KHE_FRAME days_frame, HA_ARENA a)
{
  KHE_GROUPED_TASKS_DISPLAY res;

  /* free lists */
  HaMake(res, a);
  HaArrayInit(res->entry_free_list, a);
  HaArrayInit(res->row_free_list, a);

  /* constant values */
  res->arena = a;
  res->soln = soln;
  /* res->domain_finder = KheTaskGroupDomainFi nderMake(soln, a); */
  res->id = id;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->cost = cost;
  res->days_frame = days_frame;

  /* fields that change as groups are added */
  res->curr_group = NULL;
  res->undersized_durn = 0;
  res->oversized_durn = 0;
  HaArrayInit(res->groups, a);

  /* the display */
  res->rows_up_to_date = false;
  HaArrayInit(res->rows, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayGroupBegin(KHE_GROUPED_TASKS_DISPLAY gtd,     */
/*    bool optional, int index_in_soln)                                      */
/*                                                                           */
/*  Begin a new group.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheGroupedTasksDisplayGroupBegin(KHE_GROUPED_TASKS_DISPLAY gtd,
  bool optional, int primary_durn, int index_in_soln)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupedTasksDisplayGroupBegin(gtd)\n");
  HnAssert(gtd->curr_group == NULL,
    "KheGroupedTasksDisplayGroupBegin internal error");
  gtd->curr_group = KheGtdGroupMake(optional, primary_durn,
    index_in_soln, gtd->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayGroupAddTask(KHE_GROUPED_TASKS_DISPLAY gtd,   */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Add task to the current group.                                           */
/*                                                                           */
/*****************************************************************************/

void KheGroupedTasksDisplayGroupAddTask(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_TASK task)
{
  HnAssert(gtd->curr_group != NULL,
    "KheGroupedTasksDisplayGroupAddTask internal error");
  HnAssert(KheTaskSoln(task) == gtd->soln,
    "KheGroupedTasksDisplayGroupAddTask internal error (task not from soln)");
  KheGtdGroupAddTask(gtd->curr_group, task, gtd);
  if( DEBUG1 )
    fprintf(stderr, "  KheGroupedTasksDisplayGroupAddTask(gtd, %s)\n",
      KheTaskId(task));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayGroupAddHistory(KHE_GROUPED_TASKS_DISPLAY gtd,*/
/*    KHE_RESOURCE r, int durn)                                              */
/*                                                                           */
/*  Add history to the current group.                                        */
/*                                                                           */
/*****************************************************************************/

void KheGroupedTasksDisplayGroupAddHistory(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_RESOURCE r, int durn)
{
  HnAssert(gtd->curr_group != NULL,
    "KheGroupedTasksDisplayGroupAddTask internal error");
  KheGtdGroupAddHistory(gtd->curr_group, r, durn);
  if( DEBUG1 )
    fprintf(stderr, "  KheGroupedTasksDisplayGroupAddHistory(gtd, %s, %d)\n",
      KheResourceId(r), durn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayGroupEnd(KHE_GROUPED_TASKS_DISPLAY gtd)       */
/*                                                                           */
/*  End one group.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheGroupedTasksDisplayGroupEnd(KHE_GROUPED_TASKS_DISPLAY gtd)
{
  HnAssert(gtd->curr_group != NULL,
    "KheGroupedTasksDisplayGroupEnd internal error");
  HaArrayAddLast(gtd->groups, gtd->curr_group);
  gtd->curr_group = NULL;
  gtd->rows_up_to_date = false;
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupedTasksDisplayGroupEnd(gtd)\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "print"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupedTaskDisplayIsNonEmpty(KHE_GROUPED_TASKS_DISPLAY gtd,      */
/*    int *first_row_index, int *last_row_index)                             */
/*                                                                           */
/*  If gtd is non-empty, return true with *first_row_index set to the        */
/*  index of the first non-empty row, and *last_row_index set to the index   */
/*  of the last non-empty row.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheGroupedTaskDisplayIsNonEmpty(KHE_GROUPED_TASKS_DISPLAY gtd,
  int *first_row_index, int *last_row_index)
{
  KHE_GTD_ROW gtdri, gtdrj;  int i, j;
  HaArrayForEach(gtd->rows, gtdri, i)
    if( !KheGtdRowIsEmpty(gtdri) )
      HaArrayForEachReverse(gtd->rows, gtdrj, j)
	if( !KheGtdRowIsEmpty(gtdrj) )
	  return *first_row_index = i, *last_row_index = j, true;
  return *first_row_index = *last_row_index = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksDisplayPrint(KHE_GROUPED_TASKS_DISPLAY gtd,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Print gtd onto fp with the given indent.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheNearMiddle(int pos, int width)
{
  return pos == width/2;
  /* return pos >= width/2 - 1 && pos <= width/2 + 1; */
}

void KheGroupedTasksDisplayPrint(KHE_GROUPED_TASKS_DISPLAY gtd,
  bool show_asst, int indent, FILE *fp)
{
  KHE_GTD_ROW gtdr;  KHE_GTD_ENTRY gtde;
  char buff[KHE_COL_WIDTH + 1], spaces[KHE_COL_WIDTH + 1];
  char stars[KHE_COL_WIDTH + 1], dashes[KHE_COL_WIDTH + 1];
  int i, j, first_row_index, last_row_index;

  /* check state */
  HnAssert(gtd->curr_group == NULL,
    "KheGroupedTasksDisplayPrint internal error");

  /* build (or rebuild) the display */
  if( !gtd->rows_up_to_date )
    KheGtdBuildDisplay(gtd);

  /* return early if all the rows are empty */
  if( !KheGroupedTaskDisplayIsNonEmpty(gtd, &first_row_index, &last_row_index) )
  {
    fprintf(fp, "%*s[ GroupedTasks(%s) - empty ]\n", indent, "", gtd->id);
    return;
  }

  /* make the spaces and dashes */
  for( i = 0;  i < KHE_COL_WIDTH;  i++ )
  {
    spaces[i] = ' ';
    stars[i] = KheNearMiddle(i, KHE_COL_WIDTH) ? '*' : ' ';
    dashes[i] = '-';
  }
  spaces[i] = '\0';
  dashes[i] = '\0';

  /* print the header line */
  fprintf(fp, "%*s[ GroupedTasks(%s, cost %.5f, undersized_durn %d, "
    "oversized_durn %d)\n", indent, "", gtd->id, KheCostShow(gtd->cost),
    gtd->undersized_durn, gtd->oversized_durn);

  /* print the rows, from the first non-empty row to the last non-empty row */
  for( i = first_row_index;  i <= last_row_index;  i++ )
  {
    /* print the separator line above the row */
    gtdr = HaArray(gtd->rows, i);
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    HaArrayForEach(gtdr->entries, gtde, j)
      fprintf(fp, "+%s", gtde->rule == KHE_RULE_NONE ? spaces :
	gtde->rule == KHE_RULE_SMALL ? stars : dashes);
    fprintf(fp, "+\n");

    /* print the first line of the row, holding the task names */
    fprintf(fp, "%*s%-*s", indent + 2, "", KHE_COL_WIDTH,
      gtdr->time_group == NULL ? "History" : KheTimeGroupId(gtdr->time_group));
    HaArrayForEach(gtdr->entries, gtde, j)
      fprintf(fp, "|%*s", KHE_COL_WIDTH, KheGtdEntryShowFirstLine(gtde, buff));
    fprintf(fp, "|\n");

    /* print the second line of the row, holding the domains and costs */
    snprintf(buff, KHE_COL_WIDTH, "%d (+%d, -%d)", gtdr->non_empty_count,
	gtdr->starting_count, gtdr->ending_count);
    fprintf(fp, "%*s%-*s", indent+2, "", KHE_COL_WIDTH, buff);
    HaArrayForEach(gtdr->entries, gtde, j)
      fprintf(fp, "|%*s", KHE_COL_WIDTH,
	KheGtdEntryShowSecondLine(gtde, show_asst, buff));
    fprintf(fp, "|\n");
  }

  /* print the final separator line */
  if( HaArrayCount(gtd->rows) > 0 )
  {
    gtdr = HaArrayFirst(gtd->rows);
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    HaArrayForEach(gtdr->entries, gtde, j)
      fprintf(fp, "+%s", dashes);
    fprintf(fp, "+\n");
  }

  /* finish off */
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GROUP_SIGNATURE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_SIGNATURE KheGroupSignatureMake(int length,                    */
/*    KHE_RESOURCE_GROUP domain, HA_ARENA a)                                 */
/*                                                                           */
/*  Make a group signature object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_GROUP_SIGNATURE KheGroupSignatureMake(int length,
  KHE_RESOURCE_GROUP domain, HA_ARENA a)
{
  KHE_GROUP_SIGNATURE res;
  HaMake(res, a);
  res->length = length;
  res->domain = domain;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupSignatureTypedCmp(KHE_GROUP_SIGNATURE gs1,                   */
/*    KHE_GROUP_SIGNATURE gs2)                                               */
/*                                                                           */
/*  Typed comparison function for sorting an array of group signatures by    */
/*  increasing length, then domain size, then first resource.                */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static int KheGroupSignatureTypedCmp(KHE_GROUP_SIGNATURE gs1,
  KHE_GROUP_SIGNATURE gs2)
{
  int cmp, count1, count2;  KHE_RESOURCE r1, r2;

  ** sort by length **
  cmp = gs1->length - gs2->length;
  if( cmp != 0 )
    return cmp;

  ** else sort by domain size **
  count1 = KheResourceGroupResourceCount(gs1->domain);
  count2 = KheResourceGroupResourceCount(gs2->domain);
  cmp = count1 - count2;
  if( cmp != 0 )
    return cmp;

  ** equal if both domains are empty **
  if( count1 == 0 )
    return 0;

  ** sort by first resource index **
  r1 = KheResourceGroupResource(gs1->domain, 0);
  r2 = KheResourceGroupResource(gs2->domain, 0);
  return KheResourceInstanceIndex(r1) - KheResourceInstanceIndex(r2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupSignatureCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Untyped comparison function for sorting an array of group signatures by  */
/*  increasing length, then domain size, then first resource.                */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static int KheGroupSignatureCmp(const void *t1, const void *t2)
{
  KHE_GROUP_SIGNATURE gs1 = * (KHE_GROUP_SIGNATURE *) t1;
  KHE_GROUP_SIGNATURE gs2 = * (KHE_GROUP_SIGNATURE *) t2;
  return KheGroupSignatureTypedCmp(gs1, gs2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupSignatureDebug(KHE_GROUP_SIGNATURE gs, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of gs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheGroupSignatureDebug(KHE_GROUP_SIGNATURE gs, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%d:%s", gs->length, KheResourceGroupId(gs->domain));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GROUPED_TASKS_SIGNATURE"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUPED_TASKS_SIGNATURE KheGroupedTasksSignatureMake(HA_ARENA a)     */
/*                                                                           */
/*  Make a grouped tasks signature object.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_GROUPED_TASKS_SIGNATURE KheGroupedTasksSignatureMake(HA_ARENA a)
{
  KHE_GROUPED_TASKS_SIGNATURE res;
  HaMake(res, a);
  HaArrayInit(res->group_signatures, a);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupedTasksSignatureEqual(KHE_GROUPED_TASKS_SIGNATURE sig1,     */
/*    KHE_GROUPED_TASKS_SIGNATURE sig2)                                      */
/*                                                                           */
/*  Return true if sig1 and sig2 are equal.  Their elements will have        */
/*  already been sorted.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** using KheGroupedTasksSignatureCompatible now
static bool KheGroupedTasksSignatureEqual(KHE_GROUPED_TASKS_SIGNATURE sig1,
  KHE_GROUPED_TASKS_SIGNATURE sig2)
{
  int count1, count2, i;  KHE_GROUP_SIGNATURE gs1, gs2;
  count1 = HaArrayCount(sig1->group_signatures);
  count2 = HaArrayCount(sig2->group_signatures);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    gs1 = HaArray(sig1->group_signatures, i);
    gs2 = HaArray(sig2->group_signatures, i);
    if( gs1->length != gs2->length ||
	!KheResourceGroupEqual(gs1->domain, gs2->domain) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupedTasksSignatureCompatible(KHE_GROUPED_TASKS_SIGNATURE sig1,*/
/*    KHE_GROUPED_TASKS_SIGNATURE sig2)                                      */
/*                                                                           */
/*  Return true if sig1 is compatible with sig2.  Their elements will have   */
/*  already been sorted.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheGroupedTasksSignatureCompatible(KHE_GROUPED_TASKS_SIGNATURE sig1,
  KHE_GROUPED_TASKS_SIGNATURE sig2)
{
  int count1, count2, i;  KHE_GROUP_SIGNATURE gs1, gs2;
  count1 = HaArrayCount(sig1->group_signatures);
  count2 = HaArrayCount(sig2->group_signatures);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    gs1 = HaArray(sig1->group_signatures, i);
    gs2 = HaArray(sig2->group_signatures, i);
    if( gs1->length != gs2->length ||
	!KheResourceGroupSubset(gs2->domain, gs1->domain) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupedTasksSignatureDebug(KHE_GROUPED_TASKS_SIGNATURE sig,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of sig onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheGroupedTasksSignatureDebug(KHE_GROUPED_TASKS_SIGNATURE sig,
  int verbosity, int indent, FILE *fp)
{
  KHE_GROUP_SIGNATURE gs;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[");
  HaArrayForEach(sig->group_signatures, gs, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheGroupSignatureDebug(gs, verbosity, -1, fp);
  }
  fprintf(fp, "]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "comparison"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEntryGroupSignature(KHE_GROUPED_TASKS_DISPLAY gtd, int row_index,*/
/*    int col_index, KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a, */
/*    KHE_GROUP_SIGNATURE *res)                                              */
/*                                                                           */
/*  Build the signature for the group ending at (row_index, col_index).      */
/*  This is not the same as the signature of any KHE_GTD_GROUP, because      */
/*  it may be incomplete (i.e. the KHE_GTD_GROUP may continue on to          */
/*  row_index + 1 and so on).                                                */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheEntryGroupSignature(KHE_GROUPED_TASKS_DISPLAY gtd, int row_index,
  int col_index, KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a,
  KHE_GROUP_SIGNATURE *res)
{
  KHE_GTD_ROW gtdr;  KHE_GTD_ENTRY gtde;  KHE_TASK_GROUP_DOMAIN_TYPE t;
  int ri, len;  KHE_TASK_GROUP_DOMAIN tg_domain;  KHE_RESOURCE_GROUP rg;
  len = 0;
  tg_domain = NULL;
  for( ri = row_index;  ri >= 1;  ri-- )
  {
    gtdr = HaArray(gtd->rows, ri);
    gtde = HaArray(gtdr->entries, col_index);
    len++;
    rg = KheTaskDomain(gtde->task);
    tg_domain = KheTaskGroupDomainMake(domain_finder, tg_domain, rg);
    if( gtde->rule == KHE_RULE_FULL )
      break;  ** stuff above here is not in the group **
  }
  if( len < gtd->max_limit )
  {
    *res = KheGroupSignatureMake(len, KheTaskGroupDomainValue(tg_domain,&t), a);
    return true;
  }
  else
  {
    *res = NULL;
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUPED_TASKS_SIGNATURE KheGroupedTasksDisplayBuildSignature(        */
/*    KHE_GROUPED_TASKS_DISPLAY gtd, int row_index, HA_ARENA a)              */
/*                                                                           */
/*  Build the signature of gtd at row_index.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_GROUPED_TASKS_SIGNATURE KheGroupedTasksDisplayBuildSignature(
  KHE_GROUPED_TASKS_DISPLAY gtd, int row_index,
  KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a)
{
  KHE_GTD_ROW gtdr;  KHE_GTD_ENTRY gtde;  int col_index;
  KHE_GROUPED_TASKS_SIGNATURE res;  KHE_GROUP_SIGNATURE gs;

  ** build (or rebuild) the display **
  if( !gtd->rows_up_to_date )
    KheGtdBuildDisplay(gtd);

  ** get the row **
  HnAssert(0 < row_index && row_index < HaArrayCount(gtd->rows),
    "KheGroupedTasksDisplayBuildSignature internal error (row_index %d out "
    "of range 1 .. %d)", row_index, HaArrayCount(gtd->rows) - 1);
  gtdr = HaArray(gtd->rows, row_index);

  ** build the signature **
  res = KheGroupedTasksSignatureMake(a);
  HaArrayForEach(gtdr->entries, gtde, col_index)
    if( gtde->task != NULL && !gtde->group->optional )
    {
      if( KheEntryGroupSignature(gtd, row_index,col_index,domain_finder,a,&gs) )
	HaArrayAddLast(res->group_signatures, gs);
    }

  ** sort the signature and return **
  HaArraySort(res->group_signatures, &KheGroupSignatureCmp);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupedTasksDisplayCompatible(KHE_GROUPED_TASKS_DISPLAY gtd1,    */
/*    KHE_GROUPED_TASKS_DISPLAY gtd2, int frame_index,                       */
/*    KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a)                */
/*                                                                           */
/*  Return true if gtd1 and gtd2 are compatible at frame_index.  Take        */
/*  working memory from arena a.                                             */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
bool KheGroupedTasksDisplayCompatible(KHE_GROUPED_TASKS_DISPLAY gtd1,
  KHE_GROUPED_TASKS_DISPLAY gtd2, int frame_index,
  KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a)
{
  bool res;  KHE_GROUPED_TASKS_SIGNATURE sig1, sig2;

  ** make sure the two solutions are for the same instance **
  HnAssert(KheSolnInstance(gtd1->soln) == KheSolnInstance(gtd2->soln),
    "KheGroupedTasksDisplayCompatible internal error (different instances)");

  ** build the signatures **
  sig1 = KheGroupedTasksDisplayBuildSignature(gtd1, frame_index + 1,
    domain_finder, a);
  sig2 = KheGroupedTasksDisplayBuildSignature(gtd2, frame_index + 1,
    domain_finder, a);

  ** return true if signatures are equal **
  res = KheGroupedTasksSignatureCompatible(sig1, sig2);
  if( DEBUG2 )
  {
    fprintf(stderr, "  compatible ");
    KheGroupedTasksSignatureDebug(sig1, 2, -1, stderr);
    fprintf(stderr, " vs ");
    KheGroupedTasksSignatureDebug(sig2, 2, -1, stderr);
    fprintf(stderr, " : %s\n", bool_show(res));
  }
  return res;
}
*** */

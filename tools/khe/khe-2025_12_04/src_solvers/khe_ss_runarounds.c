
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
/*  FILE:         khe_ss_runarounds.c                                        */
/*  DESCRIPTION:  Runaround structural solvers                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "minimum runaround duration"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheReset(KHE_NODE parent_node, int init_count)                      */
/*                                                                           */
/*  Reset the tree rooted at parent_node to its initial state, including     */
/*  unassigning all child meets, and removing all but the first init_count   */
/*  meets of parent_node.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheReset(KHE_NODE parent_node, int init_count)
{
  KheNodeUnAssignTimes(parent_node, NULL);
  while( KheNodeMeetCount(parent_node) > init_count )
    KheMeetDelete(KheNodeMeet(parent_node, init_count));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetCopyDomain(KHE_MEET dest_meet, KHE_MEET src_meet)            */
/*                                                                           */
/*  Add copies of the meet bounds of src_meet to dest_meet.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMeetCopyDomain(KHE_MEET dest_meet, KHE_MEET src_meet)
{
  int i;  KHE_MEET_BOUND mb;
  for( i = 0;  i < KheMeetMeetBoundCount(src_meet);  i++ )
  {
    mb = KheMeetMeetBound(src_meet, i);
    if( !KheMeetAddMeetBound(dest_meet, mb) )
    /* ***
    if( !KheMeetBoundMake(NULL, dest_meet, KheMeetBoundDuration(mb),
	  KheMeetBoundTimeGroup(mb), &mb2) )
    *** */
      HnAbort("KheMeetCopyDomain internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMinimumRunaroundDuration(KHE_NODE parent_node,                   */
/*    KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options, int *duration)  */
/*                                                                           */
/*  Set *duration to the minimum runaround duration of parent_node and       */
/*  return true, unless there is a problem, in which case return false       */
/*  with *duration undefined.  Use time_solver to carry out the time         */
/*  assignment trials required by this function.                             */
/*                                                                           */
/*  There is a problem if time_solver ever returns false, and if one         */
/*  trial produces a solution whose cost is not less than the cost of        */
/*  the solution produced by the previous trial.                             */
/*                                                                           */
/*****************************************************************************/

bool KheMinimumRunaroundDuration(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options, int *duration)
{
  KHE_COST init_cost, cost;  int init_count, i, j;  KHE_SOLN soln;
  KHE_MEET orig_meet, new_meet;
  if( DEBUG3 )
    fprintf(stderr, "[ KheMinimumRunaroundDuration(Node %d)\n",
      KheNodeSolnIndex(parent_node));

  /* deassign the children and record the cost */
  soln = KheNodeSoln(parent_node);
  KheNodeUnAssignTimes(parent_node, NULL);
  init_count = KheNodeMeetCount(parent_node);
  init_cost = KheSolnCost(soln);
  if( DEBUG3 )
    fprintf(stderr, "  child_count %d, init_cost %.5f\n",
      KheNodeChildCount(parent_node), KheCostShow(init_cost));

  /* repeatedly assign, compare costs, and add more meets if required */
  for( i = 0;  i <= KheNodeChildCount(parent_node);  i++ )
  {
    /* finish now if assigned and the cost is minimal */
    if( time_solver(parent_node, options) )
    {
      cost = KheSolnCost(soln);
      if( DEBUG3 )
      {
	fprintf(stderr, "  iteration %d assigned all times, cost %.5f:\n",
	  i, KheCostShow(cost));
	KheNodeDebug(parent_node, 2, 4, stderr);
	/* KheNodeMatchingDebug(parent_node, 3, 4, stderr); */
      }
      if( cost <= init_cost )
      {
	*duration = KheNodeDuration(parent_node);
	KheReset(parent_node, init_count);
	if( DEBUG3 )
	{
	  KheNodeDebug(parent_node, 2, 2, stderr);
	  fprintf(stderr, "] KheMinimumRunaroundDuration returning true (%d)\n",
	    *duration);
	}
	return true;
      }
    }

    /* no success yet, so widen parent_node, unassign, and try again */
    for( j = 0;  j < init_count;  j++ )
    {
      orig_meet = KheNodeMeet(parent_node, j);
      new_meet = KheMeetMake(soln, KheMeetDuration(orig_meet), NULL);
      KheMeetCopyDomain(new_meet, orig_meet);
      /* if( !KheMeetSet Domain(new_meet, KheMeetDomain(orig_meet)) ) */
      if( !KheNodeAddMeet(parent_node, new_meet) )
	HnAbort("KheMinimumRunaroundDuration internal error 2");
    }
    KheNodeUnAssignTimes(parent_node, NULL);
  }

  /* reach here only on failure */
  if( DEBUG3 )
    fprintf(stderr, "] KheMinimumRunaroundDuration returning false\n");
  KheReset(parent_node, init_count);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node templates"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_TEMPLATE - a template for a meet                                */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_meet_template_rec {
  int			  duration;		** duration of meet          **
  KHE_TIME_GROUP	  time_domain;		** domain of meet            **
} *KHE_MEET_TEMPLATE;

typedef HA_ARRAY(KHE_MEET_TEMPLATE) ARRAY_KHE_MEET_TEMPLATE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATE - a template for a node                                */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;

typedef struct khe_node_template_rec {
  int			frequency;		/* how often this occurs     */
  int			duration;		/* duration of node          */
  ARRAY_KHE_MEET	meets;			/* the meet (as templates)   */
} *KHE_NODE_TEMPLATE;

typedef HA_ARRAY(KHE_NODE_TEMPLATE) ARRAY_KHE_NODE_TEMPLATE;
typedef HA_ARRAY(ARRAY_KHE_NODE_TEMPLATE) ARRAY_ARRAY_KHE_NODE_TEMPLATE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATES - an array of node templates of equal duration        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_node_templates_rec {
  int			  duration;		/* the shared duration       */
  ARRAY_KHE_NODE_TEMPLATE node_templates;	/* the templates             */
} *KHE_NODE_TEMPLATES;

typedef HA_ARRAY(KHE_NODE_TEMPLATES) ARRAY_KHE_NODE_TEMPLATES;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATE_SET - a set of node templates, indexed by duration     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_node_template_set_rec {
  ARRAY_KHE_NODE_TEMPLATES   node_templates;	/* the templates */
} *KHE_NODE_TEMPLATE_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATE KheNodeTemplateMake(void)                              */
/*                                                                           */
/*  Make a new, empty node template.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE_TEMPLATE KheNodeTemplateMake(HA_ARENA a)
{
  KHE_NODE_TEMPLATE res;
  HaMake(res, a);
  res->frequency = 0;
  res->duration = 0;
  HaArrayInit(res->meets, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateAddMeet(KHE_NODE_TEMPLATE nt, KHE_MEET meet)         */
/*                                                                           */
/*  Add meet as a template template to nt.  Make sure the meets are in       */
/*  decreasing duration order.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheNodeTemplateAddMeet(KHE_NODE_TEMPLATE nt, KHE_MEET meet)
{
  int i;
  HaArrayAddLast(nt->meets, NULL);
  for( i = HaArrayCount(nt->meets) - 2;
    i >= 0 && KheMeetDuration(meet) > KheMeetDuration(HaArray(nt->meets, i));
    i-- )
    HaArrayPut(nt->meets, i + 1, HaArray(nt->meets, i));
  HaArrayPut(nt->meets, i + 1, meet);
  nt->duration += KheMeetDuration(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateAddNode(KHE_NODE_TEMPLATE nt, KHE_NODE node)         */
/*                                                                           */
/*  Add templates for the meets of node to nt.                               */
/*                                                                           */
/*****************************************************************************/

static void KheNodeTemplateAddNode(KHE_NODE_TEMPLATE nt, KHE_NODE node)
{
  KHE_MEET meet;  int i;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    KheNodeTemplateAddMeet(nt, meet);
    /* ***
    KheNodeTemplateAddMeetTemplate(nt, KheMeetDuration(meet),
      KheMeetDomain(meet));
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheNodeMakeFromTemplate(KHE_NODE_TEMPLATE nt, KHE_SOLN soln)    */
/*                                                                           */
/*  Make a node with no children, and meets based on nt.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE KheNodeMakeFromTemplate(KHE_NODE_TEMPLATE nt, KHE_SOLN soln)
{
  KHE_NODE res;  int i;  KHE_MEET meet, meet2;
  res = KheNodeMake(soln);
  if( DEBUG7 )
    fprintf(stderr, "[ KheNodeMakeFromTemplate(nt, soln), res = %p\n",
      (void *) res);
  HaArrayForEach(nt->meets, meet, i)
  {
    if( DEBUG7 )
      fprintf(stderr, "  (a) i = %d, res = %p\n", i, (void *) res);
    meet2 = KheMeetMake(soln, KheMeetDuration(meet), NULL);
    if( DEBUG7 )
      fprintf(stderr, "  (b) i = %d, res = %p\n", i, (void *) res);
    KheMeetCopyDomain(meet2, meet);
    /* ***
    if( !KheMeetSet Domain(meet, mt->time_domain) )
      HnAbort("KheNodeMakeFromTemplate internal error 1");
    *** */
    if( DEBUG7 )
      fprintf(stderr, "  (c) i = %d, res = %p\n", i, (void *) res);
    if( !KheNodeAddMeet(res, meet2) )
      HnAbort("KheNodeMakeFromTemplate internal error 2");
  }
  if( DEBUG7 )
    fprintf(stderr, "] KheNodeMakeFromTemplate returning %p\n", (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeTemplateEquivalent(KHE_NODE_TEMPLATE nt1,                    */
/*    KHE_NODE_TEMPLATE nt2)                                                 */
/*                                                                           */
/*  Return true if nt1 and nt2 are equivalent, in the sense of having        */
/*  meets of the same durations with the same time domains.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeTemplateEquivalent(KHE_NODE_TEMPLATE nt1,
  KHE_NODE_TEMPLATE nt2)
{
  int i;  KHE_MEET meet1, meet2;
  if( HaArrayCount(nt1->meets) != HaArrayCount(nt2->meets) )
    return false;
  for( i = 0;  i < HaArrayCount(nt1->meets);  i++ )
  {
    meet1 = HaArray(nt1->meets, i);
    meet2 = HaArray(nt2->meets, i);
    if( KheMeetDuration(meet1) != KheMeetDuration(meet2) ||
	!KheTimeGroupEqual(KheMeetDomain(meet1), KheMeetDomain(meet2)) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateDelete(KHE_NODE_TEMPLATE nt)                         */
/*                                                                           */
/*  Delete nt and the meet templates it contains.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheNodeTemplateDelete(KHE_NODE_TEMPLATE nt)
{
  ** ***
  while( HaArrayCount(nt->meet_templates) > 0 )
    MFree(HaArrayLastAndDelete(nt->meet_templates));
  *** **
  MArrayFree(nt->meets);
  MFree(nt);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateDebug(KHE_NODE_TEMPLATE nt, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of node template nt.                                         */
/*                                                                           */
/*****************************************************************************/

void KheNodeTemplateDebug(KHE_NODE_TEMPLATE nt, int verbosity,
  int indent, FILE *fp)
{
  KHE_MEET meet;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Node Template (freq %d): ", indent, "", nt->frequency);
    HaArrayForEach(nt->meets, meet, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      fprintf(fp, "%d ", KheMeetDuration(meet));
      if( KheMeetDomain(meet) == NULL )
	fprintf(fp, "auto");
      else
	KheTimeGroupDebug(KheMeetDomain(meet), 1, -1, fp);
    }
    fprintf(fp, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATE_SET KheNodeTemplateSetMake(void)                       */
/*                                                                           */
/*  Make a new, empty node template set.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE_TEMPLATE_SET KheNodeTemplateSetMake(HA_ARENA a)
{
  KHE_NODE_TEMPLATE_SET res;
  HaMake(res, a);
  HaArrayInit(res->node_templates, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_TEMPLATE KheNodeTemplateSetAddNodeTemplate(                     */
/*    KHE_NODE_TEMPLATE_SET nts, KHE_NODE_TEMPLATE nt)                       */
/*                                                                           */
/*  Add nt to nts.  If it proves to be the same as an existing node          */
/*  template, delete it, increment the frequency of the other one, and       */
/*  return it as a replacement for the nt passed in.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE_TEMPLATE KheNodeTemplateSetAddNodeTemplate(
  KHE_NODE_TEMPLATE_SET nts, KHE_NODE_TEMPLATE nt, HA_ARENA a)
{
  KHE_NODE_TEMPLATES tps;  KHE_NODE_TEMPLATE nt2;  int i, j;

  /* make sure that nt has a templates array for nt's duration */
  HaArrayFill(nts->node_templates, nt->duration + 1, NULL);
  if( HaArray(nts->node_templates, nt->duration) == NULL )
  {
    HaMake(tps, a);
    tps->duration = nt->duration;
    HaArrayInit(tps->node_templates, a);
    HaArrayPut(nts->node_templates, nt->duration, tps);
  }

  /* check whether something equivalent to nt is already in the array */
  tps = HaArray(nts->node_templates, nt->duration);
  HaArrayForEach(tps->node_templates, nt2, i)
    if( KheNodeTemplateEquivalent(nt, nt2) )
    {
      nt2->frequency++;
      for( j = i - 1; j >= 0 &&
	  HaArray(tps->node_templates, j)->frequency < nt2->frequency;  j-- )
	HaArrayPut(tps->node_templates, j+1, HaArray(tps->node_templates, j));
      HaArrayPut(tps->node_templates, j+1, nt2);
      /* KheNodeTemplateDelete(nt); */
      return nt2;
    }

  /* not present, so add it */
  HaArrayAddLast(tps->node_templates, nt);
  nt->frequency = 1;
  return nt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateSetDelete(KHE_NODE_TEMPLATE_SET nts)                 */
/*                                                                           */
/*  Delete nts.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheNodeTemplateSetDelete(KHE_NODE_TEMPLATE_SET nts)
{
  KHE_NODE_TEMPLATES tps;
  while( HaArrayCount(nts->node_templates) > 0 )
  {
    tps = HaArrayLastAndDelete(nts->node_templates);
    if( tps != NULL )
    {
      while( HaArrayCount(tps->node_templates) > 0 )
	KheNodeTemplateDelete(HaArrayLastAndDelete(tps->node_templates));
      MFree(tps);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/* KHE_NODE_TEMPLATE_SET KheNodeTemplateSetMakeFromNode(KHE_NODE parent_node)*/
/*                                                                           */
/*  Make a node template set from the child nodes of parent_node.            */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE_TEMPLATE_SET KheNodeTemplateSetMakeFromNode(
  KHE_NODE parent_node, HA_ARENA a)
{
  int i;  KHE_NODE child_node;  KHE_NODE_TEMPLATE_SET res;
  KHE_NODE_TEMPLATE nt;
  res = KheNodeTemplateSetMake(a);
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    nt = KheNodeTemplateMake(a);
    KheNodeTemplateAddNode(nt, child_node);
    nt = KheNodeTemplateSetAddNodeTemplate(res, nt, a);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeTemplateSetDebug(KHE_NODE_TEMPLATE_SET nts, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of nts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheNodeTemplateSetDebug(KHE_NODE_TEMPLATE_SET nts, int verbosity,
  int indent, FILE *fp)
{
  int i, j;  KHE_NODE_TEMPLATES tps;  KHE_NODE_TEMPLATE nt;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ NodeTemplateSet\n", indent, "");
    HaArrayForEach(nts->node_templates, tps, i)
      if( tps != NULL )
      {
	fprintf(fp, "%*s  duration %d:\n", indent, "", tps->duration);
	HaArrayForEach(tps->node_templates, nt, j)
	  KheNodeTemplateDebug(nt, verbosity, indent + 4, fp);
      }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "runarounds"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RUNAROUND_NODE - one node being processed by runaround code          */
/*                                                                           */
/*  NB we don't represent fixed nodes, they are useless so are left out.     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

typedef struct khe_runaround_node_rec {
  KHE_NODE		node;			/* the node                  */
  ARRAY_KHE_RESOURCE	layer_resources;	/* that the node holds       */
  int			meet_count;		/* its no. of meets          */
  int			child_count;		/* its no. of child nodes    */
  int			mrd;			/* its min runaround durn    */
  bool			problem;		/* true if problem node      */
} *KHE_RUNAROUND_NODE;

typedef HA_ARRAY(KHE_RUNAROUND_NODE) ARRAY_KHE_RUNAROUND_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUNAROUND_NODE KheRunaroundNodeMake(KHE_NODE node)                   */
/*                                                                           */
/*  Make a runaround node object about node, which may not be a fixed node.  */
/*  Leave its layer_resources array empty for now.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_RUNAROUND_NODE KheRunaroundNodeMake(KHE_NODE node, int mrd,
  HA_ARENA a)
{
  KHE_RUNAROUND_NODE res;
  HnAssert(KheNodeChildCount(node) >= 1, "KheRunaroundNodeMake internal error");
  HnAssert(node != NULL && KheNodeParent(node) != NULL,
    "KheRunaroundNodeMake internal error 2");
  HaMake(res, a);
  res->node = node;
  HaArrayInit(res->layer_resources, a);
  res->meet_count = KheNodeMeetCount(node);
  res->child_count = KheNodeChildCount(node);
  res->mrd = mrd;
  res->problem = (res->mrd > KheNodeDuration(node));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundNodeFree(KHE_RUNAROUND_NODE in)                         */
/*                                                                           */
/*  Free a runaround node.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheRunaroundNodeFree(KHE_RUNAROUND_NODE in)
{
  MArrayFree(in->layer_resources);
  MFree(in);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheRunaroundNodeCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Comparison function for sorting an array of runaround nodes              */
/*  into the order used by the runaround construction algorithm.             */
/*                                                                           */
/*****************************************************************************/

static int KheRunaroundNodeCmp(const void *t1, const void *t2)
{
  KHE_RUNAROUND_NODE in1 = * (KHE_RUNAROUND_NODE *) t1;
  KHE_RUNAROUND_NODE in2 = * (KHE_RUNAROUND_NODE *) t2;
  if( in1->problem )
  {
    if( in2->problem )
    {
      /* problem nodes are sorted by increasing mrd */
      if( in1->mrd != in2->mrd )
	return in1->mrd - in2->mrd;
      else
	return KheNodeSolnIndex(in1->node) - KheNodeSolnIndex(in2->node);
    }
    else
    {
      /* free node in2 precedes problem node in1 */
      return 1;
    }
  }
  else
  {
    if( in2->problem )
    {
      /* free node in1 precedes problem node in2 */
      return -1;
    }
    else
    {
      /* free nodes are sorted by decreasing duration */
      if( KheNodeDuration(in2->node) != KheNodeDuration(in1->node) )
	return KheNodeDuration(in2->node) - KheNodeDuration(in1->node);
      else
	return KheNodeSolnIndex(in1->node) - KheNodeSolnIndex(in2->node);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundNodeDebug(KHE_RUNAROUND_NODE rn, int indent, FILE *fp)  */
/*                                                                           */
/*  Print in onto fp with the given indent.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheRunaroundNodeDebug(KHE_RUNAROUND_NODE rn, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE r;
  fprintf(stderr, "%*s[ Node %d {", indent, "", KheNodeSolnIndex(rn->node));
  HaArrayForEach(rn->layer_resources, r, i)
    fprintf(stderr, "%s%s", i > 0 ? ", " : "",
      KheResourceId(r) == NULL ? "-" : KheResourceId(r));
  fprintf(stderr, "}: %d meets, %d children, %d mrd, %s ]\n",
    rn->meet_count, rn->child_count, rn->mrd, rn->problem ? "problem" : "free");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRunaroundNodeCompatible(KHE_RUNAROUND_NODE rn1,                  */
/*    KHE_RUNAROUND_NODE rn2)                                                */
/*                                                                           */
/*  Return true if rn1 and rn2 are compatible in the sense of lying in the   */
/*  same segments.                                                           */
/*                                                                           */
/*  Implementation note.  By the way that the layer resource arrays of rn1   */
/*  and rn2 were constructed, if they have the same resources they appear    */
/*  in the same order.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by comparison function used when sorting
static bool KheRunaroundNodeCompatible(KHE_RUNAROUND_NODE rn1,
  KHE_RUNAROUND_NODE rn2)
{
  int i;  KHE_RESOURCE r1, r2;
  if( HaArrayCount(rn1->layer_resources) != HaArrayCount(rn2->layer_resources) )
    return false;
  for( i = 0;  i < HaArrayCount(rn1->layer_resources);  i++ )
  {
    r1 = HaArray(rn1->layer_resources, i);
    r2 = HaArray(rn2->layer_resources, i);
    if( r1 != r2 )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMoveNodeAndChildren(KHE_NODE node, KHE_NODE run_root_node)       */
/*                                                                           */
/*  Move node and its children to below run_root_node.                       */
/*                                                                           */
/*****************************************************************************/

static void KheMoveNodeAndChildren(KHE_NODE node, KHE_NODE run_root_node)
{
  while( KheNodeChildCount(node) > 0 )
    if( !KheNodeMove(KheNodeChild(node, 0), run_root_node) )
      HnAbort("KheMoveNodeAndChildren internal error 1");
  if( !KheNodeMove(node, run_root_node) )
    HnAbort("KheMoveNodeAndChildren internal error 2");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryRunaround(KHE_RUNAROUND_NODE n,                               */
/*    ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,                  */
/*    KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options,                 */
/*    KHE_NODE_TEMPLATE nt)                                                  */
/*                                                                           */
/*  Try to make a runaround by merging n with nodes[first..last].  If        */
/*  successful, n's node will be the merged node, nodes[first..last] is      */
/*  deleted from nodes, and true is returned.  Otherwise everything is       */
/*  returned to how it was at the start of the call, and false is returned.  */
/*                                                                           */
/*****************************************************************************/

static bool KheTryRunaround(KHE_RUNAROUND_NODE n,
  ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,
  KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options,
  KHE_NODE_TEMPLATE nt, HA_ARENA a)
{
  KHE_RUNAROUND_NODE x;  int i;  KHE_COST cost_before;  KHE_SOLN soln;
  KHE_NODE parent_node, run_root_node;  KHE_MARK mark;  bool success;
  if( DEBUG6 )
  {
    fprintf(stderr, "  [ KheTryRunaround(n, nodes[%d .. %d]):\n", first, last);
    KheRunaroundNodeDebug(n, 4, stderr);
    for( i = first;  i <= last;  i++ )
    {
      x = HaArray(*nodes, i);
      KheRunaroundNodeDebug(x, 4, stderr);
    }
    if( nt != NULL )
      KheNodeTemplateDebug(nt, 2, 4, stderr);
  }

  /* boilerplate */
  parent_node = KheNodeParent(n->node);
  HnAssert(parent_node != NULL, "KheTryRunaround internal error 1");
  soln = KheNodeSoln(n->node);

  /* make a root node for the runaround based on nt, or on a new nt if none */
  if( nt != NULL )
    run_root_node = KheNodeMakeFromTemplate(nt, soln);
  else
  {
    nt = KheNodeTemplateMake(a);
    KheNodeTemplateAddNode(nt, n->node);
    for( i = first;  i <= last;  i++ )
    {
      x = HaArray(*nodes, i);
      KheNodeTemplateAddNode(nt, x->node);
    }
    run_root_node = KheNodeMakeFromTemplate(nt, soln);
    /* KheNodeTemplateDelete(nt); */
  }

  /* move the candidates nodes and their children to under run_root_node */
  HnAssert(KheNodeParent(n->node) == parent_node,
    "KheTryRunaround internal error 2");
  mark = KheMarkBegin(soln);
  KheMoveNodeAndChildren(n->node, run_root_node);
  for( i = first;  i <= last;  i++ )
  {
    x = HaArray(*nodes, i);
    KheMoveNodeAndChildren(x->node, run_root_node);
  }
  if( DEBUG6 )
  {
    fprintf(stderr, "    run root node:\n");
    KheNodeDebug(run_root_node, 3, 4, stderr);
  }

  /* timetable the run root node */
  cost_before = KheSolnCost(soln);
  success = time_solver(run_root_node, options) &&
    KheSolnCost(soln) <= cost_before;
  KheMarkEnd(mark, !success);

  if( success )
  {
    /* success; unassign, and remove the merged nodes from nodes */
    KheNodeUnAssignTimes(run_root_node, NULL);
    for( i = last;  i >= first;  i-- )
    {
      x = HaArray(*nodes, i);
      HaArrayDeleteAndShift(*nodes, i);
      /* KheRunaroundNodeFree(x); don't free x! done later by main prog */
    }
    n->node = run_root_node;  /* probably not necessary */
    if( !KheNodeAddParent(run_root_node, parent_node) )
      HnAbort("KheTryRunaround internal error 3");
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning true\n");
    return true;
  }
  else
  {
    HnAssert(KheNodeParent(n->node) == parent_node,
      "KheTryRunaround internal error 4");
    /* failure; restore the original node linkages */
    /* KheNodeUnAssignTimes(run_root_node, NULL); */
    while( KheNodeMeetCount(run_root_node) > 0 )
      KheMeetDelete(KheNodeMeet(run_root_node, 0));
    if( !KheNodeDelete(run_root_node) )
      HnAbort("KheTryRunaround internal error 5");
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning false\n");
    return false;
  }
}


/* *** old version, uses transactions
static bool KheTryRunaround(KHE_RUNAROUND_NODE n,
  ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,
  KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options, KHE_NODE_TEMPLATE nt)
{
  KHE_RUNAROUND_NODE x;  int i;  KHE_COST cost_before;  KHE_SOLN soln;
  KHE_NODE parent_node, run_root_node;  KHE_TRANSACTION t;
  if( DEBUG6 )
  {
    fprintf(stderr, "  [ KheTryRunaround(n, nodes[%d .. %d]):\n", first, last);
    KheRunaroundNodeDebug(n, 4, stderr);
    for( i = first;  i <= last;  i++ )
    {
      x = HaArray(*nodes, i);
      KheRunaroundNodeDebug(x, 4, stderr);
    }
    if( nt != NULL )
      KheNodeTemplateDebug(nt, 2, 4, stderr);
  }

  ** boilerplate **
  parent_node = KheNodeParent(n->node);
  HnAssert(parent_node != NULL, "KheTryRunaround: internal error 1");
  soln = KheNodeSoln(n->node);

  ** make a root node for the runaround based on nt, or on a new nt if none **
  if( nt != NULL )
    run_root_node = KheNodeMakeFromTemplate(nt, soln);
  else
  {
    nt = KheNodeTemplateMake();
    KheNodeTemplateAddNode(nt, n->node);
    for( i = first;  i <= last;  i++ )
    {
      x = HaArray(*nodes, i);
      KheNodeTemplateAddNode(nt, x->node);
    }
    run_root_node = KheNodeMakeFromTemplate(nt, soln);
    KheNodeTemplateDelete(nt);
  }

  ** move the candidates nodes and their children to under run_root_node **
  t = KheTransactionMake(soln);
  KheTransactionBegin(t);
  KheMoveNodeAndChildren(n->node, run_root_node);
  for( i = first;  i <= last;  i++ )
  {
    x = HaArray(*nodes, i);
    KheMoveNodeAndChildren(x->node, run_root_node);
  }
  KheTransactionEnd(t);
  if( DEBUG6 )
  {
    fprintf(stderr, "    run root node:\n");
    KheNodeDebug(run_root_node, 3, 4, stderr);
  }

  ** timetable the run root node **
  cost_before = KheSolnCost(soln);
  if( time_solver(run_root_node, options) && KheSolnCost(soln) <= cost_before )
  {
    ** success; unassign, and remove the merged nodes from nodes **
    KheNodeUnAssignTimes(run_root_node, NULL);
    for( i = last;  i >= first;  i-- )
    {
      x = HaArray(*nodes, i);
      HaArrayDeleteAndShift(*nodes, i);
      ** KheRunaroundNodeFree(x); don't free x! done later by main prog **
    }
    n->node = run_root_node;  ** probably not necessary **
    if( !KheNodeAddParent(run_root_node, parent_node) )
      HnAbort("KheTryRunaround internal error 2");
    KheTransactionDelete(t);
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning true\n");
    return true;
  }
  else
  {
    ** failure; restore the original node linkages **
    KheNodeUnAssignTimes(run_root_node, NULL);
    KheTransactionUndo(t);
    KheTransactionDelete(t);
    while( KheNodeMeetCount(run_root_node) > 0 )
      KheMeetDelete(KheNodeMeet(run_root_node, 0));
    if( !KheNodeDelete(run_root_node) )
      HnAbort("KheTryRunaround internal error 3");
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning false\n");
    return false;
  }
}
*** */


/* *** very old version
static bool KheTryRunaround(KHE_RUNAROUND_NODE n,
  ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,
  KHE_NODE_TIME_SOLVER time_solver, KHE_NODE_TEMPLATE nt)
{
  KHE_RUNAROUND_NODE x;  int i;  KHE_COST cost_before;
  if( DEBUG6 )
  {
    fprintf(stderr, "  [ KheTryRunaround(n, nodes[%d .. %d]):\n", first, last);
    KheRunaroundNodeDebug(n, 4, stderr);
    for( i = first;  i <= last;  i++ )
    {
      x = HaArray(*nodes, i);
      KheRunaroundNodeDebug(x, 4, stderr);
    }
    if( nt != NULL )
      KheNodeTemplateDebug(nt, 2, 4, stderr);
  }

  ** merge the nodes **
  for( i = first;  i <= last;  i++ )
  {
    x = HaArray(*nodes, i);
    KheNode Merge(n->node, x->node, &n->node);
    x->node = NULL;  ** it's undefined, so we make that clear **
  }
  if( DEBUG6 )
  {
    fprintf(stderr, "    merged node:\n");
    KheNodeDebug(n->node, 3, 4, stderr);
  }

  ** timetable the merged node **
  cost_before = KheSolnCost(KheNodeSoln(n->node));
  if( time_solver(n->node) && KheSolnCost(KheNodeSoln(n->node)) <= cost_before )
  {
    ** success; remove the merged nodes from nodes **
    KheNodeUnAssignTimes(n->node);
    for( i = last;  i >= first;  i-- )
    {
      x = HaArray(*nodes, i);
      HaArrayDeleteAndShift(*nodes, i);
      ** KheRunaroundNodeFree(x); don't free x! done later by main prog **
    }
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning true\n");
    return true;
  }
  else
  {
    ** failure; split the merged node back to the original state **
    KheNodeUnAssignTimes(n->node);
    for( i = last;  i >= first;  i-- )
    {
      x = HaArray(*nodes, i);
      if( !KheNode Split(n->node, KheNodeMeetCount(n->node) - x->meet_count,
	  KheNodeChildCount(n->node) - x->child_count, &n->node, &x->node) )
	HnAbort("KheTryRunaround internal error");
      if( DEBUG6 )
      {
	fprintf(stderr, "    split: ");
	KheRunaroundNodeDebug(x, 0, stderr);
	KheNodeDebug(x->node, 3, 4, stderr);
      }
    }
    if( DEBUG6 )
      fprintf(stderr, "  ] KheTryRunaround returning false\n");
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryRunarounds(KHE_RUNAROUND_NODE n,                              */
/*    ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,                  */
/*    KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options,                 */
/*    int durn, KHE_NODE_TEMPLATE_SET nts)                                   */
/*                                                                           */
/*  Try runarounds for n + nodes[first..last].  Their total duration is      */
/*  durn, so try with template nodes from nts of this duration; but, as      */
/*  a last resort, also try without a template node.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheTryRunarounds(KHE_RUNAROUND_NODE n,
  ARRAY_KHE_RUNAROUND_NODE *nodes, int first, int last,
  KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options,
  int durn, KHE_NODE_TEMPLATE_SET nts, HA_ARENA a)
{
  KHE_NODE_TEMPLATES tps;  KHE_NODE_TEMPLATE nt;  int i;
  if( DEBUG7 )
    fprintf(stderr,
      "  KheTryRunarounds s, n %p, n->node %p, KheNodeParent(n->node) %p\n",
      (void *) n, (void *) n->node, (void *) KheNodeParent(n->node));
  HnAssert(KheNodeParent(n->node) != NULL, "KheTryRunarounds internal error 1");
  if( HaArrayCount(nts->node_templates) > durn )
  {
    tps = HaArray(nts->node_templates, durn);
    if( tps != NULL )
      HaArrayForEach(tps->node_templates, nt, i)
      {
	if( DEBUG7 )
	 fprintf(stderr,
	 "  KheTryRunarounds %d, n %p, n->node %p, KheNodeParent(n->node) %p\n",
	    i, (void *) n, (void *) n->node, (void *) KheNodeParent(n->node));
	HnAssert(KheNodeParent(n->node) != NULL,
	  "KheTryRunarounds internal error 2");
        if( KheTryRunaround(n, nodes, first, last, time_solver, options, nt,a) )
	  return true;
      }
  }
  if( DEBUG7 )
    fprintf(stderr,
      "  KheTryRunarounds e, n %p, n->node %p, KheNodeParent(n->node) %p\n",
      (void *) n, (void *) n->node, (void *) KheNodeParent(n->node));
  HnAssert(KheNodeParent(n->node) != NULL, "KheTryRunarounds internal error 3");
  return KheTryRunaround(n, nodes, first, last, time_solver, options, NULL, a);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRunaroundNodeLexCompar(KHE_RUNAROUND_NODE rn1,                    */
/*    KHE_RUNAROUND_NODE rn2)                                                */
/*                                                                           */
/*  Return an integer less than, equal to, or greater than zero if the       */
/*  resources of rn1 are lexically less than, equal to, or greater than      */
/*  the resources of rn2.                                                    */
/*                                                                           */
/*****************************************************************************/
#define min(x, y) ((x) < (y) ? (x) : (y))

static int KheRunaroundNodeLexCompar(KHE_RUNAROUND_NODE rn1,
  KHE_RUNAROUND_NODE rn2)
{
  int i, lim;  KHE_RESOURCE r1, r2;
  lim = min(HaArrayCount(rn1->layer_resources), HaArrayCount(rn2->layer_resources));
  for( i = 0;  i < lim;  i++ )
  {
    r1 = HaArray(rn1->layer_resources, i);
    r2 = HaArray(rn2->layer_resources, i);
    if( KheResourceInstanceIndex(r1) != KheResourceInstanceIndex(r2) )
      return KheResourceInstanceIndex(r1) - KheResourceInstanceIndex(r2);
  }
  return HaArrayCount(rn1->layer_resources) - HaArrayCount(rn2->layer_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRunaroundNodesHaveSameResources(KHE_RUNAROUND_NODE rn1,          */
/*    KHE_RUNAROUND_NODE rn2)                                                */
/*                                                                           */
/*  Return true if these two nodes have the same resources.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheRunaroundNodesHaveSameResources(KHE_RUNAROUND_NODE rn1,
  KHE_RUNAROUND_NODE rn2)
{
  return KheRunaroundNodeLexCompar(rn1, rn2) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRunaroundNodeLexCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Comparison function for sorting an array of runaround nodes so that      */
/*  runaround nodes with the same resources appear together, and null        */
/*  array entries go on the end.                                             */
/*                                                                           */
/*****************************************************************************/

static int KheRunaroundNodeLexCmp(const void *t1, const void *t2)
{
  KHE_RUNAROUND_NODE rn1 = * (KHE_RUNAROUND_NODE *) t1;
  KHE_RUNAROUND_NODE rn2 = * (KHE_RUNAROUND_NODE *) t2;
  int cmp;
  if( rn1 == NULL )
    return rn2 == NULL ? 0 : 1;
  else if( rn2 == NULL )
    return -1;
  else
  {
    cmp = KheRunaroundNodeLexCompar(rn1, rn2);
    if( cmp != 0 )
      return cmp;
    else
      return KheNodeSolnIndex(rn1->node) - KheNodeSolnIndex(rn2->node);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HandleOneMergeableSet(ARRAY_KHE_RUNAROUND_NODE *nodes,              */
/*    int u, int v, KHE_NODE_TIME_SOLVER rs, KHE_OPTIONS options,            */
/*    KHE_NODE_TEMPLATE_SET nts, HA_ARENA a)                                 */
/*                                                                           */
/*  Handle one set of mergeable nodes.  This is the algorithm documented in  */
/*  the User's Guide, except for nts.  Fixed nodes have already been         */
/*  omitted from *nodes.                                                     */
/*                                                                           */
/*****************************************************************************/

static void HandleOneMergeableSet(ARRAY_KHE_RUNAROUND_NODE *nodes,
  int u, int v, KHE_NODE_TIME_SOLVER rs, KHE_OPTIONS options,
  KHE_NODE_TEMPLATE_SET nts, HA_ARENA a)
{
  KHE_RUNAROUND_NODE n, x;  int i, durn;  bool done;
  HaArraySort(*nodes, &KheRunaroundNodeCmp);
  if( DEBUG4 )
  {
    fprintf(stderr, "  [ HandleOneMergeableSet(nodes, u %d, v %d):\n", u, v);
    HaArrayForEach(*nodes, n, i)
      KheRunaroundNodeDebug(n, 4, stderr);
  }
  while( HaArrayCount(*nodes) > 0 )
  {
    n = HaArrayLastAndDelete(*nodes);
    if( n->problem && n->mrd < v )
    {
      /* Case 1 */
      done = false;
      for( i = HaArrayCount(*nodes) - 1;  i >= 0 && !done;  i-- )
      {
	x = HaArray(*nodes, i);
	durn = KheNodeDuration(n->node) + KheNodeDuration(x->node);
	if( n->mrd <= durn && durn == u && durn <= v &&
	    KheTryRunarounds(n, nodes, i, i, rs, options, durn, nts, a) )
	  done = true;
      }

      /* Case 2 */
      durn = KheNodeDuration(n->node);
      for( i = HaArrayCount(*nodes) - 1;  i >= 0 && !done;  i-- )
      {
	x = HaArray(*nodes, i);
	durn += KheNodeDuration(x->node);
	if( durn > v )
	  break;
	if( n->mrd <= durn && KheTryRunarounds(n, nodes, i,
	    HaArrayCount(*nodes) - 1, rs, options, durn, nts, a) )
	  done = true;
      }
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "  ] HandleOneMergeableSet returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool LayerIsOfInterest(KHE_LAYER layer)                                  */
/*                                                                           */
/*  Return true of layer is of interest (is a candidate for coordinating).   */
/*                                                                           */
/*****************************************************************************/

static bool LayerIsOfInterest(KHE_LAYER layer)
{
  return KheResourcePartition(KheLayerResource(layer, 0)) != NULL &&
    KheLayerDuration(layer) >= 0.9*KheNodeDuration(KheLayerParentNode(layer));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBuildRunarounds(KHE_NODE parent_node,                            */
/*    KHE_NODE_TIME_SOLVER mrd_solver, KHE_OPTIONS mrd_options,              */
/*    KHE_NODE_TIME_SOLVER rs, KHE_OPTIONS rr_options)                       */
/*                                                                           */
/*  Build runarounds for the problem child nodes of parent_node.             */
/*                                                                           */
/*****************************************************************************/

void KheBuildRunarounds(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER mrd_solver, KHE_OPTIONS mrd_options,
  KHE_NODE_TIME_SOLVER rs, KHE_OPTIONS runaround_options)
{
  HA_ARRAY_INT frequencies;  int i, j, u, v, freq, u_freq, mrd;
  KHE_RESOURCE r;  KHE_INSTANCE ins;  KHE_NODE child_node;  KHE_LAYER layer;
  ARRAY_KHE_RUNAROUND_NODE runaround_nodes, nodes;  KHE_RUNAROUND_NODE rn;
  bool has_problem;  KHE_NODE_TEMPLATE_SET nts;  HA_ARENA a;  KHE_SOLN soln;

  if( DEBUG4 )
    fprintf(stderr, "[ KheBuildRunarounds(Node %d)\n",
      KheNodeSolnIndex(parent_node));

  /* sort out node templates */
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */
  nts = KheNodeTemplateSetMakeFromNode(parent_node, a);
  if( DEBUG4 )
    KheNodeTemplateSetDebug(nts, 2, 2, stderr);

  /* use a frequency table to choose a suitable value of u */
  HaArrayInit(frequencies, a);
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    HaArrayFill(frequencies, KheNodeDuration(child_node) + 1, 0);
    ++HaArray(frequencies, KheNodeDuration(child_node));
  }
  u = u_freq = 0;
  HaArrayForEach(frequencies, freq, i)
    if( freq * i > u_freq * u )
      u = i, u_freq = freq;
  HaArrayFree(frequencies);

  /* use one-fifth of the duration of parent_node as v */
  v = KheNodeDuration(parent_node) / 5;
  if( DEBUG4 )
    fprintf(stderr, "  u = %d, v = %d\n", u, v);

  /* build runaround nodes for useful child nodes, indexed by node index */
  HaArrayInit(runaround_nodes, a);
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    if( DEBUG4 )
    {
      fprintf(stderr, "  examining child_node (%d children):\n",
	KheNodeChildCount(child_node));
      KheNodeDebug(child_node, 2, 2, stderr);
    }
    if( KheNodeChildCount(child_node) >= 1 &&
	KheMinimumRunaroundDuration(child_node, mrd_solver, mrd_options, &mrd)
	&& mrd <= v )
    {
      rn = KheRunaroundNodeMake(child_node, mrd, a);
      HaArrayFill(runaround_nodes, KheNodeSolnIndex(child_node) + 1, NULL);
      HaArrayPut(runaround_nodes, KheNodeSolnIndex(child_node), rn);
    }
  }

  /* distribute layer resources to runaround nodes */
  ins = KheSolnInstance(KheNodeSoln(parent_node));
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    if( KheResourceLayerDuration(r) >= 0.9 * KheNodeDuration(parent_node) )
    {
      layer = KheLayerMakeFromResource(parent_node, r);
      if( DEBUG4 )
      {
	fprintf(stderr, "  resource %s (layer durn %d) %sof interest:\n",
	  KheResourceId(r) == NULL ? "-" : KheResourceId(r),
	  KheResourceLayerDuration(r), LayerIsOfInterest(layer) ? "" : "not ");
	KheLayerDebug(layer, 2, 2, stderr);
      }
      if( LayerIsOfInterest(layer) )
      {
	for( j = 0;  j < KheLayerChildNodeCount(layer);  j++ )
	{
	  child_node = KheLayerChildNode(layer, j);
	  HaArrayFill(runaround_nodes, KheNodeSolnIndex(child_node) + 1, NULL);
	  rn = HaArray(runaround_nodes, KheNodeSolnIndex(child_node));
	  if( rn != NULL )
	    HaArrayAddLast(rn->layer_resources, r);
	}
      }
      KheLayerDelete(layer);
    }
  }

  /* bring nodes with the same layers together, and remove null entries */
  HaArraySort(runaround_nodes, &KheRunaroundNodeLexCmp);
  while( HaArrayCount(runaround_nodes) > 0 &&
         HaArrayLast(runaround_nodes)==NULL )
    HaArrayDeleteLast(runaround_nodes);
  if( DEBUG4 )
  {
    fprintf(stderr, "  [ runaround nodes:\n");
    HaArrayForEach(runaround_nodes, rn, i)
    {
      fprintf(stderr, "    %d: ", i);
      KheRunaroundNodeDebug(rn, 4, stderr);
    }
    fprintf(stderr, "  ]\n");
  }

  /* handle each set of mergeable nodes */
  HaArrayInit(nodes, a);
  for( i = 0;  i < HaArrayCount(runaround_nodes);  i = j )
  {
    /* build one set of mergeable nodes */
    HaArrayClear(nodes);
    rn = HaArray(runaround_nodes, i);
    has_problem = rn->problem;
    HaArrayAddLast(nodes, rn);
    for( j = i + 1;  j < HaArrayCount(runaround_nodes);  j++ )
    {
      rn = HaArray(runaround_nodes, j);
      if( !KheRunaroundNodesHaveSameResources(rn, HaArrayFirst(nodes)) )
	break;
      if( rn->problem )
	has_problem = true;
      HaArrayAddLast(nodes, rn);
    }

    /* if this set has a problem, handle it */
    if( has_problem )
      HandleOneMergeableSet(&nodes, u, v, rs, runaround_options, nts, a);
  }
  /* MArrayFree(nodes); */

  /* free runaround nodes memory */
  /* ***
  HaArrayForEach(runaround_nodes, rn, i)
    KheRunaroundNodeFree(rn);
  MArrayFree(runaround_nodes);
  *** */

  /* free template nodes memory */
  /* ***
  KheNodeTemplateSetDelete(nts);
  *** */

  if( DEBUG4 )
  {
    fprintf(stderr, "  at end of KheBuildRunarounds:\n");
    KheNodeDebug(parent_node, 3, 2, stderr);
    fprintf(stderr, "] KheBuildRunarounds returning\n");
  }
  /* HaAren aDelete(a); */
  KheSolnArenaEnd(soln, a);
}

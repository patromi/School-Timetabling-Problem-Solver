
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
/*  FILE:         khe_st_helper.c                                            */
/*  DESCRIPTION:  Helper functions for time solvers                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet swapping"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetSwapCheck(KHE_NODE node1, KHE_NODE node2)                */
/*                                                                           */
/*  Check whether it is possible to swap the assignments of the meets of     */
/*  node1 and node2.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMeetSwapCheck(KHE_NODE node1, KHE_NODE node2)
{
  KHE_MEET meet1, meet2;  int i;  KHE_TIME time1, time2;

  /* both nodes must be non-NULL */
  HnAssert(node1 != NULL, "KheNodeMeetSwapCheck: node1 is NULL");
  HnAssert(node2 != NULL, "KheNodeMeetSwapCheck: node2 is NULL");

  /* the nodes must be distinct */
  if( node1 == node2 )
    return false;

  /* the nodes must have the same durations */
  if( KheNodeDuration(node1) != KheNodeDuration(node2) )
    return false;

  /* the nodes must have the same number of meets */
  if( KheNodeMeetCount(node1) != KheNodeMeetCount(node2) )
    return false;

  /* corresponding meets must have equal durations and be swappable */
  for( i = 0;  i < KheNodeMeetCount(node1);  i++ )
  {
    meet1 = KheNodeMeet(node1, i);
    meet2 = KheNodeMeet(node2, i);
    if( KheMeetDuration(meet1) != KheMeetDuration(meet2) ||
	KheMeetIsAssignedPreassigned(meet1, &time1) ||
	KheMeetIsAssignedPreassigned(meet2, &time2) ||
	!KheMeetSwapCheck(meet1, meet2) )
      return false;
  }

  /* all in order */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetSwap(KHE_NODE node1, KHE_NODE node2)                     */
/*                                                                           */
/*  Swap the assignments of the meets of node1 and node2, if possible.       */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMeetSwap(KHE_NODE node1, KHE_NODE node2)
{
  KHE_MEET meet1, meet2;  int i;
  if( DEBUG8 )
    fprintf(stderr, "[ KheNodeMeetSwap(node1, node2)\n");

  /* make sure safe to proceed */
  if( !KheNodeMeetSwapCheck(node1, node2) )
  {
    if( DEBUG8 )
      fprintf(stderr, "] KheNodeMeetSwap returning false\n");
    return false;
  }

  /* do the swaps and return true */
  for( i = 0;  i < KheNodeMeetCount(node1);  i++ )
  {
    meet1 = KheNodeMeet(node1, i);
    meet2 = KheNodeMeet(node2, i);
    if( !KheMeetSwap(meet1, meet2) )
      HnAbort("KheNodeMeetSwap internal error");
  }
  if( DEBUG8 )
    fprintf(stderr, "] KheNodeMeetSwap returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node-regular assignment"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetRegularAssignCheck(KHE_NODE node, KHE_NODE sibling_node) */
/*                                                                           */
/*  Check whether it is possible to assign the meets of node, using the      */
/*  assignments of sibling_node as a template.                               */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMeetRegularAssignCheck(KHE_NODE node, KHE_NODE sibling_node)
{
  int junk, i;  KHE_MEET meet, sibling_meet;

  /* the nodes must have the same duration */
  if( KheNodeDuration(node) != KheNodeDuration(sibling_node) )
  {
    if( DEBUG6 )
      fprintf(stderr,
	"    KheNodeMeetRegularAssignCheck(Node %d, Node %d) durn\n",
	KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
    return false;
  }

  /* every meet of sibling_node must be assigned */
  if( KheNodeAssignedDuration(sibling_node) != KheNodeDuration(sibling_node) )
  {
    if( DEBUG6 )
      fprintf(stderr,
	"    KheNodeMeetRegularAssignCheck(Node %d, Node %d) ad %d != d %d",
	KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node),
	KheNodeAssignedDuration(sibling_node), KheNodeDuration(sibling_node));
    return false;
  }

  /* the nodes must be regular */
  if( !KheNodeRegular(node, sibling_node, &junk) )
  {
    if( DEBUG6 )
      fprintf(stderr,
	"    KheNodeMeetRegularAssignCheck(Node %d, Node %d) regl\n",
	KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
    return false;
  }

  /* the corresponding assignments must be possible or already done */
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    sibling_meet = KheNodeMeet(sibling_node, i);
    if( KheMeetAsst(meet) != NULL )
    {
      if( KheMeetAsst(sibling_meet) != KheMeetAsst(meet) ||
	  KheMeetAsstOffset(sibling_meet) != KheMeetAsstOffset(meet) )
      {
	if( DEBUG6 )
	  fprintf(stderr,
	    "    KheNodeMeetRegularAssignCheck(Node %d, Node %d) a\n",
	    KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
	return false;
      }
    }
    else
    {
      if( !KheMeetAssignCheck(meet, KheMeetAsst(sibling_meet),
	    KheMeetAsstOffset(sibling_meet)) )
      {
	if( DEBUG6 )
	  fprintf(stderr,
	    "    KheNodeMeetRegularAssignCheck(Node %d, Node %d) c\n",
	    KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
	return false;
      }
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "    KheNodeMeetRegularAssignCheck(Node %d, Node %d) OK\n",
      KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetRegularAssign(KHE_NODE node, KHE_NODE sibling_node)      */
/*                                                                           */
/*  If possible, assign the meets of node, using the assignments of          */
/*  sibling_node as a template.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMeetRegularAssign(KHE_NODE node, KHE_NODE sibling_node)
{
  int i;  KHE_MEET meet, sibling_meet;
  if( !KheNodeMeetRegularAssignCheck(node, sibling_node) )
    return false;
  if( DEBUG7 )
    fprintf(stderr, "    [ KheNodeMeetRegularAssign(Node %d, Node %d)\n",
      KheNodeSolnIndex(node), KheNodeSolnIndex(sibling_node));
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    sibling_meet = KheNodeMeet(sibling_node, i);
    if( KheMeetAsst(meet) == NULL )
    {
      if( !KheMeetAssign(meet, KheMeetAsst(sibling_meet),
	    KheMeetAsstOffset(sibling_meet)) )
	HnAbort("KheNodeMeetRegularAssign internal error");
    }
    if( DEBUG7 )
    {
      KHE_TIME time = KheMeetAsstTime(sibling_meet);
      fprintf(stderr, "      assigning %s to ",
	time == NULL ? "?" : KheTimeId(time) == NULL ? "-" : KheTimeId(time));
      KheMeetDebug(meet, 1, 0, stderr);
    }
  }
  if( DEBUG7 )
    fprintf(stderr, "    ] KheNodeMeetRegularAssign\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node meet unassignment"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheNodeMeetUnAssign(KHE_NODE node)                                  */
/*                                                                           */
/*  Unassign the meets of node.                                              */
/*                                                                           */
/*****************************************************************************/

void KheNodeMeetUnAssign(KHE_NODE node)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    if( KheMeetAsst(meet) != NULL )
      KheMeetUnAssign(meet);  /* in KheNodeMeetUnAssign, which is unused */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "checking for unassigned preassigned meets"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCheckForUnassignedPreassignedMeets(KHE_SOLN soln)                */
/*                                                                           */
/*  Check soln for unassigned preassigned meets; abort if any are found.     */
/*                                                                           */
/*****************************************************************************/

void KheCheckForUnassignedPreassignedMeets(KHE_SOLN soln, char *msg)
{
  KHE_MEET meet;  int i, found_count;  KHE_TIME time;
  found_count = 0;
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( !KheMeetIsCycleMeet(meet) && KheMeetIsPreassigned(meet, &time)
	&& KheMeetAsst(meet) == NULL )
    {
      fprintf(stderr, "%s: found unassigned preassigned meet %s\n",
	msg, KheMeetId(meet));
      found_count++;
    }
  }
  HnAssert(found_count == 0, "%s: aborting (%d unassigned preassigned meet%s)",
    msg, found_count, found_count == 1 ? "" : "s");
}

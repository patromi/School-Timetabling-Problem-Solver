
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
/*  FILE:         nrc_worker_set_tree.c                                      */
/*  MODULE:       A tree of worker-sets                                      */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_TREE                                                      */
/*                                                                           */
/*  A tree of worker-sets                                                    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER_SET) ARRAY_NRC_WORKER_SET;
typedef HA_ARRAY(NRC_WORKER_SET_TREE) ARRAY_NRC_WORKER_SET_TREE;

struct nrc_worker_set_tree_rec {
  ARRAY_NRC_WORKER_SET		worker_sets;
  ARRAY_NRC_WORKER_SET_TREE	children;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_TREE NrcWorkerSetTreeDoMake(NRC_INSTANCE ins)             */
/*                                                                           */
/*  Make a new worker-set tree containing no worker-sets.  This is just      */
/*  a stepping-stone to true creations.                                      */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET_TREE NrcWorkerSetTreeDoMake(NRC_INSTANCE ins)
{
  NRC_WORKER_SET_TREE res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  HaArrayInit(res->worker_sets, a);
  HaArrayInit(res->children, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_TREE NrcWorkerSetTreeMake(NRC_INSTANCE ins)               */
/*                                                                           */
/*  Make a new worker-set tree containing a single node which contains       */
/*  a single worker-set, namely NrcInstanceStaffing(ins).                    */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_TREE NrcWorkerSetTreeMake(NRC_INSTANCE ins)
{
  NRC_WORKER_SET_TREE res;
  res = NrcWorkerSetTreeDoMake(ins);
  HaArrayAddLast(res->worker_sets, NrcInstanceStaffing(ins));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetTreeFree(NRC_WORKER_SET_TREE wst)                       */
/*                                                                           */
/*  Free wst.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void NrcWorkerSetTreeFree(NRC_WORKER_SET_TREE wst)
{
  NRC_WORKER_SET_TREE child_wst;  int i;
  HaArrayForEach(wst->children, child_wst, i)
    NrcWorkerSetTreeFree(child_wst);
  MArrayFree(wst->children);
  MArrayFree(wst->worker_sets);
  MFree(wst);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET FirstWorkerSet(NRC_WORKER_SET_TREE wst)                   */
/*                                                                           */
/*  Return the first worker-set of wst.                                      */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_SET FirstWorkerSet(NRC_WORKER_SET_TREE wst)
{
  return HaArrayFirst(wst->worker_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetTreeAddWorkerSet(NRC_WORKER_SET_TREE wst,               */
/*    NRC_WORKER_SET ws, NRC_WORKER_SET *incompatible_ws)                    */
/*                                                                           */
/*  Insert ws into wst, returning true and setting *incompatible_ws to NULL  */
/*  if successful, and returning false and setting *incompatible_ws to a     */
/*  previously inserted but incompatible worker-set otherwise.               */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetTreeAddWorkerSet(NRC_WORKER_SET_TREE wst,
  NRC_WORKER_SET ws, NRC_WORKER_SET *incompatible_ws)
{
  NRC_WORKER_SET_TREE child_wst, res;  int i;  bool bres;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcWorkerSetTreeAddWorkerSet(<%s>, %s, -)\n",
      NrcWorkerSetName(FirstWorkerSet(wst)), NrcWorkerSetName(ws));

  /* insert into root if equal */
  if( NrcWorkerSetEqual(ws, FirstWorkerSet(wst)) )
  {
    HaArrayAddLast(wst->worker_sets, ws);
    *incompatible_ws = NULL;
    if( DEBUG1 )
      fprintf(stderr,
	"] NrcWorkerSetTreeAddWorkerSet returning true, equals %s\n",
	NrcWorkerSetName(FirstWorkerSet(wst)));
    return true;
  }

  /* insert into children if subset of any */
  HaArrayForEach(wst->children, child_wst, i)
    if( NrcWorkerSetSubset(ws, FirstWorkerSet(child_wst)) )
    {
      bres = NrcWorkerSetTreeAddWorkerSet(child_wst, ws, incompatible_ws);
      if( DEBUG1 )
	fprintf(stderr,
	  "] NrcWorkerSetTreeAddWorkerSet returning %s from subtree <%s>\n",
	  bres ? "true" : "false", NrcWorkerSetName(FirstWorkerSet(child_wst)));
      return bres;
    }

  /* must be either a superset of or disjoint from each child */
  HaArrayForEach(wst->children, child_wst, i)
    if( !NrcWorkerSetSubset(FirstWorkerSet(child_wst), ws) &&
	!NrcWorkerSetDisjoint(FirstWorkerSet(child_wst), ws) )
    {
      *incompatible_ws = FirstWorkerSet(child_wst);
      if( DEBUG1 )
	fprintf(stderr,
	  "] NrcWorkerSetTreeAddWorkerSet returning false, incompatible %s\n",
	  NrcWorkerSetName(*incompatible_ws));
      return false;
    }

  /* make it a child of wst, whose own children are the lower ones */
  res = NrcWorkerSetTreeDoMake(NrcWorkerSetInstance(ws));
  HaArrayAddLast(res->worker_sets, ws);
  HaArrayForEach(wst->children, child_wst, i)
    if( NrcWorkerSetSubset(FirstWorkerSet(child_wst), ws) )
    {
      if( DEBUG1 )
	fprintf(stderr, "  moving <%s> to under <%s>\n",
          NrcWorkerSetName(FirstWorkerSet(child_wst)), NrcWorkerSetName(ws));
      HaArrayAddLast(res->children, child_wst);
      HaArrayDeleteAndShift(wst->children, i);
      i--;
    }
  HaArrayAddLast(wst->children, res);
  *incompatible_ws = NULL;
  if( DEBUG1 )
    fprintf(stderr, "] NrcWorkerSetTreeAddWorkerSet returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetTreeRootWorkerSetCount(NRC_WORKER_SET_TREE wst)          */
/*                                                                           */
/*  Return the number of worker-sets in the root of wst.                     */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetTreeRootWorkerSetCount(NRC_WORKER_SET_TREE wst)
{
  return HaArrayCount(wst->worker_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcWorkerSetTreeRootWorkerSet(NRC_WORKER_SET_TREE wst,    */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th worker-set of the root of wst.                           */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcWorkerSetTreeRootWorkerSet(NRC_WORKER_SET_TREE wst, int i)
{
  return HaArray(wst->worker_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetTreeChildCount(NRC_WORKER_SET_TREE wst)                  */
/*                                                                           */
/*  Return the number of children of wst.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetTreeChildCount(NRC_WORKER_SET_TREE wst)
{
  return HaArrayCount(wst->children);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_TREE NrcWorkerSetTreeChild(NRC_WORKER_SET_TREE wst, int i)*/
/*                                                                           */
/*  Return the i'th child of wst.                                            */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_TREE NrcWorkerSetTreeChild(NRC_WORKER_SET_TREE wst, int i)
{
  return HaArray(wst->children, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetTreeDebug(NRC_WORKER_SET_TREE wst, int indent,          */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of wst onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetTreeDebug(NRC_WORKER_SET_TREE wst, int indent,
  FILE *fp)
{
  NRC_WORKER_SET ws;  int i;  NRC_WORKER_SET_TREE child_wst;
  if( HaArrayCount(wst->children) > 0 )
  {
    fprintf(fp, "%*s[ Worker-Set Tree (", indent, "");
    HaArrayForEach(wst->worker_sets, ws, i)
      fprintf(fp, "%s%s", i == 0 ? "" : ", ", NrcWorkerSetName(ws));
    fprintf(fp, ")\n");
    HaArrayForEach(wst->children, child_wst, i)
      NrcWorkerSetTreeDebug(child_wst, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "%*sWorker-Set Tree (", indent, "");
    HaArrayForEach(wst->worker_sets, ws, i)
      fprintf(fp, "%s%s", i == 0 ? "" : ", ", NrcWorkerSetName(ws));
    fprintf(fp, ")\n");
  }
}

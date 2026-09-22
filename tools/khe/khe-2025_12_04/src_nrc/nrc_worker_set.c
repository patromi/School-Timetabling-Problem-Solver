
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
/*  FILE:         nrc_worker_set.c                                           */
/*  MODULE:       A set of workers                                           */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET                                                           */
/*                                                                           */
/*  A set of workers.                                                        */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER) ARRAY_NRC_WORKER;

struct nrc_worker_set_rec {
  NRC_INSTANCE		instance;
  char			*name;
  ARRAY_NRC_WORKER	workers;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcWorkerSetMake(NRC_INSTANCE ins, char *name)            */
/*                                                                           */
/*  Return a new worker-set.                                                 */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcWorkerSetMake(NRC_INSTANCE ins, char *name)
{
  NRC_WORKER_SET res;  HA_ARENA a;
  HnAssert(name != NULL, "NrcWorkerSetMake: name is NULL");
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  HaArrayInit(res->workers, a);
  NrcInstanceAddWorkerSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcWorkerSetInstance(NRC_WORKER_SET ws)                     */
/*                                                                           */
/*  Return the instance of ws.                                               */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcWorkerSetInstance(NRC_WORKER_SET ws)
{
  return ws->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcWorkerSetName(NRC_WORKER_SET ws)                                */
/*                                                                           */
/*  Return the name of ws.                                                   */
/*                                                                           */
/*****************************************************************************/

char *NrcWorkerSetName(NRC_WORKER_SET ws)
{
  return ws->name;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetAddWorker(NRC_WORKER_SET ws, NRC_WORKER w)              */
/*                                                                           */
/*  Add w to ws, ensuring that the workers are sorted by index.              */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetAddWorker(NRC_WORKER_SET ws, NRC_WORKER w)
{
  int i;  NRC_WORKER w2;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcWorkerSetAddWorker(%p %s, %s)\n",
      (void *) ws, ws->name != NULL ? ws->name : "-", NrcWorkerName(w));
  HaArrayAddLast(ws->workers, NULL);
  for( i = HaArrayCount(ws->workers) - 2;  i >= 0;  i-- )
  {
    w2 = HaArray(ws->workers, i);
    if( NrcWorkerIndex(w2) < NrcWorkerIndex(w) )
      break;
    HaArrayPut(ws->workers, i + 1, w2);
  }
  HaArrayPut(ws->workers, i + 1, w);
  if( DEBUG1 )
    fprintf(stderr, "] NrcWorkerSetAddWorker\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetAddWorkerSet(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)    */
/*                                                                           */
/*  Add ws2 to ws1.                                                          */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetAddWorkerSet(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)
{
  int i;  NRC_WORKER w;
  HnAssert(ws1 != ws2, "NrcWorkerSetAddWorkerSet: ws1 == ws2");
  HaArrayForEach(ws2->workers, w, i)
    NrcWorkerSetAddWorker(ws1, w);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetWorkerCount(NRC_WORKER_SET ws)                           */
/*                                                                           */
/*  Return the number of workers in ws.                                      */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetWorkerCount(NRC_WORKER_SET ws)
{
  return HaArrayCount(ws->workers);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER NrcWorkerSetWorker(NRC_WORKER_SET ws, int i)                  */
/*                                                                           */
/*  Return the i'th worker of ws.                                            */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER NrcWorkerSetWorker(NRC_WORKER_SET ws, int i)
{
  return HaArray(ws->workers, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetContains(NRC_WORKER_SET ws, NRC_WORKER w, int *pos)     */
/*                                                                           */
/*  If ws contains w, set *pos to its index and return true, else set        */
/*  *pos to -1 and return false.                                             */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetContains(NRC_WORKER_SET ws, NRC_WORKER w, int *pos)
{
  if( HaArrayContains(ws->workers, w, pos) )
    return true;
  *pos = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetContainsWorker(NRC_WORKER_SET ws, NRC_WORKER w)         */
/*                                                                           */
/*  Return true if ws contains w.                                            */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetContainsWorker(NRC_WORKER_SET ws, NRC_WORKER w)
{
  int pos;
  return HaArrayContains(ws->workers, w, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetRetrieveWorker(NRC_WORKER_SET ws, char *name,           */
/*    NRC_WORKER *w)                                                         */
/*                                                                           */
/*  Retrieve a worker with the given name from ws.                           */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetRetrieveWorker(NRC_WORKER_SET ws, char *name, NRC_WORKER *w)
{
  NRC_WORKER w2;  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcWorkerSetRetrieveWorker(%p %s, %s, &w)\n",
      (void *) ws, ws->name != NULL ? ws->name : "-", name);
  HaArrayForEach(ws->workers, w2, i)
    if( strcmp(NrcWorkerName(w2), name) == 0 )
    {
      if( DEBUG1 )
	fprintf(stderr, "] NrcWorkerSetRetrieveWorker returning true\n");
      return *w = w2, true;
    }
  if( DEBUG1 )
  {
    fprintf(stderr, "] NrcWorkerSetRetrieveWorker returning false:\n");
    HaArrayForEach(ws->workers, w2, i)
      fprintf(stderr, "    %s\n", NrcWorkerName(w2));
  }
  return *w = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "set operations"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetHasNoDuplicates(NRC_WORKER_SET ws)                      */
/*                                                                           */
/*  Return true if ws has no duplicates.                                     */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetHasNoDuplicates(NRC_WORKER_SET ws)
{
  int i;  NRC_WORKER w, prev_w;
  if( HaArrayCount(ws->workers) <= 1 )
    return true;
  prev_w = HaArrayFirst(ws->workers);
  for( i = 1;  i < HaArrayCount(ws->workers);  i++ )
  {
    w = HaArray(ws->workers, i);
    if( w == prev_w )
      return false;
    prev_w = w;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetEqual(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)           */
/*                                                                           */
/*  Return true if ws1 and ws2 contain the same workers.                     */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetEqual(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)
{
  if( ws1 == ws2 )
    return true;
  if( HaArrayCount(ws1->workers) != HaArrayCount(ws2->workers) )
    return false;
  return NrcWorkerSetSubset(ws1, ws2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetDisjoint(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)        */
/*                                                                           */
/*  Return true if ws1 and ws2 have no workers in common.                    */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetDisjoint(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)
{
  NRC_WORKER w;  int i;
  HaArrayForEach(ws1->workers, w, i)
    if( NrcWorkerSetContainsWorker(ws2, w) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetSubset(NRC_WORKER_SET lower_ws, NRC_WORKER_SET upper_ws)*/
/*                                                                           */
/*  Return true if every worker of lower_ws is a worker of upper_ws.         */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetSubset(NRC_WORKER_SET lower_ws, NRC_WORKER_SET upper_ws)
{
  NRC_WORKER w;  int i;
  HaArrayForEach(lower_ws->workers, w, i)
    if( !NrcWorkerSetContainsWorker(upper_ws, w) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcWorkerSetComplement(NRC_WORKER_SET ws)                 */
/*                                                                           */
/*  Return the complement of ws.                                             */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcWorkerSetComplement(NRC_WORKER_SET ws)
{
  NRC_INSTANCE ins;  char *cname;  NRC_WORKER_SET all_ws, res;
  NRC_WORKER w;  int i, j, prev_w_index, w_index;
  ins = NrcWorkerSetInstance(ws);
  if( NrcWorkerSetWorkerCount(ws) == 0 )
    res = NrcInstanceStaffing(ins);
  else if( NrcWorkerSetWorkerCount(ws) == NrcInstanceStaffingWorkerCount(ins) )
    res = NrcInstanceEmptyWorkerSet(ins);
  else if( ws->name[0] != '!' )
  {
    cname = HnStringMake(NrcInstanceArena(ins), "!%s", ws->name);
    if( !NrcInstanceRetrieveWorkerSet(ins, cname, &res) )
    {
      /* build complement worker set with name cname */
      res = NrcWorkerSetMake(ins, cname);
      all_ws = NrcInstanceStaffing(ins);
      prev_w_index = -1;
      HaArrayForEach(ws->workers, w, i)
      {
	w_index = NrcWorkerIndex(w);
	for( j = prev_w_index + 1;  j < w_index;  j++ )
	  NrcWorkerSetAddWorker(res, HaArray(all_ws->workers, j));
	prev_w_index = w_index;
      }
      for( j = prev_w_index + 1;  j < HaArrayCount(all_ws->workers);  j++ )
	NrcWorkerSetAddWorker(res, HaArray(all_ws->workers, j));
    }
  }
  else
  {
    cname = &ws->name[1];
    if( !NrcInstanceRetrieveWorkerSet(ins, cname, &res) )
      HnAbort("NrcWorkerSetComplement: no complement to %s", ws->name);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetTypedCmp(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)         */
/*                                                                           */
/*  Comparison function for bringing equal worker sets together.             */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetTypedCmp(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2)
{
  int i, cmp;  NRC_WORKER w1, w2;
  if( ws1 != ws2 )
  {
    cmp = HaArrayCount(ws1->workers) - HaArrayCount(ws2->workers);
    if( cmp != 0 )  return cmp;
    for( i = 0;  i < HaArrayCount(ws1->workers);  i++ )
    {
      w1 = HaArray(ws1->workers, i);
      w2 = HaArray(ws2->workers, i);
      if( w1 != w2 )
	return NrcWorkerIndex(w1) - NrcWorkerIndex(w2);
    }
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Untyped worker set comparison function.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetCmp(const void *t1, const void *t2)
{
  NRC_WORKER_SET ws1 = * (NRC_WORKER_SET *) t1;
  NRC_WORKER_SET ws2 = * (NRC_WORKER_SET *) t2;
  return NrcWorkerSetTypedCmp(ws1, ws2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP NrcWorkerSetResourceGroup(NRC_WORKER_SET ws,          */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Return a KHE resource group corresponding to ws.                         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP NrcWorkerSetResourceGroup(NRC_WORKER_SET ws,
  KHE_INSTANCE ins)
{
  KHE_RESOURCE_TYPE rt;  NRC_WORKER w;  int i;  KHE_RESOURCE_GROUP res;
  if( !KheInstanceRetrieveResourceGroup(ins, ws->name, &res) )
  {
    rt = KheInstanceResourceType(ins, 0);
    if( !KheResourceGroupMake(rt, ws->name, ws->name, false, &res) )
      HnAbort("NrcWorkerSetResourceGroup internal error (%s)", ws->name);
    HaArrayForEach(ws->workers, w, i)
      KheResourceGroupAddResource(res, NrcWorkerResource(w));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetDebug(NRC_WORKER_SET ws, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug print of ws onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetDebug(NRC_WORKER_SET ws, int indent, FILE *fp)
{
  NRC_WORKER w;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*sWorkerSet(%s) = {", indent, "", ws->name);
    HaArrayForEach(ws->workers, w, i)
      fprintf(fp, "%s%s", i == 0 ? "" : ", ", NrcWorkerName(w));
    fprintf(fp, "}\n");
  }
  else
    fprintf(fp, "%s", ws->name);
}

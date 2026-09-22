
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
/*  FILE:         nrc_worker_set_set.c                                       */
/*  MODULE:       A set of worker-sets                                       */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_SET                                                       */
/*                                                                           */
/*  A set of worker-sets                                                     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER_SET) ARRAY_NRC_WORKER_SET;

struct nrc_worker_set_set_rec {
  NRC_INSTANCE		instance;
  ARRAY_NRC_WORKER_SET	worker_sets;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET_SET NrcWorkerSetSetMake(NRC_INSTANCE ins)                 */
/*                                                                           */
/*  Make a new worker-set set, initially with no worker-sets.                */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET_SET NrcWorkerSetSetMake(NRC_INSTANCE ins)
{
  NRC_WORKER_SET_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  HaArrayInit(res->worker_sets, a);
  NrcInstanceAddWorkerSetSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetSetAddWorkerSet(NRC_WORKER_SET_SET wss,                 */
/*    NRC_WORKER_SET ws)                                                     */
/*                                                                           */
/*  Add ws to wss.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetSetAddWorkerSet(NRC_WORKER_SET_SET wss, NRC_WORKER_SET ws)
{
  HaArrayAddLast(wss->worker_sets, ws);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerSetSetWorkerSetCount(NRC_WORKER_SET_SET wss)                */
/*                                                                           */
/*  Return the number of worker-sets in wss.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcWorkerSetSetWorkerSetCount(NRC_WORKER_SET_SET wss)
{
  return HaArrayCount(wss->worker_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcWorkerSetSetWorkerSet(NRC_WORKER_SET_SET wss, int i)   */
/*                                                                           */
/*  Return the i'th worker-set of wss.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcWorkerSetSetWorkerSet(NRC_WORKER_SET_SET wss, int i)
{
  return HaArray(wss->worker_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcWorkerSetSetRetrieveWorkerSet(NRC_WORKER_SET_SET wss,            */
/*    char *name, NRC_WORKER_SET *ws)                                        */
/*                                                                           */
/*  Retrieve a worker-set with the given name from wss.                      */
/*                                                                           */
/*****************************************************************************/

bool NrcWorkerSetSetRetrieveWorkerSet(NRC_WORKER_SET_SET wss,
  char *name, NRC_WORKER_SET *ws)
{
  NRC_WORKER_SET ws2;  int i;
  HaArrayForEach(wss->worker_sets, ws2, i)
    if( strcmp(NrcWorkerSetName(ws2), name) == 0 )
    {
      *ws = ws2;
      return true;
    }
  *ws = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerSetSetDebug(NRC_WORKER_SET_SET wss, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of wss.                                                      */
/*                                                                           */
/*****************************************************************************/

void NrcWorkerSetSetDebug(NRC_WORKER_SET_SET wss, int indent, FILE *fp)
{
  NRC_WORKER_SET ws;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ WorkerSetSet:\n", indent, "");
    HaArrayForEach(wss->worker_sets, ws, i)
      NrcWorkerSetDebug(ws, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "{%d worker-sets}", HaArrayCount(wss->worker_sets));
}

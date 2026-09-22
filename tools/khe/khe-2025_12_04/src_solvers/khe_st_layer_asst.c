
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
/*  FILE:         khe_st_layer_asst.c                                        */
/*  DESCRIPTION:  A record of an assignment to one layer                     */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_ASST - record of one layer assignment                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_meet_asst_rec {
  KHE_MEET			meet;		/* the meet                  */
  KHE_MEET			target_meet;	/* target meet, if assigned  */
  int				offset;		/* offset, if assigned       */
} KHE_MEET_ASST;

typedef HA_ARRAY(KHE_MEET_ASST) ARRAY_KHE_MEET_ASST;

struct khe_layer_asst_rec {
  HA_ARENA		arena;
  KHE_SOLN		soln;
  KHE_LAYER		layer;			/* the layer                 */
  ARRAY_KHE_MEET_ASST	meet_assts;		/* the meet assignments      */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_ASST KheLayerAsstMake(KHE_SOLN soln)                           */
/*                                                                           */
/*  Make a new, empty layer assignment object.                               */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER_ASST KheLayerAsstMake(KHE_SOLN soln)
{
  KHE_LAYER_ASST res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  /* a = HaAre naMake(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->layer = NULL;
  HaArrayInit(res->meet_assts, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstDelete(KHE_LAYER_ASST layer_asst)                       */
/*                                                                           */
/*  Delete layer_asst.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstDelete(KHE_LAYER_ASST layer_asst)
{
  KheSolnArenaEnd(layer_asst->soln, layer_asst->arena);
  /* HaAren aDelete(layer_asst->arena); */
  /* ***
  MArrayFree(layer_asst->meet_assts);
  MFree(layer_asst);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstBegin(KHE_LAYER_ASST layer_asst, KHE_LAYER layer)       */
/*                                                                           */
/*  Begin recording the assignments to layer.                                */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstBegin(KHE_LAYER_ASST layer_asst, KHE_LAYER layer)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;  KHE_MEET_ASST meet_asst;
  layer_asst->layer = layer;
  HaArrayClear(layer_asst->meet_assts);
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) == NULL )
      {
	meet_asst.meet = meet;
	meet_asst.target_meet = NULL;
	meet_asst.offset = -1;
	HaArrayAddLast(layer_asst->meet_assts, meet_asst);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstEnd(KHE_LAYER_ASST layer_asst)                          */
/*                                                                           */
/*  End recording the assignments to layer.                                  */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstEnd(KHE_LAYER_ASST layer_asst)
{
  KHE_MEET_ASST meet_asst;  int i;
  HaArrayForEach(layer_asst->meet_assts, meet_asst, i)
  {
    meet_asst.target_meet = KheMeetAsst(meet_asst.meet);
    meet_asst.offset = KheMeetAsstOffset(meet_asst.meet);
    HaArrayPut(layer_asst->meet_assts, i, meet_asst);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstUndo(KHE_LAYER_ASST layer_asst)                         */
/*                                                                           */
/*  Undo the assignments recorded in layer_asst.                             */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstUndo(KHE_LAYER_ASST layer_asst)
{
  KHE_MEET_ASST meet_asst;  int i;
  HaArrayForEach(layer_asst->meet_assts, meet_asst, i)
    if( meet_asst.target_meet != NULL )
      if( !KheMeetUnAssign(meet_asst.meet) )  /* OK; undoes previous assts */
	HnAbort("KheLayerAsstUndo: KheMeetUnAssign failed");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstRedo(KHE_LAYER_ASST layer_asst)                         */
/*                                                                           */
/*  Redo the assignments recorded in layer_asst.                             */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstRedo(KHE_LAYER_ASST layer_asst)
{
  KHE_MEET_ASST meet_asst;  int i;
  HaArrayForEach(layer_asst->meet_assts, meet_asst, i)
    if( meet_asst.target_meet != NULL )
      if( !KheMeetAssign(meet_asst.meet,meet_asst.target_meet,meet_asst.offset))
        HnAbort("KheLayerAsstRedo: KheMeetAssign failed");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAsstDebug(KHE_LAYER_ASST layer_asst, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of layer_asst onto fp with the given verbosity and indent.   */
/*                                                                           */
/*****************************************************************************/

void KheLayerAsstDebug(KHE_LAYER_ASST layer_asst, int verbosity,
  int indent, FILE *fp)
{
  KHE_MEET_ASST meet_asst;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ LayerAsst ", indent, "");
    if( layer_asst->layer == NULL )
      fprintf(fp, " (no layer)\n");
    else
      KheLayerDebug(layer_asst->layer, 1, 0, fp);
    HaArrayForEach(layer_asst->meet_assts, meet_asst, i)
    {
      fprintf(fp, "%*s", indent + 2, "");
      KheMeetDebug(meet_asst.meet, 1, -1, fp);
      if( meet_asst.target_meet != NULL )
      {
	fprintf(fp, " --%d-> ", meet_asst.offset);
	KheMeetDebug(meet_asst.target_meet, 1, -1, fp);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}

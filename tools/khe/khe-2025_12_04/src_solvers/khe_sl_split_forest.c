
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
/*  FILE:         khe_sl_split_forest.c                                      */
/*  DESCRIPTION:  A forest of equivalence classes for KheSplitAndLink()      */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_FOREST                                                         */
/*                                                                           */
/*  Miscellaneous global information needed when splitting and linking.      */
/*                                                                           */
/*****************************************************************************/

struct khe_split_forest_rec {
  KHE_SOLN			soln;			/* solution          */
  KHE_SPLIT_CLASS		cycle_class;		/* cycle split class */
  ARRAY_KHE_SPLIT_LAYER		split_layers;		/* all split layers  */
  ARRAY_KHE_SPLIT_CLASS		split_classes_by_event;	/* indexed by event  */
};


/*****************************************************************************/
/*                                                                           */
/*  Partitions                                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_FOREST KheSplitForestMake(KHE_SOLN soln, HA_ARENA a)           */
/*                                                                           */
/*  Make a split forest object with these attributes.                        */
/*  Arena a is for scratch memory.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_FOREST KheSplitForestMake(KHE_SOLN soln, HA_ARENA a)
{
  KHE_SPLIT_FOREST res;  KHE_SPLIT_CLASS sc;
  KHE_INSTANCE ins;  int i, class_id_num;  KHE_EVENT e;

  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheSplitForestMake(soln to %s)\n",
      KheInstanceId(ins) != NULL ? KheInstanceId(ins) : "-");

  /* make the result object and give its fields a basic initialization */
  HaMake(res, a);
  res->soln = soln;
  HaArrayInit(res->split_layers, a);
  HaArrayInit(res->split_classes_by_event, a);

  /* one class for the cycle layer, and one for each instance event */
  class_id_num = 0;
  res->cycle_class = KheSplitClassMake(res, class_id_num++, NULL, a);
  /* ***
  res->cycle_class = KheSplitClassCycleLayerMake(res, class_id_num++,
    KheSolnCycleLayer(soln));
  *** */
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    sc = KheSplitClassMake(res, class_id_num++, e, a);
    KheSplitClassAddChildClass(res->cycle_class, sc, a);
    HaArrayAddLast(res->split_classes_by_event, sc);
  }

  if( DEBUG1 )
  {
    KheSplitForestDebug(res, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitForestFree(KHE_SPLIT_FOREST sf)                             */
/*                                                                           */
/*  Free the memory used by sf and everything in it.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitForestFree(KHE_SPLIT_FOREST sf)
{
  KHE_SPLIT_LAYER split_layer;  int i;
  MArrayForEach(sf->split_layers, &split_layer, &i)
    KheSplitLayerFree(split_layer);
  KheSplitClassFree(sf->cycle_class, true);
  MArrayFree(sf->split_layers);
  MArrayFree(sf->split_classes_by_event);
  MFree(sf);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSplitForestSoln(KHE_SPLIT_FOREST sf)                         */
/*                                                                           */
/*  Return the soln attribute of sf.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSplitForestSoln(KHE_SPLIT_FOREST sf)
{
  return sf->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "trials"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_CLASS KheSplitForestSplitClass(KHE_SPLIT_FOREST sf,            */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Return the split class containing e.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_SPLIT_CLASS KheSplitForestSplitClass(KHE_SPLIT_FOREST sf,
  KHE_EVENT e)
{
  return HaArray(sf->split_classes_by_event, KheEventIndex(e));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryAddToLayer(KHE_SPLIT_FOREST sf,                    */
/*    KHE_EVENT e, KHE_LAYER layer)                                          */
/*                                                                           */
/*  Try to get sf to accept that e is a member of layer layer.               */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheSplitForestTryAddToLayer(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_LAYER layer)
{
  KHE_SPLIT_CLASS sc;  bool res;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryAddToLayer(sf, %s, layer)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-");
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  KheSplitClassTryAddToLayer(sc, layer);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryAddToLayer returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryAddToResource(KHE_SPLIT_FOREST sf, KHE_EVENT e,    */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Try to get sf to accept that e contains preassigned resource r.          */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryAddToResource(KHE_SPLIT_FOREST sf, KHE_EVENT e,
  KHE_RESOURCE r, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res, success;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryAddToResource(sf, %s, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-",
      KheResourceId(r) != NULL ? KheResourceId(r) : "-");
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  if( DEBUG2 )
    fprintf(stderr, "  KheSplitForestTryAddToResource calling "
      "KheSplitClassTryAddToResource:\n");
  KheSplitClassTryAddToResource(sc, r, a);
  if( DEBUG2 )
    fprintf(stderr, "  KheSplitForestTryAddToResource calling "
      "KheSplitClassTryAlive:\n");
  success = KheSplitClassTryAlive(sc, a);
  if( DEBUG2 )
    fprintf(stderr, "  KheSplitForestTryAddToResource calling "
      "KheSplitClassTryEnd:\n");
  res = KheSplitClassTryEnd(sc, success);
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryAddToResource returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryTotalAmount(KHE_SPLIT_FOREST sf,                   */
/*    KHE_EVENT e, int minimum, int maximum)                                 */
/*                                                                           */
/*  Try to get sf to accept that the number of sub-events of e will be at    */
/*  least minimum and at most maximum.                                       */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryTotalAmount(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, int minimum, int maximum, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryTotalAmount(sf, %s, %d-%d)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-", minimum, maximum);
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  KheSplitClassTryTotalAmount(sc, minimum, maximum);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryTotalAmount returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryDurnAmount(KHE_SPLIT_FOREST sf,                    */
/*    KHE_EVENT e, int durn, int minimum, int maximum)                       */
/*                                                                           */
/*  Try to get sf to accept that there will be at least minimum and at       */
/*  most maximum sub-events of e of duration durn.                           */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryDurnAmount(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, int durn, int minimum, int maximum, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryDurnAmount(sf, %s, %d:%d-%d)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-", durn, minimum, maximum);
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  KheSplitClassTryDurnAmount(sc, durn, minimum, maximum);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryDurnAmount returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryEventMerge(KHE_SPLIT_FOREST sf,                    */
/*    KHE_EVENT e1, KHE_EVENT e2)                                            */
/*                                                                           */
/*  Try to get sf to accept the merging of e1 and e2.                        */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryEventMerge(KHE_SPLIT_FOREST sf,
  KHE_EVENT e1, KHE_EVENT e2, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc1, sc2, tmp;  bool res;  int i;  KHE_EVENT e;

  if( DEBUG2 )
    fprintf(stderr, "[ KheSplitForestTryEventMerge(sf, %s, %s)\n",
    KheEventId(e1) != NULL ? KheEventId(e1) : "-",
    KheEventId(e2) != NULL ? KheEventId(e2) : "-");
  HnAssert(KheEventDuration(e1) == KheEventDuration(e2),
   "KheSplitForestTryEventMerge internal error 0");

  /* do nothing but succeed when e1 and e2 already lie in the same class */
  sc1 = KheSplitForestSplitClass(sf, e1);
  sc2 = KheSplitForestSplitClass(sf, e2);
  if( sc1 == sc2 )
  {
    if( DEBUG2 )
      fprintf(stderr,"] KheSplitForestTryEventMerge returning success: same\n");
    return true;
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial classes:\n");
    KheSplitClassDebug(sc1, 2, stderr, a);
    KheSplitClassDebug(sc2, 2, stderr, a);
  }

  /* make sure sc2's parent is an ancestor of sc1 */
  HnAssert(KheSplitClassParentClass(sc1) != NULL,
    "KheSplitForestTryEventMerge internal error 1");
  HnAssert(KheSplitClassParentClass(sc2) != NULL,
    "KheSplitForestTryEventMerge internal error 2");
  if( KheSplitClassAncestor(sc1, KheSplitClassParentClass(sc2)) )
  {
    /* OK as is, we "preserve" sc2's parent link when we merge it with sc1 */
  }
  else if( KheSplitClassAncestor(sc2, KheSplitClassParentClass(sc1)) )
  {
    /* swapping sc1 and sc2 gives us the previous case */
    tmp = sc1, sc1 = sc2, sc2 = tmp;
  }
  else
  {
    if( DEBUG2 )
      fprintf(stderr,"] KheSplitForestTryEventMerge returning failure: tree\n");
    return false;
  }

  /* now we're ready to try merging sc2 into sc1 */
  KheSplitClassTryBegin(sc1);
  KheSplitClassTryBegin(sc2);
  KheSplitClassTryMerge(sc1, sc2);
  res = KheSplitClassTryAlive(sc1, a);
  if( res )
  {
    KheSplitClassTryEnd(sc1, true);
    /* KheSplitClassFree(sc2, false); */
    for( i = 0;  i < KheSplitClassEventCount(sc1);  i++ )
    {
      e = KheSplitClassEvent(sc1, i);
      HaArrayPut(sf->split_classes_by_event, KheEventIndex(e), sc1);
    }
    if( DEBUG2 )
    {
      fprintf(stderr, "  final class:\n");
      KheSplitClassDebug(sc1, 2, stderr, a);
    }
  }
  else
  {
    KheSplitClassTryEnd(sc1, false);
    KheSplitClassTryEnd(sc2, false);
    if( DEBUG2 )
    {
      fprintf(stderr, "  final classes:\n");
      KheSplitClassDebug(sc1, 2, stderr, a);
      KheSplitClassDebug(sc2, 2, stderr, a);
    }
  }

  if( DEBUG2 )
    fprintf(stderr, "] KheSplitForestTryEventMerge returning %s\n",
      res ? "success" : "failure");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryEventAssign(KHE_SPLIT_FOREST sf,                   */
/*    KHE_EVENT e1, KHE_EVENT e2)                                            */
/*                                                                           */
/*  Try to get sf to accept the assignment of e1 to e2.                      */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryEventAssign(KHE_SPLIT_FOREST sf,
  KHE_EVENT e1, KHE_EVENT e2, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc1, sc2;  bool res;

  /* quietly ignore hopeless cases */
  if( KheEventDuration(e1) > KheEventDuration(e2) )
    return false;

  if( DEBUG2 )
    fprintf(stderr, "[ KheSplitForestTryEventAssign(sf, %s, %s)\n",
      KheEventId(e1) != NULL ? KheEventId(e1) : "-",
      KheEventId(e2) != NULL ? KheEventId(e2) : "-");

  /* if same class, do nothing but consider it a success */
  sc1 = KheSplitForestSplitClass(sf, e1);
  sc2 = KheSplitForestSplitClass(sf, e2);
  HnAssert(KheSplitClassParentClass(sc1) != NULL,
    "KheSplitForestTryEventAssign internal error 1");
  HnAssert(KheSplitClassParentClass(sc2) != NULL,
    "KheSplitForestTryEventAssign internal error 2");
  if( sc1 == sc2 )
  {
    if( DEBUG2 )
      fprintf(stderr,
	"] KheSplitForestTryEventAssign returning success: same\n");
    return true;
  }

  /* now we're ready to try assigning sc1 to sc2 */
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial classes:\n");
    KheSplitClassDebug(sc1, 2, stderr, a);
    KheSplitClassDebug(sc2, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc1);
  KheSplitClassTryBegin(sc2);
  KheSplitClassTryAssign(sc1, sc2, a);
  res = KheSplitClassTryAlive(sc1, a) && KheSplitClassTryAlive(sc2, a);
  KheSplitClassTryEnd(sc1, res);
  KheSplitClassTryEnd(sc2, res);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final classes:\n");
    KheSplitClassDebug(sc1, 2, stderr, a);
    KheSplitClassDebug(sc2, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryEventAssign returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryPackableInto(KHE_SPLIT_FOREST sf,                  */
/*    KHE_EVENT e, KHE_PARTITION p)                                          */
/*                                                                           */
/*  Try to get sf to accept that the solution events of e should be          */
/*  packable into p.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryPackableInto(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_PARTITION p, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryPackableInto(sf, %s, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-", KhePartitionShow(p, a));
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  KheSplitClassTryPackableInto(sc, p);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryPackableInto returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryEventDomain(KHE_SPLIT_FOREST sf,                   */
/*    KHE_EVENT e, KHE_TIME_GROUP tg, int durn)                              */
/*                                                                           */
/*  Try to get sf to accept the restriction of the domains of sub-events     */
/*  of e of duration durn to tg.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryEventDomain(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_TIME_GROUP tg, int durn, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryEventDomain(sf, %s, tg, %d)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-", durn);
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  KheSplitClassTryEventDomain(sc, tg, durn);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryEventDomain returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTryPreassignedTime(KHE_SPLIT_FOREST sf, KHE_EVENT e,  */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Try to get sf to accept that e has preassigned time t.                   */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTryPreassignedTime(KHE_SPLIT_FOREST sf, KHE_EVENT e,
  KHE_TIME t, HA_ARENA a)
{
  KHE_SPLIT_CLASS sc;  bool res;  int d;
  sc = KheSplitForestSplitClass(sf, e);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTryPreassignedTime(sf, %s, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-",
      KheTimeId(t) != NULL ? KheTimeId(t) : "-");
    KheSplitClassDebug(sc, 2, stderr, a);
  }
  KheSplitClassTryBegin(sc);
  for( d = 1;  d <= KheEventDuration(e);  d++ )
    KheSplitClassTryEventDomain(sc, KheTimeSingletonTimeGroup(t), d);
  KheSplitClassTryUnsplittable(sc);
  res = KheSplitClassTryEnd(sc, KheSplitClassTryAlive(sc, a));
  if( DEBUG2 )
  {
    KheSplitClassDebug(sc, 2, stderr, a);
    fprintf(stderr, "] KheSplitForestTryPreassignedTime returning %s\n",
      res ? "success" : "failure");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitForestTrySpread(KHE_SPLIT_FOREST sf,                        */
/*    KHE_EVENT_GROUP eg, int min_amount, int max_amount)                    */
/*                                                                           */
/*  Try to get sf to accept that the total number of sub-events of the       */
/*  events of eg should be at least min_amount and at most max_amount.       */
/*                                                                           */
/*  Return true if completely successful; there may be partial success too.  */
/*                                                                           */
/*****************************************************************************/

bool KheSplitForestTrySpread(KHE_SPLIT_FOREST sf,
  KHE_EVENT_GROUP eg, int min_amount, int max_amount, HA_ARENA a)
{
  int i, curr_min, curr_max, sc_min, sc_max;
  KHE_SPLIT_CLASS sc;  KHE_EVENT e;  bool progressing;

  /* find the current min and max amounts across eg */
  curr_min = curr_max = 0;
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    sc = KheSplitForestSplitClass(sf, e);
    curr_min += KheSplitClassMinTotalAmount(sc);
    curr_max += KheSplitClassMaxTotalAmount(sc);
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitForestTrySpread(sf, %s, %d-%d)\n",
      KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "-",
      min_amount, max_amount);
    fprintf(stderr, "  initial curr_min %d, curr_max %d\n", curr_min, curr_max);
  }

  /* try to increase curr_min to min_amount, round-robin */
  progressing = true;
  while( progressing && curr_min < min_amount )
  {
    progressing = false;
    for( i = 0; i < KheEventGroupEventCount(eg) && curr_min < min_amount; i++ )
    {
      e = KheEventGroupEvent(eg, i);
      sc = KheSplitForestSplitClass(sf, e);
      sc_min = KheSplitClassMinTotalAmount(sc);
      sc_max = KheSplitClassMaxTotalAmount(sc);
      if( sc_min < sc_max &&
	  KheSplitForestTryTotalAmount(sf, e, sc_min + 1, sc_max, a) )
	curr_min++, progressing = true;
    }
  }

  /* try to decrease curr_max to max_amount, round-robin */
  progressing = true;
  while( progressing && curr_max > max_amount )
  {
    progressing = false;
    for( i = 0; i < KheEventGroupEventCount(eg) && curr_max > max_amount; i++ )
    {
      e = KheEventGroupEvent(eg, i);
      sc = KheSplitForestSplitClass(sf, e);
      sc_min = KheSplitClassMinTotalAmount(sc);
      sc_max = KheSplitClassMaxTotalAmount(sc);
      if( sc_min < sc_max &&
	  KheSplitForestTryTotalAmount(sf, e, sc_min, sc_max - 1, a) )
	curr_max--, progressing = true;
    }
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "  final curr_min %d, curr_max %d\n", curr_min, curr_max);
    fprintf(stderr, "] KheSplitForestTrySpread returning %s\n",
      curr_min >= min_amount && curr_max <= max_amount ? "success" : "failure");
  }
  return curr_min >= min_amount && curr_max <= max_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "finalize"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitLayerContainsAllMeets(KHE_SOLN soln,                        */
/*    KHE_LAYER layer, KHE_EVENT e)                                          */
/*                                                                           */
/*  Return true if layer contains all the solution events of e.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheSplitLayerContainsAllMeets(KHE_SOLN soln,
  KHE_LAYER layer, KHE_EVENT e)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( !KheLayerContainsMeet(layer, meet) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitMeetsHaveCommonLayer(KHE_SOLN soln, KHE_EVENT e)            */
/*                                                                           */
/*  Return true if there is a layer such that all the solution events of     */
/*  e lie in that layer.                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheSplitMeetsHaveCommonLayer(KHE_SOLN soln, KHE_EVENT e)
{
  int i;  KHE_MEET first_meet;  KHE_LAYER layer;

  ** vacuously true if no solution events; but this should not happen **
  if( KheEventMeetCount(soln, e) == 0 )
    return true;

  ** see whether any layer from the first soln event lies in all the others **
  first_meet = KheEventMeet(soln, e, 0);
  for( i = 0;  i < KheMeetLayerCount(first_meet);  i++ )
  {
    layer = KheMeetLayer(first_meet, i);
    if( KheSplitLayerContainsAllMeets(soln, layer, e) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitMakeCommonLayer(KHE_SOLN soln, KHE_EVENT e,                 */
/*    ARRAY_KHE_LAYER *tmp_layers)                                           */
/*                                                                           */
/*  Ensure that the solution events of e share a layer, as far as possible.  */
/*  Parameter *tmp_layers is a scratch variable.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitMakeCommonLayer(KHE_SOLN soln, KHE_EVENT e,
  ARRAY_KHE_LAYER *tmp_layers)
{
  int i, j;  KHE_MEET meet;  KHE_LAYER layer;
  MArrayClear(*tmp_layers);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    ** add meet to the first layer of tmp_layers that will accept it **
    meet = KheEventMeet(soln, e, i);
    for( j = 0;  j < MArraySize(*tmp_layers);  j++ )
    {
      layer = HaArray(*tmp_layers, j);
      if( KheLayerAddMeet(layer, meet) )
	break;
    }

    ** if no layer accepted meet, make a new layer and add it to that **
    if( j >= MArraySize(*tmp_layers) )
    {
      layer = KheLayerMake(soln, NULL);
      MArrayAddLast(*tmp_layers, layer);
      if( !KheLayerAddMeet(layer, meet) )
	HnAbort("KheSplitMakeCommonLayer internal error");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetsTriviallyAssignable(KHE_NODE child_node,                */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Return true if the solution events of child_node are trivially           */
/*  assignable to distinct solution events of parent_node.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeMeetsTriviallyAssignable(KHE_NODE child_node,
  KHE_NODE parent_node)
{
  int i;  KHE_MEET child_meet, parent_meet;
  if( DEBUG3 )
    fprintf(stderr,
      "[ KheNodeMeetsTriviallyAssignable(child_node %d, parent_node %d)\n",
      KheNodeSolnIndex(child_node), KheNodeSolnIndex(parent_node));
  if( KheNodeChildCount(child_node) > 0 )
  {
    if( DEBUG3 )
      fprintf(stderr, "] KheNodeMeetsTriviallyAssignable false (1)\n");
    return false;
  }
  if( KheNodeDuration(child_node) != KheNodeDuration(parent_node) )
  {
    if( DEBUG3 )
      fprintf(stderr, "] KheNodeMeetsTriviallyAssignable false (2)\n");
    return false;
  }
  if( KheNodeMeetCount(child_node) != KheNodeMeetCount(parent_node) )
  {
    if( DEBUG3 )
      fprintf(stderr, "] KheNodeMeetsTriviallyAssignable false (3)\n");
    return false;
  }
  for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
  {
    child_meet = KheNodeMeet(child_node, i);
    parent_meet = KheNodeMeet(parent_node, i);
    if( KheMeetAsst(child_meet) != NULL ||
	KheMeetDuration(child_meet) != KheMeetDuration(parent_meet) ||
	!KheMeetAssignCheck(child_meet, parent_meet, 0) )
    {
      if( DEBUG3 )
	fprintf(stderr, "] KheNodeMeetsTriviallyAssignable false (4)\n");
      return false;
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] KheNodeMeetsTriviallyAssignable returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeMeetsTriviallyAssign(KHE_NODE child_node,                    */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Trivially assign child_node's soln events to parent_node's, and fix.     */
/*  KheNodeMeetsTriviallyAssignable(child_node, parent_node) just            */
/*  above is a precondition of this function.                                */
/*                                                                           */
/*****************************************************************************/

static void KheNodeMeetsTriviallyAssign(KHE_NODE child_node,
  KHE_NODE parent_node)
{
  int i;  KHE_MEET child_meet, parent_meet;
  for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
  {
    child_meet = KheNodeMeet(child_node, i);
    parent_meet = KheNodeMeet(parent_node, i);
    KheMeetCheck(child_meet);
    KheMeetCheck(parent_meet);
    if( DEBUG4 )
      fprintf(stderr, "KheNodeMeetsTriviallyAssign calling KheMeetAssign"
	"(%s, %s, 0)\n", KheMeetId(child_meet), KheMeetId(parent_meet));
    if( !KheMeetAssign(child_meet, parent_meet, 0) )
      HnAbort("KheLayerTreeMake internal error A");
    KheMeetAssignFix(child_meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerContainsAllMeetsOfNode(KHE_LAYER layer, KHE_NODE node)      */
/*                                                                           */
/*  Return true if all the solution events of node lie in layer.             */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheLayerContainsAllMeetsOfNode(KHE_LAYER layer, KHE_NODE node)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    if( !KheLayerContainsMeet(layer, meet) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetsHaveCommonLayer(KHE_NODE node)                          */
/*                                                                           */
/*  Return true if there is a layer such that all the solution events of     */
/*  node lie in that layer.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheNodeMeetsHaveCommonLayer(KHE_NODE node)
{
  int i;  KHE_MEET first_meet;  KHE_LAYER layer;

  ** vacuously true if no solution events; but this should not happen **
  if( KheNodeMeetCount(node) == 0 )
    return true;

  ** see whether any layer from the first soln event lies in all the others **
  first_meet = KheNodeMeet(node, 0);
  for( i = 0;  i < KheMeetLayerCount(first_meet);  i++ )
  {
    layer = KheMeetLayer(first_meet, i);
    if( KheLayerContainsAllMeetsOfNode(layer, node) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeMeetsMakeCommonLayer(KHE_NODE node,                          */
/*    ARRAY_KHE_LAYER *tmp_layers)                                           */
/*                                                                           */
/*  Ensure that the soln events of node share a layer, as far as possible.   */
/*  Parameter *tmp_layers is a scratch variable.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheNodeMeetsMakeCommonLayer(KHE_NODE node,
  ARRAY_KHE_LAYER *tmp_layers)
{
  int i, j;  KHE_MEET meet;  KHE_LAYER layer;
  MArrayClear(*tmp_layers);
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    ** add meet to the first layer of tmp_layers that will accept it **
    meet = KheNodeMeet(node, i);
    for( j = 0;  j < MArraySize(*tmp_layers);  j++ )
    {
      layer = HaArray(*tmp_layers, j);
      if( KheLayerAddMeet(layer, meet) )
	break;
    }

    ** if no layer accepted meet, make a new layer and add it to that **
    if( j >= MArraySize(*tmp_layers) )
    {
      layer = KheLayerMake(KheNodeSoln(node), NULL);
      MArrayAddLast(*tmp_layers, layer);
      if( !KheLayerAddMeet(layer, meet) )
	HnAbort("KheSplitMakeCommonLayer internal error");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitForestMakeTrivialAssignmentsOrLayers(KHE_NODE parent_node,  */
/*    ARRAY_KHE_LAYER *tmp_layers)                                           */
/*                                                                           */
/*  Make trivial assignments in the proper descendants of parent_node, or    */
/*  make sure there is a layer if the assignments don't work out.            */
/*  Parameter *tmp_layers is a scratch variable.                             */
/*                                                                           */
/*****************************************************************************/

static void KheSplitForestMakeTrivialAssignmentsOrLayers(KHE_NODE parent_node)
  /* , ARRAY_KHE_LAYER *tmp_layers) */
{
  KHE_NODE child_node;  int i;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    if( KheNodeMeetsTriviallyAssignable(child_node, parent_node) )
    {
      /* assign child solution events to parent soln events and delete child */
      KheNodeMeetsTriviallyAssign(child_node, parent_node);
      if( !KheNodeDelete(child_node) )
	HnAbort("KheSplitForestMakeTrivialAssignmentsOrLayers internal error");
      i--;
    }
    /* ***
    else if( !KheNodeMeetsHaveCommonLayer(child_node) )
    {
      ** soln events of child_node do not have a common layer, so add one **
      KheNodeMeetsMakeCommonLayer(child_node, tmp_layers);
    }
    *** */
    else /* this else is a 15/12/23 bug fix; why was it missing? */
      KheSplitForestMakeTrivialAssignmentsOrLayers(child_node /*,tmp_layers*/);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheSplitForestFinalize(KHE_SPLIT_FOREST sf)                     */
/*                                                                           */
/*  Finalize sf, including splitting events into solution events, assigning  */
/*  them to other events, making nodes, etc.  Return the root node.          */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheSplitForestFinalize(KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  /* ARRAY_KHE_LAYER tmp_layers  int i;  KHE_INSTANCE ins; */
  /* KHE_EVENT e; */  /* KHE_SPLIT_ASSIGNER sa; */  KHE_NODE res;

  /* ensure that the solution events of each event share at least one layer */
  /* ***
  ins = KheSolnInstance(sf->soln);
  MArrayInit(tmp_layers);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( !KheSplitMeetsHaveCommonLayer(sf->soln, e) )
      KheSplitMakeCommonLayer(sf->soln, e, &tmp_layers);
  }
  MArrayFree(tmp_layers);
  *** */

  /* finalize the cycle class and its descendants */
  /* ***
  sa = KheSplitAssignerMake(sf->soln);
  KheSplitCl assFinalize(sf->cycle_class, sa);
  KheSplitAssignerFree(sa);
  *** */
  res = KheSplitClassFinalize(sf->cycle_class, NULL, a);

  /* make trivial assignments or layers as required */

  /* ***
  MArrayInit(tmp_layers);
  KheNodeMeetsMakeCommonLayer(res, &tmp_layers);
  *** */
  KheSplitForestMakeTrivialAssignmentsOrLayers(res /* , &tmp_layers */);
  /* MArrayFree(tmp_layers); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitForestDebug(KHE_SPLIT_FOREST sf, int indent, FILE *fp)      */
/*                                                                           */
/*  Debug print of sf.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheSplitForestDebug(KHE_SPLIT_FOREST sf, int indent, FILE *fp, HA_ARENA a)
{
  KHE_INSTANCE ins;
  ins = KheSolnInstance(sf->soln);
  fprintf(fp, "%*s[ Split Forest (soln to %s)\n", indent, "",
    KheInstanceId(ins) != NULL ? KheInstanceId(ins) : "-");
  KheSplitClassDebug(sf->cycle_class, indent + 2, fp, a);
  fprintf(fp, "%*s]\n", indent, "");
}

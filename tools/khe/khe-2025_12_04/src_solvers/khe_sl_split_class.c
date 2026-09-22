
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
/*  FILE:         khe_sl_split_class.c                                       */
/*  DESCRIPTION:  Equivalence classes used by KheLayerTreeMake()             */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_CLASS - an equivalence class of linked events of equal durn    */
/*                                                                           */
/*  Implementation note.  Every class is always currently reporting one      */
/*  partition to its parent (if it has a parent), namely sc->min_partition.  */
/*                                                                           */
/*****************************************************************************/

struct khe_split_class_rec {

  /* basic structural fields */
  KHE_SPLIT_FOREST		forest;		/* enclosing forest          */
  int				id_num;		/* used for debugging only   */
  KHE_SPLIT_CLASS		parent_class;	/* class above this class    */
  ARRAY_KHE_SPLIT_CLASS		child_classes;	/* classes under this class  */
  ARRAY_KHE_SPLIT_LAYER		child_layers;	/* layers of child classes   */
  ARRAY_KHE_EVENT		events;		/* events in this class      */
  KHE_EVENT			leader;		/* class leader              */
  int				duration;	/* common duration of events */

  /* local requirements */
  int				min_total_amount;
  int				max_total_amount;
  HA_ARRAY_INT			min_amount;
  HA_ARRAY_INT			max_amount;
  ARRAY_KHE_PARTITION		upper_partitions;
  KHE_PARTITION			min_partition;
  ARRAY_KHE_TIME_GROUP		time_groups;

  /* saved copies of fields, used to restore after unsuccessful jobs */
  bool				save_active;
  KHE_SPLIT_CLASS		save_parent_class;
  ARRAY_KHE_SPLIT_CLASS		save_child_classes;
  ARRAY_KHE_SPLIT_LAYER		save_child_layers;
  ARRAY_KHE_EVENT		save_events;
  KHE_EVENT			save_leader;
  int				save_min_total_amount;
  int				save_max_total_amount;
  HA_ARRAY_INT			save_min_amount;
  HA_ARRAY_INT			save_max_amount;
  ARRAY_KHE_PARTITION		save_upper_partitions;
  KHE_PARTITION			save_min_partition;
  ARRAY_KHE_TIME_GROUP		save_time_groups;

  /* temporary variables used when generating to test packability */
  KHE_PARTITION			tmp_stem;
  ARRAY_KHE_PARTITION		tmp_partitions;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_CLASS KheSplitClassMake(KHE_SPLIT_FOREST sf,                   */
/*    int id_num, KHE_EVENT e)                                               */
/*                                                                           */
/*  Make a new split class, initially containing just event e.  Or e may     */
/*  be NULL, in which case we are making a split class for the cycle meets.  */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_CLASS KheSplitClassMake(KHE_SPLIT_FOREST sf, int id_num,
  KHE_EVENT e, HA_ARENA a)
{
  KHE_SPLIT_CLASS res;  KHE_TIME_GROUP full_tg;  KHE_PARTITION p;
  KHE_SOLN soln;  KHE_MEET meet;  int i, d, s, time_count, sz;

  /* basic structural fields */
  soln = KheSplitForestSoln(sf);
  HaMake(res, a);
  res->forest = sf;
  res->id_num = id_num;
  res->parent_class = NULL;
  HaArrayInit(res->child_classes, a);
  HaArrayInit(res->child_layers, a);
  HaArrayInit(res->events, a);
  if( e != NULL )
  {
    HaArrayAddLast(res->events, e);
    res->leader = e;
    res->duration = KheEventDuration(e);
    p = NULL;  /* keep compiler happy */
  }
  else
  {
    p = KhePartitionMake(a);
    time_count = KheInstanceTimeCount(KheSolnInstance(soln));
    sz = 0;
    for( i = 0;  i < KheSolnMeetCount(soln) && sz < time_count;  i++ )
    {
      meet = KheSolnMeet(soln, i);
      if( KheMeetIsCycleMeet(meet) )
      {
	KhePartitionAdd(p, KheMeetDuration(meet));
	sz += KheMeetDuration(meet);
      }
    }
    res->leader = NULL;
    res->duration = KhePartitionSize(p);
  }

  /* local requirements */
  if( e != NULL )
  {
    res->min_total_amount = 1;
    res->max_total_amount = res->duration;
    HaArrayInit(res->min_amount, a);
    HaArrayFill(res->min_amount, res->duration, 0);
    HaArrayInit(res->max_amount, a);
    for( d = 1;  d <= res->duration;  d++ )
      HaArrayAddLast(res->max_amount, res->duration / d);
    HaArrayInit(res->upper_partitions, a);
    res->min_partition = KhePartitionUnitary(res->duration, a);
  }
  else
  {
    res->min_total_amount = res->max_total_amount = KhePartitionParts(p);
    HaArrayInit(res->min_amount, a);
    HaArrayInit(res->max_amount, a);
    for( d = 1;  d <= res->duration;  d++ )
    {
      s = KhePartitionPartsWithSize(p, d);
      HaArrayAddLast(res->min_amount, s);
      HaArrayAddLast(res->max_amount, s);
    }
    HaArrayInit(res->upper_partitions, a);
    res->min_partition = p;
  }
  HaArrayInit(res->time_groups, a);
  full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  HaArrayFill(res->time_groups, res->duration, full_tg);

  /* saved copy of partition requirements, used when trying jobs */
  res->save_active = false;
  res->save_parent_class = NULL;
  HaArrayInit(res->save_child_classes, a);
  HaArrayInit(res->save_child_layers, a);
  HaArrayInit(res->save_events, a);
  res->save_leader = NULL;
  res->save_min_total_amount = 0;
  res->save_max_total_amount = 0;
  HaArrayInit(res->save_min_amount, a);
  HaArrayInit(res->save_max_amount, a);
  HaArrayInit(res->save_upper_partitions, a);
  res->save_min_partition = KhePartitionMake(a);
  HaArrayInit(res->save_time_groups, a);

  /* temporary variables used when generating to test packability */
  res->tmp_stem = KhePartitionMake(a);
  HaArrayInit(res->tmp_partitions, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassFree(KHE_SPLIT_CLASS sc, bool free_child_classes)      */
/*                                                                           */
/*  Free sc and everything in it except the upper partitions that have       */
/*  been passed into it over time.  Free its child classes only on request.  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitClassFree(KHE_SPLIT_CLASS sc, bool free_child_classes)
{
  KHE_SPLIT_CLASS child_sc;  int i;
  if( free_child_classes )
    HaArrayForEach(sc->child_classes, child_sc, i)
      KheSplitClassFree(child_sc, true);
  MArrayFree(sc->child_classes);
  MArrayFree(sc->child_layers);
  MArrayFree(sc->events);
  MArrayFree(sc->min_amount);
  MArrayFree(sc->max_amount);
  MArrayFree(sc->upper_partitions);
  KhePartitionFree(sc->min_partition);
  MArrayFree(sc->time_groups);
  MArrayFree(sc->save_child_classes);
  MArrayFree(sc->save_child_layers);
  MArrayFree(sc->save_events);
  MArrayFree(sc->save_min_amount);
  MArrayFree(sc->save_max_amount);
  MArrayFree(sc->save_upper_partitions);
  KhePartitionFree(sc->save_min_partition);
  MArrayFree(sc->save_time_groups);
  KhePartitionFree(sc->tmp_stem);
  MArrayFree(sc->tmp_partitions);
  MFree(sc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_FOREST KheSplitClassForest(KHE_SPLIT_CLASS sc)                 */
/*                                                                           */
/*  Return the split forest enclosing sc.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_FOREST KheSplitClassForest(KHE_SPLIT_CLASS sc)
{
  return sc->forest;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parent class"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_CLASS KheSplitClassParentClass(KHE_SPLIT_CLASS sc)             */
/*                                                                           */
/*  Return the parent class of sc.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_CLASS KheSplitClassParentClass(KHE_SPLIT_CLASS sc)
{
  return sc->parent_class;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassAncestor(KHE_SPLIT_CLASS sc, KHE_SPLIT_CLASS anc_sc)   */
/*                                                                           */
/*  Return true when anc_sc is an ancestor of sc.                            */
/*                                                                           */
/*****************************************************************************/

bool KheSplitClassAncestor(KHE_SPLIT_CLASS sc, KHE_SPLIT_CLASS anc_sc)
{
  while( sc != NULL && sc != anc_sc )
    sc = sc->parent_class;
  return sc == anc_sc;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child classes"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassContainsResource(KHE_SPLIT_CLASS sc, KHE_RESOURCE r,   */
/*    int *pos)                                                              */
/*                                                                           */
/*  If sc is incident upon r, return true with *pos set to the index of      */
/*  the split layer object in sc's child layers.   Otherwise return false.   */
/*                                                                           */
/*****************************************************************************/

bool KheSplitClassContainsResource(KHE_SPLIT_CLASS sc, KHE_RESOURCE r,
  int *pos)
{
  int i;  KHE_SPLIT_LAYER spl;
  HaArrayForEach(sc->child_layers, spl, i)
    if( KheSplitLayerResource(spl) == r )
    {
      *pos = i;
      return true;
    }
  *pos = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassContainsLayer(KHE_SPLIT_CLASS sc, KHE_LAYER layer,     */
/*    int *pos)                                                              */
/*                                                                           */
/*  If sc is incident upon layer, return true with *pos set to the index of  */
/*  the split layer object in sc's child layers.   Otherwise return false.   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheSplitClassContainsLayer(KHE_SPLIT_CLASS sc, KHE_LAYER layer,
  int *pos)
{
  int i;  KHE_SPLIT_LAYER spl;
  HaArrayForEach(sc->child_layers, spl, i)
    if( KheSplitLayerLayer(spl) == layer )
    {
      *pos = i;
      return true;
    }
  *pos = -1;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassAddChildToResource(KHE_SPLIT_CLASS sc,                 */
/*    KHE_SPLIT_CLASS child_sc, KHE_RESOURCE r)                              */
/*                                                                           */
/*  Inform sc that its child class child_sc is now incident on resource r.   */
/*                                                                           */
/*****************************************************************************/

static void KheSplitClassAddChildToResource(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc, KHE_RESOURCE r, HA_ARENA a)
{
  KHE_SPLIT_LAYER split_layer;  int pos;

  if( KheSplitClassContainsResource(sc, r, &pos) )
  {
    /* add child_sc to existing split layer */
    split_layer = HaArray(sc->child_layers, pos);
    KheSplitLayerAddSplitClass(split_layer, child_sc, a);
  }
  else
  {
    /* no suitable split layer in sc, so add one and recurse */
    split_layer = KheSplitLayerMake(r, KhePartitionMake(a), a);
    HaArrayAddLast(sc->child_layers, split_layer);
    KheSplitLayerAddSplitClass(split_layer, child_sc, a);
    if( sc->parent_class != NULL )
      KheSplitClassAddChildToResource(sc->parent_class, sc, r, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassAddChildToLayer(KHE_SPLIT_CLASS sc,                    */
/*    KHE_SPLIT_CLASS child_sc, KHE_LAYER layer)                             */
/*                                                                           */
/*  Inform sc that its child class child_sc now lies in layer layer.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitClassAddChildToLayer(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc, KHE_LAYER layer)
{
  KHE_SPLIT_LAYER split_layer;  int pos;

  if( KheSplitClassContainsLayer(sc, layer, &pos) )
  {
    ** add child_sc to existing split layer **
    split_layer = HaArray(sc->child_layers, pos);
    KheSplitLayerAddSplitClass(split_layer, child_sc);
  }
  else
  {
    ** no suitable split layer in sc, so add one and recurse **
    split_layer = KheSplitLayerMake(layer, KhePartitionMake());
    HaArrayAddLast(sc->child_layers, split_layer);
    KheSplitLayerAddSplitClass(split_layer, child_sc);
    if( sc->parent_class != NULL )
      KheSplitClassAddChildToLayer(sc->parent_class, sc, layer);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassDeleteChildFromResource(KHE_SPLIT_CLASS sc,            */
/*    KHE_SPLIT_CLASS child_sc, KHE_RESOURCE r)                              */
/*                                                                           */
/*  Inform sc that its child class child_sc is no longer incident on r.      */
/*                                                                           */
/*****************************************************************************/

static void KheSplitClassDeleteChildFromResource(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc, KHE_RESOURCE r)
{
  KHE_SPLIT_LAYER split_layer;  int pos;

  /* find the existing split layer of sc containing child_sc */
  if( !KheSplitClassContainsResource(sc, r, &pos) )
    HnAbort("KheSplitClassDeleteChildFromLayer internal error");

  /* delete child_sc from existing split layer */
  split_layer = HaArray(sc->child_layers, pos);
  KheSplitLayerDeleteSplitClass(split_layer, child_sc);

  /* if split layer is now empty, delete it and recurse */
  if( KheSplitLayerSplitClassCount(split_layer) == 0 )
  {
    HaArrayDeleteAndShift(sc->child_layers, pos);
    /* KheSplitLayerFree(split_layer); */
    if( sc->parent_class != NULL )
      KheSplitClassDeleteChildFromResource(sc->parent_class, sc, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassDeleteChildFromLayer(KHE_SPLIT_CLASS sc,               */
/*    KHE_SPLIT_CLASS child_sc, KHE_LAYER layer)                             */
/*                                                                           */
/*  Inform sc that its child class child_sc no longer lies in layer layer.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitClassDeleteChildFromLayer(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc, KHE_LAYER layer)
{
  KHE_SPLIT_LAYER split_layer;  int pos;

  ** find the existing split layer of sc containing child_sc **
  if( !KheSplitClassContainsLayer(sc, layer, &pos) )
    HnAbort("KheSplitClassDeleteChildFromLayer internal error");

  ** delete child_sc from existing split layer **
  split_layer = HaArray(sc->child_layers, pos);
  KheSplitLayerDeleteSplitClass(split_layer, child_sc);

  ** if split layer is now empty, delete it and recurse **
  if( KheSplitLayerSplitClassCount(split_layer) == 0 )
  {
    HaArrayDeleteAndShift(sc->child_layers, pos);
    KheSplitLayerFree(split_layer);
    if( sc->parent_class != NULL )
      KheSplitClassDeleteChildFromLayer(sc->parent_class, sc, layer);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassAddChildClass(KHE_SPLIT_CLASS sc,                      */
/*    KHE_SPLIT_CLASS child_sc)                                              */
/*                                                                           */
/*  Make child_sc a child class of sc.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassAddChildClass(KHE_SPLIT_CLASS sc, KHE_SPLIT_CLASS child_sc,
  HA_ARENA a)
{
  KHE_SPLIT_LAYER sl;  int i;
  HnAssert(child_sc->parent_class == NULL,
    "KheSplitClassAddChildClass internal error");
  HaArrayAddLast(sc->child_classes, child_sc);
  child_sc->parent_class = sc;
  HaArrayForEach(child_sc->child_layers, sl, i)
    KheSplitClassAddChildToResource(sc, child_sc, KheSplitLayerResource(sl), a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassDeleteChildClass(KHE_SPLIT_CLASS sc,                   */
/*    KHE_SPLIT_CLASS child_sc)                                              */
/*                                                                           */
/*  Make child_sc no longer a child of sc.                                   */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassDeleteChildClass(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc)
{
  KHE_SPLIT_LAYER sl;  int pos, i;
  HnAssert(child_sc->parent_class == sc,
    "KheSplitClassDeleteChildClass internal error");
  if( !HaArrayContains(sc->child_classes, sc, &pos) )
    HnAbort("KheSplitClassDeleteChildClass internal error 2");
  HaArrayDeleteAndShift(sc->child_classes, pos);
  child_sc->parent_class = NULL;
  HaArrayForEach(child_sc->child_layers, sl, i)
    KheSplitClassDeleteChildFromResource(sc, child_sc,
      KheSplitLayerResource(sl));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitClassChildClassCount(KHE_SPLIT_CLASS sc)                     */
/*                                                                           */
/*  Return the number of child classes of sc.                                */
/*                                                                           */
/*****************************************************************************/

int KheSplitClassChildClassCount(KHE_SPLIT_CLASS sc)
{
  return HaArrayCount(sc->child_classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_CLASS KheSplitClassChildClass(KHE_SPLIT_CLASS sc, int i)       */
/*                                                                           */
/*  Return the i'th child class of sc.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_CLASS KheSplitClassChildClass(KHE_SPLIT_CLASS sc, int i)
{
  return HaArray(sc->child_classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "local requirements"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSplitClassMinTotalAmount(KHE_SPLIT_CLASS sc)                      */
/*                                                                           */
/*  Return the min total amount requirement of sc.                           */
/*                                                                           */
/*****************************************************************************/

int KheSplitClassMinTotalAmount(KHE_SPLIT_CLASS sc)
{
  return sc->min_total_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitClassMaxTotalAmount(KHE_SPLIT_CLASS sc)                      */
/*                                                                           */
/*  Return the max total amount requirement of sc.                           */
/*                                                                           */
/*****************************************************************************/

int KheSplitClassMaxTotalAmount(KHE_SPLIT_CLASS sc)
{
  return sc->max_total_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KheSplitClassMinPartition(KHE_SPLIT_CLASS sc)              */
/*                                                                           */
/*  Return the minimum partition of sc.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_PARTITION KheSplitClassMinPartition(KHE_SPLIT_CLASS sc)
{
  return sc->min_partition;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSplitClassEventCount(KHE_SPLIT_CLASS sc)                          */
/*                                                                           */
/*  Return the number of events in sc.                                       */
/*                                                                           */
/*****************************************************************************/

int KheSplitClassEventCount(KHE_SPLIT_CLASS sc)
{
  return HaArrayCount(sc->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheSplitClassEvent(KHE_SPLIT_CLASS sc, int i)                  */
/*                                                                           */
/*  Return the i'th event of sc.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheSplitClassEvent(KHE_SPLIT_CLASS sc, int i)
{
  return HaArray(sc->events, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassContainsEvent(KHE_SPLIT_CLASS sc, KHE_EVENT e)         */
/*                                                                           */
/*  Return true if sc contains e.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheSplitClassContainsEvent(KHE_SPLIT_CLASS sc, KHE_EVENT e)
{
  int pos;
  return HaArrayContains(sc->events, e, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitClassDuration(KHE_SPLIT_CLASS sc)                            */
/*                                                                           */
/*  Return the common duration of the events of sc.                          */
/*                                                                           */
/*****************************************************************************/

int KheSplitClassDuration(KHE_SPLIT_CLASS sc)
{
  return sc->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "trials"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryBegin(KHE_SPLIT_CLASS sc)                           */
/*                                                                           */
/*  Inform sc that a trial is beginning that could affect it.                */
/*  This call may be received several times; only the first is acted on.     */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryBegin(KHE_SPLIT_CLASS sc)
{
  int i;  KHE_SPLIT_LAYER sl;
  if( !sc->save_active )
  {
    /* save everything that could be affected by a trial */
    sc->save_min_total_amount = sc->min_total_amount;
    sc->save_max_total_amount = sc->max_total_amount;
    sc->save_parent_class = sc->parent_class;
    HaArrayClear(sc->save_child_classes);
    HaArrayAppend(sc->save_child_classes, sc->child_classes, i);
    HaArrayClear(sc->save_child_layers);
    HaArrayAppend(sc->save_child_layers, sc->child_layers, i);
    HaArrayClear(sc->save_events);
    HaArrayAppend(sc->save_events, sc->events, i);
    sc->save_leader = sc->leader;
    HaArrayClear(sc->save_min_amount);
    HaArrayAppend(sc->save_min_amount, sc->min_amount, i);
    HaArrayClear(sc->save_max_amount);
    HaArrayAppend(sc->save_max_amount, sc->max_amount, i);
    HaArrayClear(sc->save_upper_partitions);
    HaArrayAppend(sc->save_upper_partitions, sc->upper_partitions, i);
    KhePartitionAssign(sc->save_min_partition, sc->min_partition);
    HaArrayClear(sc->save_time_groups);
    HaArrayAppend(sc->save_time_groups, sc->time_groups, i);
    sc->save_active = true;

    /* tell the child layers */
    HaArrayForEach(sc->child_layers, sl, i)
      KheSplitLayerTryBegin(sl);

    /* pass the message up */
    if( sc->parent_class != NULL )
      KheSplitClassTryBegin(sc->parent_class);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryAddToLayer(KHE_SPLIT_CLASS sc, KHE_LAYER layer)     */
/*                                                                           */
/*  Try to add sc to layer.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitClassTryAddToLayer(KHE_SPLIT_CLASS sc, KHE_LAYER layer)
{
  KHE_SPLIT_LAYER split_layer;  int i;

  ** fail now if there is an existing child layer of sc for layer **
  if( KheSplitClassContainsLayer(sc, layer, &i) )
    return;

  ** add a new split layer and recurse **
  split_layer = KheSplitLayerMake(layer, KhePartitionUnitary(sc->duration));
  HaArrayAddLast(sc->child_layers, split_layer);
  if( sc->parent_class != NULL )
    KheSplitClassAddChildToLayer(sc->parent_class, sc, layer);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryAddToResource(KHE_SPLIT_CLASS sc, KHE_RESOURCE r)   */
/*                                                                           */
/*  Try to add sc to the split layer for r.                                  */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryAddToResource(KHE_SPLIT_CLASS sc, KHE_RESOURCE r,
  HA_ARENA a)
{
  KHE_SPLIT_LAYER split_layer;  int i;

  if( DEBUG9 )
    fprintf(stderr, "[ KheSplitClassTryAddToResource(sc, %s, a)\n",
      KheResourceId(r));

  /* fail now if there is an existing child layer of sc for r */
  if( KheSplitClassContainsResource(sc, r, &i) )
  {
    if( DEBUG9 )
      fprintf(stderr, "] KheSplitClassTryAddToResource returning early\n");
    return;
  }

  /* add a new split layer and recurse */
  split_layer = KheSplitLayerMake(r, KhePartitionUnitary(sc->duration, a), a);
  HaArrayAddLast(sc->child_layers, split_layer);
  if( sc->parent_class != NULL )
    KheSplitClassAddChildToResource(sc->parent_class, sc, r, a);
  if( DEBUG9 )
    fprintf(stderr, "] KheSplitClassTryAddToResource returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheSplitTimeGroupIntersect(KHE_TIME_GROUP tg1,            */
/*    KHE_TIME_GROUP tg2, KHE_SOLN soln)                                     */
/*                                                                           */
/*  Return a time group which is the intersection of the two given time      */
/*  groups.  This will be one of them if either is a subset of the other,    */
/*  or a standard empty time group if they are disjoint, or, in the last     */
/*  resort, a freshly made time group.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheSplitTimeGroupIntersect(KHE_TIME_GROUP tg1,
  KHE_TIME_GROUP tg2, KHE_SOLN soln)
{
  if( KheTimeGroupSubset(tg1, tg2) )
    return tg1;
  else if( KheTimeGroupSubset(tg2, tg1) )
    return tg2;
  else if( KheTimeGroupDisjoint(tg1, tg2) )
    return KheInstanceEmptyTimeGroup(KheSolnInstance(soln));
  else
  {
    KheSolnTimeGroupBegin(soln);
    KheSolnTimeGroupUnion(soln, tg1);
    KheSolnTimeGroupIntersect(soln, tg2);
    return KheSolnTimeGroupEnd(soln);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryEventDomain(KHE_SPLIT_CLASS sc,                     */
/*    KHE_TIME_GROUP tg, int durn)                                           */
/*                                                                           */
/*  Try reducing the durn domain of sc to tg.                                */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryEventDomain(KHE_SPLIT_CLASS sc,
  KHE_TIME_GROUP tg, int durn)
{
  KHE_SOLN soln;  KHE_TIME_GROUP tg1, tg2;
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheSplitClassTryEventDomain(%s, ",
      sc->leader == NULL ? "~" : KheEventId(sc->leader) == NULL ? "-" :
      KheEventId(sc->leader));
    KheTimeGroupDebug(tg, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", durn);
  }
  if( durn <= HaArrayCount(sc->time_groups) )
  {
    soln = KheSplitForestSoln(sc->forest);
    tg1 = HaArray(sc->time_groups, durn - 1);
    tg2 = KheSplitTimeGroupIntersect(tg, tg1, soln);
    HaArrayPut(sc->time_groups, durn - 1, tg2);
    if( KheTimeGroupTimeCount(tg2) == 0 )
      KheSplitClassTryDurnAmount(sc, durn, 0, 0);
    if( sc->parent_class != NULL && sc->parent_class->duration == sc->duration )
      KheSplitClassTryEventDomain(sc->parent_class, tg, durn);
  }
  if( DEBUG5 )
    fprintf(stderr, "] KheSplitClassTryEventDomain returning\n");
}



/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryTotalAmount(KHE_SPLIT_CLASS sc,                     */
/*    int min_total_amount, int max_total_amount)                            */
/*                                                                           */
/*  Try limiting the total amount fields to this.                            */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryTotalAmount(KHE_SPLIT_CLASS sc,
  int min_total_amount, int max_total_amount)
{
  HnAssert(sc->save_active, "KheSplitClassTryTotalAmount internal error");
  sc->min_total_amount = max(sc->min_total_amount, min_total_amount);
  sc->max_total_amount = min(sc->max_total_amount, max_total_amount);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryDurnAmount(KHE_SPLIT_CLASS sc, int durn,            */
/*    int min_amount, int max_amount)                                        */
/*                                                                           */
/*  Try limiting the min and max amount of a given duration to this.         */
/*  The duration may be out of range, that just limits nothing.              */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryDurnAmount(KHE_SPLIT_CLASS sc, int durn,
  int min_amount, int max_amount)
{
  HnAssert(sc->save_active, "KheSplitClassTryDurnAmount internal error");
  if( durn <= sc->duration )
  {
    min_amount = max(min_amount, HaArray(sc->min_amount, durn - 1));
    max_amount = min(max_amount, HaArray(sc->max_amount, durn - 1));
    HaArrayPut(sc->min_amount, durn - 1, min_amount);
    HaArrayPut(sc->max_amount, durn - 1, max_amount);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryUnsplittable(KHE_SPLIT_CLASS sc)                    */
/*                                                                           */
/*  Try making sc unsplittable.                                              */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryUnsplittable(KHE_SPLIT_CLASS sc)
{
  int d;
  HnAssert(sc->save_active, "KheSplitClassTryUnsplittable internal error");
  KheSplitClassTryTotalAmount(sc, 1, 1);
  for( d = 1;  d < sc->duration;  d++ )
    KheSplitClassTryDurnAmount(sc, d, 0, 0);
  KheSplitClassTryDurnAmount(sc, sc->duration, 1, 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryPackableInto(KHE_SPLIT_CLASS sc, KHE_PARTITION p)   */
/*                                                                           */
/*  Try limiting the partitions of sc to be packable into p.                 */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryPackableInto(KHE_SPLIT_CLASS sc, KHE_PARTITION p)
{
  KHE_PARTITION p2;  int i, d;

  /* if p is already present, we don't need it again */
  HnAssert(sc->save_active, "KheSplitClassTryPackableInto internal error");
  HaArrayForEach(sc->upper_partitions, p2, i)
    if( KhePartitionEqual(p2, p) )
      return;

  /* add p to the partitions */
  HaArrayAddLast(sc->upper_partitions, p);

  /* also, there can be no parts of duration > KhePartitionMax(p) */
  for( d = KhePartitionMax(p) + 1;  d <= sc->duration;  d++ )
    KheSplitClassTryDurnAmount(sc, d, 0, 0);

  /* and the number of parts of size KhePartitionMax(p) is limited too */
  if( KhePartitionMax(p) >= 1 )
    KheSplitClassTryDurnAmount(sc, KhePartitionMax(p), 0,
      KhePartitionPartsWithSize(p, KhePartitionMax(p)));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryMergeResource(KHE_SPLIT_CLASS dst_sc,               */
/*    KHE_SPLIT_LAYER src_sl)                                                */
/*                                                                           */
/*  Try a merge of layer src_sl into the child layers of dst_sc.             */
/*                                                                           */
/*****************************************************************************/

static void KheSplitClassTryMergeResource(KHE_SPLIT_CLASS dst_sc,
  KHE_SPLIT_LAYER src_sl)
{
  int i;  KHE_SPLIT_LAYER dst_sl;
  HaArrayForEach(dst_sc->child_layers, dst_sl, i)
    if( KheSplitLayerResource(dst_sl) == KheSplitLayerResource(src_sl) )
    {
      KheSplitLayerTryMerge(dst_sl, src_sl);
      return;
    }
  HaArrayAddLast(dst_sc->child_layers, src_sl);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryMergeLayer(KHE_SPLIT_CLASS dst_sc,                  */
/*    KHE_SPLIT_LAYER src_sl)                                                */
/*                                                                           */
/*  Try a merge of layer src_sl into the child layers of dst_sc.             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitClassTryMergeLayer(KHE_SPLIT_CLASS dst_sc,
  KHE_SPLIT_LAYER src_sl)
{
  int i;  KHE_SPLIT_LAYER dst_sl;
  HaArrayForEach(dst_sc->child_layers, dst_sl, i)
    if( KheSplitLayerLayer(dst_sl) == KheSplitLayerLayer(src_sl) )
    {
      KheSplitLayerTryMerge(dst_sl, src_sl);
      return;
    }
  HaArrayAddLast(dst_sc->child_layers, src_sl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventAssigned(KHE_SOLN soln, KHE_EVENT e, KHE_EVENT *ass_event)  */
/*                                                                           */
/*  If e contains an assigned meet in soln, return true, and                 */
/*  set *ass_event to the event it's assigned to, or to NULL if the          */
/*  assignment is to some other meet (e.g. in the cycle layer).              */
/*                                                                           */
/*****************************************************************************/

static bool KheEventAssigned(KHE_SOLN soln, KHE_EVENT e, KHE_EVENT *ass_event)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetAsst(meet) != NULL )
    {
      *ass_event = KheMeetEvent(KheMeetAsst(meet));
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassEventMustBeLeader(KHE_SOLN soln, KHE_EVENT e,          */
/*    KHE_SPLIT_CLASS sc1, KHE_SPLIT_CLASS sc2)                              */
/*                                                                           */
/*  Return true if e (the leader of either sc1 or sc2) must be the leader    */
/*  when the two classes are merged.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitClassEventMustBeLeader(KHE_SOLN soln, KHE_EVENT e,
  KHE_SPLIT_CLASS sc1, KHE_SPLIT_CLASS sc2)
{
  KHE_EVENT ass_event;
  if( KheEventAssigned(soln, e, &ass_event) )
    return ass_event == NULL || (!KheSplitClassContainsEvent(sc1, ass_event)
	&& !KheSplitClassContainsEvent(sc2, ass_event));
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheSplitClassMergeLeaders(KHE_SPLIT_CLASS sc1,                 */
/*    KHE_SPLIT_CLASS sc2)                                                   */
/*                                                                           */
/*  Return the new leader when sc1 and sc2 merge, or NULL if no leader can   */
/*  be found.                                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT KheSplitClassMergeLeaders(KHE_SPLIT_CLASS sc1,
  KHE_SPLIT_CLASS sc2)
{
  KHE_SOLN soln;
  soln = KheSplitForestSoln(sc1->forest);
  if( !KheSplitClassEventMustBeLeader(soln, sc2->leader, sc1, sc2) )
    return sc1->leader;
  else if( !KheSplitClassEventMustBeLeader(soln, sc1->leader, sc1, sc2) )
    return sc2->leader;
  else
    return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryMerge(KHE_SPLIT_CLASS dst_sc,                       */
/*    KHE_SPLIT_CLASS src_sc)                                                */
/*                                                                           */
/*  Try merging the requirements of src_sc into those of dst_sc.             */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryMerge(KHE_SPLIT_CLASS dst_sc, KHE_SPLIT_CLASS src_sc)
{
  int i, d, pos;  KHE_PARTITION p;  KHE_SPLIT_LAYER src_sl;
  HnAssert(dst_sc->save_active, "KheSplitClassTryMerge internal error 1");
  HnAssert(src_sc->save_active, "KheSplitClassTryMerge internal error 2");
  HnAssert(src_sc->duration == dst_sc->duration,
    "KheSplitClassTryMerge internal error 3");

  /* merge the leaders, possibly failing to find one */
  dst_sc->leader = KheSplitClassMergeLeaders(dst_sc, src_sc);

  /* merge in the local requirements */
  KheSplitClassTryTotalAmount(dst_sc, src_sc->min_total_amount,
    src_sc->max_total_amount);
  for( d = 1;  d <= src_sc->duration;  d++ )
    KheSplitClassTryDurnAmount(dst_sc, d,
      HaArray(src_sc->min_amount, d-1), HaArray(src_sc->max_amount, d-1));
  HaArrayForEach(src_sc->upper_partitions, p, d)
    KheSplitClassTryPackableInto(dst_sc, p);
  for( d = 1;  d <= src_sc->duration;  d++ )
    KheSplitClassTryEventDomain(dst_sc, HaArray(src_sc->time_groups, d-1), d);

  /* merge the child classes, events and child layers */
  HaArrayAppend(dst_sc->child_classes, src_sc->child_classes, i);
  HaArrayAppend(dst_sc->events, src_sc->events, i);
  HaArrayForEach(src_sc->child_layers, src_sl, i)
    KheSplitClassTryMergeResource(dst_sc, src_sl);

  /* remove src_sc from its parent class */
  if( !HaArrayContains(src_sc->parent_class->child_classes, src_sc, &pos) )
    HnAbort("KheSplitClassTryMerge internal error 3");
  HaArrayDeleteAndShift(src_sc->parent_class->child_classes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassTryAssign(KHE_SPLIT_CLASS child_sc,                    */
/*    KHE_SPLIT_CLASS prnt_sc)                                               */
/*                                                                           */
/*  Try assigning child_sc to prnt_sc.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassTryAssign(KHE_SPLIT_CLASS child_sc, KHE_SPLIT_CLASS prnt_sc,
  HA_ARENA a)
{
  /* child_sc must be currently assigned to the cycle layer class */
  if( child_sc->parent_class != NULL && child_sc->parent_class->leader == NULL )
  {
    /* try moving child_sc from its current parent to prnt_sc */
    KheSplitClassDeleteChildClass(child_sc->parent_class, child_sc);
    KheSplitClassAddChildClass(prnt_sc, child_sc, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoBinPack(KHE_PARTITION p1, KHE_PARTITION p2)                    */
/*                                                                           */
/*  Basically just a call to KhePartitionBinPack, but with debugging.        */
/*                                                                           */
/*****************************************************************************/

static bool KheDoBinPack(KHE_PARTITION p1, KHE_PARTITION p2, HA_ARENA a)
{
  bool res;
  if( DEBUG8 )
    fprintf(stderr, "[ KheDoBinPack(%s, %s)\n",
      KhePartitionShow(p1, a), KhePartitionShow(p2, a));
  res = KhePartitionBinPack(p1, p2);
  if( DEBUG8 )
    fprintf(stderr, "] KheDoBinPack returning %s\n", res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassGenPartitions(KHE_SPLIT_CLASS sc,                      */
/*    int stem_part_size, int stem_durn, int stem_parts, int max_results)    */
/*                                                                           */
/*  This function does the real work of enumerating (lexically minimum       */
/*  first) the partitions which satisfy the requirements stored in sc:       */
/*                                                                           */
/*    sc holds the local requirements and gives access to the structural     */
/*    requirements.  If also holds temporary variables sc->tmp_stem and      */
/*    sc->tmp_partitions.                                                    */
/*                                                                           */
/*    stem_part_size is the size of the parts to be generated by this        */
/*    call, not counting the recursive call (the recursive call generates    */
/*    parts of all sizes smaller than stem_part_size).                       */
/*                                                                           */
/*    stem_durn is the current total duration of sc->tmp_stem.               */
/*                                                                           */
/*    stem_parts is the current number of parts of sc->tmp_stem.             */
/*                                                                           */
/*  stem_durn and stem_parts are redundant but it saves time to calculate    */
/*  them as we go, rather that recalculating them from sc->tmp_stem each     */
/*  time they are needed.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSplitClassGenPartitions(KHE_SPLIT_CLASS sc,
  int stem_part_size, int stem_durn, int stem_parts, int max_results,
  HA_ARENA a)
{
  int min_of_size, max_of_size, parts, i;  KHE_PARTITION p;
  KHE_SPLIT_LAYER sl;  KHE_SPLIT_CLASS child_sc;
  if( DEBUG10 )
    fprintf(stderr, "[ KheSplitClassGenPartitions(sc, %d, %d, %d, %d, a)\n",
      stem_part_size, stem_durn, stem_parts, max_results);

  /* quit now if enough results */
  if( HaArrayCount(sc->tmp_partitions) >= max_results )
  {
    if( DEBUG10 )
      fprintf(stderr, "] KheSplitClassGenPartition returning (max_results)\n");
    return;
  }

  /* quit now if sum of all parts is too great */
  if( stem_durn > sc->duration )
  {
    if( DEBUG10 )
      fprintf(stderr, "] KheSplitClassGenPartition returning (stem_durn)\n");
    return;
  }

  /* quit now if too many parts */
  if( stem_parts > sc->max_total_amount )
  {
    if( DEBUG10 )
      fprintf(stderr, "] KheSplitClassGenPartition returning (stem_parts)\n");
    return;
  }

  /* quit now if all parts are done, possibly saving an acceptable stem */
  if( stem_part_size == 0 )
  {
    if( stem_parts >= sc->min_total_amount && stem_durn == sc->duration )
    {
      /* make sure that tmp_stem is packable into each upper partition */
      HaArrayForEach(sc->upper_partitions, p, i)
	if( !KheDoBinPack(sc->tmp_stem, p, a) )
	{
	  if( DEBUG10 )
	    fprintf(stderr, "] KheSplitClassGenPartition returning "
	      "(KheDoBinPack 1)\n");
	  return;
	}

      /* make sure that each child layer is packable into tmp_stem */
      HaArrayForEach(sc->child_layers, sl, i)
	if( !KheDoBinPack(KheSplitLayerMinPartition(sl), sc->tmp_stem, a) )
	{
	  if( DEBUG10 )
	    fprintf(stderr, "] KheSplitClassGenPartition returning "
	      "(KheDoBinPack 2)\n");
	  return;
	}

      /* make sure that each child class is packable into tmp_stem */
      HaArrayForEach(sc->child_classes, child_sc, i)
      {
	if( DEBUG4 || DEBUG10 )
	  fprintf(stderr, " KheSplitClassGenPartitions testing %s into %s\n",
	    KhePartitionShow(child_sc->min_partition, a),
	    KhePartitionShow(sc->tmp_stem, a));
	if( /* HaArrayCount(child_sc->parent_layers) == 0 && */
	    !KheDoBinPack(child_sc->min_partition, sc->tmp_stem, a) )
	{
	  if( DEBUG10 )
	    fprintf(stderr, "] KheSplitClassGenPartition returning "
	      "(KheDoBinPack 3)\n");
	  return;
	}
      }

      /* save tmp_stem; it is an acceptable partition */
      HaArrayAddLast(sc->tmp_partitions, KhePartitionCopy(sc->tmp_stem, a));
    }

    if( DEBUG10 )
      fprintf(stderr, "] KheSplitClassGenPartition returning "
	"(stem_part_size == 0)\n");
    return;
  }

  /* generate all legal numbers of parts of size stem_part_size */
  /* NB min_of_size were already added, before recursion started */
  min_of_size = HaArray(sc->min_amount, stem_part_size - 1);
  max_of_size = HaArray(sc->max_amount, stem_part_size - 1);
  for( parts = min_of_size;  parts <= max_of_size;  parts++ )
  {
    for( i = 0;  i < parts - min_of_size;  i++ )
    {
      KhePartitionAdd(sc->tmp_stem, stem_part_size);
      stem_durn += stem_part_size;
      stem_parts += 1;
    }
    KheSplitClassGenPartitions(sc, stem_part_size - 1, stem_durn,
      stem_parts, max_results, a);
    for( i = 0;  i < parts - min_of_size;  i++ )
    {
      KhePartitionSub(sc->tmp_stem, stem_part_size);
      stem_durn -= stem_part_size;
      stem_parts -= 1;
    }
  }
  if( DEBUG10 )
    fprintf(stderr, "] KheSplitClassGenPartition returning (at end)\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassTryAlive(KHE_SPLIT_CLASS sc)                           */
/*                                                                           */
/*  Return true if sc is "alive", that is, if at least one partition         */
/*  exists which satisfies its requirements, and its ancestors are alive.    */
/*                                                                           */
/*****************************************************************************/

bool KheSplitClassTryAlive(KHE_SPLIT_CLASS sc, HA_ARENA a)
{
  int min_by_duration, max_by_duration, min_amt, max_amt, i, j;
  int stem_durn, stem_parts;  KHE_PARTITION p;  KHE_SPLIT_LAYER sl;
  if( DEBUG1 )
    fprintf(stderr, "[ KheSplitClassTryAlive(sc, a)\n");

  /* no leader test */
  if( HaArrayCount(sc->events) > 0 && sc->leader == NULL )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive returning dead: no leader\n");
    return false;
  }

  /* carry out the easy tests for non-liveness */
  if( sc->min_total_amount > sc->max_total_amount ||
      sc->min_total_amount > sc->duration )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive returning dead: total amount\n");
    return false;
  }
  min_by_duration = max_by_duration = 0;
  for( i = 0;  i < HaArrayCount(sc->min_amount);  i++ )
  {
    min_amt = HaArray(sc->min_amount, i);
    max_amt = HaArray(sc->max_amount, i);
    if( min_amt > max_amt )
    {
      if( DEBUG1 )
	fprintf(stderr, "] KheSplitClassTryAlive returning dead: amount for durn %d\n", i + 1);
      return false;
    }
    min_by_duration += min_amt;
    max_by_duration += max_amt;
  }
  if( min_by_duration > sc->max_total_amount ||
      min_by_duration > sc->duration ||
      max_by_duration < sc->min_total_amount )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive ret. dead: total amount (2)\n");
    return false;
  }

  /* test for liveness */
  KhePartitionClear(sc->tmp_stem);
  stem_durn = stem_parts = 0;
  for( i = 0;  i < HaArrayCount(sc->min_amount);  i++ )
  {
    min_amt = HaArray(sc->min_amount, i);
    for( j = 0;  j < min_amt;  j++ )
    {
      KhePartitionAdd(sc->tmp_stem, i + 1);
      stem_durn += (i + 1);
      stem_parts += 1;
    }
  }
  HaArrayClear(sc->tmp_partitions);
  /* ***
  while( HaArrayCount(sc->tmp_partitions) > 0 )
    KhePartitionFree(HaArrayLastAndDelete(sc->tmp_partitions));
  *** */
  if( DEBUG1 )
    fprintf(stderr,"  KheSplitClassTryAlive call KheSplitClassGenPartitions\n");
  KheSplitClassGenPartitions(sc, sc->duration, stem_durn, stem_parts, 1, a);

  /* fail and return if no partitions */
  if( HaArrayCount(sc->tmp_partitions) == 0 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive . dead: did not generate\n");
    return false;
  }

  /* update min partition in parents, if changed */
  p = HaArrayFirst(sc->tmp_partitions);
  if( !KhePartitionEqual(sc->min_partition, p) )
  {
    HaArrayForEach(sc->parent_class->child_layers, sl, i)
      if( KheSplitLayerContainsSplitClass(sl, sc) )
	KheSplitLayerTryMinPartitionChange(sl, sc->min_partition, p);
    /* ***
    HaArrayForEach(sc->parent_layers, sl, i)
      KheSplitLayerTryMinPartitionChange(sl, sc->min_partition, p);
    *** */
    KhePartitionAssign(sc->min_partition, p);
  }

  /* succeed if the parent class is alive (or there is no parent) */
  if( sc->parent_class == NULL || KheSplitClassTryAlive(sc->parent_class, a) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive returning alive: %s\n",
	KhePartitionShow(sc->min_partition, a));
    return true;
  }
  else
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSplitClassTryAlive returning dead: parent class\n");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitClassTryEnd(KHE_SPLIT_CLASS sc, bool success)               */
/*                                                                           */
/*  Inform sc that the trial that began earlier is now ending, either        */
/*  successfully or not.  For convenience, the value of success is also      */
/*  returned.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheSplitClassTryEnd(KHE_SPLIT_CLASS sc, bool success)
{
  int i;  KHE_SPLIT_LAYER sl;
  if( sc->save_active )
  {
    if( !success )
    {
      /* not successful, so restore local requirements */
      sc->min_total_amount = sc->save_min_total_amount;
      sc->max_total_amount = sc->save_max_total_amount;
      sc->parent_class = sc->save_parent_class;
      HaArrayClear(sc->child_classes);
      HaArrayAppend(sc->child_classes, sc->save_child_classes, i);
      HaArrayClear(sc->child_layers);
      HaArrayAppend(sc->child_layers, sc->save_child_layers, i);
      HaArrayClear(sc->events);
      HaArrayAppend(sc->events, sc->save_events, i);
      sc->leader = sc->save_leader;
      HaArrayClear(sc->min_amount);
      HaArrayAppend(sc->min_amount, sc->save_min_amount, i);
      HaArrayClear(sc->max_amount);
      HaArrayAppend(sc->max_amount, sc->save_max_amount, i);
      HaArrayClear(sc->upper_partitions);
      HaArrayAppend(sc->upper_partitions, sc->save_upper_partitions, i);
      KhePartitionAssign(sc->min_partition, sc->save_min_partition);
      HaArrayClear(sc->time_groups);
      HaArrayAppend(sc->time_groups, sc->save_time_groups, i);
    }
    sc->save_active = false;

    /* tell the child layers */
    HaArrayForEach(sc->child_layers, sl, i)
      KheSplitLayerTryEnd(sl, success);

    /* pass the message up */
    if( sc->parent_class != NULL )
      KheSplitClassTryEnd(sc->parent_class, success);
  }
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "finalize"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddEventDurationsToPartition(KHE_SOLN soln, KHE_EVENT e,    */
/*    KHE_PARTITION p)                                                       */
/*                                                                           */
/*  Add the durations of the meets of e to p.                                */
/*                                                                           */
/*****************************************************************************/

static void KheSplitAddEventDurationsToPartition(KHE_SOLN soln, KHE_EVENT e,
  KHE_PARTITION p)
{
  int i;  KHE_MEET meet;
  KhePartitionClear(p);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    HnAssert(KheMeetDuration(meet) >= 1,
      "KheSplitAddEventDurationsToPartition internal error");
    KhePartitionAdd(p, KheMeetDuration(meet));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitSplitEvent(KHE_SOLN soln, KHE_EVENT e,                      */
/*    ARRAY_KHE_PARTITION *partitions)                                       */
/*                                                                           */
/*  Split the meets of e into smaller events, according to the               */
/*  instructions in *partitions.                                             */
/*                                                                           */
/*  Implementation note.  This function assumes that new split events for    */
/*  a given event get put on the end of the list, which is true.             */
/*                                                                           */
/*****************************************************************************/

static void KheSplitSplitEvent(KHE_SOLN soln, KHE_EVENT e,
  ARRAY_KHE_PARTITION *partitions, HA_ARENA a)
{
  int i, j, initial_count, m;  KHE_MEET meet, meet2;  KHE_PARTITION p;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheSplitSplitEvent(soln, %s,",
      KheEventId(e) != NULL ? KheEventId(e) : "-");
    HaArrayForEach(*partitions, p, j)
      fprintf(stderr, " %s", KhePartitionShow(p, a));
    fprintf(stderr, ")\n");
    for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    {
      meet = KheEventMeet(soln, e, i);
      KheMeetCheck(meet);
      fprintf(stderr, "  meet %s durn %d\n",
	KheMeetId(meet), KheMeetDuration(meet));
    }
  }
  initial_count = KheEventMeetCount(soln, e);
  for( i = 0;  i < initial_count;  i++ )
  {
    meet = KheEventMeet(soln, e, i);

    /* find a partition in *partitions whose size is meet's duration */
    p = NULL;  /* keep compiler happy */
    HaArrayForEach(*partitions, p, j)
      if( KhePartitionSize(p) == KheMeetDuration(meet) )
	break;
    HnAssert(j < HaArrayCount(*partitions),"KheSplitSplitEvent internal error");

    /* split meet according to the instructions in p */
    if( DEBUG2 )
      fprintf(stderr, "  splitting meet %s with durn %d by partition %s\n",
	KheMeetId(meet), KheMeetDuration(meet), KhePartitionShow(p, a));
    while( KhePartitionParts(p) > 1 )
    {
      m = KhePartitionMax(p);
      KhePartitionSub(p, m);
      KheMeetSplit(meet, KheMeetDuration(meet) - m, false, &meet, &meet2);
    }

    /* remove all trace of p and continue */
    HaArrayDeleteAndShift(*partitions, j);
    /* KhePartitionFree(p); */
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "] KheSplitSplitEvent returning; durations:");
    for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    {
      meet = KheEventMeet(soln, e, i);
      KheMeetCheck(meet);
      fprintf(stderr, " %s %d", KheMeetId(meet), KheMeetDuration(meet));
    }
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitEventIsPartiallyAssigned(KHE_SOLN soln, KHE_EVENT e,        */
/*    KHE_EVENT *prnt_e)                                                     */
/*                                                                           */
/*  If e is partially (or totally) assigned, return true and set *prnt_e     */
/*  to the event it is assigned to.  Otherwise return false.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitEventIsPartiallyAssigned(KHE_SOLN soln, KHE_EVENT e,
  KHE_EVENT *prnt_e)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetAsst(meet) != NULL )
    {
      e = KheMeetEvent(meet);
      if( e != NULL )
      {
	*prnt_e = e;
	return true;
      }
    }
  }
  *prnt_e = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventAssign(KHE_SOLN soln, KHE_EVENT child_e,               */
/*    KHE_EVENT prnt_e, KHE_SPLIT_ASSIGNER sa)                               */
/*                                                                           */
/*  Assign the meets of child_e to the meets of prnt_e.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitEventAssign(KHE_SOLN soln, KHE_EVENT child_e,
  KHE_EVENT prnt_e, KHE_SPLIT_ASSIGNER sa)
{
  int i;  KHE_MEET meet;
  KheSplitAssignerBegin(sa);
  for( i = 0;  i < KheEventMeetCount(soln, child_e);  i++ )
  {
    meet = KheEventMeet(soln, child_e, i);
    KheSplitAssignerAddChildMeet(sa, meet);
  }
  for( i = 0;  i < KheEventMeetCount(soln, prnt_e);  i++ )
  {
    meet = KheEventMeet(soln, prnt_e, i);
    KheSplitAssignerAddParentMeet(sa, meet);
  }
  KheSplitAssignerEnd(sa);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheSplitClassFinalize(KHE_SPLIT_CLASS sc, KHE_NODE parent_node) */
/*                                                                           */
/*  Finalize sc by splitting its meets to fit its min partition,             */
/*  placing them into a node which is a child of parent_node (if non-NULL),  */
/*  and recursively finalizing sc's child nodes.  Return sc's leader node.   */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheSplitClassFinalize(KHE_SPLIT_CLASS sc, KHE_NODE parent_node,
  HA_ARENA a)
{
  KHE_EVENT e, prnt_e;  int i, j, sz, time_count, durn1;
  KHE_SPLIT_CLASS child_sc;  KHE_NODE leader_node, prnt_node, node;
  KHE_MEET meet;  KHE_TIME_GROUP tg, full_tg;  KHE_MEET_BOUND mb;
  ARRAY_KHE_PARTITION partitions;  KHE_PARTITION event_p;  KHE_SOLN soln;
  /* KHE_TIME_DOMAIN td; */
  if( DEBUG7 )
    fprintf(stderr, "[ KheSplitClassFinalize(Split Class %d)\n", sc->id_num);
  soln = KheSplitForestSoln(sc->forest);
  full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));

  if( parent_node == NULL )
  {
    /* this is the root node; make a node holding the cycle meets */
    HnAssert(HaArrayCount(sc->events) == 0, "KheLayerTreeMake internal error 1");
    HnAssert(sc->leader == NULL, "KheLayerTreeMake internal error 2");
    leader_node = KheNodeMake(soln);
    time_count = KheInstanceTimeCount(KheSolnInstance(soln));
    sz = 0;
    for( i = 0;  i < KheSolnMeetCount(soln) && sz < time_count;  i++ )
    {
      meet = KheSolnMeet(soln, i);
      if( KheMeetIsCycleMeet(meet) )
      {
	KheMeetCheck(meet);
	if( !KheNodeAddMeet(leader_node, meet) )
	  HnAbort("KheLayerTreeMake internal error 3");
	sz += KheMeetDuration(meet);
      }
    }
  }
  else
  {
    /* not the root node; build a time domain for sc's events' meets */
    /* ***
    KheSolnTimeDomainBegin(soln);
    for( d = 1;  d <= sc->duration;  d++ )
      KheSolnTimeDomainSetTimeGroup(soln, d, HaArray(sc->time_groups, d-1));
    td = KheSolnTimeDomainEnd(soln);
    *** */

    /* sort out splits, add nodes, and set time domains */
    HnAssert(HaArrayCount(sc->events) > 0, "KheLayerTreeMake internal error 4");
    HnAssert(sc->leader != NULL, "KheLayerTreeMake internal error 5");
    HaArrayInit(partitions, a);
    event_p = KhePartitionMake(a);
    HaArrayForEach(sc->events, e, i)
    {
      /* split the meets of e to conform to min_partition */
      KheSplitAddEventDurationsToPartition(soln, e, event_p);
      if( KhePartitionBinPackAndHow(sc->min_partition, event_p, &partitions,a) )
	KheSplitSplitEvent(soln, e, &partitions, a);
      else if( DEBUG7 )
	fprintf(stderr, "  failed to pack %s into %s\n",
	  KhePartitionShow(sc->min_partition, a), KhePartitionShow(event_p, a));

      /* make a meet bound for the meets of e, if required */
      mb = NULL;
      HaArrayForEach(sc->time_groups, tg, durn1)
	if( !KheTimeGroupEqual(tg, full_tg) )
	{
	  if( mb == NULL )
	    mb = KheMeetBoundMake(soln, false, full_tg);
	  KheMeetBoundAddTimeGroup(mb, durn1 + 1, tg);
	}

      /* add the meets of e to a new node, set their domains, and fix them */
      node = KheNodeMake(soln);
      for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
      {
	meet = KheEventMeet(soln, e, j);
	if( mb != NULL && !KheMeetAddMeetBound(meet, mb) )
	{
	  if( DEBUG7 )
	  {
	    fprintf(stderr, "  failing on meet ");
	    KheMeetDebug(meet, 1, -1, stderr);
	    fprintf(stderr, "\n");
	  }
	  HnAbort("KheLayerTreeMake internal error 6");
	}
	/* ***
	HaArrayForEach(sc->time_groups, tg, durn1)
	{
	  ** tg = HaArray(sc->time_groups, KheMeetDuration(meet) - 1); **
	  if( !KheTimeGroupEqual(tg, full_tg) )
	  {
	    if( DEBUG7 )
	    {
	      fprintf(stderr,"  KheSplitClass calling KheMeetBoundMake(");
	      KheMeetDebug(meet, 1, -1, stderr);
	      fprintf(stderr, ", ");
	      KheTimeGroupDebug(tg, 1, -1, stderr);
	      fprintf(stderr, ", %d)\n", durn1 + 1);
	    }
	    ** if( !KheMeetSetDomain(meet, tg) ) **
	    if( !KheMeetBoundMake(NULL, meet, durn1 + 1, tg, &mb) )
	    {
	      if( DEBUG7 )
	      {
		fprintf(stderr, "  failing on meet ");
		KheMeetDebug(meet, 1, -1, stderr);
		fprintf(stderr, "\n");
	      }
	      HnAbort("KheLayerTreeMake internal error 6");
	    }
	  }
	}
	*** */
	if( !KheNodeAddMeet(node, meet) )
	  HnAbort("KheLayerTreeMake internal error 7");
	/* KheMeetSplitFix(meet); */
	/* KheMeetDomainFix(meet); */
      }
    }
    /* KhePartitionFree(event_p); */
    /* MArrayFree(partitions); */

    /* find the leader node */
    leader_node = KheMeetNode(KheEventMeet(soln, sc->leader, 0));

    /* assign each node to some other node */
    HaArrayForEach(sc->events, e, i)
    {
      node = KheMeetNode(KheEventMeet(soln, e, 0));
      if( e == sc->leader )
      {
	/* e is the leader, make parent_node the parent */
	HnAssert(parent_node != NULL, "KheLayerTreeMake internal error 8");
	prnt_node = parent_node;
      }
      else if( KheSplitEventIsPartiallyAssigned(soln, e, &prnt_e) )
      {
	/* e is partially assigned to prnt_e, make prnt_e's node the parent */
	prnt_node = KheMeetNode(KheEventMeet(soln, e, 0));
      }
      else
      {
	/* otherwise leader_node is the parent */
	HnAssert(sc->leader != NULL, "KheLayerTreeMake internal error 9");
	prnt_node = leader_node;
      }
      if( !KheNodeAddParent(node, prnt_node) )
	HnAbort("KheLayerTreeMake internal error 10");
    }
  }

  /* finalize the child classes, linking their nodes to leader_node */
  HaArrayForEach(sc->child_classes, child_sc, i)
    KheSplitClassFinalize(child_sc, leader_node, a);

  if( DEBUG7 )
  {
    fprintf(stderr, "  KheSplitClassFinalize final node:\n");
    KheNodeDebug(leader_node, 4, 2, stderr);
    fprintf(stderr, "] KheSplitClassFinalize returning\n");
  }
  return leader_node;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassDebugBrief(KHE_SPLIT_CLASS sc, FILE *fp)               */
/*                                                                           */
/*  Brief debug of sc, just showing its identity number.                     */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassDebugBrief(KHE_SPLIT_CLASS sc, FILE *fp)
{
  fprintf(fp, "Split Class %d", sc->id_num);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitClassDebug(KHE_SPLIT_CLASS sc, int indent, FILE *fp)        */
/*                                                                           */
/*  Debug print of sc at the given indent.                                   */
/*                                                                           */
/*****************************************************************************/

void KheSplitClassDebug(KHE_SPLIT_CLASS sc, int indent, FILE *fp, HA_ARENA a)
{
  KHE_EVENT e;  int i, d, min_amount, max_amount;  bool first;
  KHE_PARTITION p;  KHE_SPLIT_LAYER sl;  KHE_SPLIT_CLASS child_sc;
  fprintf(fp, "%*s[ Split Class %d (duration %d)\n", indent, "",
    sc->id_num, sc->duration);
  fprintf(fp, "%*s  require %d-%d(", indent, "",
    sc->min_total_amount, sc->max_total_amount);
  first = true;
  for( d = 1;  d <= sc->duration;  d++ )
  {
    min_amount = HaArray(sc->min_amount, d - 1);
    max_amount = HaArray(sc->max_amount, d - 1);
    if( min_amount != 0 || max_amount != 0 )
    {
      fprintf(fp, "%s%d:%d-%d", first ? "" : ", ", d, min_amount, max_amount);
      first = false;
    }
  }
  fprintf(fp, ")");
  HaArrayForEach(sc->upper_partitions, p, i)
    fprintf(fp, " %s", KhePartitionShow(p, a));
  fprintf(fp, "\n");
  fprintf(fp, "%*s  min partition %s\n", indent, "",
    KhePartitionShow(sc->min_partition, a));
  HaArrayForEach(sc->events, e, i)
    fprintf(fp, "%*s  %s%s\n", indent, "",
      KheEventId(e) != NULL ? KheEventId(e) : "-",
      e == sc->leader ? " (leader)" : "");
  HaArrayForEach(sc->child_layers, sl, i)
    KheSplitLayerDebugBrief(sl, indent + 2, fp, a);
  HaArrayForEach(sc->child_classes, child_sc, i)
    KheSplitClassDebug(child_sc, indent + 2, fp, a);
  fprintf(fp, "%*s]\n", indent, "");
}

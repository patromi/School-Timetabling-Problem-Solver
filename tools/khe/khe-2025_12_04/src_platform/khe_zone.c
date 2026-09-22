
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
/*  FILE:         khe_zone.c                                                 */
/*  DESCRIPTION:  A zone of a node                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE                                                                 */
/*                                                                           */
/*****************************************************************************/

struct khe_zone_rec {
  void			*back;			/* back pointer              */
  int			visit_num;		/* visit number              */
  int			reference_count;	/* reference count 	     */
  KHE_NODE		node;			/* the node                  */
  int			node_index;		/* index in node             */
  ARRAY_KHE_MEET	meets;			/* meets                     */
  HA_ARRAY_INT		offsets;		/* offsets                   */
  KHE_ZONE		copy;			/* used when copying         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "back pointers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelSetBack(KHE_ZONE zone, void *back)                     */
/*                                                                           */
/*  Set the back pointer of zone to back, assuming all is well.              */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelSetBack(KHE_ZONE zone, void *back)
{
  KheSolnOpZoneSetBack(KheZoneSoln(zone), zone, zone->back, back);
  zone->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelSetBackUndo(KHE_ZONE zone, void *old_back,             */
/*    void *new_back)                                                        */
/*                                                                           */
/*  Undo KheZoneKernelSetBack.                                               */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelSetBackUndo(KHE_ZONE zone, void *old_back, void *new_back)
{
  zone->back = old_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneSetBack(KHE_ZONE zone, void *back)                           */
/*                                                                           */
/*  Set the back pointer of zone.                                            */
/*                                                                           */
/*****************************************************************************/

void KheZoneSetBack(KHE_ZONE zone, void *back)
{
  if( back != zone->back )
    KheZoneKernelSetBack(zone, back);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheZoneBack(KHE_ZONE zone)                                         */
/*                                                                           */
/*  Return the back pointer of zone.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheZoneBack(KHE_ZONE zone)
{
  return zone->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "visit numbers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheZoneSetVisitNum(KHE_ZONE zone, int num)                          */
/*                                                                           */
/*  Set the visit number of zone.                                            */
/*                                                                           */
/*****************************************************************************/

void KheZoneSetVisitNum(KHE_ZONE zone, int num)
{
  zone->visit_num = num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheZoneVisitNum(KHE_ZONE zone)                                       */
/*                                                                           */
/*  Return the visit number of zone.                                         */
/*                                                                           */
/*****************************************************************************/

int KheZoneVisitNum(KHE_ZONE zone)
{
  return zone->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheZoneVisited(KHE_ZONE zone, int slack)                            */
/*                                                                           */
/*  Return true if zone has been visited recently.                           */
/*                                                                           */
/*****************************************************************************/

bool KheZoneVisited(KHE_ZONE zone, int slack)
{
  return KheSolnGlobalVisitNum(KheNodeSoln(zone->node))-zone->visit_num<=slack;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneVisit(KHE_ZONE zone)                                         */
/*                                                                           */
/*  Visit zone.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheZoneVisit(KHE_ZONE zone)
{
  zone->visit_num = KheSolnGlobalVisitNum(KheNodeSoln(zone->node));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneUnVisit(KHE_ZONE zone)                                       */
/*                                                                           */
/*  Unvisit zone.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheZoneUnVisit(KHE_ZONE zone)
{
  zone->visit_num = KheSolnGlobalVisitNum(KheNodeSoln(zone->node)) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other simple attributes"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheZoneSoln(KHE_ZONE zone)                                      */
/*                                                                           */
/*  Return the solution of zone's node.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheZoneSoln(KHE_ZONE zone)
{
  return KheNodeSoln(zone->node);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheZoneNode(KHE_ZONE zone)                                      */
/*                                                                           */
/*  Return the node attribute of zone.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheZoneNode(KHE_ZONE zone)
{
  return zone->node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneSetNode(KHE_ZONE zone, KHE_NODE node)                        */
/*                                                                           */
/*  Set the node attribute of zone.                                          */
/*                                                                           */
/*****************************************************************************/

void KheZoneSetNode(KHE_ZONE zone, KHE_NODE node)
{
  zone->node = node;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheZoneNodeIndex(KHE_ZONE zone)                                      */
/*                                                                           */
/*  Return the node_index attribute of zone.                                 */
/*                                                                           */
/*****************************************************************************/

int KheZoneNodeIndex(KHE_ZONE zone)
{
  return zone->node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneSetNodeIndex(KHE_ZONE zone, int node_index)                  */
/*                                                                           */
/*  Set the node_index attribute of zone.                                    */
/*                                                                           */
/*****************************************************************************/

void KheZoneSetNodeIndex(KHE_ZONE zone, int node_index)
{
  zone->node_index = node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and deletion"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheZoneDoMake(HA_ARENA a)                                       */
/*                                                                           */
/*  Obtain a new zone from a; initialize its arrays.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_ZONE KheZoneDoMake(HA_ARENA a)
{
  KHE_ZONE res;
  HaMake(res, a);
  HaArrayInit(res->meets, a);
  HaArrayInit(res->offsets, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneUnMake(KHE_ZONE zone)                                        */
/*                                                                           */
/*  Undo KheZoneDoMake, returning zone's memory to the memory allocator.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheZoneUnMake(KHE_ZONE zone)
{
  MArrayFree(zone->meets);
  MArrayFree(zone->offsets);
  MFree(zone);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheZoneDoGet(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Get a zone object, either from soln's free list or allocated.            */
/*                                                                           */
/*****************************************************************************/

static KHE_ZONE KheZoneDoGet(KHE_SOLN soln)
{
  KHE_ZONE res;
  res = KheSolnGetZoneFromFreeList(soln);
  if( res == NULL )
    res = KheZoneDoMake(KheSolnArena(soln));
  res->reference_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneUnGet(KHE_ZONE zone)                                         */
/*                                                                           */
/*  Undo KheZoneDoGet, adding zone to its soln's free list.                  */
/*                                                                           */
/*****************************************************************************/

static void KheZoneUnGet(KHE_ZONE zone)
{
  KheSolnAddZoneToFreeList(KheZoneSoln(zone), zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneReferenceCountIncrement(KHE_ZONE zone)                       */
/*                                                                           */
/*  Increment zone's reference count.                                        */
/*                                                                           */
/*****************************************************************************/

void KheZoneReferenceCountIncrement(KHE_ZONE zone)
{
  zone->reference_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneReferenceCountDecrement(KHE_ZONE zone)                       */
/*                                                                           */
/*  Decrement zone's reference count, and possibly add it to the free list.  */
/*                                                                           */
/*****************************************************************************/

void KheZoneReferenceCountDecrement(KHE_ZONE zone)
{
  HnAssert(zone->reference_count >= 1,
    "KheZoneReferenceCountDecrement internal error");
  if( --zone->reference_count == 0 )
    KheZoneUnGet(zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDoAdd(KHE_ZONE zone, KHE_NODE node)                          */
/*                                                                           */
/*  Initialize zone and add it to node.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheZoneDoAdd(KHE_ZONE zone, KHE_NODE node)
{
  zone->back = NULL;
  zone->visit_num = 0;
  KheZoneReferenceCountIncrement(zone);
  KheNodeAddZone(node, zone);  /* will set node and node_index */
  HaArrayClear(zone->meets);
  HaArrayClear(zone->offsets);
  zone->copy = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneUnAdd(KHE_ZONE zone)                                         */
/*                                                                           */
/*  Undo KheZoneDoAdd, leaving zone unlinked from the solution.              */
/*                                                                           */
/*****************************************************************************/

static void KheZoneUnAdd(KHE_ZONE zone)
{
  /* delete from node */
  KheNodeDeleteZone(zone->node, zone);

  /* zone is now not referenced from solution (this call may free zone) */
  KheZoneReferenceCountDecrement(zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelAdd(KHE_ZONE zone, KHE_NODE node)                      */
/*                                                                           */
/*  Kernel operation which adds zone to node (but does not make it).         */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelAdd(KHE_ZONE zone, KHE_NODE node)
{
  KheZoneDoAdd(zone, node);
  KheSolnOpZoneAdd(KheNodeSoln(node), zone, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelAddUndo(KHE_ZONE zone)                                 */
/*                                                                           */
/*  Undo KheZoneKernelAdd.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelAddUndo(KHE_ZONE zone)
{
  KheZoneUnAdd(zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelDelete(KHE_ZONE zone)                                  */
/*                                                                           */
/*  Kernel operation which deletes zone (but does not free it).              */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelDelete(KHE_ZONE zone)
{
  KheSolnOpZoneDelete(KheZoneSoln(zone), zone, zone->node);
  KheZoneUnAdd(zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelDeleteUndo(KHE_ZONE zone, KHE_NODE node)               */
/*                                                                           */
/*  Undo KheZoneKernelDelete.                                                */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelDeleteUndo(KHE_ZONE zone, KHE_NODE node)
{
  KheZoneDoAdd(zone, node);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheZoneMake(KHE_NODE node)                                      */
/*                                                                           */
/*  Make a new zone within node.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheZoneMake(KHE_NODE node)
{
  KHE_ZONE res;

  /* make and initialize a new zone object from scratch */
  res = KheZoneDoGet(KheNodeSoln(node));

  /* carry out the kernel add operation */
  KheZoneKernelAdd(res, node);

  /* return it */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDelete(KHE_ZONE zone)                                        */
/*                                                                           */
/*  Delete zone.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheZoneDelete(KHE_ZONE zone)
{
  KHE_MEET meet;  int offset;

  /* delete the meet offsets of zone */
  while( HaArrayCount(zone->meets) > 0 )
  {
    meet = HaArrayLast(zone->meets);
    offset = HaArrayLast(zone->offsets);
    HnAssert(KheMeetOffsetZone(meet, offset) == zone,
      "KheZoneDelete internal error");
    KheZoneDeleteMeetOffset(zone, meet, offset);
  }

  /* carry out the kernel delete operation (may free zone) */
  KheZoneKernelDelete(zone);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheZoneCopyPhase1(KHE_ZONE zone, HA_ARENA a)                    */
/*                                                                           */
/*  Carry out Phase 1 of the operation of copying zone.                      */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheZoneCopyPhase1(KHE_ZONE zone, HA_ARENA a)
{
  KHE_ZONE copy;  int i;  KHE_MEET meet;
  if( zone->copy == NULL )
  {
    HaMake(copy, a);
    zone->copy = copy;
    copy->back = zone->back;
    copy->visit_num = zone->visit_num;
    copy->node = KheNodeCopyPhase1(zone->node, a);
    copy->node_index = zone->node_index;
    HaArrayInit(copy->meets, a);
    HaArrayForEach(zone->meets, meet, i)
      HaArrayAddLast(copy->meets, KheMeetCopyPhase1(meet, a));
    HaArrayInit(copy->offsets, a);
    HaArrayAppend(copy->offsets, zone->offsets, i);
    copy->copy = NULL;
  }
  return zone->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneCopyPhase2(KHE_ZONE zone)                                    */
/*                                                                           */
/*  Carry out Phase 2 of the operation of copying zone.                      */
/*                                                                           */
/*****************************************************************************/

void KheZoneCopyPhase2(KHE_ZONE zone)
{
  KHE_MEET meet;  int i;
  if( zone->copy != NULL )
  {
    zone->copy = NULL;
    KheNodeCopyPhase2(zone->node);
    HaArrayForEach(zone->meets, meet, i)
      KheMeetCopyPhase2(meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet offsets"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheZoneMeetOffsetCount(KHE_ZONE zone)                                */
/*                                                                           */
/*  Return the number of meet offsets of zone.                               */
/*                                                                           */
/*****************************************************************************/

int KheZoneMeetOffsetCount(KHE_ZONE zone)
{
  return HaArrayCount(zone->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneMeetOffset(KHE_ZONE zone, int i, KHE_MEET *meet, int *offset)*/
/*                                                                           */
/*  Return the i'th meet-offset of zone.                                     */
/*                                                                           */
/*****************************************************************************/

void KheZoneMeetOffset(KHE_ZONE zone, int i, KHE_MEET *meet, int *offset)
{
  *meet = HaArray(zone->meets, i);
  *offset = HaArray(zone->offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDoAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)    */
/*                                                                           */
/*  Add (meet, offset) to zone, assuming all is safe.                        */
/*                                                                           */
/*****************************************************************************/

void KheZoneDoAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  /* add (meet, offset) to zone */
  HaArrayAddLast(zone->meets, meet);
  HaArrayAddLast(zone->offsets, offset);

  /* update meet's zone */
  KheMeetOffsetAddZone(meet, offset, zone);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDoDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset) */
/*                                                                           */
/*  Delete (meet, offset) from zone, assuming all is safe.                   */
/*                                                                           */
/*****************************************************************************/

void KheZoneDoDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  int i, offset2;  KHE_MEET meet2;

  /* delete (meet, offset) from zone, and update meet's zone list */
  HaArrayForEach(zone->meets, meet2, i)
  {
    offset2 = HaArray(zone->offsets, i);
    if( meet2 == meet && offset2 == offset )
    {
      HaArrayDeleteAndShift(zone->meets, i);
      HaArrayDeleteAndShift(zone->offsets, i);
      KheMeetOffsetDeleteZone(meet, offset);
      return;
    }
  }
  HnAbort("KheZoneDoDeleteMeetOffset internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelAddMeetOffset(KHE_ZONE zone, KHE_MEET meet,            */
/*    int offset)                                                            */
/*                                                                           */
/*  Add (meet, offset) to zone.                                              */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  KheSolnOpZoneAddMeetOffset(KheZoneSoln(zone), zone, meet, offset);
  KheZoneDoAddMeetOffset(zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelAddMeetOffsetUndo(KHE_ZONE zone, KHE_MEET meet,        */
/*    int offset)                                                            */
/*                                                                           */
/*  Undo KheZoneKernelAddMeetOffset.                                         */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelAddMeetOffsetUndo(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  KheZoneDoDeleteMeetOffset(zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet,         */
/*    int offset)                                                            */
/*                                                                           */
/*  Delete (meet, offset) from zone.                                         */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  KheSolnOpZoneDeleteMeetOffset(KheZoneSoln(zone), zone, meet, offset);
  KheZoneDoDeleteMeetOffset(zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneKernelDeleteMeetOffsetUndo(KHE_ZONE zone, KHE_MEET meet,     */
/*    int offset)                                                            */
/*                                                                           */
/*  Undo KheZoneKernelDeleteMeetOffset.                                      */
/*                                                                           */
/*****************************************************************************/

void KheZoneKernelDeleteMeetOffsetUndo(KHE_ZONE zone, KHE_MEET meet,
  int offset)
{
  KheZoneDoAddMeetOffset(zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneUpdateMeetOffset(KHE_ZONE zone, KHE_MEET old_meet,           */
/*    int old_offset, KHE_MEET new_meet, int new_offset)                     */
/*                                                                           */
/*  Replace (old_meet, old_offset) in zone by (new_meet, new_offset).        */
/*                                                                           */
/*****************************************************************************/

void KheZoneUpdateMeetOffset(KHE_ZONE zone, KHE_MEET old_meet,
  int old_offset, KHE_MEET new_meet, int new_offset)
{
  int i, offset;  KHE_MEET meet;
  for( i = 0;  i < HaArrayCount(zone->meets);  i++ )
  {
    meet = HaArray(zone->meets, i);
    offset = HaArray(zone->offsets, i);
    if( meet == old_meet && offset == old_offset )
    {
      HaArrayPut(zone->meets, i, new_meet);
      HaArrayPut(zone->offsets, i, new_offset);
      return;
    }
  }
  HnAbort("KheZoneUpdateMeetOffset internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)      */
/*                                                                           */
/*  Add (meet, offset) to zone.                                              */
/*                                                                           */
/*****************************************************************************/

void KheZoneAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheZoneAddMeetOffset(");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", offset);
  }

  /* ensure operation is safe */
  HnAssert(offset >= 0 && offset < KheMeetDuration(meet),
    "KheZoneAddMeetOffset: offset (%d) out of range (0 .. %d)",
    offset, KheMeetDuration(meet) - 1);
  HnAssert(KheMeetOffsetZone(meet, offset) == NULL,
    "KheZoneAddMeetOffset: (meet, %d) already has a zone", offset);
  HnAssert(KheMeetNode(meet) != NULL, "KheZoneAddMeetOffset: meet not in node");
  HnAssert(KheZoneNode(zone) == KheMeetNode(meet),
    "KheZoneAddMeetOffset: zone's node and meet's node are different");

  /* carry out the kernel operation */
  KheZoneKernelAddMeetOffset(zone, meet, offset);
  if( DEBUG3 )
    fprintf(stderr, "] KheZoneAddMeetOffset returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)   */
/*                                                                           */
/*  Delete (meet, offset) from zone.                                         */
/*                                                                           */
/*****************************************************************************/

void KheZoneDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset)
{
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheZoneDeleteMeetOffset(");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", offset);
  }

  /* ensure operation is safe */
  HnAssert(offset >= 0 && offset < KheMeetDuration(meet),
    "KheZoneDeleteMeetOffset: offset (%d) out of range (0 .. %d)",
    offset, KheMeetDuration(meet) - 1);
  HnAssert(KheMeetOffsetZone(meet, offset) == zone,
    "KheZoneDeleteMeetOffset: (meet, %d) not in zone", offset);

  /* carry out the kernel operation */
  KheZoneKernelDeleteMeetOffset(zone, meet, offset);
  if( DEBUG3 )
    fprintf(stderr, "] KheZoneDeleteMeetOffset returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "helper functions involving zones"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSegmentContainsZone(KHE_MEET meet, int offset, int durn,     */
/*    KHE_ZONE zone)                                                         */
/*                                                                           */
/*  Return true if the segment of meet beginning at offset of duration       */
/*  durn contains zone.                                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetSegmentContainsZone(KHE_MEET meet, int offset, int durn,
  KHE_ZONE zone)
{
  int i;
  for( i = 0;  i < durn;  i++ )
    if( KheMeetOffsetZone(meet, offset + i) == zone )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMovePreservesZones(KHE_MEET meet1, int offset1,              */
/*    KHE_MEET meet2, int offset2, int durn)                                 */
/*                                                                           */
/*  Return true if changing the assignment of a meet of duration durn        */
/*  from meet1 at offset1 to meet2 at offset2 would not change its zones.    */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMovePreservesZones(KHE_MEET meet1, int offset1,
  KHE_MEET meet2, int offset2, int durn)
{
  int i;  KHE_ZONE zone;
  for( i = 0;  i < durn;  i++ )
  {
    zone = KheMeetOffsetZone(meet1, offset1 + i);
    if( !KheMeetSegmentContainsZone(meet2, offset2, durn, zone) )
      return false;
  }
  for( i = 0;  i < durn;  i++ )
  {
    zone = KheMeetOffsetZone(meet2, offset2 + i);
    if( !KheMeetSegmentContainsZone(meet1, offset1, durn, zone) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheZoneDebug(KHE_ZONE zone, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of zone onto fp with the given indent.                       */
/*                                                                           */
/*****************************************************************************/

void KheZoneDebug(KHE_ZONE zone, int verbosity, int indent, FILE *fp)
{
  int i, j, offset;  KHE_MEET meet, next_meet;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
    {
      fprintf(fp, "%*s[ Zone %d of Node %d: ", indent, "", zone->node_index,
	KheNodeSolnIndex(zone->node));
      for( i = 0;  i < HaArrayCount(zone->meets);  i = j )
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	meet = HaArray(zone->meets, i);
	KheMeetDebug(meet, 1, -1, fp);
	for( j = i;  j < HaArrayCount(zone->meets);  j++ )
	{
	  next_meet = HaArray(zone->meets, j);
	  offset = HaArray(zone->offsets, j);
	  if( next_meet != meet )
	    break;
	  fprintf(fp, "+%d", offset);
	}
      }
      fprintf(fp, " ]\n");
    }
    else
      fprintf(fp, "[Zone %d of Node %d]", zone->node_index,
	KheNodeSolnIndex(zone->node));
  }
}

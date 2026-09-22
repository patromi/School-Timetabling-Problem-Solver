
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
/*  FILE:         khe_meet_set.c                                             */
/*  DESCRIPTION:  Meet sets                                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SET                                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_meet_set_rec
{
  KHE_SOLN			soln;			/* encl solution     */
  ARRAY_KHE_MEET		meets;			/* soln meets        */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SET KheMeetSetMake(KHE_SOLN soln)                               */
/*                                                                           */
/*  Make a new meet set with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_SET KheMeetSetMake(KHE_SOLN soln)
{
  KHE_MEET_SET res;  HA_ARENA a;
  res = KheSolnGetMeetSetFromFreeList(soln);
  if( res == NULL )
  {
    /* MMakeCount(res, 50); */
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->meets, a);
  }
  else
    HaArrayClear(res->meets);
  res->soln = soln;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetDelete(KHE_MEET_SET ms)                                   */
/*                                                                           */
/*  Delete ms, that is, return it to its solution's free list.               */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetDelete(KHE_MEET_SET ms)
{
  KheSolnAddMeetSetToFreeList(ms->soln, ms);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetFree(KHE_MEET_SET ms)                                     */
/*                                                                           */
/*  Free ms.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMeetSetFree(KHE_MEET_SET ms)
{
  MArrayFree(ms->meets);
  MFree(ms);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetClear(KHE_MEET_SET ms)                                    */
/*                                                                           */
/*  Clear ms back to the empty set of meets.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetClear(KHE_MEET_SET ms)
{
  HaArrayClear(ms->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetDropFromEnd(KHE_MEET_SET ms, int n)                       */
/*                                                                           */
/*  Remove the last n meets from ms.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetDropFromEnd(KHE_MEET_SET ms, int n)
{
  HaArrayDeleteLastSlice(ms->meets, n);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetAddMeet(KHE_MEET_SET ms, KHE_MEET meet)                   */
/*                                                                           */
/*  Add meet to ms.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetAddMeet(KHE_MEET_SET ms, KHE_MEET meet)
{
  HaArrayAddLast(ms->meets, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetDeleteMeet(KHE_MEET_SET ms, KHE_MEET meet)                */
/*                                                                           */
/*  Delete meet from ms.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetDeleteMeet(KHE_MEET_SET ms, KHE_MEET meet)
{
  int pos;
  if( !HaArrayContains(ms->meets, meet, &pos) )
    HnAbort("KheMeetSetDeleteMeet:  ms does not contain meet");
  HaArrayDeleteAndPlug(ms->meets, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetContainsMeet(KHE_MEET_SET ms, KHE_MEET meet, int *pos)    */
/*                                                                           */
/*  Return true and set *pos if ms contains meet.                            */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSetContainsMeet(KHE_MEET_SET ms, KHE_MEET meet, int *pos)
{
  return HaArrayContains(ms->meets, meet, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSetMeetCount(KHE_MEET_SET ms)                                 */
/*                                                                           */
/*  Return the number of meets in ms.                                        */
/*                                                                           */
/*****************************************************************************/

int KheMeetSetMeetCount(KHE_MEET_SET ms)
{
  return HaArrayCount(ms->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetSetMeet(KHE_MEET_SET ms, int i)                          */
/*                                                                           */
/*  Return the i'th meet of ms.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetSetMeet(KHE_MEET_SET ms, int i)
{
  return HaArray(ms->meets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSort(KHE_MEET_SET ms,                                     */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the meets of ms.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSort(KHE_MEET_SET ms, int(*compar)(const void *, const void *))
{
  HaArraySort(ms->meets, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSortUnique(KHE_MEET_SET ms,                               */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort and uniquefy the meets of ms.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSortUnique(KHE_MEET_SET ms,
  int(*compar)(const void *, const void *))
{
  HaArraySortUnique(ms->meets, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSetDuration(KHE_MEET_SET ms)                                  */
/*                                                                           */
/*  Return the total duration of the meets of ms.                            */
/*                                                                           */
/*****************************************************************************/

int KheMeetSetDuration(KHE_MEET_SET ms)
{
  int res, i;  KHE_MEET meet;
  res = 0;
  HaArrayForEach(ms->meets, meet, i)
    res += KheMeetDuration(meet);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheMeetSetWorkload(KHE_MEET_SET ms)                                */
/*                                                                           */
/*  Return the total workload of the meets of ms.                            */
/*                                                                           */
/*****************************************************************************/

/* *** not well-defined
float KheMeetSetWorkload(KHE_MEET_SET ms)
{
  float res;  int i;  KHE_MEET meet;
  res = 0.0;
  HaArrayForEach(ms->meets, meet, i)
    res += KheMeetWorkload(meet);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSetVisitNum(KHE_MEET_SET ms, int num)                     */
/*                                                                           */
/*  Call KheMeetSetVisitNum on each meet of ms.                              */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSetVisitNum(KHE_MEET_SET ms, int num)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(ms->meets, meet, i)
    KheMeetSetVisitNum(meet, num);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSetGetVisitNum(KHE_MEET_SET ms)                               */
/*                                                                           */
/*  Call KheMeetVisitNum on any meet of ms.                                  */
/*                                                                           */
/*****************************************************************************/

int KheMeetSetGetVisitNum(KHE_MEET_SET ms)
{
  HnAssert(HaArrayCount(ms->meets) > 0, "KheMeetSetGetVisitNum:  ms is empty");
  return KheMeetVisitNum(HaArrayFirst(ms->meets));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetVisited(KHE_MEET_SET ms, int slack)                       */
/*                                                                           */
/*  Return true if any call to KheMeetVisited(meet, slack) for meet in       */
/*  ms returns true.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSetVisited(KHE_MEET_SET ms, int slack)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(ms->meets, meet, i)
    if( KheMeetVisited(meet, slack) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetVisit(KHE_MEET_SET ms)                                    */
/*                                                                           */
/*  Mark the meets of ms visited.                                            */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetVisit(KHE_MEET_SET ms)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(ms->meets, meet, i)
    KheMeetVisit(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetUnVisit(KHE_MEET_SET ms)                                  */
/*                                                                           */
/*  Mark the meets of ms unvisited.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetUnVisit(KHE_MEET_SET ms)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(ms->meets, meet, i)
    KheMeetUnVisit(meet);
}


/*****************************************************************************/
/*                                                                           */
/* void KheMeetSetDebug(KHE_MEET_SET ms, int verbosity, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of meet set ms onto fp with the given verbosity and indent.  */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetDebug(KHE_MEET_SET ms, int verbosity, int indent, FILE *fp)
{
  KHE_MEET meet;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{");
    HaArrayForEach(ms->meets, meet, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheMeetDebug(meet, verbosity, -1, fp);
    }
    fprintf(fp, "}");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

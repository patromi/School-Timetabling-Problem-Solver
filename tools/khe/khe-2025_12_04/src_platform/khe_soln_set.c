
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
/*  FILE:         khe_soln_set.c                                             */
/*  DESCRIPTION:  Soln sets                                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_SET                                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_soln_set_rec
{
  ARRAY_KHE_SOLN		solns;			/* soln solns        */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_SET KheSolnSetMake(HA_ARENA a)                                  */
/*                                                                           */
/*  Make a new, initially empty soln set in arena a.                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_SET KheSolnSetMake(HA_ARENA a)
{
  KHE_SOLN_SET res;
  HaMake(res, a);
  HaArrayInit(res->solns, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetDelete(KHE_SOLN_SET ss)                                   */
/*                                                                           */
/*  Delete ss, but not its solns.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnSetDelete(KHE_SOLN_SET ss)
{
  MArrayFree(ss->solns);
  MFree(ss);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetClear(KHE_SOLN_SET ss)                                    */
/*                                                                           */
/*  Clear ss back to the empty set of solns.                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetClear(KHE_SOLN_SET ss)
{
  HaArrayClear(ss->solns);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetAddSoln(KHE_SOLN_SET ss, KHE_SOLN soln)                   */
/*                                                                           */
/*  Add soln to ss.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetAddSoln(KHE_SOLN_SET ss, KHE_SOLN soln)
{
  HaArrayAddLast(ss->solns, soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetDeleteSoln(KHE_SOLN_SET ss, KHE_SOLN soln)                */
/*                                                                           */
/*  Delete soln from ss.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetDeleteSoln(KHE_SOLN_SET ss, KHE_SOLN soln)
{
  int pos;
  if( !HaArrayContains(ss->solns, soln, &pos) )
    HnAbort("KheSolnSetDeleteSoln:  ss does not contain soln");
  HaArrayDeleteAndPlug(ss->solns, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnSetContainsSoln(KHE_SOLN_SET ss, KHE_SOLN soln, int *pos)    */
/*                                                                           */
/*  Return true and set *pos if ss contains soln.                            */
/*                                                                           */
/*****************************************************************************/

bool KheSolnSetContainsSoln(KHE_SOLN_SET ss, KHE_SOLN soln, int *pos)
{
  return HaArrayContains(ss->solns, soln, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnSetSolnCount(KHE_SOLN_SET ss)                                 */
/*                                                                           */
/*  Return the number of solns in ss.                                        */
/*                                                                           */
/*****************************************************************************/

int KheSolnSetSolnCount(KHE_SOLN_SET ss)
{
  return HaArrayCount(ss->solns);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnSetSoln(KHE_SOLN_SET ss, int i)                          */
/*                                                                           */
/*  Return the i'th soln of ss.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnSetSoln(KHE_SOLN_SET ss, int i)
{
  return HaArray(ss->solns, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetSort(KHE_SOLN_SET ss,                                     */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the solns of ss.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetSort(KHE_SOLN_SET ss, int(*compar)(const void *, const void *))
{
  HaArraySort(ss->solns, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetSortUnique(KHE_SOLN_SET ss,                               */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort and uniquefy the solns of ss.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetSortUnique(KHE_SOLN_SET ss,
  int(*compar)(const void *, const void *))
{
  HaArraySortUnique(ss->solns, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIncreasingCostTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2)            */
/*                                                                           */
/*  Comparison function for sorting solutions by increasing cost.            */
/*                                                                           */
/*****************************************************************************/

int KheIncreasingCostTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2)
{
  int cmp;  /* bool invalid1, invalid2; */  KHE_SOLN_TYPE type1, type2;

  /* if types differ, sort on that basis */
  type1 = KheSolnType(soln1);
  type2 = KheSolnType(soln2);
  cmp = (int) type1 - (int) type2;
  if( cmp != 0 )  return cmp;

  /* invalid is larger than valid */
  /* ***
  invalid1 = KheSolnIsInvalid(soln1);
  invalid2 = KheSolnIsInvalid(soln2);
  cmp = (int) invalid1 - (int) invalid2;
  if( cmp != 0 )  return cmp;
  *** */

  /* now either both are valid or both are invalid; handle both invalid */
  if( type1 == KHE_SOLN_INVALID_PLACEHOLDER )  return 0;

  /* both valid; compare costs */
  cmp = KheCostCmp(KheSolnCost(soln1), KheSolnCost(soln2));
  if( cmp != 0 )  return cmp;

  /* equal costs, so compare running times */
  return KheSolnIncreasingRunningTimeTypedCmp(soln1, soln2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIncreasingCostCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Comparison function for sorting solutions by increasing cost.            */
/*                                                                           */
/*****************************************************************************/

int KheIncreasingCostCmp(const void *t1, const void *t2)
{
  KHE_SOLN soln1 = * (KHE_SOLN *) t1;
  KHE_SOLN soln2 = * (KHE_SOLN *) t2;
  return KheIncreasingCostTypedCmp(soln1, soln2);
}


/*****************************************************************************/
/*                                                                           */
/* void KheSolnSetDebug(KHE_SOLN_SET ss, int verbosity, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of soln set ss onto fp with the given verbosity and indent.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetDebug(KHE_SOLN_SET ss, int verbosity, int indent, FILE *fp)
{
  KHE_SOLN soln;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{");
    HaArrayForEach(ss->solns, soln, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheSolnDebug(soln, verbosity, -1, fp);
    }
    fprintf(fp, "}");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

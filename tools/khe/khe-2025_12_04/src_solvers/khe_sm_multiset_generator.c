
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
/*  FILE:         khe_sm_multiset_generator.c                                */
/*  DESCRIPTION:  Multiset generator                                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MULTISET_ELT                                                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_multiset_elt_rec {
  int			min_value;
  int			max_value;
  int			curr_value;
} *KHE_MULTISET_ELT;

typedef HA_ARRAY(KHE_MULTISET_ELT) ARRAY_KHE_MULTISET_ELT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MULTISET_GENERATOR                                              */
/*                                                                           */
/*****************************************************************************/

struct khe_multiset_generator_rec {
  HA_ARENA			arena;
  int				element_sum;
  int				curr_sum;
  bool				running;
  ARRAY_KHE_MULTISET_ELT	elt_free_list;
  ARRAY_KHE_MULTISET_ELT	elts;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MULTISET_ELT"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MULTISET_ELT KheMultisetEltMake(KHE_MULTISET_GENERATOR mg,           */
/*    int min_value, int max_value)                                          */
/*                                                                           */
/*  Make a new multiset generator element.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_MULTISET_ELT KheMultisetEltMake(KHE_MULTISET_GENERATOR mg,
  int min_value, int max_value)
{
  KHE_MULTISET_ELT res;
  if( HaArrayCount(mg->elt_free_list) > 0 )
    res = HaArrayLastAndDelete(mg->elt_free_list);
  else
    HaMake(res, mg->arena);
  res->min_value = min_value;
  res->max_value = max_value;
  res->curr_value = -1;  /* undefined */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MULTISET_GENERATOR"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MULTISET_GENERATOR KheMultisetGeneratorMake(HA_ARENA a)              */
/*                                                                           */
/*  Make a new multiset generator object.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_MULTISET_GENERATOR KheMultisetGeneratorMake(HA_ARENA a)
{
  KHE_MULTISET_GENERATOR res;
  HaMake(res, a);
  res->arena = a;
  res->element_sum = -1;  /* undefined */
  res->running = false;
  HaArrayInit(res->elt_free_list, a);
  HaArrayInit(res->elts, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultisetGeneratorSetupBegin(KHE_MULTISET_GENERATOR mg,           */
/*    int element_sum)                                                       */
/*                                                                           */
/*  Start the setup of a new generate.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMultisetGeneratorSetupBegin(KHE_MULTISET_GENERATOR mg,
  int element_sum)
{
  int i;
  HaArrayAppend(mg->elt_free_list, mg->elts, i);
  HaArrayClear(mg->elts);
  mg->element_sum = element_sum;
  mg->running = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultisetGeneratorSetupAdd(KHE_MULTISET_GENERATOR mg,             */
/*    int min_value, int max_value)                                          */
/*                                                                           */
/*  Add one element to each generated multiset.                              */
/*                                                                           */
/*****************************************************************************/

void KheMultisetGeneratorSetupAdd(KHE_MULTISET_GENERATOR mg,
  int min_value, int max_value)
{
  KHE_MULTISET_ELT elt;
  HnAssert(!mg->running, "KheMultisetGeneratorSetupAdd internal error 1");
  HnAssert(min_value <= max_value,
    "KheMultisetGeneratorSetupAdd internal error 2");
  if( max_value > mg->element_sum )
    max_value = mg->element_sum;
  elt = KheMultisetEltMake(mg, min_value, max_value);
  HaArrayAddLast(mg->elts, elt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultisetGeneratorSetupEnd(KHE_MULTISET_GENERATOR mg)             */
/*                                                                           */
/*  End the setup of mg.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheMultisetGeneratorSetupEnd(KHE_MULTISET_GENERATOR mg)
{
  KHE_MULTISET_ELT elt;  int i;
  HnAssert(!mg->running, "KheMultisetGeneratorSetupEnd internal error");
  mg->curr_sum = 0;
  HaArrayForEach(mg->elts, elt, i)
  {
    if( i == HaArrayCount(mg->elts) - 1 )
      elt->curr_value = elt->min_value - 1;
    else
      elt->curr_value = elt->min_value;
    mg->curr_sum += elt->curr_value;
  }
  mg->running = true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoNext(KHE_MULTISET_GENERATOR mg)                                */
/*                                                                           */
/*  Count up one place, keeping track (in mg->curr_sum) of what the sum of   */
/*  the elements is.                                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheDoNext(KHE_MULTISET_GENERATOR mg)
{
  KHE_MULTISET_ELT elt;  int i;
  HaArrayForEachReverse(mg->elts, elt, i)
  {
    if( elt->curr_value >= elt->max_value )
    {
      mg->curr_sum += (elt->min_value - elt->curr_value);
      elt->curr_value = elt->min_value;
    }
    else
    {
      mg->curr_sum += 1;
      elt->curr_value++;
      break;
    }
  }
  return i >= 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMultisetGeneratorNext(KHE_MULTISET_GENERATOR mg)                 */
/*                                                                           */
/*  Generate one multiset.                                                   */
/*                                                                           */
/*  Implementation note.  This is not a very efficient way to do this,       */
/*  but at the time of writing I couldn't think of anything better.          */
/*                                                                           */
/*****************************************************************************/

bool KheMultisetGeneratorNext(KHE_MULTISET_GENERATOR mg)
{
  bool res;
  HnAssert(mg->running, "KheMultisetGeneratorNext internal error");
  while( (res = KheDoNext(mg)) && mg->curr_sum != mg->element_sum );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMultisetGeneratorNextValue(KHE_MULTISET_GENERATOR mg, int j)      */
/*                                                                           */
/*  Return the j'th element of the current multiset.                         */
/*                                                                           */
/*****************************************************************************/

int KheMultisetGeneratorNextValue(KHE_MULTISET_GENERATOR mg, int j)
{
  KHE_MULTISET_ELT elt;
  elt = HaArray(mg->elts, j);
  return elt->curr_value;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMultisetGeneratorSetupAddCount(KHE_MULTISET_GENERATOR mg)         */
/*                                                                           */
/*  Return the number of elements.                                           */
/*                                                                           */
/*****************************************************************************/

int KheMultisetGeneratorSetupAddCount(KHE_MULTISET_GENERATOR mg)
{
  return HaArrayCount(mg->elts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMulisetGeneratorTest(HA_ARENA a)                                 */
/*                                                                           */
/*  Test this module.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheMulisetGeneratorTest(HA_ARENA a)
{
  KHE_MULTISET_GENERATOR mg;  int count, j;

  /* creation and setup */
  mg = KheMultisetGeneratorMake(a);
  KheMultisetGeneratorSetupBegin(mg, 7);
  KheMultisetGeneratorSetupAdd(mg, 1, 3);
  KheMultisetGeneratorSetupAdd(mg, 1, 3);
  KheMultisetGeneratorSetupAdd(mg, 1, 3);
  KheMultisetGeneratorSetupAdd(mg, 1, 3);
  KheMultisetGeneratorSetupEnd(mg);

  /* generation */
  count = KheMultisetGeneratorSetupAddCount(mg);
  while( KheMultisetGeneratorNext(mg) )
  {
    fprintf(stderr, "{");
    for( j = 0;  j < count;  j++ )
    {
      if( j > 0 )
	fprintf(stderr, ", ");
      fprintf(stderr, "%d", KheMultisetGeneratorNextValue(mg, j));
    }
    fprintf(stderr, "}\n");
  }
}

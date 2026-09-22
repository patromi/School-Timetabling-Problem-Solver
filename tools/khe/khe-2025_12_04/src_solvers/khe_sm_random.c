
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
/*  FILE:         khe_sm_random.c                                            */
/*  DESCRIPTION:  Thread-safe random numbers                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

/*****************************************************************************/
/*                                                                           */
/*  void KheRandomGeneratorInit(KHE_RANDOM_GENERATOR *rgen, uint32_t seed)   */
/*                                                                           */
/*  Initialize *rgen using seed.                                             */
/*                                                                           */
/*****************************************************************************/

void KheRandomGeneratorInit(KHE_RANDOM_GENERATOR *rgen, uint32_t seed)
{
  *rgen = (uint64_t) seed;
}


/*****************************************************************************/
/*                                                                           */
/*  uint32_t KheRandomGeneratorNext(KHE_RANDOM_GENERATOR *rgen)              */
/*                                                                           */
/*  Move *rgen onto the next random number and return that number.           */
/*                                                                           */
/*****************************************************************************/

uint32_t KheRandomGeneratorNext(KHE_RANDOM_GENERATOR *rgen)
{
  *rgen = *rgen * 1103515240351559573 + 12345678998754321;
  return (uint32_t) (*rgen % UINT_MAX);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRandomGeneratorNextRange(KHE_RANDOM_GENERATOR *rgen,              */
/*    int first, int last)                                                   */
/*                                                                           */
/*  Using KheRandomGeneratorNext, generate a random number in the range      */
/*  first to last inclusive.                                                 */
/*                                                                           */
/*****************************************************************************/

int KheRandomGeneratorNextRange(KHE_RANDOM_GENERATOR *rgen,
  int first, int last)
{
  HnAssert(first <= last, "KheRandomGeneratorNextRange: empty range %d .. %d",
    first, last);
  return first + KheRandomGeneratorNext(rgen) % (last - first + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRandomGeneratorNextBool(KHE_RANDOM_GENERATOR *rgen)              */
/*                                                                           */
/*  Using KheRandomGeneratorNext, generate a random Boolean.                 */
/*                                                                           */
/*****************************************************************************/

bool KheRandomGeneratorNextBool(KHE_RANDOM_GENERATOR *rgen)
{
  return (bool) KheRandomGeneratorNextRange(rgen, 0, 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRandomGeneratorTest(uint32_t seed, int count,                    */
/*    int first, int last, int indent, FILE *fp)                             */
/*                                                                           */
/*  Test a random number generator with the given seed, by printing out      */
/*  count random integers in the range first to last inclusive.  The print   */
/*  is onto file fp and has the given indent.                                */
/*                                                                           */
/*****************************************************************************/

void KheRandomGeneratorTest(uint32_t seed, int count,
  int first, int last, int indent, FILE *fp)
{
  KHE_RANDOM_GENERATOR rgen;  int i;
  KheRandomGeneratorInit(&rgen, seed);
  fprintf(fp, "%*s[ KheRandomGeneratorTest(seed %d, count %d, range %d..%d)\n",
    indent, "", seed, count, first, last);
  for( i = 0;  i < count;  i++ )
  {
    if( i == 0 )
      fprintf(fp, "%*s", indent, "");
    else if( i % 10 == 0 )
      fprintf(fp, ",\n%*s", indent, "");
    else
      fprintf(fp, ", ");
    fprintf(fp, "%d", KheRandomGeneratorNextRange(&rgen, first, last));
  }
  fprintf(fp, "\n%*s]\n", indent, "");
}

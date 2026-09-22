
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
/*  FILE:         nrc_pattern_set.c                                          */
/*  MODULE:       Pattern sets                                               */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN_SET - a pattern set                                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_PATTERN) ARRAY_NRC_PATTERN;

struct nrc_pattern_set_rec {
  NRC_INSTANCE			instance;
  int				index_in_instance;
  ARRAY_NRC_PATTERN		patterns;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN_SET NrcPatternSetMake(NRC_INSTANCE ins)                      */
/*                                                                           */
/*  Return a new, empty pattern set.                                         */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN_SET NrcPatternSetMake(NRC_INSTANCE ins)
{
  NRC_PATTERN_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  HaArrayInit(res->patterns, a);
  res->index_in_instance = NrcInstanceAddPatternSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcPatternSetInstance(NRC_PATTERN_SET ps)                   */
/*                                                                           */
/*  Return the instance that ps lies in.                                     */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcPatternSetInstance(NRC_PATTERN_SET ps)
{
  return ps->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPatternSetIndexInInstance(NRC_PATTERN_SET ps)                     */
/*                                                                           */
/*  Return the index of ps in its instance.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcPatternSetIndexInInstance(NRC_PATTERN_SET ps)
{
  return ps->index_in_instance;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternSetAddPattern(NRC_PATTERN_SET ps, NRC_PATTERN p)          */
/*                                                                           */
/*  Add p to ps.                                                             */
/*                                                                           */
/*****************************************************************************/

void NrcPatternSetAddPattern(NRC_PATTERN_SET ps, NRC_PATTERN p)
{
  HaArrayAddLast(ps->patterns, p);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPatternSetPatternCount(NRC_PATTERN_SET ps)                        */
/*                                                                           */
/*  Return the number of patterns in ps.                                     */
/*                                                                           */
/*****************************************************************************/

int NrcPatternSetPatternCount(NRC_PATTERN_SET ps)
{
  return HaArrayCount(ps->patterns);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN NrcPatternSetPattern(NRC_PATTERN_SET ps, int i)              */
/*                                                                           */
/*  Return the i'th pattern of ps.                                           */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN NrcPatternSetPattern(NRC_PATTERN_SET ps, int i)
{
  return HaArray(ps->patterns, i);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN_SET NrcPatternSetReduce(NRC_PATTERN_SET ps)                  */
/*                                                                           */
/*  Return a reduced version of ps.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN_SET NrcPatternSetReduce(NRC_PATTERN_SET ps)
{
  NRC_PATTERN_SET res;  NRC_PATTERN p;  int i;
  res = NrcPatternSetMake(ps->instance);
  HaArrayForEach(ps->patterns, p, i)
    NrcPatternSetMergePattern(res, p);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternSetMergePattern(NRC_PATTERN_SET ps, NRC_PATTERN p)        */
/*                                                                           */
/*  Merge p into ps.                                                         */
/*                                                                           */
/*****************************************************************************/

void NrcPatternSetMergePattern(NRC_PATTERN_SET ps, NRC_PATTERN p)
{
  NRC_PATTERN p2;  int i;
  HaArrayForEach(ps->patterns, p2, i)
    if( NrcPatternMerge(p2, p) )
      return;
  if( i >= HaArrayCount(ps->patterns) )
    NrcPatternSetAddPattern(ps, p);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternSetDebug(NRC_PATTERN_SET ps, int indent, FILE *fp)        */
/*                                                                           */
/*  Produce a debug print of ps onto fp with the given indent.               */
/*                                                                           */
/*****************************************************************************/

void NrcPatternSetDebug(NRC_PATTERN_SET ps, int indent, FILE *fp)
{
  int i;  NRC_PATTERN p;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Pattern Set (%d patterns):\n", indent, "",
      NrcPatternSetPatternCount(ps));
    HaArrayForEach(ps->patterns, p, i)
      NrcPatternDebug(p, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "<%d patterns>", NrcPatternSetPatternCount(ps));
}

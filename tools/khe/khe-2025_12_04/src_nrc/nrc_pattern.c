
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
/*  FILE:         nrc_pattern.c                                              */
/*  MODULE:       Patterns and pattern elements                              */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN - a pattern                                                  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT_TYPE_SET) ARRAY_NRC_SHIFT_TYPE_SET;
typedef HA_ARRAY(NRC_POLARITY) ARRAY_NRC_POLARITY;

struct nrc_pattern_rec {
  NRC_INSTANCE			instance;
  int				index_in_instance;
  char				*name;
  ARRAY_NRC_SHIFT_TYPE_SET	shift_type_sets;
  ARRAY_NRC_POLARITY		polarities;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_PATTERN NrcPatternMake(NRC_INSTANCE ins, char *name, NRC_DAY_SET ds) */
/*                                                                           */
/*  Make and return a new pattern with these attributes, no terms yet.       */
/*                                                                           */
/*****************************************************************************/

NRC_PATTERN NrcPatternMake(NRC_INSTANCE ins, char *name)
{
  NRC_PATTERN res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  HaArrayInit(res->shift_type_sets, a);
  HaArrayInit(res->polarities, a);
  res->index_in_instance = NrcInstanceAddPattern(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcPatternInstance(NRC_PATTERN p)                           */
/*                                                                           */
/*  Return p's instance.                                                     */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcPatternInstance(NRC_PATTERN p)
{
  return p->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPatternIndexInInstance(NRC_PATTERN p)                             */
/*                                                                           */
/*  Return the index of p in instance.                                       */
/*                                                                           */
/*****************************************************************************/

int NrcPatternIndexInInstance(NRC_PATTERN p)
{
  return p->index_in_instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcPatternName(NRC_PATTERN p)                                      */
/*                                                                           */
/*  Return the name of p, possibly NULL.                                     */
/*                                                                           */
/*****************************************************************************/

char *NrcPatternName(NRC_PATTERN p)
{
  return p->name;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternAddTerm(NRC_PATTERN p, NRC_SHIFT_TYPE_SET sts,            */
/*    NRC_POLARITY po)                                                       */
/*                                                                           */
/*  Add a term with these attributes to p.                                   */
/*                                                                           */
/*****************************************************************************/

void NrcPatternAddTerm(NRC_PATTERN p, NRC_SHIFT_TYPE_SET sts, NRC_POLARITY po)
{
  HaArrayAddLast(p->shift_type_sets, sts);
  HaArrayAddLast(p->polarities, po);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPatternTermCount(NRC_PATTERN p)                                   */
/*                                                                           */
/*  Return the number of terms in p.                                         */
/*                                                                           */
/*****************************************************************************/

int NrcPatternTermCount(NRC_PATTERN p)
{
  return HaArrayCount(p->shift_type_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternTerm(NRC_PATTERN p, int i, NRC_SHIFT_TYPE_SET *sts,       */
/*    NRC_POLARITY *po)                                                      */
/*                                                                           */
/*  Return the i'th term of pattern p.                                       */
/*                                                                           */
/*****************************************************************************/

void NrcPatternTerm(NRC_PATTERN p, int i, NRC_SHIFT_TYPE_SET *sts,
  NRC_POLARITY *po)
{
  *sts = HaArray(p->shift_type_sets, i);
  *po = HaArray(p->polarities, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPatternIsUniform(NRC_PATTERN p)                                  */
/*                                                                           */
/*  Return true if p is uniform, that is, if its shift-type sets are all     */
/*  equal, and its polarities are all equal.                                 */
/*                                                                           */
/*****************************************************************************/

bool NrcPatternIsUniform(NRC_PATTERN p)
{
  NRC_SHIFT_TYPE_SET sts0, sts;  NRC_POLARITY po0, po;  int i;
  if( NrcPatternTermCount(p) == 0 )
    return true;
  NrcPatternTerm(p, 0, &sts0, &po0);
  for( i = 1;  i < HaArrayCount(p->shift_type_sets);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    if( !NrcShiftTypeSetEqual(sts, sts0) || po != po0 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPatternMergeable(NRC_PATTERN dest_p, NRC_PATTERN source_p,       */
/*    int *pos)                                                              */
/*                                                                           */
/*  If dest_p and source_p are mergeable, set *pos to the one point where    */
/*  they differ (there must be exactly one such point) and return true.      */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool NrcPatternMergeable(NRC_PATTERN dest_p, NRC_PATTERN source_p,
  int *pos)
{
  int unequal_count, unequal_pos, i;
  NRC_SHIFT_TYPE_SET dest_sts, source_sts;
  NRC_POLARITY dest_po, source_po;

  /* lengths must be equal */
  if( NrcPatternTermCount(dest_p) != NrcPatternTermCount(source_p) )
    return false;

  unequal_count = 0, unequal_pos = -1;
  for( i = 0;  i < HaArrayCount(dest_p->shift_type_sets);  i++ )
  {
    /* polarities must be pairwise equal */
    dest_po = HaArray(dest_p->polarities, i);
    source_po = HaArray(source_p->polarities, i);
    if( dest_po != source_po )
      return false;

    /* shift sets must be pairwise equal, except at exactly one place, */
    /* where the must be disjoint */
    dest_sts = HaArray(dest_p->shift_type_sets, i);
    source_sts = HaArray(source_p->shift_type_sets, i);
    if( !NrcShiftTypeSetEqual(dest_sts, source_sts) )
    {
      unequal_count++;
      if( unequal_count > 1 || !NrcShiftTypeSetDisjoint(dest_sts, source_sts) )
	return false;
      unequal_pos = i;
    }
  }
  if( unequal_count == 1 )
    return *pos = unequal_pos, true;
  else
    return *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPatternMerge(NRC_PATTERN dest_p, NRC_PATTERN source_p)           */
/*                                                                           */
/*  If source_p can be merged into dest_p, do so (changing the contents      */
/*  of dest_p) and return true.  Otherwise change nothing and return false.  */
/*                                                                           */
/*****************************************************************************/

bool NrcPatternMerge(NRC_PATTERN dest_p, NRC_PATTERN source_p)
{
  int pos;  NRC_SHIFT_TYPE_SET dest_sts, source_sts, merged_sts;
  if( NrcPatternMergeable(dest_p, source_p, &pos) )
  {
    dest_sts = HaArray(dest_p->shift_type_sets, pos);
    source_sts = HaArray(source_p->shift_type_sets, pos);
    merged_sts = NrcShiftTypeSetMerge(dest_sts, source_sts);
    HaArrayPut(dest_p->shift_type_sets, pos, merged_sts);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "polarity"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_POLARITY NrcPolarityNegate(NRC_POLARITY po)                          */
/*                                                                           */
/*  Negate po.                                                               */
/*                                                                           */
/*****************************************************************************/

NRC_POLARITY NrcPolarityNegate(NRC_POLARITY po)
{
  switch( po )
  {
    case NRC_POSITIVE:	return NRC_NEGATIVE;
    case NRC_NEGATIVE:	return NRC_POSITIVE;

    default:		HnAbort("NrcPolarityNegate: illegal po");
			return NRC_POSITIVE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcPatternDebug(NRC_PATTERN p, int indent, FILE *fp)                */
/*                                                                           */
/*  Debug print of p onto fp with the given indent.                          */
/*                                                                           */
/*****************************************************************************/

void NrcPatternDebug(NRC_PATTERN p, int indent, FILE *fp)
{
  NRC_SHIFT_TYPE_SET sts;  NRC_SHIFT_TYPE st;  NRC_POLARITY po;  int i, j;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  for( i = 0;  i < HaArrayCount(p->shift_type_sets);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    fprintf(fp, "%s[", po == NRC_NEGATIVE ? "N" : "");
    for( j = 0;  j < NrcShiftTypeSetShiftTypeCount(sts);  j++ )
    {
      st = NrcShiftTypeSetShiftType(sts, j);
      if( j > 0 )
	fprintf(fp, ",");
      fprintf(fp, "%s", NrcShiftTypeName(st));
    }
    fprintf(fp, "]");
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}

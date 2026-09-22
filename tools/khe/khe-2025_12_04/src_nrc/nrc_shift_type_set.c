
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
/*  FILE:         nrc_shift_type_set.c                                       */
/*  MODULE:       A shift type set                                           */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET                                                       */
/*                                                                           */
/*  A shift type set.  A name is optional.                                   */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT_TYPE) ARRAY_NRC_SHIFT_TYPE;

struct nrc_shift_type_set_rec {
  NRC_INSTANCE		instance;
  char			*name;
  ARRAY_NRC_SHIFT_TYPE	shift_types;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET NrcShiftTypeSetMake(NRC_INSTANCE ins)                 */
/*                                                                           */
/*  Make a new, empty shift-type set.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE_SET NrcShiftTypeSetMake(NRC_INSTANCE ins, char *name)
{
  NRC_SHIFT_TYPE_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  HaArrayInit(res->shift_types, a);
  NrcInstanceAddShiftTypeSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftTypeSetAddShiftType(NRC_SHIFT_TYPE_SET sts,                 */
/*    NRC_SHIFT_TYPE st)                                                     */
/*                                                                           */
/*  Add st to sts.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcShiftTypeSetAddShiftType(NRC_SHIFT_TYPE_SET sts, NRC_SHIFT_TYPE st)
{
  HaArrayAddLast(sts->shift_types, st);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcShiftTypeSetInstance(NRC_SHIFT_TYPE_SET sts)             */
/*                                                                           */
/*  Return the instance containing sts.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcShiftTypeSetInstance(NRC_SHIFT_TYPE_SET sts)
{
  return sts->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftTypeSetName(NRC_SHIFT_TYPE_SET sts)                        */
/*                                                                           */
/*  Return the name of sts, or NULL if none.                                 */
/*                                                                           */
/*****************************************************************************/

char *NrcShiftTypeSetName(NRC_SHIFT_TYPE_SET sts)
{
  return sts->name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftTypeSetShiftTypeCount(NRC_SHIFT_TYPE_SET sts)                */
/*                                                                           */
/*  Return the number of shift types of sts.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcShiftTypeSetShiftTypeCount(NRC_SHIFT_TYPE_SET sts)
{
  return HaArrayCount(sts->shift_types);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE NrcShiftTypeSetShiftType(NRC_SHIFT_TYPE_SET sts, int i)   */
/*                                                                           */
/*  Return the i'th shift type of sts.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE NrcShiftTypeSetShiftType(NRC_SHIFT_TYPE_SET sts, int i)
{
  return HaArray(sts->shift_types, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTypeSetContainsShiftType(NRC_SHIFT_TYPE_SET sts,            */
/*    NRC_SHIFT_TYPE st)                                                     */
/*                                                                           */
/*  Return true if sts contains st.                                          */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftTypeSetContainsShiftType(NRC_SHIFT_TYPE_SET sts,
  NRC_SHIFT_TYPE st)
{
  int pos;
  return HaArrayContains(sts->shift_types, st, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTypeSetEqual(NRC_SHIFT_TYPE_SET sts1,                       */
/*    NRC_SHIFT_TYPE_SET sts2)                                               */
/*                                                                           */
/*  Return true if sts1 and sts2 contain the same shift-type sets.           */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftTypeSetEqual(NRC_SHIFT_TYPE_SET sts1, NRC_SHIFT_TYPE_SET sts2)
{
  int i;  NRC_SHIFT_TYPE st;
  if( sts1 == sts2 )
    return true;
  if( HaArrayCount(sts1->shift_types) != HaArrayCount(sts2->shift_types) )
    return false;
  HaArrayForEach(sts1->shift_types, st, i)
    if( !NrcShiftTypeSetContainsShiftType(sts2, st) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTypeSetDisjoint(NRC_SHIFT_TYPE_SET sts1,                    */
/*    NRC_SHIFT_TYPE_SET sts2)                                               */
/*                                                                           */
/*  Return true if sts1 and sts2 are disjoint.                               */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftTypeSetDisjoint(NRC_SHIFT_TYPE_SET sts1,
  NRC_SHIFT_TYPE_SET sts2)
{
  int i;  NRC_SHIFT_TYPE st;
  if( sts1 == sts2 )
    return false;
  HaArrayForEach(sts1->shift_types, st, i)
    if( NrcShiftTypeSetContainsShiftType(sts2, st) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET NrcShiftTypeSetMerge(NRC_SHIFT_TYPE_SET sts1,         */
/*    NRC_SHIFT_TYPE_SET sts2)                                               */
/*                                                                           */
/*  Return the merge of sts1 with sts2.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE_SET NrcShiftTypeSetMerge(NRC_SHIFT_TYPE_SET sts1,
  NRC_SHIFT_TYPE_SET sts2)
{
  NRC_SHIFT_TYPE_SET res;  int i;  NRC_SHIFT_TYPE st;
  res = NrcShiftTypeSetMake(sts1->instance, NULL);
  HaArrayForEach(sts1->shift_types, st, i)
    NrcShiftTypeSetAddShiftType(res, st);
  HaArrayForEach(sts2->shift_types, st, i)
    if( !NrcShiftTypeSetContainsShiftType(res, st) )
      NrcShiftTypeSetAddShiftType(res, st);
  return res;
}

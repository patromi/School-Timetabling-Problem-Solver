
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
/*  FILE:         nrc_shift_set_set.c                                        */
/*  MODULE:       A shift-set set                                            */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET                                                        */
/*                                                                           */
/*  A shift-set set                                                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT_SET) ARRAY_NRC_SHIFT_SET;

struct nrc_shift_set_set_rec {
  NRC_INSTANCE		instance;
  ARRAY_NRC_SHIFT_SET	shift_sets;
  NRC_SHIFT_SET		starting_ss;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcShiftSetSetMake(NRC_INSTANCE ins, char *name)       */
/*                                                                           */
/*  Make a shift-set set object with this name and initially no shift-sets.  */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcShiftSetSetMake(NRC_INSTANCE ins /* , char *name */)
{
  NRC_SHIFT_SET_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  HaArrayInit(res->shift_sets, a);
  res->starting_ss = NULL;
  NrcInstanceAddShiftSetSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetSetAddShiftSet(NRC_SHIFT_SET_SET sss, NRC_SHIFT_SET ss)  */
/*                                                                           */
/*  Add ss to sss.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetSetAddShiftSet(NRC_SHIFT_SET_SET sss, NRC_SHIFT_SET ss)
{
  HaArrayAddLast(sss->shift_sets, ss);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcShiftSetSetInstance(NRC_SHIFT_SET_SET sss)               */
/*                                                                           */
/*  Return the enclosing instance of sss.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcShiftSetSetInstance(NRC_SHIFT_SET_SET sss)
{
  return sss->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftSetSetShiftSetCount(NRC_SHIFT_SET_SET sss)                   */
/*                                                                           */
/*  Return the number of shift-sets of sss.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcShiftSetSetShiftSetCount(NRC_SHIFT_SET_SET sss)
{
  return HaArrayCount(sss->shift_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftSetSetShiftSet(NRC_SHIFT_SET_SET sss, int i)       */
/*                                                                           */
/*  Return the i'th shift-set of sss.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftSetSetShiftSet(NRC_SHIFT_SET_SET sss, int i)
{
  return HaArray(sss->shift_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftSetSetStartingShiftSet(NRC_SHIFT_SET_SET sss)      */
/*                                                                           */
/*  Return the starting shift-set of sss.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftSetSetStartingShiftSet(NRC_SHIFT_SET_SET sss)
{
  int i;  NRC_SHIFT_SET ss;
  if( sss->starting_ss == NULL )
  {
    sss->starting_ss = NrcShiftSetMake(sss->instance);
    HaArrayForEach(sss->shift_sets, ss, i)
    {
      HnAssert(NrcShiftSetShiftCount(ss) != 0,
	"NrcShiftSetSetStartingShiftSet: the %d'th shift set is empty", i);
      NrcShiftSetAddShift(sss->starting_ss, NrcShiftSetShift(ss, 0));
    }
  }
  return sss->starting_ss;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetSetDebug(NRC_SHIFT_SET_SET sss, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of sss onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetSetDebug(NRC_SHIFT_SET_SET sss, int indent, FILE *fp)
{
  int i;  NRC_SHIFT_SET ss;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ShiftSetSet:\n", indent, "");
    HaArrayForEach(sss->shift_sets, ss, i)
      NrcShiftSetDebug(ss, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "ShiftSetSet");
}

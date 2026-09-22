
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
/*  FILE:         nrc_shift_type.                                            */
/*  MODULE:       A shift type                                               */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE                                                           */
/*                                                                           */
/*  A shift type.                                                            */
/*                                                                           */
/*****************************************************************************/

struct nrc_shift_type_rec {
  NRC_INSTANCE		instance;
  char			*name;
  char			*label;
  int			workload;
  /* bool		means_time_off; */
  NRC_SHIFT_TYPE_SET	singleton_shift_type_set;
  NRC_SHIFT_SET		shift_set;
  NRC_SHIFT_SET_SET	all_sss;
  int			index;
  NRC_TIME_INTERVAL	time_interval;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE NrcShiftTypeMake(NRC_INSTANCE ins, char *name,            */
/*    int workload)                                                          */
/*                                                                           */
/*  Make a new shift type with these attributes and add it to ins.           */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE NrcShiftTypeMake(NRC_INSTANCE ins, char *name, int workload)
{
  NRC_SHIFT_TYPE res;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcShiftTypeMake(ins, %s, %d)\n", name, workload);
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  res->label = NULL;  /* set separately if required */
  res->workload = workload;
  /* res->means_time_off = false; */
  res->singleton_shift_type_set = NrcShiftTypeSetMake(ins, NULL);
  NrcShiftTypeSetAddShiftType(res->singleton_shift_type_set, res);
  res->shift_set = NrcShiftSetMake(ins);
  res->all_sss = NULL;
  res->index = NrcInstanceAddShiftType(ins, res);  /* must come last! */
  res->time_interval = NULL;
  if( DEBUG1 )
    fprintf(stderr, "] NrcShiftTypeMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftTypeUseLabelInEventName(NRC_SHIFT_TYPE st, char *label)     */
/*                                                                           */
/*  Use label in event name.                                                 */
/*                                                                           */
/*****************************************************************************/

void NrcShiftTypeUseLabelInEventName(NRC_SHIFT_TYPE st, char *label)
{
  HA_ARENA a;
  if( label != NULL && strcmp(label, "") != 0 )
  {
    a = NrcInstanceArena(st->instance);
    st->label = HnStringCopy(label, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftTypeLabel(NRC_SHIFT_TYPE st)                               */
/*                                                                           */
/*  Return the label of st, or NULL if none.                                 */
/*                                                                           */
/*****************************************************************************/

char *NrcShiftTypeLabel(NRC_SHIFT_TYPE st)
{
  return st->label;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftTypeSetMeansTimeOff(NRC_SHIFT_TYPE st, bool val)            */
/*                                                                           */
/*  Set the "means_time_off" attribute to val.                               */
/*                                                                           */
/*****************************************************************************/

/* *** abandoned
void NrcShiftTypeSetMeansTimeOff(NRC_SHIFT_TYPE st, bool val)
{
  if( DEBUG3 )
    fprintf(stderr, "NrcShiftTypeSetMeansTimeOff(%s in instance %s, %s)\n",
      st->name, NrcInstanceId(st->instance), val ? "true" : "false");
  st->means_time_off = val;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTypeMeansTimeOff(NRC_SHIFT_TYPE st)                         */
/*                                                                           */
/*  Return the "means time off" shift type.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** abandoned
bool NrcShiftTypeMeansTimeOff(NRC_SHIFT_TYPE st)
{
  return st->means_time_off;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcShiftTypeInstance(NRC_SHIFT_TYPE st)                     */
/*                                                                           */
/*  Return st's instance.                                                    */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcShiftTypeInstance(NRC_SHIFT_TYPE st)
{
  return st->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftTypeName(NRC_SHIFT_TYPE st)                                */
/*                                                                           */
/*  Return the name attribute of st.                                         */
/*                                                                           */
/*****************************************************************************/

char *NrcShiftTypeName(NRC_SHIFT_TYPE st)
{
  return st->name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftTypeWorkload(NRC_SHIFT_TYPE st)                              */
/*                                                                           */
/*  Return the workload of st.                                               */
/*                                                                           */
/*****************************************************************************/

int NrcShiftTypeWorkload(NRC_SHIFT_TYPE st)
{
  return st->workload;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftTypeIndex(NRC_SHIFT_TYPE st)                                 */
/*                                                                           */
/*  Return the index attribute of st (st's index in the instance).           */
/*                                                                           */
/*****************************************************************************/

int NrcShiftTypeIndex(NRC_SHIFT_TYPE st)
{
  return st->index;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_TYPE_SET NrcShiftTypeSingletonShiftTypeSet(NRC_SHIFT_TYPE st)  */
/*                                                                           */
/*  Return a shift-type set containing just st.                              */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE_SET NrcShiftTypeSingletonShiftTypeSet(NRC_SHIFT_TYPE st)
{
  return st->singleton_shift_type_set;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftTypeAddShift(NRC_SHIFT_TYPE st, NRC_SHIFT s)                */
/*                                                                           */
/*  Add s to st's set of all shifts of its type.                             */
/*                                                                           */
/*****************************************************************************/

void NrcShiftTypeAddShift(NRC_SHIFT_TYPE st, NRC_SHIFT s)
{
  NrcShiftSetAddShift(st->shift_set, s);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftTypeShiftSet(NRC_SHIFT_TYPE st)                    */
/*                                                                           */
/*  Return the shift-set attribute of st (shifts whose shift type is st).    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftTypeShiftSet(NRC_SHIFT_TYPE st)
{
  return st->shift_set;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcShiftTypeShiftSetSet(NRC_SHIFT_TYPE st)             */
/*                                                                           */
/*  Return a shift-set set, each of whose shift-sets contains one shift of   */
/*  type st.  The shifts appear in increasing day order.                     */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcShiftTypeShiftSetSet(NRC_SHIFT_TYPE st)
{
  NRC_SHIFT s;  int i;  NRC_SHIFT_SET ss;
  if( st->all_sss == NULL )
  {
    st->all_sss = NrcShiftSetSetMake(st->instance);
    for( i = 0;  i < NrcShiftSetShiftCount(st->shift_set);  i++ )
    {
      s = NrcShiftSetShift(st->shift_set, i);
      ss = NrcShiftSetMake(st->instance);
      NrcShiftSetAddShift(ss, s);
      NrcShiftSetSetAddShiftSet(st->all_sss, ss);
    }
  }
  return st->all_sss;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time intervals"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftTypeAddTimeInterval(NRC_SHIFT_TYPE st, NRC_TIME_INTERVAL ti)*/
/*                                                                           */
/*  Add ti to st.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcShiftTypeAddTimeInterval(NRC_SHIFT_TYPE st, NRC_TIME_INTERVAL ti)
{
  st->time_interval = ti;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_INTERVAL NrcShiftTypeTimeInterval(NRC_SHIFT_TYPE st)            */
/*                                                                           */
/*  Return st's time interval, or NULL if none.                              */
/*                                                                           */
/*****************************************************************************/

NRC_TIME_INTERVAL NrcShiftTypeTimeInterval(NRC_SHIFT_TYPE st)
{
  return st->time_interval;
}

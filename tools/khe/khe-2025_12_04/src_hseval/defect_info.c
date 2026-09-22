
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
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
/*  FILE:         defect_info.c                                              */
/*  MODULE:       Defect information                                         */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

/*****************************************************************************/
/*                                                                           */
/*  DEFECT_INFO                                                              */
/*                                                                           */
/*****************************************************************************/

struct defect_info_rec {
  char			*defect_str;
  ARRAY_KHE_CONSTRAINT	constraints;
};


/*****************************************************************************/
/*                                                                           */
/*  DEFECT_INFO DefectInfoMake(KHE_SOLN soln, char *defect_str)              */
/*                                                                           */
/*  Make and initialize a defect info object.                                */
/*                                                                           */
/*****************************************************************************/

DEFECT_INFO DefectInfoMake(KHE_SOLN soln, char *defect_str)
{
  /* still to do */
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool DefectPresent(DEFECT_INFO di, KHE_RESOURCE r, KHE_TIME_GROUP tg)    */
/*                                                                           */
/*  Return true if r is defective in tg.                                     */
/*                                                                           */
/*****************************************************************************/

bool DefectPresent(DEFECT_INFO di, KHE_RESOURCE r, KHE_TIME_GROUP tg)
{
  /* still to do */
  return false;
}

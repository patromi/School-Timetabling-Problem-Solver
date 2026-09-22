
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
/*  FILE:         model.c                                                    */
/*  MODULE:       Handle the different models.                               */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

/*****************************************************************************/
/*                                                                           */
/*  MODEL_RESOURCE_TYPE - one resource type in the model                     */
/*                                                                           */
/*****************************************************************************/

struct model_resource_type_rec {
  HA_ARRAY_NSTRING		names;
};


/*****************************************************************************/
/*                                                                           */
/*  MODEL - the model (high school timetabling, nurse rostering, etc.).      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(MODEL_RESOURCE_TYPE) ARRAY_MODEL_RESOURCE_TYPE;

struct model_rec {
  char 				*instances_summary_header;
  ARRAY_MODEL_RESOURCE_TYPE	model_resource_types;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "model resource type"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  MODEL_RESOURCE_TYPE ModelResourceTypeMake(char *init_name, HA_ARENA a)   */
/*                                                                           */
/*  Return a new model resource type object with this initial name.          */
/*                                                                           */
/*****************************************************************************/

MODEL_RESOURCE_TYPE ModelResourceTypeMake(char *init_name, HA_ARENA a)
{
  MODEL_RESOURCE_TYPE res;
  HaMake(res, a);
  HaArrayInit(res->names, a);
  HaArrayAddLast(res->names, init_name);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *ModelResourceTypeInitName(MODEL_RESOURCE_TYPE mrt)                 */
/*                                                                           */
/*  Return the initial name of mrt.                                          */
/*                                                                           */
/*****************************************************************************/

char *ModelResourceTypeInitName(MODEL_RESOURCE_TYPE mrt)
{
  return HaArrayFirst(mrt->names);
}


/*****************************************************************************/
/*                                                                           */
/*  void ModelResourceTypeAddName(MODEL_RESOURCE_TYPE mrt, char *name)       */
/*                                                                           */
/*  Add an alternative name to mrt.                                          */
/*                                                                           */
/*****************************************************************************/

void ModelResourceTypeAddName(MODEL_RESOURCE_TYPE mrt, char *name)
{
  HaArrayAddLast(mrt->names, name);
}


/*****************************************************************************/
/*                                                                           */
/*  bool ModelResourceTypeContainsName(MODEL_RESOURCE_TYPE mrt, char *name)  */
/*                                                                           */
/*  Return true if name is one of the names of mrt.                          */
/*                                                                           */
/*****************************************************************************/

bool ModelResourceTypeContainsName(MODEL_RESOURCE_TYPE mrt, char *name)
{
  char *n;  int i;
  HaArrayForEach(mrt->names, n, i)
    if( strcmp(n, name) == 0 )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "model"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  MODEL ModelMake(char *instances_summary_header)                          */
/*                                                                           */
/*  Make a new model object.                                                 */
/*                                                                           */
/*****************************************************************************/

MODEL ModelMake(char *instances_summary_header, HA_ARENA a)
{
  MODEL res;
  HaMake(res, a);
  res->instances_summary_header = instances_summary_header;
  HaArrayInit(res->model_resource_types, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *ModelInstancesSummaryHeader(MODEL model)                           */
/*                                                                           */
/*  Return the instances summary attribute of model.                         */
/*                                                                           */
/*****************************************************************************/

char *ModelInstancesSummaryHeader(MODEL model)
{
  return model->instances_summary_header;
}


/*****************************************************************************/
/*                                                                           */
/*  void ModelAddModelResourceType(MODEL model, MODEL_RESOURCE_TYPE mrt)     */
/*                                                                           */
/*  Add mrt to model.                                                        */
/*                                                                           */
/*****************************************************************************/

void ModelAddModelResourceType(MODEL model, MODEL_RESOURCE_TYPE mrt)
{
  HaArrayAddLast(model->model_resource_types, mrt);
}


/*****************************************************************************/
/*                                                                           */
/*  int ModelModelResourceTypeCount(MODEL model)                             */
/*                                                                           */
/*  Return the number of model resource types in model.                      */
/*                                                                           */
/*****************************************************************************/

int ModelModelResourceTypeCount(MODEL model)
{
  return HaArrayCount(model->model_resource_types);
}


/*****************************************************************************/
/*                                                                           */
/*  MODEL_RESOURCE_TYPE ModelModelResourceType(MODEL model, int i)           */
/*                                                                           */
/*  Return the i'th model resource type of model.                            */
/*                                                                           */
/*****************************************************************************/

MODEL_RESOURCE_TYPE ModelModelResourceType(MODEL model, int i)
{
  return HaArray(model->model_resource_types, i);
}



/*****************************************************************************/
/*                                                                           */
/*  MODEL ModelBuild(KHE_MODEL model, HA_ARENA a)                            */
/*                                                                           */
/*  Make a model object for the given model.                                 */
/*                                                                           */
/*  The last element is always Other, and it always matches.                 */
/*                                                                           */
/*****************************************************************************/

MODEL ModelBuild(KHE_MODEL model, HA_ARENA a)
{
  MODEL res;  MODEL_RESOURCE_TYPE mrt;
  switch( model )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE:

      /* instances summary header */
      res = ModelMake("High school timetabling instances summary", a);

      /* teachers */
      mrt = ModelResourceTypeMake("Teachers", a);
      ModelResourceTypeAddName(mrt, "teachers");
      ModelResourceTypeAddName(mrt, "Teacher");
      ModelResourceTypeAddName(mrt, "teacher");
      ModelAddModelResourceType(res, mrt);

      /* rooms */
      mrt = ModelResourceTypeMake("Rooms", a);
      ModelResourceTypeAddName(mrt, "rooms");
      ModelResourceTypeAddName(mrt, "Room");
      ModelResourceTypeAddName(mrt, "room");
      ModelAddModelResourceType(res, mrt);

      /* classes */
      mrt = ModelResourceTypeMake("Classes", a);
      ModelResourceTypeAddName(mrt, "classes");
      ModelResourceTypeAddName(mrt, "Class");
      ModelResourceTypeAddName(mrt, "class");
      ModelResourceTypeAddName(mrt, "SchoolClass");
      ModelAddModelResourceType(res, mrt);

      /* students */
      mrt = ModelResourceTypeMake("Students", a);
      ModelResourceTypeAddName(mrt, "students");
      ModelResourceTypeAddName(mrt, "Student");
      ModelResourceTypeAddName(mrt, "student");
      ModelAddModelResourceType(res, mrt);
      break;

    case KHE_MODEL_EMPLOYEE_SCHEDULE:

      /* instances summary header */
      res = ModelMake("Employee scheduling instances summary", a);

      /* nurses */
      mrt = ModelResourceTypeMake("Nurses", a);
      ModelResourceTypeAddName(mrt, "nurses");
      ModelResourceTypeAddName(mrt, "Nurse");
      ModelResourceTypeAddName(mrt, "nurse");
      ModelAddModelResourceType(res, mrt);

      /* employees */
      mrt = ModelResourceTypeMake("Employees", a);
      ModelResourceTypeAddName(mrt, "employees");
      ModelResourceTypeAddName(mrt, "Employee");
      ModelResourceTypeAddName(mrt, "employee");
      ModelAddModelResourceType(res, mrt);
      break;

    default:

      HnAssert(false, "ModelMake internal error");
      res = NULL;  /* keep compiler happy */
  }

  /* other */
  mrt = ModelResourceTypeMake("Other", a);
  ModelAddModelResourceType(res, mrt);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  MODEL_RESOURCE_TYPE ModelRetrieve(MODEL model, char *name)               */
/*                                                                           */
/*  Retrieve the mrt with this name.  This always succeeds, because it       */
/*  returns the last model resource type ("Other") as a last resort.         */
/*                                                                           */
/*****************************************************************************/

MODEL_RESOURCE_TYPE ModelRetrieve(MODEL model, char *name)
{
  MODEL_RESOURCE_TYPE mrt;  int i;
  HaArrayForEach(model->model_resource_types, mrt, i)
    if( ModelResourceTypeContainsName(mrt, name) )
      return mrt;
  return HaArrayLast(model->model_resource_types);
}

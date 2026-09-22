
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
/*  FILE:         khe_avoid_clashes_constraint.c                             */
/*  DESCRIPTION:  An avoid clashes constraint                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_CONSTRAINT - an avoid clashes constraint               */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_clashes_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  HA_ARRAY_INT			resource_of_type_count; /* resources of type */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesConstraintMake(KHE_INSTANCE ins, char *id,           */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_AVOID_CLASHES_CONSTRAINT *c)                                       */
/*                                                                           */
/*  Make an avoid clashes constraint with these attributes, add it to ins,   */
/*  and return it.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidClashesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_AVOID_CLASHES_CONSTRAINT *c)
{
  KHE_AVOID_CLASHES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  int i, count;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheAvoidClashesConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  count = KheInstanceResourceTypeCount(ins);
  /* ***
  res = HaAlloc(a, sizeof(struct khe_avoid_clashes_constraint_rec)
    + (count - 1) * sizeof(int));
  *** */
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_AVOID_CLASHES_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  HaArrayInit(res->resource_of_type_count, a);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->resource_of_type_count, 0);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesConstraintAppliesToCount(                             */
/*    KHE_AVOID_CLASHES_CONSTRAINT c)                                        */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesConstraintAppliesToCount(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintResetResourceOfType(                       */
/*    KHE_AVOID_CLASHES_CONSTRAINT c)                                        */
/*                                                                           */
/*  Reset the resource_of_type attributes of c.  This is only done when      */
/*  resource type partitioning is happening.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddToResourceOfType(KHE_AVOID_CLASHES_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArrayFill(c->resource_of_type_count, index + 1, 0);
  HaArray(c->resource_of_type_count, index) += 1;
}

void KheAvoidClashesConstraintResetResourceOfType(
  KHE_AVOID_CLASHES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayClear(c->resource_of_type_count);
  HaArrayForEach(c->resource_groups, rg, i)
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      KheAddToResourceOfType(c, KheResourceGroupResource(rg, j));
  HaArrayForEach(c->resources, r, i)
    KheAddToResourceOfType(c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintFinalize(KHE_AVOID_CLASHES_CONSTRAINT c)   */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintFinalize(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesConstraintDensityCount(KHE_AVOID_CLASHES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the density count of c; just the applies to count here.           */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesConstraintDensityCount(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  return KheAvoidClashesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintAddResourceGroup(                          */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)                 */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintAddResourceGroup(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  int i, rt_index;  KHE_RESOURCE r;
  HaArrayAddLast(c->resource_groups, rg);
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
  }
  rt_index = KheResourceTypeIndex(KheResourceGroupResourceType(rg));
  HaArray(c->resource_of_type_count, rt_index) +=
    KheResourceGroupResourceCount(rg);
  /*c->resource_of_type_count[rt_index] += KheResourceGroupResourceCount(rg);*/
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesConstraintResourceGroupCount(                         */
/*    KHE_AVOID_CLASHES_CONSTRAINT c)                                        */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesConstraintResourceGroupCount(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheAvoidClashesConstraintResourceGroup(               */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, int i)                                 */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheAvoidClashesConstraintResourceGroup(
  KHE_AVOID_CLASHES_CONSTRAINT c, int i)
{
  return HaArray(c->resource_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resources"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintAddResource(                               */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE r)                        */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintAddResource(KHE_AVOID_CLASHES_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int rt_index;
  HaArrayAddLast(c->resources, r);
  rt_index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArray(c->resource_of_type_count, rt_index) += 1;
  /* c->resource_of_type_count[rt_index] += 1; */
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesConstraintResourceCount(                              */
/*    KHE_AVOID_CLASHES_CONSTRAINT c)                                        */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesConstraintResourceCount(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheAvoidClashesConstraintResource(                          */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, int i)                                 */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheAvoidClashesConstraintResource(
  KHE_AVOID_CLASHES_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidClashesConstraintResourceOfTypeCount(                        */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)                  */
/*                                                                           */
/*  Return the number of resources in c of type rt.                          */
/*                                                                           */
/*****************************************************************************/

int KheAvoidClashesConstraintResourceOfTypeCount(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)
{
  return HaArray(c->resource_of_type_count, KheResourceTypeIndex(rt));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintMakeAndAttachMonitors(                     */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, KHE_SOLN soln)                         */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintMakeAndAttachMonitors(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_AVOID_CLASHES_MONITOR m;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheAvoidClashesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  HaArrayForEach(c->resources, r, i)
  {
    rs = KheSolnResourceInSoln(soln, r);
    m = KheAvoidClashesMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesConstraintMakeFromKml(KML_ELT cons_elt,              */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make an avoid clashes constraint based on cons_elt and add it to ins.    */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidClashesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_AVOID_CLASHES_CONSTRAINT res;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* build and insert the constraint object */
  if( !KheAvoidClashesConstraintMake(ins, id, name, reqd, wt, cf, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidClashesConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheAvoidClashesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidClashesConstraint> applies to 0 resources");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintWrite(KHE_AVOID_CLASHES_CONSTRAINT c,      */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintWrite(KHE_AVOID_CLASHES_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i;
  KmlBegin(kf, "AvoidClashesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in AvoidClashesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing"
        " in ResourceGroup referenced from AvoidClashesConstraint %s", c->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", KheResourceGroupId(rg));
    }
    KmlEnd(kf, "ResourceGroups");
  }
  if( HaArrayCount(c->resources) > 0 )
  {
    KmlBegin(kf, "Resources");
    HaArrayForEach(c->resources, r, i)
    {
      HnAssert(KheResourceId(r) != NULL, "KheArchiveWrite:  Id missing"
        " in Resource referenced from AvoidClashesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  KmlEnd(kf, "AvoidClashesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesConstraintDebug(KHE_AVOID_CLASHES_CONSTRAINT c,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesConstraintDebug(KHE_AVOID_CLASHES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[\n", indent, "");
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      HaArrayForEach(c->resources, r, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceId(r) != NULL ? KheResourceId(r) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

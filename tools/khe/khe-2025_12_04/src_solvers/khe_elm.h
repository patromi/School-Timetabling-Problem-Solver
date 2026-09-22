
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
/*  FILE:         khe_elm.h                                                  */
/*  DESCRIPTION:  Header file for the Elm layer matching module              */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_ELM_HEADER_FILE
#define KHE_ELM_HEADER_FILE
#include "howard_a.h"

/* type declarations */
typedef struct khe_elm_rec *KHE_ELM;
typedef struct khe_elm_demand_group_rec *KHE_ELM_DEMAND_GROUP;
typedef struct khe_elm_demand_rec *KHE_ELM_DEMAND;
typedef struct khe_elm_supply_group_rec *KHE_ELM_SUPPLY_GROUP;
typedef struct khe_elm_supply_rec *KHE_ELM_SUPPLY;

/* 10.6.2  The core module - elms */
extern KHE_ELM KheElmMake(KHE_LAYER layer, KHE_OPTIONS options, HA_ARENA a);
/* extern void KheElmDelete(KHE_ELM elm); */
extern KHE_LAYER KheElmLayer(KHE_ELM elm);
extern KHE_OPTIONS KheElmOptions(KHE_ELM elm);
extern HA_ARENA KheElmArena(KHE_ELM elm);

extern int KheElmDemandGroupCount(KHE_ELM elm);
extern KHE_ELM_DEMAND_GROUP KheElmDemandGroup(KHE_ELM elm, int i);

extern int KheElmSupplyGroupCount(KHE_ELM elm);
extern KHE_ELM_SUPPLY_GROUP KheElmSupplyGroup(KHE_ELM elm, int i);

extern int KheElmBestUnmatched(KHE_ELM elm);
extern KHE_COST KheElmBestCost(KHE_ELM elm);
extern bool KheElmBestAssignMeets(KHE_ELM elm);

extern void KheElmSpecialModeBegin(KHE_ELM elm);
extern void KheElmSpecialModeEnd(KHE_ELM elm);

extern void KheElmUnevennessTimeGroupAdd(KHE_ELM elm, KHE_TIME_GROUP tg);
extern int KheElmUnevenness(KHE_ELM elm);

extern void KheElmDebug(KHE_ELM elm, int verbosity, int indent, FILE *fp);
extern void KheElmDebugSegmentation(KHE_ELM elm,
  int verbosity, int indent, FILE *fp);

/* 10.6.2  The core module - demand groups */
extern KHE_ELM KheElmDemandGroupElm(KHE_ELM_DEMAND_GROUP dg);
extern KHE_NODE KheElmDemandGroupNode(KHE_ELM_DEMAND_GROUP dg);
extern int KheElmDemandGroupDemandCount(KHE_ELM_DEMAND_GROUP dg);
extern KHE_ELM_DEMAND KheElmDemandGroupDemand(KHE_ELM_DEMAND_GROUP dg, int i);
extern void KheElmDemandGroupHasChanged(KHE_ELM_DEMAND_GROUP dg);

/* 10.6.2  The core module - demand groups - zones */
extern void KheElmDemandGroupAddZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone);
extern void KheElmDemandGroupDeleteZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone);
extern bool KheElmDemandGroupContainsZone(KHE_ELM_DEMAND_GROUP dg,
  KHE_ZONE zone);
extern int KheElmDemandGroupZoneCount(KHE_ELM_DEMAND_GROUP dg);
extern KHE_ZONE KheElmDemandGroupZone(KHE_ELM_DEMAND_GROUP dg, int i);
extern void KheElmDemandGroupDebug(KHE_ELM_DEMAND_GROUP dg,
  int verbosity, int indent, FILE *fp);

/* 10.6.2  The core module - demands */
extern KHE_ELM_DEMAND_GROUP KheElmDemandDemandGroup(KHE_ELM_DEMAND d);
extern KHE_MEET KheElmDemandMeet(KHE_ELM_DEMAND d);
extern void KheElmDemandHasChanged(KHE_ELM_DEMAND d);
extern bool KheElmDemandBestSupply(KHE_ELM_DEMAND d,
  KHE_ELM_SUPPLY *s, KHE_COST *cost);
extern void KheElmDemandDebug(KHE_ELM_DEMAND d, int verbosity,
  int indent, FILE *fp);

/* 10.6.2  The core module - supply groups */
extern KHE_ELM KheElmSupplyGroupElm(KHE_ELM_SUPPLY_GROUP sg);
extern KHE_MEET KheElmSupplyGroupMeet(KHE_ELM_SUPPLY_GROUP sg);
extern int KheElmSupplyGroupSupplyCount(KHE_ELM_SUPPLY_GROUP sg);
extern KHE_ELM_SUPPLY KheElmSupplyGroupSupply(KHE_ELM_SUPPLY_GROUP sg, int i);
extern void KheElmSupplyGroupDebug(KHE_ELM_SUPPLY_GROUP sg,
  int verbosity, int indent, FILE *fp);

/* 10.6.2  The core module - supplies */
extern KHE_ELM_SUPPLY_GROUP KheElmSupplySupplyGroup(KHE_ELM_SUPPLY s);
extern KHE_MEET KheElmSupplyMeet(KHE_ELM_SUPPLY s);
extern int KheElmSupplyOffset(KHE_ELM_SUPPLY s);
extern int KheElmSupplyDuration(KHE_ELM_SUPPLY s);
extern int KheElmSupplyZoneCount(KHE_ELM_SUPPLY s);
extern KHE_ZONE KheElmSupplyZone(KHE_ELM_SUPPLY s, int i);
extern void KheElmSupplySetFixedDemand(KHE_ELM_SUPPLY s, KHE_ELM_DEMAND d);
extern KHE_ELM_DEMAND KheElmSupplyFixedDemand(KHE_ELM_SUPPLY s);
extern void KheElmSupplyRemove(KHE_ELM_SUPPLY s);
extern void KheElmSupplyUnRemove(KHE_ELM_SUPPLY s);
extern bool KheElmSupplyIsRemoved(KHE_ELM_SUPPLY s);
extern bool KheElmSupplySplitCheck(KHE_ELM_SUPPLY s, int offset, int durn,
  int *count);
extern bool KheElmSupplySplit(KHE_ELM_SUPPLY s, int offset,
  int durn, int *count, KHE_ELM_SUPPLY *ls, KHE_ELM_SUPPLY *rs);
extern void KheElmSupplyMerge(KHE_ELM_SUPPLY ls, KHE_ELM_SUPPLY s,
  KHE_ELM_SUPPLY rs);
extern void KheElmSupplyDebug(KHE_ELM_SUPPLY s, int verbosity,
  int indent, FILE *fp);

/* 10.6.3  Splitting supplies */
extern void KheElmSplitSupplies(KHE_ELM elm,
  KHE_SPREAD_EVENTS_CONSTRAINT sec);

extern void KheElmImproveNodeRegularity(KHE_ELM elm);

/* 10.6.5  Handling irregular monitors */
extern int KheElmIrregularMonitorCount(KHE_ELM elm);
extern KHE_MONITOR KheElmIrregularMonitor(KHE_ELM elm, int i);
extern void KheElmSortIrregularMonitors(KHE_ELM elm,
  int(*compar)(const void *, const void *));
extern bool KheElmIrregularMonitorsAttached(KHE_ELM elm);

extern void KheElmDetachIrregularMonitors(KHE_ELM elm);
extern void KheElmAttachIrregularMonitors(KHE_ELM elm);

extern void KheElmReduceIrregularMonitors(KHE_ELM elm);

#endif

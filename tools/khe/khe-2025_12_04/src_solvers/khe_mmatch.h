
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
/*  FILE:         khe_mmatch.h                                               */
/*  MODULE:       Weighted bipartite matching with multi-nodes               */
/*                                                                           */
/*  This module offers weighted bipartite matching, like khe_wmatch.h        */
/*  but with a different selection of features.  The aim is to run as        */
/*  fast as possible, to offer multi-nodes (nodes with a multiplicity,       */
/*  representing that many individual nodes with the same edges), and        */
/*  to offer 96-bits costs, in the form of (int, int, int) triples.          */
/*                                                                           */
/*  These features are needed by time sweep resource assignment.             */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_MMATCH_HEADER_FILE
#define KHE_MMATCH_HEADER_FILE
#include <stdio.h>
#include <stdbool.h>
#include "howard_a.h"

/* object type declarations */
typedef struct khe_mmatch_node_rec	*KHE_MMATCH_NODE;
typedef struct khe_mmatch_rec		*KHE_MMATCH;

/* function type declarations */
typedef void (*MMATCH_DEBUG_FN)(void *value, int verbosity,
  int indent, FILE *fp);
typedef void (*MMATCH_COST_DEBUG_FN)(int c1, int c2, int c3, FILE *fp);

/* matchings */
extern KHE_MMATCH KheMMatchMake(void *back, MMATCH_DEBUG_FN back_debug,
  MMATCH_DEBUG_FN demand_back_debug, MMATCH_DEBUG_FN supply_back_debug,
  MMATCH_COST_DEBUG_FN cost_debug, HA_ARENA a);
extern void KheMMatchClear(KHE_MMATCH m);

/* demand nodes */
extern KHE_MMATCH_NODE KheMMatchDemandNodeMake(KHE_MMATCH m,
  int multiplicity, void *back);
extern KHE_MMATCH KheMMatchDemandNodeMMatch(KHE_MMATCH_NODE dn);
extern void *KheMMatchDemandNodeBack(KHE_MMATCH_NODE dn);

/* supply nodes */
extern KHE_MMATCH_NODE KheMMatchSupplyNodeMake(KHE_MMATCH m,
  int multiplicity, void *back);
extern KHE_MMATCH KheMMatchSupplyNodeMMatch(KHE_MMATCH_NODE sn);
extern void *KheMMatchSupplyNodeBack(KHE_MMATCH_NODE sn);

/* edges */
extern void KheMMatchAddEdge(KHE_MMATCH_NODE dn, KHE_MMATCH_NODE sn,
  int multiplicity, int c1, int c2, int c3);

/* solving and result reporting */
extern bool KheMMatchSolve(KHE_MMATCH m);
extern bool KheMMatchResultEdge(KHE_MMATCH_NODE dn, KHE_MMATCH_NODE *sn,
  int *multiplicity, int *c1, int *c2, int *c3);

/* debugging and testing */
extern void KheMMatchDebug(KHE_MMATCH m, int verbosity, int indent, FILE *fp);
extern void KheMMatchTest(FILE *fp);

#endif

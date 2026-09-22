
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
/*  FILE:         khe_part_graph.h                                           */
/*  DESCRIPTION:  Generic two-colouring                                      */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_PART_GRAPH_HEADER_FILE
#define KHE_PART_GRAPH_HEADER_FILE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "howard_a.h"

/*****************************************************************************/
/*                                                                           */
/*  Type declarations                                                        */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_PART_GRAPH_UNRELATED,
  KHE_PART_GRAPH_DIFFERENT,
  KHE_PART_GRAPH_SAME
} KHE_PART_GRAPH_REL;

typedef KHE_PART_GRAPH_REL (*KHE_PART_GRAPH_REL_FN)(void *node1, void *node2);

typedef struct khe_part_graph_graph_rec *KHE_PART_GRAPH;
typedef struct khe_part_graph_component_rec *KHE_PART_GRAPH_COMPONENT;
typedef struct khe_part_graph_part_rec *KHE_PART_GRAPH_PART;


/*****************************************************************************/
/*                                                                           */
/*  Functions                                                                */
/*                                                                           */
/*****************************************************************************/

extern KHE_PART_GRAPH KhePartGraphMake(KHE_PART_GRAPH_REL_FN rel_fn,
  HA_ARENA a);
/* extern void KhePartGraphDelete(KHE_PART_GRAPH graph); */
extern void KhePartGraphAddNode(KHE_PART_GRAPH graph, void *node);
extern int KhePartGraphNodeCount(KHE_PART_GRAPH graph);
extern void *KhePartGraphNode(KHE_PART_GRAPH graph, int i);

extern void KhePartGraphFindConnectedComponents(KHE_PART_GRAPH graph);
extern int KhePartGraphComponentCount(KHE_PART_GRAPH graph);
extern KHE_PART_GRAPH_COMPONENT KhePartGraphComponent(KHE_PART_GRAPH graph,
  int i);
extern KHE_PART_GRAPH KhePartGraphComponentGraph(KHE_PART_GRAPH_COMPONENT comp);
extern int KhePartGraphComponentNodeCount(KHE_PART_GRAPH_COMPONENT comp);
extern void *KhePartGraphComponentNode(KHE_PART_GRAPH_COMPONENT comp, int i);

extern void KhePartGraphComponentFindParts(KHE_PART_GRAPH_COMPONENT comp);
extern bool KhePartGraphComponentParts(KHE_PART_GRAPH_COMPONENT comp,
  KHE_PART_GRAPH_PART *part1, KHE_PART_GRAPH_PART *part2);
extern KHE_PART_GRAPH_COMPONENT KhePartGraphPartComponent(
  KHE_PART_GRAPH_PART part);
extern int KhePartGraphPartNodeCount(KHE_PART_GRAPH_PART part);
extern void *KhePartGraphPartNode(KHE_PART_GRAPH_PART part, int i);

#endif

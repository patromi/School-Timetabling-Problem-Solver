
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
/*  FILE:         khe_priqueue.h                                             */
/*  DESCRIPTION:  Generic priority queue                                     */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_PRIQUEUE_HEADER_FILE
#define KHE_PRIQUEUE_HEADER_FILE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "howard_a.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_PRIQUEUE                                                             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_priqueue_rec *KHE_PRIQUEUE;

typedef int64_t (*KHE_PRIQUEUE_KEY_FN)(void *entry);
typedef int (*KHE_PRIQUEUE_INDEX_GET_FN)(void *entry);
typedef void (*KHE_PRIQUEUE_INDEX_SET_FN)(void *entry, int index);


/*****************************************************************************/
/*                                                                           */
/*  Functions                                                                */
/*                                                                           */
/*****************************************************************************/

extern KHE_PRIQUEUE KhePriQueueMake(KHE_PRIQUEUE_KEY_FN key,
  KHE_PRIQUEUE_INDEX_GET_FN index_get,
  KHE_PRIQUEUE_INDEX_SET_FN index_set, HA_ARENA a);
/* extern void KhePriQueueDelete(KHE_PRIQUEUE p); */
extern bool KhePriQueueEmpty(KHE_PRIQUEUE p);
extern void KhePriQueueInsert(KHE_PRIQUEUE p, void *entry);
extern void *KhePriQueueFindMin(KHE_PRIQUEUE p);
extern void *KhePriQueueDeleteMin(KHE_PRIQUEUE p);
extern void KhePriQueueDeleteEntry(KHE_PRIQUEUE p, void *entry);
extern void KhePriQueueClear(KHE_PRIQUEUE p);
extern void KhePriQueueNotifyKeyChange(KHE_PRIQUEUE p, void *entry);

/* testing */
extern void KhePriQueueTest(FILE *fp);

#endif

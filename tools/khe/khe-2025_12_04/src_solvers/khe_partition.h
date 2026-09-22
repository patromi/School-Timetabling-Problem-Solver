
/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2008 Jeffrey H. Kingston                             */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  FILE:         khe_partition.h                                            */
/*  MODULE:       Partitions                                                 */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_PARTITION_HEADER_FILE
#define KHE_PARTITION_HEADER_FILE
#include <stdio.h>
#include <stdbool.h>
#include "howard_a.h"

typedef struct khe_partition_rec	*KHE_PARTITION;
typedef HA_ARRAY(KHE_PARTITION)		ARRAY_KHE_PARTITION;

/* implementation-specific code */
extern KHE_PARTITION KhePartitionMake(HA_ARENA a);
/* extern void KhePartitionFree(KHE_PARTITION p); */
extern void KhePartitionClear(KHE_PARTITION p);
extern KHE_PARTITION KhePartitionCopy(KHE_PARTITION p, HA_ARENA a);
extern bool KhePartitionEqual(KHE_PARTITION p1, KHE_PARTITION p2);
extern bool KhePartitionIsEmpty(KHE_PARTITION p);
extern int KhePartitionSize(KHE_PARTITION p);
extern int KhePartitionPartsWithSize(KHE_PARTITION p, int i);
extern int KhePartitionMax(KHE_PARTITION p);
extern int KhePartitionMin(KHE_PARTITION p);
extern bool KhePartitionContains(KHE_PARTITION p, int i);
extern bool KhePartitionContainsAtLeast(KHE_PARTITION p, int i, int *res);
extern bool KhePartitionContainsAtMost(KHE_PARTITION p, int i, int *res);
extern bool KhePartitionCovers(KHE_PARTITION p1, KHE_PARTITION p2);
extern int KhePartitionParts(KHE_PARTITION p);
extern void KhePartitionAdd(KHE_PARTITION p, int i);
extern void KhePartitionSub(KHE_PARTITION p, int i);
extern void KhePartitionMul(KHE_PARTITION p, int i);
extern void KhePartitionDiv(KHE_PARTITION p, int i);
extern void KhePartitionDivAndRound(KHE_PARTITION p, int i);
extern void KhePartitionDivAndRoundUp(KHE_PARTITION p, int i);
extern void KhePartitionReduce(KHE_PARTITION p, int i);
extern void KhePartitionSum(KHE_PARTITION p1, KHE_PARTITION p2);
extern void KhePartitionAssign(KHE_PARTITION p1, KHE_PARTITION p2);
extern void KhePartitionDifference(KHE_PARTITION p1, KHE_PARTITION p2);
extern void KhePartitionMaxToLimit(KHE_PARTITION p1, KHE_PARTITION p2,
  int limit);
extern char *KhePartitionShow(KHE_PARTITION p, HA_ARENA a);
extern char *KhePartitionShowRaw(KHE_PARTITION p, HA_ARENA a);
extern char *KhePartitionShowBrief(KHE_PARTITION p, HA_ARENA a);

/* implementation-independent code */
extern bool KhePartitionFromString(char *str, int total, KHE_PARTITION *p,
  char **diagnosis, HA_ARENA a);
extern bool KhePartitionListEqual(ARRAY_KHE_PARTITION *sap,
  ARRAY_KHE_PARTITION *iap);
extern KHE_PARTITION KhePartitionUnitary(int width, HA_ARENA a);
extern void KhePartitionsOf(int n, int m, ARRAY_KHE_PARTITION *res, HA_ARENA a);
extern void KhePartitionsUpTo(int n, int m, ARRAY_KHE_PARTITION *res,
  HA_ARENA a);
extern bool KhePartitionBinPack(KHE_PARTITION p1, KHE_PARTITION p2);
extern bool KhePartitionBinPackAndHow(KHE_PARTITION p1, KHE_PARTITION p2,
  ARRAY_KHE_PARTITION *partitions, HA_ARENA a);
extern void KhePartitionTest(FILE *fp);

#endif

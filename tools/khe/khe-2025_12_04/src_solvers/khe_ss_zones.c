
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
/*  FILE:         khe_ss_zones.c                                             */
/*  DESCRIPTION:  Zone solvers                                               */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerInstallZonesInParent(KHE_LAYER layer)                       */
/*                                                                           */
/*  Install zones in layer's parent corresponding to how the meets           */
/*  of the nodes of layer are assigned.                                      */
/*                                                                           */
/*  NB despite its location in this file, this function is really a          */
/*  solver; it depends on khe.h but not khe_interns.h.                       */
/*                                                                           */
/*****************************************************************************/

void KheLayerInstallZonesInParent(KHE_LAYER layer)
{
  KHE_NODE parent_node, child_node;  KHE_ZONE zone;
  KHE_MEET child_meet, parent_meet;  int i, j, k, parent_offset;
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheLayerInstallZonesInParent(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* remove existing zones from the parent of layer */
  parent_node = KheLayerParentNode(layer);
  while( KheNodeZoneCount(parent_node) > 0 )
    KheZoneDelete(KheNodeZone(parent_node, 0));

  /* make one zone for each child node of layer that has assigned meets */
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    if( KheNodeAssignedDuration(child_node) > 0 )
    {
      zone = NULL;
      for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
      {
	child_meet = KheNodeMeet(child_node, j);
	if( KheMeetAsst(child_meet) != NULL )
	{
          parent_meet = KheMeetAsst(child_meet);
	  parent_offset = KheMeetAsstOffset(child_meet);
	  for( k = 0;  k < KheMeetDuration(child_meet);  k++ )
	    if( KheMeetOffsetZone(parent_meet, parent_offset + k) == NULL )
	    {
	      if( zone == NULL )
		zone = KheZoneMake(parent_node);
	      KheZoneAddMeetOffset(zone, parent_meet, parent_offset + k);
	    }
	}
      }
      if( DEBUG3 )
      {
	fprintf(stderr, "  Zone %d from child node ", KheZoneNodeIndex(zone));
	KheNodeDebug(child_node, 1, 0, stderr);
      }
    }
  }

  /* all done */
  if( DEBUG3 )
  {
    /* ***
    fprintf(stderr, "  final parent node:");
    KheNodeDebug(parent_node, 3, 2, stderr);
    *** */
    fprintf(stderr, "] KheLayerInstallZonesInParent returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RUN - one run of a zone, possibly a NULL zone"            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_run_rec {
  KHE_MEET		meet;			/* the meet                  */
  int			start;			/* offset where run starts   */
  int			stop;			/* offset after it stops     */
  KHE_ZONE		zone;			/* the zone through the run  */
} KHE_RUN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUN KheRunMake(KHE_MEET meet, int start, int stop, KHE_ZONE zone)    */
/*                                                                           */
/*  Make a new run record with these attributes.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_RUN KheRunMake(KHE_MEET meet, int start, int stop, KHE_ZONE zone)
{
  KHE_RUN res;
  res.meet = meet;
  res.start = start;
  res.stop = stop;
  res.zone = zone;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUN KheRunNext(KHE_RUN zr)                                           */
/*                                                                           */
/*  Return the next run after zr, possibly an empty run if off the end.      */
/*                                                                           */
/*****************************************************************************/

static KHE_RUN KheRunNext(KHE_RUN zr)
{
  int stop;  KHE_ZONE zone;
  if( zr.stop >= KheMeetDuration(zr.meet) )
    return KheRunMake(zr.meet, zr.stop, zr.stop, NULL);
    /* return (KHE_RUN) {zr.meet, zr.stop, zr.stop, NULL}; */
  zone = KheMeetOffsetZone(zr.meet, zr.stop);
  for( stop = zr.stop + 1;  stop < KheMeetDuration(zr.meet) &&
     KheMeetOffsetZone(zr.meet, stop) == zone;  stop++ );
  return KheRunMake(zr.meet, zr.stop, stop, zone);
  /* return (KHE_RUN) {zr.meet, zr.stop, stop, zone}; */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUN KheRunPrev(KHE_RUN zr)                                           */
/*                                                                           */
/*  Return the previous run to zr, possibly an empty run if off the end.     */
/*                                                                           */
/*****************************************************************************/

static KHE_RUN KheRunPrev(KHE_RUN zr)
{
  int start;  KHE_ZONE zone;
  if( zr.start == 0 )
    return KheRunMake(zr.meet, 0, 0, NULL);
    /* return (KHE_RUN) {zr.meet, 0, 0, NULL}; */
  zone = KheMeetOffsetZone(zr.meet, zr.start - 1);
  for( start = zr.start - 2;  start >= 0 &&
     KheMeetOffsetZone(zr.meet, start) == zone;  start-- );
  return KheRunMake(zr.meet, start + 1, zr.start, zone);
  /* return (KHE_RUN) {zr.meet, start + 1, zr.start, zone}; */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUN KheEmptyRun(KHE_MEET meet)                                       */
/*                                                                           */
/*  Return an empty run for meet.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_RUN KheEmptyRun(KHE_MEET meet)
{
  return KheRunMake(meet, 0, 0, NULL);
  /* return (KHE_RUN) {meet, 0, 0, NULL}; */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRunIsEmpty(KHE_RUN zr)                                           */
/*                                                                           */
/*  Return true if zr is empty.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheRunIsEmpty(KHE_RUN zr)
{
  return zr.start == zr.stop;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUN KheRunFirst(KHE_MEET meet)                                       */
/*                                                                           */
/*  Return the first run of meet.  There is always a first run.              */
/*                                                                           */
/*****************************************************************************/

static KHE_RUN KheRunFirst(KHE_MEET meet)
{
  return KheRunNext(KheEmptyRun(meet));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRunDuration(KHE_RUN r)                                            */
/*                                                                           */
/*  Return the duration of r, or 0 if r is empty.                            */
/*                                                                           */
/*****************************************************************************/

static int KheRunDuration(KHE_RUN r)
{
  return r.stop - r.start;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_OPTION - one option for assigning a zone to a meet"       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_option_rec {
  KHE_MEET		meet;				/* the meet          */
  int			offset;				/* offset in meet    */
  int			duration;			/* number of offsets */
  KHE_ZONE		zone;				/* zone to assign    */
  int			priority;			/* priority          */
} KHE_OPTION;


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTION KheOptionMake(KHE_MEET meet, int offset, int duration,        */
/*    KHE_ZONE zone, int priority)                                           */
/*                                                                           */
/*  Make a new option record with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_OPTION KheOptionMake(KHE_MEET meet, int offset, int duration,
  KHE_ZONE zone, int priority)
{
  KHE_OPTION res;
  res.meet = meet;
  res.offset = offset;
  res.duration = duration;
  res.zone = zone;
  res.priority = priority;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTION KheOptionNull(void)                                           */
/*                                                                           */
/*  Return the null option.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_OPTION KheOptionNull(void)
{
  return KheOptionMake(NULL, 0, 0, NULL, INT_MAX);
  /* return (KHE_OPTION) {NULL, 0, 0, NULL, INT_MAX}; */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionIsNull(KHE_OPTION opt)                                     */
/*                                                                           */
/*  Return true if opt is null.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheOptionIsNull(KHE_OPTION opt)
{
  return opt.meet == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionExecute(KHE_OPTION opt)                                    */
/*                                                                           */
/*  Carry out opt.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheOptionExecute(KHE_OPTION opt)
{
  int i;
  for( i = 0;  i < opt.duration;  i++ )
    KheZoneAddMeetOffset(opt.zone, opt.meet, opt.offset + i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheNodeIdealZoneDuration(KHE_NODE node)                              */
/*                                                                           */
/*  Return the ideal duration of a zone in node.                             */
/*                                                                           */
/*****************************************************************************/

static int KheNodeIdealZoneDuration(KHE_NODE node)
{
  int denom;
  denom = KheNodeZoneCount(node) * KheNodeMeetCount(node);
  return (KheNodeDuration(node) + denom - 1) / denom;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAbs(int val)                                                      */
/*                                                                           */
/*  Return the absolute value of val.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheAbs(int val)
{
  return val < 0 ? - val : val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLim(int min_val, int max_val, int val)                            */
/*                                                                           */
/*  Return the closest value x to val such that min_val <= x <= max_val.     */
/*                                                                           */
/*****************************************************************************/

static int KheLim(int min_val, int max_val, int val)
{
  return val < min_val ? min_val : val > max_val ? max_val : val;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFindOption(KHE_MEET meet, KHE_ZONE zone, KHE_OPTION *best_opt)   */
/*                                                                           */
/*  Find an option for assigning some of zone to meet, and if it is better   */
/*  than best_opt, copy it into best_opt.                                    */
/*                                                                           */
/*****************************************************************************/
#define KHE_WEIGHT_ZONE 1			/* zone size weight          */
#define KHE_WEIGHT_FRAG 4			/* fragmentation weight      */
#define KHE_WEIGHT_DURN 1			/* run duration weight       */

static void KheFindOption(KHE_MEET meet, KHE_ZONE zone, KHE_OPTION *best_opt)
{
  KHE_NODE node;  KHE_RUN r, prev_r, next_r;  KHE_OPTION opt;
  int ideal_durn, rdurn, zone_size, existing_runs, extra_runs;

  /* find the ideal run duration and the current size of the zone */
  node = KheMeetNode(meet);
  ideal_durn = KheNodeIdealZoneDuration(node);
  zone_size = KheZoneMeetOffsetCount(zone);

  /* find the number of runs of this zone already in meet */
  existing_runs = 0;
  for( r = KheRunFirst(meet);  !KheRunIsEmpty(r);  r = KheRunNext(r) )
    if( r.zone == zone )
      existing_runs++;

  /* try each zoneless run in turn */
  opt = KheOptionMake(meet, 0, 0, zone, 0);
  for( r = KheRunFirst(meet);  !KheRunIsEmpty(r);  r = KheRunNext(r) )
    if( r.zone == NULL )
    {
      prev_r = KheRunPrev(r);
      next_r = KheRunNext(r);
      rdurn = KheRunDuration(r);
      if( prev_r.zone == zone && next_r.zone == zone )
      {
	/* in the unlikely event of same zone each side, fill it up */
	opt.duration = rdurn;
	opt.offset = r.start;
	extra_runs = -1;
      }
      else if( prev_r.zone == zone )
      {
	/* if same before, extend that */
	opt.duration = KheLim(1, rdurn, ideal_durn - KheRunDuration(prev_r));
	opt.offset = r.start;
	extra_runs = 0;
      }
      else if( next_r.zone == zone )
      {
	/* if same after, extend that */
	opt.duration = KheLim(1, rdurn, ideal_durn - KheRunDuration(next_r));
	opt.offset = next_r.start - opt.duration;
	extra_runs = 0;
      }
      else
      {
	/* if not same nearby, just fill some space */
	opt.duration = KheLim(1, rdurn, ideal_durn);
	opt.offset = r.start;
	extra_runs = 1;
      }
      opt.priority = (zone_size + opt.duration) * KHE_WEIGHT_ZONE +
	(existing_runs + extra_runs) * KHE_WEIGHT_FRAG +
	KheAbs(opt.duration - ideal_durn) * KHE_WEIGHT_DURN ;
      if( opt.priority < best_opt->priority )
	*best_opt = opt;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeDebugZones(KHE_NODE node, int indent, FILE *fp)              */
/*                                                                           */
/*  Debug print of the zones of node.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheNodeDebugZones(KHE_NODE node, int indent, FILE *fp)
{
  KHE_MEET meet;  int i, offset;  KHE_ZONE zone;
  fprintf(fp, "%*s[ Zones of ", indent, "");
  KheNodeDebug(node, 1, 0, fp);
  fprintf(fp, "%*s  --------------------------------------------------\n",
    indent, "");
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    fprintf(fp, "%*s  Meet %d:", indent, "", i);
    for( offset = 0;  offset < KheMeetDuration(meet);  offset++ )
    {
      zone = KheMeetOffsetZone(meet, offset);
      if( zone == NULL )
	fprintf(fp, "  --");
      else
	fprintf(fp, "  Z%d", KheZoneNodeIndex(zone));
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "%*s  --------------------------------------------------\n",
    indent, "");
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTION KheNodeBestOption(KHE_NODE node)                              */
/*                                                                           */
/*  Return the best option for assigning node, or the null option if none.   */
/*                                                                           */
/*****************************************************************************/

static KHE_OPTION KheNodeBestOption(KHE_NODE node)
{
  int i, j;  KHE_OPTION best_opt;
  best_opt = KheOptionNull();
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
    for( j = 0;  j < KheNodeZoneCount(node);  j++ )
      KheFindOption(KheNodeMeet(node, i), KheNodeZone(node, j), &best_opt);
  return best_opt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeExtendZones(KHE_NODE node)                                   */
/*                                                                           */
/*  Heuristically extend the zones of node so that every offset of every     */
/*  meet of node has a zone.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheNodeExtendZones(KHE_NODE node)
{
  KHE_OPTION best_opt;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheNodeExtendZones(");
    KheNodeDebug(node, 1, -1, stderr);
    fprintf(stderr, ")\n");
    KheNodeDebugZones(node, 2, stderr);
  }
  if( KheNodeZoneCount(node) > 0 )
  {
    best_opt = KheNodeBestOption(node);
    while( !KheOptionIsNull(best_opt) )
    {
      KheOptionExecute(best_opt);
      best_opt = KheNodeBestOption(node);
    }
  }
  if( DEBUG1 )
  {
    KheNodeDebugZones(node, 2, stderr);
    fprintf(stderr, "] KheNodeExtendZones returning\n");
  }
}


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
/*  FILE:         khe_soln_write_only.c                                      */
/*  DESCRIPTION:  A solution in an abbreviated, write-only form              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_WRITE_ONLY - a task in write-only form                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_write_only_rec {
  KHE_EVENT_RESOURCE		event_resource;		/* event resource    */
  KHE_RESOURCE			assigned_resource;	/* assigned resource */
} KHE_TASK_WRITE_ONLY;

typedef HA_ARRAY(KHE_TASK_WRITE_ONLY) ARRAY_KHE_TASK_WRITE_ONLY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_WRITE_ONLY - a meet in write-only form                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_meet_write_only_rec {
  KHE_EVENT			event;			/* comes from event  */
  int				duration;		/* duration          */
  KHE_TIME			assigned_time;		/* if any            */
  ARRAY_KHE_TASK_WRITE_ONLY	tasks;			/* tasks             */
} *KHE_MEET_WRITE_ONLY;

typedef HA_ARRAY(KHE_MEET_WRITE_ONLY) ARRAY_KHE_MEET_WRITE_ONLY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_WRITE_ONLY - a solution in write-only form                      */
/*                                                                           */
/*****************************************************************************/

struct khe_soln_write_only_rec {
  HA_ARENA			arena;			/* holds this obj    */
  /* KHE_INSTANCE		instance; */		/* instance solved   */
  /* char			*description; */	/* description       */
  /* float			running_time; */	/* running_time      */
  ARRAY_KHE_MEET_WRITE_ONLY	meets;			/* all meets         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyMake(HA_ARENA a, KHE_INSTANCE ins,   */
/*    char *description, float running_time)                                 */
/*                                                                           */
/*  Make and return a new write-only soln with these attributes, and         */
/*  initially no meets.                                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyMake(HA_ARENA a)
  /* , KHE_INSTANCE ins, char *description, float running_time) */
{
  KHE_SOLN_WRITE_ONLY res;
  HaMake(res, a);
  res->arena = a;
  /* res->instance = ins; */
  /* res->description = HnStringCopy(description, a); */
  /* res->running_time = running_time; */
  HaArrayInit(res->meets, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_WRITE_ONLY KheSolnWriteOnlyAddMeet(KHE_SOLN_WRITE_ONLY swo,     */
/*    KHE_EVENT event, int duration, KHE_TIME assigned_time)                 */
/*                                                                           */
/*  Make, add to swo, and return a new write-only meet with these            */
/*  attributes, and initially no tasks.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET_WRITE_ONLY KheSolnWriteOnlyAddMeet(KHE_SOLN_WRITE_ONLY swo,
  KHE_EVENT event, int duration, KHE_TIME assigned_time)
{
  KHE_MEET_WRITE_ONLY res;
  HaMake(res, swo->arena);
  res->event = event;
  res->duration = duration;
  res->assigned_time = assigned_time;
  HaArrayInit(res->tasks, swo->arena);
  HaArrayAddLast(swo->meets, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_WRITE_ONLY KheMeetWriteOnlyAddTask(KHE_MEET_WRITE_ONLY mwo,     */
/*    KHE_EVENT_RESOURCE er, KHE_RESOURCE r)                                 */
/*                                                                           */
/*  Make, add to mwo, and return a new write-only task with these            */
/*  attributes.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetWriteOnlyAddTask(KHE_MEET_WRITE_ONLY mwo,
  KHE_EVENT_RESOURCE er, KHE_RESOURCE r)
{
  KHE_TASK_WRITE_ONLY res;
  res.event_resource = er;
  res.assigned_resource = r;
  HaArrayAddLast(mwo->tasks, res);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetIncreasingIndexAndTimeCmp(const void *t1, const void *t2)     */
/*                                                                           */
/*  Comparison function for sorting an array of write-only meets by          */
/*  increasing event index and assigned time.                                */
/*                                                                           */
/*****************************************************************************/

static int KheMeetIncreasingIndexAndTimeCmp(const void *t1, const void *t2)
{
  KHE_MEET_WRITE_ONLY mwo1 = * (KHE_MEET_WRITE_ONLY *) t1;
  KHE_MEET_WRITE_ONLY mwo2 = * (KHE_MEET_WRITE_ONLY *) t2;
  int event_index1 = KheEventIndex(mwo1->event);
  int event_index2 = KheEventIndex(mwo2->event);
  int cmp = event_index1 - event_index2;
  if( cmp != 0 )
    return cmp;
  else if( mwo1->assigned_time == NULL )
    return 1;
  else if( mwo2->assigned_time == NULL )
    return -1;
  else
   return KheTimeIndex(mwo1->assigned_time) - KheTimeIndex(mwo2->assigned_time);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyFromSoln(KHE_SOLN soln, HA_ARENA a)  */
/*                                                                           */
/*  Return a write-only version of soln, in arena a.                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyFromSoln(KHE_SOLN soln, HA_ARENA a)
{
  KHE_SOLN_WRITE_ONLY res;  KHE_MEET meet;  KHE_TASK task;
  KHE_MEET_WRITE_ONLY mwo;  int i, j, k;  KHE_INSTANCE ins;  KHE_EVENT e;

  /* build the initial object */
  ins = KheSolnInstance(soln);
  /* ***
  KheSolnHasRunningTime(soln, &running_time);
  res = KheSolnWriteOnlyMake(a, ins, KheSolnDescription(soln), running_time);
  *** */
  res = KheSolnWriteOnlyMake(a);

  /* add the meets and their tasks */
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
    {
      meet = KheEventMeet(soln, e, j);
      KheMeetCheckForWriting(meet);
      if( KheMeetMustWrite(meet) )
      {
	mwo = KheSolnWriteOnlyAddMeet(res, KheMeetEvent(meet),
	  KheMeetDuration(meet), KheMeetAsstTime(meet));
	for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
	{
	  task = KheMeetTask(meet, k);
	  KheTaskCheckForWriting(task);
	  if( KheTaskMustWrite(task) )
	    KheMeetWriteOnlyAddTask(mwo, KheTaskEventResource(task),
	      KheTaskAsstResource(task));
	}
      }
    }
  }

  /* sort the meets by increasing event index then assigned time */
  HaArraySort(res->meets, &KheMeetIncreasingIndexAndTimeCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyCopyPhase1(KHE_SOLN_WRITE_ONLY swo,  */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of copying swo.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_WRITE_ONLY KheSolnWriteOnlyCopyPhase1(KHE_SOLN_WRITE_ONLY swo,
  HA_ARENA a)
{
  KHE_SOLN_WRITE_ONLY res;  KHE_MEET_WRITE_ONLY meet, cm;  int i, j;
  KHE_TASK_WRITE_ONLY task;
  res = KheSolnWriteOnlyMake(a);
    /* , swo->instance, swo->description, swo->running_time); */
  HaArrayForEach(swo->meets, meet, i)
  {
    cm = KheSolnWriteOnlyAddMeet(res, meet->event, meet->duration,
      meet->assigned_time);
    HaArrayForEach(meet->tasks, task, j)
      KheMeetWriteOnlyAddTask(cm, task.event_resource, task.assigned_resource);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnWriteOnlyCopyPhase2(KHE_SOLN_WRITE_ONLY swo)                 */
/*                                                                           */
/*  Carry out Phase 2 of copying swo.                                        */
/*                                                                           */
/*****************************************************************************/

void KheSolnWriteOnlyCopyPhase2(KHE_SOLN_WRITE_ONLY swo)
{
  /* nothing to do here */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "writing"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskWriteOnlyWrite(KHE_TASK_WRITE_ONLY two, KML_FILE kf)         */
/*                                                                           */
/*  Write two to kf.                                                         */
/*                                                                           */
/*  Implementation note.  This function follows KheTaskWrite.                */
/*                                                                           */
/*****************************************************************************/

static void KheTaskWriteOnlyWrite(KHE_TASK_WRITE_ONLY two, KML_FILE kf)
{
  KmlEltAttributeEltPlainText(kf, "Resource", "Reference",
    KheResourceId(two.assigned_resource), "Role",
    KheEventResourceRole(two.event_resource));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetWriteOnlyWrite(KHE_MEET_WRITE_ONLY mwo, KML_FILE kf)         */
/*                                                                           */
/*  Write mwo to kf.                                                         */
/*                                                                           */
/*  Implementation note.  This function follows KheMeetWrite.                */
/*                                                                           */
/*****************************************************************************/

static void KheMeetWriteOnlyWrite(KHE_MEET_WRITE_ONLY mwo, KML_FILE kf)
{
  KHE_TIME ass_t, pre_t;  KHE_TASK_WRITE_ONLY two;  int i;

  /* print event header, and duration if required */
  KmlBegin(kf, "Event");
  KmlAttribute(kf, "Reference", KheEventId(mwo->event));
  if( mwo->duration != KheEventDuration(mwo->event) )
    KmlEltFmtText(kf, "Duration", "%d", mwo->duration);

  /* print assigned time if present and different from preassigned */
  ass_t = mwo->assigned_time;
  pre_t = KheEventPreassignedTime(mwo->event);
  if( ass_t != NULL && ass_t != pre_t )
    KmlEltAttribute(kf, "Time", "Reference", KheTimeId(ass_t));

  /* print resources */
  if( HaArrayCount(mwo->tasks) > 0 )
  {
    KmlBegin(kf, "Resources");
    HaArrayForEach(mwo->tasks, two, i)
      KheTaskWriteOnlyWrite(two, kf);
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "Event");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnWriteOnlyWrite(KHE_SOLN_WRITE_ONLY swo, KHE_INSTANCE ins,    */
/*    char *description, float running_time, KML_FILE kf)                    */
/*                                                                           */
/*  Write swo to kf.                                                         */
/*                                                                           */
/*  Implementation note.  This function follows KheSolnWrite, except that    */
/*  it is not able to write a report.                                        */
/*                                                                           */
/*****************************************************************************/

void KheSolnWriteOnlyWrite(KHE_SOLN_WRITE_ONLY swo, KHE_INSTANCE ins,
  char *description, float running_time, KML_FILE kf)
{
  KHE_MEET_WRITE_ONLY mwo;  int i;
  KmlBegin(kf, "Solution");
  KmlAttribute(kf, "Reference", KheInstanceId(ins));
  if( description != NULL )
    KmlEltPlainText(kf, "Description", description);
  if( running_time >= 0.0 )
    KmlEltFmtText(kf, "RunningTime", "%.1f", running_time);
  if( HaArrayCount(swo->meets) > 0 )
  {
    KmlBegin(kf, "Events");
    HaArrayForEach(swo->meets, mwo, i)
      KheMeetWriteOnlyWrite(mwo, kf);
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "Solution");
}

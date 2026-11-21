// frontend/src/lib/components/editor/state/modalHandlers.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  ActiveAvailabilityTarget,
  ActiveLessonPrefTarget,
  ResourceContextInfo,
} from "../types";

export function buildAvailabilityTarget(options: {
  id: string;
  name: string;
  type: "teacher" | "room" | "class";
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  getTeacherAssignedHours: (id: string) => number;
  getTeacherClasses: (id: string) => { classId: string; className: string; subject: string; hours: number }[];
  getClassLessons: (id: string) => SimpleLesson[];
  getClassWeeklyHours: (id: string) => number;
}): ActiveAvailabilityTarget {
  const {
    id,
    name,
    type,
    teachers,
    classes,
    rooms,
    lessons,
    getTeacherAssignedHours,
    getTeacherClasses,
    getClassLessons,
    getClassWeeklyHours,
  } = options;

  let unavail: string[] = [];
  let soft = false;
  let w = 10;
  let maxDays: number | null = null;
  let minDays: number | null = null;
  let maxDaily: number | null = null;
  let maxGaps: number | null = null;
  let minDaily: number | null = null;
  let contextInfo: ResourceContextInfo | undefined = undefined;

  if (type === "teacher") {
    const t = teachers.find((tch) => tch.id === id);
    if (t) {
      unavail = t.unavailableTimes || [];
      soft = t.unavailableSoft || false;
      w = t.unavailableWeight || 10;
      maxDays = t.maxDaysPerWeek ?? null;
      minDays = t.minDaysPerWeek ?? null;
      maxDaily = t.maxDailyLessons ?? null;
      maxGaps = t.maxGapsPerWeek ?? null;
      contextInfo = {
        workload: t.workload,
        totalAssignedHours: getTeacherAssignedHours(t.id),
        classes: getTeacherClasses(t.id),
      };
    }
  } else if (type === "class") {
    const c = classes.find((cls) => cls.id === id);
    if (c) {
      unavail = c.unavailableTimes || [];
      soft = c.unavailableSoft || false;
      w = c.unavailableWeight || 10;
      minDaily = c.minDailyLessons ?? null;
      maxDaily = c.maxDailyLessons ?? null;
      const classLessons = getClassLessons(c.id);
      contextInfo = {
        totalAssignedHours: getClassWeeklyHours(c.id),
        lessons: classLessons.map((l) => {
          const tch = teachers.find((t) => t.id === l.teacherId);
          return {
            subject: l.subject,
            teacherName: tch ? tch.name : l.teacherId,
            hours: l.weeklyCount,
          };
        }),
      };
    }
  } else {
    const r = rooms.find((rm) => rm.id === id);
    if (r) {
      unavail = r.unavailableTimes || [];
      soft = r.unavailableSoft || false;
      w = r.unavailableWeight || 10;
      const roomLessons = lessons.filter((l) => l.roomId === r.id);
      contextInfo = {
        totalAssignedHours: roomLessons.reduce((sum, l) => sum + l.weeklyCount, 0),
      };
    }
  }

  return {
    id,
    name,
    type,
    unavailableTimes: unavail,
    isSoft: soft,
    weight: w,
    maxDaysPerWeek: maxDays,
    minDaysPerWeek: minDays,
    maxDailyLessons: maxDaily,
    maxGapsPerWeek: maxGaps,
    minDailyLessons: minDaily,
    contextInfo,
  };
}

export function updateResourceAvailability(options: {
  target: ActiveAvailabilityTarget;
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
  unavailableTimes: string[];
  isSoft: boolean;
  weight: number;
  extraParams?: {
    maxDaysPerWeek?: number | null;
    minDaysPerWeek?: number | null;
    maxDailyLessons?: number | null;
    maxGapsPerWeek?: number | null;
    minDailyLessons?: number | null;
  };
}): {
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
} {
  const { target, teachers, classes, rooms, unavailableTimes, isSoft, weight, extraParams } = options;
  const { id, type } = target;

  if (type === "teacher") {
    const nextTeachers = teachers.map((t) =>
      t.id === id
        ? {
            ...t,
            unavailableTimes,
            unavailableSoft: isSoft,
            unavailableWeight: weight,
            maxDaysPerWeek: extraParams?.maxDaysPerWeek ?? t.maxDaysPerWeek,
            minDaysPerWeek: extraParams?.minDaysPerWeek ?? t.minDaysPerWeek,
            maxDailyLessons: extraParams?.maxDailyLessons ?? t.maxDailyLessons,
            maxGapsPerWeek: extraParams?.maxGapsPerWeek ?? t.maxGapsPerWeek,
          }
        : t
    );
    return { teachers: nextTeachers, classes, rooms };
  } else if (type === "class") {
    const nextClasses = classes.map((c) =>
      c.id === id
        ? {
            ...c,
            unavailableTimes,
            unavailableSoft: isSoft,
            unavailableWeight: weight,
            minDailyLessons: extraParams?.minDailyLessons ?? c.minDailyLessons,
            maxDailyLessons: extraParams?.maxDailyLessons ?? c.maxDailyLessons,
          }
        : c
    );
    return { teachers, classes: nextClasses, rooms };
  } else {
    const nextRooms = rooms.map((r) =>
      r.id === id
        ? {
            ...r,
            unavailableTimes,
            unavailableSoft: isSoft,
            unavailableWeight: weight,
          }
        : r
    );
    return { teachers, classes, rooms: nextRooms };
  }
}

export function buildLessonPrefTarget(
  lesson: SimpleLesson,
  classes: SimpleClass[]
): ActiveLessonPrefTarget {
  const cls = classes.find((c) => c.id === lesson.classId);
  return {
    lessonId: lesson.id,
    subject: lesson.subject,
    className: cls ? cls.name : lesson.classId,
    preference: lesson.timePreference || "any",
    preferredTimes: lesson.preferredTimes || [],
    isSoft: lesson.preferenceSoft ?? true,
    weight: lesson.preferenceWeight ?? 4,
  };
}

export function updateLessonPreferences(
  lessonId: string,
  lessons: SimpleLesson[],
  pref: "any" | "morning" | "afternoon" | "custom",
  times: string[],
  isSoft: boolean,
  weight: number
): SimpleLesson[] {
  return lessons.map((l) =>
    l.id === lessonId
      ? {
          ...l,
          timePreference: pref,
          preferredTimes: times,
          preferenceSoft: isSoft,
          preferenceWeight: weight,
        }
      : l
  );
}

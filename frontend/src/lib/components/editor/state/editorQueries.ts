// frontend/src/lib/components/editor/state/editorQueries.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
} from "../types";

export function detectClassGrade(cls: SimpleClass): number {
  if (cls.grade !== undefined && cls.grade !== null) return cls.grade;
  const match = cls.name.match(/^(\d+)/) || cls.id.match(/^(\d+)/);
  return match ? parseInt(match[1], 10) : 1;
}

export function getAvailableGrades(classes: SimpleClass[]): number[] {
  const grades = new Set<number>();
  classes.forEach((c) => grades.add(detectClassGrade(c)));
  return Array.from(grades).sort((a, b) => a - b);
}

export function getTotalHoursAssigned(lessons: SimpleLesson[]): number {
  return lessons.reduce((sum, l) => sum + l.weeklyCount, 0);
}

export function getTotalTeacherWorkload(teachers: SimpleTeacher[]): number {
  return teachers.reduce((sum, t) => sum + (Number(t.workload) || 0), 0);
}

export function getTotalUnavailabilitiesCount(
  teachers: SimpleTeacher[],
  classes: SimpleClass[],
  rooms: SimpleRoom[]
): number {
  return [...teachers, ...classes, ...rooms].reduce(
    (sum, r) => sum + (r.unavailableTimes?.length || 0),
    0
  );
}

export function getResourcesWithUnavailability(
  teachers: SimpleTeacher[],
  classes: SimpleClass[],
  rooms: SimpleRoom[]
): (SimpleTeacher | SimpleClass | SimpleRoom)[] {
  return [...teachers, ...classes, ...rooms].filter(
    (r) => (r.unavailableTimes?.length || 0) > 0
  );
}

export function getTeachersWithCustomDays(
  teachers: SimpleTeacher[],
  selectedDaysCount: number
): SimpleTeacher[] {
  return teachers.filter((t) => t.maxDaysPerWeek && t.maxDaysPerWeek < selectedDaysCount);
}

export function getTeachersWithCustomDailyLimit(teachers: SimpleTeacher[]): SimpleTeacher[] {
  return teachers.filter((t) => t.maxDailyLessons !== null && t.maxDailyLessons !== undefined);
}

export function getTeachersWithCustomGaps(teachers: SimpleTeacher[]): SimpleTeacher[] {
  return teachers.filter((t) => t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined);
}

export function getClassesWithCustomDailyLimit(classes: SimpleClass[]): SimpleClass[] {
  return classes.filter(
    (c) =>
      (c.minDailyLessons !== null && c.minDailyLessons !== undefined) ||
      (c.maxDailyLessons !== null && c.maxDailyLessons !== undefined)
  );
}

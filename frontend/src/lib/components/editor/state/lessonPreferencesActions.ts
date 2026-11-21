// frontend/src/lib/components/editor/state/lessonPreferencesActions.ts
import type { SimpleLesson, DayFormatted } from "../types";

export function buildQuickLessonPrefTimes(
  pref: "any" | "morning" | "afternoon",
  days: DayFormatted[],
  periodsPerDay: number
): string[] {
  const prefTimes: string[] = [];
  if (pref === "morning") {
    days.forEach((d) => {
      for (let p = 1; p <= Math.min(4, periodsPerDay); p++) {
        prefTimes.push(`${d.id}_${p}`);
      }
    });
  } else if (pref === "afternoon") {
    days.forEach((d) => {
      for (let p = 5; p <= periodsPerDay; p++) {
        prefTimes.push(`${d.id}_${p}`);
      }
    });
  }
  return prefTimes;
}

export function applyQuickPrefToLesson(
  lessonId: string,
  lessons: SimpleLesson[],
  pref: "any" | "morning" | "afternoon",
  days: DayFormatted[],
  periodsPerDay: number
): SimpleLesson[] {
  const prefTimes = buildQuickLessonPrefTimes(pref, days, periodsPerDay);
  return lessons.map((l) =>
    l.id === lessonId
      ? {
          ...l,
          timePreference: pref,
          preferredTimes: prefTimes,
          preferenceSoft: l.preferenceSoft ?? true,
          preferenceWeight: l.preferenceWeight ?? 4,
        }
      : l
  );
}

export function applyBatchPrefToLessons(
  selectedLessonIds: Set<string>,
  lessons: SimpleLesson[],
  pref: "any" | "morning" | "afternoon",
  days: DayFormatted[],
  periodsPerDay: number
): SimpleLesson[] {
  if (selectedLessonIds.size === 0) return lessons;
  const prefTimes = buildQuickLessonPrefTimes(pref, days, periodsPerDay);
  return lessons.map((l) =>
    selectedLessonIds.has(l.id)
      ? {
          ...l,
          timePreference: pref,
          preferredTimes: prefTimes,
          preferenceSoft: l.preferenceSoft ?? true,
          preferenceWeight: l.preferenceWeight ?? 4,
        }
      : l
  );
}

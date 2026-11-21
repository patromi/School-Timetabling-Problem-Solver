// frontend/src/lib/components/editor/state/lessonSelectionActions.ts
import type { SimpleLesson } from "../types";

export function toggleIdInSet(set: Set<string>, id: string): Set<string> {
  const next = new Set(set);
  if (next.has(id)) {
    next.delete(id);
  } else {
    next.add(id);
  }
  return next;
}

export function extractAllLessonIds(lessons: SimpleLesson[]): Set<string> {
  return new Set(lessons.map((l) => l.id));
}

export function resetFiltersState() {
  return {
    searchQuery: "",
    filterClass: "all",
    filterTeacher: "all",
    filterGrade: null as number | null,
    filterPreference: "all" as const,
  };
}

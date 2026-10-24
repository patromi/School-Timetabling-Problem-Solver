// frontend/src/lib/components/editor/state/editorPresets.ts
import type { SimpleClass, SimpleTeacher, SimpleRoom, SimpleLesson } from "../types";
import { generateDefaultCurriculumForClasses } from "./curriculumPresets";

export function getHighSchoolPresetBasics() {
  return {
    schoolName: "Liceum Ogólnokształcące (profilowane)",
    schoolId: "LO_Profilowane",
    selectedDays: ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"],
    periodsPerDay: 8,
  };
}

export function generateDefaultCurriculumLessons(
  classes: SimpleClass[],
  teachers: SimpleTeacher[],
  rooms: SimpleRoom[]
): SimpleLesson[] {
  return generateDefaultCurriculumForClasses(classes, teachers, rooms);
}

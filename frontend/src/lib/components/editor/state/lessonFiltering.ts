// frontend/src/lib/components/editor/state/lessonFiltering.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
} from "../types";

export function filterLessons(options: {
  lessons: SimpleLesson[];
  classes: SimpleClass[];
  teachers: SimpleTeacher[];
  rooms: SimpleRoom[];
  searchQuery: string;
  filterClass: string;
  filterTeacher: string;
  filterGrade: number | null;
  filterPreference: "all" | "has_pref" | "morning" | "afternoon" | "custom" | "none";
  getClassGrade: (cls: SimpleClass) => number;
}): SimpleLesson[] {
  const {
    lessons,
    classes,
    teachers,
    rooms,
    searchQuery,
    filterClass,
    filterTeacher,
    filterGrade,
    filterPreference,
    getClassGrade,
  } = options;

  return lessons.filter((l) => {
    // 1. Wyszukiwarka tekstowa
    if (searchQuery.trim()) {
      const query = searchQuery.toLowerCase().trim();
      const cls = classes.find((c) => c.id === l.classId);
      const t = teachers.find((tch) => tch.id === l.teacherId);
      const r = rooms.find((rm) => rm.id === l.roomId);
      const matchSubject = l.subject.toLowerCase().includes(query);
      const matchClass = cls ? cls.name.toLowerCase().includes(query) : l.classId.toLowerCase().includes(query);
      const matchTeacher = t ? t.name.toLowerCase().includes(query) : l.teacherId.toLowerCase().includes(query);
      const matchRoom = r ? r.name.toLowerCase().includes(query) : false;
      if (!matchSubject && !matchClass && !matchTeacher && !matchRoom) return false;
    }

    // 2. Filtr klasy
    if (filterClass !== "all" && l.classId !== filterClass) {
      return false;
    }

    // 3. Filtr nauczyciela
    if (filterTeacher !== "all" && l.teacherId !== filterTeacher) {
      return false;
    }

    // 4. Filtr rocznika
    if (filterGrade !== null) {
      const cls = classes.find((c) => c.id === l.classId);
      if (!cls || getClassGrade(cls) !== filterGrade) return false;
    }

    // 5. Filtr preferencji
    if (filterPreference === "has_pref" && (!l.timePreference || l.timePreference === "any")) {
      return false;
    }
    if (filterPreference === "none" && l.timePreference && l.timePreference !== "any") {
      return false;
    }
    if (filterPreference === "morning" && l.timePreference !== "morning") {
      return false;
    }
    if (filterPreference === "afternoon" && l.timePreference !== "afternoon") {
      return false;
    }
    if (filterPreference === "custom" && l.timePreference !== "custom") {
      return false;
    }

    return true;
  });
}

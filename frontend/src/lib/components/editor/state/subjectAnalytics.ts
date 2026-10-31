// frontend/src/lib/components/editor/state/subjectAnalytics.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleLesson,
  SubjectSummary,
} from "../types";

export function computeUniqueSubjects(
  lessons: SimpleLesson[],
  teachers: SimpleTeacher[],
  classes: SimpleClass[],
  customSubjects: string[] = []
): SubjectSummary[] {
  const map = new Map<
    string,
    { totalHours: number; lessonCount: number; teachers: Set<string>; classes: Set<string> }
  >();

  // Z zarejestrowanych własnych przedmiotów
  for (const cs of customSubjects) {
    const trimmed = cs?.trim();
    if (trimmed && !map.has(trimmed)) {
      map.set(trimmed, {
        totalHours: 0,
        lessonCount: 0,
        teachers: new Set(),
        classes: new Set(),
      });
    }
  }

  // Z lekcji
  for (const l of lessons) {
    const entry = map.get(l.subject) || {
      totalHours: 0,
      lessonCount: 0,
      teachers: new Set(),
      classes: new Set(),
    };
    entry.totalHours += l.weeklyCount;
    entry.lessonCount += 1;
    if (l.teacherId) {
      const t = teachers.find((tch) => tch.id === l.teacherId);
      entry.teachers.add(t ? t.name : l.teacherId);
    }
    if (l.classId) {
      const c = classes.find((cls) => cls.id === l.classId);
      entry.classes.add(c ? c.name : l.classId);
    }
    map.set(l.subject, entry);
  }

  // Z domyślnych przedmiotów nauczycieli
  for (const t of teachers) {
    if (t.subject) {
      const entry = map.get(t.subject) || {
        totalHours: 0,
        lessonCount: 0,
        teachers: new Set(),
        classes: new Set(),
      };
      entry.teachers.add(t.name);
      map.set(t.subject, entry);
    }
  }

  return Array.from(map.entries())
    .map(([name, data]) => ({
      name,
      lessonCount: data.lessonCount,
      totalHours: data.totalHours,
      teacherCount: data.teachers.size,
      teachers: Array.from(data.teachers),
      classes: Array.from(data.classes),
    }))
    .sort((a, b) => a.name.localeCompare(b.name));
}

export function calculateTeacherAssignedHours(lessons: SimpleLesson[], teacherId: string): number {
  return lessons
    .filter((l) => l.teacherId === teacherId)
    .reduce((sum, l) => sum + l.weeklyCount, 0);
}

export function getTeacherClassAssignments(
  lessons: SimpleLesson[],
  classes: SimpleClass[],
  teacherId: string
): { classId: string; className: string; subject: string; hours: number }[] {
  const list = lessons.filter((l) => l.teacherId === teacherId);
  return list.map((l) => {
    const cls = classes.find((c) => c.id === l.classId);
    return {
      classId: l.classId,
      className: cls ? cls.name : l.classId,
      subject: l.subject,
      hours: l.weeklyCount,
    };
  });
}

export function findConsistentTeacherForClassSubject(
  lessons: SimpleLesson[],
  teachers: SimpleTeacher[],
  classId: string,
  subject: string
): string | null {
  const existing = lessons.find((l) => l.classId === classId && l.subject === subject);
  if (existing) return existing.teacherId;
  const qualified = teachers.filter((t) => t.subject === subject);
  if (qualified.length === 0) return null;
  qualified.sort(
    (a, b) =>
      calculateTeacherAssignedHours(lessons, a.id) - calculateTeacherAssignedHours(lessons, b.id)
  );
  return qualified[0].id;
}

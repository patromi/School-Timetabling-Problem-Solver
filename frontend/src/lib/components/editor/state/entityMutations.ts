// frontend/src/lib/components/editor/state/entityMutations.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  OrderRuleItem,
} from "../types";

export function addClassToState(
  classes: SimpleClass[],
  name: string,
  grade: number
): { classes: SimpleClass[]; newId?: string; error?: string } {
  if (!name.trim()) return { classes };
  const cleanId = name.trim().replace(/\s+/g, "_");
  if (classes.some((c) => c.id === cleanId)) {
    return { classes, error: "Oddział o tej nazwie już istnieje!" };
  }
  const match = name.match(/^(\d+)/);
  const detectedGrade = match ? parseInt(match[1], 10) : grade;
  return {
    classes: [...classes, { id: cleanId, name: name.trim(), grade: detectedGrade }],
    newId: cleanId,
  };
}

export function removeClassFromState(
  classes: SimpleClass[],
  lessons: SimpleLesson[],
  orderRules: OrderRuleItem[],
  classId: string
): {
  classes: SimpleClass[];
  lessons: SimpleLesson[];
  orderRules: OrderRuleItem[];
} {
  const classLessonIds = new Set(lessons.filter((l) => l.classId === classId).map((l) => l.id));
  return {
    classes: classes.filter((c) => c.id !== classId),
    lessons: lessons.filter((l) => l.classId !== classId),
    orderRules: orderRules.filter(
      (r) => !classLessonIds.has(r.firstLessonId) && !classLessonIds.has(r.secondLessonId)
    ),
  };
}

export function addTeacherToState(
  teachers: SimpleTeacher[],
  name: string,
  subject: string,
  workload: number
): { teachers: SimpleTeacher[]; newId?: string; error?: string } {
  if (!name.trim()) return { teachers };
  const cleanId = name.trim().replace(/\s+/g, "_");
  if (teachers.some((t) => t.id === cleanId)) {
    return { teachers, error: "Nauczyciel o tym nazwisku już istnieje!" };
  }
  return {
    teachers: [
      ...teachers,
      {
        id: cleanId,
        name: name.trim(),
        subject,
        workload: Number(workload) || 18,
      },
    ],
    newId: cleanId,
  };
}

export function removeTeacherFromState(
  teachers: SimpleTeacher[],
  teacherId: string
): SimpleTeacher[] {
  return teachers.filter((t) => t.id !== teacherId);
}

export function addRoomToState(
  rooms: SimpleRoom[],
  name: string,
  type: SimpleRoom["type"]
): { rooms: SimpleRoom[]; newId?: string; error?: string } {
  if (!name.trim()) return { rooms };
  const cleanId = name.trim().replace(/\s+/g, "_");
  if (rooms.some((r) => r.id === cleanId)) {
    return { rooms, error: "Sala o tej nazwie już istnieje!" };
  }
  return {
    rooms: [...rooms, { id: cleanId, name: name.trim(), type }],
    newId: cleanId,
  };
}

export function removeRoomFromState(rooms: SimpleRoom[], roomId: string): SimpleRoom[] {
  return rooms.filter((r) => r.id !== roomId);
}

export function addLessonToState(
  lessons: SimpleLesson[],
  rooms: SimpleRoom[],
  params: {
    subject: string;
    classId: string;
    teacherId: string;
    roomId?: string;
    duration?: number;
    weeklyCount?: number;
  }
): { lessons: SimpleLesson[]; error?: string } {
  if (!params.classId || !params.teacherId) {
    return { lessons, error: "Wybierz klasę oraz nauczyciela!" };
  }
  const cleanId = `${params.classId}_${params.subject.replace(/\s+/g, "")}_${Date.now().toString().slice(-3)}`;
  return {
    lessons: [
      ...lessons,
      {
        id: cleanId,
        subject: params.subject,
        classId: params.classId,
        teacherId: params.teacherId,
        roomId: params.roomId || (rooms[0] ? rooms[0].id : ""),
        duration: params.duration || 1,
        weeklyCount: Number(params.weeklyCount) || 1,
      },
    ],
  };
}

export function removeLessonFromState(
  lessons: SimpleLesson[],
  orderRules: OrderRuleItem[],
  lessonId: string
): {
  lessons: SimpleLesson[];
  orderRules: OrderRuleItem[];
} {
  return {
    lessons: lessons.filter((l) => l.id !== lessonId),
    orderRules: orderRules.filter(
      (r) => r.firstLessonId !== lessonId && r.secondLessonId !== lessonId
    ),
  };
}

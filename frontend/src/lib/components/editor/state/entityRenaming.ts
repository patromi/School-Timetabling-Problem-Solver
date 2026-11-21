// frontend/src/lib/components/editor/state/entityRenaming.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  OrderRuleItem,
} from "../types";

export function applySubjectRename(
  oldName: string,
  newName: string,
  lessons: SimpleLesson[],
  teachers: SimpleTeacher[],
  orderRules: OrderRuleItem[]
): {
  lessons: SimpleLesson[];
  teachers: SimpleTeacher[];
  orderRules: OrderRuleItem[];
} {
  const cleanNew = newName.trim();
  if (!cleanNew || oldName === cleanNew) {
    return { lessons, teachers, orderRules };
  }

  // 1. Aktualizacja w lekcjach
  const nextLessons = lessons.map((l) =>
    l.subject === oldName ? { ...l, subject: cleanNew } : l
  );

  // 2. Aktualizacja w nauczycielach
  const nextTeachers = teachers.map((t) =>
    t.subject === oldName ? { ...t, subject: cleanNew } : t
  );

  // 3. Aktualizacja w regułach kolejności
  const nextRules = orderRules.map((r) => {
    const l1 = nextLessons.find((l) => l.id === r.firstLessonId);
    const l2 = nextLessons.find((l) => l.id === r.secondLessonId);
    return {
      ...r,
      name: `${l1?.subject || "Lekcja 1"} przed ${l2?.subject || "Lekcja 2"}`,
    };
  });

  return {
    lessons: nextLessons,
    teachers: nextTeachers,
    orderRules: nextRules,
  };
}

export function applyClassUpdate(
  classId: string,
  updated: Partial<SimpleClass>,
  classes: SimpleClass[],
  lessons: SimpleLesson[]
): {
  classes: SimpleClass[];
  lessons: SimpleLesson[];
  newId: string;
} {
  const oldId = classId;
  const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

  const nextClasses = classes.map((c) =>
    c.id === oldId ? { ...c, ...updated, id: newId } : c
  );

  let nextLessons = lessons;
  if (newId !== oldId) {
    nextLessons = lessons.map((l) =>
      l.classId === oldId ? { ...l, classId: newId } : l
    );
  }

  return { classes: nextClasses, lessons: nextLessons, newId };
}

export function applyTeacherUpdate(
  teacherId: string,
  updated: Partial<SimpleTeacher>,
  teachers: SimpleTeacher[],
  lessons: SimpleLesson[]
): {
  teachers: SimpleTeacher[];
  lessons: SimpleLesson[];
  newId: string;
} {
  const oldId = teacherId;
  const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

  const nextTeachers = teachers.map((t) =>
    t.id === oldId ? { ...t, ...updated, id: newId } : t
  );

  let nextLessons = lessons;
  if (newId !== oldId) {
    nextLessons = lessons.map((l) =>
      l.teacherId === oldId ? { ...l, teacherId: newId } : l
    );
  }

  return { teachers: nextTeachers, lessons: nextLessons, newId };
}

export function applyRoomUpdate(
  roomId: string,
  updated: Partial<SimpleRoom>,
  rooms: SimpleRoom[],
  lessons: SimpleLesson[]
): {
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  newId: string;
} {
  const oldId = roomId;
  const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

  const nextRooms = rooms.map((r) =>
    r.id === oldId ? { ...r, ...updated, id: newId } : r
  );

  let nextLessons = lessons;
  if (newId !== oldId) {
    nextLessons = lessons.map((l) =>
      l.roomId === oldId ? { ...l, roomId: newId } : l
    );
  }

  return { rooms: nextRooms, lessons: nextLessons, newId };
}

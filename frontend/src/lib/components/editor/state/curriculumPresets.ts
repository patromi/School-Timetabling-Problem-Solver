// frontend/src/lib/components/editor/state/curriculumPresets.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
} from "../types";

export const DEFAULT_LO_CURRICULUM_SPECS: {
  subject: string;
  count: number;
  isBlock: boolean;
  roomType: SimpleRoom["type"];
}[] = [
  { subject: "Język polski", count: 4, isBlock: false, roomType: "general" },
  { subject: "Język angielski", count: 3, isBlock: false, roomType: "general" },
  { subject: "Matematyka", count: 4, isBlock: false, roomType: "general" },
  { subject: "Historia", count: 2, isBlock: false, roomType: "general" },
  { subject: "Biologia", count: 2, isBlock: false, roomType: "lab" },
  { subject: "Chemia", count: 2, isBlock: false, roomType: "lab" },
  { subject: "Fizyka", count: 2, isBlock: false, roomType: "general" },
  { subject: "Geografia", count: 2, isBlock: false, roomType: "general" },
  { subject: "Informatyka", count: 2, isBlock: true, roomType: "it" },
  { subject: "Wychowanie Fizyczne", count: 3, isBlock: false, roomType: "gym" },
];

export function generateDefaultCurriculumForClasses(
  classes: SimpleClass[],
  teachers: SimpleTeacher[],
  rooms: SimpleRoom[]
): SimpleLesson[] {
  const generatedLessons: SimpleLesson[] = [];

  classes.forEach((c) => {
    DEFAULT_LO_CURRICULUM_SPECS.forEach((spec) => {
      let teacher = teachers.find((t) => t.subject === spec.subject);
      if (!teacher) {
        teacher = teachers[0];
      }
      let room = rooms.find((r) => r.type === spec.roomType);
      if (!room) {
        room = rooms[0];
      }
      const cleanSub = spec.subject.replace(/\s+/g, "");
      const lid = `${c.id}_${cleanSub}`;
      generatedLessons.push({
        id: lid,
        subject: spec.subject,
        classId: c.id,
        teacherId: teacher ? teacher.id : "",
        roomId: room ? room.id : "",
        duration: spec.isBlock ? 2 : 1,
        weeklyCount: spec.count,
      });
    });
  });

  return generatedLessons;
}

export function duplicateClassWithLessons(
  sourceClassId: string,
  classes: SimpleClass[],
  lessons: SimpleLesson[]
): {
  newClasses: SimpleClass[];
  newLessons: SimpleLesson[];
  newClassId: string;
} | null {
  const source = classes.find((c) => c.id === sourceClassId);
  if (!source) return null;

  let suffixIndex = 2;
  let newId = `${source.id}_kopia`;
  let newName = `${source.name} (kopia)`;
  while (classes.some((c) => c.id === newId)) {
    newId = `${source.id}_kopia${suffixIndex}`;
    newName = `${source.name} (kopia ${suffixIndex})`;
    suffixIndex++;
  }

  const newClass: SimpleClass = {
    ...source,
    id: newId,
    name: newName,
    grade: source.grade,
    unavailableTimes: source.unavailableTimes ? [...source.unavailableTimes] : [],
  };

  const sourceLessons = lessons.filter((l) => l.classId === sourceClassId);
  const duplicatedLessons: SimpleLesson[] = sourceLessons.map((l) => {
    const cleanSub = l.subject.replace(/\s+/g, "");
    const newLessonId = `${newId}_${cleanSub}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
    return {
      ...l,
      id: newLessonId,
      classId: newId,
      preferredTimes: l.preferredTimes ? [...l.preferredTimes] : undefined,
    };
  });

  return {
    newClasses: [...classes, newClass],
    newLessons: [...lessons, ...duplicatedLessons],
    newClassId: newId,
  };
}

export function duplicateCurriculumBetweenClasses(
  sourceClassId: string,
  targetClassIds: string[],
  lessons: SimpleLesson[],
  getConsistentTeacher: (classId: string, subject: string) => string | null
): SimpleLesson[] {
  const sourceLessons = lessons.filter((l) => l.classId === sourceClassId);
  if (sourceLessons.length === 0 || targetClassIds.length === 0) return lessons;

  const targetSet = new Set(targetClassIds);
  const keptLessons = lessons.filter((l) => !targetSet.has(l.classId));
  const newLessons: SimpleLesson[] = [];

  targetClassIds.forEach((targetId) => {
    sourceLessons.forEach((sl) => {
      const cleanSub = sl.subject.replace(/\s+/g, "");
      const newLessonId = `${targetId}_${cleanSub}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
      const assignedTeacherId = getConsistentTeacher(targetId, sl.subject) || sl.teacherId;

      newLessons.push({
        ...sl,
        id: newLessonId,
        classId: targetId,
        teacherId: assignedTeacherId,
        preferredTimes: sl.preferredTimes ? [...sl.preferredTimes] : undefined,
      });
    });
  });

  return [...keptLessons, ...newLessons];
}

export function addSubjectToGradeClasses(options: {
  grade: number;
  subject: string;
  weeklyCount: number;
  isBlock: boolean;
  classes: SimpleClass[];
  teachers: SimpleTeacher[];
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  getClassGrade: (cls: SimpleClass) => number;
  getConsistentTeacher: (classId: string, subject: string) => string | null;
}): SimpleLesson[] {
  const {
    grade,
    subject,
    weeklyCount,
    isBlock,
    classes,
    teachers,
    rooms,
    lessons,
    getClassGrade,
    getConsistentTeacher,
  } = options;

  const targetClasses = classes.filter((c) => getClassGrade(c) === grade);
  if (targetClasses.length === 0) return lessons;

  const count = Math.max(1, weeklyCount);
  const updated = [...lessons];

  targetClasses.forEach((cls) => {
    const teacherId = getConsistentTeacher(cls.id, subject) || (teachers[0] ? teachers[0].id : "");
    let defaultRoomId = rooms[0] ? rooms[0].id : "";
    const lower = subject.toLowerCase();
    if (lower.includes("wychowanie") || lower.includes("wf")) {
      const gym = rooms.find((r) => r.type === "gym");
      if (gym) defaultRoomId = gym.id;
    } else if (lower.includes("informat") || lower.includes("it")) {
      const it = rooms.find((r) => r.type === "it");
      if (it) defaultRoomId = it.id;
    } else if (lower.includes("chem") || lower.includes("biol")) {
      const lab = rooms.find((r) => r.type === "lab");
      if (lab) defaultRoomId = lab.id;
    }

    const existingIndex = updated.findIndex((l) => l.classId === cls.id && l.subject === subject);
    if (existingIndex >= 0) {
      updated[existingIndex] = {
        ...updated[existingIndex],
        weeklyCount: count,
        duration: isBlock ? 2 : 1,
      };
    } else {
      const cleanId = `${cls.id}_${subject.replace(/\s+/g, "")}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
      updated.push({
        id: cleanId,
        subject,
        classId: cls.id,
        teacherId,
        roomId: defaultRoomId,
        duration: isBlock ? 2 : 1,
        weeklyCount: count,
      });
    }
  });

  return updated;
}

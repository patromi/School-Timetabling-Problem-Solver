// frontend/src/lib/components/editor/state/editorMutationsBridge.ts
import type { EditorState } from "../editorState.svelte";
import type { SimpleLesson } from "../types";
import {
  addClassToState,
  removeClassFromState,
  addTeacherToState,
  removeTeacherFromState,
  addRoomToState,
  removeRoomFromState,
  addLessonToState,
  removeLessonFromState,
} from "./entityMutations";
import {
  duplicateClassWithLessons,
  duplicateCurriculumBetweenClasses,
  addSubjectToGradeClasses,
} from "./curriculumPresets";
import {
  buildAvailabilityTarget,
  updateResourceAvailability,
  buildLessonPrefTarget,
  updateLessonPreferences,
} from "./modalHandlers";
import {
  createOrderRule,
  removeOrderRuleFromList,
} from "./orderRulesActions";

export function executeAddClass(state: EditorState) {
  const res = addClassToState(state.classes, state.newClassName, state.newClassGrade);
  if (res.error) return alert(res.error);
  state.classes = res.classes;
  state.newClassName = "";
}

export function executeRemoveClass(state: EditorState, id: string) {
  const res = removeClassFromState(state.classes, state.lessons, state.orderRules, id);
  state.classes = res.classes;
  state.lessons = res.lessons;
  state.orderRules = res.orderRules;
}

export function executeDuplicateClass(state: EditorState, sourceClassId: string) {
  const res = duplicateClassWithLessons(sourceClassId, state.classes, state.lessons);
  if (!res) return;
  state.classes = res.newClasses;
  state.lessons = res.newLessons;
  state.selectedClassViewId = res.newClassId;
}

export function executeDuplicateCurriculum(state: EditorState, sourceClassId: string, targetClassIds: string[]) {
  state.lessons = duplicateCurriculumBetweenClasses(
    sourceClassId,
    targetClassIds,
    state.lessons,
    (cId, sub) => state.getConsistentTeacherForClassSubject(cId, sub)
  );
}

export function executeAddSubjectToGrade(
  state: EditorState,
  grade: number,
  subject: string,
  weeklyCount: number,
  isBlock: boolean = false
) {
  state.lessons = addSubjectToGradeClasses({
    grade,
    subject,
    weeklyCount,
    isBlock,
    classes: state.classes,
    teachers: state.teachers,
    rooms: state.rooms,
    lessons: state.lessons,
    getClassGrade: (c) => state.getClassGrade(c),
    getConsistentTeacher: (cId, sub) => state.getConsistentTeacherForClassSubject(cId, sub),
  });
}

export function executeAddTeacher(state: EditorState) {
  const res = addTeacherToState(state.teachers, state.newTeacherName, state.newTeacherSubject, state.newTeacherWorkload);
  if (res.error) return alert(res.error);
  state.teachers = res.teachers;
  state.newTeacherName = "";
}

export function executeRemoveTeacher(state: EditorState, id: string) {
  state.teachers = removeTeacherFromState(state.teachers, id);
}

export function executeAddRoom(state: EditorState) {
  const res = addRoomToState(state.rooms, state.newRoomName, state.newRoomType);
  if (res.error) return alert(res.error);
  state.rooms = res.rooms;
  state.newRoomName = "";
}

export function executeRemoveRoom(state: EditorState, id: string) {
  state.rooms = removeRoomFromState(state.rooms, id);
}

export function executeAddCustomLesson(state: EditorState) {
  const res = addLessonToState(state.lessons, state.rooms, {
    subject: state.lessonSubject,
    classId: state.lessonClassId,
    teacherId: state.lessonTeacherId,
    roomId: state.lessonRoomId,
    duration: state.lessonIsBlock ? 2 : 1,
    weeklyCount: state.lessonCount,
  });
  if (res.error) return alert(res.error);
  state.lessons = res.lessons;
}

export function executeRemoveLesson(state: EditorState, id: string) {
  const res = removeLessonFromState(state.lessons, state.orderRules, id);
  state.lessons = res.lessons;
  state.orderRules = res.orderRules;
}

export function executeOpenAvailability(
  state: EditorState,
  id: string,
  name: string,
  type: "teacher" | "room" | "class"
) {
  state.activeAvailabilityTarget = buildAvailabilityTarget({
    id,
    name,
    type,
    teachers: state.teachers,
    classes: state.classes,
    rooms: state.rooms,
    lessons: state.lessons,
    getTeacherAssignedHours: (tId) => state.getTeacherAssignedHours(tId),
    getTeacherClasses: (tId) => state.getTeacherClasses(tId),
    getClassLessons: (cId) => state.getClassLessons(cId),
    getClassWeeklyHours: (cId) => state.getClassWeeklyHours(cId),
  });
}

export function executeSaveAvailability(
  state: EditorState,
  unavailableTimes: string[],
  isSoft: boolean,
  weight: number,
  extraParams?: {
    maxDaysPerWeek?: number | null;
    minDaysPerWeek?: number | null;
    maxDailyLessons?: number | null;
    maxGapsPerWeek?: number | null;
    minDailyLessons?: number | null;
  }
) {
  if (!state.activeAvailabilityTarget) return;
  const res = updateResourceAvailability({
    target: state.activeAvailabilityTarget,
    teachers: state.teachers,
    classes: state.classes,
    rooms: state.rooms,
    unavailableTimes,
    isSoft,
    weight,
    extraParams,
  });
  state.teachers = res.teachers;
  state.classes = res.classes;
  state.rooms = res.rooms;
  state.activeAvailabilityTarget = null;
}

export function executeOpenLessonPref(state: EditorState, lesson: SimpleLesson) {
  state.activeLessonPrefTarget = buildLessonPrefTarget(lesson, state.classes);
}

export function executeSaveLessonPref(
  state: EditorState,
  pref: "any" | "morning" | "afternoon" | "custom",
  times: string[],
  isSoft: boolean,
  weight: number
) {
  if (!state.activeLessonPrefTarget) return;
  state.lessons = updateLessonPreferences(
    state.activeLessonPrefTarget.lessonId,
    state.lessons,
    pref,
    times,
    isSoft,
    weight
  );
  state.activeLessonPrefTarget = null;
}

export function executeAddOrderRule(state: EditorState) {
  const res = createOrderRule(
    state.lessons,
    state.newOrderFirstLesson,
    state.newOrderSecondLesson,
    state.newOrderSeparation,
    state.newOrderIsSoft,
    state.newOrderWeight
  );
  if (res.error) return alert(res.error);
  if (res.rule) state.orderRules = [...state.orderRules, res.rule];
  state.newOrderFirstLesson = "";
  state.newOrderSecondLesson = "";
}

export function executeRemoveOrderRule(state: EditorState, ruleId: string) {
  state.orderRules = removeOrderRuleFromList(state.orderRules, ruleId);
}

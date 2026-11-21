// frontend/src/lib/components/editor/state/instanceSync.ts
import type { InstanceDetail } from "../../../types";
import type { EditorState } from "../editorState.svelte";
import { parseInstanceDetail } from "../xhsttLoader";
import { assembleXhsttPayload } from "../xhsttAssembler";

export function loadInstanceIntoEditor(state: EditorState, inst: InstanceDetail) {
  const loaded = parseInstanceDetail(inst);
  state.schoolId = loaded.schoolId;
  state.schoolName = loaded.schoolName;
  state.selectedDays = loaded.selectedDays;
  state.periodsPerDay = loaded.periodsPerDay;
  if (loaded.classes.length > 0) state.classes = loaded.classes;
  if (loaded.teachers.length > 0) state.teachers = loaded.teachers;
  if (loaded.rooms.length > 0) state.rooms = loaded.rooms;

  const cd = loaded.constraintsData;
  state.ruleContinuousBlocks = cd.ruleContinuousBlocks;
  state.ruleAssignAllTimes = cd.ruleAssignAllTimes;
  state.ruleAvoidClashes = cd.ruleAvoidClashes;
  state.ruleTeacherClusterDays = cd.ruleTeacherClusterDays;
  state.teacherClusterWeight = cd.teacherClusterWeight;
  state.ruleMaxLessonsDaily = cd.ruleMaxLessonsDaily;
  state.studentMinDailyLimit = cd.studentMinDailyLimit;
  state.studentMaxDailyLimit = cd.studentMaxDailyLimit;
  state.studentDailyWeight = cd.studentDailyWeight;
  state.ruleTeacherMaxDaily = cd.ruleTeacherMaxDaily;
  state.teacherMaxDailyLimit = cd.teacherMaxDailyLimit;
  state.teacherDailyWeight = cd.teacherDailyWeight;
  state.ruleTeacherMaxGaps = cd.ruleTeacherMaxGaps;
  state.teacherMaxGapsLimit = cd.teacherMaxGapsLimit;
  state.teacherGapsWeight = cd.teacherGapsWeight;
  state.ruleStudentNoGaps = cd.ruleStudentNoGaps;
  state.ruleStudentNoGapsSoft = cd.ruleStudentNoGapsSoft;
  state.studentGapsWeight = cd.studentGapsWeight;
  state.ruleSpreadSubjects = cd.ruleSpreadSubjects;
  state.ruleSpreadSubjectsSoft = cd.ruleSpreadSubjectsSoft;
  state.spreadSubjectsWeight = cd.spreadSubjectsWeight;
  state.spreadSubjectsMode = cd.spreadSubjectsMode;
  state.ruleMorningCoreSubjects = cd.ruleMorningCoreSubjects;
  state.morningCoreSubjectsSoft = cd.morningCoreSubjectsSoft;
  state.morningCoreSubjectsWeight = cd.morningCoreSubjectsWeight;
  state.ruleAfternoonFlexSubjects = cd.ruleAfternoonFlexSubjects;
  state.afternoonFlexSubjectsSoft = cd.afternoonFlexSubjectsSoft;
  state.afternoonFlexSubjectsWeight = cd.afternoonFlexSubjectsWeight;
  state.ruleOrderLessons = cd.ruleOrderLessons;
  state.newOrderIsSoft = cd.orderLessonsIsSoft;
  state.newOrderWeight = cd.orderLessonsWeight;

  state.linkedGroups = loaded.linkedGroups;
  state.orderRules = loaded.orderRules;
  state.externalConstraintsNotice = loaded.externalConstraintsNotice;

  if (loaded.lessons && loaded.lessons.length > 0) {
    state.lessons = loaded.lessons;
  } else if (state.classes.length > 0 && state.teachers.length > 0) {
    state.generateDefaultCurriculum();
  }
}

export function buildPayloadFromEditor(state: EditorState): InstanceDetail {
  return assembleXhsttPayload({
    schoolId: state.schoolId,
    schoolName: state.schoolName,
    selectedDays: state.selectedDays,
    periodsPerDay: state.periodsPerDay,
    teachers: state.teachers,
    classes: state.classes,
    rooms: state.rooms,
    lessons: state.lessons,
    ruleContinuousBlocks: state.ruleContinuousBlocks,
    ruleAssignAllTimes: state.ruleAssignAllTimes,
    ruleAvoidClashes: state.ruleAvoidClashes,
    ruleStudentNoGaps: state.ruleStudentNoGaps,
    ruleStudentNoGapsSoft: state.ruleStudentNoGapsSoft,
    studentGapsWeight: state.studentGapsWeight,
    ruleTeacherMaxGaps: state.ruleTeacherMaxGaps,
    teacherMaxGapsLimit: state.teacherMaxGapsLimit,
    teacherGapsWeight: state.teacherGapsWeight,
    ruleMaxLessonsDaily: state.ruleMaxLessonsDaily,
    studentMinDailyLimit: state.studentMinDailyLimit,
    studentMaxDailyLimit: state.studentMaxDailyLimit,
    studentDailyWeight: state.studentDailyWeight,
    ruleTeacherMaxDaily: state.ruleTeacherMaxDaily,
    teacherMaxDailyLimit: state.teacherMaxDailyLimit,
    teacherDailyWeight: state.teacherDailyWeight,
    ruleTeacherClusterDays: state.ruleTeacherClusterDays,
    teacherClusterWeight: state.teacherClusterWeight,
    ruleSpreadSubjects: state.ruleSpreadSubjects,
    ruleSpreadSubjectsSoft: state.ruleSpreadSubjectsSoft,
    spreadSubjectsWeight: state.spreadSubjectsWeight,
    spreadSubjectsMode: state.spreadSubjectsMode,
    ruleMorningCoreSubjects: state.ruleMorningCoreSubjects,
    morningCoreSubjectsSoft: state.morningCoreSubjectsSoft,
    morningCoreSubjectsWeight: state.morningCoreSubjectsWeight,
    ruleAfternoonFlexSubjects: state.ruleAfternoonFlexSubjects,
    afternoonFlexSubjectsSoft: state.afternoonFlexSubjectsSoft,
    afternoonFlexSubjectsWeight: state.afternoonFlexSubjectsWeight,
    linkedGroups: state.linkedGroups,
    ruleOrderLessons: state.ruleOrderLessons,
    orderRules: state.orderRules,
    orderLessonsIsSoft: state.newOrderIsSoft,
    orderLessonsWeight: state.newOrderWeight,
  });
}

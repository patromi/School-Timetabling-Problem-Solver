// frontend/src/lib/components/editor/xhsttAssembler.ts
import type { InstanceDetail } from "../../types";
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  LinkedGroupItem,
  OrderRuleItem,
} from "./types";
import { buildBaseStructure } from "./assembler/baseAssembler.ts";
import { buildStandardConstraints } from "./assembler/standardConstraints.ts";
import { buildPedagogicalConstraints } from "./assembler/pedagogicalConstraints.ts";

export interface XhsttAssemblerOptions {
  schoolId: string;
  schoolName: string;
  selectedDays: string[];
  periodsPerDay: number;
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  // Reguły
  ruleContinuousBlocks?: boolean;
  ruleAssignAllTimes?: boolean;
  ruleAvoidClashes?: boolean;
  ruleStudentNoGaps?: boolean;
  ruleStudentNoGapsSoft?: boolean;
  studentGapsWeight?: number;
  ruleTeacherMaxGaps?: boolean;
  teacherMaxGapsLimit?: number;
  teacherGapsWeight?: number;
  ruleMaxLessonsDaily?: boolean;
  studentMinDailyLimit?: number;
  studentMaxDailyLimit?: number;
  studentDailyWeight?: number;
  ruleTeacherMaxDaily?: boolean;
  teacherMaxDailyLimit?: number;
  teacherDailyWeight?: number;
  ruleTeacherClusterDays?: boolean;
  teacherClusterWeight?: number;
  ruleSpreadSubjects?: boolean;
  ruleSpreadSubjectsSoft?: boolean;
  spreadSubjectsWeight?: number;
  spreadSubjectsMode?: "single" | "blocks";
  ruleMorningCoreSubjects?: boolean;
  morningCoreSubjectsSoft?: boolean;
  morningCoreSubjectsWeight?: number;
  ruleAfternoonFlexSubjects?: boolean;
  afternoonFlexSubjectsSoft?: boolean;
  afternoonFlexSubjectsWeight?: number;
  linkedGroups?: LinkedGroupItem[];
  ruleOrderLessons?: boolean;
  orderRules?: OrderRuleItem[];
  orderLessonsIsSoft?: boolean;
  orderLessonsWeight?: number;
}

export function assembleXhsttPayload(options: XhsttAssemblerOptions): InstanceDetail {
  const {
    schoolId,
    schoolName,
    selectedDays,
    periodsPerDay,
    teachers,
    classes,
    rooms,
    lessons,
    ruleContinuousBlocks = false,
  } = options;

  // 1. Zbuduj bazową strukturę encji (dni, godziny, typy zasobów, zasoby, lekcje, grupy lekcyjne)
  const base = buildBaseStructure({
    selectedDays,
    periodsPerDay,
    teachers,
    classes,
    rooms,
    lessons,
    ruleContinuousBlocks,
  });

  // 2. Ograniczenia standardowe (czas, brak kolizji, okienka, limity dzienne, dostępności)
  const standardConstraints = buildStandardConstraints({
    events: base.events,
    resources: base.resources,
    time_groups: base.time_groups,
    teachers,
    classes,
    rooms,
    lessons,
    selectedDays,
    ruleAssignAllTimes: options.ruleAssignAllTimes,
    ruleAvoidClashes: options.ruleAvoidClashes,
    ruleStudentNoGaps: options.ruleStudentNoGaps,
    ruleStudentNoGapsSoft: options.ruleStudentNoGapsSoft,
    studentGapsWeight: options.studentGapsWeight,
    ruleTeacherMaxGaps: options.ruleTeacherMaxGaps,
    teacherMaxGapsLimit: options.teacherMaxGapsLimit,
    teacherGapsWeight: options.teacherGapsWeight,
    ruleMaxLessonsDaily: options.ruleMaxLessonsDaily,
    studentMinDailyLimit: options.studentMinDailyLimit,
    studentMaxDailyLimit: options.studentMaxDailyLimit,
    studentDailyWeight: options.studentDailyWeight,
    ruleTeacherMaxDaily: options.ruleTeacherMaxDaily,
    teacherMaxDailyLimit: options.teacherMaxDailyLimit,
    teacherDailyWeight: options.teacherDailyWeight,
    ruleTeacherClusterDays: options.ruleTeacherClusterDays,
    teacherClusterWeight: options.teacherClusterWeight,
  });

  // 3. Ograniczenia dydaktyczne i preferencje (rozkład, rano/popołudnie, powiązania, kolejność)
  const pedagogicalConstraints = buildPedagogicalConstraints({
    events: base.events,
    event_groups: base.event_groups,
    time_groups: base.time_groups,
    lessons,
    classes,
    selectedDays,
    periodsPerDay,
    ruleSpreadSubjects: options.ruleSpreadSubjects,
    ruleSpreadSubjectsSoft: options.ruleSpreadSubjectsSoft,
    spreadSubjectsWeight: options.spreadSubjectsWeight,
    spreadSubjectsMode: options.spreadSubjectsMode,
    ruleMorningCoreSubjects: options.ruleMorningCoreSubjects,
    morningCoreSubjectsSoft: options.morningCoreSubjectsSoft,
    morningCoreSubjectsWeight: options.morningCoreSubjectsWeight,
    ruleAfternoonFlexSubjects: options.ruleAfternoonFlexSubjects,
    afternoonFlexSubjectsSoft: options.afternoonFlexSubjectsSoft,
    afternoonFlexSubjectsWeight: options.afternoonFlexSubjectsWeight,
    linkedGroups: options.linkedGroups,
    ruleOrderLessons: options.ruleOrderLessons,
    orderRules: options.orderRules,
    orderLessonsIsSoft: options.orderLessonsIsSoft,
    orderLessonsWeight: options.orderLessonsWeight,
  });

  return {
    id: schoolId.trim() || `Szkola_${Date.now()}`,
    name: schoolName.trim(),
    time_groups: base.time_groups,
    times: base.times,
    resource_types: base.resource_types,
    resource_groups: [],
    resources: base.resources,
    event_groups: base.event_groups,
    events: base.events,
    constraints: [...standardConstraints, ...pedagogicalConstraints],
  };
}

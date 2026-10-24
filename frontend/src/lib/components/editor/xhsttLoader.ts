// frontend/src/lib/components/editor/xhsttLoader.ts
import type { InstanceDetail } from "../../types";
import type { SimpleClass, SimpleTeacher, SimpleRoom } from "./types";

export interface LoadedInstanceData {
  schoolId: string;
  schoolName: string;
  selectedDays: string[];
  periodsPerDay: number;
  classes: SimpleClass[];
  teachers: SimpleTeacher[];
  rooms: SimpleRoom[];
  constraintsData: {
    ruleTeacherClusterDays: boolean;
    teacherClusterWeight: number;
    ruleMaxLessonsDaily: boolean;
    studentMinDailyLimit: number;
    studentMaxDailyLimit: number;
    studentDailyWeight: number;
    ruleTeacherMaxGaps: boolean;
    teacherMaxGapsLimit: number;
    teacherGapsWeight: number;
    ruleStudentNoGaps: boolean;
    ruleStudentNoGapsSoft: boolean;
    studentGapsWeight: number;
    ruleSpreadSubjects: boolean;
    ruleSpreadSubjectsSoft: boolean;
    spreadSubjectsWeight: number;
    spreadSubjectsMode: "single" | "blocks";
    ruleMorningCoreSubjects: boolean;
    morningCoreSubjectsSoft: boolean;
    morningCoreSubjectsWeight: number;
    ruleAfternoonFlexSubjects: boolean;
    afternoonFlexSubjectsSoft: boolean;
    afternoonFlexSubjectsWeight: number;
    ruleOrderLessons: boolean;
  };
}

export function parseInstanceDetail(inst: InstanceDetail): LoadedInstanceData {
  const schoolId = `${inst.id}_kopia`;
  const schoolName = `${inst.name} (edycja)`;

  // Dni
  const days = inst.time_groups.filter((g) => g.kind.toLowerCase() === "day");
  let selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
  let periodsPerDay = 8;
  if (days.length > 0) {
    selectedDays = days.map((d) => d.name || d.id);
    periodsPerDay = Math.max(5, Math.round(inst.times.length / days.length));
  }

  // Extract constraints
  const unavailMap: Record<string, { times: string[]; isSoft: boolean; weight: number }> = {};
  const clusterDaysMap: Record<string, { minDays: number | null; maxDays: number | null }> = {};
  const busyLimitMap: Record<string, { minDaily: number | null; maxDaily: number | null }> = {};
  const gapsLimitMap: Record<string, number | null> = {};

  const constraintsData = {
    ruleTeacherClusterDays: false,
    teacherClusterWeight: 6,
    ruleMaxLessonsDaily: false,
    studentMinDailyLimit: 4,
    studentMaxDailyLimit: 8,
    studentDailyWeight: 4,
    ruleTeacherMaxGaps: false,
    teacherMaxGapsLimit: 2,
    teacherGapsWeight: 5,
    ruleStudentNoGaps: false,
    ruleStudentNoGapsSoft: false,
    studentGapsWeight: 10,
    ruleSpreadSubjects: false,
    ruleSpreadSubjectsSoft: true,
    spreadSubjectsWeight: 5,
    spreadSubjectsMode: "single" as "single" | "blocks",
    ruleMorningCoreSubjects: false,
    morningCoreSubjectsSoft: true,
    morningCoreSubjectsWeight: 4,
    ruleAfternoonFlexSubjects: false,
    afternoonFlexSubjectsSoft: true,
    afternoonFlexSubjectsWeight: 3,
    ruleOrderLessons: false,
  };

  (inst.constraints || []).forEach((c) => {
    if (c.type === "AvoidUnavailableTimesConstraint") {
      const times = Array.isArray(c.params.Times)
        ? c.params.Times.map((item: any) => (typeof item === "string" ? item : item?.reference)).filter(Boolean)
        : [];
      (c.applies_to?.resources || []).forEach((rId) => {
        unavailMap[rId] = {
          times: [...(unavailMap[rId]?.times || []), ...times],
          isSoft: !c.required,
          weight: c.weight || 10,
        };
      });
    } else if (c.type === "ClusterBusyTimesConstraint") {
      const maxDays = c.params.Maximum !== undefined ? Number(c.params.Maximum) : null;
      const minDays = c.params.Minimum !== undefined ? Number(c.params.Minimum) : null;
      (c.applies_to?.resources || []).forEach((rId) => {
        clusterDaysMap[rId] = { minDays, maxDays };
      });
      if (c.weight) constraintsData.teacherClusterWeight = c.weight;
      constraintsData.ruleTeacherClusterDays = true;
    } else if (c.type === "LimitBusyTimesConstraint") {
      const minVal = c.params.Minimum !== undefined ? Number(c.params.Minimum) : null;
      const maxVal = c.params.Maximum !== undefined ? Number(c.params.Maximum) : null;
      (c.applies_to?.resources || []).forEach((rId) => {
        busyLimitMap[rId] = { minDaily: minVal, maxDaily: maxVal };
      });
      if (minVal !== null && minVal > 0) constraintsData.studentMinDailyLimit = minVal;
      if (maxVal !== null && maxVal > 0) constraintsData.studentMaxDailyLimit = maxVal;
      if (c.weight) constraintsData.studentDailyWeight = c.weight;
      constraintsData.ruleMaxLessonsDaily = true;
    } else if (c.type === "LimitIdleTimesConstraint") {
      const maxGaps = c.params.Maximum !== undefined ? Number(c.params.Maximum) : 0;
      (c.applies_to?.resources || []).forEach((rId) => {
        gapsLimitMap[rId] = maxGaps;
      });
      if (maxGaps === 0) {
        constraintsData.ruleStudentNoGaps = true;
        constraintsData.ruleStudentNoGapsSoft = !c.required;
        if (c.weight) constraintsData.studentGapsWeight = c.weight;
      } else {
        constraintsData.ruleTeacherMaxGaps = true;
        constraintsData.teacherMaxGapsLimit = maxGaps;
        if (c.weight) constraintsData.teacherGapsWeight = c.weight;
      }
    } else if (c.type === "SpreadEventsConstraint") {
      constraintsData.ruleSpreadSubjects = true;
      constraintsData.ruleSpreadSubjectsSoft = !c.required;
      if (c.weight) constraintsData.spreadSubjectsWeight = c.weight;
      const tgList = c.params?.TimeGroups as any[];
      if (Array.isArray(tgList) && tgList.some((tg) => tg?.Maximum === 2 || tg?.Maximum === "2")) {
        constraintsData.spreadSubjectsMode = "blocks";
      } else {
        constraintsData.spreadSubjectsMode = "single";
      }
    } else if (c.type === "PreferTimesConstraint") {
      if (c.id === "Regula_PrzedmiotyScisleRano") {
        constraintsData.ruleMorningCoreSubjects = true;
        constraintsData.morningCoreSubjectsSoft = !c.required;
        if (c.weight) constraintsData.morningCoreSubjectsWeight = c.weight;
      } else if (c.id === "Regula_PrzedmiotyPopoludniowe") {
        constraintsData.ruleAfternoonFlexSubjects = true;
        constraintsData.afternoonFlexSubjectsSoft = !c.required;
        if (c.weight) constraintsData.afternoonFlexSubjectsWeight = c.weight;
      }
    } else if (c.type === "OrderEventsConstraint") {
      constraintsData.ruleOrderLessons = true;
    }
  });

  // Klasy
  const classes: SimpleClass[] = inst.resources
    .filter((r) => r.resource_type_ref.toLowerCase().includes("class"))
    .map((r) => {
      const match = r.name.match(/^(\d+)/) || r.id.match(/^(\d+)/);
      const grade = match ? parseInt(match[1], 10) : 1;
      return {
        id: r.id,
        name: r.name || r.id,
        grade,
        unavailableTimes: unavailMap[r.id]?.times || [],
        unavailableSoft: unavailMap[r.id]?.isSoft || false,
        unavailableWeight: unavailMap[r.id]?.weight || 10,
        minDailyLessons: busyLimitMap[r.id]?.minDaily ?? null,
        maxDailyLessons: busyLimitMap[r.id]?.maxDaily ?? null,
      };
    });

  // Nauczyciele
  const teachers: SimpleTeacher[] = inst.resources
    .filter((r) => r.resource_type_ref.toLowerCase().includes("teach"))
    .map((r) => ({
      id: r.id,
      name: r.name || r.id,
      subject: "Przedmiot ogólny",
      workload: 18,
      unavailableTimes: unavailMap[r.id]?.times || [],
      unavailableSoft: unavailMap[r.id]?.isSoft || false,
      unavailableWeight: unavailMap[r.id]?.weight || 10,
      maxDaysPerWeek: clusterDaysMap[r.id]?.maxDays ?? null,
      minDaysPerWeek: clusterDaysMap[r.id]?.minDays ?? null,
      maxDailyLessons: busyLimitMap[r.id]?.maxDaily ?? null,
      maxGapsPerWeek: gapsLimitMap[r.id] ?? null,
    }));

  // Sale
  const rooms: SimpleRoom[] = inst.resources
    .filter((r) => r.resource_type_ref.toLowerCase().includes("room"))
    .map((r) => {
      let type: "general" | "gym" | "lab" | "it" = "general";
      const lower = (r.name || "").toLowerCase();
      if (lower.includes("gym") || lower.includes("wf") || lower.includes("gimn")) type = "gym";
      else if (lower.includes("lab") || lower.includes("chem") || lower.includes("biol")) type = "lab";
      else if (lower.includes("it") || lower.includes("inf") || lower.includes("komp")) type = "it";

      return {
        id: r.id,
        name: r.name || r.id,
        type,
        unavailableTimes: unavailMap[r.id]?.times || [],
        unavailableSoft: unavailMap[r.id]?.isSoft || false,
        unavailableWeight: unavailMap[r.id]?.weight || 10,
      };
    });

  return {
    schoolId,
    schoolName,
    selectedDays,
    periodsPerDay,
    classes,
    teachers,
    rooms,
    constraintsData,
  };
}

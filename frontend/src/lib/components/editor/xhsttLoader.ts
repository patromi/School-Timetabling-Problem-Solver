// frontend/src/lib/components/editor/xhsttLoader.ts
import type { InstanceDetail } from "../../types";
import { CONSTRAINT_FRIENDLY_MAP } from "../../school_helpers";
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  LinkedGroupItem,
  OrderRuleItem,
} from "./types";

export interface LoadedInstanceData {
  schoolId: string;
  schoolName: string;
  selectedDays: string[];
  periodsPerDay: number;
  classes: SimpleClass[];
  teachers: SimpleTeacher[];
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  linkedGroups: LinkedGroupItem[];
  orderRules: OrderRuleItem[];
  externalConstraintsNotice: string[];
  constraintsData: {
    ruleContinuousBlocks: boolean;
    ruleAssignAllTimes: boolean;
    ruleAvoidClashes: boolean;
    ruleTeacherClusterDays: boolean;
    teacherClusterWeight: number;
    ruleMaxLessonsDaily: boolean;
    studentMinDailyLimit: number;
    studentMaxDailyLimit: number;
    studentDailyWeight: number;
    ruleTeacherMaxDaily: boolean;
    teacherMaxDailyLimit: number;
    teacherDailyWeight: number;
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
    orderLessonsIsSoft: boolean;
    orderLessonsWeight: number;
  };
}

export function parseInstanceDetail(inst: InstanceDetail): LoadedInstanceData {
  const schoolId = inst.id;
  const schoolName = inst.name || inst.id;

  // Dni
  const days = inst.time_groups.filter((g) => g.kind.toLowerCase() === "day");
  let selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
  let periodsPerDay = 8;
  if (days.length > 0) {
    selectedDays = days.map((d) => d.name || d.id);
    periodsPerDay = Math.max(5, Math.round(inst.times.length / days.length));
  }

  // Mapy ograniczeń zasobowych
  const unavailMap: Record<string, { times: string[]; isSoft: boolean; weight: number }> = {};
  const clusterDaysMap: Record<string, { minDays: number | null; maxDays: number | null }> = {};
  const busyLimitMap: Record<string, { minDaily: number | null; maxDaily: number | null }> = {};
  const gapsLimitMap: Record<string, number | null> = {};

  const constraintsData = {
    ruleContinuousBlocks: false,
    ruleAssignAllTimes: true,
    ruleAvoidClashes: true,
    ruleTeacherClusterDays: false,
    teacherClusterWeight: 6,
    ruleMaxLessonsDaily: false,
    studentMinDailyLimit: 4,
    studentMaxDailyLimit: 8,
    studentDailyWeight: 4,
    ruleTeacherMaxDaily: false,
    teacherMaxDailyLimit: 6,
    teacherDailyWeight: 4,
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
    orderLessonsIsSoft: true,
    orderLessonsWeight: 8,
  };

  // Najpierw klasy, nauczyciele i sale (aby znać ich ID do rekonstrukcji lekcji)
  const classResources = inst.resources.filter((r) =>
    r.resource_type_ref.toLowerCase().includes("class")
  );
  const teacherResources = inst.resources.filter((r) =>
    r.resource_type_ref.toLowerCase().includes("teach")
  );
  const roomResources = inst.resources.filter((r) =>
    r.resource_type_ref.toLowerCase().includes("room")
  );

  const classIdSet = new Set(classResources.map((r) => r.id));
  const teacherIdSet = new Set(teacherResources.map((r) => r.id));
  const roomIdSet = new Set(roomResources.map((r) => r.id));

  // Rekonstrukcja lekcji (SimpleLesson[]) i mapy eventId -> lessonId
  const lessons: SimpleLesson[] = [];
  const teacherSubjectsMap = new Map<string, Set<string>>();
  const eventIdToLessonId = new Map<string, string>();
  let hasAnyDoubleBlock = false;

  if (Array.isArray(inst.events) && inst.events.length > 0) {
    const groups = new Map<
      string,
      {
        classId: string;
        subject: string;
        teacherId: string;
        roomId: string;
        duration: number;
        totalCount: number;
        eventIds: string[];
      }
    >();

    inst.events.forEach((ev) => {
      let classId = "";
      let teacherId = "";
      let roomId = "";

      (ev.resources || []).forEach((r) => {
        const role = (r.role || "").toLowerCase();
        const typeRef = (r.resource_type_ref || "").toLowerCase();
        const ref = r.resource_ref;

        if (role.includes("teach") || typeRef.includes("teach")) {
          teacherId = ref || "__ANY__";
        } else if (role.includes("class") || typeRef.includes("class")) {
          if (ref) classId = ref;
        } else if (role.includes("room") || typeRef.includes("room")) {
          if (ref) roomId = ref;
        } else if (ref) {
          if (!classId && classIdSet.has(ref)) classId = ref;
          else if (!teacherId && teacherIdSet.has(ref)) teacherId = ref;
          else if (!roomId && roomIdSet.has(ref)) roomId = ref;
        }
      });

      if (!classId && classResources.length > 0) {
        classId = classResources[0].id;
      }

      const rawName = ev.name || ev.id;
      let cleanSubject = rawName
        .replace(/\s*\([^)]*\)/g, "")
        .replace(/\s*#\d+$/g, "")
        .replace(/_#\d+$/g, "")
        .trim();

      if (!cleanSubject) {
        cleanSubject = ev.course_ref || "Zajęcia edukacyjne";
      }

      const dur = ev.duration || 1;
      if (dur >= 2) hasAnyDoubleBlock = true;
      const key = `${classId}:::${cleanSubject}:::${teacherId}:::${roomId}`;

      if (!groups.has(key)) {
        groups.set(key, {
          classId,
          subject: cleanSubject,
          teacherId,
          roomId,
          duration: dur,
          totalCount: dur,
          eventIds: [ev.id],
        });
      } else {
        const existing = groups.get(key)!;
        existing.totalCount += dur;
        existing.eventIds.push(ev.id);
        if (dur === 2) existing.duration = 2;
      }

      if (teacherId && teacherId !== "__ANY__") {
        if (!teacherSubjectsMap.has(teacherId)) {
          teacherSubjectsMap.set(teacherId, new Set());
        }
        teacherSubjectsMap.get(teacherId)!.add(cleanSubject);
      }
    });

    let lessonIdx = 1;
    groups.forEach((g) => {
      const lessonId = `L_${g.classId}_${g.subject.replace(/[^a-zA-Z0-9_]/g, "_")}_${lessonIdx++}`;
      g.eventIds.forEach((evId) => eventIdToLessonId.set(evId, lessonId));
      lessons.push({
        id: lessonId,
        classId: g.classId,
        subject: g.subject,
        teacherId: g.teacherId,
        roomId: g.roomId,
        duration: g.duration >= 2 ? 2 : 1,
        weeklyCount: g.totalCount,
        timePreference: "any",
      });
    });
  }

  constraintsData.ruleContinuousBlocks = hasAnyDoubleBlock;

  const lessonsById = new Map(lessons.map((l) => [l.id, l]));
  const linkedGroups: LinkedGroupItem[] = [];
  const orderRules: OrderRuleItem[] = [];
  const seenOrderKeys = new Set<string>();
  const externalTypeCounts = new Map<string, number>();

  const markExternal = (cType: string) => {
    externalTypeCounts.set(cType, (externalTypeCounts.get(cType) || 0) + 1);
  };

  let hasAssignTime = false;
  let hasAvoidClashes = false;

  (inst.constraints || []).forEach((c) => {
    if (c.type === "AssignTimeConstraint") {
      hasAssignTime = true;
    } else if (c.type === "AvoidClashesConstraint") {
      hasAvoidClashes = true;
    } else if (c.type === "AvoidUnavailableTimesConstraint") {
      const times = Array.isArray(c.params.Times)
        ? c.params.Times.map((item: any) => (typeof item === "string" ? item : item?.reference)).filter(Boolean)
        : [];
      const directResources = c.applies_to?.resources || [];
      if (directResources.length === 0 && (c.applies_to?.resource_groups?.length || 0) > 0) {
        markExternal(c.type);
      }
      directResources.forEach((rId) => {
        unavailMap[rId] = {
          times: [...(unavailMap[rId]?.times || []), ...times],
          isSoft: !c.required,
          weight: c.weight || 10,
        };
      });
    } else if (c.type === "ClusterBusyTimesConstraint") {
      const maxDays = c.params.Maximum !== undefined ? Number(c.params.Maximum) : null;
      const minDays = c.params.Minimum !== undefined ? Number(c.params.Minimum) : null;
      const directResources = c.applies_to?.resources || [];
      if (directResources.length === 0 && (c.applies_to?.resource_groups?.length || 0) > 0) {
        markExternal(c.type);
      }
      directResources.forEach((rId) => {
        clusterDaysMap[rId] = { minDays, maxDays };
      });
      if (c.weight) constraintsData.teacherClusterWeight = c.weight;
      constraintsData.ruleTeacherClusterDays = true;
    } else if (c.type === "LimitBusyTimesConstraint") {
      const minVal = c.params.Minimum !== undefined ? Number(c.params.Minimum) : null;
      const maxVal = c.params.Maximum !== undefined ? Number(c.params.Maximum) : null;
      const directResources = c.applies_to?.resources || [];
      if (directResources.length === 0 && (c.applies_to?.resource_groups?.length || 0) > 0) {
        markExternal(c.type);
      }
      directResources.forEach((rId) => {
        busyLimitMap[rId] = { minDaily: minVal, maxDaily: maxVal };
      });

      const touchesTeacher = directResources.some((rId) => teacherIdSet.has(rId));
      const touchesClass = directResources.some((rId) => classIdSet.has(rId));
      const isPerEntityOverride =
        c.id.startsWith("LimitLekcji_") || c.id.startsWith("MaxLekcji_");

      if (c.id === "Regula_MaxLekcjiNauczycieli" || (touchesTeacher && !touchesClass)) {
        if (!isPerEntityOverride) {
          if (maxVal !== null && maxVal > 0) constraintsData.teacherMaxDailyLimit = maxVal;
          if (c.weight) constraintsData.teacherDailyWeight = c.weight;
        }
        constraintsData.ruleTeacherMaxDaily = true;
      } else {
        if (!isPerEntityOverride) {
          if (minVal !== null && minVal > 0) constraintsData.studentMinDailyLimit = minVal;
          if (maxVal !== null && maxVal > 0) constraintsData.studentMaxDailyLimit = maxVal;
          if (c.weight) constraintsData.studentDailyWeight = c.weight;
        }
        constraintsData.ruleMaxLessonsDaily = true;
      }
    } else if (c.type === "LimitIdleTimesConstraint") {
      const maxGaps = c.params.Maximum !== undefined ? Number(c.params.Maximum) : 0;
      const directResources = c.applies_to?.resources || [];
      if (directResources.length === 0 && (c.applies_to?.resource_groups?.length || 0) > 0) {
        markExternal(c.type);
      }
      directResources.forEach((rId) => {
        gapsLimitMap[rId] = maxGaps;
      });
      const touchesClass = directResources.some((rId) => classIdSet.has(rId));
      const isPerTeacherOverride = c.id.startsWith("LimitOkienek_");
      if (c.id === "Regula_KlasyBezOkienek" || (maxGaps === 0 && touchesClass)) {
        constraintsData.ruleStudentNoGaps = true;
        constraintsData.ruleStudentNoGapsSoft = !c.required;
        if (c.weight) constraintsData.studentGapsWeight = c.weight;
      } else {
        constraintsData.ruleTeacherMaxGaps = true;
        if (!isPerTeacherOverride) {
          constraintsData.teacherMaxGapsLimit = maxGaps;
          if (c.weight) constraintsData.teacherGapsWeight = c.weight;
        }
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
      } else if (c.id.startsWith("PrefTimes_")) {
        const times = Array.isArray(c.params?.Times)
          ? c.params.Times.map((item: any) => (typeof item === "string" ? item : item?.reference)).filter(Boolean)
          : [];
        const matchedLessons = new Set(
          (c.applies_to?.events || [])
            .map((evId) => eventIdToLessonId.get(evId))
            .filter((id): id is string => Boolean(id))
        );
        matchedLessons.forEach((lId) => {
          const lesson = lessonsById.get(lId);
          if (lesson) {
            lesson.timePreference = "custom";
            lesson.preferredTimes = times;
            lesson.preferenceSoft = !c.required;
            lesson.preferenceWeight = c.weight || 4;
          }
        });
      } else {
        markExternal(c.type);
      }
    } else if (c.type === "LinkEventsConstraint") {
      const egIds = new Set(c.applies_to?.event_groups || []);
      const linkedLessonIds = new Set<string>();
      inst.events.forEach((ev) => {
        if ((ev.group_refs || []).some((gRef) => egIds.has(gRef))) {
          const lId = eventIdToLessonId.get(ev.id);
          if (lId) linkedLessonIds.add(lId);
        }
      });
      if (linkedLessonIds.size >= 2) {
        const cleanName = c.name.replace(/^Lekcje równoległe:\s*/i, "").trim() || c.id;
        linkedGroups.push({
          id: c.id.replace(/^Link_/, "") || c.id,
          name: cleanName,
          lessonIds: Array.from(linkedLessonIds),
          isSoft: !c.required,
          weight: c.weight || 10,
        });
      } else {
        markExternal(c.type);
      }
    } else if (c.type === "OrderEventsConstraint") {
      constraintsData.ruleOrderLessons = true;
      constraintsData.orderLessonsIsSoft = !c.required;
      if (c.weight) constraintsData.orderLessonsWeight = c.weight;

      const pairs = c.applies_to?.event_pairs || [];
      let hadExternalPair = false;
      pairs.forEach((pair, idx) => {
        const firstLessonId = eventIdToLessonId.get(pair.first_event);
        const secondLessonId = eventIdToLessonId.get(pair.second_event);
        if (pair.max_separation !== null && pair.max_separation !== undefined) {
          hadExternalPair = true;
        }
        if (firstLessonId && secondLessonId && firstLessonId !== secondLessonId) {
          const key = `${firstLessonId}:::${secondLessonId}:::${pair.min_separation}`;
          if (!seenOrderKeys.has(key)) {
            seenOrderKeys.add(key);
            orderRules.push({
              id: `order_${c.id}_${idx}`,
              name: c.name || "Kolejność lekcji",
              firstLessonId,
              secondLessonId,
              minSeparation: pair.min_separation || 0,
              isSoft: !c.required,
              weight: c.weight || 8,
            });
          }
        } else {
          hadExternalPair = true;
        }
      });
      if (hadExternalPair || pairs.length === 0) {
        markExternal(c.type);
      }
    } else if (
      c.type === "AssignResourceConstraint" ||
      c.type === "PreferResourcesConstraint" ||
      c.type === "AvoidSplitAssignmentsConstraint"
    ) {
      const isUiAutoTeacherRule =
        c.id === "Regula_ObsadzenieNauczycieli" ||
        c.id.startsWith("Regula_PreferowanyNauczyciel_") ||
        c.id.startsWith("Regula_JedenNauczyciel_");
      if (!isUiAutoTeacherRule) {
        markExternal(c.type);
      }
    } else {
      markExternal(c.type);
    }
  });

  if (inst.constraints.length > 0) {
    constraintsData.ruleAssignAllTimes = hasAssignTime;
    constraintsData.ruleAvoidClashes = hasAvoidClashes;
  }

  // Klasy
  const classes: SimpleClass[] = classResources.map((r) => {
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
  const teachers: SimpleTeacher[] = teacherResources.map((r) => {
    const subs = teacherSubjectsMap.get(r.id);
    const subjectsArr = subs && subs.size > 0 ? Array.from(subs) : undefined;
    return {
      id: r.id,
      name: r.name || r.id,
      subject: subjectsArr ? subjectsArr[0] : "Przedmiot ogólny",
      subjects: subjectsArr,
      workload: 18,
      unavailableTimes: unavailMap[r.id]?.times || [],
      unavailableSoft: unavailMap[r.id]?.isSoft || false,
      unavailableWeight: unavailMap[r.id]?.weight || 10,
      maxDaysPerWeek: clusterDaysMap[r.id]?.maxDays ?? null,
      minDaysPerWeek: clusterDaysMap[r.id]?.minDays ?? null,
      maxDailyLessons: busyLimitMap[r.id]?.maxDaily ?? null,
      maxGapsPerWeek: gapsLimitMap[r.id] ?? null,
    };
  });

  // Sale
  const rooms: SimpleRoom[] = roomResources.map((r) => {
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

  const externalConstraintsNotice = Array.from(externalTypeCounts.entries()).map(([cType, count]) => {
    const label = CONSTRAINT_FRIENDLY_MAP[cType]?.title || cType;
    return count > 1 ? `${label} (×${count})` : label;
  });

  return {
    schoolId,
    schoolName,
    selectedDays,
    periodsPerDay,
    classes,
    teachers,
    rooms,
    lessons,
    linkedGroups,
    orderRules,
    externalConstraintsNotice,
    constraintsData,
  };
}

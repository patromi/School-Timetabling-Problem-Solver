// frontend/src/lib/components/editor/xhsttAssembler.ts
import type {
  InstanceDetail,
  GroupItem,
  TimeItem,
  ResourceTypeItem,
  ResourceItem,
  EventItem,
  ConstraintItem,
  EventPairItem,
} from "../../types";
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  LinkedGroupItem,
  OrderRuleItem,
} from "./types";

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
    ruleAssignAllTimes = true,
    ruleAvoidClashes = true,
    ruleStudentNoGaps = true,
    ruleStudentNoGapsSoft = false,
    studentGapsWeight = 10,
    ruleTeacherMaxGaps = true,
    teacherMaxGapsLimit = 2,
    teacherGapsWeight = 5,
    ruleMaxLessonsDaily = true,
    studentMinDailyLimit = 4,
    studentMaxDailyLimit = 8,
    studentDailyWeight = 4,
    ruleTeacherMaxDaily = true,
    teacherMaxDailyLimit = 6,
    teacherDailyWeight = 4,
    ruleTeacherClusterDays = true,
    teacherClusterWeight = 6,
    ruleSpreadSubjects = true,
    ruleSpreadSubjectsSoft = true,
    spreadSubjectsWeight = 5,
    spreadSubjectsMode = "single",
    ruleMorningCoreSubjects = true,
    morningCoreSubjectsSoft = true,
    morningCoreSubjectsWeight = 4,
    ruleAfternoonFlexSubjects = false,
    afternoonFlexSubjectsSoft = true,
    afternoonFlexSubjectsWeight = 3,
    linkedGroups = [],
    ruleOrderLessons = false,
    orderRules = [],
    orderLessonsIsSoft = true,
    orderLessonsWeight = 5,
  } = options;

  const dayMap: Record<string, string> = {
    Poniedziałek: "Mon",
    Wtorek: "Tue",
    Środa: "Wed",
    Czwartek: "Thu",
    Piątek: "Fri",
    Sobota: "Sat",
    Niedziela: "Sun",
  };

  // 1. Time Groups (Dni)
  const time_groups: GroupItem[] = selectedDays.map((dName) => ({
    id: dayMap[dName] || dName.slice(0, 3),
    name: dName,
    kind: "Day",
  }));

  // 2. Times (Godziny lekcyjne)
  const times: TimeItem[] = [];
  time_groups.forEach((tg) => {
    for (let p = 1; p <= periodsPerDay; p++) {
      times.push({
        id: `${tg.id}_${p}`,
        name: `${tg.name} lekcja ${p}`,
        group_refs: [tg.id],
      });
    }
  });

  // 3. Typy zasobów
  const resource_types: ResourceTypeItem[] = [
    { id: "Teacher", name: "Nauczyciel" },
    { id: "Class", name: "Oddział / Klasa" },
    { id: "Room", name: "Sala lekcyjna" },
  ];

  // 4. Zasoby
  const resources: ResourceItem[] = [
    ...teachers.map((t) => ({
      id: t.id,
      name: t.name,
      resource_type_ref: "Teacher",
      group_refs: [],
    })),
    ...classes.map((c) => ({
      id: c.id,
      name: c.name,
      resource_type_ref: "Class",
      group_refs: [],
    })),
    ...rooms.map((r) => ({
      id: r.id,
      name: r.name,
      resource_type_ref: "Room",
      group_refs: [],
    })),
  ];

  // 5. Zdarzenia (Events / Lekcje) i Grupy Lekcyjne (EventGroups)
  const events: EventItem[] = [];
  const event_groups: GroupItem[] = [];

  lessons.forEach((l) => {
    const egId = `EG_${l.classId}_${l.subject.replace(/[^a-zA-Z0-9_]/g, "_")}`;
    if (!event_groups.some((eg) => eg.id === egId)) {
      event_groups.push({
        id: egId,
        name: `${l.subject} - klasa ${l.classId}`,
        kind: "EventGroup",
      });
    }

    let remainingCount = l.weeklyCount;
    let counter = 1;

    // Jeśli zaznaczono blok i lekcja ma min 2h
    const allowBlock = ruleContinuousBlocks && l.duration === 2 && remainingCount >= 2;
    if (allowBlock) {
      events.push({
        id: `${l.id}_blok`,
        name: `${l.subject} (blok 2h)`,
        duration: 2,
        course_ref: l.subject,
        time_ref: null,
        workload: 2,
        group_refs: [egId],
        resources: [
          { role: "Teacher", resource_ref: l.teacherId, resource_type_ref: "Teacher", workload: 2 },
          { role: "Class", resource_ref: l.classId, resource_type_ref: "Class", workload: 2 },
          ...(l.roomId ? [{ role: "Room", resource_ref: l.roomId, resource_type_ref: "Room", workload: 2 }] : []),
        ],
      });
      remainingCount -= 2;
    }

    // Pozostałe jako lekcje pojedyncze
    for (let i = 0; i < remainingCount; i++) {
      events.push({
        id: `${l.id}_${counter++}`,
        name: l.subject,
        duration: 1,
        course_ref: l.subject,
        time_ref: null,
        workload: 1,
        group_refs: [egId],
        resources: [
          { role: "Teacher", resource_ref: l.teacherId, resource_type_ref: "Teacher", workload: 1 },
          { role: "Class", resource_ref: l.classId, resource_type_ref: "Class", workload: 1 },
          ...(l.roomId ? [{ role: "Room", resource_ref: l.roomId, resource_type_ref: "Room", workload: 1 }] : []),
        ],
      });
    }
  });

  // 6. Ograniczenia i Reguły (Constraints)
  const constraints: ConstraintItem[] = [];

  // Reguła bezwzględna 1: Obsadzenie czasu
  if (ruleAssignAllTimes) {
    constraints.push({
      type: "AssignTimeConstraint",
      id: "Regula_ObsadzenieCzasu",
      name: "Każda lekcja musi mieć wyznaczony termin",
      required: true,
      weight: 1,
      cost_function: "Linear",
      applies_to: {
        event_groups: [],
        events: events.map((e) => e.id),
        resource_groups: [],
        resources: [],
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {},
    });
  }

  // Reguła bezwzględna 2: Brak nakładania się lekcji
  if (ruleAvoidClashes) {
    constraints.push({
      type: "AvoidClashesConstraint",
      id: "Regula_BrakKonfliktow",
      name: "Żaden nauczyciel, klasa ani sala nie ma lekcji w tym samym czasie",
      required: true,
      weight: 1,
      cost_function: "Linear",
      applies_to: {
        event_groups: [],
        events: [],
        resource_groups: [],
        resources: resources.map((r) => r.id),
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {},
    });
  }

  const dayGroupRefs = time_groups.map((tg) => tg.id);

  // Reguła 3: Brak okienek dla klas (XHSTT LimitIdleTimesConstraint)
  if (ruleStudentNoGaps && classes.length > 0) {
    constraints.push({
      type: "LimitIdleTimesConstraint",
      id: "Regula_KlasyBezOkienek",
      name: "Brak pustych godzin (okienek) w planie uczniów",
      required: !ruleStudentNoGapsSoft,
      weight: studentGapsWeight,
      cost_function: "Linear",
      applies_to: {
        event_groups: [],
        events: [],
        resource_groups: [],
        resources: classes.map((c) => c.id),
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {
        TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
        Minimum: 0,
        Maximum: 0,
      },
    });
  }

  // Reguła 4: Ograniczenie okienek dla nauczycieli (XHSTT LimitIdleTimesConstraint)
  // 4a. Indywidualne limity okienek
  teachers.forEach((t) => {
    if (t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined) {
      constraints.push({
        type: "LimitIdleTimesConstraint",
        id: `LimitOkienek_${t.id}`,
        name: `Limit okienek dla ${t.name}: maks. ${t.maxGapsPerWeek}`,
        required: false,
        weight: teacherGapsWeight,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: [t.id],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
          Minimum: 0,
          Maximum: t.maxGapsPerWeek,
        },
      });
    }
  });

  // 4b. Domyślny limit okienek dla pozostałych nauczycieli
  const teachersDefaultGaps = teachers.filter(
    (t) => t.maxGapsPerWeek === null || t.maxGapsPerWeek === undefined
  );
  if (ruleTeacherMaxGaps && teachersDefaultGaps.length > 0) {
    constraints.push({
      type: "LimitIdleTimesConstraint",
      id: "Regula_KomfortNauczycieli",
      name: "Minimalizacja okienek dla kadry pedagogicznej",
      required: false,
      weight: teacherGapsWeight,
      cost_function: "Linear",
      applies_to: {
        event_groups: [],
        events: [],
        resource_groups: [],
        resources: teachersDefaultGaps.map((t) => t.id),
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {
        TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
        Minimum: 0,
        Maximum: teacherMaxGapsLimit,
      },
    });
  }

  // Reguła 5: Limit lekcji dziennie dla klas (XHSTT LimitBusyTimesConstraint)
  if (ruleMaxLessonsDaily && classes.length > 0) {
    const defaultDailyClasses = classes.filter(
      (c) =>
        (c.minDailyLessons === null || c.minDailyLessons === undefined) &&
        (c.maxDailyLessons === null || c.maxDailyLessons === undefined)
    );
    if (defaultDailyClasses.length > 0) {
      constraints.push({
        type: "LimitBusyTimesConstraint",
        id: "Regula_HigienaNauki",
        name: "Maksymalna dopuszczalna liczba lekcji dziennie dla klas",
        required: false,
        weight: studentDailyWeight,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: defaultDailyClasses.map((c) => c.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
          Minimum: studentMinDailyLimit,
          Maximum: studentMaxDailyLimit,
        },
      });
    }

    classes.forEach((c) => {
      if (
        (c.minDailyLessons !== null && c.minDailyLessons !== undefined) ||
        (c.maxDailyLessons !== null && c.maxDailyLessons !== undefined)
      ) {
        constraints.push({
          type: "LimitBusyTimesConstraint",
          id: `LimitLekcji_${c.id}`,
          name: `Dzienne ramy lekcyjne dla oddziału ${c.name}`,
          required: false,
          weight: studentDailyWeight,
          cost_function: "Linear",
          applies_to: {
            event_groups: [],
            events: [],
            resource_groups: [],
            resources: [c.id],
            time_groups: [],
            times: [],
            event_pairs: [],
          },
          params: {
            TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
            Minimum: c.minDailyLessons ?? studentMinDailyLimit,
            Maximum: c.maxDailyLessons ?? studentMaxDailyLimit,
          },
        });
      }
    });
  }

  // Reguła 5b: Maksymalne obciążenie dzienne nauczycieli (XHSTT LimitBusyTimesConstraint)
  if (ruleTeacherMaxDaily && teachers.length > 0) {
    const defaultDailyTeachers = teachers.filter(
      (t) => t.maxDailyLessons === null || t.maxDailyLessons === undefined
    );
    if (defaultDailyTeachers.length > 0) {
      constraints.push({
        type: "LimitBusyTimesConstraint",
        id: "Regula_MaxLekcjiNauczycieli",
        name: "Maksymalna liczba lekcji dziennie dla kadry",
        required: false,
        weight: teacherDailyWeight,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: defaultDailyTeachers.map((t) => t.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
          Minimum: 0,
          Maximum: teacherMaxDailyLimit,
        },
      });
    }

    teachers.forEach((t) => {
      if (t.maxDailyLessons !== null && t.maxDailyLessons !== undefined) {
        constraints.push({
          type: "LimitBusyTimesConstraint",
          id: `MaxLekcji_${t.id}`,
          name: `Maks. liczba lekcji dziennie dla ${t.name}: ${t.maxDailyLessons}`,
          required: false,
          weight: teacherDailyWeight,
          cost_function: "Linear",
          applies_to: {
            event_groups: [],
            events: [],
            resource_groups: [],
            resources: [t.id],
            time_groups: [],
            times: [],
            event_pairs: [],
          },
          params: {
            TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
            Minimum: 0,
            Maximum: t.maxDailyLessons,
          },
        });
      }
    });
  }

  // Reguła 5c: Kompaktowy tydzień pracy kadry (XHSTT ClusterBusyTimesConstraint)
  if (ruleTeacherClusterDays) {
    teachers.forEach((t) => {
      if (t.maxDaysPerWeek && t.maxDaysPerWeek < selectedDays.length) {
        constraints.push({
          type: "ClusterBusyTimesConstraint",
          id: `ClusterDni_${t.id}`,
          name: `Kompaktowy tydzień pracy: ${t.name} (maks. ${t.maxDaysPerWeek} dni)`,
          required: false,
          weight: teacherClusterWeight,
          cost_function: "Linear",
          applies_to: {
            event_groups: [],
            events: [],
            resource_groups: [],
            resources: [t.id],
            time_groups: [],
            times: [],
            event_pairs: [],
          },
          params: {
            TimeGroups: dayGroupRefs.map((d) => ({ reference: d })),
            Minimum: t.minDaysPerWeek ?? 1,
            Maximum: t.maxDaysPerWeek,
          },
        });
      }
    });
  }

  // Reguła 6: Równomierny rozkład zajęć (XHSTT SpreadEventsConstraint - Faza 3)
  if (ruleSpreadSubjects && event_groups.length > 0) {
    constraints.push({
      type: "SpreadEventsConstraint",
      id: "Regula_RozkladPrzedmiotow",
      name: "Równomierny rozkład lekcji w poszczególne dni tygodnia",
      required: !ruleSpreadSubjectsSoft,
      weight: spreadSubjectsWeight,
      cost_function: "Linear",
      applies_to: {
        event_groups: event_groups.map((eg) => eg.id),
        events: [],
        resource_groups: [],
        resources: [],
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {
        TimeGroups: dayGroupRefs.map((d) => ({
          reference: d,
          Minimum: 0,
          Maximum: spreadSubjectsMode === "single" ? 1 : 2,
        })),
      },
    });
  }

  // Reguła 7: Indywidualne niedostępności zasobów (XHSTT AvoidUnavailableTimesConstraint - Faza 1)
  [...teachers, ...classes, ...rooms].forEach((res) => {
    if (res.unavailableTimes && res.unavailableTimes.length > 0) {
      constraints.push({
        type: "AvoidUnavailableTimesConstraint",
        id: `Niedostepnosc_${res.id}`,
        name: `Niedostępność w planie: ${res.name}`,
        required: !res.unavailableSoft,
        weight: res.unavailableWeight || 10,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: [res.id],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Times: res.unavailableTimes.map((tId) => ({ reference: tId })),
        },
      });
    }
  });

  // Reguła 8: Indywidualne preferencje godzinowe dla lekcji (XHSTT PreferTimesConstraint - Faza 3)
  lessons.forEach((l) => {
    if (l.timePreference && l.timePreference !== "any" && l.preferredTimes && l.preferredTimes.length > 0) {
      const matchingEvents = events.filter((e) => e.id.startsWith(`${l.id}_`));
      if (matchingEvents.length > 0) {
        constraints.push({
          type: "PreferTimesConstraint",
          id: `PrefTimes_${l.id}`,
          name: `Preferowane godziny: ${l.subject} (${classes.find((c) => c.id === l.classId)?.name || l.classId})`,
          required: !l.preferenceSoft,
          weight: l.preferenceWeight || 4,
          cost_function: "Linear",
          applies_to: {
            events: matchingEvents.map((e) => e.id),
            event_groups: [],
            resource_groups: [],
            resources: [],
            time_groups: [],
            times: [],
            event_pairs: [],
          },
          params: {
            Times: l.preferredTimes.map((t) => ({ reference: t })),
          },
        });
      }
    }
  });

  // Reguła 9: Przedmioty ścisłe i maturalne rano (XHSTT PreferTimesConstraint - Faza 3)
  if (ruleMorningCoreSubjects) {
    const coreKeywords = ["matemat", "fizyk", "chem", "polsk", "biolog"];
    const morningEvents = events.filter((e) =>
      coreKeywords.some((kw) => e.name.toLowerCase().includes(kw))
    );
    if (morningEvents.length > 0) {
      const morningTimes: { reference: string }[] = [];
      selectedDays.forEach((dName) => {
        const dId = dayMap[dName] || dName.slice(0, 3);
        for (let p = 1; p <= Math.min(4, periodsPerDay); p++) {
          morningTimes.push({ reference: `${dId}_${p}` });
        }
      });
      constraints.push({
        type: "PreferTimesConstraint",
        id: "Regula_PrzedmiotyScisleRano",
        name: "Przedmioty ścisłe i maturalne rano (lekcje 1-4)",
        required: !morningCoreSubjectsSoft,
        weight: morningCoreSubjectsWeight,
        cost_function: "Linear",
        applies_to: {
          events: morningEvents.map((e) => e.id),
          event_groups: [],
          resource_groups: [],
          resources: [],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Times: morningTimes,
        },
      });
    }
  }

  // Reguła 10: WF i przedmioty artystyczne po południu (XHSTT PreferTimesConstraint - Faza 3)
  if (ruleAfternoonFlexSubjects) {
    const flexKeywords = ["wychowanie", "wf", "plastyk", "muzyk", "relig", "etyk"];
    const afternoonEvents = events.filter((e) =>
      flexKeywords.some((kw) => e.name.toLowerCase().includes(kw))
    );
    if (afternoonEvents.length > 0) {
      const afternoonTimes: { reference: string }[] = [];
      selectedDays.forEach((dName) => {
        const dId = dayMap[dName] || dName.slice(0, 3);
        for (let p = 5; p <= periodsPerDay; p++) {
          afternoonTimes.push({ reference: `${dId}_${p}` });
        }
      });
      constraints.push({
        type: "PreferTimesConstraint",
        id: "Regula_PrzedmiotyPopoludniowe",
        name: "WF i przedmioty artystyczne po południu (lekcje 5+)",
        required: !afternoonFlexSubjectsSoft,
        weight: afternoonFlexSubjectsWeight,
        cost_function: "Linear",
        applies_to: {
          events: afternoonEvents.map((e) => e.id),
          event_groups: [],
          resource_groups: [],
          resources: [],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Times: afternoonTimes,
        },
      });
    }
  }

  // Reguła 11: Lekcje równoległe i jednoczesne (XHSTT LinkEventsConstraint - Faza 3)
  linkedGroups.forEach((group) => {
    const memberLessons = lessons.filter((l) => group.lessonIds.includes(l.id));
    if (memberLessons.length < 2) return;
    const minCount = Math.min(...memberLessons.map((l) => l.weeklyCount));
    const linkedEgIds: string[] = [];
    for (let i = 1; i <= minCount; i++) {
      const egId = `EG_Linked_${group.id}_${i}`;
      linkedEgIds.push(egId);
      if (!event_groups.some((eg) => eg.id === egId)) {
        event_groups.push({
          id: egId,
          name: `${group.name} (część ${i})`,
          kind: "EventGroup",
        });
      }
      memberLessons.forEach((l) => {
        const ev = events.find((e) => e.id === `${l.id}_${i}`);
        if (ev && !ev.group_refs.includes(egId)) {
          ev.group_refs.push(egId);
        }
      });
    }
    if (linkedEgIds.length > 0) {
      constraints.push({
        type: "LinkEventsConstraint",
        id: `Link_${group.id}`,
        name: `Lekcje równoległe: ${group.name}`,
        required: !group.isSoft,
        weight: group.weight || 10,
        cost_function: "Linear",
        applies_to: {
          event_groups: linkedEgIds,
          events: [],
          resource_groups: [],
          resources: [],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {},
      });
    }
  });

  // Reguła 12: Następstwo i kolejność lekcji (XHSTT OrderEventsConstraint - Faza 3)
  if (ruleOrderLessons && orderRules.length > 0) {
    const pairs: EventPairItem[] = [];
    orderRules.forEach((rule) => {
      const firstEvents = events.filter((e) => e.id.startsWith(`${rule.firstLessonId}_`));
      const secondEvents = events.filter((e) => e.id.startsWith(`${rule.secondLessonId}_`));
      if (firstEvents.length > 0 && secondEvents.length > 0) {
        pairs.push({
          first_event: firstEvents[0].id,
          second_event: secondEvents[0].id,
          min_separation: rule.minSeparation,
          max_separation: null,
        });
      }
    });
    if (pairs.length > 0) {
      constraints.push({
        type: "OrderEventsConstraint",
        id: "Regula_KolejnoscLekcji",
        name: "Kolejność i następstwo wybranych lekcji",
        required: !orderLessonsIsSoft,
        weight: orderLessonsWeight,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: [],
          time_groups: [],
          times: [],
          event_pairs: pairs,
        },
        params: {},
      });
    }
  }

  return {
    id: schoolId.trim() || `Szkola_${Date.now()}`,
    name: schoolName.trim(),
    time_groups,
    times,
    resource_types,
    resource_groups: [],
    resources,
    event_groups,
    events,
    constraints,
  };
}

// frontend/src/lib/components/editor/assembler/standardConstraints.ts
import type { ConstraintItem, EventItem, ResourceItem, GroupItem } from "../../../types";
import type { SimpleClass, SimpleTeacher, SimpleRoom, SimpleLesson } from "../types";

export interface StandardConstraintsOptions {
  events: EventItem[];
  resources: ResourceItem[];
  time_groups: GroupItem[];
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
  lessons?: SimpleLesson[];
  selectedDays: string[];
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
}

export function buildStandardConstraints(options: StandardConstraintsOptions): ConstraintItem[] {
  const {
    events,
    resources,
    time_groups,
    teachers,
    classes,
    rooms,
    selectedDays,
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
  } = options;

  const constraints: ConstraintItem[] = [];
  const dayGroupRefs = time_groups.map((tg) => tg.id);

  // 1. Obsadzenie czasu (AssignTimeConstraint)
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

  // 2. Brak nakładania się lekcji (AvoidClashesConstraint)
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

  // 3. Brak okienek dla klas (LimitIdleTimesConstraint)
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

  // 4. Okienka dla nauczycieli (LimitIdleTimesConstraint)
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

  // 5. Limit lekcji dziennie dla klas (LimitBusyTimesConstraint)
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

  // 6. Maksymalne obciążenie dzienne nauczycieli (LimitBusyTimesConstraint)
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

  // 7. Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint)
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

  // 8. Indywidualne niedostępności zasobów (AvoidUnavailableTimesConstraint)
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

  // 9. Automatyczny dobór nauczycieli przez solver ("Dowolny nauczyciel")
  const anyTeacherLessons = (options.lessons || []).filter((l) => l.teacherId === "__ANY__");
  const unassignedTeacherEvents = events.filter((e) =>
    e.resources.some((r) => r.role === "Teacher" && !r.resource_ref)
  );

  if (unassignedTeacherEvents.length > 0) {
    constraints.push({
      type: "AssignResourceConstraint",
      id: "Regula_ObsadzenieNauczycieli",
      name: "Obsadzenie nauczyciela dla lekcji z opcją 'Dowolny nauczyciel'",
      required: true,
      weight: 1,
      cost_function: "Linear",
      applies_to: {
        event_groups: [],
        events: unassignedTeacherEvents.map((e) => e.id),
        resource_groups: [],
        resources: [],
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {
        Role: "Teacher",
      },
    });
  }

  anyTeacherLessons.forEach((l) => {
    const targetSub = l.subject.trim().toLowerCase();
    const qualified = teachers.filter((t) => {
      const primary = (t.subject || "").toLowerCase();
      const subs = (t.subjects || []).map((s) => s.toLowerCase());
      return primary === targetSub || subs.includes(targetSub);
    });

    const lEvents = events.filter(
      (e) => e.id === `${l.id}_blok` || e.id.startsWith(`${l.id}_`)
    );

    // PreferResourcesConstraint: tylko nauczyciele z uprawnieniami do tego przedmiotu
    if (qualified.length > 0 && lEvents.length > 0) {
      constraints.push({
        type: "PreferResourcesConstraint",
        id: `Regula_PreferowanyNauczyciel_${l.id}`,
        name: `Uprawnienia: dobór nauczyciela ${l.subject} dla ${l.classId}`,
        required: true,
        weight: 10,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: lEvents.map((e) => e.id),
          resource_groups: [],
          resources: qualified.map((t) => t.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Role: "Teacher",
        },
      });
    }

    // AvoidSplitAssignmentsConstraint: jeden stały nauczyciel dla całej grupy godzin
    const egId = `EG_${l.classId}_${l.subject.replace(/[^a-zA-Z0-9_]/g, "_")}`;
    constraints.push({
      type: "AvoidSplitAssignmentsConstraint",
      id: `Regula_JedenNauczyciel_${l.id}`,
      name: `Jeden stały nauczyciel dla ${l.subject} (${l.classId})`,
      required: true,
      weight: 10,
      cost_function: "Linear",
      applies_to: {
        event_groups: [egId],
        events: [],
        resource_groups: [],
        resources: [],
        time_groups: [],
        times: [],
        event_pairs: [],
      },
      params: {
        Role: "Teacher",
      },
    });
  });

  return constraints;
}

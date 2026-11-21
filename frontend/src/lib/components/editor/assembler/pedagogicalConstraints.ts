// frontend/src/lib/components/editor/assembler/pedagogicalConstraints.ts
import type {
  ConstraintItem,
  EventItem,
  GroupItem,
  EventPairItem,
} from "../../../types";
import type {
  SimpleClass,
  SimpleLesson,
  LinkedGroupItem,
  OrderRuleItem,
} from "../types";
import { DAY_MAP } from "./baseAssembler.ts";

export interface PedagogicalConstraintsOptions {
  events: EventItem[];
  event_groups: GroupItem[];
  time_groups: GroupItem[];
  lessons: SimpleLesson[];
  classes: SimpleClass[];
  selectedDays: string[];
  periodsPerDay: number;
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

export function buildPedagogicalConstraints(
  options: PedagogicalConstraintsOptions
): ConstraintItem[] {
  const {
    events,
    event_groups,
    time_groups,
    lessons,
    classes,
    selectedDays,
    periodsPerDay,
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

  const constraints: ConstraintItem[] = [];
  const dayGroupRefs = time_groups.map((tg) => tg.id);

  // 1. Równomierny rozkład zajęć (SpreadEventsConstraint)
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

  // 2. Indywidualne preferencje godzinowe dla lekcji (PreferTimesConstraint)
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

  // 3. Przedmioty ścisłe i maturalne rano (PreferTimesConstraint)
  if (ruleMorningCoreSubjects) {
    const coreKeywords = ["matemat", "fizyk", "chem", "polsk", "biolog"];
    const morningEvents = events.filter((e) =>
      coreKeywords.some((kw) => e.name.toLowerCase().includes(kw))
    );
    if (morningEvents.length > 0) {
      const morningTimes: { reference: string }[] = [];
      selectedDays.forEach((dName) => {
        const dId = DAY_MAP[dName] || dName.slice(0, 3);
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

  // 4. WF i przedmioty artystyczne po południu (PreferTimesConstraint)
  if (ruleAfternoonFlexSubjects) {
    const flexKeywords = ["wychowanie", "wf", "plastyk", "muzyk", "relig", "etyk"];
    const afternoonEvents = events.filter((e) =>
      flexKeywords.some((kw) => e.name.toLowerCase().includes(kw))
    );
    if (afternoonEvents.length > 0) {
      const afternoonTimes: { reference: string }[] = [];
      selectedDays.forEach((dName) => {
        const dId = DAY_MAP[dName] || dName.slice(0, 3);
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

  // 5. Lekcje równoległe i jednoczesne (LinkEventsConstraint)
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

  // 6. Następstwo i kolejność lekcji (OrderEventsConstraint)
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

  return constraints;
}

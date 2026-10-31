// frontend/src/lib/components/editor/assembler/baseAssembler.ts
import type {
  GroupItem,
  TimeItem,
  ResourceTypeItem,
  ResourceItem,
  EventItem,
} from "../../../types";
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
} from "../types";

export const DAY_MAP: Record<string, string> = {
  Poniedziałek: "Mon",
  Wtorek: "Tue",
  Środa: "Wed",
  Czwartek: "Thu",
  Piątek: "Fri",
  Sobota: "Sat",
  Niedziela: "Sun",
};

export interface BaseStructureResult {
  time_groups: GroupItem[];
  times: TimeItem[];
  resource_types: ResourceTypeItem[];
  resources: ResourceItem[];
  events: EventItem[];
  event_groups: GroupItem[];
}

export function buildBaseStructure(options: {
  selectedDays: string[];
  periodsPerDay: number;
  teachers: SimpleTeacher[];
  classes: SimpleClass[];
  rooms: SimpleRoom[];
  lessons: SimpleLesson[];
  ruleContinuousBlocks?: boolean;
}): BaseStructureResult {
  const {
    selectedDays,
    periodsPerDay,
    teachers,
    classes,
    rooms,
    lessons,
    ruleContinuousBlocks = false,
  } = options;

  // 1. Time Groups (Dni)
  const time_groups: GroupItem[] = selectedDays.map((dName) => ({
    id: DAY_MAP[dName] || dName.slice(0, 3),
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
          ...(l.teacherId ? [{ role: "Teacher", resource_ref: l.teacherId, resource_type_ref: "Teacher", workload: 2 }] : []),
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
          ...(l.teacherId ? [{ role: "Teacher", resource_ref: l.teacherId, resource_type_ref: "Teacher", workload: 1 }] : []),
          { role: "Class", resource_ref: l.classId, resource_type_ref: "Class", workload: 1 },
          ...(l.roomId ? [{ role: "Room", resource_ref: l.roomId, resource_type_ref: "Room", workload: 1 }] : []),
        ],
      });
    }
  });

  return {
    time_groups,
    times,
    resource_types,
    resources,
    events,
    event_groups,
  };
}

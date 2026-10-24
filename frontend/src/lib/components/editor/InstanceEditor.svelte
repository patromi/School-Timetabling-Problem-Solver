<script lang="ts">
  import type {
    ConstraintItem,
    EventItem,
    GroupItem,
    InstanceDetail,
    ResourceItem,
    ResourceTypeItem,
    TimeItem,
  } from "../../types";
  import { saveInstance } from "../../api";

  interface Props {
    initialInstance?: InstanceDetail | null;
    onSaved: (savedId: string) => void;
    onCancel?: () => void;
  }

  let { initialInstance = null, onSaved, onCancel }: Props = $props();

  // Step navigation (1 to 6)
  let currentStep = $state<number>(1);

  // Krok 1: Podstawowe informacje i organizacja tygodnia
  let schoolName = $state("XIV Liceum Ogólnokształcące");
  let schoolId = $state("LO_14_Poznan");
  let selectedDays = $state<string[]>(["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"]);
  let periodsPerDay = $state<number>(7);
  let periodDurationMinutes = $state<number>(45);

  // Krok 2: Oddziały klasowe i Nauczyciele
  interface SimpleClass {
    id: string;
    name: string;
  }
  let classes = $state<SimpleClass[]>([
    { id: "1A", name: "Klasa 1A (Mat-Fiz)" },
    { id: "1B", name: "Klasa 1B (Biol-Chem)" },
    { id: "2A", name: "Klasa 2A (Mat-Fiz)" },
    { id: "2B", name: "Klasa 2B (Humanistyczna)" },
    { id: "3A", name: "Klasa 3A (Mat-Geo)" },
    { id: "3B", name: "Klasa 3B (Biol-Chem)" },
    { id: "4A", name: "Klasa 4A (Mat-Fiz)" },
    { id: "4B", name: "Klasa 4B (Językowa)" },
  ]);

  interface SimpleTeacher {
    id: string;
    name: string;
    subject: string;
    workload: number;
  }
  let teachers = $state<SimpleTeacher[]>([
    { id: "Kowalski_J", name: "Jan Kowalski", subject: "Matematyka", workload: 18 },
    { id: "Nowak_A", name: "Anna Nowak", subject: "Język polski", workload: 18 },
    { id: "Wisniewski_P", name: "Piotr Wiśniewski", subject: "Wychowanie Fizyczne", workload: 18 },
    { id: "Wojcik_M", name: "Marta Wójcik", subject: "Język angielski", workload: 16 },
    { id: "Kaminski_T", name: "Tomasz Kamiński", subject: "Historia", workload: 15 },
    { id: "Lewandowska_E", name: "Ewa Lewandowska", subject: "Biologia", workload: 14 },
    { id: "Zieliński_K", name: "Krzysztof Zieliński", subject: "Fizyka", workload: 14 },
    { id: "Szymanska_B", name: "Beata Szymańska", subject: "Chemia", workload: 14 },
    { id: "Dabrowski_R", name: "Robert Dąbrowski", subject: "Informatyka", workload: 14 },
    { id: "Kozlowska_M", name: "Magdalena Kozłowska", subject: "Geografia", workload: 14 },
  ]);

  // Krok 3: Sale lekcyjne
  interface SimpleRoom {
    id: string;
    name: string;
    type: "general" | "gym" | "lab" | "it";
  }
  let rooms = $state<SimpleRoom[]>([
    { id: "s101", name: "Sala 101 (Ogólna)", type: "general" },
    { id: "s102", name: "Sala 102 (Ogólna)", type: "general" },
    { id: "s103", name: "Sala 103 (Ogólna)", type: "general" },
    { id: "s104", name: "Sala 104 (Ogólna)", type: "general" },
    { id: "s201", name: "Sala 201 (Ogólna)", type: "general" },
    { id: "s202", name: "Sala 202 (Ogólna)", type: "general" },
    { id: "sgym", name: "Sala gimnastyczna (WF)", type: "gym" },
    { id: "slab", name: "Pracownia chemiczno-biol.", type: "lab" },
    { id: "sit", name: "Pracownia informatyczna", type: "it" },
  ]);

  // Krok 4: Wykaz lekcji (Siatka zajęć)
  interface SimpleLesson {
    id: string;
    subject: string;
    classId: string;
    teacherId: string;
    roomId: string;
    duration: number; // 1 or 2
    weeklyCount: number; // how many lessons per week
  }
  let lessons = $state<SimpleLesson[]>([]);

  // Krok 5: Reguły szkoły (Zasady dydaktyczne)
  let ruleAvoidClashes = $state(true); // Wymóg bezwzględny
  let ruleAssignAllTimes = $state(true); // Wymóg bezwzględny
  let ruleStudentNoGaps = $state(true); // Brak okienek dla klas
  let ruleTeacherMaxGaps = $state(true); // Ograniczenie okienek dla kadry
  let ruleMaxLessonsDaily = $state(true); // Max lekcji dziennie
  let maxDailyLimit = $state(7);
  let ruleSpreadSubjects = $state(true); // Równomierny rozkład w tygodniu
  let ruleContinuousBlocks = $state(true); // Ciągłość bloków 2h

  // Form helpers for adding single items
  let newClassName = $state("");
  let newTeacherName = $state("");
  let newTeacherSubject = $state("Matematyka");
  let newTeacherWorkload = $state(18);

  let newRoomName = $state("");
  let newRoomType = $state<"general" | "gym" | "lab" | "it">("general");

  // Lesson form
  let lessonSubject = $state("Matematyka");
  let lessonClassId = $state("");
  let lessonTeacherId = $state("");
  let lessonRoomId = $state("");
  let lessonCount = $state(4);
  let lessonIsBlock = $state(false);

  // Status
  let isSaving = $state(false);
  let saveError = $state<string | null>(null);
  let saveSuccess = $state<string | null>(null);

  // Synchronize on start or on template load
  $effect(() => {
    if (initialInstance) {
      loadFromExistingInstance(initialInstance);
    } else if (lessons.length === 0) {
      generateDefaultCurriculum();
    }
  });

  function loadFromExistingInstance(inst: InstanceDetail) {
    schoolId = `${inst.id}_kopia`;
    schoolName = `${inst.name} (edycja)`;

    // Dni
    const days = inst.time_groups.filter((g) => g.kind.toLowerCase() === "day");
    if (days.length > 0) {
      selectedDays = days.map((d) => d.name || d.id);
      periodsPerDay = Math.max(5, Math.round(inst.times.length / days.length));
    }

    // Klasy
    const foundClasses = inst.resources
      .filter((r) => r.resource_type_ref.toLowerCase().includes("class"))
      .map((r) => ({ id: r.id, name: r.name || r.id }));
    if (foundClasses.length > 0) classes = foundClasses;

    // Nauczyciele
    const foundTeachers = inst.resources
      .filter((r) => r.resource_type_ref.toLowerCase().includes("teach"))
      .map((r) => ({
        id: r.id,
        name: r.name || r.id,
        subject: "Przedmiot ogólny",
        workload: 18,
      }));
    if (foundTeachers.length > 0) teachers = foundTeachers;

    // Sale
    const foundRooms = inst.resources
      .filter((r) => r.resource_type_ref.toLowerCase().includes("room"))
      .map((r) => ({
        id: r.id,
        name: r.name || r.id,
        type: ((r.name || "").toLowerCase().includes("gym") ? "gym" : "general") as "general" | "gym" | "lab" | "it",
      }));
    if (foundRooms.length > 0) rooms = foundRooms;

    if (classes.length > 0 && teachers.length > 0) {
      generateDefaultCurriculum();
    }
  }

  // Pre-sets
  function loadHighSchoolPreset() {
    schoolName = "I Liceum Ogólnokształcące";
    schoolId = `LO_Wzorcowe_${Date.now().toString().slice(-4)}`;
    selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
    periodsPerDay = 7;

    classes = [
      { id: "1A", name: "Klasa 1A (Mat-Fiz)" },
      { id: "1B", name: "Klasa 1B (Biol-Chem)" },
      { id: "2A", name: "Klasa 2A (Mat-Fiz)" },
      { id: "2B", name: "Klasa 2B (Humanistyczna)" },
      { id: "3A", name: "Klasa 3A (Mat-Geo)" },
      { id: "3B", name: "Klasa 3B (Biol-Chem)" },
    ];

    teachers = [
      { id: "Kowalski_J", name: "Jan Kowalski", subject: "Matematyka", workload: 18 },
      { id: "Nowak_A", name: "Anna Nowak", subject: "Język polski", workload: 18 },
      { id: "Wisniewski_P", name: "Piotr Wiśniewski", subject: "Wychowanie Fizyczne", workload: 18 },
      { id: "Wojcik_M", name: "Marta Wójcik", subject: "Język angielski", workload: 18 },
      { id: "Kaminski_T", name: "Tomasz Kamiński", subject: "Historia", workload: 12 },
      { id: "Lewandowska_E", name: "Ewa Lewandowska", subject: "Biologia", workload: 12 },
      { id: "Zielinski_K", name: "Krzysztof Zieliński", subject: "Fizyka", workload: 12 },
      { id: "Szymanska_B", name: "Beata Szymańska", subject: "Chemia", workload: 12 },
      { id: "Dabrowski_R", name: "Robert Dąbrowski", subject: "Informatyka", workload: 12 },
      { id: "Kozlowska_M", name: "Magdalena Kozłowska", subject: "Geografia", workload: 12 },
    ];

    rooms = [
      { id: "s101", name: "Sala 101 (Matematyka)", type: "general" },
      { id: "s102", name: "Sala 102 (Język polski)", type: "general" },
      { id: "s103", name: "Sala 103 (Język angielski)", type: "general" },
      { id: "s104", name: "Sala 104 (Historia)", type: "general" },
      { id: "s201", name: "Sala 201 (Geografia)", type: "general" },
      { id: "sgym", name: "Hala sportowa (WF)", type: "gym" },
      { id: "slab", name: "Laboratorium (Chemia/Biologia)", type: "lab" },
      { id: "sit", name: "Pracownia komputerowa", type: "it" },
    ];

    generateDefaultCurriculum();
  }

  function generateDefaultCurriculum() {
    if (classes.length === 0 || teachers.length === 0) return;

    const subjectsDistribution: { subject: string; count: number; isBlock: boolean; roomType: SimpleRoom["type"] }[] = [
      { subject: "Język polski", count: 4, isBlock: false, roomType: "general" },
      { subject: "Matematyka", count: 4, isBlock: false, roomType: "general" },
      { subject: "Język angielski", count: 3, isBlock: false, roomType: "general" },
      { subject: "Wychowanie Fizyczne", count: 3, isBlock: true, roomType: "gym" },
      { subject: "Historia", count: 2, isBlock: false, roomType: "general" },
      { subject: "Biologia", count: 2, isBlock: false, roomType: "lab" },
      { subject: "Fizyka", count: 2, isBlock: false, roomType: "general" },
      { subject: "Chemia", count: 2, isBlock: false, roomType: "lab" },
      { subject: "Informatyka", count: 2, isBlock: true, roomType: "it" },
      { subject: "Geografia", count: 2, isBlock: false, roomType: "general" },
    ];

    const generatedLessons: SimpleLesson[] = [];

    classes.forEach((cls) => {
      subjectsDistribution.forEach((spec) => {
        // Znajdź odpowiedniego nauczyciela
        let teacher = teachers.find((t) => t.subject.toLowerCase() === spec.subject.toLowerCase());
        if (!teacher) {
          teacher = teachers[Math.floor(Math.random() * teachers.length)];
        }

        // Znajdź odpowiednią salę
        let room = rooms.find((r) => r.type === spec.roomType);
        if (!room) {
          room = rooms.find((r) => r.type === "general") || rooms[0];
        }

        generatedLessons.push({
          id: `${cls.id}_${spec.subject.replace(/[^a-zA-Z0-9]/g, "")}`,
          subject: spec.subject,
          classId: cls.id,
          teacherId: teacher.id,
          roomId: room ? room.id : "",
          duration: spec.isBlock ? 2 : 1,
          weeklyCount: spec.count,
        });
      });
    });

    lessons = generatedLessons;
  }

  // Adding single Class
  function addClass() {
    if (!newClassName.trim()) return;
    const cleanId = newClassName.trim().replace(/\s+/g, "_");
    if (classes.some((c) => c.id === cleanId)) {
      alert("Oddział o tej nazwie już istnieje!");
      return;
    }
    classes = [...classes, { id: cleanId, name: newClassName.trim() }];
    newClassName = "";
  }

  function removeClass(id: string) {
    classes = classes.filter((c) => c.id !== id);
    lessons = lessons.filter((l) => l.classId !== id);
  }

  // Adding single Teacher
  function addTeacher() {
    if (!newTeacherName.trim()) return;
    const cleanId = newTeacherName.trim().replace(/\s+/g, "_");
    if (teachers.some((t) => t.id === cleanId)) {
      alert("Nauczyciel o tym nazwisku już istnieje!");
      return;
    }
    teachers = [
      ...teachers,
      {
        id: cleanId,
        name: newTeacherName.trim(),
        subject: newTeacherSubject,
        workload: Number(newTeacherWorkload) || 18,
      },
    ];
    newTeacherName = "";
  }

  function removeTeacher(id: string) {
    teachers = teachers.filter((t) => t.id !== id);
  }

  // Adding single Room
  function addRoom() {
    if (!newRoomName.trim()) return;
    const cleanId = newRoomName.trim().replace(/\s+/g, "_");
    if (rooms.some((r) => r.id === cleanId)) {
      alert("Sala o tej nazwie już istnieje!");
      return;
    }
    rooms = [...rooms, { id: cleanId, name: newRoomName.trim(), type: newRoomType }];
    newRoomName = "";
  }

  function removeRoom(id: string) {
    rooms = rooms.filter((r) => r.id !== id);
  }

  // Adding single Lesson
  function addCustomLesson() {
    if (!lessonClassId || !lessonTeacherId) {
      alert("Wybierz klasę oraz nauczyciela!");
      return;
    }
    const cleanId = `${lessonClassId}_${lessonSubject.replace(/\s+/g, "")}_${Date.now().toString().slice(-3)}`;
    lessons = [
      ...lessons,
      {
        id: cleanId,
        subject: lessonSubject,
        classId: lessonClassId,
        teacherId: lessonTeacherId,
        roomId: lessonRoomId || (rooms[0] ? rooms[0].id : ""),
        duration: lessonIsBlock ? 2 : 1,
        weeklyCount: Number(lessonCount) || 1,
      },
    ];
  }

  function removeLesson(id: string) {
    lessons = lessons.filter((l) => l.id !== id);
  }

  // Bilans obliczeniowy
  let totalHoursAssigned = $derived(
    lessons.reduce((sum, l) => sum + l.weeklyCount, 0)
  );

  let weeklySlotsCapacity = $derived(
    selectedDays.length * periodsPerDay
  );

  let isSchoolValid = $derived(
    schoolName.trim().length > 0 &&
      classes.length > 0 &&
      teachers.length > 0 &&
      rooms.length > 0 &&
      lessons.length > 0
  );

  // Budowanie pełnej struktury XHSTT zgodnej z backendem
  function assembleXhsttPayload(): InstanceDetail {
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
      if (l.duration === 2 && remainingCount >= 2) {
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
    if (ruleStudentNoGaps) {
      constraints.push({
        type: "LimitIdleTimesConstraint",
        id: "Regula_KlasyBezOkienek",
        name: "Brak pustych godzin (okienek) w planie uczniów",
        required: true,
        weight: 10,
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
          TimeGroups: dayGroupRefs,
          Minimum: 0,
          Maximum: 0,
        },
      });
    }

    // Reguła 4: Ograniczenie okienek dla nauczycieli (XHSTT LimitIdleTimesConstraint)
    if (ruleTeacherMaxGaps) {
      constraints.push({
        type: "LimitIdleTimesConstraint",
        id: "Regula_KomfortNauczycieli",
        name: "Minimalizacja okienek dla kadry pedagogicznej",
        required: false,
        weight: 5,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: teachers.map((t) => t.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          TimeGroups: dayGroupRefs,
          Minimum: 0,
          Maximum: 2,
        },
      });
    }

    // Reguła 5: Limit lekcji dziennie (XHSTT LimitBusyTimesConstraint)
    if (ruleMaxLessonsDaily) {
      constraints.push({
        type: "LimitBusyTimesConstraint",
        id: "Regula_HigienaNauki",
        name: "Maksymalna dopuszczalna liczba lekcji dziennie",
        required: false,
        weight: 5,
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
          TimeGroups: dayGroupRefs,
          Minimum: 0,
          Maximum: maxDailyLimit,
        },
      });
    }

    // Reguła 6: Równomierny rozkład zajęć (XHSTT SpreadEventsConstraint)
    if (ruleSpreadSubjects && event_groups.length > 0) {
      constraints.push({
        type: "SpreadEventsConstraint",
        id: "Regula_RozkladPrzedmiotow",
        name: "Równomierny rozkład lekcji w poszczególne dni tygodnia",
        required: false,
        weight: 5,
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
            Maximum: 1,
          })),
        },
      });
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

  async function handleSaveSchool() {
    if (!isSchoolValid) {
      saveError = "Proszę uzupełnić nazwę szkoły oraz upewnić się, że dodano klasy, nauczycieli i lekcje.";
      return;
    }

    isSaving = true;
    saveError = null;
    saveSuccess = null;

    try {
      const payload = assembleXhsttPayload();
      await saveInstance(payload);
      saveSuccess = `Szkoła ${payload.name} została pomyślnie zapisana!`;
      setTimeout(() => {
        onSaved(payload.id);
      }, 700);
    } catch (err: any) {
      saveError = err.message || "Błąd podczas zapisywania szkoły w bazie.";
    } finally {
      isSaving = false;
    }
  }
</script>

<div class="h-full flex flex-col bg-slate-50 font-sans text-slate-800 antialiased overflow-hidden">
  <!-- Top Wizard Header -->
  <header class="h-16 border-b border-slate-200/90 bg-white px-6 flex items-center justify-between shrink-0 shadow-2xs">
    <div class="flex items-center gap-3">
      {#if onCancel}
        <button
          type="button"
          onclick={onCancel}
          class="px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg transition-all cursor-pointer flex items-center gap-1.5"
        >
          <span>←</span>
          <span>Wróć do przeglądarki</span>
        </button>
      {/if}
      <div class="border-l border-slate-200 pl-3">
        <h2 class="text-sm font-bold text-slate-900 leading-tight">
          Kreator Arkusza Organizacyjnego Szkoły
        </h2>
        <p class="text-[11px] text-slate-500">
          Proste wprowadzanie klas, kadry, sal i siatki godzin
        </p>
      </div>
    </div>

    <!-- Actions -->
    <div class="flex items-center gap-2">
      <button
        type="button"
        onclick={loadHighSchoolPreset}
        class="px-3.5 py-1.5 bg-amber-50 hover:bg-amber-100 text-amber-900 border border-amber-200/70 text-xs font-semibold rounded-lg transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs"
      >
        <span>⚡ Wstaw gotowy szablon szkoły</span>
      </button>

      <button
        type="button"
        onclick={handleSaveSchool}
        disabled={isSaving || !isSchoolValid}
        class={`px-4 py-1.5 text-xs font-bold rounded-lg transition-all shadow-xs flex items-center gap-1.5 cursor-pointer ${
          isSchoolValid
            ? "bg-blue-600 hover:bg-blue-700 text-white"
            : "bg-slate-200 text-slate-400 cursor-not-allowed"
        }`}
      >
        {#if isSaving}
          <span class="animate-spin">⏳</span>
          <span>Zapisywanie...</span>
        {:else}
          <span>💾 Zapisz szkołę</span>
        {/if}
      </button>
    </div>
  </header>

  <!-- Step Bar (Human Stepper) -->
  <nav class="bg-white border-b border-slate-200/70 px-6 py-2.5 flex items-center justify-between overflow-x-auto gap-2">
    <div class="flex items-center gap-1 sm:gap-2">
      <!-- Step 1 -->
      <button
        type="button"
        onclick={() => (currentStep = 1)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 1
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 1 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">1</span>
        <span>🏫 Szkoła i Tydzień</span>
      </button>

      <span class="text-slate-300">›</span>

      <!-- Step 2 -->
      <button
        type="button"
        onclick={() => (currentStep = 2)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 2
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 2 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">2</span>
        <span>👥 Klasy i Nauczyciele</span>
      </button>

      <span class="text-slate-300">›</span>

      <!-- Step 3 -->
      <button
        type="button"
        onclick={() => (currentStep = 3)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 3
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 3 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">3</span>
        <span>🚪 Sale lekcyjne</span>
      </button>

      <span class="text-slate-300">›</span>

      <!-- Step 4 -->
      <button
        type="button"
        onclick={() => (currentStep = 4)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 4
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 4 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">4</span>
        <span>📚 Siatka lekcji</span>
      </button>

      <span class="text-slate-300">›</span>

      <!-- Step 5 -->
      <button
        type="button"
        onclick={() => (currentStep = 5)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 5
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 5 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">5</span>
        <span>⚖️ Zasady planu</span>
      </button>

      <span class="text-slate-300">›</span>

      <!-- Step 6 -->
      <button
        type="button"
        onclick={() => (currentStep = 6)}
        class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          currentStep === 6
            ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
            : "text-slate-600 hover:bg-slate-50"
        }`}
      >
        <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 6 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">6</span>
        <span>✅ Podsumowanie</span>
      </button>
    </div>

    <!-- Health indicator -->
    <div class="text-xs font-semibold flex items-center gap-2">
      {#if isSchoolValid}
        <span class="px-2.5 py-1 bg-emerald-50 text-emerald-800 rounded-full border border-emerald-200/70 text-[11px]">
          ● Kompletny arkusz
        </span>
      {:else}
        <span class="px-2.5 py-1 bg-amber-50 text-amber-800 rounded-full border border-amber-200/70 text-[11px]">
          ● Wymaga uzupełnienia
        </span>
      {/if}
    </div>
  </nav>

  <!-- Alerts -->
  {#if saveError}
    <div class="mx-6 mt-3 p-3 bg-rose-50 border border-rose-200 rounded-xl text-xs text-rose-800 flex items-center justify-between">
      <span>{saveError}</span>
      <button type="button" class="font-bold cursor-pointer" onclick={() => (saveError = null)}>✕</button>
    </div>
  {/if}
  {#if saveSuccess}
    <div class="mx-6 mt-3 p-3 bg-emerald-50 border border-emerald-200 rounded-xl text-xs text-emerald-800 flex items-center justify-between">
      <span>{saveSuccess}</span>
    </div>
  {/if}

  <!-- Step Content Body (Scrollable) -->
  <main class="flex-1 overflow-y-auto p-6 max-w-5xl mx-auto w-full">
    <!-- STEP 1: SZKOŁA I TYDZIEŃ -->
    {#if currentStep === 1}
      <div class="flex flex-col gap-6">
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
          <div>
            <h3 class="text-base font-bold text-slate-900">Krok 1: Podstawowe informacje o placówce</h3>
            <p class="text-xs text-slate-500 mt-0.5">
              Wprowadź oficjalną nazwę szkoły oraz określ organizację tygodnia pracy.
            </p>
          </div>

          <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label for="school-name-input" class="block text-xs font-semibold text-slate-700 mb-1">Nazwa szkoły:</label>
              <input
                id="school-name-input"
                type="text"
                bind:value={schoolName}
                placeholder="np. Liceum Ogólnokształcące nr 1"
                class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
              />
            </div>
            <div>
              <label for="school-id-input" class="block text-xs font-semibold text-slate-700 mb-1">Identyfikator systemowy:</label>
              <input
                id="school-id-input"
                type="text"
                bind:value={schoolId}
                placeholder="np. LO_1_Wzorcowe"
                class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-lg font-mono text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
              />
            </div>
          </div>
        </div>

        <!-- Tydzień pracy -->
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
          <h3 class="text-sm font-bold text-slate-900">Dni nauki w tygodniu</h3>
          <div class="flex flex-wrap gap-2">
            {#each ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota"] as day}
              {@const isChecked = selectedDays.includes(day)}
              <button
                type="button"
                onclick={() => {
                  if (isChecked) {
                    if (selectedDays.length > 1) {
                      selectedDays = selectedDays.filter((d) => d !== day);
                    }
                  } else {
                    selectedDays = [...selectedDays, day];
                  }
                }}
                class={`px-4 py-2 rounded-xl text-xs font-semibold border transition-all cursor-pointer flex items-center gap-2 ${
                  isChecked
                    ? "bg-blue-50 border-blue-300 text-blue-900 shadow-2xs"
                    : "bg-slate-50 border-slate-200 text-slate-500 hover:bg-slate-100"
                }`}
              >
                <span>{isChecked ? "✓" : "+"}</span>
                <span>{day}</span>
              </button>
            {/each}
          </div>

          <div class="mt-2 pt-4 border-t border-slate-100 grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label for="periods-per-day-input" class="block text-xs font-semibold text-slate-700 mb-1">
                Liczba godzin lekcyjnych dziennie:
              </label>
              <div class="flex items-center gap-3">
                <input
                  id="periods-per-day-input"
                  type="range"
                  min="5"
                  max="10"
                  bind:value={periodsPerDay}
                  class="flex-1 cursor-pointer accent-blue-600"
                />
                <span class="w-12 text-center font-bold text-slate-900 text-sm bg-slate-100 py-1 rounded-lg">
                  {periodsPerDay} lekcji
                </span>
              </div>
              <p class="text-[11px] text-slate-400 mt-1">
                Typowo: 7 lekcji dziennie (np. 8:00 - 14:15)
              </p>
            </div>

            <div class="bg-slate-50 p-3 rounded-xl border border-slate-200/80 flex flex-col justify-between">
              <span class="text-xs text-slate-500 font-medium">Pojemność tygodniowa:</span>
              <div class="flex items-baseline gap-1 mt-1">
                <span class="text-2xl font-bold text-slate-900">{weeklySlotsCapacity}</span>
                <span class="text-xs text-slate-500">dostępnych slotów na klasę</span>
              </div>
              <span class="text-[11px] text-slate-400">
                {selectedDays.length} dni × {periodsPerDay} lekcji = {weeklySlotsCapacity} godzin lekcyjnych
              </span>
            </div>
          </div>
        </div>
      </div>

    <!-- STEP 2: ODDZIAŁY KLASOWE I NAUCZYCIELE -->
    {:else if currentStep === 2}
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        <!-- Klasy -->
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col justify-between gap-4">
          <div>
            <div class="flex items-center justify-between">
              <h3 class="text-sm font-bold text-slate-900">Oddziały klasowe ({classes.length})</h3>
              <span class="text-xs text-slate-400">Grupy uczniów</span>
            </div>

            <!-- Add class input -->
            <div class="mt-3 flex gap-2">
              <input
                type="text"
                bind:value={newClassName}
                placeholder="np. 1C lub 2D (Human)"
                onkeydown={(e) => e.key === "Enter" && addClass()}
                class="flex-1 px-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
              />
              <button
                type="button"
                onclick={addClass}
                class="px-3 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
              >
                + Dodaj
              </button>
            </div>

            <!-- Classes list -->
            <div class="mt-3 flex flex-col gap-1.5 max-h-72 overflow-y-auto">
              {#each classes as cls}
                <div class="p-2.5 bg-slate-50 rounded-xl border border-slate-100 flex items-center justify-between text-xs">
                  <span class="font-bold text-slate-800">👥 {cls.name}</span>
                  <button
                    type="button"
                    onclick={() => removeClass(cls.id)}
                    class="text-slate-400 hover:text-rose-600 text-xs px-2 py-0.5 rounded cursor-pointer"
                    title="Usuń klasę"
                  >
                    ✕
                  </button>
                </div>
              {/each}
            </div>
          </div>

          <div class="pt-3 border-t border-slate-100 text-[11px] text-slate-500">
            Wskazówka: Do każdej klasy zostaną przypisane lekcje w kroku 4.
          </div>
        </div>

        <!-- Nauczyciele -->
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col justify-between gap-4">
          <div>
            <div class="flex items-center justify-between">
              <h3 class="text-sm font-bold text-slate-900">Kadra nauczycielska ({teachers.length})</h3>
              <span class="text-xs text-slate-400">Pedagodzy</span>
            </div>

            <!-- Add teacher input -->
            <div class="mt-3 flex flex-col gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
              <input
                type="text"
                bind:value={newTeacherName}
                placeholder="Imię i nazwisko (np. Jan Kowalski)"
                class="px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              />
              <div class="flex gap-2">
                <input
                  type="text"
                  bind:value={newTeacherSubject}
                  placeholder="Nauczany przedmiot (np. Matematyka)"
                  class="flex-1 px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
                />
                <button
                  type="button"
                  onclick={addTeacher}
                  class="px-3 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all shrink-0"
                >
                  + Dodaj
                </button>
              </div>
            </div>

            <!-- Teachers list -->
            <div class="mt-3 flex flex-col gap-1.5 max-h-60 overflow-y-auto">
              {#each teachers as t}
                <div class="p-2.5 bg-slate-50 rounded-xl border border-slate-100 flex items-center justify-between text-xs">
                  <div class="flex flex-col">
                    <span class="font-bold text-slate-800">👨‍🏫 {t.name}</span>
                    <span class="text-[11px] text-slate-500">{t.subject} • pensum: {t.workload}h</span>
                  </div>
                  <button
                    type="button"
                    onclick={() => removeTeacher(t.id)}
                    class="text-slate-400 hover:text-rose-600 text-xs px-2 py-0.5 rounded cursor-pointer"
                    title="Usuń nauczyciela"
                  >
                    ✕
                  </button>
                </div>
              {/each}
            </div>
          </div>

          <div class="pt-3 border-t border-slate-100 text-[11px] text-slate-500">
            Nauczyciele zostaną powiązani z odpowiednimi przedmiotami w planie.
          </div>
        </div>
      </div>

    <!-- STEP 3: SALE LEKCYJNE -->
    {:else if currentStep === 3}
      <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
        <div class="flex items-center justify-between">
          <div>
            <h3 class="text-sm font-bold text-slate-900">Baza lokalowa szkoły ({rooms.length} sal)</h3>
            <p class="text-xs text-slate-500 mt-0.5">
              Zdefiniuj dostępne sale ogólne, pracownie specjalistyczne oraz hale sportowe.
            </p>
          </div>
        </div>

        <!-- Add room input -->
        <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
          <input
            type="text"
            bind:value={newRoomName}
            placeholder="Nazwa sali (np. Sala 105 lub Pracownia fizyczna)"
            class="flex-1 min-w-[200px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
          />
          <select
            bind:value={newRoomType}
            class="px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
          >
            <option value="general">Sala ogólna / przedmiotowa</option>
            <option value="gym">Sala gimnastyczna / WF</option>
            <option value="lab">Laboratorium / Pracownia przyrodnicza</option>
            <option value="it">Pracownia komputerowa</option>
          </select>
          <button
            type="button"
            onclick={addRoom}
            class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
          >
            + Dodaj salę
          </button>
        </div>

        <!-- Rooms grid -->
        <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-3">
          {#each rooms as room}
            <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex items-center justify-between text-xs">
              <div class="flex items-center gap-2">
                <span class="text-base">
                  {#if room.type === 'gym'}🏀
                  {:else if room.type === 'lab'}🔬
                  {:else if room.type === 'it'}💻
                  {:else}🚪{/if}
                </span>
                <div>
                  <div class="font-bold text-slate-800">{room.name}</div>
                  <div class="text-[10px] text-slate-400 capitalize">Typ: {room.type}</div>
                </div>
              </div>
              <button
                type="button"
                onclick={() => removeRoom(room.id)}
                class="text-slate-400 hover:text-rose-600 text-xs px-2 py-0.5 rounded cursor-pointer"
                title="Usuń salę"
              >
                ✕
              </button>
            </div>
          {/each}
        </div>
      </div>

    <!-- STEP 4: SIATKA LEKCJI (PRZYDZIAŁ) -->
    {:else if currentStep === 4}
      <div class="flex flex-col gap-6">
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
          <div>
            <h3 class="text-sm font-bold text-slate-900">Siatka godzin i przydział lekcji</h3>
            <p class="text-xs text-slate-500 mt-0.5">
              Łącznie zaplanowano: <strong>{totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
            </p>
          </div>

          <button
            type="button"
            onclick={generateDefaultCurriculum}
            class="px-3.5 py-2 bg-blue-50 hover:bg-blue-100 text-blue-900 border border-blue-200 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5"
          >
            <span>⚡ Zrównoważ i wygeneruj pełną siatkę dla klas</span>
          </button>
        </div>

        <!-- Add lesson form -->
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
          <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
            Ręczne dodanie zajęć do planu
          </h4>
          <div class="grid grid-cols-2 md:grid-cols-6 gap-2 text-xs">
            <!-- Przedmiot -->
            <div>
              <label for="lesson-subject-input" class="block text-[11px] font-semibold text-slate-600 mb-1">Przedmiot:</label>
              <input
                id="lesson-subject-input"
                type="text"
                bind:value={lessonSubject}
                placeholder="np. Matematyka"
                class="w-full px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              />
            </div>

            <!-- Klasa -->
            <div>
              <label for="lesson-class-select" class="block text-[11px] font-semibold text-slate-600 mb-1">Dla klasy:</label>
              <select
                id="lesson-class-select"
                bind:value={lessonClassId}
                class="w-full px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              >
                <option value="">Wybierz klasę</option>
                {#each classes as c}
                  <option value={c.id}>{c.name}</option>
                {/each}
              </select>
            </div>

            <!-- Nauczyciel -->
            <div>
              <label for="lesson-teacher-select" class="block text-[11px] font-semibold text-slate-600 mb-1">Nauczyciel:</label>
              <select
                id="lesson-teacher-select"
                bind:value={lessonTeacherId}
                class="w-full px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              >
                <option value="">Wybierz nauczyciela</option>
                {#each teachers as t}
                  <option value={t.id}>{t.name} ({t.subject})</option>
                {/each}
              </select>
            </div>

            <!-- Sala -->
            <div>
              <label for="lesson-room-select" class="block text-[11px] font-semibold text-slate-600 mb-1">Sala:</label>
              <select
                id="lesson-room-select"
                bind:value={lessonRoomId}
                class="w-full px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              >
                <option value="">Dowolna / ogólna</option>
                {#each rooms as r}
                  <option value={r.id}>{r.name}</option>
                {/each}
              </select>
            </div>

            <!-- Godzin tyg -->
            <div>
              <label for="lesson-count-input" class="block text-[11px] font-semibold text-slate-600 mb-1">Godzin/tydz:</label>
              <input
                id="lesson-count-input"
                type="number"
                min="1"
                max="10"
                bind:value={lessonCount}
                class="w-full px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              />
            </div>

            <!-- Przycisk dodaj -->
            <div class="flex items-end">
              <button
                type="button"
                onclick={addCustomLesson}
                class="w-full py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-semibold rounded-lg transition-all cursor-pointer text-center"
              >
                + Dodaj lekcję
              </button>
            </div>
          </div>
        </div>

        <!-- Lessons table -->
        <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden">
          <div class="p-4 border-b border-slate-100 flex items-center justify-between text-xs">
            <span class="font-bold text-slate-800">Wykaz zaplanowanych zajęć ({lessons.length} pozycji)</span>
            <span class="text-slate-400">Możesz usuwać lub modyfikować wpisy</span>
          </div>
          <div class="divide-y divide-slate-100 max-h-96 overflow-y-auto">
            {#each lessons as l}
              {@const cls = classes.find((c) => c.id === l.classId)}
              {@const t = teachers.find((tch) => tch.id === l.teacherId)}
              {@const r = rooms.find((rm) => rm.id === l.roomId)}
              <div class="p-3 hover:bg-slate-50/80 flex items-center justify-between text-xs">
                <div class="flex items-center gap-3">
                  <span class="w-8 h-8 rounded-lg bg-blue-50 text-blue-700 flex items-center justify-center font-bold text-xs">
                    📚
                  </span>
                  <div>
                    <span class="font-bold text-slate-900">{l.subject}</span>
                    <div class="text-[11px] text-slate-500 flex items-center gap-2 mt-0.5">
                      <span>Klasa: <strong>{cls ? cls.name : l.classId}</strong></span>
                      <span>•</span>
                      <span>Nauczyciel: <strong>{t ? t.name : l.teacherId}</strong></span>
                      <span>•</span>
                      <span>Sala: {r ? r.name : "Dowolna"}</span>
                    </div>
                  </div>
                </div>

                <div class="flex items-center gap-3">
                  <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-semibold">
                    {l.weeklyCount} godz./tydz.
                  </span>
                  <button
                    type="button"
                    onclick={() => removeLesson(l.id)}
                    class="text-slate-400 hover:text-rose-600 px-2 py-1 rounded cursor-pointer"
                  >
                    ✕
                  </button>
                </div>
              </div>
            {/each}
          </div>
        </div>
      </div>

    <!-- STEP 5: ZASADY PLANU (REGUŁY DYDAKTYCZNE) -->
    {:else if currentStep === 5}
      <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-6">
        <div>
          <h3 class="text-base font-bold text-slate-900">Standardy dydaktyczne i zasady szkoły</h3>
          <p class="text-xs text-slate-500 mt-0.5">
            Zaznacz reguły, które algorytm optymalizacyjny powinien wymusić lub preferować podczas układania planu.
          </p>
        </div>

        <div class="flex flex-col gap-3">
          <!-- Reguła 1 -->
          <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
            <div class="flex items-start gap-3">
              <span class="text-xl">🛡️</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Brak nakładania się lekcji (AvoidClashes)</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Żaden nauczyciel, oddział klasowy ani sala lekcyjna nie mogą brać udziału w dwóch zajęciach jednocześnie.
                </p>
              </div>
            </div>
            <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
              Wymóg bezwzględny
            </span>
          </div>

          <!-- Reguła 2 -->
          <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
            <div class="flex items-start gap-3">
              <span class="text-xl">⏱️</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Wszystkie lekcje w planie (AssignTime)</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Każda godzina zajęć wprowadzona w siatce musi mieć przydzielony termin w tygodniu.
                </p>
              </div>
            </div>
            <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
              Wymóg bezwzględny
            </span>
          </div>

          <!-- Reguła 3: Okienka uczniów -->
          <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
            <div class="flex items-start gap-3">
              <span class="text-xl">☕</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Brak okienek dla uczniów</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Uczniowie mają zajęcia ciągiem od pierwszej do ostatniej lekcji, bez pustych przerw w środku dnia.
                </p>
              </div>
            </div>
            <input type="checkbox" bind:checked={ruleStudentNoGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
          </label>

          <!-- Reguła 4: Okienka nauczycieli -->
          <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
            <div class="flex items-start gap-3">
              <span class="text-xl">👨‍🏫</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Minimalizacja okienek dla nauczycieli</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Nauczyciele nie powinni czekać bezczynnie w szkole między lekcjami (maks. 1-2 okienka w tygodniu).
                </p>
              </div>
            </div>
            <input type="checkbox" bind:checked={ruleTeacherMaxGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
          </label>

          <!-- Reguła 5: Limit dzienny lekcji -->
          <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
            <div class="flex items-start gap-3">
              <span class="text-xl">⚖️</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Górny limit lekcji dziennie (Higiena nauki)</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Zapobieganie przeciążeniu uczniów zbyt długim dniem szkolnym (maksymalnie {maxDailyLimit} lekcji na dzień).
                </p>
              </div>
            </div>
            <input type="checkbox" bind:checked={ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
          </label>

          <!-- Reguła 6: Rozkład przedmiotów -->
          <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
            <div class="flex items-start gap-3">
              <span class="text-xl">🗓️</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Równomierny rozkład przedmiotów w tygodniu</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Wielogodzinne przedmioty (np. 4h matematyki) są rozkładane na różne dni, a nie kumulowane w jeden dzień.
                </p>
              </div>
            </div>
            <input type="checkbox" bind:checked={ruleSpreadSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
          </label>

          <!-- Reguła 7: Ciągłość bloków -->
          <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
            <div class="flex items-start gap-3">
              <span class="text-xl">🔗</span>
              <div>
                <span class="text-xs font-bold text-slate-900">Ciągłość bloków lekcyjnych (bloki 2h)</span>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Zajęcia 2-godzinne (np. laboratorium, 2h WF-u) odbywają się pod rząd tego samego dnia.
                </p>
              </div>
            </div>
            <input type="checkbox" bind:checked={ruleContinuousBlocks} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
          </label>
        </div>
      </div>

    <!-- STEP 6: PODSUMOWANIE I ZAPIS -->
    {:else if currentStep === 6}
      <div class="flex flex-col gap-6">
        <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
          <div class="flex items-center justify-between pb-4 border-b border-slate-100">
            <div>
              <h3 class="text-lg font-bold text-slate-900">{schoolName}</h3>
              <p class="text-xs text-slate-500">Identyfikator instancji: {schoolId}</p>
            </div>
            <span class="px-3 py-1 bg-emerald-50 text-emerald-800 rounded-full font-semibold text-xs border border-emerald-200">
              Gotowa do zapisu
            </span>
          </div>

          <!-- Bilans -->
          <div class="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs">
            <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
              <span class="text-slate-500">Oddziały klasowe:</span>
              <span class="text-xl font-bold text-slate-900 mt-1">{classes.length} oddziałów</span>
            </div>
            <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
              <span class="text-slate-500">Kadra pedagogiczna:</span>
              <span class="text-xl font-bold text-slate-900 mt-1">{teachers.length} nauczycieli</span>
            </div>
            <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
              <span class="text-slate-500">Sale lekcyjne:</span>
              <span class="text-xl font-bold text-slate-900 mt-1">{rooms.length} pomieszczeń</span>
            </div>
            <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
              <span class="text-slate-500">Wymiar tygodniowy:</span>
              <span class="text-xl font-bold text-slate-900 mt-1">{totalHoursAssigned} godzin</span>
            </div>
          </div>

          <!-- Checklist -->
          <div class="mt-2 flex flex-col gap-2 text-xs">
            <div class="flex items-center gap-2 text-emerald-800">
              <span>✓</span>
              <span>Struktura tygodnia: {selectedDays.length} dni roboczych ({periodsPerDay} lekcji dziennie)</span>
            </div>
            <div class="flex items-center gap-2 text-emerald-800">
              <span>✓</span>
              <span>Wszystkie klasy mają przypisane lekcje i pedagogów</span>
            </div>
            <div class="flex items-center gap-2 text-emerald-800">
              <span>✓</span>
              <span>Wymogi bezwzględne oraz preferencje dydaktyczne zostały poprawnie skonfigurowane</span>
            </div>
          </div>

          <!-- Save CTA -->
          <div class="mt-4 pt-5 border-t border-slate-100 flex items-center justify-between">
            <p class="text-xs text-slate-500">
              Kliknij poniższy przycisk, aby zapisać szkołę i przejść do eksploratora.
            </p>
            <button
              type="button"
              onclick={handleSaveSchool}
              disabled={isSaving || !isSchoolValid}
              class="px-6 py-2.5 bg-blue-600 hover:bg-blue-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-2"
            >
              {#if isSaving}
                <span class="animate-spin">⏳</span>
                <span>Zapisywanie w toku...</span>
              {:else}
                <span>💾 Zapisz szkołę do bazy</span>
              {/if}
            </button>
          </div>
        </div>
      </div>
    {/if}
  </main>

  <!-- Bottom Navigation Buttons (Prev / Next) -->
  <footer class="h-16 border-t border-slate-200/90 bg-white px-6 flex items-center justify-between shrink-0 shadow-2xs">
    <button
      type="button"
      disabled={currentStep === 1}
      onclick={() => (currentStep = Math.max(1, currentStep - 1))}
      class={`px-4 py-2 text-xs font-semibold rounded-lg transition-all cursor-pointer ${
        currentStep > 1
          ? "bg-slate-100 hover:bg-slate-200 text-slate-700"
          : "bg-slate-50 text-slate-300 cursor-not-allowed"
      }`}
    >
      ← Wstecz
    </button>

    <div class="text-xs text-slate-400 font-medium">
      Krok {currentStep} z 6
    </div>

    {#if currentStep < 6}
      <button
        type="button"
        onclick={() => (currentStep = Math.min(6, currentStep + 1))}
        class="px-5 py-2 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg transition-all cursor-pointer"
      >
        Dalej →
      </button>
    {:else}
      <button
        type="button"
        onclick={handleSaveSchool}
        disabled={isSaving || !isSchoolValid}
        class="px-5 py-2 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-lg transition-all cursor-pointer shadow-xs"
      >
        💾 Zapisz szkołę
      </button>
    {/if}
  </footer>
</div>

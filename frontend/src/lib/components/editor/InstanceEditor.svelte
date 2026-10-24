<script lang="ts">
  import type {
    ConstraintItem,
    EventItem,
    GroupItem,
    InstanceDetail,
    ResourceItem,
    ResourceTypeItem,
    TimeItem,
    EventPairItem,
  } from "../../types";
  import { saveInstance } from "../../api";
  import AvailabilityModal from "./AvailabilityModal.svelte";
  import LessonPreferencesModal from "./LessonPreferencesModal.svelte";
  import LinkedLessonsModal, { type LinkedGroupItem } from "./LinkedLessonsModal.svelte";
  import DuplicateCurriculumModal from "./DuplicateCurriculumModal.svelte";

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

  const dayMap: Record<string, string> = {
    Poniedziałek: "Mon",
    Wtorek: "Tue",
    Środa: "Wed",
    Czwartek: "Thu",
    Piątek: "Fri",
    Sobota: "Sat",
    Niedziela: "Sun",
  };

  let currentDaysFormatted = $derived(
    selectedDays.map((dName) => ({
      id: dayMap[dName] || dName.slice(0, 3),
      name: dName,
    }))
  );

  // Krok 2: Oddziały klasowe i Nauczyciele
  interface SimpleClass {
    id: string;
    name: string;
    grade?: number;
    unavailableTimes?: string[];
    unavailableSoft?: boolean;
    unavailableWeight?: number;
    minDailyLessons?: number | null;
    maxDailyLessons?: number | null;
  }
  let classes = $state<SimpleClass[]>([
    { id: "1A", name: "Klasa 1A (Mat-Fiz)", grade: 1 },
    { id: "1B", name: "Klasa 1B (Biol-Chem)", grade: 1 },
    { id: "2A", name: "Klasa 2A (Mat-Fiz)", grade: 2 },
    { id: "2B", name: "Klasa 2B (Humanistyczna)", grade: 2 },
    { id: "3A", name: "Klasa 3A (Mat-Geo)", grade: 3 },
    { id: "3B", name: "Klasa 3B (Biol-Chem)", grade: 3 },
    { id: "4A", name: "Klasa 4A (Mat-Fiz)", grade: 4 },
    { id: "4B", name: "Klasa 4B (Językowa)", grade: 4 },
  ]);

  interface SimpleTeacher {
    id: string;
    name: string;
    subject: string;
    workload: number;
    unavailableTimes?: string[];
    unavailableSoft?: boolean;
    unavailableWeight?: number;
    maxDaysPerWeek?: number | null;
    minDaysPerWeek?: number | null;
    maxDailyLessons?: number | null;
    maxGapsPerWeek?: number | null;
  }
  let teachers = $state<SimpleTeacher[]>([
    { id: "Kowalski_J", name: "Jan Kowalski", subject: "Matematyka", workload: 18 },
    { id: "Nowak_A", name: "Anna Nowak", subject: "Język polski", workload: 18 },
    { id: "Wisniewski_P", name: "Piotr Wiśniewski", subject: "Wychowanie Fizyczne", workload: 18 },
    { id: "Wojcik_M", name: "Marta Wójcik", subject: "Język angielski", workload: 16 },
    { id: "Kaminski_T", name: "Tomasz Kamiński", subject: "Historia", workload: 15, maxDaysPerWeek: 4 },
    { id: "Lewandowska_E", name: "Ewa Lewandowska", subject: "Biologia", workload: 14, maxDaysPerWeek: 3 },
    { id: "Zieliński_K", name: "Krzysztof Zieliński", subject: "Fizyka", workload: 14, maxDaysPerWeek: 3 },
    { id: "Szymanska_B", name: "Beata Szymańska", subject: "Chemia", workload: 14, maxDailyLessons: 5 },
    { id: "Dabrowski_R", name: "Robert Dąbrowski", subject: "Informatyka", workload: 14, maxGapsPerWeek: 1 },
    { id: "Kozlowska_M", name: "Magdalena Kozłowska", subject: "Geografia", workload: 14, maxGapsPerWeek: 1 },
  ]);

  // Sub-nawigacja i organizacja roczników w Kroku 2
  let step2ActiveTab = $state<"classes" | "teachers">("classes");
  let selectedGradeFilter = $state<number | null>(null);
  let duplicateModalTarget = $state<SimpleClass | null>(null);

  // Sub-nawigacja i widoki w Kroku 4 (Siatka zajęć)
  let step4ActiveTab = $state<"lessons" | "byClass" | "byTeacher">("lessons");
  let selectedClassViewId = $state<string>("1A");
  let selectedTeacherViewId = $state<string>("");
  let selectedGradeCurriculum = $state<number>(1);
  let batchGradeSubject = $state("Matematyka");
  let batchGradeHours = $state(4);
  let batchGradeIsBlock = $state(false);

  // Pomocnik rozpoznawania rocznika
  function getClassGrade(cls: SimpleClass): number {
    if (cls.grade !== undefined && cls.grade !== null) return cls.grade;
    const match = cls.name.match(/^(\d+)/) || cls.id.match(/^(\d+)/);
    return match ? parseInt(match[1], 10) : 1;
  }

  let availableGrades = $derived(
    Array.from(new Set(classes.map(getClassGrade))).sort((a, b) => a - b)
  );

  let filteredClasses = $derived(
    selectedGradeFilter !== null
      ? classes.filter((c) => getClassGrade(c) === selectedGradeFilter)
      : classes
  );

  let currentGradeClasses = $derived(
    classes.filter((c) => getClassGrade(c) === selectedGradeCurriculum)
  );

  function getClassWeeklyHours(classId: string): number {
    return lessons
      .filter((l) => l.classId === classId)
      .reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  function getClassLessons(classId: string) {
    return lessons.filter((l) => l.classId === classId);
  }

  let activeViewClass = $derived(
    classes.find((c) => c.id === selectedClassViewId) || classes[0]
  );

  let activeViewTeacher = $derived(
    teachers.find((t) => t.id === selectedTeacherViewId) || teachers[0]
  );

  function getTeacherAssignedHours(teacherId: string): number {
    return lessons
      .filter((l) => l.teacherId === teacherId)
      .reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  function getTeacherClasses(teacherId: string): { classId: string; className: string; subject: string; hours: number }[] {
    return lessons
      .filter((l) => l.teacherId === teacherId)
      .map((l) => ({
        classId: l.classId,
        className: classes.find((c) => c.id === l.classId)?.name || l.classId,
        subject: l.subject,
        hours: l.weeklyCount,
      }));
  }

  function updateTeacherWorkload(teacherId: string, newWorkload: number) {
    const val = Math.max(1, Math.min(60, Number(newWorkload) || 18));
    teachers = teachers.map((t) => (t.id === teacherId ? { ...t, workload: val } : t));
  }

  // Zasada spójności: Stały nauczyciel dla klasy z danego przedmiotu
  function getConsistentTeacherForClassSubject(classId: string, subject: string): string | null {
    const match = lessons.find(
      (l) => l.classId === classId && l.subject.toLowerCase() === subject.toLowerCase()
    );
    return match ? match.teacherId : null;
  }

  // Duplikowanie oddziału klasowego (np. 4A -> 4B)
  function duplicateClass(sourceClassId: string) {
    const srcClass = classes.find((c) => c.id === sourceClassId);
    if (!srcClass) return;

    const baseName = srcClass.name;
    let nextLetter = "B";
    const match = baseName.match(/^(\d+)([A-Z])/i);
    if (match) {
      const gradeNum = match[1];
      const letter = match[2].toUpperCase();
      const charCode = letter.charCodeAt(0);
      for (let c = charCode + 1; c <= 90; c++) {
        const candidateLetter = String.fromCharCode(c);
        const candidateName = `${gradeNum}${candidateLetter}`;
        if (!classes.some((cl) => cl.name.startsWith(candidateName) || cl.id === candidateName)) {
          nextLetter = candidateLetter;
          break;
        }
      }
    }
    const newId = match ? `${match[1]}${nextLetter}` : `${srcClass.id}_kopia`;
    const newName = match ? `Klasa ${match[1]}${nextLetter}` : `${srcClass.name} (kopia)`;

    if (classes.some((c) => c.id === newId)) {
      alert("Klasa o identyfikatorze " + newId + " już istnieje!");
      return;
    }

    const grade = getClassGrade(srcClass);
    const newClassObj: SimpleClass = {
      id: newId,
      name: newName,
      grade: grade,
      unavailableTimes: srcClass.unavailableTimes ? [...srcClass.unavailableTimes] : [],
      unavailableSoft: srcClass.unavailableSoft,
      unavailableWeight: srcClass.unavailableWeight,
      minDailyLessons: srcClass.minDailyLessons,
      maxDailyLessons: srcClass.maxDailyLessons,
    };

    classes = [...classes, newClassObj];
    duplicateCurriculum(sourceClassId, [newId]);
  }

  // Kopiowanie siatki zajęć do wybranych klas
  function duplicateCurriculum(sourceClassId: string, targetClassIds: string[]) {
    if (targetClassIds.length === 0) return;
    const sourceLessons = lessons.filter((l) => l.classId === sourceClassId);
    if (sourceLessons.length === 0) {
      alert("Wybrana klasa źródłowa nie posiada jeszcze żadnych zaplanowanych lekcji.");
      return;
    }

    let newLessons = lessons.filter((l) => !targetClassIds.includes(l.classId));

    targetClassIds.forEach((targetId) => {
      sourceLessons.forEach((sl) => {
        const existingTeacherId = lessons.find(
          (l) => l.classId === targetId && l.subject.toLowerCase() === sl.subject.toLowerCase()
        )?.teacherId;
        const assignedTeacherId = existingTeacherId || sl.teacherId;

        newLessons.push({
          id: `${targetId}_${sl.subject.replace(/[^a-zA-Z0-9]/g, "")}_${Date.now().toString().slice(-4)}_${Math.random().toString(36).substring(2, 5)}`,
          subject: sl.subject,
          classId: targetId,
          teacherId: assignedTeacherId,
          roomId: sl.roomId,
          duration: sl.duration,
          weeklyCount: sl.weeklyCount,
          timePreference: sl.timePreference,
          preferredTimes: sl.preferredTimes ? [...sl.preferredTimes] : undefined,
          preferenceSoft: sl.preferenceSoft,
          preferenceWeight: sl.preferenceWeight,
        });
      });
    });

    lessons = newLessons;
    duplicateModalTarget = null;
  }

  // Przypisanie przedmiotu do całego rocznika
  function addSubjectToGrade(grade: number, subject: string, weeklyCount: number, isBlock: boolean = false) {
    const targetClasses = classes.filter((c) => getClassGrade(c) === grade);
    if (targetClasses.length === 0) {
      alert("Brak klas w roczniku " + grade);
      return;
    }

    let updated = [...lessons];
    targetClasses.forEach((cls) => {
      const existingIndex = updated.findIndex(
        (l) => l.classId === cls.id && l.subject.toLowerCase() === subject.toLowerCase()
      );
      const teacherId =
        getConsistentTeacherForClassSubject(cls.id, subject) ||
        teachers.find((t) => t.subject.toLowerCase() === subject.toLowerCase())?.id ||
        teachers[0]?.id ||
        "";

      if (existingIndex >= 0) {
        updated[existingIndex].weeklyCount = weeklyCount;
      } else {
        updated.push({
          id: `${cls.id}_${subject.replace(/[^a-zA-Z0-9]/g, "")}_${Date.now().toString().slice(-4)}_${Math.random().toString(36).substring(2, 5)}`,
          subject: subject,
          classId: cls.id,
          teacherId: teacherId,
          roomId: "",
          duration: isBlock ? 2 : 1,
          weeklyCount: weeklyCount,
        });
      }
    });
    lessons = updated;
  }

  function selectGradeCurriculum(grade: number) {
    selectedGradeCurriculum = grade;
    const firstInGrade = classes.find((c) => getClassGrade(c) === grade);
    if (firstInGrade) {
      selectedClassViewId = firstInGrade.id;
    }
  }

  // Krok 3: Sale lekcyjne
  interface SimpleRoom {
    id: string;
    name: string;
    type: "general" | "gym" | "lab" | "it";
    unavailableTimes?: string[];
    unavailableSoft?: boolean;
    unavailableWeight?: number;
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
    // Faza 3:
    timePreference?: "any" | "morning" | "afternoon" | "custom";
    preferredTimes?: string[];
    preferenceSoft?: boolean;
    preferenceWeight?: number;
  }
  let lessons = $state<SimpleLesson[]>([]);

  // Faza 3: Lekcje równoległe (LinkEventsConstraint)
  let linkedGroups = $state<LinkedGroupItem[]>([]);
  let isLinkedModalOpen = $state(false);

  // Faza 3: Modal preferencji godzinowych lekcji (PreferTimesConstraint)
  let activeLessonPrefTarget = $state<{
    lessonId: string;
    subject: string;
    className: string;
    preference: "any" | "morning" | "afternoon" | "custom";
    preferredTimes: string[];
    isSoft: boolean;
    weight: number;
  } | null>(null);

  function openLessonPrefModal(lesson: SimpleLesson) {
    const cls = classes.find((c) => c.id === lesson.classId);
    activeLessonPrefTarget = {
      lessonId: lesson.id,
      subject: lesson.subject,
      className: cls ? cls.name : lesson.classId,
      preference: lesson.timePreference || "any",
      preferredTimes: lesson.preferredTimes || [],
      isSoft: lesson.preferenceSoft ?? true,
      weight: lesson.preferenceWeight ?? 4,
    };
  }

  function handleSaveLessonPref(
    pref: "any" | "morning" | "afternoon" | "custom",
    times: string[],
    isSoft: boolean,
    weight: number
  ) {
    if (!activeLessonPrefTarget) return;
    const { lessonId } = activeLessonPrefTarget;
    lessons = lessons.map((l) =>
      l.id === lessonId
        ? {
            ...l,
            timePreference: pref,
            preferredTimes: times,
            preferenceSoft: isSoft,
            preferenceWeight: weight,
          }
        : l
    );
    activeLessonPrefTarget = null;
  }

  // Faza 3: Reguły kolejności lekcji (OrderEventsConstraint)
  interface OrderRuleItem {
    id: string;
    name: string;
    firstLessonId: string;
    secondLessonId: string;
    minSeparation: number;
    isSoft?: boolean;
    weight?: number;
  }
  let orderRules = $state<OrderRuleItem[]>([]);
  let ruleOrderLessons = $state(false);
  let newOrderFirstLesson = $state("");
  let newOrderSecondLesson = $state("");
  let newOrderSeparation = $state(0);
  let newOrderIsSoft = $state(false);
  let newOrderWeight = $state(8);

  function addOrderRule() {
    if (!newOrderFirstLesson || !newOrderSecondLesson || newOrderFirstLesson === newOrderSecondLesson) {
      alert("Wybierz dwie różne lekcje do określenia kolejności!");
      return;
    }
    const l1 = lessons.find((l) => l.id === newOrderFirstLesson);
    const l2 = lessons.find((l) => l.id === newOrderSecondLesson);
    const ruleName = `${l1?.subject || "Lekcja 1"} przed ${l2?.subject || "Lekcja 2"}`;
    orderRules = [
      ...orderRules,
      {
        id: `order_${Date.now()}_${Math.random().toString(36).substring(2, 6)}`,
        name: ruleName,
        firstLessonId: newOrderFirstLesson,
        secondLessonId: newOrderSecondLesson,
        minSeparation: Number(newOrderSeparation) || 0,
        isSoft: newOrderIsSoft,
        weight: newOrderWeight,
      },
    ];
    newOrderFirstLesson = "";
    newOrderSecondLesson = "";
  }

  function removeOrderRule(ruleId: string) {
    orderRules = orderRules.filter((r) => r.id !== ruleId);
  }

  // Krok 5: Reguły szkoły (Zasady dydaktyczne)
  let ruleAvoidClashes = $state(true); // Wymóg bezwzględny
  let ruleAssignAllTimes = $state(true); // Wymóg bezwzględny
  let ruleStudentNoGaps = $state(true); // Brak okienek dla klas (LimitIdleTimesConstraint)
  let ruleStudentNoGapsSoft = $state(false); // Domyślnie HARD
  let studentGapsWeight = $state(10);
  let ruleTeacherMaxGaps = $state(true); // Ograniczenie okienek dla kadry (LimitIdleTimesConstraint)
  let teacherMaxGapsLimit = $state(2); // Domyślnie maks. 2 okienka
  let teacherGapsWeight = $state(5); // Waga kary
  let ruleMaxLessonsDaily = $state(true); // Ramy lekcji dziennie dla klas (LimitBusyTimesConstraint)
  let studentMinDailyLimit = $state(3); // Min lekcji dziennie
  let studentMaxDailyLimit = $state(7); // Max lekcji dziennie
  let studentDailyWeight = $state(5); // Waga kary
  let ruleTeacherMaxDaily = $state(true); // Max lekcji dziennie dla nauczycieli (LimitBusyTimesConstraint)
  let teacherMaxDailyLimit = $state(6); // Domyślnie maks. 6 lekcji
  let teacherDailyWeight = $state(4); // Waga kary
  let ruleTeacherClusterDays = $state(true); // Kompaktowy tydzień pracy (ClusterBusyTimesConstraint)
  let teacherClusterWeight = $state(6); // Waga kary

  // Faza 3: Rozszerzony rozkład przedmiotów w tygodniu (SpreadEventsConstraint)
  let ruleSpreadSubjects = $state(true);
  let spreadSubjectsMode = $state<"single" | "blocks">("single");
  let ruleSpreadSubjectsSoft = $state(true);
  let spreadSubjectsWeight = $state(5);

  let ruleContinuousBlocks = $state(true); // Ciągłość bloków 2h

  // Faza 3: Globalne reguły dydaktyczne pory dnia (PreferTimesConstraint)
  let ruleMorningCoreSubjects = $state(true);
  let morningCoreSubjectsSoft = $state(true);
  let morningCoreSubjectsWeight = $state(4);

  let ruleAfternoonFlexSubjects = $state(false);
  let afternoonFlexSubjectsSoft = $state(true);
  let afternoonFlexSubjectsWeight = $state(3);

  // Form helpers for adding single items
  let newClassName = $state("");
  let newClassGrade = $state(1);
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

  // Modal availability state (Phase 1 & 2)
  let activeAvailabilityTarget = $state<{
    id: string;
    name: string;
    type: "teacher" | "room" | "class";
    unavailableTimes: string[];
    isSoft?: boolean;
    weight?: number;
    maxDaysPerWeek?: number | null;
    minDaysPerWeek?: number | null;
    maxDailyLessons?: number | null;
    maxGapsPerWeek?: number | null;
    minDailyLessons?: number | null;
  } | null>(null);

  function openAvailabilityModal(id: string, name: string, type: "teacher" | "room" | "class") {
    if (type === "teacher") {
      const t = teachers.find((x) => x.id === id);
      if (!t) return;
      activeAvailabilityTarget = {
        id: t.id,
        name: t.name,
        type: "teacher",
        unavailableTimes: t.unavailableTimes || [],
        isSoft: t.unavailableSoft || false,
        weight: t.unavailableWeight || 10,
        maxDaysPerWeek: t.maxDaysPerWeek ?? null,
        minDaysPerWeek: t.minDaysPerWeek ?? null,
        maxDailyLessons: t.maxDailyLessons ?? null,
        maxGapsPerWeek: t.maxGapsPerWeek ?? null,
      };
    } else if (type === "room") {
      const r = rooms.find((x) => x.id === id);
      if (!r) return;
      activeAvailabilityTarget = {
        id: r.id,
        name: r.name,
        type: "room",
        unavailableTimes: r.unavailableTimes || [],
        isSoft: r.unavailableSoft || false,
        weight: r.unavailableWeight || 10,
      };
    } else if (type === "class") {
      const c = classes.find((x) => x.id === id);
      if (!c) return;
      activeAvailabilityTarget = {
        id: c.id,
        name: c.name,
        type: "class",
        unavailableTimes: c.unavailableTimes || [],
        isSoft: c.unavailableSoft || false,
        weight: c.unavailableWeight || 10,
        minDailyLessons: c.minDailyLessons ?? null,
        maxDailyLessons: c.maxDailyLessons ?? null,
      };
    }
  }

  function handleSaveAvailability(
    times: string[],
    isSoft: boolean,
    weight: number,
    extra?: {
      maxDaysPerWeek?: number | null;
      minDaysPerWeek?: number | null;
      maxDailyLessons?: number | null;
      maxGapsPerWeek?: number | null;
      minDailyLessons?: number | null;
    }
  ) {
    if (!activeAvailabilityTarget) return;
    const { id, type } = activeAvailabilityTarget;

    if (type === "teacher") {
      teachers = teachers.map((t) =>
        t.id === id
          ? {
              ...t,
              unavailableTimes: times,
              unavailableSoft: isSoft,
              unavailableWeight: weight,
              maxDaysPerWeek: extra?.maxDaysPerWeek ?? null,
              minDaysPerWeek: extra?.minDaysPerWeek ?? null,
              maxDailyLessons: extra?.maxDailyLessons ?? null,
              maxGapsPerWeek: extra?.maxGapsPerWeek ?? null,
            }
          : t
      );
    } else if (type === "room") {
      rooms = rooms.map((r) =>
        r.id === id
          ? { ...r, unavailableTimes: times, unavailableSoft: isSoft, unavailableWeight: weight }
          : r
      );
    } else if (type === "class") {
      classes = classes.map((c) =>
        c.id === id
          ? {
              ...c,
              unavailableTimes: times,
              unavailableSoft: isSoft,
              unavailableWeight: weight,
              minDailyLessons: extra?.minDailyLessons ?? null,
              maxDailyLessons: extra?.maxDailyLessons ?? null,
            }
          : c
      );
    }
  }

  let totalUnavailabilitiesCount = $derived(
    [...teachers, ...classes, ...rooms].reduce(
      (sum, r) => sum + (r.unavailableTimes?.length || 0),
      0
    )
  );

  let resourcesWithUnavailability = $derived(
    [...teachers, ...classes, ...rooms].filter(
      (r) => (r.unavailableTimes?.length || 0) > 0
    )
  );

  let teachersWithCustomDays = $derived(
    teachers.filter((t) => t.maxDaysPerWeek && t.maxDaysPerWeek < selectedDays.length)
  );

  let teachersWithCustomDailyLimit = $derived(
    teachers.filter((t) => t.maxDailyLessons !== null && t.maxDailyLessons !== undefined)
  );

  let teachersWithCustomGaps = $derived(
    teachers.filter((t) => t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined)
  );

  let classesWithCustomDailyLimit = $derived(
    classes.filter(
      (c) =>
        (c.minDailyLessons !== null && c.minDailyLessons !== undefined) ||
        (c.maxDailyLessons !== null && c.maxDailyLessons !== undefined)
    )
  );

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

    // Extract any existing constraints into maps
    const unavailMap: Record<string, { times: string[]; isSoft: boolean; weight: number }> = {};
    const clusterDaysMap: Record<string, { minDays: number | null; maxDays: number | null }> = {};
    const busyLimitMap: Record<string, { minDaily: number | null; maxDaily: number | null }> = {};
    const gapsLimitMap: Record<string, number | null> = {};

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
        if (c.weight) teacherClusterWeight = c.weight;
        ruleTeacherClusterDays = true;
      } else if (c.type === "LimitBusyTimesConstraint") {
        const minVal = c.params.Minimum !== undefined ? Number(c.params.Minimum) : null;
        const maxVal = c.params.Maximum !== undefined ? Number(c.params.Maximum) : null;
        (c.applies_to?.resources || []).forEach((rId) => {
          busyLimitMap[rId] = { minDaily: minVal, maxDaily: maxVal };
        });
        if (minVal !== null && minVal > 0) studentMinDailyLimit = minVal;
        if (maxVal !== null && maxVal > 0) studentMaxDailyLimit = maxVal;
        if (c.weight) studentDailyWeight = c.weight;
        ruleMaxLessonsDaily = true;
      } else if (c.type === "LimitIdleTimesConstraint") {
        const maxGaps = c.params.Maximum !== undefined ? Number(c.params.Maximum) : 0;
        (c.applies_to?.resources || []).forEach((rId) => {
          gapsLimitMap[rId] = maxGaps;
        });
        if (maxGaps === 0) {
          ruleStudentNoGaps = true;
          ruleStudentNoGapsSoft = !c.required;
          if (c.weight) studentGapsWeight = c.weight;
        } else {
          ruleTeacherMaxGaps = true;
          teacherMaxGapsLimit = maxGaps;
          if (c.weight) teacherGapsWeight = c.weight;
        }
      } else if (c.type === "SpreadEventsConstraint") {
        ruleSpreadSubjects = true;
        ruleSpreadSubjectsSoft = !c.required;
        if (c.weight) spreadSubjectsWeight = c.weight;
        const tgList = c.params?.TimeGroups as any[];
        if (Array.isArray(tgList) && tgList.some((tg) => tg?.Maximum === 2 || tg?.Maximum === "2")) {
          spreadSubjectsMode = "blocks";
        } else {
          spreadSubjectsMode = "single";
        }
      } else if (c.type === "PreferTimesConstraint") {
        if (c.id === "Regula_PrzedmiotyScisleRano") {
          ruleMorningCoreSubjects = true;
          morningCoreSubjectsSoft = !c.required;
          if (c.weight) morningCoreSubjectsWeight = c.weight;
        } else if (c.id === "Regula_PrzedmiotyPopoludniowe") {
          ruleAfternoonFlexSubjects = true;
          afternoonFlexSubjectsSoft = !c.required;
          if (c.weight) afternoonFlexSubjectsWeight = c.weight;
        }
      } else if (c.type === "OrderEventsConstraint") {
        ruleOrderLessons = true;
      }
    });

    // Klasy
    const foundClasses = inst.resources
      .filter((r) => r.resource_type_ref.toLowerCase().includes("class"))
      .map((r) => ({
        id: r.id,
        name: r.name || r.id,
        unavailableTimes: unavailMap[r.id]?.times || [],
        unavailableSoft: unavailMap[r.id]?.isSoft || false,
        unavailableWeight: unavailMap[r.id]?.weight || 10,
        minDailyLessons: busyLimitMap[r.id]?.minDaily ?? null,
        maxDailyLessons: busyLimitMap[r.id]?.maxDaily ?? null,
      }));
    if (foundClasses.length > 0) classes = foundClasses;

    // Nauczyciele
    const foundTeachers = inst.resources
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
    if (foundTeachers.length > 0) teachers = foundTeachers;

    // Sale
    const foundRooms = inst.resources
      .filter((r) => r.resource_type_ref.toLowerCase().includes("room"))
      .map((r) => ({
        id: r.id,
        name: r.name || r.id,
        type: ((r.name || "").toLowerCase().includes("gym") ? "gym" : "general") as "general" | "gym" | "lab" | "it",
        unavailableTimes: unavailMap[r.id]?.times || [],
        unavailableSoft: unavailMap[r.id]?.isSoft || false,
        unavailableWeight: unavailMap[r.id]?.weight || 10,
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
      { id: "Kaminski_T", name: "Tomasz Kamiński", subject: "Historia", workload: 12, maxDaysPerWeek: 3 },
      { id: "Lewandowska_E", name: "Ewa Lewandowska", subject: "Biologia", workload: 12, maxDaysPerWeek: 3 },
      { id: "Zielinski_K", name: "Krzysztof Zieliński", subject: "Fizyka", workload: 12, maxDaysPerWeek: 3 },
      { id: "Szymanska_B", name: "Beata Szymańska", subject: "Chemia", workload: 12, maxDailyLessons: 5 },
      { id: "Dabrowski_R", name: "Robert Dąbrowski", subject: "Informatyka", workload: 12, maxGapsPerWeek: 0 },
      { id: "Kozlowska_M", name: "Magdalena Kozłowska", subject: "Geografia", workload: 12, maxGapsPerWeek: 1 },
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
    const match = newClassName.match(/^(\d+)/);
    const grade = match ? parseInt(match[1], 10) : newClassGrade;
    classes = [...classes, { id: cleanId, name: newClassName.trim(), grade: grade }];
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

  let totalTeacherWorkload = $derived(
    teachers.reduce((sum, t) => sum + (Number(t.workload) || 0), 0)
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
          required: !newOrderIsSoft,
          weight: newOrderWeight,
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

    <!-- STEP 2: ODDZIAŁY KLASOWE I KADRA NAUCZYCIELSKA -->
    {:else if currentStep === 2}
      <div class="flex flex-col gap-5">
        <!-- Sub-tabs bar -->
        <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
          <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
            <button
              type="button"
              onclick={() => (step2ActiveTab = "classes")}
              class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
                step2ActiveTab === "classes"
                  ? "bg-white text-blue-700 shadow-xs"
                  : "text-slate-600 hover:text-slate-900"
              }`}
            >
              <span>👥 Klasy i roczniki</span>
              <span class="px-1.5 py-0.5 rounded-full text-[10px] {step2ActiveTab === 'classes' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
                {classes.length}
              </span>
            </button>
            <button
              type="button"
              onclick={() => (step2ActiveTab = "teachers")}
              class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
                step2ActiveTab === "teachers"
                  ? "bg-white text-blue-700 shadow-xs"
                  : "text-slate-600 hover:text-slate-900"
              }`}
            >
              <span>👨‍🏫 Kadra i pensum</span>
              <span class="px-1.5 py-0.5 rounded-full text-[10px] {step2ActiveTab === 'teachers' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
                {teachers.length}
              </span>
            </button>
          </div>

          <div class="text-xs text-slate-500 px-3 py-1">
            {#if step2ActiveTab === "classes"}
              Zarządzaj poziomami klas, powielaj siatki godzin i ustalaj ograniczenia dzienne.
            {:else}
              Ustalaj pensum dydaktyczne (etat) i monitoruj zbilansowanie godzin kadry.
            {/if}
          </div>
        </div>

        {#if step2ActiveTab === "classes"}
          <!-- ZAKŁADKA KLAS I ROCZNIKÓW -->
          <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
            <!-- Header & Grade filter -->
            <div class="flex flex-col md:flex-row md:items-center justify-between gap-3 pb-4 border-b border-slate-100">
              <div>
                <h3 class="text-sm font-bold text-slate-900">Oddziały klasowe według roczników</h3>
                <p class="text-xs text-slate-500 mt-0.5">
                  Możesz duplikować klasy (np. 4A &rarr; 4B) lub kopiować cały program nauczania między oddziałami.
                </p>
              </div>

              <!-- Filtry roczników -->
              <div class="flex flex-wrap items-center gap-1.5">
                <span class="text-xs font-semibold text-slate-500 mr-1">Rocznik:</span>
                <button
                  type="button"
                  onclick={() => (selectedGradeFilter = null)}
                  class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
                    selectedGradeFilter === null
                      ? "bg-blue-600 text-white border-blue-600"
                      : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                  }`}
                >
                  Wszystkie ({classes.length})
                </button>
                {#each availableGrades as gr}
                  {@const count = classes.filter((c) => getClassGrade(c) === gr).length}
                  <button
                    type="button"
                    onclick={() => (selectedGradeFilter = gr)}
                    class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
                      selectedGradeFilter === gr
                        ? "bg-blue-600 text-white border-blue-600"
                        : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                    }`}
                  >
                    Rocznik {gr} ({count})
                  </button>
                {/each}
              </div>
            </div>

            <!-- Formularz dodawania klasy -->
            <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
              <input
                type="text"
                bind:value={newClassName}
                placeholder="Nazwa klasy (np. 4C Mat-Inf lub 1A)"
                onkeydown={(e) => e.key === "Enter" && addClass()}
                class="flex-1 min-w-[200px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
              />
              <div class="flex items-center gap-1.5">
                <span class="text-xs text-slate-500">Rocznik:</span>
                <select
                  bind:value={newClassGrade}
                  class="px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
                >
                  <option value={1}>Poziom 1</option>
                  <option value={2}>Poziom 2</option>
                  <option value={3}>Poziom 3</option>
                  <option value={4}>Poziom 4</option>
                  <option value={5}>Poziom 5</option>
                </select>
              </div>
              <button
                type="button"
                onclick={addClass}
                class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
              >
                + Dodaj klasę
              </button>
            </div>

            <!-- Siatka klas -->
            <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
              {#each filteredClasses as cls}
                {@const weeklyHours = getClassWeeklyHours(cls.id)}
                {@const clsLessons = getClassLessons(cls.id)}
                {@const grade = getClassGrade(cls)}
                <div class="p-4 bg-slate-50/90 rounded-2xl border border-slate-200 flex flex-col justify-between gap-3 shadow-2xs hover:border-slate-300 transition-colors">
                  <div>
                    <div class="flex items-start justify-between gap-2">
                      <div class="flex items-center gap-2">
                        <span class="w-8 h-8 rounded-xl bg-blue-100 text-blue-800 font-black text-xs flex items-center justify-center">
                          {grade}
                        </span>
                        <div>
                          <div class="flex items-center gap-2">
                            <span class="font-bold text-slate-900 text-xs">👥 {cls.name}</span>
                            <span class="text-[10px] px-1.5 py-0.2 bg-slate-200/80 text-slate-700 font-semibold rounded">
                              Rocznik {grade}
                            </span>
                          </div>
                          <span class="text-[11px] text-slate-500 block mt-0.5">
                            Łącznie: <strong>{weeklyHours} godz./tydz.</strong> ({clsLessons.length} przedmiotów/wpisów)
                          </span>
                        </div>
                      </div>

                      <div class="flex items-center gap-1">
                        <button
                          type="button"
                          onclick={() => openAvailabilityModal(cls.id, cls.name, "class")}
                          class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                            (cls.unavailableTimes?.length || 0) > 0 || cls.minDailyLessons || cls.maxDailyLessons
                              ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                              : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                          }`}
                          title="Ustal dostępność klasy i ramy lekcyjne"
                        >
                          <span>🗓️</span>
                          <span>{cls.unavailableTimes?.length ? `${cls.unavailableTimes.length} wykl.` : "Dostępność"}</span>
                        </button>
                        <button
                          type="button"
                          onclick={() => removeClass(cls.id)}
                          class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                          title="Usuń klasę"
                        >
                          ✕
                        </button>
                      </div>
                    </div>

                    <!-- Mini podgląd siatki przedmiotów klasy -->
                    {#if clsLessons.length > 0}
                      <div class="mt-3 flex flex-wrap gap-1">
                        {#each clsLessons.slice(0, 5) as l}
                          <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-700 rounded-md">
                            {l.subject}: {l.weeklyCount}h
                          </span>
                        {/each}
                        {#if clsLessons.length > 5}
                          <span class="text-[10px] px-1.5 py-0.5 text-slate-400 font-medium">
                            +{clsLessons.length - 5} więcej
                          </span>
                        {/if}
                      </div>
                    {:else}
                      <p class="mt-2 text-[11px] text-slate-400 italic">
                        Brak przypisanych lekcji. Użyj przycisku poniżej, aby skopiować siatkę z innej klasy.
                      </p>
                    {/if}
                  </div>

                  <!-- Akcje dla klasy: duplikowanie -->
                  <div class="pt-2 border-t border-slate-200/60 flex items-center justify-between gap-2 text-xs">
                    <button
                      type="button"
                      onclick={() => (duplicateModalTarget = cls)}
                      class="px-2.5 py-1 bg-white hover:bg-blue-50 text-blue-700 border border-slate-200 hover:border-blue-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1.5 text-[11px]"
                      title="Skopiuj wszystkie lekcje z tej klasy do innych klas tego samego lub innych roczników"
                    >
                      <span>📋 Duplikuj siatkę do...</span>
                    </button>

                    <button
                      type="button"
                      onclick={() => duplicateClass(cls.id)}
                      class="px-2.5 py-1 bg-white hover:bg-slate-100 text-slate-700 border border-slate-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1 text-[11px]"
                      title="Utwórz nowy oddział na bazie tej klasy z identyczną siatką zajęć"
                    >
                      <span>➕ Nowy oddział</span>
                    </button>
                  </div>
                </div>
              {/each}
            </div>
          </div>
        {:else}
          <!-- ZAKŁADKA KADRY I PENSUM -->
          <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
            <!-- Globalny pasek bilansu pensum -->
            <div class="p-4 bg-gradient-to-r from-blue-50/80 to-indigo-50/80 border border-blue-200/80 rounded-2xl flex flex-wrap items-center justify-between gap-4">
              <div class="flex items-center gap-4">
                <div class="w-10 h-10 rounded-xl bg-blue-600 text-white flex items-center justify-center text-lg font-black shadow-xs">
                  ⚖️
                </div>
                <div>
                  <h4 class="text-xs font-bold uppercase tracking-wider text-blue-950">
                    Bilans pensum i etaty kadry pedagogicznej
                  </h4>
                  <p class="text-xs text-blue-800 mt-0.5">
                    Możesz bezpośrednio edytować pensum każdego nauczyciela i śledzić jego obciążenie.
                  </p>
                </div>
              </div>

              <div class="flex items-center gap-6 text-xs">
                <div>
                  <span class="text-slate-500 block text-[11px]">Łączne pensum:</span>
                  <span class="font-black text-slate-900 text-sm">{totalTeacherWorkload} godz.</span>
                </div>
                <div>
                  <span class="text-slate-500 block text-[11px]">Przydzielone lekcje:</span>
                  <span class="font-black text-blue-700 text-sm">{totalHoursAssigned} godz.</span>
                </div>
                <div>
                  <span class="text-slate-500 block text-[11px]">Bilans:</span>
                  {#if totalHoursAssigned === totalTeacherWorkload}
                    <span class="font-black text-emerald-600 text-sm">Zrównoważony (0h)</span>
                  {:else if totalHoursAssigned > totalTeacherWorkload}
                    <span class="font-black text-rose-600 text-sm">+{totalHoursAssigned - totalTeacherWorkload}h (nadgodziny)</span>
                  {:else}
                    <span class="font-black text-amber-600 text-sm">-{totalTeacherWorkload - totalHoursAssigned}h (wakat)</span>
                  {/if}
                </div>
              </div>
            </div>

            <!-- Formularz dodawania nauczyciela z pensum -->
            <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3.5 rounded-xl border border-slate-200/70">
              <input
                type="text"
                bind:value={newTeacherName}
                placeholder="Imię i nazwisko (np. Anna Nowak)"
                class="flex-1 min-w-[180px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              />
              <input
                type="text"
                bind:value={newTeacherSubject}
                placeholder="Nauczany przedmiot (np. Matematyka)"
                class="flex-1 min-w-[160px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
              />
              <div class="flex items-center gap-1.5">
                <span class="text-xs font-semibold text-slate-600">Pensum:</span>
                <input
                  type="number"
                  min="1"
                  max="60"
                  bind:value={newTeacherWorkload}
                  class="w-16 px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500 text-center font-bold"
                />
                <span class="text-xs text-slate-500">h</span>
              </div>
              <button
                type="button"
                onclick={addTeacher}
                class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
              >
                + Dodaj nauczyciela
              </button>
            </div>

            <!-- Siatka nauczycieli z edycją pensum i podglądem klas -->
            <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
              {#each teachers as t}
                {@const assigned = getTeacherAssignedHours(t.id)}
                {@const tClasses = getTeacherClasses(t.id)}
                {@const workloadDiff = assigned - t.workload}
                <div class="p-4 bg-slate-50/90 rounded-2xl border border-slate-200 flex flex-col justify-between gap-3 shadow-2xs hover:border-slate-300 transition-colors">
                  <div>
                    <div class="flex items-start justify-between gap-2">
                      <div>
                        <div class="flex items-center gap-2">
                          <span class="font-bold text-slate-900 text-xs">👨‍🏫 {t.name}</span>
                          <span class="text-[10px] px-2 py-0.5 bg-blue-50 text-blue-700 border border-blue-200 font-semibold rounded-full">
                            {t.subject}
                          </span>
                        </div>

                        <!-- Edycja pensum bezpośrednio w wierszu -->
                        <div class="flex items-center gap-2 mt-2">
                          <span class="text-[11px] text-slate-500">Pensum:</span>
                          <input
                            type="number"
                            min="1"
                            max="60"
                            value={t.workload}
                            oninput={(e) => updateTeacherWorkload(t.id, parseInt(e.currentTarget.value, 10))}
                            class="w-14 px-2 py-0.5 text-xs bg-white border border-slate-300 rounded text-slate-900 font-bold text-center focus:outline-none focus:ring-1 focus:ring-blue-500"
                            title="Edytuj wymiar pensum nauczyciela"
                          />
                          <span class="text-xs text-slate-500">h / tydzień</span>

                          <!-- Wskaźnik zbilansowania -->
                          <span class={`text-[10px] px-2 py-0.5 rounded-full font-bold ${
                            workloadDiff === 0
                              ? "bg-emerald-50 text-emerald-700 border border-emerald-200"
                              : workloadDiff > 0
                              ? "bg-rose-50 text-rose-700 border border-rose-200"
                              : "bg-amber-50 text-amber-700 border border-amber-200"
                          }`}>
                            {assigned} / {t.workload}h ({workloadDiff === 0 ? "100%" : workloadDiff > 0 ? `+${workloadDiff}h` : `${workloadDiff}h`})
                          </span>
                        </div>
                      </div>

                      <div class="flex items-center gap-1">
                        <button
                          type="button"
                          onclick={() => openAvailabilityModal(t.id, t.name, "teacher")}
                          class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                            (t.unavailableTimes?.length || 0) > 0 || t.maxDaysPerWeek || t.maxDailyLessons || (t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined)
                              ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                              : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                          }`}
                          title="Ustal dostępność nauczyciela, dni pracy i okienka"
                        >
                          <span>🗓️</span>
                          <span>{t.unavailableTimes?.length ? `${t.unavailableTimes.length} wykl.` : "Dostępność"}</span>
                        </button>
                        <button
                          type="button"
                          onclick={() => removeTeacher(t.id)}
                          class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                          title="Usuń nauczyciela"
                        >
                          ✕
                        </button>
                      </div>
                    </div>

                    <!-- Podgląd przypisanych klas (Requirement 4) -->
                    <div class="mt-3">
                      <span class="text-[10px] font-bold uppercase tracking-wider text-slate-400 block mb-1">
                        Przydzielone oddziały ({tClasses.length}):
                      </span>
                      {#if tClasses.length > 0}
                        <div class="flex flex-wrap gap-1">
                          {#each tClasses as tc}
                            <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-800 rounded-md font-medium">
                              {tc.className} ({tc.subject}: {tc.hours}h)
                            </span>
                          {/each}
                        </div>
                      {:else}
                        <span class="text-[11px] text-slate-400 italic">
                          Brak przypisanych klas (0 godzin).
                        </span>
                      {/if}
                    </div>
                  </div>
                </div>
              {/each}
            </div>
          </div>
        {/if}
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
              <div class="flex items-center gap-1.5">
                <button
                  type="button"
                  onclick={() => openAvailabilityModal(room.id, room.name, "room")}
                  class={`text-[10px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                    (room.unavailableTimes?.length || 0) > 0
                      ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                      : "bg-white text-slate-500 border-slate-200 hover:border-slate-300"
                  }`}
                  title="Ustal dostępność sali w tygodniu (AvoidUnavailableTimesConstraint)"
                >
                  <span>🗓️</span>
                  <span>{room.unavailableTimes?.length ? `${room.unavailableTimes.length} wykl.` : "Dostępność"}</span>
                </button>
                <button
                  type="button"
                  onclick={() => removeRoom(room.id)}
                  class="text-slate-400 hover:text-rose-600 text-xs px-2 py-0.5 rounded cursor-pointer"
                  title="Usuń salę"
                >
                  ✕
                </button>
              </div>
            </div>
          {/each}
        </div>
      </div>

    <!-- STEP 4: SIATKA LEKCJI (PRZYDZIAŁ) -->
    {:else if currentStep === 4}
      <div class="flex flex-col gap-5">
        <!-- Sub-tabs bar -->
        <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
          <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
            <button
              type="button"
              onclick={() => (step4ActiveTab = "lessons")}
              class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
                step4ActiveTab === "lessons"
                  ? "bg-white text-blue-700 shadow-xs"
                  : "text-slate-600 hover:text-slate-900"
              }`}
            >
              <span>📋 Wszystkie lekcje</span>
              <span class="px-1.5 py-0.5 rounded-full text-[10px] {step4ActiveTab === 'lessons' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
                {lessons.length}
              </span>
            </button>
            <button
              type="button"
              onclick={() => (step4ActiveTab = "byClass")}
              class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
                step4ActiveTab === "byClass"
                  ? "bg-white text-blue-700 shadow-xs"
                  : "text-slate-600 hover:text-slate-900"
              }`}
            >
              <span>🏫 Widok klas i roczników</span>
            </button>
            <button
              type="button"
              onclick={() => {
                step4ActiveTab = "byTeacher";
                if (!selectedTeacherViewId && teachers[0]) {
                  selectedTeacherViewId = teachers[0].id;
                }
              }}
              class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
                step4ActiveTab === "byTeacher"
                  ? "bg-white text-blue-700 shadow-xs"
                  : "text-slate-600 hover:text-slate-900"
              }`}
            >
              <span>👨‍🏫 Widok nauczycieli</span>
            </button>
          </div>

          <div class="text-xs text-slate-500 px-3 py-1">
            {#if step4ActiveTab === "lessons"}
              Łącznie zaplanowano: <strong>{totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
            {:else if step4ActiveTab === "byClass"}
              Przeglądaj plany poszczególnych klas, powielaj siatki i przypisuj przedmioty do roczników.
            {:else}
              Podgląd klas każdego nauczyciela oraz weryfikacja zasady spójności przydziału.
            {/if}
          </div>
        </div>

        {#if step4ActiveTab === "lessons"}
          <!-- ZAKŁADKA 1: WSZYSTKIE LEKCJE -->
          <div class="flex flex-col gap-6">
            <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
              <div>
                <h3 class="text-sm font-bold text-slate-900">Siatka godzin i przydział lekcji</h3>
                <p class="text-xs text-slate-500 mt-0.5">
                  Łącznie zaplanowano: <strong>{totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
                </p>
              </div>

              <div class="flex flex-wrap items-center gap-2">
                <button
                  type="button"
                  onclick={() => (isLinkedModalOpen = true)}
                  class="px-3.5 py-2 bg-purple-50 hover:bg-purple-100 text-purple-900 border border-purple-200 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5 shadow-xs"
                  title="Zarządzaj lekcjami równoległymi i jednoczesnymi (np. języki, WF)"
                >
                  <span>🔗 Lekcje równoległe ({linkedGroups.length})</span>
                </button>

                <button
                  type="button"
                  onclick={generateDefaultCurriculum}
                  class="px-3.5 py-2 bg-blue-50 hover:bg-blue-100 text-blue-900 border border-blue-200 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5 shadow-xs"
                >
                  <span>⚡ Zrównoważ i wygeneruj pełną siatkę dla klas</span>
                </button>
              </div>
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
                <span class="text-slate-400">Możesz ustalać preferencje godzinowe oraz usuwać wpisy</span>
              </div>
              <div class="divide-y divide-slate-100 max-h-96 overflow-y-auto">
                {#each lessons as l}
                  {@const cls = classes.find((c) => c.id === l.classId)}
                  {@const t = teachers.find((tch) => tch.id === l.teacherId)}
                  {@const r = rooms.find((rm) => rm.id === l.roomId)}
                  {@const linkedGroup = linkedGroups.find((g) => g.lessonIds.includes(l.id))}
                  <div class="p-3 hover:bg-slate-50/80 flex items-center justify-between text-xs transition-colors">
                    <div class="flex items-center gap-3">
                      <span class="w-8 h-8 rounded-lg bg-blue-50 text-blue-700 flex items-center justify-center font-bold text-xs shrink-0">
                        📚
                      </span>
                      <div>
                        <div class="flex items-center gap-2">
                          <span class="font-bold text-slate-900">{l.subject}</span>
                          {#if linkedGroup}
                            <span class="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-purple-50 text-purple-700 border border-purple-200">
                              🔗 {linkedGroup.name}
                            </span>
                          {/if}
                        </div>
                        <div class="text-[11px] text-slate-500 flex items-center gap-2 mt-0.5">
                          <span>Klasa: <strong>{cls ? cls.name : l.classId}</strong></span>
                          <span>•</span>
                          <span>Nauczyciel: <strong>{t ? t.name : l.teacherId}</strong></span>
                          <span>•</span>
                          <span>Sala: {r ? r.name : "Dowolna"}</span>
                        </div>
                      </div>
                    </div>

                    <div class="flex items-center gap-2.5">
                      <!-- Preference button (Phase 3: PreferTimesConstraint) -->
                      <button
                        type="button"
                        onclick={() => openLessonPrefModal(l)}
                        class="px-2.5 py-1 rounded-lg border text-[11px] font-semibold transition-all cursor-pointer flex items-center gap-1 {l.timePreference === 'morning' ? 'bg-amber-50 text-amber-800 border-amber-200' : l.timePreference === 'afternoon' ? 'bg-indigo-50 text-indigo-800 border-indigo-200' : l.timePreference === 'custom' ? 'bg-emerald-50 text-emerald-800 border-emerald-200' : 'bg-white text-slate-600 border-slate-200 hover:border-slate-300'}"
                        title="Ustal preferowane godziny lub porę dnia dla tej lekcji"
                      >
                        {#if l.timePreference === 'morning'}
                          <span>🌅</span>
                          <span>Rano (1–4)</span>
                        {:else if l.timePreference === 'afternoon'}
                          <span>🌇</span>
                          <span>Popołudnie (5+)</span>
                        {:else if l.timePreference === 'custom'}
                          <span>⭐</span>
                          <span>{l.preferredTimes?.length || 0} slotów</span>
                        {:else}
                          <span>🕒</span>
                          <span class="text-slate-500">Dowolna pora</span>
                        {/if}
                      </button>

                      <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-semibold">
                        {l.weeklyCount} godz./tydz.
                      </span>
                      <button
                        type="button"
                        onclick={() => removeLesson(l.id)}
                        class="text-slate-400 hover:text-rose-600 px-2 py-1 rounded cursor-pointer"
                        title="Usuń lekcję"
                      >
                        ✕
                      </button>
                    </div>
                  </div>
                {/each}
              </div>
            </div>
          </div>
        {:else if step4ActiveTab === "byClass"}
          <!-- ZAKŁADKA 2: WIDOK KLAS I ROCZNIKÓW -->
          <div class="flex flex-col gap-6">
            <!-- Pasek wyboru rocznika i oddziału -->
            <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-4">
              <div class="flex flex-col md:flex-row md:items-center justify-between gap-3 pb-3 border-b border-slate-100">
                <div class="flex items-center gap-3">
                  <span class="text-xs font-bold uppercase tracking-wider text-slate-500">Wybierz rocznik:</span>
                  <div class="flex flex-wrap items-center gap-1.5">
                    {#each availableGrades as gr}
                      <button
                        type="button"
                        onclick={() => selectGradeCurriculum(gr)}
                        class={`px-3 py-1.5 text-xs rounded-xl font-bold transition-all cursor-pointer border ${
                          selectedGradeCurriculum === gr
                            ? "bg-blue-600 text-white border-blue-600 shadow-xs"
                            : "bg-slate-50 text-slate-600 border-slate-200 hover:bg-slate-100"
                        }`}
                      >
                        Rocznik {gr}
                      </button>
                    {/each}
                  </div>
                </div>

                <!-- Oddziały w wybranym roczniku -->
                <div class="flex items-center gap-2">
                  <span class="text-xs text-slate-400">Oddział:</span>
                  <div class="flex flex-wrap gap-1">
                    {#each currentGradeClasses as cls}
                      <button
                        type="button"
                        onclick={() => (selectedClassViewId = cls.id)}
                        class={`px-3 py-1 text-xs rounded-lg font-bold transition-all cursor-pointer border ${
                          selectedClassViewId === cls.id
                            ? "bg-slate-900 text-white border-slate-900 shadow-xs"
                            : "bg-white text-slate-700 border-slate-200 hover:border-slate-300"
                        }`}
                      >
                        {cls.name} ({getClassWeeklyHours(cls.id)}h)
                      </button>
                    {/each}
                  </div>
                </div>
              </div>

              <!-- Narzędzie: Grupowe przypisanie przedmiotu do całego rocznika -->
              <div class="p-4 bg-gradient-to-r from-blue-50/70 to-indigo-50/70 border border-blue-200 rounded-xl flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
                <div>
                  <h4 class="text-xs font-bold uppercase tracking-wider text-blue-950 flex items-center gap-1.5">
                    <span>⚡ Grupowe przypisanie do wszystkich klas Rocznika {selectedGradeCurriculum}</span>
                  </h4>
                  <p class="text-[11px] text-blue-800 mt-0.5">
                    Wszystkie klasy rocznika {selectedGradeCurriculum} otrzymają ten sam wymiar godzin z zachowaniem stałego nauczyciela.
                  </p>
                </div>

                <div class="flex flex-wrap items-center gap-2 text-xs">
                  <input
                    type="text"
                    bind:value={batchGradeSubject}
                    placeholder="Przedmiot (np. Matematyka)"
                    class="px-2.5 py-1.5 bg-white border border-blue-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500 w-36"
                  />
                  <div class="flex items-center gap-1 bg-white px-2 py-1 border border-blue-200 rounded-lg">
                    <input
                      type="number"
                      min="1"
                      max="10"
                      bind:value={batchGradeHours}
                      class="w-10 text-center font-bold text-slate-900 focus:outline-none"
                    />
                    <span class="text-[11px] text-slate-500">h/tydz.</span>
                  </div>
                  <label class="flex items-center gap-1.5 text-[11px] font-semibold text-blue-950 cursor-pointer bg-white px-2.5 py-1.5 border border-blue-200 rounded-lg">
                    <input type="checkbox" bind:checked={batchGradeIsBlock} class="rounded accent-blue-600" />
                    Blok 2h
                  </label>
                  <button
                    type="button"
                    onclick={() => addSubjectToGrade(selectedGradeCurriculum, batchGradeSubject, batchGradeHours, batchGradeIsBlock)}
                    class="px-3.5 py-1.5 bg-blue-600 hover:bg-blue-700 text-white font-bold rounded-lg cursor-pointer transition-all shadow-xs shrink-0"
                  >
                    Zastosuj dla Rocznika {selectedGradeCurriculum}
                  </button>
                </div>
              </div>
            </div>

            <!-- Karta aktywnej klasy i jej siatka -->
            {#if activeViewClass}
              {@const classLessons = getClassLessons(activeViewClass.id)}
              <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden flex flex-col">
                <div class="p-5 border-b border-slate-100 flex flex-col sm:flex-row sm:items-center justify-between gap-3 bg-slate-50/60">
                  <div class="flex items-center gap-3">
                    <span class="w-10 h-10 rounded-xl bg-blue-600 text-white font-black text-sm flex items-center justify-center shadow-xs">
                      {getClassGrade(activeViewClass)}
                    </span>
                    <div>
                      <div class="flex items-center gap-2">
                        <h4 class="text-sm font-bold text-slate-900">{activeViewClass.name}</h4>
                        <span class="text-[10px] px-2 py-0.5 bg-blue-100 text-blue-800 font-bold rounded-full">
                          Rocznik {getClassGrade(activeViewClass)}
                        </span>
                      </div>
                      <span class="text-xs text-slate-500 mt-0.5 block">
                        Wymiar godzin: <strong>{getClassWeeklyHours(activeViewClass.id)} godz./tydz.</strong> ({classLessons.length} przedmiotów)
                      </span>
                    </div>
                  </div>

                  <!-- Przycisk duplikowania siatki tej klasy -->
                  <div class="flex items-center gap-2">
                    <button
                      type="button"
                      onclick={() => (duplicateModalTarget = activeViewClass)}
                      class="px-3.5 py-2 bg-white hover:bg-blue-50 text-blue-700 border border-blue-200 text-xs font-bold rounded-xl cursor-pointer transition-all flex items-center gap-1.5 shadow-xs"
                      title="Powiel siatkę tej klasy dla innych klas rocznika"
                    >
                      <span>📋 Duplikuj siatkę do innych klas ({getClassGrade(activeViewClass)})...</span>
                    </button>
                  </div>
                </div>

                <!-- Tabela lekcji danej klasy -->
                {#if classLessons.length === 0}
                  <div class="p-8 text-center text-xs text-slate-400">
                    Brak zaplanowanych zajęć dla tej klasy. Skorzystaj z narzędzia powyżej lub skopiuj plan z innej klasy.
                  </div>
                {:else}
                  <div class="divide-y divide-slate-100 max-h-96 overflow-y-auto">
                    {#each classLessons as l}
                      {@const t = teachers.find((tch) => tch.id === l.teacherId)}
                      {@const r = rooms.find((rm) => rm.id === l.roomId)}
                      <div class="p-3.5 hover:bg-slate-50/80 flex items-center justify-between text-xs transition-colors">
                        <div class="flex items-center gap-3">
                          <span class="w-7 h-7 rounded-lg bg-slate-100 text-slate-700 flex items-center justify-center font-bold text-xs shrink-0">
                            📖
                          </span>
                          <div>
                            <span class="font-bold text-slate-900">{l.subject}</span>
                            <div class="text-[11px] text-slate-500 flex items-center gap-2 mt-0.5">
                              <span>Nauczyciel: <strong>{t ? t.name : l.teacherId}</strong></span>
                              <span>•</span>
                              <span>Sala: {r ? r.name : "Dowolna"}</span>
                            </div>
                          </div>
                        </div>

                        <div class="flex items-center gap-2.5">
                          <button
                            type="button"
                            onclick={() => openLessonPrefModal(l)}
                            class="px-2.5 py-1 rounded-lg border text-[11px] font-semibold transition-all cursor-pointer flex items-center gap-1 {l.timePreference === 'morning' ? 'bg-amber-50 text-amber-800 border-amber-200' : l.timePreference === 'afternoon' ? 'bg-indigo-50 text-indigo-800 border-indigo-200' : l.timePreference === 'custom' ? 'bg-emerald-50 text-emerald-800 border-emerald-200' : 'bg-white text-slate-600 border-slate-200 hover:border-slate-300'}"
                          >
                            <span>🕒</span>
                            <span>{l.timePreference || "Dowolna"}</span>
                          </button>
                          <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-bold">
                            {l.weeklyCount}h/tydz.
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
                {/if}
              </div>
            {/if}
          </div>
        {:else}
          <!-- ZAKŁADKA 3: WIDOK NAUCZYCIELI I SPÓJNOŚCI -->
          <div class="flex flex-col gap-6">
            <!-- Wybór nauczyciela -->
            <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
              <span class="text-xs font-bold uppercase tracking-wider text-slate-500">Wybierz pedagoga:</span>
              <div class="flex flex-wrap gap-1.5 max-h-36 overflow-y-auto">
                {#each teachers as t}
                  {@const assigned = getTeacherAssignedHours(t.id)}
                  <button
                    type="button"
                    onclick={() => (selectedTeacherViewId = t.id)}
                    class={`px-3 py-1.5 text-xs rounded-xl font-bold transition-all cursor-pointer border flex items-center gap-1.5 ${
                      (selectedTeacherViewId || teachers[0]?.id) === t.id
                        ? "bg-slate-900 text-white border-slate-900 shadow-xs"
                        : "bg-white text-slate-700 border-slate-200 hover:border-slate-300"
                    }`}
                  >
                    <span>👨‍🏫 {t.name}</span>
                    <span class="text-[10px] px-1.5 py-0.2 rounded font-semibold {assigned === t.workload ? 'bg-emerald-100 text-emerald-800' : 'bg-slate-100 text-slate-700'}">
                      {assigned}/{t.workload}h
                    </span>
                  </button>
                {/each}
              </div>
            </div>

            <!-- Karta wybranego nauczyciela -->
            {#if activeViewTeacher}
              {@const teacherLessons = lessons.filter((l) => l.teacherId === activeViewTeacher.id)}
              {@const tClasses = getTeacherClasses(activeViewTeacher.id)}
              {@const assigned = getTeacherAssignedHours(activeViewTeacher.id)}
              <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden flex flex-col">
                <div class="p-5 border-b border-slate-100 bg-slate-50/60 flex flex-col md:flex-row md:items-center justify-between gap-4">
                  <div>
                    <div class="flex items-center gap-2">
                      <h4 class="text-sm font-bold text-slate-900">👨‍🏫 {activeViewTeacher.name}</h4>
                      <span class="text-[10px] px-2 py-0.5 bg-blue-100 text-blue-800 font-bold rounded-full">
                        {activeViewTeacher.subject}
                      </span>
                    </div>
                    <span class="text-xs text-slate-500 mt-0.5 block">
                      Pensum: <strong>{activeViewTeacher.workload}h</strong> • Przydzielono: <strong>{assigned}h</strong>
                      ({assigned === activeViewTeacher.workload ? "100% zbilansowane" : assigned > activeViewTeacher.workload ? `+${assigned - activeViewTeacher.workload}h nadgodziny` : `${assigned - activeViewTeacher.workload}h wakat`})
                    </span>
                  </div>

                  <!-- Zasada spójności przydziału -->
                  <div class="px-3.5 py-2 bg-emerald-50 border border-emerald-200 rounded-xl flex items-center gap-2 text-xs text-emerald-900 font-medium">
                    <span>✅</span>
                    <span><strong>Zasada spójności:</strong> Stały nauczyciel dla klas: {tClasses.length > 0 ? tClasses.map((c) => c.className).join(", ") : "brak przydziału"}</span>
                  </div>
                </div>

                <!-- Tabela zajęć nauczyciela -->
                {#if teacherLessons.length === 0}
                  <div class="p-8 text-center text-xs text-slate-400">
                    Brak przypisanych lekcji do tego nauczyciela.
                  </div>
                {:else}
                  <div class="divide-y divide-slate-100 max-h-96 overflow-y-auto">
                    {#each teacherLessons as l}
                      {@const cls = classes.find((c) => c.id === l.classId)}
                      {@const r = rooms.find((rm) => rm.id === l.roomId)}
                      <div class="p-3.5 hover:bg-slate-50/80 flex items-center justify-between text-xs transition-colors">
                        <div class="flex items-center gap-3">
                          <span class="w-7 h-7 rounded-lg bg-blue-50 text-blue-700 flex items-center justify-center font-bold text-xs shrink-0">
                            👥
                          </span>
                          <div>
                            <span class="font-bold text-slate-900">Klasa: {cls ? cls.name : l.classId}</span>
                            <div class="text-[11px] text-slate-500 flex items-center gap-2 mt-0.5">
                              <span>Przedmiot: <strong>{l.subject}</strong></span>
                              <span>•</span>
                              <span>Sala: {r ? r.name : "Dowolna"}</span>
                            </div>
                          </div>
                        </div>

                        <div class="flex items-center gap-2.5">
                          <button
                            type="button"
                            onclick={() => openLessonPrefModal(l)}
                            class="px-2.5 py-1 rounded-lg border text-[11px] font-semibold transition-all cursor-pointer flex items-center gap-1 {l.timePreference === 'morning' ? 'bg-amber-50 text-amber-800 border-amber-200' : l.timePreference === 'afternoon' ? 'bg-indigo-50 text-indigo-800 border-indigo-200' : l.timePreference === 'custom' ? 'bg-emerald-50 text-emerald-800 border-emerald-200' : 'bg-white text-slate-600 border-slate-200 hover:border-slate-300'}"
                          >
                            <span>🕒</span>
                            <span>{l.timePreference || "Dowolna"}</span>
                          </button>
                          <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-bold">
                            {l.weeklyCount}h/tydz.
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
                {/if}
              </div>
            {/if}
          </div>
        {/if}
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

          <!-- Reguła 3: Okienka uczniów (LimitIdleTimesConstraint) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">☕</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Brak okienek dla uczniów (LimitIdleTimesConstraint)</span>
                    <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
                      !ruleStudentNoGapsSoft ? "bg-rose-50 text-rose-700 border-rose-200" : "bg-amber-50 text-amber-700 border-amber-200"
                    }`}>
                      {!ruleStudentNoGapsSoft ? "Wymóg HARD" : `SOFT (waga ${studentGapsWeight})`}
                    </span>
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Uczniowie mają zajęcia ciągiem od pierwszej do ostatniej lekcji, bez pustych przerw w środku dnia.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleStudentNoGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleStudentNoGaps}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <div class="flex items-center gap-4">
                  <span class="text-slate-500 text-[11px] font-medium">Rygor ograniczenia:</span>
                  <label class="flex items-center gap-1.5 cursor-pointer">
                    <input type="radio" name="studentGapsRigor" checked={!ruleStudentNoGapsSoft} onchange={() => (ruleStudentNoGapsSoft = false)} class="w-3.5 h-3.5 accent-rose-600" />
                    <span class="font-semibold text-rose-700 text-[11px]">HARD (Zakaz bezwzględny)</span>
                  </label>
                  <label class="flex items-center gap-1.5 cursor-pointer">
                    <input type="radio" name="studentGapsRigor" checked={ruleStudentNoGapsSoft} onchange={() => (ruleStudentNoGapsSoft = true)} class="w-3.5 h-3.5 accent-amber-600" />
                    <span class="font-semibold text-amber-700 text-[11px]">SOFT (Preferencja z wagą)</span>
                  </label>
                </div>
                {#if ruleStudentNoGapsSoft}
                  <div class="flex items-center gap-1.5">
                    <span class="text-slate-500 text-[11px]">Waga kary:</span>
                    <input type="number" min="1" max="50" bind:value={studentGapsWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                  </div>
                {/if}
              </div>
            {/if}
          </div>

          <!-- Reguła 4: Okienka nauczycieli (LimitIdleTimesConstraint) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">👨‍🏫</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Minimalizacja okienek dla nauczycieli (LimitIdleTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
                      Maks. {teacherMaxGapsLimit} {teacherMaxGapsLimit === 1 ? "okienko" : "okienka"} (waga {teacherGapsWeight})
                    </span>
                    {#if teachersWithCustomGaps.length > 0}
                      <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                        {teachersWithCustomGaps.length} indywidualnych
                      </span>
                    {/if}
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Nauczyciele nie powinni czekać bezczynnie w szkole między lekcjami w ciągu tygodnia roboczego.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleTeacherMaxGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleTeacherMaxGaps}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <div class="flex items-center gap-3">
                  <span class="text-slate-500 text-[11px] font-medium">Dopuszczalny limit ogólny:</span>
                  <select bind:value={teacherMaxGapsLimit} class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-semibold text-slate-800">
                    <option value={0}>0 okienek w tygodniu (ciągły plan)</option>
                    <option value={1}>Maks. 1 okienko w tygodniu</option>
                    <option value={2}>Maks. 2 okienka w tygodniu</option>
                    <option value={3}>Maks. 3 okienka w tygodniu</option>
                    <option value={4}>Maks. 4 okienka w tygodniu</option>
                  </select>
                </div>
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga kary (kara/okienko):</span>
                  <input type="number" min="1" max="50" bind:value={teacherGapsWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              </div>
              {#if teachersWithCustomGaps.length > 0}
                <div class="p-2 bg-cyan-50/60 rounded-lg border border-cyan-100 text-[11px] text-cyan-900">
                  Nauczyciele z indywidualnym limitem: {teachersWithCustomGaps.map(t => `${t.name} (max ${t.maxGapsPerWeek})`).join(", ")}
                </div>
              {/if}
            {/if}
          </div>

          <!-- Reguła 5: Dzienne obciążenie klas (LimitBusyTimesConstraint) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">⚖️</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Dzienne ramy lekcyjne dla oddziałów (LimitBusyTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
                      {studentMinDailyLimit} - {studentMaxDailyLimit} lekcji/dzień
                    </span>
                    {#if classesWithCustomDailyLimit.length > 0}
                      <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                        {classesWithCustomDailyLimit.length} indywidualnych
                      </span>
                    {/if}
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Higiena nauki: zapobiega przychodzeniu uczniów na pojedyncze lekcje oraz przeciążeniu zbyt długim dniem nauki.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleMaxLessonsDaily}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <div class="flex items-center gap-3">
                  <div class="flex items-center gap-1.5">
                    <span class="text-slate-500 text-[11px]">Min. lekcji dziennie:</span>
                    <input type="number" min="1" max="6" bind:value={studentMinDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                  </div>
                  <div class="flex items-center gap-1.5">
                    <span class="text-slate-500 text-[11px]">Maks. lekcji dziennie:</span>
                    <input type="number" min="4" max="10" bind:value={studentMaxDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                  </div>
                </div>
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga kary:</span>
                  <input type="number" min="1" max="50" bind:value={studentDailyWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              </div>
            {/if}
          </div>

          <!-- Reguła 5b: Maksymalne obciążenie dzienne nauczycieli (LimitBusyTimesConstraint) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">⏱️</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Maksymalne obciążenie dzienne nauczycieli (LimitBusyTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 bg-amber-50 text-amber-700 rounded-full border border-amber-200">
                      Maks. {teacherMaxDailyLimit} lekcji/dzień
                    </span>
                    {#if teachersWithCustomDailyLimit.length > 0}
                      <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                        {teachersWithCustomDailyLimit.length} indywidualnych
                      </span>
                    {/if}
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Chroni kadrę pedagogiczną przed kumulacją zbyt wielu godzin dydaktycznych w jednym dniu.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleTeacherMaxDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleTeacherMaxDaily}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <div class="flex items-center gap-3">
                  <span class="text-slate-500 text-[11px] font-medium">Maks. lekcji w dniu:</span>
                  <select bind:value={teacherMaxDailyLimit} class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-semibold text-slate-800">
                    <option value={4}>Maks. 4 lekcje dziennie</option>
                    <option value={5}>Maks. 5 lekcji dziennie</option>
                    <option value={6}>Maks. 6 lekcji dziennie</option>
                    <option value={7}>Maks. 7 lekcji dziennie</option>
                    <option value={8}>Maks. 8 lekcji dziennie</option>
                  </select>
                </div>
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga kary:</span>
                  <input type="number" min="1" max="50" bind:value={teacherDailyWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              </div>
              {#if teachersWithCustomDailyLimit.length > 0}
                <div class="p-2 bg-amber-50/60 rounded-lg border border-amber-100 text-[11px] text-amber-900">
                  Nauczyciele z indywidualnym limitem: {teachersWithCustomDailyLimit.map(t => `${t.name} (max ${t.maxDailyLessons}h)`).join(", ")}
                </div>
              {/if}
            {/if}
          </div>

          <!-- Reguła 5c: Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">📅</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                      {teachersWithCustomDays.length} z limitem dni
                    </span>
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Egzekwuje ograniczenie liczby dni obecności w szkole dla nauczycieli na niepełnym etacie lub dojeżdżających.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleTeacherClusterDays} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleTeacherClusterDays}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <span class="text-slate-500 text-[11px]">
                  Limity dni konfiguruje się w kroku 2 przy każdym pedagogu (przycisk Dostępność).
                </span>
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga kary:</span>
                  <input type="number" min="1" max="50" bind:value={teacherClusterWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              </div>
              {#if teachersWithCustomDays.length > 0}
                <div class="p-2 bg-purple-50/60 rounded-lg border border-purple-100 text-[11px] text-purple-900">
                  Nauczyciele z limitem dni: {teachersWithCustomDays.map(t => `${t.name} (maks. ${t.maxDaysPerWeek} dni)`).join(", ")}
                </div>
              {/if}
            {/if}
          </div>

          <!-- Reguła 6: Rozkład przedmiotów (SpreadEventsConstraint - Faza 3) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {ruleSpreadSubjects ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">🗓️</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Równomierny rozkład przedmiotów w tygodniu (SpreadEventsConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {ruleSpreadSubjectsSoft ? 'bg-indigo-50 text-indigo-700 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
                      {ruleSpreadSubjectsSoft ? `SOFT (waga ${spreadSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
                    </span>
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Zapobiega kumulowaniu wielu godzin tego samego przedmiotu (np. 4h matematyki) w jeden dzień.
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleSpreadSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleSpreadSubjects}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <div class="flex items-center gap-2">
                  <span class="text-slate-600 font-medium text-[11px]">Dopuszczalna liczba lekcji dziennie:</span>
                  <select
                    bind:value={spreadSubjectsMode}
                    class="px-2 py-1 bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800 cursor-pointer"
                  >
                    <option value="single">Ściśle 1 lekcja dziennie (maks. 1h/dzień)</option>
                    <option value="blocks">Dopuszczalne bloki 2-godzinne (maks. 2h/dzień)</option>
                  </select>
                </div>

                <div class="flex items-center gap-3">
                  <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
                    <button
                      type="button"
                      onclick={() => (ruleSpreadSubjectsSoft = true)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {ruleSpreadSubjectsSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      SOFT
                    </button>
                    <button
                      type="button"
                      onclick={() => (ruleSpreadSubjectsSoft = false)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!ruleSpreadSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      HARD
                    </button>
                  </div>

                  {#if ruleSpreadSubjectsSoft}
                    <div class="flex items-center gap-1.5">
                      <span class="text-slate-500 text-[11px]">Waga:</span>
                      <input type="number" min="1" max="20" bind:value={spreadSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                    </div>
                  {/if}
                </div>
              </div>
            {/if}
          </div>

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

          <!-- Reguła 8: Poranne lekcje przedmiotów ścisłych i maturalnych (PreferTimesConstraint - Faza 3) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {ruleMorningCoreSubjects ? 'border-amber-300 ring-2 ring-amber-500/10' : ''}">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">🌅</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Przedmioty ścisłe i maturalne rano (PreferTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {morningCoreSubjectsSoft ? 'bg-amber-50 text-amber-800 border border-amber-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
                      {morningCoreSubjectsSoft ? `SOFT (waga ${morningCoreSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
                    </span>
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Kieruje przedmioty wymagające koncentracji (Matematyka, Fizyka, Chemia, Polski, Biologia) na poranne godziny (lekcje 1–4).
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleMorningCoreSubjects} class="w-4 h-4 accent-amber-600 cursor-pointer mt-1" />
            </div>

            {#if ruleMorningCoreSubjects}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <span class="text-slate-500 text-[11px]">
                  Preferowane lekcje 1–4 każdego dnia (8:00–11:30).
                </span>
                <div class="flex items-center gap-3">
                  <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
                    <button
                      type="button"
                      onclick={() => (morningCoreSubjectsSoft = true)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {morningCoreSubjectsSoft ? 'bg-amber-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      SOFT
                    </button>
                    <button
                      type="button"
                      onclick={() => (morningCoreSubjectsSoft = false)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!morningCoreSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      HARD
                    </button>
                  </div>
                  {#if morningCoreSubjectsSoft}
                    <div class="flex items-center gap-1.5">
                      <span class="text-slate-500 text-[11px]">Waga:</span>
                      <input type="number" min="1" max="20" bind:value={morningCoreSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                    </div>
                  {/if}
                </div>
              </div>
            {/if}
          </div>

          <!-- Reguła 9: Popołudniowe lekcje WF i przedmiotów artystycznych (PreferTimesConstraint - Faza 3) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {ruleAfternoonFlexSubjects ? 'border-indigo-300 ring-2 ring-indigo-500/10' : ''}">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">🌇</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">WF i zajęcia artystyczne po południu (PreferTimesConstraint)</span>
                    <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {afternoonFlexSubjectsSoft ? 'bg-indigo-50 text-indigo-800 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
                      {afternoonFlexSubjectsSoft ? `SOFT (waga ${afternoonFlexSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
                    </span>
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Kieruje zajęcia sportowe i artystyczne (Wychowanie Fizyczne, Plastyka, Muzyka, Religia, Etyka) na godziny popołudniowe (od 5. lekcji).
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleAfternoonFlexSubjects} class="w-4 h-4 accent-indigo-600 cursor-pointer mt-1" />
            </div>

            {#if ruleAfternoonFlexSubjects}
              <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
                <span class="text-slate-500 text-[11px]">
                  Preferowane lekcje 5+ każdego dnia (od 11:45).
                </span>
                <div class="flex items-center gap-3">
                  <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
                    <button
                      type="button"
                      onclick={() => (afternoonFlexSubjectsSoft = true)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {afternoonFlexSubjectsSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      SOFT
                    </button>
                    <button
                      type="button"
                      onclick={() => (afternoonFlexSubjectsSoft = false)}
                      class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!afternoonFlexSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
                    >
                      HARD
                    </button>
                  </div>
                  {#if afternoonFlexSubjectsSoft}
                    <div class="flex items-center gap-1.5">
                      <span class="text-slate-500 text-[11px]">Waga:</span>
                      <input type="number" min="1" max="20" bind:value={afternoonFlexSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
                    </div>
                  {/if}
                </div>
              </div>
            {/if}
          </div>

          <!-- Reguła 10: Lekcje równoległe i jednoczesne (LinkEventsConstraint - Faza 3) -->
          <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
            <div class="flex items-start gap-3">
              <span class="text-xl">🔗</span>
              <div>
                <div class="flex items-center gap-2">
                  <span class="text-xs font-bold text-slate-900">Lekcje równoległe i jednoczesne (LinkEventsConstraint)</span>
                  {#if linkedGroups.length > 0}
                    <span class="text-[10px] font-bold px-2 py-0.5 bg-purple-100 text-purple-800 rounded-full border border-purple-200">
                      {linkedGroups.length} grup powiązanych
                    </span>
                  {/if}
                </div>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Wymusza odbywanie się wybranych lekcji w tym samym slocie czasowym (np. grupy językowe z klas 1A i 1B).
                  {#if linkedGroups.length > 0}
                    <span class="text-purple-900 font-medium block mt-1">
                      Aktywne powiązania: {linkedGroups.map((g) => g.name).join(", ")}.
                    </span>
                  {:else}
                    <span class="text-slate-400 italic block mt-1">
                      (Brak powiązań — możesz je skonfigurować w kroku 4 przyciskiem „Lekcje równoległe”).
                    </span>
                  {/if}
                </p>
              </div>
            </div>
            <button
              type="button"
              onclick={() => (isLinkedModalOpen = true)}
              class="text-[11px] font-semibold px-2.5 py-1 bg-purple-50 hover:bg-purple-100 text-purple-700 rounded-lg border border-purple-200 shrink-0 cursor-pointer transition-colors"
            >
              Zarządzaj powiązaniami
            </button>
          </div>

          <!-- Reguła 11: Następstwo i kolejność lekcji (OrderEventsConstraint - Faza 3) -->
          <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {ruleOrderLessons ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
            <div class="flex items-start justify-between gap-4">
              <div class="flex items-start gap-3">
                <span class="text-xl">📐</span>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-xs font-bold text-slate-900">Następstwo i kolejność lekcji (OrderEventsConstraint)</span>
                    {#if orderRules.length > 0}
                      <span class="text-[10px] font-bold px-2 py-0.5 bg-blue-100 text-blue-800 rounded-full border border-blue-200">
                        {orderRules.length} reguł kolejności
                      </span>
                    {/if}
                  </div>
                  <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                    Egzekwuje kolejność pomiędzy zajęciami (np. wykład teoretyczny musi odbyć się przed ćwiczeniami/laboratorium).
                  </p>
                </div>
              </div>
              <input type="checkbox" bind:checked={ruleOrderLessons} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
            </div>

            {#if ruleOrderLessons}
              <div class="pt-3 border-t border-slate-100 flex flex-col gap-3 text-xs">
                <!-- Add new order pair -->
                <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/80 flex flex-col gap-2.5">
                  <span class="font-bold text-slate-800 text-[11px] uppercase tracking-wider">
                    Dodaj parę sekwencji (Lekcja 1 musi być PRZED Lekcją 2):
                  </span>
                  <div class="grid grid-cols-1 sm:grid-cols-3 gap-2">
                    <div>
                      <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja pierwsza:</span>
                      <select
                        bind:value={newOrderFirstLesson}
                        class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                      >
                        <option value="">Wybierz lekcję 1...</option>
                        {#each lessons as l}
                          <option value={l.id}>{l.subject} ({classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                        {/each}
                      </select>
                    </div>

                    <div>
                      <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja następująca:</span>
                      <select
                        bind:value={newOrderSecondLesson}
                        class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                      >
                        <option value="">Wybierz lekcję 2...</option>
                        {#each lessons as l}
                          <option value={l.id}>{l.subject} ({classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                        {/each}
                      </select>
                    </div>

                    <div class="flex items-end gap-2">
                      <div class="flex-1">
                        <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Min. odstęp:</span>
                        <select
                          bind:value={newOrderSeparation}
                          class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                        >
                          <option value={0}>W tym samym tygodniu (wcześniej)</option>
                          <option value={periodsPerDay}>Min. 1 dzień odstępu</option>
                        </select>
                      </div>

                      <button
                        type="button"
                        onclick={addOrderRule}
                        class="px-3 py-1 bg-slate-900 hover:bg-slate-800 text-white font-bold rounded text-xs cursor-pointer transition-colors"
                      >
                        + Dodaj
                      </button>
                    </div>
                  </div>
                </div>

                <!-- Existing order pairs -->
                {#if orderRules.length > 0}
                  <div class="divide-y divide-slate-100 border border-slate-200 rounded-lg overflow-hidden bg-white">
                    {#each orderRules as rule}
                      <div class="p-2.5 flex items-center justify-between text-xs hover:bg-slate-50">
                        <div class="flex items-center gap-2">
                          <span class="text-blue-600 font-bold">1.</span>
                          <span class="font-bold text-slate-900">{lessons.find((l) => l.id === rule.firstLessonId)?.subject || rule.firstLessonId}</span>
                          <span class="text-slate-400">➔</span>
                          <span class="text-blue-600 font-bold">2.</span>
                          <span class="font-bold text-slate-900">{lessons.find((l) => l.id === rule.secondLessonId)?.subject || rule.secondLessonId}</span>
                          <span class="text-slate-400 text-[11px]">
                            ({rule.minSeparation > 0 ? "min. 1 dzień odstępu" : "przed"})
                          </span>
                        </div>
                        <button
                          type="button"
                          onclick={() => removeOrderRule(rule.id)}
                          class="text-slate-400 hover:text-rose-600 p-1 cursor-pointer"
                        >
                          ✕
                        </button>
                      </div>
                    {/each}
                  </div>
                {/if}
              </div>
            {/if}
          </div>

          <!-- Reguła 12: Indywidualne niedostępności zasobów (AvoidUnavailableTimesConstraint - Faza 1) -->
          <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
            <div class="flex items-start gap-3">
              <span class="text-xl">🗓️</span>
              <div>
                <div class="flex items-center gap-2">
                  <span class="text-xs font-bold text-slate-900">Indywidualne wykluczenia czasowe (AvoidUnavailableTimesConstraint)</span>
                  {#if totalUnavailabilitiesCount > 0}
                    <span class="text-[10px] font-bold px-2 py-0.5 bg-rose-100 text-rose-800 rounded-full border border-rose-200">
                      {totalUnavailabilitiesCount} zablokowanych godzin
                    </span>
                  {/if}
                </div>
                <p class="text-xs text-slate-500 mt-0.5 leading-snug">
                  Uwzględnia zdefiniowane w krokach 2 i 3 blokady godzin dla nauczycieli, sal i klas (urlopy, zebrania, wyłączenia pracowni).
                  {#if resourcesWithUnavailability.length > 0}
                    <span class="text-slate-700 font-medium block mt-1">
                      Zasoby z ograniczeniami ({resourcesWithUnavailability.length}): {resourcesWithUnavailability.map((r) => r.name).slice(0, 4).join(", ")}{resourcesWithUnavailability.length > 4 ? ` i ${resourcesWithUnavailability.length - 4} innych` : ""}.
                    </span>
                  {:else}
                    <span class="text-slate-400 italic block mt-1">
                      (Brak wykluczeń — wszyscy nauczyciele, sale i klasy są w pełni dyspozycyjni).
                    </span>
                  {/if}
                </p>
              </div>
            </div>
            <span class="text-[11px] font-semibold px-2.5 py-1 bg-blue-50 text-blue-700 rounded-full border border-blue-200 shrink-0">
              {resourcesWithUnavailability.length} zasobów
            </span>
          </div>
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
            {#if totalUnavailabilitiesCount > 0}
              <div class="flex items-center gap-2 text-emerald-800">
                <span>✓</span>
                <span>
                  Uwzględniono {totalUnavailabilitiesCount} godzin wykluczeń czasowych dla {resourcesWithUnavailability.length} zasobów (AvoidUnavailableTimesConstraint)
                </span>
              </div>
            {/if}
            {#if teachersWithCustomDays.length > 0}
              <div class="flex items-center gap-2 text-emerald-800">
                <span>✓</span>
                <span>
                  Zdefiniowano kompaktowy tydzień pracy (ClusterBusyTimes) dla {teachersWithCustomDays.length} pedagogów
                </span>
              </div>
            {/if}
            {#if teachersWithCustomDailyLimit.length > 0 || classesWithCustomDailyLimit.length > 0}
              <div class="flex items-center gap-2 text-emerald-800">
                <span>✓</span>
                <span>
                  Skonfigurowano indywidualne limity obciążenia dziennego (LimitBusyTimes)
                </span>
              </div>
            {/if}
            {#if linkedGroups.length > 0}
              <div class="flex items-center gap-2 text-purple-800">
                <span>✓</span>
                <span>
                  Skonfigurowano {linkedGroups.length} grup lekcji równoległych / jednoczesnych (LinkEventsConstraint)
                </span>
              </div>
            {/if}
            {#if ruleOrderLessons && orderRules.length > 0}
              <div class="flex items-center gap-2 text-blue-800">
                <span>✓</span>
                <span>
                  Wymuszono reguły kolejności dla {orderRules.length} par zajęć (OrderEventsConstraint)
                </span>
              </div>
            {/if}
            {#if ruleMorningCoreSubjects || ruleAfternoonFlexSubjects}
              <div class="flex items-center gap-2 text-amber-800">
                <span>✓</span>
                <span>
                  Włączono dydaktyczne preferencje pory dnia dla przedmiotów (PreferTimesConstraint)
                </span>
              </div>
            {/if}
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

  <!-- Modal Availability Grid (Phase 1 & 2) -->
  {#if activeAvailabilityTarget}
    <AvailabilityModal
      resourceId={activeAvailabilityTarget.id}
      resourceName={activeAvailabilityTarget.name}
      resourceType={activeAvailabilityTarget.type}
      days={currentDaysFormatted}
      periodsPerDay={periodsPerDay}
      initialUnavailableTimes={activeAvailabilityTarget.unavailableTimes}
      initialIsSoft={activeAvailabilityTarget.isSoft}
      initialWeight={activeAvailabilityTarget.weight}
      initialMaxDaysPerWeek={activeAvailabilityTarget.maxDaysPerWeek}
      initialMinDaysPerWeek={activeAvailabilityTarget.minDaysPerWeek}
      initialMaxDailyLessons={activeAvailabilityTarget.maxDailyLessons}
      initialMaxGapsPerWeek={activeAvailabilityTarget.maxGapsPerWeek}
      initialMinDailyLessons={activeAvailabilityTarget.minDailyLessons}
      onSave={handleSaveAvailability}
      onClose={() => (activeAvailabilityTarget = null)}
    />
  {/if}

  <!-- Modal Preferencji Godzinowych dla Lekcji (Phase 3) -->
  {#if activeLessonPrefTarget}
    <LessonPreferencesModal
      lessonId={activeLessonPrefTarget.lessonId}
      lessonSubject={activeLessonPrefTarget.subject}
      className={activeLessonPrefTarget.className}
      days={currentDaysFormatted}
      periodsPerDay={periodsPerDay}
      initialPreference={activeLessonPrefTarget.preference}
      initialPreferredTimes={activeLessonPrefTarget.preferredTimes}
      initialIsSoft={activeLessonPrefTarget.isSoft}
      initialWeight={activeLessonPrefTarget.weight}
      onSave={handleSaveLessonPref}
      onClose={() => (activeLessonPrefTarget = null)}
    />
  {/if}

  <!-- Modal Lekcji Równoległych / Jednoczesnych (Phase 3) -->
  {#if isLinkedModalOpen}
    <LinkedLessonsModal
      lessons={lessons}
      classes={classes}
      teachers={teachers}
      initialGroups={linkedGroups}
      onSave={(updatedGroups) => {
        linkedGroups = updatedGroups;
        isLinkedModalOpen = false;
      }}
      onClose={() => (isLinkedModalOpen = false)}
    />
  {/if}

  <!-- Modal Duplikowania Siatki Zajęć (Krok 2 & Krok 4) -->
  {#if duplicateModalTarget}
    {@const target = duplicateModalTarget}
    <DuplicateCurriculumModal
      sourceClass={target}
      classes={classes}
      sourceHours={getClassWeeklyHours(target.id)}
      sourceLessonsCount={getClassLessons(target.id).length}
      onDuplicate={(targetIds) => duplicateCurriculum(target.id, targetIds)}
      onClose={() => (duplicateModalTarget = null)}
    />
  {/if}
</div>

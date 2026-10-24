// frontend/src/lib/components/editor/editorState.svelte.ts
import type {
  SimpleClass,
  SimpleTeacher,
  SimpleRoom,
  SimpleLesson,
  LinkedGroupItem,
  OrderRuleItem,
  DayFormatted,
  BellTime,
  SubjectSummary,
  ActiveAvailabilityTarget,
  ActiveLessonPrefTarget,
  ResourceContextInfo,
} from "./types";
import { assembleXhsttPayload } from "./xhsttAssembler";
import { parseInstanceDetail } from "./xhsttLoader";
import type { InstanceDetail } from "../../types";

export class EditorState {
  // Nawigacja kreatora
  currentStep = $state<number>(1);
  saveSuccess = $state<string | null>(null);
  saveError = $state<string | null>(null);
  isSaving = $state<boolean>(false);

  // Krok 1: Parametry szkoły
  schoolName = $state("Liceum Ogólnokształcące nr 1");
  schoolId = $state("LO_1_Warszawa");
  selectedDays = $state<string[]>([
    "Poniedziałek",
    "Wtorek",
    "Środa",
    "Czwartek",
    "Piątek",
  ]);
  periodsPerDay = $state<number>(8);
  bellTimes = $state<BellTime[]>([
    { period: 1, start: "08:00", end: "08:45" },
    { period: 2, start: "08:55", end: "09:40" },
    { period: 3, start: "09:50", end: "10:35" },
    { period: 4, start: "10:50", end: "11:35" },
    { period: 5, start: "11:55", end: "12:40" },
    { period: 6, start: "12:55", end: "13:40" },
    { period: 7, start: "13:50", end: "14:35" },
    { period: 8, start: "14:45", end: "15:30" },
  ]);

  // Krok 2: Oddziały klasowe
  classes = $state<SimpleClass[]>([
    { id: "1A", name: "Klasa 1A (Mat-Fiz)", grade: 1 },
    { id: "1B", name: "Klasa 1B (Biol-Chem)", grade: 1 },
    { id: "2A", name: "Klasa 2A (Mat-Fiz)", grade: 2 },
    { id: "2B", name: "Klasa 2B (Humanistyczna)", grade: 2 },
    { id: "3A", name: "Klasa 3A (Mat-Geo)", grade: 3 },
    { id: "3B", name: "Klasa 3B (Biol-Chem)", grade: 3 },
    { id: "4A", name: "Klasa 4A (Mat-Fiz)", grade: 4 },
    { id: "4B", name: "Klasa 4B (Językowa)", grade: 4 },
  ]);
  step2ActiveTab = $state<"classes" | "teachers">("classes");
  selectedGradeFilter = $state<number | null>(null);
  duplicateModalTarget = $state<SimpleClass | null>(null);
  newClassName = $state("");
  newClassGrade = $state(1);

  // Krok 3: Nauczyciele i Sale
  teachers = $state<SimpleTeacher[]>([
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
  newTeacherName = $state("");
  newTeacherSubject = $state("Matematyka");
  newTeacherWorkload = $state(18);

  rooms = $state<SimpleRoom[]>([
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
  newRoomName = $state("");
  newRoomType = $state<SimpleRoom["type"]>("general");

  // Krok 4: Wykaz lekcji
  lessons = $state<SimpleLesson[]>([]);
  step4ActiveTab = $state<"lessons" | "byClass" | "byTeacher">("lessons");
  selectedClassViewId = $state<string>("1A");
  selectedTeacherViewId = $state<string>("");
  selectedGradeCurriculum = $state<number>(1);
  batchGradeSubject = $state("Matematyka");
  batchGradeHours = $state(4);
  batchGradeIsBlock = $state(false);

  // Filtry Kroku 4
  lessonSearchQuery = $state("");
  lessonFilterClass = $state<string>("all");
  lessonFilterTeacher = $state<string>("all");
  lessonFilterGrade = $state<number | null>(null);
  lessonFilterPreference = $state<"all" | "has_pref" | "morning" | "afternoon" | "custom" | "none">("all");
  selectedLessonIds = $state<Set<string>>(new Set());

  // Formularz dodawania lekcji
  lessonSubject = $state("Matematyka");
  lessonClassId = $state("1A");
  lessonTeacherId = $state("Kowalski_J");
  lessonRoomId = $state("s101");
  lessonCount = $state(4);
  lessonIsBlock = $state(false);

  // Krok 5: Reguły dydaktyczne i ograniczenia
  ruleContinuousBlocks = $state(false);
  ruleAssignAllTimes = $state(true);
  ruleAvoidClashes = $state(true);

  ruleStudentNoGaps = $state(true);
  ruleStudentNoGapsSoft = $state(false);
  studentGapsWeight = $state(10);

  ruleTeacherMaxGaps = $state(true);
  teacherMaxGapsLimit = $state(2);
  teacherGapsWeight = $state(5);

  ruleMaxLessonsDaily = $state(true);
  studentMinDailyLimit = $state(4);
  studentMaxDailyLimit = $state(8);
  studentDailyWeight = $state(4);

  ruleTeacherMaxDaily = $state(true);
  teacherMaxDailyLimit = $state(6);
  teacherDailyWeight = $state(4);

  ruleTeacherClusterDays = $state(true);
  teacherClusterWeight = $state(6);

  ruleSpreadSubjects = $state(true);
  spreadSubjectsMode = $state<"single" | "blocks">("single");
  ruleSpreadSubjectsSoft = $state(true);
  spreadSubjectsWeight = $state(5);

  ruleMorningCoreSubjects = $state(true);
  morningCoreSubjectsSoft = $state(true);
  morningCoreSubjectsWeight = $state(4);

  ruleAfternoonFlexSubjects = $state(false);
  afternoonFlexSubjectsSoft = $state(true);
  afternoonFlexSubjectsWeight = $state(3);

  // Lekcje równoległe (Faza 3)
  linkedGroups = $state<LinkedGroupItem[]>([]);
  isLinkedModalOpen = $state(false);

  // Reguły kolejności
  ruleOrderLessons = $state(false);
  orderRules = $state<OrderRuleItem[]>([]);
  newOrderFirstLesson = $state("");
  newOrderSecondLesson = $state("");
  newOrderSeparation = $state(0);
  newOrderIsSoft = $state(false);
  newOrderWeight = $state(8);

  // Modale pomocnicze
  activeAvailabilityTarget = $state<ActiveAvailabilityTarget | null>(null);
  activeLessonPrefTarget = $state<ActiveLessonPrefTarget | null>(null);
  isSubjectManagerOpen = $state(false);
  editEntityTarget = $state<{
    type: "class" | "teacher" | "room" | "lesson";
    data: any;
  } | null>(null);

  // MAPY I DNI
  readonly dayMap: Record<string, string> = {
    Poniedziałek: "Mon",
    Wtorek: "Tue",
    Środa: "Wed",
    Czwartek: "Thu",
    Piątek: "Fri",
    Sobota: "Sat",
    Niedziela: "Sun",
  };

  // GETTERY REAKTYWNE ($derived)
  get currentDaysFormatted(): DayFormatted[] {
    return this.selectedDays.map((dName) => ({
      id: this.dayMap[dName] || dName.slice(0, 3),
      name: dName,
    }));
  }

  get availableGrades(): number[] {
    const grades = new Set<number>();
    this.classes.forEach((c) => grades.add(this.getClassGrade(c)));
    return Array.from(grades).sort((a, b) => a - b);
  }

  get totalHoursAssigned(): number {
    return this.lessons.reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  get totalTeacherWorkload(): number {
    return this.teachers.reduce((sum, t) => sum + (Number(t.workload) || 0), 0);
  }

  get weeklySlotsCapacity(): number {
    return this.selectedDays.length * this.periodsPerDay;
  }

  get isSchoolValid(): boolean {
    return (
      Boolean(this.schoolName.trim()) &&
      this.classes.length > 0 &&
      this.teachers.length > 0 &&
      this.lessons.length > 0
    );
  }

  get filteredLessons(): SimpleLesson[] {
    return this.lessons.filter((l) => {
      // 1. Wyszukiwarka tekstowa
      if (this.lessonSearchQuery.trim()) {
        const query = this.lessonSearchQuery.toLowerCase().trim();
        const cls = this.classes.find((c) => c.id === l.classId);
        const t = this.teachers.find((tch) => tch.id === l.teacherId);
        const r = this.rooms.find((rm) => rm.id === l.roomId);
        const matchSubject = l.subject.toLowerCase().includes(query);
        const matchClass = cls ? cls.name.toLowerCase().includes(query) : l.classId.toLowerCase().includes(query);
        const matchTeacher = t ? t.name.toLowerCase().includes(query) : l.teacherId.toLowerCase().includes(query);
        const matchRoom = r ? r.name.toLowerCase().includes(query) : false;
        if (!matchSubject && !matchClass && !matchTeacher && !matchRoom) return false;
      }

      // 2. Filtr klasy
      if (this.lessonFilterClass !== "all" && l.classId !== this.lessonFilterClass) {
        return false;
      }

      // 3. Filtr nauczyciela
      if (this.lessonFilterTeacher !== "all" && l.teacherId !== this.lessonFilterTeacher) {
        return false;
      }

      // 4. Filtr rocznika
      if (this.lessonFilterGrade !== null) {
        const cls = this.classes.find((c) => c.id === l.classId);
        if (!cls || this.getClassGrade(cls) !== this.lessonFilterGrade) return false;
      }

      // 5. Filtr preferencji
      if (this.lessonFilterPreference === "has_pref" && (!l.timePreference || l.timePreference === "any")) {
        return false;
      }
      if (this.lessonFilterPreference === "none" && l.timePreference && l.timePreference !== "any") {
        return false;
      }
      if (this.lessonFilterPreference === "morning" && l.timePreference !== "morning") {
        return false;
      }
      if (this.lessonFilterPreference === "afternoon" && l.timePreference !== "afternoon") {
        return false;
      }
      if (this.lessonFilterPreference === "custom" && l.timePreference !== "custom") {
        return false;
      }

      return true;
    });
  }

  get filteredLessonsHours(): number {
    return this.filteredLessons.reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  get uniqueSubjects(): SubjectSummary[] {
    const map = new Map<
      string,
      { totalHours: number; lessonCount: number; teachers: Set<string>; classes: Set<string> }
    >();

    // Z lekcji
    for (const l of this.lessons) {
      const entry = map.get(l.subject) || {
        totalHours: 0,
        lessonCount: 0,
        teachers: new Set(),
        classes: new Set(),
      };
      entry.totalHours += l.weeklyCount;
      entry.lessonCount += 1;
      if (l.teacherId) {
        const t = this.teachers.find((tch) => tch.id === l.teacherId);
        entry.teachers.add(t ? t.name : l.teacherId);
      }
      if (l.classId) {
        const c = this.classes.find((cls) => cls.id === l.classId);
        entry.classes.add(c ? c.name : l.classId);
      }
      map.set(l.subject, entry);
    }

    // Z domyślnych przedmiotów nauczycieli
    for (const t of this.teachers) {
      if (t.subject) {
        const entry = map.get(t.subject) || {
          totalHours: 0,
          lessonCount: 0,
          teachers: new Set(),
          classes: new Set(),
        };
        entry.teachers.add(t.name);
        map.set(t.subject, entry);
      }
    }

    return Array.from(map.entries())
      .map(([name, data]) => ({
        name,
        lessonCount: data.lessonCount,
        totalHours: data.totalHours,
        teacherCount: data.teachers.size,
        teachers: Array.from(data.teachers),
        classes: Array.from(data.classes),
      }))
      .sort((a, b) => a.name.localeCompare(b.name));
  }

  get totalUnavailabilitiesCount(): number {
    return [...this.teachers, ...this.classes, ...this.rooms].reduce(
      (sum, r) => sum + (r.unavailableTimes?.length || 0),
      0
    );
  }

  get resourcesWithUnavailability(): (SimpleTeacher | SimpleClass | SimpleRoom)[] {
    return [...this.teachers, ...this.classes, ...this.rooms].filter(
      (r) => (r.unavailableTimes?.length || 0) > 0
    );
  }

  get teachersWithCustomDays(): SimpleTeacher[] {
    return this.teachers.filter(
      (t) => t.maxDaysPerWeek && t.maxDaysPerWeek < this.selectedDays.length
    );
  }

  get teachersWithCustomDailyLimit(): SimpleTeacher[] {
    return this.teachers.filter(
      (t) => t.maxDailyLessons !== null && t.maxDailyLessons !== undefined
    );
  }

  get teachersWithCustomGaps(): SimpleTeacher[] {
    return this.teachers.filter(
      (t) => t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined
    );
  }

  get classesWithCustomDailyLimit(): SimpleClass[] {
    return this.classes.filter(
      (c) =>
        (c.minDailyLessons !== null && c.minDailyLessons !== undefined) ||
        (c.maxDailyLessons !== null && c.maxDailyLessons !== undefined)
    );
  }

  // POMOCNIKI ROZPOZNAWANIA
  getClassGrade(cls: SimpleClass): number {
    if (cls.grade !== undefined && cls.grade !== null) return cls.grade;
    const match = cls.name.match(/^(\d+)/) || cls.id.match(/^(\d+)/);
    return match ? parseInt(match[1], 10) : 1;
  }

  getClassWeeklyHours(classId: string): number {
    return this.lessons
      .filter((l) => l.classId === classId)
      .reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  getClassLessons(classId: string) {
    return this.lessons.filter((l) => l.classId === classId);
  }

  getTeacherAssignedHours(teacherId: string): number {
    return this.lessons
      .filter((l) => l.teacherId === teacherId)
      .reduce((sum, l) => sum + l.weeklyCount, 0);
  }

  getTeacherClasses(teacherId: string): { classId: string; className: string; subject: string; hours: number }[] {
    const list = this.lessons.filter((l) => l.teacherId === teacherId);
    return list.map((l) => {
      const cls = this.classes.find((c) => c.id === l.classId);
      return {
        classId: l.classId,
        className: cls ? cls.name : l.classId,
        subject: l.subject,
        hours: l.weeklyCount,
      };
    });
  }

  getConsistentTeacherForClassSubject(classId: string, subject: string): string | null {
    const existing = this.lessons.find((l) => l.classId === classId && l.subject === subject);
    if (existing) return existing.teacherId;
    const qualified = this.teachers.filter((t) => t.subject === subject);
    if (qualified.length === 0) return null;
    qualified.sort((a, b) => this.getTeacherAssignedHours(a.id) - this.getTeacherAssignedHours(b.id));
    return qualified[0].id;
  }

  // MUTACJE: ZMIANA NAZW I EDYCJA (Filar kluczowy użytkownika)
  renameSubject(oldName: string, newName: string) {
    const cleanNew = newName.trim();
    if (!cleanNew || oldName === cleanNew) return;

    // 1. Aktualizacja w lekcjach
    this.lessons = this.lessons.map((l) =>
      l.subject === oldName ? { ...l, subject: cleanNew } : l
    );

    // 2. Aktualizacja w nauczycielach
    this.teachers = this.teachers.map((t) =>
      t.subject === oldName ? { ...t, subject: cleanNew } : t
    );

    // 3. Aktualizacja w nazwach reguł kolejności
    this.orderRules = this.orderRules.map((r) => {
      const l1 = this.lessons.find((l) => l.id === r.firstLessonId);
      const l2 = this.lessons.find((l) => l.id === r.secondLessonId);
      return {
        ...r,
        name: `${l1?.subject || "Lekcja 1"} przed ${l2?.subject || "Lekcja 2"}`,
      };
    });

    // 4. Formularze robocze
    if (this.batchGradeSubject === oldName) this.batchGradeSubject = cleanNew;
    if (this.lessonSubject === oldName) this.lessonSubject = cleanNew;
    if (this.newTeacherSubject === oldName) this.newTeacherSubject = cleanNew;
  }

  mergeSubjects(sourceSubject: string, targetSubject: string) {
    this.renameSubject(sourceSubject, targetSubject);
  }

  updateClass(classId: string, updated: Partial<SimpleClass>) {
    const oldId = classId;
    const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

    this.classes = this.classes.map((c) =>
      c.id === oldId ? { ...c, ...updated, id: newId } : c
    );

    // Kaskada do lekcji jeśli zmieniono ID
    if (newId !== oldId) {
      this.lessons = this.lessons.map((l) =>
        l.classId === oldId ? { ...l, classId: newId } : l
      );

      if (this.selectedClassViewId === oldId) {
        this.selectedClassViewId = newId;
      }
      if (this.lessonClassId === oldId) {
        this.lessonClassId = newId;
      }
    }
  }

  updateTeacher(teacherId: string, updated: Partial<SimpleTeacher>) {
    const oldId = teacherId;
    const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

    this.teachers = this.teachers.map((t) =>
      t.id === oldId ? { ...t, ...updated, id: newId } : t
    );

    // Kaskada do lekcji jeśli zmieniono ID
    if (newId !== oldId) {
      this.lessons = this.lessons.map((l) =>
        l.teacherId === oldId ? { ...l, teacherId: newId } : l
      );
      if (this.selectedTeacherViewId === oldId) {
        this.selectedTeacherViewId = newId;
      }
      if (this.lessonTeacherId === oldId) {
        this.lessonTeacherId = newId;
      }
    }
  }

  updateRoom(roomId: string, updated: Partial<SimpleRoom>) {
    const oldId = roomId;
    const newId = updated.id ? updated.id.trim().replace(/\s+/g, "_") : oldId;

    this.rooms = this.rooms.map((r) =>
      r.id === oldId ? { ...r, ...updated, id: newId } : r
    );

    // Kaskada do lekcji jeśli zmieniono ID
    if (newId !== oldId) {
      this.lessons = this.lessons.map((l) =>
        l.roomId === oldId ? { ...l, roomId: newId } : l
      );
      if (this.lessonRoomId === oldId) {
        this.lessonRoomId = newId;
      }
    }
  }

  updateLesson(lessonId: string, updated: Partial<SimpleLesson>) {
    this.lessons = this.lessons.map((l) =>
      l.id === lessonId ? { ...l, ...updated } : l
    );
  }

  // MUTACJE KLAS
  addClass() {
    if (!this.newClassName.trim()) return;
    const cleanId = this.newClassName.trim().replace(/\s+/g, "_");
    if (this.classes.some((c) => c.id === cleanId)) {
      alert("Oddział o tej nazwie już istnieje!");
      return;
    }
    const match = this.newClassName.match(/^(\d+)/);
    const grade = match ? parseInt(match[1], 10) : this.newClassGrade;
    this.classes = [...this.classes, { id: cleanId, name: this.newClassName.trim(), grade }];
    this.newClassName = "";
  }

  removeClass(id: string) {
    const classLessonIds = new Set(this.lessons.filter((l) => l.classId === id).map((l) => l.id));
    this.classes = this.classes.filter((c) => c.id !== id);
    this.lessons = this.lessons.filter((l) => l.classId !== id);
    this.orderRules = this.orderRules.filter(
      (r) => !classLessonIds.has(r.firstLessonId) && !classLessonIds.has(r.secondLessonId)
    );
  }

  duplicateClass(sourceClassId: string) {
    const source = this.classes.find((c) => c.id === sourceClassId);
    if (!source) return;

    let suffixIndex = 2;
    let newId = `${source.id}_kopia`;
    let newName = `${source.name} (kopia)`;
    while (this.classes.some((c) => c.id === newId)) {
      newId = `${source.id}_kopia${suffixIndex}`;
      newName = `${source.name} (kopia ${suffixIndex})`;
      suffixIndex++;
    }

    const newClass: SimpleClass = {
      ...source,
      id: newId,
      name: newName,
      grade: source.grade,
      unavailableTimes: source.unavailableTimes ? [...source.unavailableTimes] : [],
    };
    this.classes = [...this.classes, newClass];

    const sourceLessons = this.lessons.filter((l) => l.classId === sourceClassId);
    const duplicatedLessons: SimpleLesson[] = sourceLessons.map((l) => {
      const cleanSub = l.subject.replace(/\s+/g, "");
      const newLessonId = `${newId}_${cleanSub}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
      return {
        ...l,
        id: newLessonId,
        classId: newId,
        preferredTimes: l.preferredTimes ? [...l.preferredTimes] : undefined,
      };
    });

    this.lessons = [...this.lessons, ...duplicatedLessons];
    this.selectedClassViewId = newId;
  }

  duplicateCurriculum(sourceClassId: string, targetClassIds: string[]) {
    const sourceLessons = this.lessons.filter((l) => l.classId === sourceClassId);
    if (sourceLessons.length === 0 || targetClassIds.length === 0) return;

    const targetSet = new Set(targetClassIds);
    const keptLessons = this.lessons.filter((l) => !targetSet.has(l.classId));
    const newLessons: SimpleLesson[] = [];

    targetClassIds.forEach((targetId) => {
      sourceLessons.forEach((sl) => {
        const cleanSub = sl.subject.replace(/\s+/g, "");
        const newLessonId = `${targetId}_${cleanSub}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
        const assignedTeacherId = this.getConsistentTeacherForClassSubject(targetId, sl.subject) || sl.teacherId;

        newLessons.push({
          ...sl,
          id: newLessonId,
          classId: targetId,
          teacherId: assignedTeacherId,
          preferredTimes: sl.preferredTimes ? [...sl.preferredTimes] : undefined,
        });
      });
    });

    this.lessons = [...keptLessons, ...newLessons];
  }

  addSubjectToGrade(grade: number, subject: string, weeklyCount: number, isBlock: boolean = false) {
    const targetClasses = this.classes.filter((c) => this.getClassGrade(c) === grade);
    if (targetClasses.length === 0) return;

    const count = Math.max(1, weeklyCount);
    const updated = [...this.lessons];

    targetClasses.forEach((cls) => {
      const teacherId = this.getConsistentTeacherForClassSubject(cls.id, subject) || (this.teachers[0] ? this.teachers[0].id : "");
      let defaultRoomId = this.rooms[0] ? this.rooms[0].id : "";
      const lower = subject.toLowerCase();
      if (lower.includes("wychowanie") || lower.includes("wf")) {
        const gym = this.rooms.find((r) => r.type === "gym");
        if (gym) defaultRoomId = gym.id;
      } else if (lower.includes("informat") || lower.includes("it")) {
        const it = this.rooms.find((r) => r.type === "it");
        if (it) defaultRoomId = it.id;
      } else if (lower.includes("chem") || lower.includes("biol")) {
        const lab = this.rooms.find((r) => r.type === "lab");
        if (lab) defaultRoomId = lab.id;
      }

      const existingIndex = updated.findIndex((l) => l.classId === cls.id && l.subject === subject);
      if (existingIndex >= 0) {
        updated[existingIndex] = {
          ...updated[existingIndex],
          weeklyCount: count,
          duration: isBlock ? 2 : 1,
        };
      } else {
        const cleanId = `${cls.id}_${subject.replace(/\s+/g, "")}_${Date.now().toString().slice(-3)}_${Math.random().toString(36).substring(2, 5)}`;
        updated.push({
          id: cleanId,
          subject,
          classId: cls.id,
          teacherId,
          roomId: defaultRoomId,
          duration: isBlock ? 2 : 1,
          weeklyCount: count,
        });
      }
    });

    this.lessons = updated;
  }

  // MUTACJE NAUCZYCIELI I SAL
  addTeacher() {
    if (!this.newTeacherName.trim()) return;
    const cleanId = this.newTeacherName.trim().replace(/\s+/g, "_");
    if (this.teachers.some((t) => t.id === cleanId)) {
      alert("Nauczyciel o tym nazwisku już istnieje!");
      return;
    }
    this.teachers = [
      ...this.teachers,
      {
        id: cleanId,
        name: this.newTeacherName.trim(),
        subject: this.newTeacherSubject,
        workload: Number(this.newTeacherWorkload) || 18,
      },
    ];
    this.newTeacherName = "";
  }

  removeTeacher(id: string) {
    this.teachers = this.teachers.filter((t) => t.id !== id);
  }

  updateTeacherWorkload(teacherId: string, newWorkload: number) {
    const val = Math.max(1, Number(newWorkload) || 18);
    this.teachers = this.teachers.map((t) =>
      t.id === teacherId ? { ...t, workload: val } : t
    );
  }

  addRoom() {
    if (!this.newRoomName.trim()) return;
    const cleanId = this.newRoomName.trim().replace(/\s+/g, "_");
    if (this.rooms.some((r) => r.id === cleanId)) {
      alert("Sala o tej nazwie już istnieje!");
      return;
    }
    this.rooms = [
      ...this.rooms,
      { id: cleanId, name: this.newRoomName.trim(), type: this.newRoomType },
    ];
    this.newRoomName = "";
  }

  removeRoom(id: string) {
    this.rooms = this.rooms.filter((r) => r.id !== id);
  }

  // MUTACJE LEKCJI
  addCustomLesson() {
    if (!this.lessonClassId || !this.lessonTeacherId) {
      alert("Wybierz klasę oraz nauczyciela!");
      return;
    }
    const cleanId = `${this.lessonClassId}_${this.lessonSubject.replace(/\s+/g, "")}_${Date.now().toString().slice(-3)}`;
    this.lessons = [
      ...this.lessons,
      {
        id: cleanId,
        subject: this.lessonSubject,
        classId: this.lessonClassId,
        teacherId: this.lessonTeacherId,
        roomId: this.lessonRoomId || (this.rooms[0] ? this.rooms[0].id : ""),
        duration: this.lessonIsBlock ? 2 : 1,
        weeklyCount: Number(this.lessonCount) || 1,
      },
    ];
  }

  removeLesson(id: string) {
    this.lessons = this.lessons.filter((l) => l.id !== id);
    this.orderRules = this.orderRules.filter(
      (r) => r.firstLessonId !== id && r.secondLessonId !== id
    );
  }

  setQuickLessonPref(lessonId: string, pref: "any" | "morning" | "afternoon") {
    let prefTimes: string[] = [];
    if (pref === "morning") {
      this.currentDaysFormatted.forEach((d) => {
        for (let p = 1; p <= Math.min(4, this.periodsPerDay); p++) {
          prefTimes.push(`${d.id}_${p}`);
        }
      });
    } else if (pref === "afternoon") {
      this.currentDaysFormatted.forEach((d) => {
        for (let p = 5; p <= this.periodsPerDay; p++) {
          prefTimes.push(`${d.id}_${p}`);
        }
      });
    }
    this.lessons = this.lessons.map((l) =>
      l.id === lessonId
        ? {
            ...l,
            timePreference: pref,
            preferredTimes: prefTimes,
            preferenceSoft: l.preferenceSoft ?? true,
            preferenceWeight: l.preferenceWeight ?? 4,
          }
        : l
    );
  }

  applyBatchPreference(pref: "any" | "morning" | "afternoon") {
    if (this.selectedLessonIds.size === 0) return;
    let prefTimes: string[] = [];
    if (pref === "morning") {
      this.currentDaysFormatted.forEach((d) => {
        for (let p = 1; p <= Math.min(4, this.periodsPerDay); p++) {
          prefTimes.push(`${d.id}_${p}`);
        }
      });
    } else if (pref === "afternoon") {
      this.currentDaysFormatted.forEach((d) => {
        for (let p = 5; p <= this.periodsPerDay; p++) {
          prefTimes.push(`${d.id}_${p}`);
        }
      });
    }
    this.lessons = this.lessons.map((l) =>
      this.selectedLessonIds.has(l.id)
        ? {
            ...l,
            timePreference: pref,
            preferredTimes: prefTimes,
            preferenceSoft: l.preferenceSoft ?? true,
            preferenceWeight: l.preferenceWeight ?? 4,
          }
        : l
    );
    this.selectedLessonIds = new Set();
  }

  toggleLessonSelection(id: string) {
    const next = new Set(this.selectedLessonIds);
    if (next.has(id)) next.delete(id);
    else next.add(id);
    this.selectedLessonIds = next;
  }

  selectAllFilteredLessons() {
    this.selectedLessonIds = new Set(this.filteredLessons.map((l) => l.id));
  }

  clearSelectedLessons() {
    this.selectedLessonIds = new Set();
  }

  clearLessonFilters() {
    this.lessonSearchQuery = "";
    this.lessonFilterClass = "all";
    this.lessonFilterTeacher = "all";
    this.lessonFilterGrade = null;
    this.lessonFilterPreference = "all";
  }

  // MODALE DOSTĘPNOŚCI I PREFERENCJI
  openAvailabilityModal(id: string, name: string, type: "teacher" | "room" | "class") {
    let unavail: string[] = [];
    let soft = false;
    let w = 10;
    let maxDays: number | null = null;
    let minDays: number | null = null;
    let maxDaily: number | null = null;
    let maxGaps: number | null = null;
    let minDaily: number | null = null;
    let contextInfo: ResourceContextInfo | undefined = undefined;

    if (type === "teacher") {
      const t = this.teachers.find((tch) => tch.id === id);
      if (t) {
        unavail = t.unavailableTimes || [];
        soft = t.unavailableSoft || false;
        w = t.unavailableWeight || 10;
        maxDays = t.maxDaysPerWeek ?? null;
        minDays = t.minDaysPerWeek ?? null;
        maxDaily = t.maxDailyLessons ?? null;
        maxGaps = t.maxGapsPerWeek ?? null;

        contextInfo = {
          workload: t.workload,
          totalAssignedHours: this.getTeacherAssignedHours(t.id),
          classes: this.getTeacherClasses(t.id),
        };
      }
    } else if (type === "class") {
      const c = this.classes.find((cls) => cls.id === id);
      if (c) {
        unavail = c.unavailableTimes || [];
        soft = c.unavailableSoft || false;
        w = c.unavailableWeight || 10;
        minDaily = c.minDailyLessons ?? null;
        maxDaily = c.maxDailyLessons ?? null;

        const classLessons = this.getClassLessons(c.id);
        contextInfo = {
          totalAssignedHours: this.getClassWeeklyHours(c.id),
          lessons: classLessons.map((l) => {
            const tch = this.teachers.find((t) => t.id === l.teacherId);
            return {
              subject: l.subject,
              teacherName: tch ? tch.name : l.teacherId,
              hours: l.weeklyCount,
            };
          }),
        };
      }
    } else {
      const r = this.rooms.find((rm) => rm.id === id);
      if (r) {
        unavail = r.unavailableTimes || [];
        soft = r.unavailableSoft || false;
        w = r.unavailableWeight || 10;

        const roomLessons = this.lessons.filter((l) => l.roomId === r.id);
        const totalRoomHours = roomLessons.reduce((sum, l) => sum + l.weeklyCount, 0);
        contextInfo = {
          totalAssignedHours: totalRoomHours,
        };
      }
    }

    this.activeAvailabilityTarget = {
      id,
      name,
      type,
      unavailableTimes: unavail,
      isSoft: soft,
      weight: w,
      maxDaysPerWeek: maxDays,
      minDaysPerWeek: minDays,
      maxDailyLessons: maxDaily,
      maxGapsPerWeek: maxGaps,
      minDailyLessons: minDaily,
      contextInfo,
    };
  }

  handleSaveAvailability(
    unavailableTimes: string[],
    isSoft: boolean,
    weight: number,
    extraParams?: {
      maxDaysPerWeek?: number | null;
      minDaysPerWeek?: number | null;
      maxDailyLessons?: number | null;
      maxGapsPerWeek?: number | null;
      minDailyLessons?: number | null;
    }
  ) {
    if (!this.activeAvailabilityTarget) return;
    const { id, type } = this.activeAvailabilityTarget;

    if (type === "teacher") {
      this.teachers = this.teachers.map((t) =>
        t.id === id
          ? {
              ...t,
              unavailableTimes,
              unavailableSoft: isSoft,
              unavailableWeight: weight,
              maxDaysPerWeek: extraParams?.maxDaysPerWeek ?? t.maxDaysPerWeek,
              minDaysPerWeek: extraParams?.minDaysPerWeek ?? t.minDaysPerWeek,
              maxDailyLessons: extraParams?.maxDailyLessons ?? t.maxDailyLessons,
              maxGapsPerWeek: extraParams?.maxGapsPerWeek ?? t.maxGapsPerWeek,
            }
          : t
      );
    } else if (type === "class") {
      this.classes = this.classes.map((c) =>
        c.id === id
          ? {
              ...c,
              unavailableTimes,
              unavailableSoft: isSoft,
              unavailableWeight: weight,
              minDailyLessons: extraParams?.minDailyLessons ?? c.minDailyLessons,
              maxDailyLessons: extraParams?.maxDailyLessons ?? c.maxDailyLessons,
            }
          : c
      );
    } else {
      this.rooms = this.rooms.map((r) =>
        r.id === id
          ? {
              ...r,
              unavailableTimes,
              unavailableSoft: isSoft,
              unavailableWeight: weight,
            }
          : r
      );
    }
    this.activeAvailabilityTarget = null;
  }

  openLessonPrefModal(lesson: SimpleLesson) {
    const cls = this.classes.find((c) => c.id === lesson.classId);
    this.activeLessonPrefTarget = {
      lessonId: lesson.id,
      subject: lesson.subject,
      className: cls ? cls.name : lesson.classId,
      preference: lesson.timePreference || "any",
      preferredTimes: lesson.preferredTimes || [],
      isSoft: lesson.preferenceSoft ?? true,
      weight: lesson.preferenceWeight ?? 4,
    };
  }

  handleSaveLessonPref(
    pref: "any" | "morning" | "afternoon" | "custom",
    times: string[],
    isSoft: boolean,
    weight: number
  ) {
    if (!this.activeLessonPrefTarget) return;
    const id = this.activeLessonPrefTarget.lessonId;
    this.lessons = this.lessons.map((l) =>
      l.id === id
        ? {
            ...l,
            timePreference: pref,
            preferredTimes: times,
            preferenceSoft: isSoft,
            preferenceWeight: weight,
          }
        : l
    );
    this.activeLessonPrefTarget = null;
  }

  // REGUŁY KOLEJNOŚCI
  addOrderRule() {
    if (!this.newOrderFirstLesson || !this.newOrderSecondLesson || this.newOrderFirstLesson === this.newOrderSecondLesson) {
      alert("Wybierz dwie różne lekcje do określenia kolejności!");
      return;
    }
    const l1 = this.lessons.find((l) => l.id === this.newOrderFirstLesson);
    const l2 = this.lessons.find((l) => l.id === this.newOrderSecondLesson);
    const ruleName = `${l1?.subject || "Lekcja 1"} przed ${l2?.subject || "Lekcja 2"}`;
    this.orderRules = [
      ...this.orderRules,
      {
        id: `order_${Date.now()}_${Math.random().toString(36).substring(2, 6)}`,
        name: ruleName,
        firstLessonId: this.newOrderFirstLesson,
        secondLessonId: this.newOrderSecondLesson,
        minSeparation: Number(this.newOrderSeparation) || 0,
        isSoft: this.newOrderIsSoft,
        weight: this.newOrderWeight,
      },
    ];
    this.newOrderFirstLesson = "";
    this.newOrderSecondLesson = "";
  }

  removeOrderRule(ruleId: string) {
    this.orderRules = this.orderRules.filter((r) => r.id !== ruleId);
  }

  // PRESETY I ŁADOWANIE
  loadHighSchoolPreset() {
    this.schoolName = "Liceum Ogólnokształcące (profilowane)";
    this.schoolId = "LO_Profilowane";
    this.selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
    this.periodsPerDay = 8;
    this.generateDefaultCurriculum();
  }

  generateDefaultCurriculum() {
    const subjectsDistribution: {
      subject: string;
      count: number;
      isBlock: boolean;
      roomType: SimpleRoom["type"];
    }[] = [
      { subject: "Język polski", count: 4, isBlock: false, roomType: "general" },
      { subject: "Język angielski", count: 3, isBlock: false, roomType: "general" },
      { subject: "Matematyka", count: 4, isBlock: false, roomType: "general" },
      { subject: "Historia", count: 2, isBlock: false, roomType: "general" },
      { subject: "Biologia", count: 2, isBlock: false, roomType: "lab" },
      { subject: "Chemia", count: 2, isBlock: false, roomType: "lab" },
      { subject: "Fizyka", count: 2, isBlock: false, roomType: "general" },
      { subject: "Geografia", count: 2, isBlock: false, roomType: "general" },
      { subject: "Informatyka", count: 2, isBlock: true, roomType: "it" },
      { subject: "Wychowanie Fizyczne", count: 3, isBlock: false, roomType: "gym" },
    ];

    const generatedLessons: SimpleLesson[] = [];
    this.classes.forEach((c) => {
      subjectsDistribution.forEach((spec) => {
        let teacher = this.teachers.find((t) => t.subject === spec.subject);
        if (!teacher) {
          teacher = this.teachers[0];
        }
        let room = this.rooms.find((r) => r.type === spec.roomType);
        if (!room) {
          room = this.rooms[0];
        }
        const cleanSub = spec.subject.replace(/\s+/g, "");
        const lid = `${c.id}_${cleanSub}`;
        generatedLessons.push({
          id: lid,
          subject: spec.subject,
          classId: c.id,
          teacherId: teacher ? teacher.id : "",
          roomId: room ? room.id : "",
          duration: spec.isBlock ? 2 : 1,
          weeklyCount: spec.count,
        });
      });
    });

    this.lessons = generatedLessons;
  }

  loadFromExistingInstance(inst: InstanceDetail) {
    const loaded = parseInstanceDetail(inst);
    this.schoolId = loaded.schoolId;
    this.schoolName = loaded.schoolName;
    this.selectedDays = loaded.selectedDays;
    this.periodsPerDay = loaded.periodsPerDay;
    if (loaded.classes.length > 0) this.classes = loaded.classes;
    if (loaded.teachers.length > 0) this.teachers = loaded.teachers;
    if (loaded.rooms.length > 0) this.rooms = loaded.rooms;

    // Aplikacja reguł
    const cd = loaded.constraintsData;
    this.ruleTeacherClusterDays = cd.ruleTeacherClusterDays;
    this.teacherClusterWeight = cd.teacherClusterWeight;
    this.ruleMaxLessonsDaily = cd.ruleMaxLessonsDaily;
    this.studentMinDailyLimit = cd.studentMinDailyLimit;
    this.studentMaxDailyLimit = cd.studentMaxDailyLimit;
    this.studentDailyWeight = cd.studentDailyWeight;
    this.ruleTeacherMaxGaps = cd.ruleTeacherMaxGaps;
    this.teacherMaxGapsLimit = cd.teacherMaxGapsLimit;
    this.teacherGapsWeight = cd.teacherGapsWeight;
    this.ruleStudentNoGaps = cd.ruleStudentNoGaps;
    this.ruleStudentNoGapsSoft = cd.ruleStudentNoGapsSoft;
    this.studentGapsWeight = cd.studentGapsWeight;
    this.ruleSpreadSubjects = cd.ruleSpreadSubjects;
    this.ruleSpreadSubjectsSoft = cd.ruleSpreadSubjectsSoft;
    this.spreadSubjectsWeight = cd.spreadSubjectsWeight;
    this.spreadSubjectsMode = cd.spreadSubjectsMode;
    this.ruleMorningCoreSubjects = cd.ruleMorningCoreSubjects;
    this.morningCoreSubjectsSoft = cd.morningCoreSubjectsSoft;
    this.morningCoreSubjectsWeight = cd.morningCoreSubjectsWeight;
    this.ruleAfternoonFlexSubjects = cd.ruleAfternoonFlexSubjects;
    this.afternoonFlexSubjectsSoft = cd.afternoonFlexSubjectsSoft;
    this.afternoonFlexSubjectsWeight = cd.afternoonFlexSubjectsWeight;
    this.ruleOrderLessons = cd.ruleOrderLessons;

    if (this.classes.length > 0 && this.teachers.length > 0) {
      this.generateDefaultCurriculum();
    }
  }

  buildXhsttPayload(): InstanceDetail {
    return assembleXhsttPayload({
      schoolId: this.schoolId,
      schoolName: this.schoolName,
      selectedDays: this.selectedDays,
      periodsPerDay: this.periodsPerDay,
      teachers: this.teachers,
      classes: this.classes,
      rooms: this.rooms,
      lessons: this.lessons,
      ruleContinuousBlocks: this.ruleContinuousBlocks,
      ruleAssignAllTimes: this.ruleAssignAllTimes,
      ruleAvoidClashes: this.ruleAvoidClashes,
      ruleStudentNoGaps: this.ruleStudentNoGaps,
      ruleStudentNoGapsSoft: this.ruleStudentNoGapsSoft,
      studentGapsWeight: this.studentGapsWeight,
      ruleTeacherMaxGaps: this.ruleTeacherMaxGaps,
      teacherMaxGapsLimit: this.teacherMaxGapsLimit,
      teacherGapsWeight: this.teacherGapsWeight,
      ruleMaxLessonsDaily: this.ruleMaxLessonsDaily,
      studentMinDailyLimit: this.studentMinDailyLimit,
      studentMaxDailyLimit: this.studentMaxDailyLimit,
      studentDailyWeight: this.studentDailyWeight,
      ruleTeacherMaxDaily: this.ruleTeacherMaxDaily,
      teacherMaxDailyLimit: this.teacherMaxDailyLimit,
      teacherDailyWeight: this.teacherDailyWeight,
      ruleTeacherClusterDays: this.ruleTeacherClusterDays,
      teacherClusterWeight: this.teacherClusterWeight,
      ruleSpreadSubjects: this.ruleSpreadSubjects,
      ruleSpreadSubjectsSoft: this.ruleSpreadSubjectsSoft,
      spreadSubjectsWeight: this.spreadSubjectsWeight,
      spreadSubjectsMode: this.spreadSubjectsMode,
      ruleMorningCoreSubjects: this.ruleMorningCoreSubjects,
      morningCoreSubjectsSoft: this.morningCoreSubjectsSoft,
      morningCoreSubjectsWeight: this.morningCoreSubjectsWeight,
      ruleAfternoonFlexSubjects: this.ruleAfternoonFlexSubjects,
      afternoonFlexSubjectsSoft: this.afternoonFlexSubjectsSoft,
      afternoonFlexSubjectsWeight: this.afternoonFlexSubjectsWeight,
      linkedGroups: this.linkedGroups,
      ruleOrderLessons: this.ruleOrderLessons,
      orderRules: this.orderRules,
      orderLessonsIsSoft: this.newOrderIsSoft,
      orderLessonsWeight: this.newOrderWeight,
    });
  }
}

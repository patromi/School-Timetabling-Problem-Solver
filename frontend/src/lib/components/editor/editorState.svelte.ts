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
} from "./types";
import type { InstanceDetail } from "../../types";
import {
  computeUniqueSubjects,
  calculateTeacherAssignedHours,
  getTeacherClassAssignments,
  findConsistentTeacherForClassSubject,
} from "./state/subjectAnalytics";
import { filterLessons } from "./state/lessonFiltering";
import {
  applySubjectRename,
  applyClassUpdate,
  applyTeacherUpdate,
  applyRoomUpdate,
} from "./state/entityRenaming";
import {
  applyQuickPrefToLesson,
  applyBatchPrefToLessons,
} from "./state/lessonPreferencesActions";
import {
  toggleIdInSet,
  extractAllLessonIds,
  resetFiltersState,
} from "./state/lessonSelectionActions";
import {
  getHighSchoolPresetBasics,
  generateDefaultCurriculumLessons,
} from "./state/editorPresets";
import {
  createDefaultBellTimes,
  createDefaultClasses,
  createDefaultTeachers,
  createDefaultRooms,
} from "./state/initialEditorDefaults";
import {
  loadInstanceIntoEditor,
  buildPayloadFromEditor,
} from "./state/instanceSync";
import {
  detectClassGrade,
  getAvailableGrades,
  getTotalHoursAssigned,
  getTotalTeacherWorkload,
  getTotalUnavailabilitiesCount,
  getResourcesWithUnavailability,
  getTeachersWithCustomDays,
  getTeachersWithCustomDailyLimit,
  getTeachersWithCustomGaps,
  getClassesWithCustomDailyLimit,
} from "./state/editorQueries";
import {
  executeAddClass,
  executeRemoveClass,
  executeDuplicateClass,
  executeDuplicateCurriculum,
  executeAddSubjectToGrade,
  executeAddTeacher,
  executeRemoveTeacher,
  executeAddRoom,
  executeRemoveRoom,
  executeAddCustomLesson,
  executeRemoveLesson,
  executeOpenAvailability,
  executeSaveAvailability,
  executeOpenLessonPref,
  executeSaveLessonPref,
  executeAddOrderRule,
  executeRemoveOrderRule,
} from "./state/editorMutationsBridge";

export class EditorState {
  // Nawigacja kreatora
  currentStep = $state<number>(1);
  saveSuccess = $state<string | null>(null);
  saveError = $state<string | null>(null);
  isSaving = $state<boolean>(false);

  // Nawigacja w trybie Studio Workbench
  studioSection = $state<"teachers" | "classes" | "rooms" | "structure" | "rules" | "summary">("teachers");
  selectedStudioTeacherId = $state<string>("");
  selectedStudioClassId = $state<string>("");
  selectedStudioRoomId = $state<string>("");
  studioSearchQuery = $state<string>("");

  // Krok 1: Parametry szkoły
  schoolName = $state("Liceum Ogólnokształcące nr 1");
  schoolId = $state("LO_1_Warszawa");
  selectedDays = $state<string[]>(["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"]);
  periodsPerDay = $state<number>(8);
  bellTimes = $state<BellTime[]>(createDefaultBellTimes());

  // Krok 2: Oddziały klasowe
  classes = $state<SimpleClass[]>(createDefaultClasses());
  step2ActiveTab = $state<"classes" | "teachers">("classes");
  selectedGradeFilter = $state<number | null>(null);
  duplicateModalTarget = $state<SimpleClass | null>(null);
  newClassName = $state("");
  newClassGrade = $state(1);

  // Krok 3: Nauczyciele i Sale
  teachers = $state<SimpleTeacher[]>(createDefaultTeachers());
  newTeacherName = $state("");
  newTeacherSubject = $state("Matematyka");
  newTeacherWorkload = $state(18);
  rooms = $state<SimpleRoom[]>(createDefaultRooms());
  newRoomName = $state("");
  newRoomType = $state<SimpleRoom["type"]>("general");

  // Krok 4: Wykaz lekcji i filtry
  lessons = $state<SimpleLesson[]>([]);
  step4ActiveTab = $state<"lessons" | "byClass" | "byTeacher">("lessons");
  selectedClassViewId = $state<string>("1A");
  selectedTeacherViewId = $state<string>("");
  selectedGradeCurriculum = $state<number>(1);
  batchGradeSubject = $state("Matematyka");
  batchGradeHours = $state(4);
  batchGradeIsBlock = $state(false);
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

  // Słownik przedmiotów
  customSubjects = $state<string[]>([]);

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

  // Powiązania i kolejność
  linkedGroups = $state<LinkedGroupItem[]>([]);
  isLinkedModalOpen = $state(false);
  ruleOrderLessons = $state(false);
  orderRules = $state<OrderRuleItem[]>([]);
  newOrderFirstLesson = $state("");
  newOrderSecondLesson = $state("");
  newOrderSeparation = $state(0);
  newOrderIsSoft = $state(false);
  newOrderWeight = $state(8);
  externalConstraintsNotice = $state<string[]>([]);

  // Modale pomocnicze
  activeAvailabilityTarget = $state<ActiveAvailabilityTarget | null>(null);
  activeLessonPrefTarget = $state<ActiveLessonPrefTarget | null>(null);
  isSubjectManagerOpen = $state(false);
  editEntityTarget = $state<{ type: "class" | "teacher" | "room" | "lesson"; data: any } | null>(null);

  readonly dayMap: Record<string, string> = {
    Poniedziałek: "Mon", Wtorek: "Tue", Środa: "Wed", Czwartek: "Thu", Piątek: "Fri", Sobota: "Sat", Niedziela: "Sun",
  };

  // GETTERY REAKTYWNE
  get currentDaysFormatted(): DayFormatted[] {
    return this.selectedDays.map((dName) => ({ id: this.dayMap[dName] || dName.slice(0, 3), name: dName }));
  }
  get availableGrades(): number[] { return getAvailableGrades(this.classes); }
  get totalHoursAssigned(): number { return getTotalHoursAssigned(this.lessons); }
  get totalTeacherWorkload(): number { return getTotalTeacherWorkload(this.teachers); }
  get weeklySlotsCapacity(): number { return this.selectedDays.length * this.periodsPerDay; }
  get isSchoolValid(): boolean {
    return Boolean(this.schoolName.trim()) && this.classes.length > 0 && this.teachers.length > 0 && this.lessons.length > 0;
  }
  get classesWithoutLessons(): SimpleClass[] {
    return this.classes.filter((c) => !this.lessons.some((l) => l.classId === c.id));
  }
  get validationIssues(): { step: number; text: string; action: string }[] {
    const issues: { step: number; text: string; action: string }[] = [];
    if (!this.schoolName.trim()) {
      issues.push({ step: 1, text: "Brak nazwy szkoły", action: "Wpisz nazwę w Kroku 1" });
    }
    if (this.classes.length === 0) {
      issues.push({ step: 2, text: "Brak oddziałów klasowych", action: "Dodaj klasy w Kroku 2" });
    }
    if (this.teachers.length === 0) {
      issues.push({ step: 2, text: "Brak kadry pedagogicznej", action: "Dodaj nauczycieli w Kroku 2" });
    }
    if (this.rooms.length === 0) {
      issues.push({ step: 3, text: "Brak sal lekcyjnych", action: "Dodaj sale w Kroku 3" });
    }
    if (this.lessons.length === 0) {
      issues.push({ step: 4, text: "Siatka lekcji jest pusta", action: "Wprowadź zajęcia w Kroku 4" });
    } else if (this.classesWithoutLessons.length > 0) {
      issues.push({
        step: 4,
        text: `Klasy bez lekcji: ${this.classesWithoutLessons.map((c) => c.name).join(", ")}`,
        action: "Przydziel lekcje dla tych klas w Kroku 4",
      });
    }
    return issues;
  }

  clearToEmptySchool() {
    this.schoolName = "";
    this.schoolId = "";
    this.classes = [];
    this.teachers = [];
    this.rooms = [];
    this.lessons = [];
    this.customSubjects = [];
  }
  get filteredLessons(): SimpleLesson[] {
    return filterLessons({
      lessons: this.lessons, classes: this.classes, teachers: this.teachers, rooms: this.rooms,
      searchQuery: this.lessonSearchQuery, filterClass: this.lessonFilterClass,
      filterTeacher: this.lessonFilterTeacher, filterGrade: this.lessonFilterGrade,
      filterPreference: this.lessonFilterPreference, getClassGrade: (c) => this.getClassGrade(c),
    });
  }
  get filteredLessonsHours(): number { return this.filteredLessons.reduce((s, l) => s + l.weeklyCount, 0); }
  get uniqueSubjects(): SubjectSummary[] { return computeUniqueSubjects(this.lessons, this.teachers, this.classes, this.customSubjects); }
  get allKnownSubjects(): string[] {
    const set = new Set<string>();
    // Domyślny kanon przedmiotów szkolnych
    const defaults = [
      "Język polski", "Język angielski", "Język niemiecki", "Matematyka", 
      "Fizyka", "Chemia", "Biologia", "Geografia", "Historia", 
      "Wiedza o społeczeństwie", "Informatyka", "Wychowanie Fizyczne", 
      "Plastyka", "Muzyka", "Filozofia", "Religia / Etyka", "Edukacja dla bezpieczeństwa"
    ];
    defaults.forEach((s) => set.add(s));
    this.customSubjects.forEach((s) => { if (s?.trim()) set.add(s.trim()); });
    this.teachers.forEach((t) => {
      if (t.subject?.trim()) set.add(t.subject.trim());
      if (Array.isArray(t.subjects)) {
        t.subjects.forEach((s) => { if (s?.trim()) set.add(s.trim()); });
      }
    });
    this.lessons.forEach((l) => { if (l.subject?.trim()) set.add(l.subject.trim()); });
    return Array.from(set).sort((a, b) => a.localeCompare(b, "pl"));
  }

  get availableRoomCategories(): string[] {
    const defaults = [
      "Ogólna / Dydaktyczna",
      "Sala gimnastyczna / Sportowa",
      "Pracownia komputerowa",
      "Laboratorium / Przyrodnicza",
      "Pracownia językowa",
    ];
    const set = new Set<string>(defaults);
    this.rooms.forEach((r) => {
      if (r.type?.trim()) set.add(r.type.trim());
    });
    return Array.from(set);
  }

  getTeacherSubjects(teacher: SimpleTeacher): string[] {
    const set = new Set<string>();
    if (teacher.subject?.trim()) set.add(teacher.subject.trim());
    if (Array.isArray(teacher.subjects)) {
      teacher.subjects.forEach((s) => {
        if (s?.trim()) set.add(s.trim());
      });
    }
    this.lessons
      .filter((l) => l.teacherId === teacher.id)
      .forEach((l) => {
        if (l.subject?.trim()) set.add(l.subject.trim());
      });
    return Array.from(set);
  }

  addSubjectToTeacher(teacherId: string, subject: string) {
    const trimmed = subject.trim();
    if (!trimmed) return;
    this.registerSubject(trimmed);
    const t = this.teachers.find((x) => x.id === teacherId);
    if (!t) return;
    const currentSubs = new Set(this.getTeacherSubjects(t));
    currentSubs.add(trimmed);
    t.subjects = Array.from(currentSubs);
    if (!t.subject) t.subject = trimmed;
    this.teachers = [...this.teachers];
  }

  removeSubjectFromTeacher(teacherId: string, subjectToRemove: string) {
    const t = this.teachers.find((x) => x.id === teacherId);
    if (!t) return;
    const currentSubs = this.getTeacherSubjects(t).filter((s) => s !== subjectToRemove);
    t.subjects = currentSubs;
    if (t.subject === subjectToRemove) {
      t.subject = currentSubs[0] || "";
    }
    this.teachers = [...this.teachers];
  }

  getQualifiedTeachersForSubject(subject: string): SimpleTeacher[] {
    const trimmed = subject?.trim().toLowerCase();
    if (!trimmed) return [];
    return this.teachers.filter((t) => {
      const subs = this.getTeacherSubjects(t).map((s) => s.toLowerCase());
      return subs.includes(trimmed);
    });
  }

  registerSubject(name: string) {
    const trimmed = name?.trim();
    if (!trimmed) return;
    if (!this.allKnownSubjects.some((s) => s.toLowerCase() === trimmed.toLowerCase())) {
      this.customSubjects = [...this.customSubjects, trimmed];
    }
  }
  get totalUnavailabilitiesCount(): number { return getTotalUnavailabilitiesCount(this.teachers, this.classes, this.rooms); }
  get resourcesWithUnavailability(): (SimpleTeacher | SimpleClass | SimpleRoom)[] {
    return getResourcesWithUnavailability(this.teachers, this.classes, this.rooms);
  }
  get teachersWithCustomDays(): SimpleTeacher[] { return getTeachersWithCustomDays(this.teachers, this.selectedDays.length); }
  get teachersWithCustomDailyLimit(): SimpleTeacher[] { return getTeachersWithCustomDailyLimit(this.teachers); }
  get teachersWithCustomGaps(): SimpleTeacher[] { return getTeachersWithCustomGaps(this.teachers); }
  get classesWithCustomDailyLimit(): SimpleClass[] { return getClassesWithCustomDailyLimit(this.classes); }

  // ROZPOZNAWANIE I RELACJE
  getClassGrade(cls: SimpleClass): number { return detectClassGrade(cls); }
  getClassWeeklyHours(classId: string): number {
    return this.lessons.filter((l) => l.classId === classId).reduce((s, l) => s + l.weeklyCount, 0);
  }
  getClassLessons(classId: string) { return this.lessons.filter((l) => l.classId === classId); }
  getTeacherAssignedHours(teacherId: string): number { return calculateTeacherAssignedHours(this.lessons, teacherId); }
  getTeacherClasses(teacherId: string) { return getTeacherClassAssignments(this.lessons, this.classes, teacherId); }
  getConsistentTeacherForClassSubject(classId: string, subject: string): string | null {
    return findConsistentTeacherForClassSubject(this.lessons, this.teachers, classId, subject);
  }
  getRoomLessons(roomId: string): SimpleLesson[] {
    return this.lessons.filter((l) => l.roomId === roomId);
  }
  getRoomAssignedHours(roomId: string): number {
    return this.lessons.filter((l) => l.roomId === roomId).reduce((s, l) => s + l.weeklyCount, 0);
  }

  // NAWIGACJA W TRYBIE STUDIO WORKBENCH (CROSS-LINKING)
  navigateToTeacher(teacherId: string) {
    this.selectedStudioTeacherId = teacherId;
    this.studioSection = "teachers";
  }
  navigateToClass(classId: string) {
    this.selectedStudioClassId = classId;
    this.studioSection = "classes";
  }
  navigateToRoom(roomId: string) {
    this.selectedStudioRoomId = roomId;
    this.studioSection = "rooms";
  }

  // BEZPOŚREDNIE PRZYPISANIE LEKCJI W KONTEKŚCIE OBIEKTU
  assignLessonToTeacher(teacherId: string, classId: string, subject: string, roomId: string, hours: number, isBlock: boolean) {
    const trimmed = subject?.trim();
    if (!trimmed || !classId || hours < 1) return;
    this.registerSubject(trimmed);
    const lessonId = `L_${classId}_${teacherId}_${trimmed.replace(/[\s/]/g, "_")}_${Date.now()}`;
    const newLesson: SimpleLesson = {
      id: lessonId,
      subject: trimmed,
      classId,
      teacherId,
      roomId: roomId || "",
      duration: isBlock ? 2 : 1,
      weeklyCount: hours,
      timePreference: "any",
    };
    this.lessons = [...this.lessons, newLesson];
  }

  assignLessonToClass(classId: string, subject: string, teacherId: string, roomId: string, hours: number, isBlock: boolean) {
    const trimmed = subject?.trim();
    if (!trimmed || hours < 1) return;
    this.registerSubject(trimmed);
    const teacherPart = teacherId || "unassigned";
    const lessonId = `L_${classId}_${teacherPart}_${trimmed.replace(/[\s/]/g, "_")}_${Date.now()}`;
    const newLesson: SimpleLesson = {
      id: lessonId,
      subject: trimmed,
      classId,
      teacherId: teacherId || "",
      roomId: roomId || "",
      duration: isBlock ? 2 : 1,
      weeklyCount: hours,
      timePreference: "any",
    };
    this.lessons = [...this.lessons, newLesson];
  }

  updateLesson(lessonId: string, patch: Partial<SimpleLesson>) {
    const index = this.lessons.findIndex((l) => l.id === lessonId);
    if (index === -1) return;
    const current = this.lessons[index];
    if (patch.subject?.trim()) {
      this.registerSubject(patch.subject.trim());
    }
    const updated: SimpleLesson = {
      ...current,
      ...patch,
      weeklyCount: patch.weeklyCount !== undefined ? Math.max(1, patch.weeklyCount) : current.weeklyCount,
    };
    this.lessons = [
      ...this.lessons.slice(0, index),
      updated,
      ...this.lessons.slice(index + 1),
    ];
  }

  deleteLesson(lessonId: string) {
    this.lessons = this.lessons.filter((l) => l.id !== lessonId);
  }

  // BEZPOŚREDNIA EDYCJA DOSTĘPNOŚCI OBIEKTU W WIDOKU 360°
  toggleEntitySlot(type: "teacher" | "class" | "room", entityId: string, slotId: string) {
    if (type === "teacher") {
      const t = this.teachers.find((x) => x.id === entityId);
      if (!t) return;
      const current = new Set(t.unavailableTimes || []);
      if (current.has(slotId)) current.delete(slotId); else current.add(slotId);
      t.unavailableTimes = Array.from(current);
      this.teachers = [...this.teachers];
    } else if (type === "class") {
      const c = this.classes.find((x) => x.id === entityId);
      if (!c) return;
      const current = new Set(c.unavailableTimes || []);
      if (current.has(slotId)) current.delete(slotId); else current.add(slotId);
      c.unavailableTimes = Array.from(current);
      this.classes = [...this.classes];
    } else if (type === "room") {
      const r = this.rooms.find((x) => x.id === entityId);
      if (!r) return;
      const current = new Set(r.unavailableTimes || []);
      if (current.has(slotId)) current.delete(slotId); else current.add(slotId);
      r.unavailableTimes = Array.from(current);
      this.rooms = [...this.rooms];
    }
  }

  toggleEntityDay(type: "teacher" | "class" | "room", entityId: string, dayId: string) {
    const slotsForDay: string[] = [];
    for (let p = 1; p <= this.periodsPerDay; p++) {
      slotsForDay.push(`${dayId}_${p}`);
    }
    const getSet = () => {
      if (type === "teacher") return new Set(this.teachers.find((x) => x.id === entityId)?.unavailableTimes || []);
      if (type === "class") return new Set(this.classes.find((x) => x.id === entityId)?.unavailableTimes || []);
      return new Set(this.rooms.find((x) => x.id === entityId)?.unavailableTimes || []);
    };
    const current = getSet();
    const allBlocked = slotsForDay.every((s) => current.has(s));
    slotsForDay.forEach((s) => {
      if (allBlocked) current.delete(s); else current.add(s);
    });
    const arr = Array.from(current);
    if (type === "teacher") {
      const t = this.teachers.find((x) => x.id === entityId);
      if (t) { t.unavailableTimes = arr; this.teachers = [...this.teachers]; }
    } else if (type === "class") {
      const c = this.classes.find((x) => x.id === entityId);
      if (c) { c.unavailableTimes = arr; this.classes = [...this.classes]; }
    } else if (type === "room") {
      const r = this.rooms.find((x) => x.id === entityId);
      if (r) { r.unavailableTimes = arr; this.rooms = [...this.rooms]; }
    }
  }

  toggleEntityPeriod(type: "teacher" | "class" | "room", entityId: string, period: number) {
    const slotsForPeriod = this.currentDaysFormatted.map((d) => `${d.id}_${period}`);
    const getSet = () => {
      if (type === "teacher") return new Set(this.teachers.find((x) => x.id === entityId)?.unavailableTimes || []);
      if (type === "class") return new Set(this.classes.find((x) => x.id === entityId)?.unavailableTimes || []);
      return new Set(this.rooms.find((x) => x.id === entityId)?.unavailableTimes || []);
    };
    const current = getSet();
    const allBlocked = slotsForPeriod.every((s) => current.has(s));
    slotsForPeriod.forEach((s) => {
      if (allBlocked) current.delete(s); else current.add(s);
    });
    const arr = Array.from(current);
    if (type === "teacher") {
      const t = this.teachers.find((x) => x.id === entityId);
      if (t) { t.unavailableTimes = arr; this.teachers = [...this.teachers]; }
    } else if (type === "class") {
      const c = this.classes.find((x) => x.id === entityId);
      if (c) { c.unavailableTimes = arr; this.classes = [...this.classes]; }
    } else if (type === "room") {
      const r = this.rooms.find((x) => x.id === entityId);
      if (r) { r.unavailableTimes = arr; this.rooms = [...this.rooms]; }
    }
  }

  clearEntityAvailability(type: "teacher" | "class" | "room", entityId: string) {
    if (type === "teacher") {
      const t = this.teachers.find((x) => x.id === entityId);
      if (t) { t.unavailableTimes = []; this.teachers = [...this.teachers]; }
    } else if (type === "class") {
      const c = this.classes.find((x) => x.id === entityId);
      if (c) { c.unavailableTimes = []; this.classes = [...this.classes]; }
    } else if (type === "room") {
      const r = this.rooms.find((x) => x.id === entityId);
      if (r) { r.unavailableTimes = []; this.rooms = [...this.rooms]; }
    }
  }

  // MUTACJE: ZMIANA NAZW I EDYCJA
  renameSubject(oldName: string, newName: string) {
    const res = applySubjectRename(oldName, newName, this.lessons, this.teachers, this.orderRules);
    this.lessons = res.lessons; this.teachers = res.teachers; this.orderRules = res.orderRules;
    if (this.batchGradeSubject === oldName) this.batchGradeSubject = newName.trim();
    if (this.lessonSubject === oldName) this.lessonSubject = newName.trim();
    if (this.newTeacherSubject === oldName) this.newTeacherSubject = newName.trim();
  }
  mergeSubjects(sourceSubject: string, targetSubject: string) { this.renameSubject(sourceSubject, targetSubject); }

  updateClass(classId: string, updated: Partial<SimpleClass>) {
    const res = applyClassUpdate(classId, updated, this.classes, this.lessons);
    this.classes = res.classes; this.lessons = res.lessons;
    if (this.selectedClassViewId === classId) this.selectedClassViewId = res.newId;
    if (this.lessonClassId === classId) this.lessonClassId = res.newId;
  }
  updateTeacher(teacherId: string, updated: Partial<SimpleTeacher>) {
    const res = applyTeacherUpdate(teacherId, updated, this.teachers, this.lessons);
    this.teachers = res.teachers; this.lessons = res.lessons;
    if (this.selectedTeacherViewId === teacherId) this.selectedTeacherViewId = res.newId;
    if (this.lessonTeacherId === teacherId) this.lessonTeacherId = res.newId;
  }
  updateRoom(roomId: string, updated: Partial<SimpleRoom>) {
    const res = applyRoomUpdate(roomId, updated, this.rooms, this.lessons);
    this.rooms = res.rooms; this.lessons = res.lessons;
    if (this.lessonRoomId === roomId) this.lessonRoomId = res.newId;
  }

  // MUTACJE KLAS
  addClass() { executeAddClass(this); }
  removeClass(id: string) { executeRemoveClass(this, id); }
  duplicateClass(sourceClassId: string) { executeDuplicateClass(this, sourceClassId); }
  duplicateCurriculum(sourceClassId: string, targetClassIds: string[]) {
    executeDuplicateCurriculum(this, sourceClassId, targetClassIds);
  }
  addSubjectToGrade(grade: number, subject: string, weeklyCount: number, isBlock: boolean = false) {
    executeAddSubjectToGrade(this, grade, subject, weeklyCount, isBlock);
  }

  // MUTACJE NAUCZYCIELI I SAL
  addTeacher() { executeAddTeacher(this); }
  removeTeacher(id: string) { executeRemoveTeacher(this, id); }
  updateTeacherWorkload(teacherId: string, newWorkload: number) {
    const val = Math.max(1, Number(newWorkload) || 18);
    this.teachers = this.teachers.map((t) => (t.id === teacherId ? { ...t, workload: val } : t));
  }
  addRoom() { executeAddRoom(this); }
  removeRoom(id: string) { executeRemoveRoom(this, id); }

  // MUTACJE LEKCJI
  addCustomLesson() { executeAddCustomLesson(this); }
  removeLesson(id: string) { executeRemoveLesson(this, id); }
  setQuickLessonPref(lessonId: string, pref: "any" | "morning" | "afternoon") {
    this.lessons = applyQuickPrefToLesson(lessonId, this.lessons, pref, this.currentDaysFormatted, this.periodsPerDay);
  }
  applyBatchPreference(pref: "any" | "morning" | "afternoon") {
    this.lessons = applyBatchPrefToLessons(this.selectedLessonIds, this.lessons, pref, this.currentDaysFormatted, this.periodsPerDay);
    this.selectedLessonIds = new Set();
  }
  toggleLessonSelection(id: string) {
    this.selectedLessonIds = toggleIdInSet(this.selectedLessonIds, id);
  }
  selectAllFilteredLessons() {
    this.selectedLessonIds = extractAllLessonIds(this.filteredLessons);
  }
  clearSelectedLessons() { this.selectedLessonIds = new Set(); }
  clearLessonFilters() {
    const f = resetFiltersState();
    this.lessonSearchQuery = f.searchQuery;
    this.lessonFilterClass = f.filterClass;
    this.lessonFilterTeacher = f.filterTeacher;
    this.lessonFilterGrade = f.filterGrade;
    this.lessonFilterPreference = f.filterPreference;
  }

  // MODALE DOSTĘPNOŚCI I PREFERENCJI
  openAvailabilityModal(id: string, name: string, type: "teacher" | "room" | "class") {
    executeOpenAvailability(this, id, name, type);
  }
  handleSaveAvailability(
    unavailableTimes: string[], isSoft: boolean, weight: number,
    extraParams?: { maxDaysPerWeek?: number | null; minDaysPerWeek?: number | null; maxDailyLessons?: number | null; maxGapsPerWeek?: number | null; minDailyLessons?: number | null; }
  ) {
    executeSaveAvailability(this, unavailableTimes, isSoft, weight, extraParams);
  }
  openLessonPrefModal(lesson: SimpleLesson) {
    executeOpenLessonPref(this, lesson);
  }
  handleSaveLessonPref(pref: "any" | "morning" | "afternoon" | "custom", times: string[], isSoft: boolean, weight: number) {
    executeSaveLessonPref(this, pref, times, isSoft, weight);
  }

  // REGUŁY KOLEJNOŚCI
  addOrderRule() { executeAddOrderRule(this); }
  removeOrderRule(ruleId: string) { executeRemoveOrderRule(this, ruleId); }

  // PRESETY I ŁADOWANIE
  resetToNewSchool(customName?: string) {
    this.schoolId = `szkola_${Date.now().toString().slice(-6)}`;
    this.schoolName = customName || "Nowa Placówka Oświatowa";
    this.selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
    this.periodsPerDay = 8;
    this.classes = createDefaultClasses();
    this.teachers = createDefaultTeachers();
    this.rooms = createDefaultRooms();
    this.linkedGroups = [];
    this.orderRules = [];
    this.ruleOrderLessons = false;
    this.externalConstraintsNotice = [];
    this.generateDefaultCurriculum();
    this.studioSection = "teachers";
  }
  loadHighSchoolPreset() {
    const p = getHighSchoolPresetBasics();
    this.schoolName = p.schoolName;
    this.schoolId = p.schoolId;
    this.selectedDays = p.selectedDays;
    this.periodsPerDay = p.periodsPerDay;
    this.linkedGroups = [];
    this.orderRules = [];
    this.ruleOrderLessons = false;
    this.externalConstraintsNotice = [];
    this.generateDefaultCurriculum();
  }
  generateDefaultCurriculum() {
    this.lessons = generateDefaultCurriculumLessons(this.classes, this.teachers, this.rooms);
  }
  loadFromExistingInstance(inst: InstanceDetail) { loadInstanceIntoEditor(this, inst); }
  buildXhsttPayload(): InstanceDetail { return buildPayloadFromEditor(this); }
}

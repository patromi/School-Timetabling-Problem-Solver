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
    this.teachers.forEach((t) => { if (t.subject?.trim()) set.add(t.subject.trim()); });
    this.lessons.forEach((l) => { if (l.subject?.trim()) set.add(l.subject.trim()); });
    return Array.from(set).sort((a, b) => a.localeCompare(b, "pl"));
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
  updateLesson(lessonId: string, updated: Partial<SimpleLesson>) {
    this.lessons = this.lessons.map((l) => (l.id === lessonId ? { ...l, ...updated } : l));
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
  loadHighSchoolPreset() {
    const p = getHighSchoolPresetBasics();
    this.schoolName = p.schoolName;
    this.schoolId = p.schoolId;
    this.selectedDays = p.selectedDays;
    this.periodsPerDay = p.periodsPerDay;
    this.generateDefaultCurriculum();
  }
  generateDefaultCurriculum() {
    this.lessons = generateDefaultCurriculumLessons(this.classes, this.teachers, this.rooms);
  }
  loadFromExistingInstance(inst: InstanceDetail) { loadInstanceIntoEditor(this, inst); }
  buildXhsttPayload(): InstanceDetail { return buildPayloadFromEditor(this); }
}

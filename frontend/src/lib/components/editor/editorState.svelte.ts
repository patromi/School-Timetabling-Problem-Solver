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
  generateDefaultCurriculumForClasses,
  duplicateClassWithLessons,
  duplicateCurriculumBetweenClasses,
  addSubjectToGradeClasses,
} from "./state/curriculumPresets";
import {
  buildAvailabilityTarget,
  updateResourceAvailability,
  buildLessonPrefTarget,
  updateLessonPreferences,
} from "./state/modalHandlers";
import {
  applyQuickPrefToLesson,
  applyBatchPrefToLessons,
} from "./state/lessonPreferencesActions";
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
  addClassToState,
  removeClassFromState,
  addTeacherToState,
  removeTeacherFromState,
  addRoomToState,
  removeRoomFromState,
  addLessonToState,
  removeLessonFromState,
} from "./state/entityMutations";
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
  createOrderRule,
  removeOrderRuleFromList,
} from "./state/orderRulesActions";

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
  get uniqueSubjects(): SubjectSummary[] { return computeUniqueSubjects(this.lessons, this.teachers, this.classes); }
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
  addClass() {
    const res = addClassToState(this.classes, this.newClassName, this.newClassGrade);
    if (res.error) return alert(res.error);
    this.classes = res.classes; this.newClassName = "";
  }
  removeClass(id: string) {
    const res = removeClassFromState(this.classes, this.lessons, this.orderRules, id);
    this.classes = res.classes; this.lessons = res.lessons; this.orderRules = res.orderRules;
  }
  duplicateClass(sourceClassId: string) {
    const res = duplicateClassWithLessons(sourceClassId, this.classes, this.lessons);
    if (!res) return;
    this.classes = res.newClasses; this.lessons = res.newLessons; this.selectedClassViewId = res.newClassId;
  }
  duplicateCurriculum(sourceClassId: string, targetClassIds: string[]) {
    this.lessons = duplicateCurriculumBetweenClasses(
      sourceClassId, targetClassIds, this.lessons, (cId, sub) => this.getConsistentTeacherForClassSubject(cId, sub)
    );
  }
  addSubjectToGrade(grade: number, subject: string, weeklyCount: number, isBlock: boolean = false) {
    this.lessons = addSubjectToGradeClasses({
      grade, subject, weeklyCount, isBlock, classes: this.classes, teachers: this.teachers, rooms: this.rooms,
      lessons: this.lessons, getClassGrade: (c) => this.getClassGrade(c),
      getConsistentTeacher: (cId, sub) => this.getConsistentTeacherForClassSubject(cId, sub),
    });
  }

  // MUTACJE NAUCZYCIELI I SAL
  addTeacher() {
    const res = addTeacherToState(this.teachers, this.newTeacherName, this.newTeacherSubject, this.newTeacherWorkload);
    if (res.error) return alert(res.error);
    this.teachers = res.teachers; this.newTeacherName = "";
  }
  removeTeacher(id: string) { this.teachers = removeTeacherFromState(this.teachers, id); }
  updateTeacherWorkload(teacherId: string, newWorkload: number) {
    const val = Math.max(1, Number(newWorkload) || 18);
    this.teachers = this.teachers.map((t) => (t.id === teacherId ? { ...t, workload: val } : t));
  }
  addRoom() {
    const res = addRoomToState(this.rooms, this.newRoomName, this.newRoomType);
    if (res.error) return alert(res.error);
    this.rooms = res.rooms; this.newRoomName = "";
  }
  removeRoom(id: string) { this.rooms = removeRoomFromState(this.rooms, id); }

  // MUTACJE LEKCJI
  addCustomLesson() {
    const res = addLessonToState(this.lessons, this.rooms, {
      subject: this.lessonSubject, classId: this.lessonClassId, teacherId: this.lessonTeacherId,
      roomId: this.lessonRoomId, duration: this.lessonIsBlock ? 2 : 1, weeklyCount: this.lessonCount,
    });
    if (res.error) return alert(res.error);
    this.lessons = res.lessons;
  }
  removeLesson(id: string) {
    const res = removeLessonFromState(this.lessons, this.orderRules, id);
    this.lessons = res.lessons; this.orderRules = res.orderRules;
  }
  setQuickLessonPref(lessonId: string, pref: "any" | "morning" | "afternoon") {
    this.lessons = applyQuickPrefToLesson(lessonId, this.lessons, pref, this.currentDaysFormatted, this.periodsPerDay);
  }
  applyBatchPreference(pref: "any" | "morning" | "afternoon") {
    this.lessons = applyBatchPrefToLessons(this.selectedLessonIds, this.lessons, pref, this.currentDaysFormatted, this.periodsPerDay);
    this.selectedLessonIds = new Set();
  }
  toggleLessonSelection(id: string) {
    const next = new Set(this.selectedLessonIds);
    if (next.has(id)) next.delete(id); else next.add(id);
    this.selectedLessonIds = next;
  }
  selectAllFilteredLessons() { this.selectedLessonIds = new Set(this.filteredLessons.map((l) => l.id)); }
  clearSelectedLessons() { this.selectedLessonIds = new Set(); }
  clearLessonFilters() {
    this.lessonSearchQuery = ""; this.lessonFilterClass = "all";
    this.lessonFilterTeacher = "all"; this.lessonFilterGrade = null; this.lessonFilterPreference = "all";
  }

  // MODALE DOSTĘPNOŚCI I PREFERENCJI
  openAvailabilityModal(id: string, name: string, type: "teacher" | "room" | "class") {
    this.activeAvailabilityTarget = buildAvailabilityTarget({
      id, name, type, teachers: this.teachers, classes: this.classes, rooms: this.rooms, lessons: this.lessons,
      getTeacherAssignedHours: (tId) => this.getTeacherAssignedHours(tId),
      getTeacherClasses: (tId) => this.getTeacherClasses(tId),
      getClassLessons: (cId) => this.getClassLessons(cId),
      getClassWeeklyHours: (cId) => this.getClassWeeklyHours(cId),
    });
  }
  handleSaveAvailability(
    unavailableTimes: string[], isSoft: boolean, weight: number,
    extraParams?: { maxDaysPerWeek?: number | null; minDaysPerWeek?: number | null; maxDailyLessons?: number | null; maxGapsPerWeek?: number | null; minDailyLessons?: number | null; }
  ) {
    if (!this.activeAvailabilityTarget) return;
    const res = updateResourceAvailability({
      target: this.activeAvailabilityTarget, teachers: this.teachers, classes: this.classes, rooms: this.rooms,
      unavailableTimes, isSoft, weight, extraParams,
    });
    this.teachers = res.teachers; this.classes = res.classes; this.rooms = res.rooms;
    this.activeAvailabilityTarget = null;
  }
  openLessonPrefModal(lesson: SimpleLesson) {
    this.activeLessonPrefTarget = buildLessonPrefTarget(lesson, this.classes);
  }
  handleSaveLessonPref(pref: "any" | "morning" | "afternoon" | "custom", times: string[], isSoft: boolean, weight: number) {
    if (!this.activeLessonPrefTarget) return;
    this.lessons = updateLessonPreferences(this.activeLessonPrefTarget.lessonId, this.lessons, pref, times, isSoft, weight);
    this.activeLessonPrefTarget = null;
  }

  // REGUŁY KOLEJNOŚCI
  addOrderRule() {
    const res = createOrderRule(this.lessons, this.newOrderFirstLesson, this.newOrderSecondLesson, this.newOrderSeparation, this.newOrderIsSoft, this.newOrderWeight);
    if (res.error) return alert(res.error);
    if (res.rule) this.orderRules = [...this.orderRules, res.rule];
    this.newOrderFirstLesson = ""; this.newOrderSecondLesson = "";
  }
  removeOrderRule(ruleId: string) { this.orderRules = removeOrderRuleFromList(this.orderRules, ruleId); }

  // PRESETY I ŁADOWANIE
  loadHighSchoolPreset() {
    this.schoolName = "Liceum Ogólnokształcące (profilowane)";
    this.schoolId = "LO_Profilowane";
    this.selectedDays = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek"];
    this.periodsPerDay = 8;
    this.generateDefaultCurriculum();
  }
  generateDefaultCurriculum() {
    this.lessons = generateDefaultCurriculumForClasses(this.classes, this.teachers, this.rooms);
  }
  loadFromExistingInstance(inst: InstanceDetail) { loadInstanceIntoEditor(this, inst); }
  buildXhsttPayload(): InstanceDetail { return buildPayloadFromEditor(this); }
}

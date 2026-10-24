<!-- frontend/src/lib/components/editor/InstanceEditor.svelte -->
<script lang="ts">
  import type { InstanceDetail } from "../../types";
  import { saveInstance } from "../../api";
  import { EditorState } from "./editorState.svelte";
  import AvailabilityModal from "./AvailabilityModal.svelte";
  import LessonPreferencesModal from "./LessonPreferencesModal.svelte";
  import LinkedLessonsModal from "./LinkedLessonsModal.svelte";
  import DuplicateCurriculumModal from "./DuplicateCurriculumModal.svelte";
  import SubjectManagerModal from "./SubjectManagerModal.svelte";
  import EditEntityModal from "./EditEntityModal.svelte";
  import Step1SchoolBasics from "./steps/Step1SchoolBasics.svelte";
  import Step2Classes from "./steps/Step2Classes.svelte";
  import Step3TeachersRooms from "./steps/Step3TeachersRooms.svelte";
  import Step4Lessons from "./steps/Step4Lessons.svelte";
  import Step5Constraints from "./steps/Step5Constraints.svelte";
  import Step6SummaryExport from "./steps/Step6SummaryExport.svelte";

  interface Props {
    initialInstance?: InstanceDetail | null;
    onSaved: (savedId: string) => void;
    onCancel?: () => void;
  }

  let { initialInstance = null, onSaved, onCancel }: Props = $props();

  const editor = new EditorState();

  let currentStep = $state<number>(1);
  let isSaving = $state(false);
  let saveError = $state<string | null>(null);
  let saveSuccess = $state<string | null>(null);

  $effect(() => {
    if (initialInstance) {
      editor.loadFromExistingInstance(initialInstance);
    } else if (editor.lessons.length === 0) {
      editor.generateDefaultCurriculum();
    }
  });

  async function handleSaveSchool() {
    if (!editor.isSchoolValid) {
      saveError = "Proszę uzupełnić nazwę szkoły oraz upewnić się, że dodano klasy, nauczycieli i lekcje.";
      return;
    }

    isSaving = true;
    saveError = null;
    saveSuccess = null;

    try {
      const payload = editor.buildXhsttPayload();
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
        onclick={() => editor.loadHighSchoolPreset()}
        class="px-3.5 py-1.5 bg-amber-50 hover:bg-amber-100 text-amber-900 border border-amber-200/70 text-xs font-semibold rounded-lg transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs"
      >
        <span>⚡ Wstaw gotowy szablon szkoły</span>
      </button>

      <button
        type="button"
        onclick={handleSaveSchool}
        disabled={isSaving || !editor.isSchoolValid}
        class={`px-4 py-1.5 text-xs font-bold rounded-lg transition-all shadow-xs flex items-center gap-1.5 cursor-pointer ${
          editor.isSchoolValid
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
      {#if editor.isSchoolValid}
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
    {#if currentStep === 1}
      <Step1SchoolBasics editor={editor} />
    {:else if currentStep === 2}
      <Step2Classes editor={editor} />
    {:else if currentStep === 3}
      <Step3TeachersRooms editor={editor} />
    {:else if currentStep === 4}
      <Step4Lessons editor={editor} />
    {:else if currentStep === 5}
      <Step5Constraints editor={editor} />
    {:else if currentStep === 6}
      <Step6SummaryExport editor={editor} isSaving={isSaving} onSave={handleSaveSchool} />
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
        disabled={isSaving || !editor.isSchoolValid}
        class="px-5 py-2 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-lg transition-all cursor-pointer shadow-xs"
      >
        💾 Zapisz szkołę
      </button>
    {/if}
  </footer>

  <!-- Modal Availability Grid (Phase 1 & 2) -->
  {#if editor.activeAvailabilityTarget}
    <AvailabilityModal
      resourceId={editor.activeAvailabilityTarget.id}
      resourceName={editor.activeAvailabilityTarget.name}
      resourceType={editor.activeAvailabilityTarget.type}
      days={editor.currentDaysFormatted}
      periodsPerDay={editor.periodsPerDay}
      initialUnavailableTimes={editor.activeAvailabilityTarget.unavailableTimes}
      initialIsSoft={editor.activeAvailabilityTarget.isSoft}
      initialWeight={editor.activeAvailabilityTarget.weight}
      initialMaxDaysPerWeek={editor.activeAvailabilityTarget.maxDaysPerWeek}
      initialMinDaysPerWeek={editor.activeAvailabilityTarget.minDaysPerWeek}
      initialMaxDailyLessons={editor.activeAvailabilityTarget.maxDailyLessons}
      initialMaxGapsPerWeek={editor.activeAvailabilityTarget.maxGapsPerWeek}
      initialMinDailyLessons={editor.activeAvailabilityTarget.minDailyLessons}
      contextInfo={editor.activeAvailabilityTarget.contextInfo}
      onSave={(times, isSoft, weight, extra) => editor.handleSaveAvailability(times, isSoft, weight, extra)}
      onClose={() => (editor.activeAvailabilityTarget = null)}
    />
  {/if}

  <!-- Modal Preferencji Godzinowych dla Lekcji (Phase 3) -->
  {#if editor.activeLessonPrefTarget}
    <LessonPreferencesModal
      lessonId={editor.activeLessonPrefTarget.lessonId}
      lessonSubject={editor.activeLessonPrefTarget.subject}
      className={editor.activeLessonPrefTarget.className}
      days={editor.currentDaysFormatted}
      periodsPerDay={editor.periodsPerDay}
      initialPreference={editor.activeLessonPrefTarget.preference}
      initialPreferredTimes={editor.activeLessonPrefTarget.preferredTimes}
      initialIsSoft={editor.activeLessonPrefTarget.isSoft}
      initialWeight={editor.activeLessonPrefTarget.weight}
      onSave={(pref, times, isSoft, weight) => editor.handleSaveLessonPref(pref, times, isSoft, weight)}
      onClose={() => (editor.activeLessonPrefTarget = null)}
    />
  {/if}

  <!-- Modal Lekcji Równoległych / Jednoczesnych (Phase 3) -->
  {#if editor.isLinkedModalOpen}
    <LinkedLessonsModal
      lessons={editor.lessons}
      classes={editor.classes}
      teachers={editor.teachers}
      initialGroups={editor.linkedGroups}
      onSave={(updatedGroups) => {
        editor.linkedGroups = updatedGroups;
        editor.isLinkedModalOpen = false;
      }}
      onClose={() => (editor.isLinkedModalOpen = false)}
    />
  {/if}

  <!-- Modal Duplikowania Siatki Zajęć (Krok 2 & Krok 4) -->
  {#if editor.duplicateModalTarget}
    {@const target = editor.duplicateModalTarget}
    <DuplicateCurriculumModal
      sourceClass={target}
      classes={editor.classes}
      sourceHours={editor.getClassWeeklyHours(target.id)}
      sourceLessonsCount={editor.getClassLessons(target.id).length}
      onDuplicate={(targetIds) => editor.duplicateCurriculum(target.id, targetIds)}
      onClose={() => (editor.duplicateModalTarget = null)}
    />
  {/if}

  <!-- Modal Menedżera Przedmiotów (Renaming & Merging) -->
  {#if editor.isSubjectManagerOpen}
    <SubjectManagerModal
      editor={editor}
      onClose={() => (editor.isSubjectManagerOpen = false)}
    />
  {/if}

  <!-- Modal Edycji Encji (Klasa, Nauczyciel, Sala, Lekcja) -->
  {#if editor.editEntityTarget}
    <EditEntityModal
      editor={editor}
      target={editor.editEntityTarget}
      onClose={() => (editor.editEntityTarget = null)}
    />
  {/if}
</div>

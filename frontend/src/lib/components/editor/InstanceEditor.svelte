<!-- frontend/src/lib/components/editor/InstanceEditor.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail } from "../../types";
  import { saveInstance } from "../../api";
  import { EditorState } from "./editorState.svelte";
  import EditorWizardNav from "./EditorWizardNav.svelte";
  import EditorModalsHost from "./EditorModalsHost.svelte";
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

  onMount(() => {
    if (typeof window !== "undefined") {
      const params = new URLSearchParams(window.location.search);
      const s = params.get("step");
      if (s) {
        const parsed = parseInt(s, 10);
        if (parsed >= 1 && parsed <= 6) currentStep = parsed;
      }
      const tab = params.get("tab");
      if (tab === "teachers" || tab === "classes") {
        editor.step2ActiveTab = tab;
      }
      if (tab === "lessons" || tab === "byClass" || tab === "byTeacher") {
        editor.step4ActiveTab = tab;
      }
    }
  });

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
  <EditorWizardNav {editor} bind:currentStep />

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
      <Step1SchoolBasics {editor} />
    {:else if currentStep === 2}
      <Step2Classes {editor} />
    {:else if currentStep === 3}
      <Step3TeachersRooms {editor} />
    {:else if currentStep === 4}
      <Step4Lessons {editor} />
    {:else if currentStep === 5}
      <Step5Constraints {editor} />
    {:else if currentStep === 6}
      <Step6SummaryExport {editor} {isSaving} onSave={handleSaveSchool} />
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

  <!-- Modals Host -->
  <EditorModalsHost {editor} />
</div>

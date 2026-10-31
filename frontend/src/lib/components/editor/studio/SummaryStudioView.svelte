<!-- frontend/src/lib/components/editor/studio/SummaryStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
    isSaving: boolean;
    onSave: () => void;
  }

  let { editor, isSaving, onSave }: Props = $props();

  let anyTeacherLessonsCount = $derived(editor.lessons.filter((l) => l.teacherId === "__ANY__").length);
</script>

<div class="flex flex-col gap-6">
  <!-- Header -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col sm:flex-row sm:items-center justify-between gap-4">
    <div>
      <div class="flex items-center gap-2">
        <h3 class="text-xl font-bold text-slate-900">{editor.schoolName || "Nowa szkoła"}</h3>
        <span class="text-[11px] font-mono px-2 py-0.5 rounded-full bg-slate-100 text-slate-600 border border-slate-200">
          {editor.schoolId || "LO_1"}
        </span>
      </div>
      <p class="text-xs text-slate-500 mt-0.5">
        Podsumowanie kompletności arkusza organizacyjnego i gotowość do generowania planu lekcji.
      </p>
    </div>

    <div>
      {#if editor.isSchoolValid}
        <span class="px-3.5 py-1.5 rounded-xl text-xs font-bold bg-emerald-50 text-emerald-800 border border-emerald-200 flex items-center gap-1.5 shadow-2xs">
          <span>✓</span>
          <span>Arkusz gotowy do zapisu</span>
        </span>
      {:else}
        <span class="px-3.5 py-1.5 rounded-xl text-xs font-bold bg-amber-50 text-amber-800 border border-amber-200 flex items-center gap-1.5 shadow-2xs">
          <span>⚠️</span>
          <span>Wymaga uzupełnienia ({editor.validationIssues.length})</span>
        </span>
      {/if}
    </div>
  </div>

  <!-- KPI Cards -->
  <div class="grid grid-cols-2 lg:grid-cols-4 gap-4 text-xs">
    <div class="bg-white p-5 rounded-2xl border border-slate-200/90 shadow-2xs flex flex-col justify-between">
      <span class="text-slate-400 font-bold uppercase tracking-wider text-[10px]">Oddziały klasowe:</span>
      <div class="my-2">
        <span class="text-2xl font-extrabold text-slate-900">{editor.classes.length}</span>
        <span class="text-slate-500 font-medium ml-1">oddziałów</span>
      </div>
      <span class="text-[11px] text-slate-400">
        Klasy bez lekcji: <strong class="{editor.classesWithoutLessons.length > 0 ? 'text-rose-600' : 'text-emerald-700'}">{editor.classesWithoutLessons.length}</strong>
      </span>
    </div>

    <div class="bg-white p-5 rounded-2xl border border-slate-200/90 shadow-2xs flex flex-col justify-between">
      <span class="text-slate-400 font-bold uppercase tracking-wider text-[10px]">Kadra pedagogiczna:</span>
      <div class="my-2">
        <span class="text-2xl font-extrabold text-slate-900">{editor.teachers.length}</span>
        <span class="text-slate-500 font-medium ml-1">nauczycieli</span>
      </div>
      <span class="text-[11px] text-slate-400">
        Łączne pensum: <strong>{editor.totalTeacherWorkload}h</strong>
      </span>
    </div>

    <div class="bg-white p-5 rounded-2xl border border-slate-200/90 shadow-2xs flex flex-col justify-between">
      <span class="text-slate-400 font-bold uppercase tracking-wider text-[10px]">Sale lekcyjne:</span>
      <div class="my-2">
        <span class="text-2xl font-extrabold text-slate-900">{editor.rooms.length}</span>
        <span class="text-slate-500 font-medium ml-1">pomieszczeń</span>
      </div>
      <span class="text-[11px] text-slate-400">
        Baza dydaktyczna i sportowa
      </span>
    </div>

    <div class="bg-white p-5 rounded-2xl border border-slate-200/90 shadow-2xs flex flex-col justify-between">
      <span class="text-slate-400 font-bold uppercase tracking-wider text-[10px]">Wymiar godzinowy:</span>
      <div class="my-2">
        <span class="text-2xl font-extrabold text-blue-700">{editor.totalHoursAssigned}</span>
        <span class="text-slate-500 font-medium ml-1">godz./tydz.</span>
      </div>
      <span class="text-[11px] text-slate-400">
        Lekcje wprowadzone w siatce
      </span>
    </div>
  </div>

  <!-- Validation Issues (if any) -->
  {#if editor.validationIssues.length > 0}
    <div class="p-5 bg-amber-50/80 border border-amber-200 rounded-2xl flex flex-col gap-3 text-xs">
      <span class="font-bold text-amber-900 flex items-center gap-1.5 text-sm">
        <span>⚠️</span>
        <span>Przed zapisem arkusza należy uzupełnić następujące pozycje:</span>
      </span>
      <div class="divide-y divide-amber-200/60 bg-white/70 rounded-xl border border-amber-200/60 overflow-hidden">
        {#each editor.validationIssues as issue}
          <div class="p-3 flex items-center justify-between text-xs">
            <span class="font-bold text-slate-900">{issue.text}</span>
            <span class="text-[11px] text-amber-800">{issue.action}</span>
          </div>
        {/each}
      </div>
    </div>
  {/if}

  <!-- Audit Checklist -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-3.5 text-xs">
    <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
      Raport gotowości instancji:
    </span>

    <div class="flex flex-col gap-2 text-slate-700">
      <div class="flex items-center gap-2 text-emerald-800">
        <span>✓</span>
        <span>Struktura tygodnia: {editor.selectedDays.length} dni roboczych ({editor.periodsPerDay} lekcji dziennie)</span>
      </div>

      {#if editor.classes.length > 0}
        <div class="flex items-center gap-2 {editor.classesWithoutLessons.length > 0 ? 'text-amber-800' : 'text-emerald-800'}">
          <span>{editor.classesWithoutLessons.length > 0 ? '⚠️' : '✓'}</span>
          <span>
            {editor.classes.length} oddziałów klasowych
            {#if editor.classesWithoutLessons.length > 0}
              (uwaga: {editor.classesWithoutLessons.length} klas nie ma jeszcze przypisanych lekcji)
            {/if}
          </span>
        </div>
      {/if}

      {#if editor.teachers.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>{editor.teachers.length} pedagogów (łączne zapotrzebowanie: {editor.totalHoursAssigned} z {editor.totalTeacherWorkload} godz. pensum)</span>
        </div>
      {/if}

      {#if editor.rooms.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>{editor.rooms.length} sal lekcyjnych zdefiniowanych w bazie</span>
        </div>
      {/if}

      {#if anyTeacherLessonsCount > 0}
        <div class="flex items-center gap-2 text-indigo-900 bg-indigo-50/80 p-2.5 rounded-xl border border-indigo-200">
          <span class="text-base">🎲</span>
          <span>
            <strong>{anyTeacherLessonsCount} lekcji</strong> z opcją <strong>"Dowolny nauczyciel"</strong> — solver dobierze pedagogów z uprawnieniami podczas optymalizacji.
          </span>
        </div>
      {/if}
    </div>

    <!-- CTA Save Button -->
    <div class="pt-5 mt-2 border-t border-slate-100 flex items-center justify-between">
      <p class="text-xs text-slate-500">
        Po zapisaniu szkoła pojawi się w przeglądarce i będzie gotowa do generowania planu w solverze.
      </p>

      <button
        type="button"
        onclick={onSave}
        disabled={isSaving || !editor.isSchoolValid}
        class={`px-6 py-2.5 rounded-xl font-bold text-xs transition-all cursor-pointer shadow-xs flex items-center gap-2 ${
          editor.isSchoolValid
            ? 'bg-blue-600 hover:bg-blue-700 text-white shadow-blue-500/20 active:scale-[0.99]'
            : 'bg-slate-200 text-slate-400 cursor-not-allowed'
        }`}
      >
        <span>💾</span>
        <span>{isSaving ? "Zapisywanie..." : "Zapisz szkołę w systemie"}</span>
      </button>
    </div>
  </div>
</div>


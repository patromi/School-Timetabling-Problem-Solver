<!-- frontend/src/lib/components/editor/steps/lessons/ByClassTab.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import SubjectCombobox from "../../common/SubjectCombobox.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let selectedClass = $derived(
    editor.classes.find((c) => c.id === editor.selectedClassViewId) || editor.classes[0]
  );
  let selectedClassLessons = $derived(
    selectedClass ? editor.lessons.filter((l) => l.classId === selectedClass.id) : []
  );
  let selectedClassHours = $derived(
    selectedClassLessons.reduce((sum, l) => sum + l.weeklyCount, 0)
  );
</script>

<div class="flex flex-col gap-6">
  <!-- Pasek wyboru rocznika i oddziału -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-4">
    <div class="flex flex-col md:flex-row md:items-center justify-between gap-3 pb-3 border-b border-slate-100">
      <div class="flex items-center gap-3">
        <span class="text-xs font-bold uppercase tracking-wider text-slate-500">Wybierz rocznik:</span>
        <div class="flex flex-wrap items-center gap-1.5">
          {#each editor.availableGrades as gr}
            <button
              type="button"
              onclick={() => {
                editor.selectedGradeCurriculum = gr;
                const firstInGrade = editor.classes.find((c) => editor.getClassGrade(c) === gr);
                if (firstInGrade) editor.selectedClassViewId = firstInGrade.id;
              }}
              class={`px-3 py-1.5 rounded-xl text-xs font-bold transition-all cursor-pointer border ${
                editor.selectedGradeCurriculum === gr
                  ? "bg-blue-600 text-white border-blue-600 shadow-xs"
                  : "bg-slate-50 hover:bg-slate-100 text-slate-700 border-slate-200"
              }`}
            >
              Klasy {gr}.
            </button>
          {/each}
        </div>
      </div>

      <div class="flex items-center gap-2">
        <span class="text-xs text-slate-500">Oddział:</span>
        <select
          bind:value={editor.selectedClassViewId}
          class="px-3 py-1.5 bg-slate-50 border border-slate-200 rounded-xl text-xs font-bold text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
        >
          {#each editor.classes.filter((c) => editor.getClassGrade(c) === editor.selectedGradeCurriculum) as cls}
            <option value={cls.id}>{cls.name}</option>
          {/each}
        </select>

        {#if selectedClass}
          <button
            type="button"
            onclick={() => (editor.duplicateModalTarget = selectedClass)}
            class="px-3 py-1.5 bg-blue-50 hover:bg-blue-100 text-blue-700 border border-blue-200 text-xs font-bold rounded-xl transition-all cursor-pointer flex items-center gap-1.5"
            title="Skopiuj program nauczania tej klasy do innych klas"
          >
            <span>📋 Skopiuj siatkę do innych</span>
          </button>
        {/if}
      </div>
    </div>

    <!-- Formularz masowego przypisania przedmiotu do wszystkich klas tego rocznika -->
    <div class="p-3 bg-blue-50/50 border border-blue-100 rounded-xl flex flex-col md:flex-row items-start md:items-center justify-between gap-3 text-xs">
      <div class="flex items-center gap-2 text-blue-950 font-semibold">
        <span>⚡ Szybkie dodanie przedmiotu dla CAŁEGO rocznika {editor.selectedGradeCurriculum}:</span>
      </div>

      <div class="flex flex-wrap items-center gap-2 w-full md:w-auto">
        <div class="w-48">
          <SubjectCombobox
            bind:value={editor.batchGradeSubject}
            subjects={editor.allKnownSubjects}
            placeholder="Nazwa przedmiotu (np. Fizyka)"
            onAddNew={(newSub) => editor.registerSubject(newSub)}
          />
        </div>
        <div class="flex items-center gap-1">
          <input
            type="number"
            min="1"
            max="10"
            bind:value={editor.batchGradeHours}
            class="w-14 px-2 py-1.5 bg-white border border-blue-200 rounded-lg text-xs font-bold text-center text-slate-900"
          />
          <span class="text-blue-900 font-semibold">h</span>
        </div>
        <label class="flex items-center gap-1 text-[11px] text-blue-900 cursor-pointer">
          <input type="checkbox" bind:checked={editor.batchGradeIsBlock} class="accent-blue-600 rounded" />
          <span>Blok 2h</span>
        </label>
        <button
          type="button"
          onclick={() => editor.addSubjectToGrade(editor.selectedGradeCurriculum, editor.batchGradeSubject, editor.batchGradeHours, editor.batchGradeIsBlock)}
          class="px-3 py-1.5 bg-blue-600 hover:bg-blue-700 text-white font-bold rounded-lg transition-all cursor-pointer shadow-xs"
        >
          + Przypisz rocznikowi
        </button>
      </div>
    </div>
  </div>

  <!-- Tabela lekcji wybranej klasy -->
  {#if selectedClass}
    <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden">
      <div class="p-4 border-b border-slate-100 flex items-center justify-between text-xs">
        <div class="flex items-center gap-2">
          <span class="font-bold text-slate-900 text-sm">Oddział: {selectedClass.name}</span>
          <span class="px-2 py-0.5 bg-blue-100 text-blue-800 rounded-full font-bold">
            {selectedClassHours} godz./tydzień
          </span>
        </div>
        <span class="text-slate-400">Pozycje w planie: {selectedClassLessons.length}</span>
      </div>

      {#if selectedClassLessons.length === 0}
        <div class="p-8 text-center text-slate-400 text-xs">
          Ta klasa nie ma jeszcze żadnych przypisanych lekcji.
        </div>
      {:else}
        <div class="divide-y divide-slate-100 max-h-[30rem] overflow-y-auto">
          {#each selectedClassLessons as l}
            {@const t = editor.teachers.find((tch) => tch.id === l.teacherId)}
            {@const r = editor.rooms.find((rm) => rm.id === l.roomId)}
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

              <div class="flex items-center gap-3">
                <!-- Segmented quick-switch for preferences -->
                <div class="inline-flex items-center rounded-lg border border-slate-200 bg-slate-50/80 p-0.5 text-xs shadow-2xs">
                  <button
                    type="button"
                    onclick={() => editor.setQuickLessonPref(l.id, "morning")}
                    class="px-2 py-0.5 rounded-md font-medium transition-all {l.timePreference === 'morning' ? 'bg-amber-100 text-amber-900 font-bold shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                    title="Preferuj rano (lekcje 1–4)"
                  >
                    🌅 Rano
                  </button>
                  <button
                    type="button"
                    onclick={() => editor.setQuickLessonPref(l.id, "afternoon")}
                    class="px-2 py-0.5 rounded-md font-medium transition-all {l.timePreference === 'afternoon' ? 'bg-indigo-100 text-indigo-900 font-bold shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                    title="Preferuj po południu (lekcje 5+)"
                  >
                    🌇 Popołudnie
                  </button>
                  <button
                    type="button"
                    onclick={() => editor.setQuickLessonPref(l.id, "any")}
                    class="px-2 py-0.5 rounded-md font-medium transition-all {(!l.timePreference || l.timePreference === 'any') ? 'bg-white text-slate-800 font-bold shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                    title="Brak preferencji (dowolna pora)"
                  >
                    🕒 Dowolna
                  </button>
                  <button
                    type="button"
                    onclick={() => editor.openLessonPrefModal(l)}
                    class="px-2 py-0.5 rounded-md font-medium transition-all {l.timePreference === 'custom' ? 'bg-emerald-100 text-emerald-900 font-bold shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                    title="Własna siatka godzin oraz wagi reguły"
                  >
                    {#if l.timePreference === 'custom'}
                      ⭐ Własna ({l.preferredTimes?.length || 0})
                    {:else}
                      ⚙️ Własna...
                    {/if}
                  </button>
                </div>

                <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-bold">
                  {l.weeklyCount}h/tydz.
                </span>

                <button
                  type="button"
                  onclick={() => (editor.editEntityTarget = { type: "lesson", data: l })}
                  class="text-slate-500 hover:text-blue-600 p-1 rounded cursor-pointer"
                  title="Edytuj lekcję"
                >
                  ✏️
                </button>

                <button
                  type="button"
                  onclick={() => editor.removeLesson(l.id)}
                  class="text-slate-400 hover:text-rose-600 px-1.5 py-1 rounded cursor-pointer"
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

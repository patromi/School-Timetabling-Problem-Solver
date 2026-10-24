<!-- frontend/src/lib/components/editor/steps/lessons/ByTeacherTab.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let selectedTeacher = $derived(
    editor.teachers.find((t) => t.id === editor.selectedTeacherViewId) || editor.teachers[0]
  );
  let selectedTeacherLessons = $derived(
    selectedTeacher ? editor.lessons.filter((l) => l.teacherId === selectedTeacher.id) : []
  );
  let selectedTeacherHours = $derived(
    selectedTeacherLessons.reduce((sum, l) => sum + l.weeklyCount, 0)
  );
</script>

<div class="flex flex-col gap-6">
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
    <div class="flex flex-col sm:flex-row sm:items-center justify-between gap-3 pb-3 border-b border-slate-100">
      <div class="flex items-center gap-3">
        <span class="text-xs font-bold uppercase tracking-wider text-slate-500">Wybierz nauczyciela:</span>
        <select
          bind:value={editor.selectedTeacherViewId}
          class="px-3 py-1.5 bg-slate-50 border border-slate-200 rounded-xl text-xs font-bold text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
        >
          {#each editor.teachers as t}
            <option value={t.id}>{t.name} ({t.subject})</option>
          {/each}
        </select>
      </div>

      {#if selectedTeacher}
        <div class="flex items-center gap-3 text-xs">
          <span class="text-slate-500">Pensum etatowe: <strong>{selectedTeacher.workload}h</strong></span>
          <span>•</span>
          <span class="text-slate-500">Przydział: <strong class="text-blue-700">{selectedTeacherHours}h</strong></span>
        </div>
      {/if}
    </div>
  </div>

  <!-- Tabela lekcji nauczyciela -->
  {#if selectedTeacher}
    <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden">
      <div class="p-4 border-b border-slate-100 flex items-center justify-between text-xs">
        <span class="font-bold text-slate-900 text-sm">Wykaz zajęć: {selectedTeacher.name}</span>
        <span class="text-slate-400">Pozycji w siatce: {selectedTeacherLessons.length}</span>
      </div>

      {#if selectedTeacherLessons.length === 0}
        <div class="p-8 text-center text-slate-400 text-xs">
          Ten nauczyciel nie ma jeszcze przypisanych żadnych zajęć.
        </div>
      {:else}
        <div class="divide-y divide-slate-100 max-h-[30rem] overflow-y-auto">
          {#each selectedTeacherLessons as l}
            {@const cls = editor.classes.find((c) => c.id === l.classId)}
            {@const r = editor.rooms.find((rm) => rm.id === l.roomId)}
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

<!-- frontend/src/lib/components/editor/steps/lessons/AllLessonsTab.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import AddLessonBar from "./AddLessonBar.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div>
      <h3 class="text-sm font-bold text-slate-900">Siatka godzin i przydział lekcji</h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Łącznie zaplanowano: <strong>{editor.totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
      </p>
    </div>

    <div class="flex flex-wrap items-center gap-2">
      <button
        type="button"
        onclick={() => (editor.isSubjectManagerOpen = true)}
        class="px-3 py-2 bg-purple-50 hover:bg-purple-100 text-purple-900 border border-purple-200 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5 shadow-xs"
        title="Otwórz Menedżer Przedmiotów, aby zmienić nazwę lub scalić przedmioty"
      >
        <span>📚 Menedżer przedmiotów</span>
      </button>

      <button
        type="button"
        onclick={() => (editor.isLinkedModalOpen = true)}
        class="px-3.5 py-2 bg-indigo-50 hover:bg-indigo-100 text-indigo-900 border border-indigo-200 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5 shadow-xs"
        title="Zarządzaj lekcjami równoległymi i jednoczesnymi (np. języki, WF)"
      >
        <span>🔗 Lekcje równoległe ({editor.linkedGroups.length})</span>
      </button>

      <button
        type="button"
        onclick={() => editor.generateDefaultCurriculum()}
        class="px-3.5 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-xl cursor-pointer transition-all flex items-center gap-1.5"
        title="Wygeneruj standardową ramówkę licealną dla wszystkich oddziałów"
      >
        <span>⚡ Wzorzec LO</span>
      </button>
    </div>
  </div>

  <!-- Formularz dodawania pojedynczej lekcji -->
  <AddLessonBar {editor} />

  <!-- Tabela lekcji z wyszukiwarką, filtrami i operacjami masowymi -->
  <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden flex flex-col">
    <!-- Pasek filtrów i wyszukiwarki -->
    <div class="p-4 bg-slate-50/70 border-b border-slate-200/80 flex flex-col gap-3">
      <div class="flex flex-col md:flex-row items-stretch md:items-center justify-between gap-3">
        <!-- Wyszukiwarka tekstowa -->
        <div class="relative flex-1">
          <span class="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none text-slate-400 text-sm">
            🔍
          </span>
          <input
            type="text"
            bind:value={editor.lessonSearchQuery}
            placeholder="Szukaj po przedmiocie, klasie, nauczycielu lub sali..."
            class="w-full pl-9 pr-8 py-2 text-xs bg-white border border-slate-200 rounded-xl focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 text-slate-800 placeholder-slate-400"
          />
          {#if editor.lessonSearchQuery}
            <button
              type="button"
              onclick={() => (editor.lessonSearchQuery = "")}
              class="absolute inset-y-0 right-0 pr-3 flex items-center text-slate-400 hover:text-slate-600 text-xs"
            >
              ✕
            </button>
          {/if}
        </div>

        <!-- Licznik i czyszczenie filtrów -->
        <div class="flex items-center justify-between md:justify-end gap-3 shrink-0">
          <span class="text-xs text-slate-500">
            Znaleziono: <strong class="text-slate-800">{editor.filteredLessons.length}</strong> z {editor.lessons.length} ({editor.filteredLessonsHours}h)
          </span>
          {#if editor.lessonSearchQuery || editor.lessonFilterClass !== "all" || editor.lessonFilterTeacher !== "all" || editor.lessonFilterGrade !== null || editor.lessonFilterPreference !== "all"}
            <button
              type="button"
              onclick={() => editor.clearLessonFilters()}
              class="px-2.5 py-1 text-xs font-semibold text-rose-600 bg-rose-50 hover:bg-rose-100 rounded-lg transition-colors cursor-pointer"
            >
              Wyczyść filtry ✕
            </button>
          {/if}
        </div>
      </div>

      <!-- Kontrolki wielokryterialne -->
      <div class="grid grid-cols-2 sm:grid-cols-4 gap-2 pt-1 text-xs">
        <div>
          <label for="filter-class" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Klasa</label>
          <select
            id="filter-class"
            bind:value={editor.lessonFilterClass}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs text-slate-700 focus:outline-none focus:border-blue-500"
          >
            <option value="all">Wszystkie klasy ({editor.classes.length})</option>
            {#each editor.classes as c}
              <option value={c.id}>{c.name}</option>
            {/each}
          </select>
        </div>

        <div>
          <label for="filter-teacher" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Nauczyciel</label>
          <select
            id="filter-teacher"
            bind:value={editor.lessonFilterTeacher}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs text-slate-700 focus:outline-none focus:border-blue-500"
          >
            <option value="all">Wszyscy nauczyciele ({editor.teachers.length})</option>
            {#each editor.teachers as t}
              <option value={t.id}>{t.name}</option>
            {/each}
          </select>
        </div>

        <div>
          <label for="filter-grade" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Rocznik</label>
          <select
            id="filter-grade"
            bind:value={editor.lessonFilterGrade}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs text-slate-700 focus:outline-none focus:border-blue-500"
          >
            <option value={null}>Wszystkie roczniki</option>
            {#each editor.availableGrades as gr}
              <option value={gr}>Klasy {gr}.</option>
            {/each}
          </select>
        </div>

        <div>
          <label for="filter-pref" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Preferencja pory</label>
          <select
            id="filter-pref"
            bind:value={editor.lessonFilterPreference}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs text-slate-700 focus:outline-none focus:border-blue-500"
          >
            <option value="all">Wszystkie stany</option>
            <option value="has_pref">Tylko z preferencją (🌅/🌇/⭐)</option>
            <option value="morning">🌅 Tylko rano (1–4)</option>
            <option value="afternoon">🌇 Tylko popołudnie (5+)</option>
            <option value="custom">⭐ Tylko własna siatka</option>
            <option value="none">🕒 Bez preferencji (dowolna)</option>
          </select>
        </div>
      </div>

      <!-- Pasek operacji masowych -->
      {#if editor.selectedLessonIds.size > 0}
        <div class="mt-1 p-2.5 bg-blue-50/80 border border-blue-200/80 rounded-xl flex flex-wrap items-center justify-between gap-2.5 text-xs">
          <div class="flex items-center gap-2">
            <span class="w-6 h-6 rounded-full bg-blue-600 text-white font-bold text-xs flex items-center justify-center">
              {editor.selectedLessonIds.size}
            </span>
            <span class="font-bold text-blue-950">
              Zaznaczono {editor.selectedLessonIds.size} {editor.selectedLessonIds.size === 1 ? 'lekcję' : 'lekcji'}
            </span>
          </div>

          <div class="flex flex-wrap items-center gap-2">
            <span class="text-[11px] text-blue-800 font-medium">Ustaw dla zaznaczonych:</span>
            <button
              type="button"
              onclick={() => editor.applyBatchPreference("morning")}
              class="px-2.5 py-1 bg-amber-100 hover:bg-amber-200 text-amber-900 font-semibold rounded-lg border border-amber-300 transition-colors cursor-pointer flex items-center gap-1 shadow-xs"
            >
              <span>🌅</span> Rano
            </button>
            <button
              type="button"
              onclick={() => editor.applyBatchPreference("afternoon")}
              class="px-2.5 py-1 bg-indigo-100 hover:bg-indigo-200 text-indigo-900 font-semibold rounded-lg border border-indigo-300 transition-colors cursor-pointer flex items-center gap-1 shadow-xs"
            >
              <span>🌇</span> Popołudnie
            </button>
            <button
              type="button"
              onclick={() => editor.applyBatchPreference("any")}
              class="px-2.5 py-1 bg-white hover:bg-slate-100 text-slate-700 font-semibold rounded-lg border border-slate-200 transition-colors cursor-pointer flex items-center gap-1 shadow-xs"
            >
              <span>🕒</span> Dowolna (wyczyść)
            </button>
            <button
              type="button"
              onclick={() => editor.clearSelectedLessons()}
              class="text-xs text-blue-700 hover:text-blue-900 underline ml-2 cursor-pointer"
            >
              Odznacz wszystkie
            </button>
          </div>
        </div>
      {/if}
    </div>

    <!-- Nagłówek tabeli -->
    <div class="p-3 px-4 border-b border-slate-200 bg-white flex items-center justify-between text-xs text-slate-500">
      <div class="flex items-center gap-3">
        <input
          type="checkbox"
          checked={editor.filteredLessons.length > 0 && editor.selectedLessonIds.size >= editor.filteredLessons.length}
          onchange={(e) => {
            if (e.currentTarget.checked) editor.selectAllFilteredLessons();
            else editor.clearSelectedLessons();
          }}
          class="rounded text-blue-600 focus:ring-blue-500 cursor-pointer"
          title="Zaznacz wszystkie przefiltrowane"
        />
        <span class="font-semibold text-slate-700">Przedmiot i przypisanie</span>
      </div>
      <div class="flex items-center gap-4">
        <span class="font-semibold text-slate-700">Szybkie preferencje pory dnia</span>
        <span class="font-semibold text-slate-700">Wymiar</span>
      </div>
    </div>

    <!-- Lista lekcji -->
    <div class="divide-y divide-slate-100 max-h-[32rem] overflow-y-auto">
      {#if editor.filteredLessons.length === 0}
        <div class="p-8 text-center text-slate-400 flex flex-col items-center justify-center gap-2">
          <span class="text-3xl">🔍</span>
          <span class="font-semibold text-slate-600">Brak lekcji spełniających kryteria</span>
          <p class="text-xs text-slate-400">Spróbuj zmienić parametry wyszukiwania lub filtry rocznika/klasy/nauczyciela.</p>
          <button
            type="button"
            onclick={() => editor.clearLessonFilters()}
            class="mt-2 px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg text-xs font-semibold transition-colors"
          >
            Wyczyść filtry
          </button>
        </div>
      {:else}
        {#each editor.filteredLessons as l}
          {@const cls = editor.classes.find((c) => c.id === l.classId)}
          {@const t = editor.teachers.find((tch) => tch.id === l.teacherId)}
          {@const r = editor.rooms.find((rm) => rm.id === l.roomId)}
          {@const linkedGroup = editor.linkedGroups.find((g) => g.lessonIds.includes(l.id))}
          <div class="p-3 hover:bg-slate-50/80 flex items-center justify-between text-xs transition-colors {editor.selectedLessonIds.has(l.id) ? 'bg-blue-50/40' : ''}">
            <div class="flex items-center gap-3">
              <input
                type="checkbox"
                checked={editor.selectedLessonIds.has(l.id)}
                onchange={() => editor.toggleLessonSelection(l.id)}
                class="rounded text-blue-600 focus:ring-blue-500 cursor-pointer"
              />
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

              <span class="px-2.5 py-1 bg-slate-100 text-slate-800 rounded-full font-semibold">
                {l.weeklyCount} godz./tydz.
              </span>

              <!-- Przycisk Edycji Lekcji -->
              <button
                type="button"
                onclick={() => (editor.editEntityTarget = { type: "lesson", data: l })}
                class="text-slate-500 hover:text-blue-600 p-1 rounded cursor-pointer"
                title="Edytuj lekcję (przedmiot, nauczyciel, sala, wymiar)"
              >
                ✏️
              </button>

              <button
                type="button"
                onclick={() => editor.removeLesson(l.id)}
                class="text-slate-400 hover:text-rose-600 px-1.5 py-1 rounded cursor-pointer"
                title="Usuń lekcję"
              >
                ✕
              </button>
            </div>
          </div>
        {/each}
      {/if}
    </div>
  </div>
</div>

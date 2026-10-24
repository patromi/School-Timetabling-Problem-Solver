<!-- frontend/src/lib/components/editor/steps/Step4Lessons.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

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

<div class="flex flex-col gap-5">
  <!-- Pasek zakładek Krok 4 -->
  <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
    <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
      <button
        type="button"
        onclick={() => (editor.step4ActiveTab = "lessons")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "lessons"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>📋 Wszystkie lekcje</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step4ActiveTab === 'lessons' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.lessons.length}
        </span>
      </button>
      <button
        type="button"
        onclick={() => (editor.step4ActiveTab = "byClass")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "byClass"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>🏫 Widok klas i roczników</span>
      </button>
      <button
        type="button"
        onclick={() => {
          editor.step4ActiveTab = "byTeacher";
          if (!editor.selectedTeacherViewId && editor.teachers[0]) {
            editor.selectedTeacherViewId = editor.teachers[0].id;
          }
        }}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "byTeacher"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👨‍🏫 Widok nauczycieli</span>
      </button>
    </div>

    <div class="text-xs text-slate-500 px-3 py-1">
      {#if editor.step4ActiveTab === "lessons"}
        Łącznie zaplanowano: <strong>{editor.totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
      {:else if editor.step4ActiveTab === "byClass"}
        Przeglądaj plany poszczególnych klas, edytuj siatki i przypisuj przedmioty do roczników.
      {:else}
        Podgląd klas każdego nauczyciela oraz weryfikacja zasady spójności przydziału.
      {/if}
    </div>
  </div>

  {#if editor.step4ActiveTab === "lessons"}
    <!-- ZAKŁADKA 1: WSZYSTKIE LEKCJE -->
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
      <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
        <span class="text-xs font-bold text-slate-800">Dodaj nową pozycję do planu:</span>
        <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-6 gap-2.5 text-xs">
          <!-- Przedmiot -->
          <div>
            <label for="lesson-subject-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Przedmiot</label>
            <input
              id="lesson-subject-input"
              type="text"
              bind:value={editor.lessonSubject}
              class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
            />
          </div>

          <!-- Klasa -->
          <div>
            <label for="lesson-class-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Oddział / Klasa</label>
            <select
              id="lesson-class-input"
              bind:value={editor.lessonClassId}
              class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
            >
              {#each editor.classes as c}
                <option value={c.id}>{c.name}</option>
              {/each}
            </select>
          </div>

          <!-- Nauczyciel -->
          <div>
            <label for="lesson-teacher-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Nauczyciel</label>
            <select
              id="lesson-teacher-input"
              bind:value={editor.lessonTeacherId}
              class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
            >
              {#each editor.teachers as t}
                <option value={t.id}>{t.name} ({t.subject})</option>
              {/each}
            </select>
          </div>

          <!-- Sala -->
          <div>
            <label for="lesson-room-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Sala</label>
            <select
              id="lesson-room-input"
              bind:value={editor.lessonRoomId}
              class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
            >
              <option value="">Dowolna</option>
              {#each editor.rooms as r}
                <option value={r.id}>{r.name}</option>
              {/each}
            </select>
          </div>

          <!-- Wymiar godzin -->
          <div>
            <label for="lesson-count-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Godz./tydzień</label>
            <div class="flex items-center gap-1.5">
              <input
                id="lesson-count-input"
                type="number"
                min="1"
                max="10"
                bind:value={editor.lessonCount}
                class="w-12 py-1.5 px-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-bold text-center text-slate-800"
              />
              <label class="flex items-center gap-1 text-[11px] text-slate-500 cursor-pointer">
                <input type="checkbox" bind:checked={editor.lessonIsBlock} class="accent-blue-600 rounded" />
                <span>Blok 2h</span>
              </label>
            </div>
          </div>

          <!-- Przycisk dodaj -->
          <div class="flex items-end">
            <button
              type="button"
              onclick={() => editor.addCustomLesson()}
              class="w-full py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-semibold rounded-lg transition-all cursor-pointer text-center"
            >
              + Dodaj lekcję
            </button>
          </div>
        </div>
      </div>

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
  {:else if editor.step4ActiveTab === "byClass"}
    <!-- ZAKŁADKA 2: WIDOK KLAS I ROCZNIKÓW -->
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
            <input
              type="text"
              bind:value={editor.batchGradeSubject}
              placeholder="Nazwa przedmiotu (np. Fizyka)"
              class="px-2.5 py-1.5 bg-white border border-blue-200 rounded-lg text-xs text-slate-800 font-medium focus:outline-none focus:border-blue-500"
            />
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
  {:else}
    <!-- ZAKŁADKA 3: WIDOK NAUCZYCIELI I SPÓJNOŚCI -->
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
  {/if}
</div>

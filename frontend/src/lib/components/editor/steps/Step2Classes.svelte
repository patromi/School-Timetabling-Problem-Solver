<!-- frontend/src/lib/components/editor/steps/Step2Classes.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let filteredClasses = $derived(
    editor.selectedGradeFilter === null
      ? editor.classes
      : editor.classes.filter((c) => editor.getClassGrade(c) === editor.selectedGradeFilter)
  );
</script>

<div class="flex flex-col gap-5">
  <!-- Pasek zakładek Krok 2 -->
  <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
    <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
      <button
        type="button"
        onclick={() => (editor.step2ActiveTab = "classes")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step2ActiveTab === "classes"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👥 Klasy i roczniki</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step2ActiveTab === 'classes' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.classes.length}
        </span>
      </button>
      <button
        type="button"
        onclick={() => (editor.step2ActiveTab = "teachers")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step2ActiveTab === "teachers"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👨‍🏫 Kadra i pensum</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step2ActiveTab === 'teachers' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.teachers.length}
        </span>
      </button>
    </div>

    <div class="text-xs text-slate-500 px-3 py-1">
      {#if editor.step2ActiveTab === "classes"}
        Zarządzaj poziomami klas, edytuj nazwy, powielaj siatki godzin i ustalaj ograniczenia dzienne.
      {:else}
        Ustalaj pensum dydaktyczne (etat), edytuj dane pedagogów i monitoruj zbilansowanie godzin kadry.
      {/if}
    </div>
  </div>

  {#if editor.step2ActiveTab === "classes"}
    <!-- ZAKŁADKA KLAS I ROCZNIKÓW -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
      <!-- Header & Grade filter -->
      <div class="flex flex-col md:flex-row md:items-center justify-between gap-3 pb-4 border-b border-slate-100">
        <div>
          <h3 class="text-sm font-bold text-slate-900">Oddziały klasowe według roczników</h3>
          <p class="text-xs text-slate-500 mt-0.5">
            Możesz edytować nazwy klas, duplikować oddziały lub kopiować cały program nauczania.
          </p>
        </div>

        <!-- Filtry roczników -->
        <div class="flex flex-wrap items-center gap-1.5">
          <span class="text-xs font-semibold text-slate-500 mr-1">Rocznik:</span>
          <button
            type="button"
            onclick={() => (editor.selectedGradeFilter = null)}
            class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
              editor.selectedGradeFilter === null
                ? "bg-blue-600 text-white border-blue-600"
                : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
            }`}
          >
            Wszystkie ({editor.classes.length})
          </button>
          {#each editor.availableGrades as gr}
            {@const count = editor.classes.filter((c) => editor.getClassGrade(c) === gr).length}
            <button
              type="button"
              onclick={() => (editor.selectedGradeFilter = gr)}
              class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
                editor.selectedGradeFilter === gr
                  ? "bg-blue-600 text-white border-blue-600"
                  : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
              }`}
            >
              Rocznik {gr} ({count})
            </button>
          {/each}
        </div>
      </div>

      <!-- Formularz dodawania klasy -->
      <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
        <input
          type="text"
          bind:value={editor.newClassName}
          placeholder="Nazwa klasy (np. 4C Mat-Inf lub 1A)"
          onkeydown={(e) => e.key === "Enter" && editor.addClass()}
          class="flex-1 min-w-[200px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
        />
        <div class="flex items-center gap-1.5">
          <span class="text-xs text-slate-500">Rocznik:</span>
          <select
            bind:value={editor.newClassGrade}
            class="px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
          >
            <option value={1}>Poziom 1</option>
            <option value={2}>Poziom 2</option>
            <option value={3}>Poziom 3</option>
            <option value={4}>Poziom 4</option>
            <option value={5}>Poziom 5</option>
          </select>
        </div>
        <button
          type="button"
          onclick={() => editor.addClass()}
          class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
        >
          + Dodaj klasę
        </button>
      </div>

      <!-- Siatka klas -->
      <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
        {#each filteredClasses as cls}
          {@const weeklyHours = editor.getClassWeeklyHours(cls.id)}
          {@const clsLessons = editor.getClassLessons(cls.id)}
          {@const grade = editor.getClassGrade(cls)}
          <div class="p-4 bg-slate-50/90 rounded-2xl border border-slate-200 flex flex-col justify-between gap-3 shadow-2xs hover:border-slate-300 transition-colors">
            <div>
              <div class="flex items-start justify-between gap-2">
                <div class="flex items-center gap-2">
                  <span class="w-8 h-8 rounded-xl bg-blue-100 text-blue-800 font-black text-xs flex items-center justify-center">
                    {grade}
                  </span>
                  <div>
                    <div class="flex items-center gap-2">
                      <span class="font-bold text-slate-900 text-xs">👥 {cls.name}</span>
                      <span class="text-[10px] px-1.5 py-0.2 bg-slate-200/80 text-slate-700 font-semibold rounded">
                        Rocznik {grade}
                      </span>
                    </div>
                    <span class="text-[11px] text-slate-500 block mt-0.5">
                      Łącznie: <strong>{weeklyHours} godz./tydz.</strong> ({clsLessons.length} przedmiotów/wpisów)
                    </span>
                  </div>
                </div>

                <div class="flex items-center gap-1">
                  <!-- Przycisk Edycji / Zmiany nazwy -->
                  <button
                    type="button"
                    onclick={() => (editor.editEntityTarget = { type: "class", data: cls })}
                    class="text-[11px] px-2 py-1 bg-white hover:bg-slate-100 text-slate-700 rounded-lg border border-slate-200 flex items-center gap-1 transition-all cursor-pointer font-medium"
                    title="Zmień nazwę klasy, rocznik lub ID"
                  >
                    <span>✏️</span>
                    <span>Edytuj</span>
                  </button>

                  <button
                    type="button"
                    onclick={() => editor.openAvailabilityModal(cls.id, cls.name, "class")}
                    class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                      (cls.unavailableTimes?.length || 0) > 0 || cls.minDailyLessons || cls.maxDailyLessons
                        ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                        : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                    }`}
                    title="Ustal dostępność klasy i ramy lekcyjne"
                  >
                    <span>🗓️</span>
                    <span>{cls.unavailableTimes?.length ? `${cls.unavailableTimes.length} wykl.` : "Dostępność"}</span>
                  </button>

                  <button
                    type="button"
                    onclick={() => editor.removeClass(cls.id)}
                    class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                    title="Usuń klasę"
                  >
                    ✕
                  </button>
                </div>
              </div>

              <!-- Mini podgląd siatki przedmiotów klasy -->
              {#if clsLessons.length > 0}
                <div class="mt-3 flex flex-wrap gap-1">
                  {#each clsLessons.slice(0, 5) as l}
                    <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-700 rounded-md">
                      {l.subject}: {l.weeklyCount}h
                    </span>
                  {/each}
                  {#if clsLessons.length > 5}
                    <span class="text-[10px] px-1.5 py-0.5 text-slate-400 font-medium">
                      +{clsLessons.length - 5} więcej
                    </span>
                  {/if}
                </div>
              {:else}
                <p class="mt-2 text-[11px] text-slate-400 italic">
                  Brak przypisanych lekcji. Użyj przycisku poniżej, aby skopiować siatkę z innej klasy.
                </p>
              {/if}
            </div>

            <!-- Akcje dla klasy: duplikowanie -->
            <div class="pt-2 border-t border-slate-200/60 flex items-center justify-between gap-2 text-xs">
              <button
                type="button"
                onclick={() => (editor.duplicateModalTarget = cls)}
                class="px-2.5 py-1 bg-white hover:bg-blue-50 text-blue-700 border border-slate-200 hover:border-blue-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1.5 text-[11px]"
                title="Skopiuj wszystkie lekcje z tej klasy do innych klas"
              >
                <span>📋 Duplikuj siatkę do...</span>
              </button>

              <button
                type="button"
                onclick={() => editor.duplicateClass(cls.id)}
                class="px-2.5 py-1 bg-white hover:bg-slate-100 text-slate-700 border border-slate-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1 text-[11px]"
                title="Utwórz nowy oddział na bazie tej klasy z identyczną siatką zajęć"
              >
                <span>➕ Nowy oddział</span>
              </button>
            </div>
          </div>
        {/each}
      </div>
    </div>
  {:else}
    <!-- ZAKŁADKA KADRY I PENSUM -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
      <!-- Globalny pasek bilansu pensum -->
      <div class="p-4 bg-gradient-to-r from-blue-50/80 to-indigo-50/80 border border-blue-200/80 rounded-2xl flex flex-wrap items-center justify-between gap-4">
        <div class="flex items-center gap-4">
          <div class="w-10 h-10 rounded-xl bg-blue-600 text-white flex items-center justify-center text-lg font-black shadow-xs">
            ⚖️
          </div>
          <div>
            <h4 class="text-xs font-bold uppercase tracking-wider text-blue-950">
              Bilans pensum i etaty kadry pedagogicznej
            </h4>
            <p class="text-xs text-blue-800 mt-0.5">
              Możesz bezpośrednio edytować pensum każdego nauczyciela i śledzić jego obciążenie.
            </p>
          </div>
        </div>

        <div class="flex items-center gap-6 text-xs">
          <div>
            <span class="text-slate-500 block text-[11px]">Łączne pensum:</span>
            <span class="font-black text-slate-900 text-sm">{editor.totalTeacherWorkload} godz.</span>
          </div>
          <div>
            <span class="text-slate-500 block text-[11px]">Przydzielone lekcje:</span>
            <span class="font-black text-blue-700 text-sm">{editor.totalHoursAssigned} godz.</span>
          </div>
          <div>
            <span class="text-slate-500 block text-[11px]">Bilans:</span>
            {#if editor.totalHoursAssigned === editor.totalTeacherWorkload}
              <span class="font-black text-emerald-600 text-sm">Zrównoważony (0h)</span>
            {:else if editor.totalHoursAssigned > editor.totalTeacherWorkload}
              <span class="font-black text-rose-600 text-sm">+{editor.totalHoursAssigned - editor.totalTeacherWorkload}h (nadgodziny)</span>
            {:else}
              <span class="font-black text-amber-600 text-sm">-{editor.totalTeacherWorkload - editor.totalHoursAssigned}h (wakat)</span>
            {/if}
          </div>
        </div>
      </div>

      <!-- Formularz dodawania nauczyciela z pensum -->
      <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3.5 rounded-xl border border-slate-200/70">
        <input
          type="text"
          bind:value={editor.newTeacherName}
          placeholder="Imię i nazwisko (np. Anna Nowak)"
          class="flex-1 min-w-[180px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
        />
        <input
          type="text"
          bind:value={editor.newTeacherSubject}
          placeholder="Nauczany przedmiot (np. Matematyka)"
          class="flex-1 min-w-[160px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
        />
        <div class="flex items-center gap-1.5">
          <span class="text-xs font-semibold text-slate-600">Pensum:</span>
          <input
            type="number"
            min="1"
            max="60"
            bind:value={editor.newTeacherWorkload}
            class="w-16 px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500 text-center font-bold"
          />
          <span class="text-xs text-slate-500">h</span>
        </div>
        <button
          type="button"
          onclick={() => editor.addTeacher()}
          class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
        >
          + Dodaj nauczyciela
        </button>
      </div>

      <!-- Siatka nauczycieli z edycją pensum i podglądem klas -->
      <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
        {#each editor.teachers as t}
          {@const assigned = editor.getTeacherAssignedHours(t.id)}
          {@const tClasses = editor.getTeacherClasses(t.id)}
          {@const workloadDiff = assigned - t.workload}
          <div class="p-4 bg-slate-50/90 rounded-2xl border border-slate-200 flex flex-col justify-between gap-3 shadow-2xs hover:border-slate-300 transition-colors">
            <div>
              <div class="flex items-start justify-between gap-2">
                <div>
                  <div class="flex items-center gap-2">
                    <span class="font-bold text-slate-900 text-xs">👨‍🏫 {t.name}</span>
                    <span class="text-[10px] px-2 py-0.5 bg-blue-50 text-blue-700 border border-blue-200 font-semibold rounded-full">
                      {t.subject}
                    </span>
                  </div>

                  <!-- Edycja pensum bezpośrednio w wierszu -->
                  <div class="flex items-center gap-2 mt-2">
                    <span class="text-[11px] text-slate-500">Pensum:</span>
                    <input
                      type="number"
                      min="1"
                      max="60"
                      value={t.workload}
                      oninput={(e) => editor.updateTeacherWorkload(t.id, parseInt(e.currentTarget.value, 10))}
                      class="w-14 px-2 py-0.5 text-xs bg-white border border-slate-300 rounded text-slate-900 font-bold text-center focus:outline-none focus:ring-1 focus:ring-blue-500"
                      title="Edytuj wymiar pensum nauczyciela"
                    />
                    <span class="text-xs text-slate-500">h / tydzień</span>

                    <!-- Wskaźnik zbilansowania -->
                    <span class={`text-[10px] px-2 py-0.5 rounded-full font-bold ${
                      workloadDiff === 0
                        ? "bg-emerald-50 text-emerald-700 border border-emerald-200"
                        : workloadDiff > 0
                        ? "bg-rose-50 text-rose-700 border-rose-200"
                        : "bg-amber-50 text-amber-700 border-amber-200"
                    }`}>
                      {assigned} / {t.workload}h ({workloadDiff === 0 ? "100%" : workloadDiff > 0 ? `+${workloadDiff}h` : `${workloadDiff}h`})
                    </span>
                  </div>
                </div>

                <div class="flex items-center gap-1">
                  <!-- Przycisk Edycji / Zmiany nazwiska/przedmiotu -->
                  <button
                    type="button"
                    onclick={() => (editor.editEntityTarget = { type: "teacher", data: t })}
                    class="text-[11px] px-2 py-1 bg-white hover:bg-slate-100 text-slate-700 rounded-lg border border-slate-200 flex items-center gap-1 transition-all cursor-pointer font-medium"
                    title="Zmień nazwisko, nauczany przedmiot lub pensum"
                  >
                    <span>✏️</span>
                    <span>Edytuj</span>
                  </button>

                  <button
                    type="button"
                    onclick={() => editor.openAvailabilityModal(t.id, t.name, "teacher")}
                    class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                      (t.unavailableTimes?.length || 0) > 0 || t.maxDaysPerWeek || t.maxDailyLessons || (t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined)
                        ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                        : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                    }`}
                    title="Ustal dostępność nauczyciela, dni pracy i okienka"
                  >
                    <span>🗓️</span>
                    <span>{t.unavailableTimes?.length ? `${t.unavailableTimes.length} wykl.` : "Dostępność"}</span>
                  </button>

                  <button
                    type="button"
                    onclick={() => editor.removeTeacher(t.id)}
                    class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                    title="Usuń nauczyciela"
                  >
                    ✕
                  </button>
                </div>
              </div>

              <!-- Podgląd przypisanych klas -->
              <div class="mt-3">
                <span class="text-[10px] font-bold uppercase tracking-wider text-slate-400 block mb-1">
                  Przydzielone oddziały ({tClasses.length}):
                </span>
                {#if tClasses.length > 0}
                  <div class="flex flex-wrap gap-1">
                    {#each tClasses as tc}
                      <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-800 rounded-md font-medium">
                        {tc.className} ({tc.subject}: {tc.hours}h)
                      </span>
                    {/each}
                  </div>
                {:else}
                  <span class="text-[11px] text-slate-400 italic">
                    Brak przypisanych klas (0 godzin).
                  </span>
                {/if}
              </div>
            </div>
          </div>
        {/each}
      </div>
    </div>
  {/if}
</div>

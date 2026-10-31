<!-- frontend/src/lib/components/editor/studio/TeacherStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import type { SimpleTeacher } from "../types";
  import AvailabilityGrid from "../availability/AvailabilityGrid.svelte";
  import SubjectCombobox from "../common/SubjectCombobox.svelte";

  interface Props {
    editor: EditorState;
    teacher: SimpleTeacher;
    onNavigateToClass: (classId: string) => void;
    onNavigateToRoom: (roomId: string) => void;
  }

  let { editor, teacher, onNavigateToClass, onNavigateToRoom }: Props = $props();

  // Local state for quick lesson assigner inside teacher profile
  let newLessonClassId = $state<string>("");
  let newLessonSubject = $state<string>("");
  let newLessonRoomId = $state<string>("");
  let newLessonHours = $state<number>(3);
  let newLessonIsBlock = $state<boolean>(false);

  // Sync default values when teacher changes
  $effect(() => {
    newLessonSubject = teacher.subject || "Matematyka";
    if (editor.classes.length > 0 && !newLessonClassId) {
      newLessonClassId = editor.classes[0].id;
    }
  });

  let assignedHours = $derived(editor.getTeacherAssignedHours(teacher.id));
  let workload = $derived(teacher.workload || 18);
  let teacherLessons = $derived(editor.lessons.filter((l) => l.teacherId === teacher.id));
  let unavailableSet = $derived(new Set(teacher.unavailableTimes || []));
  let isOverworked = $derived(assignedHours > workload);
  let isUnderworked = $derived(assignedHours < workload);

  function handleAddLesson() {
    if (!newLessonClassId || !newLessonSubject.trim() || newLessonHours < 1) return;
    editor.assignLessonToTeacher(
      teacher.id,
      newLessonClassId,
      newLessonSubject.trim(),
      newLessonRoomId,
      newLessonHours,
      newLessonIsBlock
    );
  }

  function handleToggleAllSlots() {
    if (unavailableSet.size === editor.weeklySlotsCapacity) {
      editor.clearEntityAvailability("teacher", teacher.id);
    } else {
      const all: string[] = [];
      editor.currentDaysFormatted.forEach((d) => {
        for (let p = 1; p <= editor.periodsPerDay; p++) all.push(`${d.id}_${p}`);
      });
      teacher.unavailableTimes = all;
      editor.teachers = [...editor.teachers];
    }
  }
</script>

<div class="flex flex-col gap-6">
  <!-- Teacher Card Header (Clean, Flat, No Gradients) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
      <div class="flex items-center gap-4">
        <div class="w-14 h-14 rounded-2xl bg-blue-50 border border-blue-200/80 text-blue-700 flex items-center justify-center text-2xl font-bold shrink-0">
          👨‍🏫
        </div>
        <div>
          <div class="flex items-center gap-2.5">
            <input
              type="text"
              bind:value={teacher.name}
              class="text-xl font-bold text-slate-900 bg-transparent hover:bg-slate-50 focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg px-1.5 py-0.5 border border-transparent hover:border-slate-300 transition-all"
              title="Kliknij, aby edytować imię i nazwisko"
            />
          </div>
          <p class="text-xs text-slate-500 px-1.5 mt-0.5 flex items-center gap-2">
            <span>Przedmiot wiodący: <strong>{teacher.subject}</strong></span>
            <span>•</span>
            <span>ID: <code class="text-slate-400 font-mono">{teacher.id}</code></span>
          </p>
        </div>
      </div>

      <!-- Quick parameters: Subject & Workload -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="flex items-center gap-2 bg-slate-50 border border-slate-200/90 rounded-xl px-3 py-1.5 text-xs">
          <span class="text-slate-500 font-medium">Przedmiot:</span>
          <input
            type="text"
            bind:value={teacher.subject}
            class="font-bold text-slate-800 bg-transparent focus:outline-none w-28 text-right"
          />
        </div>

        <div class="flex items-center gap-2 bg-slate-50 border border-slate-200/90 rounded-xl px-3 py-1.5 text-xs">
          <label for="workload-input-{teacher.id}" class="text-slate-500 font-medium">Pensum:</label>
          <input
            id="workload-input-{teacher.id}"
            type="number"
            min="1"
            max="40"
            bind:value={teacher.workload}
            class="w-12 font-bold text-slate-800 text-center bg-white border border-slate-200 rounded-lg py-0.5 focus:outline-none focus:border-blue-500"
          />
          <span class="text-slate-400 font-medium">godz.</span>
        </div>
      </div>
    </div>

    <!-- Live Workload Balance Bar -->
    <div class="bg-slate-50/90 rounded-xl p-4 border border-slate-200/70 flex flex-col gap-2">
      <div class="flex items-center justify-between text-xs">
        <span class="font-bold text-slate-700">Bilans przydziału godzin:</span>
        <div class="flex items-center gap-2">
          <span class="font-bold text-slate-900 text-sm">{assignedHours} / {workload} godz.</span>
          {#if isUnderworked}
            <span class="px-2 py-0.5 rounded-full text-[11px] font-semibold bg-amber-100 text-amber-800">
              Brakuje {workload - assignedHours}h do pełnego pensum
            </span>
          {:else if isOverworked}
            <span class="px-2 py-0.5 rounded-full text-[11px] font-semibold bg-indigo-100 text-indigo-800">
              Nadgodziny: +{assignedHours - workload}h
            </span>
          {:else}
            <span class="px-2 py-0.5 rounded-full text-[11px] font-semibold bg-emerald-100 text-emerald-800">
              ✓ Pełen etat (100%)
            </span>
          {/if}
        </div>
      </div>
      <!-- Progress Bar -->
      <div class="w-full bg-slate-200 rounded-full h-2.5 overflow-hidden">
        <div
          class={`h-2.5 rounded-full transition-all duration-300 ${
            isUnderworked ? 'bg-amber-500' : isOverworked ? 'bg-indigo-600' : 'bg-emerald-500'
          }`}
          style="width: {Math.min(100, Math.round((assignedHours / (workload || 1)) * 100))}%"
        ></div>
      </div>
    </div>
  </div>

  <!-- Section 1: Przypisane lekcje i przedmioty (z opcją natychmiastowego dodawania) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>📚</span>
          <span>Plan zajęć prowadzonych przez tego nauczyciela ({teacherLessons.length})</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Kliknij w nazwę oddziału lub sali, aby natychmiast przejść do jej widoku.
        </p>
      </div>
      <span class="text-xs font-bold text-slate-700 bg-slate-100 px-3 py-1 rounded-full">
        Łącznie: {assignedHours} godz./tydzień
      </span>
    </div>

    <!-- Table of assigned lessons -->
    {#if teacherLessons.length === 0}
      <div class="p-8 text-center bg-slate-50/60 rounded-xl border border-dashed border-slate-200 text-slate-400 text-xs">
        Ten nauczyciel nie ma jeszcze przypisanych żadnych zajęć. Użyj formularza poniżej, aby dodać pierwszą lekcję.
      </div>
    {:else}
      <div class="border border-slate-200/90 rounded-xl overflow-hidden divide-y divide-slate-100">
        <div class="bg-slate-50/90 px-4 py-2.5 text-[11px] font-bold text-slate-500 uppercase tracking-wider grid grid-cols-12 gap-3 items-center">
          <div class="col-span-4">Przedmiot</div>
          <div class="col-span-3">Oddział / Klasa</div>
          <div class="col-span-2">Dedykowana sala</div>
          <div class="col-span-2 text-center">Wymiar</div>
          <div class="col-span-1 text-right">Akcja</div>
        </div>

        {#each teacherLessons as lesson}
          {@const assignedClass = editor.classes.find((c) => c.id === lesson.classId)}
          {@const assignedRoom = editor.rooms.find((r) => r.id === lesson.roomId)}
          <div class="px-4 py-3 text-xs grid grid-cols-12 gap-3 items-center hover:bg-slate-50/80 transition-colors">
            <!-- Przedmiot -->
            <div class="col-span-4">
              <span class="font-bold text-slate-900 block">{lesson.subject}</span>
              {#if lesson.duration === 2}
                <span class="inline-block mt-0.5 text-[10px] font-semibold px-1.5 py-0.2 bg-blue-50 text-blue-700 rounded border border-blue-200">
                  Blok 2h (podwójna lekcja)
                </span>
              {/if}
            </div>

            <!-- Oddział (Klikalny Cross-Link!) -->
            <div class="col-span-3">
              <button
                type="button"
                onclick={() => onNavigateToClass(lesson.classId)}
                class="px-2.5 py-1 bg-blue-50 hover:bg-blue-100 text-blue-800 rounded-lg font-bold border border-blue-200/80 transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs group"
                title="Przejdź do widoku tej klasy"
              >
                <span>👥</span>
                <span class="group-hover:underline">{assignedClass?.name || lesson.classId}</span>
                <span class="text-blue-400 group-hover:text-blue-600 text-[10px]">➔</span>
              </button>
            </div>

            <!-- Sala (Klikalny Cross-Link!) -->
            <div class="col-span-2">
              {#if assignedRoom}
                <button
                  type="button"
                  onclick={() => onNavigateToRoom(assignedRoom.id)}
                  class="px-2 py-1 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg font-medium border border-slate-200 transition-all cursor-pointer flex items-center gap-1 text-[11px]"
                  title="Przejdź do widoku tej sali"
                >
                  <span>🚪</span>
                  <span>{assignedRoom.name}</span>
                </button>
              {:else}
                <span class="text-slate-400 italic text-[11px]">Dowolna</span>
              {/if}
            </div>

            <!-- Wymiar -->
            <div class="col-span-2 text-center font-bold text-slate-800">
              {lesson.weeklyCount}h / tydz.
            </div>

            <!-- Usuwanie -->
            <div class="col-span-1 text-right">
              <button
                type="button"
                onclick={() => editor.deleteLesson(lesson.id)}
                class="p-1.5 text-slate-400 hover:text-rose-600 hover:bg-rose-50 rounded-lg transition-colors cursor-pointer"
                title="Usuń tę pozycję z planu"
              >
                ✕
              </button>
            </div>
          </div>
        {/each}
      </div>
    {/if}

    <!-- Pasek szybkiego przypisywania lekcji wprost w profilu nauczyciela -->
    <div class="pt-4 border-t border-slate-100 flex flex-col gap-3">
      <span class="text-xs font-bold text-slate-800 flex items-center gap-1.5">
        <span>➕</span>
        <span>Przypisz nową klasę / przedmiot dla {teacher.name}:</span>
      </span>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-12 gap-3 text-xs bg-slate-50/80 p-3.5 rounded-xl border border-slate-200/70">
        <!-- Klasa (3 cols) -->
        <div class="lg:col-span-3">
          <label for="assign-class-input-{teacher.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Oddział / Klasa:
          </label>
          <select
            id="assign-class-input-{teacher.id}"
            bind:value={newLessonClassId}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 cursor-pointer"
          >
            {#each editor.classes as c}
              <option value={c.id}>{c.name}</option>
            {/each}
          </select>
        </div>

        <!-- Przedmiot (4 cols) -->
        <div class="lg:col-span-4">
          <label for="assign-subject-input-{teacher.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Przedmiot (wybierz / wpisz):
          </label>
          <SubjectCombobox
            id="assign-subject-input-{teacher.id}"
            bind:value={newLessonSubject}
            subjects={editor.allKnownSubjects}
            onSelect={(s) => { newLessonSubject = s; }}
            onAddNew={(s) => editor.registerSubject(s)}
            placeholder="np. Matematyka"
          />
        </div>

        <!-- Sala (2 cols) -->
        <div class="lg:col-span-2">
          <label for="assign-room-input-{teacher.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Sala:
          </label>
          <select
            id="assign-room-input-{teacher.id}"
            bind:value={newLessonRoomId}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 cursor-pointer"
          >
            <option value="">Dowolna</option>
            {#each editor.rooms as r}
              <option value={r.id}>{r.name}</option>
            {/each}
          </select>
        </div>

        <!-- Godziny (1 col) -->
        <div class="lg:col-span-1">
          <label for="assign-hours-input-{teacher.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Godz./tydz.
          </label>
          <input
            id="assign-hours-input-{teacher.id}"
            type="number"
            min="1"
            max="12"
            bind:value={newLessonHours}
            class="w-full py-1.5 px-1 bg-white border border-slate-200 rounded-lg text-xs font-bold text-center text-slate-800"
          />
        </div>

        <!-- Akcja (2 cols) -->
        <div class="lg:col-span-2 flex items-end">
          <button
            type="button"
            onclick={handleAddLesson}
            class="w-full py-2 bg-slate-900 hover:bg-blue-600 text-white font-bold rounded-lg transition-all cursor-pointer shadow-xs text-xs"
          >
            + Dodaj lekcję
          </button>
        </div>
      </div>
    </div>
  </div>

  <!-- Section 2: Zintegrowana dostępność i czas pracy (Brak modalnych wyskakujących okien!) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>🗓️</span>
          <span>Dostępność i indywidualne reguły czasu pracy</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Klikaj na komórki lub całe dni/godziny, aby zablokować terminy niedostępności tego pedagoga.
        </p>
      </div>

      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={() => editor.clearEntityAvailability("teacher", teacher.id)}
          class="px-2.5 py-1 text-xs bg-slate-100 hover:bg-slate-200 text-slate-700 font-semibold rounded-lg transition-colors cursor-pointer"
        >
          Odblokuj wszystko
        </button>
        <span class={`text-xs font-bold px-2.5 py-1 rounded-full border ${
          unavailableSet.size > 0 ? 'bg-rose-50 text-rose-700 border-rose-200' : 'bg-emerald-50 text-emerald-700 border-emerald-200'
        }`}>
          {unavailableSet.size > 0 ? `${unavailableSet.size} godz. zablokowanych` : 'W pełni dyspozycyjny'}
        </span>
      </div>
    </div>

    <!-- Integrated Availability Grid -->
    <AvailabilityGrid
      days={editor.currentDaysFormatted}
      periodsPerDay={editor.periodsPerDay}
      {unavailableSet}
      totalSlots={editor.weeklySlotsCapacity}
      onToggleSlot={(dayId, period) => editor.toggleEntitySlot("teacher", teacher.id, `${dayId}_${period}`)}
      onToggleWholeDay={(dayId) => editor.toggleEntityDay("teacher", teacher.id, dayId)}
      onToggleWholePeriod={(period) => editor.toggleEntityPeriod("teacher", teacher.id, period)}
      onToggleAllSlots={handleToggleAllSlots}
    />

    <!-- Workload & Schedule Rules for this teacher -->
    <div class="grid grid-cols-1 sm:grid-cols-3 gap-3.5 pt-2">
      <!-- Dni w tygodniu -->
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/70 flex flex-col justify-between">
        <label for="teacher-cluster-input-{teacher.id}" class="block text-xs font-bold text-slate-800 mb-1">
          📅 Maks. dni roboczych:
        </label>
        <p class="text-[11px] text-slate-500 mb-2">Limit obecności w szkole (np. dla niepełnego etatu).</p>
        <select
          id="teacher-cluster-input-{teacher.id}"
          bind:value={teacher.maxDaysPerWeek}
          class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
        >
          <option value={null}>Brak limitu (cały tydzień)</option>
          {#each editor.currentDaysFormatted as d, i}
            {#if i + 1 < editor.currentDaysFormatted.length}
              <option value={i + 1}>Maks. {i + 1} {i === 0 ? "dzień" : "dni"}</option>
            {/if}
          {/each}
        </select>
      </div>

      <!-- Lekcji dziennie -->
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/70 flex flex-col justify-between">
        <label for="teacher-max-daily-{teacher.id}" class="block text-xs font-bold text-slate-800 mb-1">
          ⏱️ Maks. lekcji w dniu:
        </label>
        <p class="text-[11px] text-slate-500 mb-2">Ochrona przed przeciążeniem dziennym.</p>
        <select
          id="teacher-max-daily-{teacher.id}"
          bind:value={teacher.maxDailyLessons}
          class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
        >
          <option value={null}>Domyślny ze szkoły</option>
          <option value={3}>Maks. 3 lekcje</option>
          <option value={4}>Maks. 4 lekcje</option>
          <option value={5}>Maks. 5 lekcji</option>
          <option value={6}>Maks. 6 lekcji</option>
          <option value={7}>Maks. 7 lekcji</option>
          <option value={8}>Maks. 8 lekcji</option>
        </select>
      </div>

      <!-- Okienka w tygodniu -->
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/70 flex flex-col justify-between">
        <label for="teacher-max-gaps-{teacher.id}" class="block text-xs font-bold text-slate-800 mb-1">
          ☕ Dopuszczalne okienka:
        </label>
        <p class="text-[11px] text-slate-500 mb-2">Limit wolnych godzin pomiędzy lekcjami.</p>
        <select
          id="teacher-max-gaps-{teacher.id}"
          bind:value={teacher.maxGapsPerWeek}
          class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
        >
          <option value={null}>Domyślny ze szkoły</option>
          <option value={0}>0 okienek (plan ciągły)</option>
          <option value={1}>Maks. 1 okienko</option>
          <option value={2}>Maks. 2 okienka</option>
          <option value={3}>Maks. 3 okienka</option>
          <option value={4}>Maks. 4 okienka</option>
        </select>
      </div>
    </div>
  </div>
</div>

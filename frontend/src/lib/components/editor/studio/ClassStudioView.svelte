<!-- frontend/src/lib/components/editor/studio/ClassStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import type { SimpleClass } from "../types";
  import AvailabilityGrid from "../availability/AvailabilityGrid.svelte";
  import SubjectCombobox from "../common/SubjectCombobox.svelte";

  interface Props {
    editor: EditorState;
    cls: SimpleClass;
    onNavigateToTeacher: (teacherId: string) => void;
    onNavigateToRoom: (roomId: string) => void;
  }

  let { editor, cls, onNavigateToTeacher, onNavigateToRoom }: Props = $props();

  // Local state for quick subject assigner inside class profile
  let newLessonSubject = $state<string>("Matematyka");
  let newLessonTeacherId = $state<string>("");
  let newLessonRoomId = $state<string>("");
  let newLessonHours = $state<number>(4);
  let newLessonIsBlock = $state<boolean>(false);

  // Sync default teacher when subject changes
  function handleSubjectChange(sub: string) {
    newLessonSubject = sub;
    if (sub.trim()) {
      editor.registerSubject(sub.trim());
      const match = editor.teachers.find(
        (t) => t.subject?.toLowerCase() === sub.toLowerCase()
      );
      if (match) {
        newLessonTeacherId = match.id;
      }
    }
  }

  $effect(() => {
    if (editor.teachers.length > 0 && !newLessonTeacherId) {
      newLessonTeacherId = editor.teachers[0].id;
    }
  });

  let classLessons = $derived(editor.lessons.filter((l) => l.classId === cls.id));
  let totalHours = $derived(editor.getClassWeeklyHours(cls.id));
  let unavailableSet = $derived(new Set(cls.unavailableTimes || []));

  function handleAddLesson() {
    if (!newLessonSubject.trim() || !newLessonTeacherId || newLessonHours < 1) return;
    editor.assignLessonToClass(
      cls.id,
      newLessonSubject.trim(),
      newLessonTeacherId,
      newLessonRoomId,
      newLessonHours,
      newLessonIsBlock
    );
  }

  function handleToggleAllSlots() {
    if (unavailableSet.size === editor.weeklySlotsCapacity) {
      editor.clearEntityAvailability("class", cls.id);
    } else {
      const all: string[] = [];
      editor.currentDaysFormatted.forEach((d) => {
        for (let p = 1; p <= editor.periodsPerDay; p++) all.push(`${d.id}_${p}`);
      });
      cls.unavailableTimes = all;
      editor.classes = [...editor.classes];
    }
  }
</script>

<div class="flex flex-col gap-6">
  <!-- Class Card Header -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
      <div class="flex items-center gap-4">
        <div class="w-14 h-14 rounded-2xl bg-indigo-50 border border-indigo-200/80 text-indigo-700 flex items-center justify-center text-2xl font-bold shrink-0">
          👥
        </div>
        <div>
          <div class="flex items-center gap-2.5">
            <input
              type="text"
              bind:value={cls.name}
              class="text-xl font-bold text-slate-900 bg-transparent hover:bg-slate-50 focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg px-1.5 py-0.5 border border-transparent hover:border-slate-300 transition-all"
              title="Kliknij, aby zmienić nazwę oddziału"
            />
          </div>
          <p class="text-xs text-slate-500 px-1.5 mt-0.5 flex items-center gap-2">
            <span>Rocznik / Poziom: <strong>Klasa {cls.grade || editor.getClassGrade(cls)}</strong></span>
            <span>•</span>
            <span>ID: <code class="text-slate-400 font-mono">{cls.id}</code></span>
          </p>
        </div>
      </div>

      <!-- Quick KPI summary for this class -->
      <div class="flex items-center gap-3">
        <div class="bg-slate-50 border border-slate-200/90 rounded-xl px-4 py-2 text-right">
          <span class="text-[10px] uppercase font-bold text-slate-400 block tracking-wider">Liczba lekcji w tygodniu:</span>
          <div class="flex items-center justify-end gap-1.5">
            <span class="text-xl font-extrabold text-blue-700">{totalHours}</span>
            <span class="text-xs font-semibold text-slate-500">godz./tydz.</span>
          </div>
        </div>

        {#if totalHours === 0}
          <span class="px-3 py-1.5 rounded-xl text-xs font-bold bg-amber-50 text-amber-800 border border-amber-200 flex items-center gap-1.5">
            <span>⚠️</span>
            <span>Brak lekcji</span>
          </span>
        {:else}
          <span class="px-3 py-1.5 rounded-xl text-xs font-bold bg-emerald-50 text-emerald-800 border border-emerald-200 flex items-center gap-1">
            <span>✓</span>
            <span>Aktywny oddział</span>
          </span>
        {/if}
      </div>
    </div>
  </div>

  <!-- Section 1: Plan nauczania i przedmioty oddziału -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>📖</span>
          <span>Plan nauczania oddziału {cls.name} ({classLessons.length} przedmiotów)</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Kliknij nazwisko nauczyciela lub salę, aby bezpośrednio przejść do jej widoku.
        </p>
      </div>
      <span class="text-xs font-bold text-slate-700 bg-slate-100 px-3 py-1 rounded-full">
        Łącznie: {totalHours} godz./tydzień
      </span>
    </div>

    <!-- Table of class curriculum -->
    {#if classLessons.length === 0}
      <div class="p-8 text-center bg-slate-50/60 rounded-xl border border-dashed border-slate-200 text-slate-400 text-xs">
        Ten oddział nie ma jeszcze zaplanowanych żadnych zajęć. Użyj formularza poniżej, aby przypisać pierwszy przedmiot.
      </div>
    {:else}
      <div class="border border-slate-200/90 rounded-xl overflow-hidden divide-y divide-slate-100">
        <div class="bg-slate-50/90 px-4 py-2.5 text-[11px] font-bold text-slate-500 uppercase tracking-wider grid grid-cols-12 gap-3 items-center">
          <div class="col-span-4">Przedmiot</div>
          <div class="col-span-3">Prowadzący nauczyciel</div>
          <div class="col-span-2">Dedykowana sala</div>
          <div class="col-span-2 text-center">Wymiar</div>
          <div class="col-span-1 text-right">Akcja</div>
        </div>

        {#each classLessons as lesson}
          {@const assignedTeacher = editor.teachers.find((t) => t.id === lesson.teacherId)}
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

            <!-- Nauczyciel (Klikalny Cross-Link!) -->
            <div class="col-span-3">
              {#if assignedTeacher}
                <button
                  type="button"
                  onclick={() => onNavigateToTeacher(assignedTeacher.id)}
                  class="px-2.5 py-1 bg-blue-50 hover:bg-blue-100 text-blue-800 rounded-lg font-bold border border-blue-200/80 transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs group text-left"
                  title="Przejdź do profilu tego nauczyciela"
                >
                  <span>👨‍🏫</span>
                  <span class="group-hover:underline truncate max-w-[150px]">{assignedTeacher.name}</span>
                  <span class="text-blue-400 group-hover:text-blue-600 text-[10px]">➔</span>
                </button>
              {:else}
                <span class="text-rose-500 font-semibold italic text-[11px]">Brak nauczyciela</span>
              {/if}
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
                title="Usuń ten przedmiot z planu oddziału"
              >
                ✕
              </button>
            </div>
          </div>
        {/each}
      </div>
    {/if}

    <!-- Pasek szybkiego dodawania przedmiotu wprost w profilu klasy -->
    <div class="pt-4 border-t border-slate-100 flex flex-col gap-3">
      <span class="text-xs font-bold text-slate-800 flex items-center gap-1.5">
        <span>➕</span>
        <span>Dodaj brakujący przedmiot do planu {cls.name}:</span>
      </span>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-12 gap-3 text-xs bg-slate-50/80 p-3.5 rounded-xl border border-slate-200/70">
        <!-- Przedmiot (4 cols) -->
        <div class="lg:col-span-4">
          <label for="class-subject-input-{cls.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Przedmiot (wybierz / wpisz):
          </label>
          <SubjectCombobox
            id="class-subject-input-{cls.id}"
            bind:value={newLessonSubject}
            subjects={editor.allKnownSubjects}
            onSelect={handleSubjectChange}
            onAddNew={(s) => editor.registerSubject(s)}
            placeholder="np. Informatyka"
          />
        </div>

        <!-- Nauczyciel (3 cols) -->
        <div class="lg:col-span-3">
          <label for="class-teacher-input-{cls.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Nauczyciel prowadzący:
          </label>
          <select
            id="class-teacher-input-{cls.id}"
            bind:value={newLessonTeacherId}
            class="w-full py-1.5 px-2 bg-white border border-slate-200 rounded-lg text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 cursor-pointer"
          >
            {#each editor.teachers as t}
              <option value={t.id}>{t.name} ({t.subject})</option>
            {/each}
          </select>
        </div>

        <!-- Sala (2 cols) -->
        <div class="lg:col-span-2">
          <label for="class-room-input-{cls.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Sala:
          </label>
          <select
            id="class-room-input-{cls.id}"
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
          <label for="class-hours-input-{cls.id}" class="block text-[10px] font-bold text-slate-500 uppercase mb-1">
            Godz./tydz.
          </label>
          <input
            id="class-hours-input-{cls.id}"
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
            + Dodaj przedmiot
          </button>
        </div>
      </div>
    </div>
  </div>

  <!-- Section 2: Dostępność i ramy higieniczne oddziału (Wbudowane w widok!) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>🗓️</span>
          <span>Dostępność i higiena planu oddziału {cls.name}</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Zablokuj godziny, w których ta klasa nie może mieć lekcji (np. dni bez zajęć, późne godziny w piątek).
        </p>
      </div>

      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={() => editor.clearEntityAvailability("class", cls.id)}
          class="px-2.5 py-1 text-xs bg-slate-100 hover:bg-slate-200 text-slate-700 font-semibold rounded-lg transition-colors cursor-pointer"
        >
          Odblokuj wszystko
        </button>
        <span class={`text-xs font-bold px-2.5 py-1 rounded-full border ${
          unavailableSet.size > 0 ? 'bg-rose-50 text-rose-700 border-rose-200' : 'bg-emerald-50 text-emerald-700 border-emerald-200'
        }`}>
          {unavailableSet.size > 0 ? `${unavailableSet.size} godz. wykluczonych` : 'Brak wykluczeń'}
        </span>
      </div>
    </div>

    <!-- Integrated Availability Grid -->
    <AvailabilityGrid
      days={editor.currentDaysFormatted}
      periodsPerDay={editor.periodsPerDay}
      {unavailableSet}
      totalSlots={editor.weeklySlotsCapacity}
      onToggleSlot={(dayId, period) => editor.toggleEntitySlot("class", cls.id, `${dayId}_${period}`)}
      onToggleWholeDay={(dayId) => editor.toggleEntityDay("class", cls.id, dayId)}
      onToggleWholePeriod={(period) => editor.toggleEntityPeriod("class", cls.id, period)}
      onToggleAllSlots={handleToggleAllSlots}
    />

    <!-- Class daily limits -->
    <div class="grid grid-cols-1 sm:grid-cols-2 gap-3.5 pt-2">
      <!-- Min lekcji dziennie -->
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/70 flex flex-col justify-between">
        <label for="class-min-daily-{cls.id}" class="block text-xs font-bold text-slate-800 mb-1">
          📉 Minimalna liczba lekcji w dniu:
        </label>
        <p class="text-[11px] text-slate-500 mb-2">Zapobiega przychodzeniu uczniów na zbyt małą liczbę lekcji.</p>
        <select
          id="class-min-daily-{cls.id}"
          bind:value={cls.minDailyLessons}
          class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
        >
          <option value={null}>Domyślny ze szkoły</option>
          <option value={2}>Min. 2 lekcje</option>
          <option value={3}>Min. 3 lekcje</option>
          <option value={4}>Min. 4 lekcje</option>
          <option value={5}>Min. 5 lekcji</option>
        </select>
      </div>

      <!-- Maks lekcji dziennie -->
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/70 flex flex-col justify-between">
        <label for="class-max-daily-{cls.id}" class="block text-xs font-bold text-slate-800 mb-1">
          📈 Maksymalna liczba lekcji w dniu:
        </label>
        <p class="text-[11px] text-slate-500 mb-2">Chroni uczniów przed nadmiernym zmęczeniem długim dniem.</p>
        <select
          id="class-max-daily-{cls.id}"
          bind:value={cls.maxDailyLessons}
          class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
        >
          <option value={null}>Domyślny ze szkoły</option>
          <option value={5}>Maks. 5 lekcji</option>
          <option value={6}>Maks. 6 lekcji</option>
          <option value={7}>Maks. 7 lekcji</option>
          <option value={8}>Maks. 8 lekcji</option>
          <option value={9}>Maks. 9 lekcji</option>
        </select>
      </div>
    </div>
  </div>
</div>


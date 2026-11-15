<!-- frontend/src/lib/components/editor/studio/ClassStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import type { SimpleClass } from "../types";
  import AvailabilityGrid from "../availability/AvailabilityGrid.svelte";
  import SubjectCombobox from "../common/SubjectCombobox.svelte";
  import Icon from "../../common/Icon.svelte";

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

  // Inline row editing state
  let editingLessonId = $state<string | null>(null);
  let editSubject = $state("");
  let editTeacherId = $state("");
  let editRoomId = $state("");
  let editWeeklyCount = $state(1);
  let editIsBlock = $state(false);

  function startEditingLesson(lesson: typeof editor.lessons[0]) {
    editingLessonId = lesson.id;
    editSubject = lesson.subject;
    editTeacherId = lesson.teacherId || "";
    editRoomId = lesson.roomId || "";
    editWeeklyCount = lesson.weeklyCount;
    editIsBlock = lesson.duration === 2;
  }

  function cancelEditingLesson() {
    editingLessonId = null;
  }

  function saveEditedLesson(lessonId: string) {
    if (!editSubject.trim() || editWeeklyCount < 1) return;
    editor.updateLesson(lessonId, {
      subject: editSubject.trim(),
      teacherId: editTeacherId,
      roomId: editRoomId,
      weeklyCount: editWeeklyCount,
      duration: editIsBlock ? 2 : 1,
    });
    editingLessonId = null;
  }

  let roomsByCategory = $derived.by(() => {
    const map = new Map<string, typeof editor.rooms>();
    for (const r of editor.rooms) {
      const cat = r.type?.trim() || "Ogólna / Dydaktyczna";
      if (!map.has(cat)) map.set(cat, []);
      map.get(cat)!.push(r);
    }
    return map;
  });

  // Sync default teacher when subject changes
  function handleSubjectChange(sub: string) {
    newLessonSubject = sub;
    if (sub.trim()) {
      editor.registerSubject(sub.trim());
      const match = editor.teachers.find((t) =>
        editor.getTeacherSubjects(t).some((s) => s.toLowerCase() === sub.toLowerCase())
      );
      if (match) {
        newLessonTeacherId = match.id;
      }
    }
  }

  let classLessons = $derived(editor.lessons.filter((l) => l.classId === cls.id));
  let totalHours = $derived(editor.getClassWeeklyHours(cls.id));
  let unavailableSet = $derived(new Set(cls.unavailableTimes || []));

  function handleAddLesson() {
    if (!newLessonSubject.trim() || newLessonHours < 1) return;
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
        <div class="w-14 h-14 rounded-2xl bg-indigo-50 border border-indigo-200/80 text-indigo-700 flex items-center justify-center font-bold shrink-0">
          <Icon name="groups" size="2xl" class="text-indigo-700" />
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
            <Icon name="warning" size="xs" class="text-amber-700" />
            <span>Brak lekcji</span>
          </span>
        {:else}
          <span class="px-3 py-1.5 rounded-xl text-xs font-bold bg-emerald-50 text-emerald-800 border border-emerald-200 flex items-center gap-1">
            <Icon name="check" size="xs" class="text-emerald-700" />
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
          <Icon name="menu_book" size="xs" class="text-slate-500" />
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
          <div class="col-span-1 text-center">Wymiar</div>
          <div class="col-span-2 text-right">Akcje</div>
        </div>

        {#each classLessons as lesson}
          {@const assignedTeacher = editor.teachers.find((t) => t.id === lesson.teacherId)}
          {@const assignedRoom = editor.rooms.find((r) => r.id === lesson.roomId)}
          {#if editingLessonId === lesson.id}
            <!-- Inline Row Editor -->
            <div class="p-3.5 bg-blue-50/70 border-y border-blue-200/90 flex flex-col gap-3">
              <div class="flex items-center justify-between text-xs font-bold text-blue-900">
                <span class="flex items-center gap-1.5">
                  <Icon name="edit" size="xs" class="text-blue-700" />
                  <span>Edycja przedmiotu: {lesson.subject}</span>
                </span>
                <span class="text-[11px] text-blue-700 font-normal">Zmień dane i kliknij Zapisz</span>
              </div>
              <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-12 gap-2.5 text-xs">
                <!-- Przedmiot -->
                <div class="lg:col-span-4">
                  <label for="class-edit-sub-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Przedmiot:</label>
                  <input
                    id="class-edit-sub-{lesson.id}"
                    type="text"
                    bind:value={editSubject}
                    list="class-edit-sub-list-{lesson.id}"
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg font-bold text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500"
                  />
                  <datalist id="class-edit-sub-list-{lesson.id}">
                    {#each editor.allKnownSubjects as s}
                      <option value={s}></option>
                    {/each}
                  </datalist>
                </div>
                <!-- Nauczyciel -->
                <div class="lg:col-span-3">
                  <label for="class-edit-teacher-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Nauczyciel:</label>
                  <select
                    id="class-edit-teacher-{lesson.id}"
                    bind:value={editTeacherId}
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500 font-semibold"
                  >
                    <option value="__ANY__">[Auto] Dowolny nauczyciel (dobiera system)</option>
                    <option value="">[Wakat] Brak nauczyciela</option>
                    <optgroup label="Przypisz konkretnego pedagoga:">
                      {#each editor.teachers as t}
                        <option value={t.id}>{t.name} ({editor.getTeacherSubjects(t).join(", ") || t.subject})</option>
                      {/each}
                    </optgroup>
                  </select>
                </div>
                <!-- Sala -->
                <div class="lg:col-span-3">
                  <label for="class-edit-room-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Sala:</label>
                  <select
                    id="class-edit-room-{lesson.id}"
                    bind:value={editRoomId}
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500"
                  >
                    <option value="">(Dowolna / brak)</option>
                    {#each Array.from(roomsByCategory.entries()) as [category, rms]}
                      <optgroup label={category}>
                        {#each rms as r}
                          <option value={r.id}>{r.name}</option>
                        {/each}
                      </optgroup>
                    {/each}
                  </select>
                </div>
                <!-- Wymiar i blok -->
                <div class="lg:col-span-2 flex items-center gap-2">
                  <div class="w-16">
                    <label for="class-edit-count-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Godz./tydz:</label>
                    <input
                      id="class-edit-count-{lesson.id}"
                      type="number"
                      min="1"
                      max="16"
                      bind:value={editWeeklyCount}
                      class="w-full px-1.5 py-1.5 bg-white border border-slate-300 rounded-lg font-bold text-slate-900 text-center"
                    />
                  </div>
                  <div class="pt-4">
                    <label class="inline-flex items-center gap-1 text-[11px] text-slate-700 font-semibold cursor-pointer">
                      <input type="checkbox" bind:checked={editIsBlock} class="rounded text-blue-600" />
                      <span>Blok 2h</span>
                    </label>
                  </div>
                </div>
              </div>
              <div class="flex justify-end gap-2 pt-2 border-t border-blue-200/60">
                <button
                  type="button"
                  onclick={cancelEditingLesson}
                  class="px-3 py-1 bg-white hover:bg-slate-100 text-slate-700 border border-slate-300 rounded-lg text-xs font-semibold cursor-pointer"
                >
                  Anuluj
                </button>
                <button
                  type="button"
                  onclick={() => saveEditedLesson(lesson.id)}
                  class="px-3.5 py-1 bg-blue-600 hover:bg-blue-700 text-white rounded-lg text-xs font-bold cursor-pointer shadow-2xs flex items-center gap-1"
                >
                  <Icon name="check" size="xs" class="text-white" />
                  <span>Zapisz zmiany</span>
                </button>
              </div>
            </div>
          {:else}
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

              <!-- Nauczyciel (Klikalny Cross-Link, Dowolny nauczyciel lub Wakat) -->
              <div class="col-span-3">
                {#if lesson.teacherId === "__ANY__"}
                  {@const qualified = editor.getQualifiedTeachersForSubject(lesson.subject)}
                  <div class="flex flex-col gap-0.5">
                    <span class="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-lg bg-indigo-50 text-indigo-900 font-bold text-xs border border-indigo-200 shadow-2xs">
                      <Icon name="shuffle" size="xs" class="text-indigo-700" />
                      <span>Dowolny nauczyciel</span>
                    </span>
                    <span class="text-[10px] text-indigo-600 truncate max-w-[170px]" title={qualified.map((t) => t.name).join(", ")}>
                      {#if qualified.length > 0}
                        Kandydaci ({qualified.length}): {qualified.map((t) => t.name).join(", ")}
                      {:else}
                        Dowolny pedagog z kadry
                      {/if}
                    </span>
                  </div>
                {:else if assignedTeacher}
                  <button
                    type="button"
                    onclick={() => onNavigateToTeacher(assignedTeacher.id)}
                    class="px-2.5 py-1 bg-blue-50 hover:bg-blue-100 text-blue-800 rounded-lg font-bold border border-blue-200/80 transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs group text-left"
                    title="Przejdź do profilu tego nauczyciela"
                  >
                    <Icon name="badge" size="xs" class="text-blue-600" />
                    <span class="group-hover:underline truncate max-w-[150px]">{assignedTeacher.name}</span>
                    <Icon name="arrow_forward" size="xs" class="text-blue-400 group-hover:text-blue-600" />
                  </button>
                {:else}
                  <span class="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-md bg-slate-100 text-slate-600 font-medium text-[11px] border border-slate-200 shadow-2xs">
                    <Icon name="person_off" size="xs" class="text-slate-400" />
                    <span>Wakat (brak nauczyciela)</span>
                  </span>
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
                    <Icon name="meeting_room" size="xs" class="text-slate-500" />
                    <span>{assignedRoom.name}</span>
                  </button>
                {:else}
                  <span class="text-slate-400 italic text-[11px]">Dowolna</span>
                {/if}
              </div>

              <!-- Wymiar -->
              <div class="col-span-1 text-center font-bold text-slate-800">
                {lesson.weeklyCount}h / tydz.
              </div>

              <!-- Akcje: Edycja i Usuwanie -->
              <div class="col-span-2 text-right flex items-center justify-end gap-1">
                <button
                  type="button"
                  onclick={() => startEditingLesson(lesson)}
                  class="p-1.5 text-slate-500 hover:text-blue-700 hover:bg-blue-50 rounded-lg transition-colors cursor-pointer"
                  title="Edytuj ten przedmiot"
                >
                  <Icon name="edit" size="xs" />
                </button>
                <button
                  type="button"
                  onclick={() => editor.deleteLesson(lesson.id)}
                  class="p-1.5 text-slate-400 hover:text-rose-600 hover:bg-rose-50 rounded-lg transition-colors cursor-pointer"
                  title="Usuń ten przedmiot z planu oddziału"
                >
                  <Icon name="close" size="xs" />
                </button>
              </div>
            </div>
          {/if}
        {/each}
      </div>
    {/if}

    <!-- Pasek szybkiego dodawania przedmiotu wprost w profilu klasy -->
    <div class="pt-4 border-t border-slate-100 flex flex-col gap-3">
      <span class="text-xs font-bold text-slate-800 flex items-center gap-1.5">
        <Icon name="add" size="xs" class="text-slate-700" />
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
            <option value="__ANY__">[Auto] Dowolny nauczyciel (dobiera system)</option>
            <option value="">[Wakat] Brak nauczyciela</option>
            <optgroup label="Przypisz konkretnego pedagoga:">
              {#each editor.teachers as t}
                <option value={t.id}>{t.name} ({editor.getTeacherSubjects(t).join(", ") || t.subject})</option>
              {/each}
            </optgroup>
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
            {#each Array.from(roomsByCategory.entries()) as [category, rms]}
              <optgroup label={category}>
                {#each rms as r}
                  <option value={r.id}>{r.name}</option>
                {/each}
              </optgroup>
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
            max="16"
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
          <Icon name="calendar_month" size="base" class="text-indigo-600" />
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
        <label for="class-min-daily-{cls.id}" class="text-xs font-bold text-slate-800 mb-1 flex items-center gap-1.5">
          <Icon name="trending_down" size="xs" class="text-blue-600" />
          <span>Minimalna liczba lekcji w dniu:</span>
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
        <label for="class-max-daily-{cls.id}" class="text-xs font-bold text-slate-800 mb-1 flex items-center gap-1.5">
          <Icon name="trending_up" size="xs" class="text-amber-600" />
          <span>Maksymalna liczba lekcji w dniu:</span>
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


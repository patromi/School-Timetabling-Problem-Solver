<!-- frontend/src/lib/components/editor/studio/RoomStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import type { SimpleRoom, RoomType } from "../types";
  import AvailabilityGrid from "../availability/AvailabilityGrid.svelte";

  interface Props {
    editor: EditorState;
    room: SimpleRoom;
    onNavigateToClass: (classId: string) => void;
    onNavigateToTeacher: (teacherId: string) => void;
  }

  let { editor, room, onNavigateToClass, onNavigateToTeacher }: Props = $props();

  let roomLessons = $derived(editor.getRoomLessons(room.id));
  let assignedHours = $derived(editor.getRoomAssignedHours(room.id));
  let unavailableSet = $derived(new Set(room.unavailableTimes || []));
  let totalSlots = $derived(editor.weeklySlotsCapacity);
  let utilizationPercent = $derived(totalSlots > 0 ? Math.round((assignedHours / totalSlots) * 100) : 0);

  function getRoomCategoryIcon(cat: string): string {
    const lower = (cat || "").toLowerCase();
    if (lower.includes("gimnas") || lower.includes("wf") || lower.includes("sport") || lower.includes("hala") || lower.includes("basen")) return "🏀";
    if (lower.includes("komp") || lower.includes("it") || lower.includes("inf")) return "💻";
    if (lower.includes("lab") || lower.includes("chem") || lower.includes("fiz") || lower.includes("biol") || lower.includes("przyrod")) return "🔬";
    if (lower.includes("język") || lower.includes("angiel") || lower.includes("niemiec")) return "🗣️";
    if (lower.includes("warsztat") || lower.includes("tech") || lower.includes("praktyk")) return "🔧";
    if (lower.includes("muzyk") || lower.includes("plastyk") || lower.includes("sztuk")) return "🎨";
    return "🚪";
  }

  // Inline row editing state
  let editingLessonId = $state<string | null>(null);
  let editSubject = $state("");
  let editClassId = $state("");
  let editTeacherId = $state("");
  let editRoomId = $state("");
  let editWeeklyCount = $state(1);
  let editIsBlock = $state(false);

  function startEditingLesson(lesson: typeof editor.lessons[0]) {
    editingLessonId = lesson.id;
    editSubject = lesson.subject;
    editClassId = lesson.classId;
    editTeacherId = lesson.teacherId || "";
    editRoomId = lesson.roomId || "";
    editWeeklyCount = lesson.weeklyCount;
    editIsBlock = lesson.duration === 2;
  }

  function cancelEditingLesson() {
    editingLessonId = null;
  }

  function saveEditedLesson(lessonId: string) {
    if (!editSubject.trim() || !editClassId || editWeeklyCount < 1) return;
    editor.updateLesson(lessonId, {
      subject: editSubject.trim(),
      classId: editClassId,
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

  function handleToggleAllSlots() {
    if (unavailableSet.size === totalSlots) {
      editor.clearEntityAvailability("room", room.id);
    } else {
      const all: string[] = [];
      editor.currentDaysFormatted.forEach((d) => {
        for (let p = 1; p <= editor.periodsPerDay; p++) all.push(`${d.id}_${p}`);
      });
      room.unavailableTimes = all;
      editor.rooms = [...editor.rooms];
    }
  }

  function handleUnassignLesson(lessonId: string) {
    const lesson = editor.lessons.find((l) => l.id === lessonId);
    if (lesson) {
      lesson.roomId = "";
      editor.lessons = [...editor.lessons];
    }
  }
</script>

<div class="flex flex-col gap-6">
  <!-- Room Card Header -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
      <div class="flex items-center gap-4">
        <div class="w-14 h-14 rounded-2xl bg-amber-50 border border-amber-200/80 text-amber-700 flex items-center justify-center text-2xl font-bold shrink-0">
          {getRoomCategoryIcon(room.type)}
        </div>
        <div>
          <div class="flex items-center gap-2.5">
            <input
              type="text"
              bind:value={room.name}
              class="text-xl font-bold text-slate-900 bg-transparent hover:bg-slate-50 focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg px-1.5 py-0.5 border border-transparent hover:border-slate-300 transition-all"
              title="Kliknij, aby zmienić nazwę sali"
            />
          </div>
          <p class="text-xs text-slate-500 px-1.5 mt-0.5 flex items-center gap-2">
            <span>Kategoria: <strong>{room.type || "Ogólna"}</strong></span>
            <span>•</span>
            <span>ID: <code class="text-slate-400 font-mono">{room.id}</code></span>
          </p>
        </div>
      </div>

      <!-- Quick Room Parameters -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="flex items-center gap-2 bg-slate-50 border border-slate-200/90 rounded-xl px-3 py-1.5 text-xs">
          <label for="room-type-select-{room.id}" class="text-slate-500 font-medium">Grupa / Typ sali:</label>
          <input
            id="room-type-select-{room.id}"
            type="text"
            bind:value={room.type}
            list="room-category-list-{room.id}"
            placeholder="np. Pracownia komputerowa"
            class="font-bold text-slate-800 bg-transparent focus:outline-none w-48 text-right"
          />
          <datalist id="room-category-list-{room.id}">
            {#each editor.availableRoomCategories as cat}
              <option value={cat}></option>
            {/each}
          </datalist>
        </div>

        <div class="bg-slate-50 border border-slate-200/90 rounded-xl px-4 py-1.5 text-right">
          <span class="text-[10px] uppercase font-bold text-slate-400 block tracking-wider">Zajętość sali:</span>
          <div class="flex items-center justify-end gap-1.5">
            <span class="text-base font-extrabold text-slate-900">{assignedHours} / {totalSlots}h</span>
            <span class="text-[11px] font-semibold text-slate-500">({utilizationPercent}%)</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- Section 1: Przypisane zajęcia w tej sali -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>📋</span>
          <span>Harmonogram wykorzystania sali ({roomLessons.length} lekcji)</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Lista lekcji, którym przypisano tę salę jako pomieszczenie dedykowane.
        </p>
      </div>
      <span class="text-xs font-bold text-slate-700 bg-slate-100 px-3 py-1 rounded-full">
        Łącznie zaplanowano: {assignedHours} godz./tydzień
      </span>
    </div>

    {#if roomLessons.length === 0}
      <div class="p-8 text-center bg-slate-50/60 rounded-xl border border-dashed border-slate-200 text-slate-400 text-xs">
        Żadna lekcja nie ma jeszcze na stałe przypisanej tej sali. Możesz ją przypisać w widoku Nauczyciela lub Oddziału.
      </div>
    {:else}
      <div class="border border-slate-200/90 rounded-xl overflow-hidden divide-y divide-slate-100">
        <div class="bg-slate-50/90 px-4 py-2.5 text-[11px] font-bold text-slate-500 uppercase tracking-wider grid grid-cols-12 gap-3 items-center">
          <div class="col-span-4">Przedmiot</div>
          <div class="col-span-3">Oddział / Klasa</div>
          <div class="col-span-3">Prowadzący nauczyciel</div>
          <div class="col-span-1 text-center">Wymiar</div>
          <div class="col-span-1 text-right">Akcje</div>
        </div>

        {#each roomLessons as lesson}
          {@const assignedClass = editor.classes.find((c) => c.id === lesson.classId)}
          {@const assignedTeacher = editor.teachers.find((t) => t.id === lesson.teacherId)}
          {#if editingLessonId === lesson.id}
            <!-- Inline Row Editor -->
            <div class="p-3.5 bg-blue-50/70 border-y border-blue-200/90 flex flex-col gap-3">
              <div class="flex items-center justify-between text-xs font-bold text-blue-900">
                <span class="flex items-center gap-1.5">
                  <span>✏️</span>
                  <span>Edycja zajęć w sali: {lesson.subject}</span>
                </span>
                <span class="text-[11px] text-blue-700 font-normal">Zmień parametry i kliknij Zapisz</span>
              </div>
              <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-12 gap-2.5 text-xs">
                <!-- Przedmiot -->
                <div class="lg:col-span-3">
                  <label for="room-edit-sub-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Przedmiot:</label>
                  <input
                    id="room-edit-sub-{lesson.id}"
                    type="text"
                    bind:value={editSubject}
                    list="room-edit-sub-list-{lesson.id}"
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg font-bold text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500"
                  />
                  <datalist id="room-edit-sub-list-{lesson.id}">
                    {#each editor.allKnownSubjects as s}
                      <option value={s}></option>
                    {/each}
                  </datalist>
                </div>
                <!-- Oddział -->
                <div class="lg:col-span-3">
                  <label for="room-edit-class-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Oddział (klasa):</label>
                  <select
                    id="room-edit-class-{lesson.id}"
                    bind:value={editClassId}
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500 font-semibold"
                  >
                    {#each editor.classes as c}
                      <option value={c.id}>{c.name || c.id}</option>
                    {/each}
                  </select>
                </div>
                <!-- Nauczyciel -->
                <div class="lg:col-span-3">
                  <label for="room-edit-teacher-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Nauczyciel:</label>
                  <select
                    id="room-edit-teacher-{lesson.id}"
                    bind:value={editTeacherId}
                    class="w-full px-2.5 py-1.5 bg-white border border-slate-300 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500"
                  >
                    <option value="__ANY__">🎲 Dowolny nauczyciel (dobiera system)</option>
                    <option value="">👤 Brak nauczyciela (wakat)</option>
                    <optgroup label="Przypisz konkretnego pedagoga:">
                      {#each editor.teachers as t}
                        <option value={t.id}>{t.name} ({editor.getTeacherSubjects(t).join(", ") || t.subject})</option>
                      {/each}
                    </optgroup>
                  </select>
                </div>
                <!-- Sala (można zmienić salę lub odpiąć) -->
                <div class="lg:col-span-2">
                  <label for="room-edit-room-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Sala:</label>
                  <select
                    id="room-edit-room-{lesson.id}"
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
                <!-- Wymiar -->
                <div class="lg:col-span-1">
                  <label for="room-edit-count-{lesson.id}" class="block text-[10px] font-bold text-slate-600 uppercase mb-1">Godz./tydz:</label>
                  <input
                    id="room-edit-count-{lesson.id}"
                    type="number"
                    min="1"
                    max="16"
                    bind:value={editWeeklyCount}
                    class="w-full px-1.5 py-1.5 bg-white border border-slate-300 rounded-lg font-bold text-slate-900 text-center"
                  />
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
                  <span>✓</span>
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
                    Blok 2h
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

              <!-- Nauczyciel (Klikalny Cross-Link, Dowolny nauczyciel lub Wakat) -->
              <div class="col-span-3">
                {#if lesson.teacherId === "__ANY__"}
                  <span class="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-md bg-indigo-50 text-indigo-900 font-bold text-[11px] border border-indigo-200 shadow-2xs">
                    <span>🎲</span>
                    <span>Dowolny nauczyciel</span>
                  </span>
                {:else if assignedTeacher}
                  <button
                    type="button"
                    onclick={() => onNavigateToTeacher(assignedTeacher.id)}
                    class="px-2.5 py-1 bg-slate-100 hover:bg-slate-200 text-slate-800 rounded-lg font-medium border border-slate-200 transition-all cursor-pointer flex items-center gap-1.5 group text-left"
                    title="Przejdź do profilu tego pedagoga"
                  >
                    <span>👨‍🏫</span>
                    <span class="group-hover:underline truncate max-w-[140px]">{assignedTeacher.name}</span>
                    <span class="text-slate-400 group-hover:text-slate-600 text-[10px]">➔</span>
                  </button>
                {:else}
                  <span class="inline-flex items-center gap-1 px-2 py-0.5 rounded bg-slate-100 text-slate-600 text-[11px]">
                    <span>👤</span>
                    <span>Wakat</span>
                  </span>
                {/if}
              </div>

              <!-- Wymiar -->
              <div class="col-span-1 text-center font-bold text-slate-800">
                {lesson.weeklyCount}h
              </div>

              <!-- Akcje: Edycja i Odłącz salę -->
              <div class="col-span-1 text-right flex items-center justify-end gap-1">
                <button
                  type="button"
                  onclick={() => startEditingLesson(lesson)}
                  class="p-1.5 text-slate-500 hover:text-blue-700 hover:bg-blue-50 rounded-lg transition-colors cursor-pointer"
                  title="Edytuj parametry tej lekcji"
                >
                  ✏️
                </button>
                <button
                  type="button"
                  onclick={() => handleUnassignLesson(lesson.id)}
                  class="p-1.5 text-slate-400 hover:text-amber-600 hover:bg-amber-50 rounded-lg transition-colors cursor-pointer"
                  title="Odłącz tę salę od lekcji (pozostaw lekcję w sali dowolnej)"
                >
                  ✕
                </button>
              </div>
            </div>
          {/if}
        {/each}
      </div>
    {/if}
  </div>

  <!-- Section 2: Zintegrowana dostępność sali (Natychmiastowe wykrycie i odblokowanie omyłek!) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900 flex items-center gap-2">
          <span>🗓️</span>
          <span>Dostępność pomieszczenia ({room.name})</span>
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Zablokowane komórki oznaczają godziny, w których sala jest wyłączona z użytku (np. wynajem komercyjny, sprzątanie).
        </p>
      </div>

      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={() => editor.clearEntityAvailability("room", room.id)}
          class="px-2.5 py-1 text-xs bg-slate-100 hover:bg-slate-200 text-slate-700 font-semibold rounded-lg transition-colors cursor-pointer"
        >
          Odblokuj wszystkie godziny
        </button>
        <span class={`text-xs font-bold px-2.5 py-1 rounded-full border ${
          unavailableSet.size > 0 ? 'bg-rose-50 text-rose-700 border-rose-200' : 'bg-emerald-50 text-emerald-700 border-emerald-200'
        }`}>
          {unavailableSet.size > 0 ? `${unavailableSet.size} godz. wyłączonych` : 'W pełni dyspozycyjna'}
        </span>
      </div>
    </div>

    <!-- Integrated Availability Grid -->
    <AvailabilityGrid
      days={editor.currentDaysFormatted}
      periodsPerDay={editor.periodsPerDay}
      {unavailableSet}
      totalSlots={totalSlots}
      onToggleSlot={(dayId, period) => editor.toggleEntitySlot("room", room.id, `${dayId}_${period}`)}
      onToggleWholeDay={(dayId) => editor.toggleEntityDay("room", room.id, dayId)}
      onToggleWholePeriod={(period) => editor.toggleEntityPeriod("room", room.id, period)}
      onToggleAllSlots={handleToggleAllSlots}
    />
  </div>
</div>


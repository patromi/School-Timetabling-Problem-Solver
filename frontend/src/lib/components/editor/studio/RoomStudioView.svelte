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

  const roomTypeLabels: Record<RoomType, { label: string; icon: string }> = {
    general: { label: "Ogólna dydaktyczna", icon: "🚪" },
    gym: { label: "Sportowa / Gimnastyczna", icon: "⚽" },
    lab: { label: "Laboratorium przyrodnicze", icon: "🧪" },
    it: { label: "Pracownia komputerowa", icon: "💻" },
  };

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
          {roomTypeLabels[room.type]?.icon || "🚪"}
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
            <span>Profil: <strong>{roomTypeLabels[room.type]?.label || room.type}</strong></span>
            <span>•</span>
            <span>ID: <code class="text-slate-400 font-mono">{room.id}</code></span>
          </p>
        </div>
      </div>

      <!-- Quick Room Parameters -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="flex items-center gap-2 bg-slate-50 border border-slate-200/90 rounded-xl px-3 py-1.5 text-xs">
          <label for="room-type-select-{room.id}" class="text-slate-500 font-medium">Przeznaczenie:</label>
          <select
            id="room-type-select-{room.id}"
            bind:value={room.type}
            class="font-bold text-slate-800 bg-transparent focus:outline-none cursor-pointer"
          >
            <option value="general">Ogólna</option>
            <option value="gym">Sportowa / Gimnastyczna</option>
            <option value="lab">Laboratorium</option>
            <option value="it">Pracownia IT</option>
          </select>
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
          <div class="col-span-1 text-right">Akcja</div>
        </div>

        {#each roomLessons as lesson}
          {@const assignedClass = editor.classes.find((c) => c.id === lesson.classId)}
          {@const assignedTeacher = editor.teachers.find((t) => t.id === lesson.teacherId)}
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

            <!-- Nauczyciel (Klikalny Cross-Link!) -->
            <div class="col-span-3">
              {#if assignedTeacher}
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
                <span class="text-slate-400 italic">Brak</span>
              {/if}
            </div>

            <!-- Wymiar -->
            <div class="col-span-1 text-center font-bold text-slate-800">
              {lesson.weeklyCount}h
            </div>

            <!-- Odłącz salę -->
            <div class="col-span-1 text-right">
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

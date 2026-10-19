<script lang="ts">
  import type { EventItem, EventResourceItem, ResourceItem, TimeItem } from "../../types";

  interface Props {
    events: EventItem[];
    resources: ResourceItem[];
    times: TimeItem[];
    onChangeEvents: (events: EventItem[]) => void;
  }

  let { events, resources, times, onChangeEvents }: Props = $props();

  // Form state
  let eventId = $state("");
  let eventName = $state("");
  let eventDuration = $state(1);
  let eventCourse = $state("");
  let selectedTeacher = $state("");
  let selectedClass = $state("");
  let selectedRoom = $state("");
  let preassignedTime = $state("");

  // Filters
  let searchQuery = $state("");
  let filterClass = $state("all");

  // Derived resource helpers
  let teachers = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("teach"))
  );
  let classes = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("class"))
  );
  let rooms = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("room"))
  );

  let totalDuration = $derived(
    events.reduce((sum, e) => sum + (e.duration || 1), 0)
  );

  let filteredEvents = $derived.by(() => {
    return events.filter((e) => {
      if (filterClass !== "all") {
        const hasClass = e.resources.some((r) => r.resource_ref === filterClass);
        if (!hasClass) return false;
      }
      if (
        searchQuery &&
        !e.id.toLowerCase().includes(searchQuery.toLowerCase()) &&
        !e.name.toLowerCase().includes(searchQuery.toLowerCase()) &&
        !(e.course_ref && e.course_ref.toLowerCase().includes(searchQuery.toLowerCase()))
      ) {
        return false;
      }
      return true;
    });
  });

  function addEvent() {
    if (!eventId.trim()) return;
    const id = eventId.trim();
    if (events.some((e) => e.id === id)) {
      alert(`Zdarzenie o ID ${id} już istnieje!`);
      return;
    }

    const name = eventName.trim() || id;
    const resList: EventResourceItem[] = [];

    if (selectedTeacher) {
      resList.push({
        role: "Teacher",
        resource_ref: selectedTeacher,
        resource_type_ref: "Teacher",
        workload: eventDuration,
      });
    }
    if (selectedClass) {
      resList.push({
        role: "Class",
        resource_ref: selectedClass,
        resource_type_ref: "Class",
        workload: eventDuration,
      });
    }
    if (selectedRoom) {
      resList.push({
        role: "Room",
        resource_ref: selectedRoom,
        resource_type_ref: "Room",
        workload: eventDuration,
      });
    }

    const newEvent: EventItem = {
      id,
      name,
      duration: Number(eventDuration) || 1,
      course_ref: eventCourse.trim() || null,
      time_ref: preassignedTime.trim() || null,
      workload: Number(eventDuration) || 1,
      resources: resList,
      group_refs: eventCourse.trim() ? [eventCourse.trim()] : [],
    };

    onChangeEvents([...events, newEvent]);

    // Reset form
    eventId = "";
    eventName = "";
    preassignedTime = "";
  }

  function removeEvent(id: string) {
    onChangeEvents(events.filter((e) => e.id !== id));
  }

  function generateSampleCurriculum() {
    if (classes.length === 0 || teachers.length === 0) {
      alert("Najpierw zdefiniuj w zakładce Zasoby przynajmniej jednego nauczyciela i jedną klasę!");
      return;
    }

    const newEvents: EventItem[] = [];
    const subjects = [
      { name: "Matematyka", dur: 2, count: 2 },
      { name: "Język Polski", dur: 2, count: 2 },
      { name: "Język Angielski", dur: 1, count: 3 },
      { name: "Fizyka", dur: 1, count: 2 },
      { name: "Informatyka", dur: 2, count: 1 },
      { name: "Historia", dur: 1, count: 2 },
    ];

    let tIdx = 0;
    let rIdx = 0;

    for (const cls of classes) {
      for (const subj of subjects) {
        const assignedTeacher = teachers[tIdx % teachers.length];
        tIdx++;

        for (let i = 1; i <= subj.count; i++) {
          const eId = `E_${cls.id}_${subj.name.slice(0, 3).toUpperCase()}_${i}`;
          const assignedRoom = rooms.length > 0 ? rooms[rIdx % rooms.length].id : null;
          if (rooms.length > 0) rIdx++;

          const resList: EventResourceItem[] = [
            {
              role: "Teacher",
              resource_ref: assignedTeacher.id,
              resource_type_ref: "Teacher",
              workload: subj.dur,
            },
            {
              role: "Class",
              resource_ref: cls.id,
              resource_type_ref: "Class",
              workload: subj.dur,
            },
          ];

          if (assignedRoom) {
            resList.push({
              role: "Room",
              resource_ref: assignedRoom,
              resource_type_ref: "Room",
              workload: subj.dur,
            });
          }

          newEvents.push({
            id: eId,
            name: `${subj.name} - ${cls.name || cls.id} (część ${i})`,
            duration: subj.dur,
            course_ref: subj.name,
            time_ref: null,
            workload: subj.dur,
            resources: resList,
            group_refs: [subj.name],
          });
        }
      }
    }

    const existingIds = new Set(events.map((e) => e.id));
    const toAdd = newEvents.filter((e) => !existingIds.has(e.id));
    onChangeEvents([...events, ...toAdd]);
  }
</script>

<div class="flex flex-col gap-6 font-mono text-xs text-zinc-200">
  <!-- Top Banner & Quick Generator -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div class="flex items-center gap-4">
      <span class="text-zinc-400">Podsumowanie lekcji:</span>
      <span class="px-2 py-0.5 bg-zinc-950 border border-zinc-800 text-amber-400 font-bold rounded">
        Lekcje: {events.length}
      </span>
      <span class="px-2 py-0.5 bg-zinc-950 border border-zinc-800 text-zinc-300 rounded">
        Łączny czas zajęć: {totalDuration} godzin
      </span>
    </div>

    <button
      type="button"
      onclick={generateSampleCurriculum}
      class="px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-amber-400 font-bold rounded border border-zinc-700 transition-colors cursor-pointer"
      title="Automatycznie generuje siatkę przedmiotów dla klas i nauczycieli"
    >
      ⚡ Generuj zestaw lekcji dla klas
    </button>
  </div>

  <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
    <!-- Form Column -->
    <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <h4 class="font-bold text-white text-sm border-b border-zinc-800 pb-2">
        + Nowa Lekcja / Zajęcia
      </h4>

      <div class="flex flex-col gap-2">
        <div>
          <label for="event-id" class="text-[11px] text-zinc-400">Identyfikator (ID):</label>
          <input
            id="event-id"
            type="text"
            placeholder="np. E_MAT_1A_1"
            bind:value={eventId}
            class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500 font-bold"
          />
        </div>

        <div>
          <label for="event-name" class="text-[11px] text-zinc-400">Nazwa lekcji:</label>
          <input
            id="event-name"
            type="text"
            placeholder="np. Matematyka 1A (blok 2h)"
            bind:value={eventName}
            class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500 font-sans"
          />
        </div>

        <div class="grid grid-cols-2 gap-2">
          <div>
            <label for="event-dur" class="text-[11px] text-zinc-400">Czas trwania (godz.):</label>
            <input
              id="event-dur"
              type="number"
              min="1"
              max="6"
              bind:value={eventDuration}
              class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none text-center font-bold"
            />
          </div>

          <div>
            <label for="event-course" class="text-[11px] text-zinc-400">Kurs / Przedmiot:</label>
            <input
              id="event-course"
              type="text"
              placeholder="np. Matematyka"
              bind:value={eventCourse}
              class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
            />
          </div>
        </div>

        <!-- Role Assignments -->
        <div class="border-t border-zinc-800 pt-2 flex flex-col gap-2">
          <span class="text-[11px] font-bold text-amber-400 uppercase tracking-wider">
            Obsada ról zasobów:
          </span>

          <div>
            <label for="event-teacher" class="text-[11px] text-zinc-400">Nauczyciel (Teacher):</label>
            <select
              id="event-teacher"
              bind:value={selectedTeacher}
              class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
            >
              <option value="">(Wybierz nauczyciela)</option>
              {#each teachers as t}
                <option value={t.id}>{t.name ? `${t.name} (${t.id})` : t.id}</option>
              {/each}
            </select>
          </div>

          <div>
            <label for="event-class" class="text-[11px] text-zinc-400">Klasa / Oddział (Class):</label>
            <select
              id="event-class"
              bind:value={selectedClass}
              class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
            >
              <option value="">(Wybierz klasę)</option>
              {#each classes as c}
                <option value={c.id}>{c.name ? `${c.name} (${c.id})` : c.id}</option>
              {/each}
            </select>
          </div>

          <div>
            <label for="event-room" class="text-[11px] text-zinc-400">Sala (Room):</label>
            <select
              id="event-room"
              bind:value={selectedRoom}
              class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
            >
              <option value="">(Wybierz salę / Dowolna)</option>
              {#each rooms as r}
                <option value={r.id}>{r.name ? `${r.name} (${r.id})` : r.id}</option>
              {/each}
            </select>
          </div>

          <div>
            <label for="event-time" class="text-[11px] text-zinc-400">Prealokowany czas startu (opcjonalnie):</label>
            <select
              id="event-time"
              bind:value={preassignedTime}
              class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
            >
              <option value="">(Brak - czas ustala solver)</option>
              {#each times as t}
                <option value={t.id}>{t.name ? `${t.name} (${t.id})` : t.id}</option>
              {/each}
            </select>
          </div>
        </div>

        <button
          type="button"
          onclick={addEvent}
          class="mt-2 w-full py-2 bg-amber-500 hover:bg-amber-400 text-zinc-950 font-bold rounded transition-colors cursor-pointer"
        >
          + Dodaj lekcję
        </button>
      </div>
    </div>

    <!-- Right: Table of Events (2 cols) -->
    <div class="lg:col-span-2 bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 border-b border-zinc-800 pb-3">
        <h4 class="font-bold text-white">
          Zaplanowane Zajęcia ({filteredEvents.length} / {events.length})
        </h4>

        <div class="flex items-center gap-2 w-full sm:w-auto">
          <!-- Filter by class -->
          <select
            bind:value={filterClass}
            class="px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
          >
            <option value="all">Wszystkie klasy</option>
            {#each classes as c}
              <option value={c.id}>{c.name || c.id}</option>
            {/each}
          </select>

          <!-- Search -->
          <input
            type="text"
            placeholder="Szukaj lekcji..."
            bind:value={searchQuery}
            class="px-2.5 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
          />
        </div>
      </div>

      <div class="max-h-96 overflow-y-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-1 w-12">#</th>
              <th class="pb-1">ID</th>
              <th class="pb-1">Nazwa</th>
              <th class="pb-1 w-14 text-center">Czas</th>
              <th class="pb-1">Obsada ról</th>
              <th class="pb-1 w-20">Start</th>
              <th class="pb-1 text-right w-12">Usuń</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if events.length === 0}
              <tr>
                <td colspan="7" class="py-4 text-center text-zinc-500 italic">
                  Brak lekcji. Dodaj zajęcia formularzem lub użyj przycisku generowania zestawu lekcji.
                </td>
              </tr>
            {:else if filteredEvents.length === 0}
              <tr>
                <td colspan="7" class="py-4 text-center text-zinc-500 italic">
                  Brak lekcji spełniających kryteria filtra.
                </td>
              </tr>
            {:else}
              {#each filteredEvents as e, idx}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2 text-zinc-500">{idx + 1}</td>
                  <td class="py-2 text-amber-400 font-bold">{e.id}</td>
                  <td class="py-2 text-zinc-200 font-sans font-medium">{e.name || e.id}</td>
                  <td class="py-2 text-center">
                    <span class="px-1.5 py-0.5 bg-zinc-950 border border-zinc-800 rounded font-bold">
                      {e.duration}h
                    </span>
                  </td>
                  <td class="py-2">
                    <div class="flex flex-wrap gap-1">
                      {#each e.resources as er}
                        <span class="px-1.5 py-0.2 bg-zinc-950 border border-zinc-800 rounded text-[10px] text-zinc-300">
                          <span class="text-zinc-500">{er.role}:</span> {er.resource_ref}
                        </span>
                      {/each}
                    </div>
                  </td>
                  <td class="py-2">
                    {#if e.time_ref}
                      <span class="px-1 py-0.2 bg-emerald-950 text-emerald-300 border border-emerald-800 rounded text-[10px]">
                        {e.time_ref}
                      </span>
                    {:else}
                      <span class="text-zinc-600 text-[10px]">Auto</span>
                    {/if}
                  </td>
                  <td class="py-2 text-right">
                    <button
                      type="button"
                      onclick={() => removeEvent(e.id)}
                      class="text-zinc-500 hover:text-rose-400 cursor-pointer px-1 text-sm"
                      title="Usuń lekcję"
                    >
                      ✕
                    </button>
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    </div>
  </div>
</div>

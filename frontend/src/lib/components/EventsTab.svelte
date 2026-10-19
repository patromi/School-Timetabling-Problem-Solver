<script lang="ts">
  import type { EventItem, InstanceDetail } from "../types";
  import { getResourceCategory } from "../school_helpers";

  interface Props {
    instance: InstanceDetail;
    selectedResourceFilter?: string;
  }

  let { instance, selectedResourceFilter = "" }: Props = $props();

  let searchQuery = $state("");
  let resourceFilter = $state("");
  let durationFilter = $state<string>("all");

  // Keep in sync if parent updates resource filter
  $effect(() => {
    if (selectedResourceFilter !== undefined) {
      resourceFilter = selectedResourceFilter;
    }
  });

  // Resource map for fast lookup of names and types
  let resourceMap = $derived.by(() => {
    const map = new Map<string, { name: string; category: string }>();
    for (const r of instance.resources) {
      map.set(r.id, {
        name: r.name || r.id,
        category: getResourceCategory(r, instance),
      });
    }
    return map;
  });

  let filteredEvents = $derived.by(() => {
    return instance.events.filter((e) => {
      const matchesSearch =
        e.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        e.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        (e.course_ref && e.course_ref.toLowerCase().includes(searchQuery.toLowerCase()));
      if (!matchesSearch) return false;

      if (durationFilter === "single" && e.duration !== 1) return false;
      if (durationFilter === "block" && e.duration <= 1) return false;

      if (resourceFilter) {
        const hasResource = e.resources.some((er) => er.resource_ref === resourceFilter);
        if (!hasResource) return false;
      }

      return true;
    });
  });

  function getResourcesForEvent(event: EventItem) {
    const rMap = resourceMap;
    let teacher: string | null = null;
    let classroom: string | null = null;
    let studentClass: string | null = null;
    const others: string[] = [];

    for (const er of event.resources) {
      if (!er.resource_ref) continue;
      const meta = rMap.get(er.resource_ref);
      const label = meta ? meta.name : er.resource_ref;
      const cat = meta ? meta.category : "other";

      if (cat === "teacher" && !teacher) teacher = label;
      else if (cat === "class" && !studentClass) studentClass = label;
      else if (cat === "room" && !classroom) classroom = label;
      else others.push(label);
    }

    return { teacher, studentClass, classroom, others };
  }
</script>

<div class="p-6 max-w-6xl mx-auto flex flex-col gap-6 text-slate-800">
  <!-- Controls Bar -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-4">
    <div class="flex flex-wrap items-center gap-3">
      <!-- Search Input -->
      <div class="relative w-full sm:w-64">
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj przedmiotu lub lekcji..."
          class="w-full pl-8 pr-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all"
        />
        <span class="absolute left-2.5 top-2 text-slate-400 text-xs">🔍</span>
      </div>

      <!-- Resource Filter -->
      <select
        bind:value={resourceFilter}
        class="px-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all cursor-pointer"
      >
        <option value="">Wszyscy nauczyciele i klasy</option>
        {#each instance.resources as r}
          <option value={r.id}>
            {r.name || r.id} ({r.id})
          </option>
        {/each}
      </select>

      <!-- Duration Filter -->
      <select
        bind:value={durationFilter}
        class="px-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all cursor-pointer"
      >
        <option value="all">Każda długość zajęć</option>
        <option value="single">Tylko lekcje pojedyncze (1 godz.)</option>
        <option value="block">Tylko bloki (2+ godz.)</option>
      </select>
    </div>

    <div class="text-xs text-slate-500 font-medium shrink-0">
      Pokazano <strong>{filteredEvents.length}</strong> z {instance.events.length} pozycji
    </div>
  </div>

  <!-- Events List -->
  {#if filteredEvents.length === 0}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-12 text-center text-slate-400 text-xs shadow-xs">
      Nie znaleziono żadnych pozycji zajęć spełniających kryteria.
    </div>
  {:else}
    <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden divide-y divide-slate-100">
      {#each filteredEvents as event (event.id)}
        {@const res = getResourcesForEvent(event)}
        <div class="p-4 hover:bg-slate-50/80 transition-all flex flex-col md:flex-row md:items-center justify-between gap-4">
          <!-- Left: Event Name & Course -->
          <div class="flex items-start gap-3.5">
            <div class="w-10 h-10 rounded-xl bg-blue-50 text-blue-700 border border-blue-100 flex items-center justify-center font-bold text-sm shrink-0 shadow-2xs">
              📚
            </div>
            <div>
              <div class="flex items-center gap-2">
                <h4 class="text-sm font-bold text-slate-900 leading-tight">
                  {event.name || event.course_ref || event.id}
                </h4>
                <span class="text-[10px] px-2 py-0.5 rounded-full font-medium
                  {event.duration > 1 ? 'bg-amber-100 text-amber-800' : 'bg-slate-100 text-slate-600'}">
                  {event.duration} {event.duration === 1 ? 'godzina' : 'godziny (blok)'}
                </span>
              </div>
              <p class="text-xs text-slate-400 font-mono mt-0.5">
                Identyfikator: {event.id}
              </p>
            </div>
          </div>

          <!-- Middle: Assigned Participants (Human friendly pills) -->
          <div class="flex flex-wrap items-center gap-2 text-xs">
            {#if res.studentClass}
              <span class="px-2.5 py-1 bg-emerald-50 text-emerald-800 border border-emerald-200/60 rounded-lg flex items-center gap-1 font-medium">
                <span>👥</span>
                <span>Klasa: <strong>{res.studentClass}</strong></span>
              </span>
            {/if}

            {#if res.teacher}
              <span class="px-2.5 py-1 bg-indigo-50 text-indigo-800 border border-indigo-200/60 rounded-lg flex items-center gap-1 font-medium">
                <span>👨‍🏫</span>
                <span>Nauczyciel: <strong>{res.teacher}</strong></span>
              </span>
            {/if}

            {#if res.classroom}
              <span class="px-2.5 py-1 bg-amber-50 text-amber-800 border border-amber-200/60 rounded-lg flex items-center gap-1 font-medium">
                <span>🚪</span>
                <span>Sala: <strong>{res.classroom}</strong></span>
              </span>
            {/if}

            {#each res.others as other}
              <span class="px-2.5 py-1 bg-slate-100 text-slate-700 border border-slate-200/60 rounded-lg font-medium">
                {other}
              </span>
            {/each}
          </div>

          <!-- Right: Preassigned status -->
          <div class="text-xs text-right shrink-0">
            {#if event.time_ref}
              <span class="text-emerald-700 font-semibold bg-emerald-50 px-2 py-1 rounded-md border border-emerald-200/60">
                Termin stały: {event.time_ref}
              </span>
            {:else}
              <span class="text-slate-400 text-[11px]">
                Termin wyznacza solver
              </span>
            {/if}
          </div>
        </div>
      {/each}
    </div>
  {/if}
</div>

<script lang="ts">
  import type { InstanceDetail } from "../types";
  import { getResourceCategory } from "../school_helpers";

  interface Props {
    instance: InstanceDetail;
    onSelectResourceForEvents?: (resourceId: string) => void;
  }

  let { instance, onSelectResourceForEvents }: Props = $props();

  let searchQuery = $state("");
  let activeCategory = $state<"all" | "teacher" | "class" | "room">("all");

  // Precompute workload
  let resourceUsage = $derived.by(() => {
    const usage: Record<string, { eventCount: number; totalHours: number }> = {};
    for (const r of instance.resources) {
      usage[r.id] = { eventCount: 0, totalHours: 0 };
    }
    for (const e of instance.events) {
      for (const er of e.resources) {
        if (er.resource_ref && usage[er.resource_ref]) {
          usage[er.resource_ref].eventCount += 1;
          usage[er.resource_ref].totalHours += e.duration || 1;
        }
      }
    }
    return usage;
  });

  let teachersCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "teacher").length
  );
  let classesCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "class").length
  );
  let roomsCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "room").length
  );

  let filteredResources = $derived.by(() => {
    const usage = resourceUsage;
    return instance.resources
      .filter((r) => {
        const cat = getResourceCategory(r, instance);
        if (activeCategory !== "all" && cat !== activeCategory) {
          return false;
        }
        const matchesSearch =
          r.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
          r.name.toLowerCase().includes(searchQuery.toLowerCase());
        return matchesSearch;
      })
      .map((r) => ({
        ...r,
        category: getResourceCategory(r, instance),
        eventCount: usage[r.id]?.eventCount || 0,
        totalHours: usage[r.id]?.totalHours || 0,
      }));
  });
</script>

<div class="p-6 max-w-6xl mx-auto flex flex-col gap-6 text-slate-800">
  <!-- Controls Card -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-4">
    <!-- Category Tabs -->
    <div class="flex items-center gap-1 bg-slate-100 p-1 rounded-xl text-xs font-semibold text-slate-600">
      <button
        onclick={() => (activeCategory = "all")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
          activeCategory === "all" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        Wszystkie ({instance.resources.length})
      </button>

      <button
        onclick={() => (activeCategory = "teacher")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
          activeCategory === "teacher" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        <span>👨‍🏫 Nauczyciele</span>
        <span class="text-[10px] bg-slate-200/70 px-1.5 py-0.2 rounded-full">{teachersCount}</span>
      </button>

      <button
        onclick={() => (activeCategory = "class")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
          activeCategory === "class" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        <span>👥 Klasy</span>
        <span class="text-[10px] bg-slate-200/70 px-1.5 py-0.2 rounded-full">{classesCount}</span>
      </button>

      <button
        onclick={() => (activeCategory = "room")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
          activeCategory === "room" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        <span>🚪 Sale</span>
        <span class="text-[10px] bg-slate-200/70 px-1.5 py-0.2 rounded-full">{roomsCount}</span>
      </button>
    </div>

    <!-- Search Input -->
    <div class="relative w-full sm:w-72">
      <input
        type="text"
        bind:value={searchQuery}
        placeholder="Szukaj po nazwie lub symbolu..."
        class="w-full pl-8 pr-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all"
      />
      <span class="absolute left-2.5 top-2 text-slate-400 text-xs">🔍</span>
    </div>
  </div>

  <!-- Resource Cards Grid -->
  {#if filteredResources.length === 0}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-12 text-center text-slate-400 text-xs shadow-xs">
      Nie znaleziono żadnych zasobów pasujących do wyszukiwania.
    </div>
  {:else}
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
      {#each filteredResources as res (res.id)}
        <div class="bg-white border border-slate-200/90 rounded-2xl p-4 shadow-xs flex flex-col justify-between hover:border-slate-300 transition-all">
          <div>
            <!-- Card Header -->
            <div class="flex items-start justify-between gap-2">
              <div class="flex items-center gap-2.5">
                <div class="w-9 h-9 rounded-xl flex items-center justify-center text-base font-bold shadow-2xs shrink-0
                  {res.category === 'teacher' ? 'bg-indigo-50 text-indigo-700 border border-indigo-100' :
                   res.category === 'class' ? 'bg-emerald-50 text-emerald-700 border border-emerald-100' :
                   res.category === 'room' ? 'bg-amber-50 text-amber-700 border border-amber-100' :
                   'bg-slate-50 text-slate-700 border border-slate-200'}">
                  {#if res.category === 'teacher'}
                    👨‍🏫
                  {:else if res.category === 'class'}
                    👥
                  {:else if res.category === 'room'}
                    🚪
                  {:else}
                    📦
                  {/if}
                </div>
                <div>
                  <h4 class="text-sm font-bold text-slate-900 leading-tight">
                    {res.name || res.id}
                  </h4>
                  <span class="text-[10px] text-slate-400 font-mono">
                    ID: {res.id}
                  </span>
                </div>
              </div>

              <!-- Category Badge -->
              <span class="text-[10px] font-medium px-2 py-0.5 rounded-full
                {res.category === 'teacher' ? 'bg-indigo-50 text-indigo-700' :
                 res.category === 'class' ? 'bg-emerald-50 text-emerald-700' :
                 res.category === 'room' ? 'bg-amber-50 text-amber-700' :
                 'bg-slate-100 text-slate-600'}">
                {res.category === 'teacher' ? 'Nauczyciel' :
                 res.category === 'class' ? 'Oddział' :
                 res.category === 'room' ? 'Sala' :
                 res.resource_type_ref}
              </span>
            </div>

            <!-- Stats -->
            <div class="mt-4 pt-3 border-t border-slate-100 grid grid-cols-2 gap-2 text-xs">
              <div class="flex flex-col">
                <span class="text-[10px] text-slate-400">Tygodniowo</span>
                <span class="font-bold text-slate-900 text-sm">
                  {res.totalHours} godz.
                </span>
              </div>
              <div class="flex flex-col">
                <span class="text-[10px] text-slate-400">Liczba pozycji</span>
                <span class="font-bold text-slate-900 text-sm">
                  {res.eventCount} zajęć
                </span>
              </div>
            </div>
          </div>

          <!-- Footer Action -->
          {#if onSelectResourceForEvents}
            <div class="mt-4 pt-3 border-t border-slate-100 flex justify-end">
              <button
                onclick={() => onSelectResourceForEvents(res.id)}
                class="text-xs font-semibold text-blue-600 hover:text-blue-700 cursor-pointer flex items-center gap-1"
              >
                <span>Pokaż lekcje w planie</span>
                <span>→</span>
              </button>
            </div>
          {/if}
        </div>
      {/each}
    </div>
  {/if}
</div>

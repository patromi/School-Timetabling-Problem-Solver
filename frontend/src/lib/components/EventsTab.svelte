<script lang="ts">
  import type { EventItem, InstanceDetail } from "../types";

  interface Props {
    instance: InstanceDetail;
    selectedResourceFilter?: string;
  }

  let { instance, selectedResourceFilter = "" }: Props = $props();

  let searchQuery = $state("");
  let resourceFilter = $state("");
  let durationFilter = $state<string>("all");
  let preassignedFilter = $state<string>("all");

  // Keep in sync if parent updates resource filter
  $effect(() => {
    if (selectedResourceFilter !== undefined) {
      resourceFilter = selectedResourceFilter;
    }
  });

  // Resource map for fast lookup of names
  let resourceMap = $derived.by(() => {
    const map = new Map<string, string>();
    for (const r of instance.resources) {
      map.set(r.id, r.name || r.id);
    }
    return map;
  });

  let filteredEvents = $derived.by(() => {
    const rMap = resourceMap;
    return instance.events.filter((e) => {
      // Search text
      const matchesSearch =
        e.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        e.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        (e.course_ref && e.course_ref.toLowerCase().includes(searchQuery.toLowerCase()));
      if (!matchesSearch) return false;

      // Duration filter
      if (durationFilter !== "all" && e.duration !== Number(durationFilter)) {
        return false;
      }

      // Preassigned time filter
      if (preassignedFilter === "preassigned" && e.time_ref === null) {
        return false;
      }
      if (preassignedFilter === "unassigned" && e.time_ref !== null) {
        return false;
      }

      // Resource filter
      if (resourceFilter) {
        const hasResource = e.resources.some((er) => er.resource_ref === resourceFilter);
        if (!hasResource) return false;
      }

      return true;
    });
  });
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-4">
    <!-- Header & Filters -->
    <div class="flex flex-col md:flex-row items-start md:items-center justify-between gap-3">
      <div>
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Lekcje i Zajęcia (Events)
        </h3>
        <p class="text-xs text-zinc-500 font-mono mt-0.5">
          Wyświetlanie {filteredEvents.length} z {instance.events.length} lekcji
        </p>
      </div>

      <!-- Filters -->
      <div class="flex flex-wrap items-center gap-2 w-full md:w-auto">
        <!-- Resource filter -->
        <select
          bind:value={resourceFilter}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500 max-w-xs"
        >
          <option value="">Wszystkie zasoby</option>
          {#each instance.resources as r}
            <option value={r.id}>
              {r.resource_type_ref}: {r.name ? `${r.name} (${r.id})` : r.id}
            </option>
          {/each}
        </select>

        <!-- Duration filter -->
        <select
          bind:value={durationFilter}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
        >
          <option value="all">Wszystkie długości</option>
          <option value="1">1 godzina</option>
          <option value="2">2 godziny</option>
          <option value="3">3 godziny</option>
          <option value="4">4 godziny</option>
        </select>

        <!-- Time preassignment filter -->
        <select
          bind:value={preassignedFilter}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
        >
          <option value="all">Wszystkie czasy</option>
          <option value="preassigned">Tylko prealokowane</option>
          <option value="unassigned">Tylko do wyznaczenia</option>
        </select>

        <!-- Search Input -->
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj lekcji (ID, nazwa, kurs)..."
          class="px-2.5 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 font-mono focus:outline-none focus:border-zinc-500"
        />

        {#if resourceFilter}
          <button
            onclick={() => (resourceFilter = "")}
            class="px-2 py-1 bg-zinc-800 hover:bg-zinc-700 text-amber-400 text-xs rounded border border-zinc-700 font-mono cursor-pointer"
          >
            ✕ Reset filtra zasobu
          </button>
        {/if}
      </div>
    </div>

    <!-- Table of Events -->
    <div class="overflow-x-auto">
      <table class="w-full text-left text-xs font-mono">
        <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
          <tr>
            <th class="pb-2 w-20">ID</th>
            <th class="pb-2">Nazwa zajęć</th>
            <th class="pb-2 w-20 text-center">Długość</th>
            <th class="pb-2">Kurs / Przedmiot</th>
            <th class="pb-2">Obsada ról i zasobów (Resources)</th>
            <th class="pb-2 w-24">Czas prealokowany</th>
            <th class="pb-2">Grupy zdarzeń</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-zinc-800/60">
          {#each filteredEvents as e}
            <tr class="hover:bg-zinc-800/40">
              <td class="py-2.5 font-bold text-amber-400">{e.id}</td>
              <td class="py-2.5 text-zinc-200 font-sans font-medium">{e.name || e.id}</td>
              <td class="py-2.5 text-center">
                <span class="px-2 py-0.5 bg-zinc-800 text-zinc-200 rounded border border-zinc-700 font-bold">
                  {e.duration} h
                </span>
              </td>
              <td class="py-2.5">
                {#if e.course_ref}
                  <span class="text-indigo-300 font-medium">{e.course_ref}</span>
                {:else}
                  <span class="text-zinc-600">—</span>
                {/if}
              </td>
              <td class="py-2.5">
                <div class="flex flex-wrap gap-1.5">
                  {#each e.resources as er}
                    {#if er.resource_ref}
                      <span class="px-1.5 py-0.5 bg-zinc-950 text-zinc-200 rounded border border-zinc-800 text-[11px] flex items-center gap-1">
                        <span class="text-zinc-500 text-[10px]">{er.role}:</span>
                        <strong class="text-amber-300">
                          {resourceMap.get(er.resource_ref) || er.resource_ref}
                        </strong>
                      </span>
                    {:else}
                      <span class="px-1.5 py-0.5 bg-amber-950/40 text-amber-400 rounded border border-amber-800/60 text-[11px] flex items-center gap-1">
                        <span class="text-amber-500 text-[10px]">{er.role}:</span>
                        <span>[Dowolny {er.resource_type_ref}]</span>
                      </span>
                    {/if}
                  {/each}
                </div>
              </td>
              <td class="py-2.5">
                {#if e.time_ref}
                  <span class="px-1.5 py-0.5 bg-zinc-800 text-emerald-400 rounded border border-emerald-900 font-semibold text-[10px]">
                    {e.time_ref}
                  </span>
                {:else}
                  <span class="text-zinc-600 text-[10px]">Do ustalenia</span>
                {/if}
              </td>
              <td class="py-2.5">
                <div class="flex flex-wrap gap-1">
                  {#if e.group_refs.length === 0}
                    <span class="text-zinc-600">—</span>
                  {:else}
                    {#each e.group_refs as gr}
                      <span class="px-1 py-0.2 bg-zinc-950 text-zinc-400 rounded border border-zinc-800 text-[10px]">
                        {gr}
                      </span>
                    {/each}
                  {/if}
                </div>
              </td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

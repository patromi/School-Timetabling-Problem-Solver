<script lang="ts">
  import type { InstanceDetail, ResourceItem } from "../types";

  interface Props {
    instance: InstanceDetail;
    onSelectResourceForEvents?: (resourceId: string) => void;
  }

  let { instance, onSelectResourceForEvents }: Props = $props();

  let searchQuery = $state("");
  let selectedType = $state<string>("all");
  let selectedGroup = $state<string>("all");

  // Precompute lesson count and hours per resource
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

  let filteredResources = $derived.by(() => {
    const usage = resourceUsage;
    return instance.resources
      .filter((r) => {
        const matchesSearch =
          r.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
          r.name.toLowerCase().includes(searchQuery.toLowerCase());
        if (!matchesSearch) return false;
        if (selectedType !== "all" && r.resource_type_ref !== selectedType) {
          return false;
        }
        if (selectedGroup !== "all" && !r.group_refs.includes(selectedGroup)) {
          return false;
        }
        return true;
      })
      .map((r) => ({
        ...r,
        eventCount: usage[r.id]?.eventCount || 0,
        totalHours: usage[r.id]?.totalHours || 0,
      }));
  });

  function getResourceTypeBadgeClass(type: string): string {
    const t = type.toLowerCase();
    if (t.includes("teach") || t.includes("naucz") || t.includes("prof")) {
      return "bg-indigo-950 text-indigo-300 border-indigo-800";
    }
    if (t.includes("class") || t.includes("stud") || t.includes("oddz") || t.includes("klas")) {
      return "bg-emerald-950 text-emerald-300 border-emerald-800";
    }
    if (t.includes("room") || t.includes("sal") || t.includes("pok")) {
      return "bg-amber-950 text-amber-300 border-amber-800";
    }
    return "bg-zinc-800 text-zinc-300 border-zinc-700";
  }
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-4">
    <!-- Header with Filters -->
    <div class="flex flex-col md:flex-row items-start md:items-center justify-between gap-3">
      <div>
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Zasoby Instancji (Nauczyciele, Klasy, Sale)
        </h3>
        <p class="text-xs text-zinc-500 font-mono mt-0.5">
          Znaleziono {filteredResources.length} z {instance.resources.length} zasobów
        </p>
      </div>

      <!-- Controls -->
      <div class="flex flex-wrap items-center gap-2 w-full md:w-auto">
        <!-- Resource Type Filter -->
        <select
          bind:value={selectedType}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
        >
          <option value="all">Wszystkie typy ({instance.resources.length})</option>
          {#each instance.resource_types as rt}
            <option value={rt.id}>{rt.name || rt.id}</option>
          {/each}
        </select>

        <!-- Resource Group Filter -->
        {#if instance.resource_groups.length > 0}
          <select
            bind:value={selectedGroup}
            class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
          >
            <option value="all">Wszystkie grupy ({instance.resource_groups.length})</option>
            {#each instance.resource_groups as rg}
              <option value={rg.id}>{rg.name || rg.id}</option>
            {/each}
          </select>
        {/if}

        <!-- Search Input -->
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj zasobu (ID lub nazwa)..."
          class="px-2.5 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 font-mono focus:outline-none focus:border-zinc-500"
        />
      </div>
    </div>

    <!-- Table of Resources -->
    <div class="overflow-x-auto">
      <table class="w-full text-left text-xs font-mono">
        <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
          <tr>
            <th class="pb-2">Identyfikator</th>
            <th class="pb-2">Nazwa / Imię</th>
            <th class="pb-2">Typ zasobu</th>
            <th class="pb-2">Grupy zasobów (ResourceGroups)</th>
            <th class="pb-2 text-right">Liczba zajęć</th>
            <th class="pb-2 text-right">Łączny czas</th>
            <th class="pb-2 text-center w-28">Akcja</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-zinc-800/60">
          {#each filteredResources as r}
            <tr class="hover:bg-zinc-800/40">
              <td class="py-2 font-bold text-amber-400">{r.id}</td>
              <td class="py-2 text-zinc-200 font-sans font-medium">{r.name || "—"}</td>
              <td class="py-2">
                <span class={`px-2 py-0.5 rounded text-[10px] font-semibold border ${getResourceTypeBadgeClass(r.resource_type_ref)}`}>
                  {r.resource_type_ref}
                </span>
              </td>
              <td class="py-2">
                <div class="flex flex-wrap gap-1">
                  {#if r.group_refs.length === 0}
                    <span class="text-zinc-600">—</span>
                  {:else}
                    {#each r.group_refs as gr}
                      <span class="px-1.5 py-0.2 bg-zinc-950 text-zinc-300 rounded border border-zinc-800 text-[10px]">
                        {gr}
                      </span>
                    {/each}
                  {/if}
                </div>
              </td>
              <td class="py-2 text-right font-bold text-zinc-100">{r.eventCount}</td>
              <td class="py-2 text-right text-zinc-400">{r.totalHours} h</td>
              <td class="py-2 text-center">
                {#if onSelectResourceForEvents}
                  <button
                    onclick={() => onSelectResourceForEvents?.(r.id)}
                    class="px-2 py-0.5 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white rounded text-[11px] font-mono transition-colors cursor-pointer border border-zinc-700"
                    title="Przejdź do zajęć tego zasobu"
                  >
                    Lekcje →
                  </button>
                {/if}
              </td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

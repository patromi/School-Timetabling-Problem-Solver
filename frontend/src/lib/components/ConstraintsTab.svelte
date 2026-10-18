<script lang="ts">
  import type { ConstraintItem, InstanceDetail } from "../types";

  interface Props {
    instance: InstanceDetail;
  }

  let { instance }: Props = $props();

  let searchQuery = $state("");
  let requiredFilter = $state<"all" | "hard" | "soft">("all");
  let typeFilter = $state<string>("all");
  let expandedRows = $state<Record<string, boolean>>({});

  function toggleRow(id: string) {
    expandedRows = { ...expandedRows, [id]: !expandedRows[id] };
  }

  // Count constraints per type for dropdown
  let typeCounts = $derived.by(() => {
    const counts: Record<string, number> = {};
    for (const c of instance.constraints) {
      counts[c.type] = (counts[c.type] || 0) + 1;
    }
    return Object.entries(counts).sort((a, b) => b[1] - a[1]);
  });

  let filteredConstraints = $derived.by(() => {
    return instance.constraints.filter((c) => {
      // Search
      const matchesSearch =
        c.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        c.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        c.type.toLowerCase().includes(searchQuery.toLowerCase());
      if (!matchesSearch) return false;

      // Required filter
      if (requiredFilter === "hard" && !c.required) return false;
      if (requiredFilter === "soft" && c.required) return false;

      // Type filter
      if (typeFilter !== "all" && c.type !== typeFilter) return false;

      return true;
    });
  });

  function getScopeCount(c: ConstraintItem): number {
    return (
      c.applies_to.events.length +
      c.applies_to.event_groups.length +
      c.applies_to.resources.length +
      c.applies_to.resource_groups.length +
      c.applies_to.times.length +
      c.applies_to.time_groups.length +
      c.applies_to.event_pairs.length
    );
  }

  function formatParams(params: Record<string, unknown>): string {
    if (!params || Object.keys(params).length === 0) return "brak";
    return Object.entries(params)
      .map(([k, v]) => {
        if (Array.isArray(v)) {
          return `${k}: [${v.length} el.]`;
        }
        return `${k}: ${v}`;
      })
      .join(" · ");
  }
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-4">
    <!-- Filters Header -->
    <div class="flex flex-col md:flex-row items-start md:items-center justify-between gap-3">
      <div>
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Ograniczenia i Reguły (Constraints)
        </h3>
        <p class="text-xs text-zinc-500 font-mono mt-0.5">
          Wyświetlanie {filteredConstraints.length} z {instance.constraints.length} reguł
        </p>
      </div>

      <!-- Controls -->
      <div class="flex flex-wrap items-center gap-2 w-full md:w-auto">
        <!-- Required Filter -->
        <div class="flex items-center rounded border border-zinc-800 bg-zinc-950 p-0.5 text-xs font-mono">
          <button
            onclick={() => (requiredFilter = "all")}
            class={`px-2 py-1 rounded transition-colors ${
              requiredFilter === "all" ? "bg-zinc-800 text-white font-medium" : "text-zinc-400 hover:text-zinc-200"
            }`}
          >
            Wszystkie
          </button>
          <button
            onclick={() => (requiredFilter = "hard")}
            class={`px-2 py-1 rounded transition-colors ${
              requiredFilter === "hard" ? "bg-rose-950 text-rose-300 font-medium" : "text-zinc-400 hover:text-zinc-200"
            }`}
          >
            Twarde (Hard)
          </button>
          <button
            onclick={() => (requiredFilter = "soft")}
            class={`px-2 py-1 rounded transition-colors ${
              requiredFilter === "soft" ? "bg-amber-950 text-amber-300 font-medium" : "text-zinc-400 hover:text-zinc-200"
            }`}
          >
            Miękkie (Soft)
          </button>
        </div>

        <!-- Constraint Type Filter -->
        <select
          bind:value={typeFilter}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
        >
          <option value="all">Wszystkie typy reguł ({instance.constraints.length})</option>
          {#each typeCounts as [type, count]}
            <option value={type}>{type} ({count})</option>
          {/each}
        </select>

        <!-- Search Input -->
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj reguły (ID, nazwa, typ)..."
          class="px-2.5 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 font-mono focus:outline-none focus:border-zinc-500"
        />
      </div>
    </div>

    <!-- Table of Constraints with Expandable Details -->
    <div class="overflow-x-auto">
      <table class="w-full text-left text-xs font-mono">
        <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
          <tr>
            <th class="pb-2 w-8"></th>
            <th class="pb-2 w-28">Status</th>
            <th class="pb-2">Typ ograniczenia</th>
            <th class="pb-2">Identyfikator / Nazwa</th>
            <th class="pb-2 text-right w-16">Waga</th>
            <th class="pb-2 text-center w-24">Funkcja</th>
            <th class="pb-2">Parametry & Zakres (AppliesTo)</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-zinc-800/60">
          {#each filteredConstraints as c (c.id)}
            <tr
              onclick={() => toggleRow(c.id)}
              class="hover:bg-zinc-800/40 cursor-pointer transition-colors"
            >
              <td class="py-2.5 text-center text-zinc-500">
                <span>{expandedRows[c.id] ? "▼" : "▶"}</span>
              </td>
              <td class="py-2.5">
                {#if c.required}
                  <span class="px-2 py-0.5 rounded text-[10px] font-bold uppercase bg-rose-950/80 text-rose-300 border border-rose-800">
                    HARD (Wymóg)
                  </span>
                {:else}
                  <span class="px-2 py-0.5 rounded text-[10px] font-bold uppercase bg-amber-950/80 text-amber-300 border border-amber-800">
                    SOFT (Preferencja)
                  </span>
                {/if}
              </td>
              <td class="py-2.5 font-bold text-zinc-100">{c.type}</td>
              <td class="py-2.5">
                <div class="flex flex-col">
                  <span class="font-bold text-amber-400">{c.id}</span>
                  {#if c.name && c.name !== c.id}
                    <span class="text-zinc-400 text-[11px] font-sans truncate max-w-xs">{c.name}</span>
                  {/if}
                </div>
              </td>
              <td class="py-2.5 text-right font-bold text-zinc-200">{c.weight}</td>
              <td class="py-2.5 text-center">
                <span class="px-1.5 py-0.5 bg-zinc-950 rounded border border-zinc-800 text-[10px] text-zinc-400">
                  {c.cost_function}
                </span>
              </td>
              <td class="py-2.5">
                <div class="flex items-center gap-2">
                  <span class="text-zinc-400 text-[11px] truncate max-w-sm">
                    {formatParams(c.params)}
                  </span>
                  <span class="px-1.5 py-0.2 bg-zinc-950 text-zinc-400 rounded border border-zinc-800 text-[10px] shrink-0">
                    Zakres: {getScopeCount(c)} el.
                  </span>
                </div>
              </td>
            </tr>

            <!-- Expanded Row Details -->
            {#if expandedRows[c.id]}
              <tr class="bg-zinc-950/80 border-b border-zinc-800">
                <td colspan="7" class="p-4">
                  <div class="grid grid-cols-1 md:grid-cols-2 gap-4 text-xs font-mono">
                    <!-- Scope (AppliesTo) Details -->
                    <div class="bg-zinc-900 p-3 rounded border border-zinc-800 flex flex-col gap-2">
                      <span class="text-[11px] font-bold text-zinc-400 uppercase tracking-wider">
                        Zakres działania (AppliesTo):
                      </span>
                      <div class="flex flex-col gap-1.5 text-[11px]">
                        {#if c.applies_to.resources.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Zasoby ({c.applies_to.resources.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.resources.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.resource_groups.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Grupy zasobów ({c.applies_to.resource_groups.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.resource_groups.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.events.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Zdarzenia ({c.applies_to.events.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.events.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.event_groups.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Grupy zdarzeń ({c.applies_to.event_groups.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.event_groups.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.times.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Sloty czasowe ({c.applies_to.times.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.times.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.time_groups.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Grupy czasowe ({c.applies_to.time_groups.length}):</span>
                            <span class="text-zinc-300 ml-1 break-all">{c.applies_to.time_groups.join(", ")}</span>
                          </div>
                        {/if}
                        {#if c.applies_to.event_pairs.length > 0}
                          <div>
                            <span class="text-zinc-500 font-semibold">Pary zdarzeń ({c.applies_to.event_pairs.length}):</span>
                            <div class="mt-1 flex flex-col gap-0.5">
                              {#each c.applies_to.event_pairs as pair}
                                <span class="text-zinc-300">
                                  {pair.first_event} → {pair.second_event} (min: {pair.min_separation}, max: {pair.max_separation ?? "∞"})
                                </span>
                              {/each}
                            </div>
                          </div>
                        {/if}
                        {#if getScopeCount(c) === 0}
                          <span class="text-zinc-500 italic">Brak wyszczególnionych celów (reguła globalna lub domyślna)</span>
                        {/if}
                      </div>
                    </div>

                    <!-- Raw Parameters JSON -->
                    <div class="bg-zinc-900 p-3 rounded border border-zinc-800 flex flex-col gap-2">
                      <span class="text-[11px] font-bold text-zinc-400 uppercase tracking-wider">
                        Parametry specyficzne (params):
                      </span>
                      <pre class="text-[11px] bg-zinc-950 p-2 rounded border border-zinc-800 text-amber-300/90 overflow-x-auto whitespace-pre-wrap">{JSON.stringify(c.params, null, 2)}</pre>
                    </div>
                  </div>
                </td>
              </tr>
            {/if}
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

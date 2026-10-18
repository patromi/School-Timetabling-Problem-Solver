<script lang="ts">
  import type { InstanceDetail, TimeItem } from "../types";

  interface Props {
    instance: InstanceDetail;
  }

  let { instance }: Props = $props();

  let searchQuery = $state("");
  let selectedGroup = $state<string>("all");

  // Detect day groups (either kind == 'Day' or all time_groups if kind isn't specified)
  let dayGroups = $derived.by(() => {
    const days = instance.time_groups.filter(
      (g) => g.kind.toLowerCase() === "day"
    );
    return days.length > 0 ? days : instance.time_groups;
  });

  // Build matrix of days -> periods
  let daysMatrix = $derived.by(() => {
    const groups = dayGroups;
    if (groups.length === 0) {
      return [{ group: { id: "all", name: "Wszystkie godziny", kind: "Day" }, times: instance.times }];
    }
    return groups.map((g) => ({
      group: g,
      times: instance.times.filter((t) => t.group_refs.includes(g.id)),
    }));
  });

  // Filtered times list for the table
  let filteredTimes = $derived.by(() => {
    return instance.times
      .map((t, idx) => ({ ...t, index: idx }))
      .filter((t) => {
        const matchesSearch =
          t.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
          t.name.toLowerCase().includes(searchQuery.toLowerCase());
        if (!matchesSearch) return false;
        if (selectedGroup !== "all" && !t.group_refs.includes(selectedGroup)) {
          return false;
        }
        return true;
      });
  });
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <!-- Visual Day Columns Grid -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
    <div class="flex items-center justify-between">
      <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
        Siatka Dni i Okresów Lekcyjnych
      </h3>
      <span class="text-xs font-mono text-zinc-500">
        Łącznie {instance.times.length} slotów liniowych (0 .. {instance.times.length - 1})
      </span>
    </div>

    <!-- Day Columns Layout -->
    <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-5 gap-3">
      {#each daysMatrix as col}
        <div class="bg-zinc-950 border border-zinc-800 rounded flex flex-col overflow-hidden">
          <div class="bg-zinc-900/90 px-3 py-2 border-b border-zinc-800 flex items-center justify-between">
            <span class="font-bold text-xs text-white truncate">{col.group.name || col.group.id}</span>
            <span class="text-[10px] font-mono text-amber-400 bg-amber-500/10 px-1.5 py-0.5 rounded border border-amber-500/20">
              {col.times.length} slotów
            </span>
          </div>

          <div class="p-2 flex flex-col gap-1.5 max-h-96 overflow-y-auto">
            {#each col.times as slot, slotIdx}
              <div class="p-2 bg-zinc-900/60 hover:bg-zinc-800/80 rounded border border-zinc-800/60 flex items-center justify-between text-xs transition-colors">
                <div class="flex items-center gap-2">
                  <span class="w-5 text-right font-mono text-zinc-500 text-[11px] font-semibold">{slotIdx + 1}.</span>
                  <span class="font-medium text-zinc-200">{slot.name || slot.id}</span>
                </div>
                <span class="font-mono text-[10px] text-zinc-400 bg-zinc-950 px-1 py-0.5 rounded border border-zinc-800">
                  {slot.id}
                </span>
              </div>
            {/each}
          </div>
        </div>
      {/each}
    </div>
  </div>

  <!-- Complete Times Table -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
    <div class="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3">
      <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
        Wszystkie sloty czasowe (Times)
      </h3>

      <div class="flex items-center gap-2 w-full sm:w-auto">
        <!-- Group filter -->
        <select
          bind:value={selectedGroup}
          class="px-2 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 font-mono focus:outline-none focus:border-zinc-500"
        >
          <option value="all">Wszystkie grupy ({instance.times.length})</option>
          {#each instance.time_groups as g}
            <option value={g.id}>{g.name || g.id} ({g.kind})</option>
          {/each}
        </select>

        <!-- Search -->
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj slotu..."
          class="px-2.5 py-1 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 font-mono focus:outline-none focus:border-zinc-500"
        />
      </div>
    </div>

    <div class="overflow-x-auto">
      <table class="w-full text-left text-xs font-mono">
        <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
          <tr>
            <th class="pb-2 w-16">Indeks</th>
            <th class="pb-2">Identyfikator</th>
            <th class="pb-2">Etykieta (Nazwa)</th>
            <th class="pb-2">Grupy czasowe (TimeGroups)</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-zinc-800/60">
          {#each filteredTimes as time}
            <tr class="hover:bg-zinc-800/40">
              <td class="py-2 text-zinc-500 font-bold">{time.index}</td>
              <td class="py-2 font-bold text-amber-400">{time.id}</td>
              <td class="py-2 text-zinc-200 font-sans">{time.name || "—"}</td>
              <td class="py-2">
                <div class="flex flex-wrap gap-1">
                  {#each time.group_refs as gr}
                    <span class="px-1.5 py-0.5 bg-zinc-950 text-zinc-300 rounded border border-zinc-800 text-[10px]">
                      {gr}
                    </span>
                  {/each}
                </div>
              </td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

<script lang="ts">
  import type { InstanceDetail } from "../types";

  interface Props {
    instance: InstanceDetail;
    onGoToTab: (tab: string) => void;
  }

  let { instance, onGoToTab }: Props = $props();

  // Constraints breakdown by type
  let constraintTypesMap = $derived.by(() => {
    const map: Record<string, { total: number; hard: number; soft: number; weightSum: number }> = {};
    for (const c of instance.constraints) {
      if (!map[c.type]) {
        map[c.type] = { total: 0, hard: 0, soft: 0, weightSum: 0 };
      }
      map[c.type].total += 1;
      if (c.required) map[c.type].hard += 1;
      else map[c.type].soft += 1;
      map[c.type].weightSum += c.weight;
    }
    return Object.entries(map).sort((a, b) => b[1].total - a[1].total);
  });

  // Duration distribution
  let durationDist = $derived.by(() => {
    const dist: Record<number, number> = {};
    for (const e of instance.events) {
      dist[e.duration] = (dist[e.duration] || 0) + 1;
    }
    return Object.entries(dist).sort((a, b) => Number(a[0]) - Number(b[0]));
  });

  // Preassigned vs unassigned
  let preassignedTimes = $derived(
    instance.events.filter((e) => e.time_ref !== null).length
  );
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <!-- Informational Banner -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div>
      <h2 class="text-sm font-bold text-white uppercase tracking-wider font-mono">
        Status instancji: {instance.id}
      </h2>
      <p class="text-xs text-zinc-400 mt-0.5">
        Instancja gotowa do przeszukiwania przez solver. Wszystkie referencje poprawnie zindeksowane.
      </p>
    </div>
    <div class="flex items-center gap-2 font-mono text-xs">
      <span class="px-2 py-1 bg-zinc-800 rounded border border-zinc-700">
        Prealokacje czasu: <strong class="text-amber-400">{preassignedTimes}</strong> / {instance.events.length}
      </span>
      <span class="px-2 py-1 bg-zinc-800 rounded border border-zinc-700">
        Typy zasobów: <strong class="text-amber-400">{instance.resource_types.length}</strong>
      </span>
    </div>
  </div>

  <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
    <!-- Constraints Profile Table -->
    <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex items-center justify-between">
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Rozkład ograniczeń (16 typów XHSTT)
        </h3>
        <button
          onclick={() => onGoToTab("constraints")}
          class="text-xs text-amber-400 hover:text-amber-300 font-mono underline cursor-pointer"
        >
          Przejdź do reguł →
        </button>
      </div>

      <div class="overflow-x-auto">
        <table class="w-full text-left text-xs font-mono">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-2">Typ reguły</th>
              <th class="pb-2 text-right">Liczba</th>
              <th class="pb-2 text-right">Twarde</th>
              <th class="pb-2 text-right">Miękkie</th>
              <th class="pb-2 text-right">Wagi</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#each constraintTypesMap as [type, stats]}
              <tr class="hover:bg-zinc-800/40">
                <td class="py-1.5 font-medium text-zinc-200">{type}</td>
                <td class="py-1.5 text-right font-bold">{stats.total}</td>
                <td class="py-1.5 text-right">
                  {#if stats.hard > 0}
                    <span class="text-rose-400 font-semibold">{stats.hard}</span>
                  {:else}
                    <span class="text-zinc-600">0</span>
                  {/if}
                </td>
                <td class="py-1.5 text-right">
                  {#if stats.soft > 0}
                    <span class="text-amber-400 font-semibold">{stats.soft}</span>
                  {:else}
                    <span class="text-zinc-600">0</span>
                  {/if}
                </td>
                <td class="py-1.5 text-right text-zinc-400">{stats.weightSum}</td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>

    <!-- Events & Resources Breakdown -->
    <div class="flex flex-col gap-6">
      <!-- Duration Breakdown -->
      <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
        <div class="flex items-center justify-between">
          <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
            Rozkład długości lekcji (Duration)
          </h3>
          <button
            onclick={() => onGoToTab("events")}
            class="text-xs text-amber-400 hover:text-amber-300 font-mono underline cursor-pointer"
          >
            Zobacz lekcje →
          </button>
        </div>

        <div class="grid grid-cols-2 sm:grid-cols-4 gap-2">
          {#each durationDist as [dur, count]}
            <div class="bg-zinc-950 p-2.5 rounded border border-zinc-800 flex flex-col items-center">
              <span class="text-[11px] text-zinc-400 font-mono">{dur} {Number(dur) === 1 ? "godzina" : "godziny"}</span>
              <span class="text-lg font-bold font-mono text-zinc-100 mt-0.5">{count}</span>
              <span class="text-[10px] text-zinc-500 font-mono">
                {Math.round((count / instance.events.length) * 100)}% zdarzeń
              </span>
            </div>
          {/each}
        </div>
      </div>

      <!-- Quick Exploration Cards -->
      <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Struktura danych
        </h3>
        <div class="grid grid-cols-1 sm:grid-cols-3 gap-2 text-xs font-mono">
          <button
            onclick={() => onGoToTab("times")}
            class="p-2.5 bg-zinc-950 hover:bg-zinc-800/80 rounded border border-zinc-800 text-left transition-colors cursor-pointer flex flex-col justify-between"
          >
            <span class="text-zinc-400 text-[11px]">Dni i sloty</span>
            <span class="text-amber-400 font-bold text-sm mt-1">{instance.times.length} slotów</span>
            <span class="text-zinc-500 text-[10px] mt-1">{instance.time_groups.length} grup czasowych</span>
          </button>

          <button
            onclick={() => onGoToTab("resources")}
            class="p-2.5 bg-zinc-950 hover:bg-zinc-800/80 rounded border border-zinc-800 text-left transition-colors cursor-pointer flex flex-col justify-between"
          >
            <span class="text-zinc-400 text-[11px]">Zasoby</span>
            <span class="text-amber-400 font-bold text-sm mt-1">{instance.resources.length} zasobów</span>
            <span class="text-zinc-500 text-[10px] mt-1">{instance.resource_types.length} typów zasobów</span>
          </button>

          <button
            onclick={() => onGoToTab("groups")}
            class="p-2.5 bg-zinc-950 hover:bg-zinc-800/80 rounded border border-zinc-800 text-left transition-colors cursor-pointer flex flex-col justify-between"
          >
            <span class="text-zinc-400 text-[11px]">Grupy obiektów</span>
            <span class="text-amber-400 font-bold text-sm mt-1">
              {instance.time_groups.length + instance.resource_groups.length + instance.event_groups.length} grup
            </span>
            <span class="text-zinc-500 text-[10px] mt-1">Czas / Zasób / Kurs</span>
          </button>
        </div>
      </div>
    </div>
  </div>
</div>

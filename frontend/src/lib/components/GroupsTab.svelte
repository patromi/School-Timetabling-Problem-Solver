<script lang="ts">
  import type { InstanceDetail } from "../types";

  interface Props {
    instance: InstanceDetail;
  }

  let { instance }: Props = $props();

  let activeSection = $state<"time" | "resource" | "event">("time");

  // Member counts
  let timeGroupCounts = $derived.by(() => {
    const counts: Record<string, number> = {};
    for (const t of instance.times) {
      for (const gr of t.group_refs) {
        counts[gr] = (counts[gr] || 0) + 1;
      }
    }
    return counts;
  });

  let resourceGroupCounts = $derived.by(() => {
    const counts: Record<string, number> = {};
    for (const r of instance.resources) {
      for (const gr of r.group_refs) {
        counts[gr] = (counts[gr] || 0) + 1;
      }
    }
    return counts;
  });

  let eventGroupCounts = $derived.by(() => {
    const counts: Record<string, number> = {};
    for (const e of instance.events) {
      for (const gr of e.group_refs) {
        counts[gr] = (counts[gr] || 0) + 1;
      }
    }
    return counts;
  });
</script>

<div class="p-6 flex flex-col gap-6 max-w-7xl mx-auto text-zinc-200">
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-4">
    <!-- Sub-navigation -->
    <div class="flex items-center justify-between border-b border-zinc-800 pb-3">
      <div>
        <h3 class="text-xs font-bold text-zinc-300 uppercase tracking-wider font-mono">
          Struktury Grupowania (Groups)
        </h3>
        <p class="text-xs text-zinc-500 font-mono mt-0.5">
          Grupy czasowe, grupy zasobów oraz grupy lekcji / kursów
        </p>
      </div>

      <div class="flex items-center gap-1 font-mono text-xs">
        <button
          onclick={() => (activeSection = "time")}
          class={`px-3 py-1 rounded border transition-colors ${
            activeSection === "time"
              ? "bg-zinc-800 border-amber-500 text-amber-400 font-bold"
              : "border-zinc-800 text-zinc-400 hover:text-zinc-200"
          }`}
        >
          Czas ({instance.time_groups.length})
        </button>
        <button
          onclick={() => (activeSection = "resource")}
          class={`px-3 py-1 rounded border transition-colors ${
            activeSection === "resource"
              ? "bg-zinc-800 border-amber-500 text-amber-400 font-bold"
              : "border-zinc-800 text-zinc-400 hover:text-zinc-200"
          }`}
        >
          Zasoby ({instance.resource_groups.length})
        </button>
        <button
          onclick={() => (activeSection = "event")}
          class={`px-3 py-1 rounded border transition-colors ${
            activeSection === "event"
              ? "bg-zinc-800 border-amber-500 text-amber-400 font-bold"
              : "border-zinc-800 text-zinc-400 hover:text-zinc-200"
          }`}
        >
          Kursy / Lekcje ({instance.event_groups.length})
        </button>
      </div>
    </div>

    <!-- Tables based on activeSection -->
    {#if activeSection === "time"}
      <div class="overflow-x-auto">
        <table class="w-full text-left text-xs font-mono">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-2">Identyfikator</th>
              <th class="pb-2">Nazwa grupy</th>
              <th class="pb-2">Typ / Tag XML (Kind)</th>
              <th class="pb-2 text-right">Liczba slotów czasowych</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if instance.time_groups.length === 0}
              <tr>
                <td colspan="4" class="py-4 text-center text-zinc-500 italic">Brak zdefiniowanych grup czasowych</td>
              </tr>
            {:else}
              {#each instance.time_groups as g}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2.5 font-bold text-amber-400">{g.id}</td>
                  <td class="py-2.5 text-zinc-200 font-sans">{g.name || "—"}</td>
                  <td class="py-2.5">
                    <span class="px-2 py-0.5 rounded text-[10px] bg-zinc-950 text-zinc-300 border border-zinc-800">
                      {g.kind}
                    </span>
                  </td>
                  <td class="py-2.5 text-right font-bold text-zinc-100">
                    {timeGroupCounts[g.id] || 0}
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    {:else if activeSection === "resource"}
      <div class="overflow-x-auto">
        <table class="w-full text-left text-xs font-mono">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-2">Identyfikator</th>
              <th class="pb-2">Nazwa grupy</th>
              <th class="pb-2">Typ / Tag XML (Kind)</th>
              <th class="pb-2 text-right">Liczba przypisanych zasobów</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if instance.resource_groups.length === 0}
              <tr>
                <td colspan="4" class="py-4 text-center text-zinc-500 italic">Brak zdefiniowanych grup zasobów</td>
              </tr>
            {:else}
              {#each instance.resource_groups as g}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2.5 font-bold text-amber-400">{g.id}</td>
                  <td class="py-2.5 text-zinc-200 font-sans">{g.name || "—"}</td>
                  <td class="py-2.5">
                    <span class="px-2 py-0.5 rounded text-[10px] bg-zinc-950 text-zinc-300 border border-zinc-800">
                      {g.kind}
                    </span>
                  </td>
                  <td class="py-2.5 text-right font-bold text-zinc-100">
                    {resourceGroupCounts[g.id] || 0}
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    {:else if activeSection === "event"}
      <div class="overflow-x-auto">
        <table class="w-full text-left text-xs font-mono">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-2">Identyfikator</th>
              <th class="pb-2">Nazwa grupy</th>
              <th class="pb-2">Typ / Tag XML (Kind)</th>
              <th class="pb-2 text-right">Liczba lekcji w grupie</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if instance.event_groups.length === 0}
              <tr>
                <td colspan="4" class="py-4 text-center text-zinc-500 italic">Brak zdefiniowanych grup zdarzeń</td>
              </tr>
            {:else}
              {#each instance.event_groups as g}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2.5 font-bold text-amber-400">{g.id}</td>
                  <td class="py-2.5 text-zinc-200 font-sans">{g.name || "—"}</td>
                  <td class="py-2.5">
                    <span class="px-2 py-0.5 rounded text-[10px] bg-zinc-950 text-zinc-300 border border-zinc-800">
                      {g.kind}
                    </span>
                  </td>
                  <td class="py-2.5 text-right font-bold text-zinc-100">
                    {eventGroupCounts[g.id] || 0}
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    {/if}
  </div>
</div>

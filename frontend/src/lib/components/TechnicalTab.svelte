<script lang="ts">
  import type { InstanceDetail } from "../types";

  interface Props {
    instance: InstanceDetail;
  }

  let { instance }: Props = $props();

  let activeSubTab = $state<"times" | "groups" | "xml">("times");
</script>

<div class="p-6 max-w-6xl mx-auto flex flex-col gap-6 text-slate-800">
  <!-- Sub-navigation -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-4 shadow-xs flex items-center justify-between">
    <div class="flex items-center gap-1 bg-slate-100 p-1 rounded-xl text-xs font-semibold text-slate-600">
      <button
        onclick={() => (activeSubTab = "times")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
          activeSubTab === "times" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        ⏰ Siatka godzin i dni ({instance.times.length})
      </button>

      <button
        onclick={() => (activeSubTab = "groups")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
          activeSubTab === "groups" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        🏷️ Grupy XHSTT ({instance.time_groups.length + instance.resource_groups.length + instance.event_groups.length})
      </button>

      <button
        onclick={() => (activeSubTab = "xml")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
          activeSubTab === "xml" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        📄 Metadane i JSON
      </button>
    </div>

    <span class="text-xs text-slate-400 font-mono">
      Instancja: {instance.id}
    </span>
  </div>

  <!-- Content -->
  {#if activeSubTab === "times"}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
      <h3 class="text-sm font-bold text-slate-900">
        Jednostki lekcyjne i sloty czasowe
      </h3>
      <div class="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-6 gap-2">
        {#each instance.times as t}
          <div class="p-2.5 bg-slate-50 border border-slate-200/80 rounded-xl flex flex-col text-xs">
            <span class="font-bold text-slate-800">{t.name || t.id}</span>
            <span class="text-[10px] text-slate-400 font-mono">ID: {t.id}</span>
          </div>
        {/each}
      </div>
    </div>
  {:else if activeSubTab === "groups"}
    <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
      <!-- Time Groups -->
      <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
        <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
          Grupy Czasowe ({instance.time_groups.length})
        </h4>
        <div class="flex flex-col gap-1.5 max-h-80 overflow-y-auto">
          {#each instance.time_groups as g}
            <div class="p-2 bg-slate-50 rounded-lg text-xs flex justify-between">
              <span class="font-semibold text-slate-800">{g.name || g.id}</span>
              <span class="text-slate-400 font-mono text-[10px]">{g.kind}</span>
            </div>
          {/each}
        </div>
      </div>

      <!-- Resource Groups -->
      <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
        <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
          Grupy Zasobów ({instance.resource_groups.length})
        </h4>
        <div class="flex flex-col gap-1.5 max-h-80 overflow-y-auto">
          {#each instance.resource_groups as g}
            <div class="p-2 bg-slate-50 rounded-lg text-xs flex justify-between">
              <span class="font-semibold text-slate-800">{g.name || g.id}</span>
              <span class="text-slate-400 font-mono text-[10px]">{g.kind}</span>
            </div>
          {/each}
        </div>
      </div>

      <!-- Event Groups -->
      <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
        <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
          Grupy Lekcji ({instance.event_groups.length})
        </h4>
        <div class="flex flex-col gap-1.5 max-h-80 overflow-y-auto">
          {#each instance.event_groups as g}
            <div class="p-2 bg-slate-50 rounded-lg text-xs flex justify-between">
              <span class="font-semibold text-slate-800">{g.name || g.id}</span>
              <span class="text-slate-400 font-mono text-[10px]">{g.kind}</span>
            </div>
          {/each}
        </div>
      </div>
    </div>
  {:else if activeSubTab === "xml"}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-3">
      <h3 class="text-sm font-bold text-slate-900">
        Struktura danych instancji
      </h3>
      <pre class="bg-slate-50 p-4 rounded-xl border border-slate-200 text-xs font-mono text-slate-700 overflow-x-auto max-h-96">{JSON.stringify({
        id: instance.id,
        name: instance.name,
        times_count: instance.times.length,
        resources_count: instance.resources.length,
        events_count: instance.events.length,
        constraints_count: instance.constraints.length,
        resource_types: instance.resource_types
      }, null, 2)}</pre>
    </div>
  {/if}
</div>

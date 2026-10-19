<script lang="ts">
  import type { ConstraintItem, InstanceDetail } from "../types";
  import { getConstraintFriendlyInfo } from "../school_helpers";

  interface Props {
    instance: InstanceDetail;
  }

  let { instance }: Props = $props();

  let searchQuery = $state("");
  let filterType = $state<"all" | "hard" | "soft">("all");
  let expandedConstraintId = $state<string | null>(null);

  let filteredConstraints = $derived(
    instance.constraints.filter((c) => {
      const info = getConstraintFriendlyInfo(c);
      const matchesSearch =
        c.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        c.type.toLowerCase().includes(searchQuery.toLowerCase()) ||
        c.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        info.title.toLowerCase().includes(searchQuery.toLowerCase());
      if (!matchesSearch) return false;

      if (filterType === "hard") return c.required;
      if (filterType === "soft") return !c.required;
      return true;
    })
  );

  let hardCount = $derived(instance.constraints.filter((c) => c.required).length);
  let softCount = $derived(instance.constraints.filter((c) => !c.required).length);

  function toggleExpand(id: string) {
    expandedConstraintId = expandedConstraintId === id ? null : id;
  }
</script>

<div class="p-6 max-w-6xl mx-auto flex flex-col gap-6 text-slate-800">
  <!-- Controls Bar -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-4">
    <!-- Filter Pills -->
    <div class="flex items-center gap-1 bg-slate-100 p-1 rounded-xl text-xs font-semibold text-slate-600">
      <button
        onclick={() => (filterType = "all")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
          filterType === "all" ? "bg-white text-slate-900 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        Wszystkie ({instance.constraints.length})
      </button>

      <button
        onclick={() => (filterType = "hard")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
          filterType === "hard" ? "bg-white text-rose-700 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        <span>🔴 Wymogi bezwzględne</span>
        <span class="text-[10px] bg-rose-50 text-rose-700 px-1.5 py-0.2 rounded-full">{hardCount}</span>
      </button>

      <button
        onclick={() => (filterType = "soft")}
        class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
          filterType === "soft" ? "bg-white text-amber-800 shadow-xs" : "hover:text-slate-900"
        }`}
      >
        <span>🟡 Preferencje</span>
        <span class="text-[10px] bg-amber-50 text-amber-800 px-1.5 py-0.2 rounded-full">{softCount}</span>
      </button>
    </div>

    <!-- Search Input -->
    <div class="relative w-full sm:w-72">
      <input
        type="text"
        bind:value={searchQuery}
        placeholder="Filtruj reguły..."
        class="w-full pl-8 pr-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all"
      />
      <span class="absolute left-2.5 top-2 text-slate-400 text-xs">🔍</span>
    </div>
  </div>

  <!-- Constraints Cards List -->
  {#if filteredConstraints.length === 0}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-12 text-center text-slate-400 text-xs shadow-xs">
      Brak reguł spełniających kryteria wyszukiwania.
    </div>
  {:else}
    <div class="flex flex-col gap-3">
      {#each filteredConstraints as c (c.id)}
        {@const info = getConstraintFriendlyInfo(c)}
        {@const isExpanded = expandedConstraintId === c.id}
        <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs transition-all hover:border-slate-300">
          <div class="flex flex-col sm:flex-row sm:items-start justify-between gap-4">
            <div class="flex items-start gap-3.5">
              <div class="w-10 h-10 rounded-xl flex items-center justify-center font-bold text-base shrink-0 shadow-2xs
                {c.required ? 'bg-rose-50 text-rose-700 border border-rose-100' : 'bg-amber-50 text-amber-700 border border-amber-100'}">
                {#if c.required}
                  🛡️
                {:else}
                  ⭐
                {/if}
              </div>
              <div class="flex flex-col gap-1">
                <div class="flex items-center gap-2 flex-wrap">
                  <h4 class="text-sm font-bold text-slate-900 leading-tight">
                    {info.title}
                  </h4>
                  <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full
                    {c.required ? 'bg-rose-100 text-rose-800' : 'bg-amber-100 text-amber-800'}">
                    {c.required ? 'Wymóg bezwzględny (Hard)' : `Preferencja jakościowa (Waga: ${c.weight})`}
                  </span>
                </div>
                <p class="text-xs text-slate-600 leading-relaxed mt-0.5">
                  {info.description}
                </p>
              </div>
            </div>

            <!-- Scope summary badge & Toggle button -->
            <div class="flex items-center gap-2 self-end sm:self-start shrink-0">
              <button
                onclick={() => toggleExpand(c.id)}
                class="px-2.5 py-1 text-xs font-medium text-slate-600 bg-slate-100 hover:bg-slate-200 rounded-lg transition-all cursor-pointer flex items-center gap-1"
              >
                <span>{isExpanded ? 'Zwiń szczegóły' : 'Szczegóły techniczne'}</span>
                <span>{isExpanded ? '▲' : '▼'}</span>
              </button>
            </div>
          </div>

          <!-- Expandable Technical Details -->
          {#if isExpanded}
            <div class="mt-4 pt-4 border-t border-slate-100 flex flex-col gap-3 text-xs bg-slate-50/70 p-4 rounded-xl">
              <div class="grid grid-cols-2 md:grid-cols-4 gap-3">
                <div>
                  <span class="text-[10px] text-slate-400 font-mono">Typ XHSTT:</span>
                  <div class="font-mono font-semibold text-slate-800">{c.type}</div>
                </div>
                <div>
                  <span class="text-[10px] text-slate-400 font-mono">Identyfikator:</span>
                  <div class="font-mono font-semibold text-slate-800">{c.id}</div>
                </div>
                <div>
                  <span class="text-[10px] text-slate-400 font-mono">Funkcja kosztu:</span>
                  <div class="font-mono font-semibold text-slate-800">{c.cost_function}</div>
                </div>
                <div>
                  <span class="text-[10px] text-slate-400 font-mono">Waga reguły:</span>
                  <div class="font-mono font-semibold text-slate-800">{c.weight}</div>
                </div>
              </div>

              <!-- Scope details -->
              <div class="mt-2 text-[11px] text-slate-600">
                <span class="font-semibold text-slate-700">Zakres oddziaływania: </span>
                <span>
                  {c.applies_to.events.length} lekcji, {c.applies_to.resources.length} zasobów, {c.applies_to.times.length} godzin, {c.applies_to.event_groups.length} grup lekcji
                </span>
              </div>

              {#if Object.keys(c.params).length > 0}
                <div class="mt-1">
                  <span class="font-semibold text-slate-700 text-[11px]">Parametry algorytmu:</span>
                  <pre class="bg-white p-2.5 rounded-lg border border-slate-200 text-[11px] font-mono mt-1 text-slate-700 overflow-x-auto">{JSON.stringify(c.params, null, 2)}</pre>
                </div>
              {/if}
            </div>
          {/if}
        </div>
      {/each}
    </div>
  {/if}
</div>

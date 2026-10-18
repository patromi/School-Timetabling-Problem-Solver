<script lang="ts">
  import type { InstanceDetail } from "../types";
  import { getExportInstanceUrl } from "../api";

  interface Props {
    instance: InstanceDetail;
    activeTab: string;
    onTabChange: (tab: string) => void;
  }

  let { instance, activeTab, onTabChange }: Props = $props();

  let daysCount = $derived(
    instance.time_groups.filter((g) => g.kind.toLowerCase() === "day").length ||
      instance.time_groups.length
  );

  let totalDuration = $derived(
    instance.events.reduce((sum, e) => sum + (e.duration || 1), 0)
  );

  let hardConstraints = $derived(
    instance.constraints.filter((c) => c.required).length
  );

  let softConstraints = $derived(
    instance.constraints.filter((c) => !c.required).length
  );

  let resourcesByType = $derived.by(() => {
    const counts: Record<string, number> = {};
    for (const r of instance.resources) {
      counts[r.resource_type_ref] = (counts[r.resource_type_ref] || 0) + 1;
    }
    return counts;
  });
</script>

<div class="bg-zinc-900 border-b border-zinc-800 p-4 text-zinc-100 flex flex-col gap-4">
  <!-- Title & Action -->
  <div class="flex items-start justify-between gap-4">
    <div>
      <div class="flex items-center gap-2">
        <span class="font-mono text-sm px-2 py-0.5 bg-amber-500/10 text-amber-400 border border-amber-500/30 rounded font-semibold">
          {instance.id}
        </span>
        <h1 class="text-lg font-bold text-white tracking-tight">
          {instance.name || instance.id}
        </h1>
      </div>
      <p class="text-xs text-zinc-400 mt-1">
        Instancja XHSTT benchmarku planowania zajęć szkolnych
      </p>
    </div>

    <div class="flex items-center gap-2">
      <a
        href={getExportInstanceUrl(instance.id)}
        download={`${instance.id}.xml`}
        class="flex items-center gap-1.5 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-xs font-mono font-medium rounded border border-zinc-700 transition-colors"
      >
        <span>📥 Pobierz XML</span>
      </a>
    </div>
  </div>

  <!-- KPI Grid -->
  <div class="grid grid-cols-2 md:grid-cols-4 gap-2 text-xs">
    <!-- Time KPI -->
    <div class="bg-zinc-950 p-2.5 rounded border border-zinc-800 flex flex-col justify-between">
      <span class="text-[11px] text-zinc-400 uppercase font-semibold">Czas i Dni</span>
      <div class="flex items-baseline gap-2 mt-1">
        <span class="text-xl font-bold font-mono text-zinc-100">{instance.times.length}</span>
        <span class="text-zinc-400 text-xs">slotów</span>
      </div>
      <span class="text-[11px] text-zinc-500 mt-0.5 font-mono">
        {daysCount} dni / grup czasowych
      </span>
    </div>

    <!-- Resources KPI -->
    <div class="bg-zinc-950 p-2.5 rounded border border-zinc-800 flex flex-col justify-between">
      <span class="text-[11px] text-zinc-400 uppercase font-semibold">Zasoby</span>
      <div class="flex items-baseline gap-2 mt-1">
        <span class="text-xl font-bold font-mono text-zinc-100">{instance.resources.length}</span>
        <span class="text-zinc-400 text-xs">zasobów</span>
      </div>
      <div class="text-[10px] text-zinc-500 mt-0.5 truncate font-mono">
        {#each Object.entries(resourcesByType) as [type, count], idx}
          <span>{type}: {count}{idx < Object.keys(resourcesByType).length - 1 ? " · " : ""}</span>
        {/each}
      </div>
    </div>

    <!-- Events KPI -->
    <div class="bg-zinc-950 p-2.5 rounded border border-zinc-800 flex flex-col justify-between">
      <span class="text-[11px] text-zinc-400 uppercase font-semibold">Lekcje (Events)</span>
      <div class="flex items-baseline gap-2 mt-1">
        <span class="text-xl font-bold font-mono text-zinc-100">{instance.events.length}</span>
        <span class="text-zinc-400 text-xs">zajęć</span>
      </div>
      <span class="text-[11px] text-zinc-500 mt-0.5 font-mono">
        ∑ {totalDuration} godz. lekcyjnych
      </span>
    </div>

    <!-- Constraints KPI -->
    <div class="bg-zinc-950 p-2.5 rounded border border-zinc-800 flex flex-col justify-between">
      <span class="text-[11px] text-zinc-400 uppercase font-semibold">Ograniczenia</span>
      <div class="flex items-baseline gap-2 mt-1">
        <span class="text-xl font-bold font-mono text-zinc-100">{instance.constraints.length}</span>
        <span class="text-zinc-400 text-xs">reguł</span>
      </div>
      <div class="flex items-center gap-2 text-[10px] font-mono mt-0.5">
        <span class="text-rose-400 font-medium">Twarde: {hardConstraints}</span>
        <span class="text-zinc-600">|</span>
        <span class="text-amber-400 font-medium">Miękkie: {softConstraints}</span>
      </div>
    </div>
  </div>

  <!-- Navigation Tabs -->
  <div class="flex items-center gap-1 border-b border-zinc-800 -mb-4 pt-1 font-mono text-xs overflow-x-auto">
    <button
      onclick={() => onTabChange("overview")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "overview"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      0. Przegląd
    </button>
    <button
      onclick={() => onTabChange("times")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "times"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      1. Czas i Dni ({instance.times.length})
    </button>
    <button
      onclick={() => onTabChange("resources")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "resources"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      2. Zasoby ({instance.resources.length})
    </button>
    <button
      onclick={() => onTabChange("events")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "events"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      3. Lekcje i Zajęcia ({instance.events.length})
    </button>
    <button
      onclick={() => onTabChange("constraints")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "constraints"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      4. Ograniczenia ({instance.constraints.length})
    </button>
    <button
      onclick={() => onTabChange("groups")}
      class={`px-3 py-2 border-b-2 font-medium transition-colors cursor-pointer shrink-0 ${
        activeTab === "groups"
          ? "border-amber-400 text-amber-400"
          : "border-transparent text-zinc-400 hover:text-zinc-200"
      }`}
    >
      5. Grupy ({instance.time_groups.length + instance.resource_groups.length + instance.event_groups.length})
    </button>
  </div>
</div>

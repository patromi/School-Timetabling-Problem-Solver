<script lang="ts">
  interface Props {
    availableCount: number;
    totalSlots: number;
    availabilityPercent: number;
    onClearAll: () => void;
    onBlockAll: () => void;
    onInvertSelection: () => void;
  }

  let {
    availableCount,
    totalSlots,
    availabilityPercent,
    onClearAll,
    onBlockAll,
    onInvertSelection,
  }: Props = $props();
</script>

<div class="px-6 py-2.5 bg-slate-100/70 border-b border-slate-200/80 flex flex-wrap items-center justify-between gap-3 text-xs shrink-0">
  <div class="flex items-center gap-1.5 flex-wrap">
    <span class="text-slate-500 font-semibold text-[11px] mr-1">Narzędzia globalne:</span>
    <button
      type="button"
      onclick={onClearAll}
      class="px-2.5 py-1 bg-white hover:bg-emerald-50 border border-slate-200 hover:border-emerald-200 text-emerald-700 font-medium rounded-lg transition-colors cursor-pointer flex items-center gap-1"
      title="Odblokowuje wszystkie godziny w tygodniu"
    >
      <span>✨</span>
      <span>Odblokuj wszystko</span>
    </button>
    <button
      type="button"
      onclick={onBlockAll}
      class="px-2.5 py-1 bg-white hover:bg-rose-50 border border-slate-200 hover:border-rose-200 text-rose-700 font-medium rounded-lg transition-colors cursor-pointer flex items-center gap-1"
      title="Blokuje wszystkie godziny w tygodniu"
    >
      <span>🚫</span>
      <span>Zablokuj wszystko</span>
    </button>
    <button
      type="button"
      onclick={onInvertSelection}
      class="px-2.5 py-1 bg-white hover:bg-slate-50 border border-slate-200 text-slate-700 font-medium rounded-lg transition-colors cursor-pointer flex items-center gap-1"
      title="Odwraca obecne zaznaczenie"
    >
      <span>🔄</span>
      <span>Odwróć zaznaczenie</span>
    </button>
  </div>

  <!-- Live Availability KPI -->
  <div class="flex items-center gap-2">
    <span class="text-slate-500">Dostępność:</span>
    <span
      class={`font-bold px-2 py-0.5 rounded-full text-[11px] border ${
        availabilityPercent >= 80
          ? "bg-emerald-50 text-emerald-700 border-emerald-200"
          : availabilityPercent >= 50
          ? "bg-amber-50 text-amber-700 border-amber-200"
          : "bg-rose-50 text-rose-700 border-rose-200"
      }`}
    >
      {availableCount} / {totalSlots} godz. ({availabilityPercent}%)
    </span>
  </div>
</div>

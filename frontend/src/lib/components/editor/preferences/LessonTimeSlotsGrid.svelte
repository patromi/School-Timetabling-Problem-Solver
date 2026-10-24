<!-- frontend/src/lib/components/editor/preferences/LessonTimeSlotsGrid.svelte -->
<script lang="ts">
  interface Props {
    days: { id: string; name: string }[];
    periodsPerDay: number;
    preference: "any" | "morning" | "afternoon" | "custom";
    preferredSet: Set<string>;
    onToggleSlot: (dayId: string, period: number) => void;
    onToggleDay: (dayId: string) => void;
    onTogglePeriod: (period: number) => void;
    onSelectAll: () => void;
    onClearAll: () => void;
  }

  let {
    days,
    periodsPerDay,
    preference,
    preferredSet,
    onToggleSlot,
    onToggleDay,
    onTogglePeriod,
    onSelectAll,
    onClearAll,
  }: Props = $props();

  function getSlotId(dayId: string, period: number): string {
    return `${dayId}_${period}`;
  }

  function isSlotPreferred(dayId: string, period: number): boolean {
    if (preference === "any") return true;
    if (preference === "morning") return period <= 4;
    if (preference === "afternoon") return period >= 5;
    return preferredSet.has(getSlotId(dayId, period));
  }
</script>

<div class="bg-slate-50 p-4 rounded-xl border border-slate-200 flex flex-col gap-3">
  <div class="flex items-center justify-between text-xs">
    <div class="flex items-center gap-2">
      <span class="font-bold text-slate-800">
        {preference === 'custom' ? 'Zaznacz preferowane godziny' : 'Podgląd preferowanych slotów'}:
      </span>
      <span class="text-[11px] text-slate-500">
        (zielone = preferowane, szare = unikane)
      </span>
    </div>

    {#if preference === 'custom'}
      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={onSelectAll}
          class="px-2 py-0.5 bg-white border border-slate-200 hover:bg-slate-100 text-slate-700 text-[10px] font-semibold rounded cursor-pointer"
        >
          Zaznacz wszystkie
        </button>
        <button
          type="button"
          onclick={onClearAll}
          class="px-2 py-0.5 bg-white border border-slate-200 hover:bg-slate-100 text-slate-700 text-[10px] font-semibold rounded cursor-pointer"
        >
          Wyczyść
        </button>
      </div>
    {/if}
  </div>

  <!-- Grid -->
  <div class="overflow-x-auto">
    <table class="w-full text-center border-collapse text-xs">
      <thead>
        <tr>
          <th class="p-1.5 text-[11px] text-slate-400 font-medium text-left w-20">Lekcja</th>
          {#each days as d}
            <th class="p-1.5 text-[11px] font-bold text-slate-700">
              {#if preference === 'custom'}
                <button
                  type="button"
                  onclick={() => onToggleDay(d.id)}
                  class="px-2 py-0.5 bg-white border border-slate-200 hover:border-emerald-500 rounded text-slate-800 font-bold transition-all cursor-pointer text-xs"
                  title="Kliknij, aby przełączyć cały dzień"
                >
                  {d.name}
                </button>
              {:else}
                <span>{d.name}</span>
              {/if}
            </th>
          {/each}
        </tr>
      </thead>
      <tbody>
        {#each Array.from({ length: periodsPerDay }, (_, i) => i + 1) as p}
          <tr class="border-t border-slate-200/60">
            <td class="p-1.5 text-left text-[11px] font-semibold text-slate-600">
              {#if preference === 'custom'}
                <button
                  type="button"
                  onclick={() => onTogglePeriod(p)}
                  class="px-1.5 py-0.5 bg-white border border-slate-200 hover:border-emerald-500 rounded text-slate-700 font-semibold transition-all cursor-pointer text-[10px]"
                  title="Kliknij, aby przełączyć tę godzinę we wszystkie dni"
                >
                  Lekcja {p}
                </button>
              {:else}
                <span>Lekcja {p}</span>
              {/if}
            </td>
            {#each days as d}
              {@const preferred = isSlotPreferred(d.id, p)}
              <td class="p-1">
                <button
                  type="button"
                  onclick={() => onToggleSlot(d.id, p)}
                  class="w-full py-1.5 rounded-lg font-medium text-[11px] transition-all cursor-pointer border {preferred ? 'bg-emerald-500 text-white border-emerald-600 shadow-xs' : 'bg-slate-200/70 text-slate-500 border-slate-300 hover:bg-slate-300'}"
                  title="{preferred ? 'Preferowana' : 'Neutralna / unikana'}"
                >
                  {preferred ? '⭐' : '—'}
                </button>
              </td>
            {/each}
          </tr>
        {/each}
      </tbody>
    </table>
  </div>
</div>

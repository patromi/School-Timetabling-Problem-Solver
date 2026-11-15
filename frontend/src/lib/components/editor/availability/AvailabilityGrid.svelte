<!-- frontend/src/lib/components/editor/availability/AvailabilityGrid.svelte -->
<script lang="ts">
  import Icon from "../../common/Icon.svelte";

  interface Props {
    days: { id: string; name: string }[];
    periodsPerDay: number;
    unavailableSet: Set<string>;
    totalSlots: number;
    onToggleSlot: (dayId: string, period: number) => void;
    onToggleWholeDay: (dayId: string) => void;
    onToggleWholePeriod: (period: number) => void;
    onToggleAllSlots: () => void;
  }

  let {
    days,
    periodsPerDay,
    unavailableSet,
    totalSlots,
    onToggleSlot,
    onToggleWholeDay,
    onToggleWholePeriod,
    onToggleAllSlots,
  }: Props = $props();

  function getSlotId(dayId: string, period: number): string {
    return `${dayId}_${period}`;
  }

  function isSlotUnavailable(dayId: string, period: number): boolean {
    return unavailableSet.has(getSlotId(dayId, period));
  }

  function isPeriodFullyBlocked(period: number): boolean {
    return days.length > 0 && days.every((d) => unavailableSet.has(getSlotId(d.id, period)));
  }

  function isDayFullyBlocked(dayId: string): boolean {
    if (periodsPerDay === 0) return false;
    for (let p = 1; p <= periodsPerDay; p++) {
      if (!unavailableSet.has(getSlotId(dayId, p))) return false;
    }
    return true;
  }
</script>

<div class="border border-slate-200 rounded-xl overflow-hidden shadow-2xs">
  <table class="w-full text-center border-collapse text-xs select-none">
    <thead>
      <tr class="bg-slate-100/90 text-slate-700 border-b border-slate-200">
        <!-- Corner header with all-slots toggle -->
        <th class="p-1 text-left w-36 border-r border-slate-200 bg-slate-100/90">
          <button
            type="button"
            onclick={onToggleAllSlots}
            class="w-full py-2 px-2 text-[11px] font-bold rounded-lg border transition-all flex flex-col justify-center cursor-pointer hover:bg-white text-slate-700 border-slate-200"
            title="Kliknij, aby przełączyć cały tydzień naraz"
          >
            <div class="flex items-center justify-between gap-1">
              <span>Godzina \ Dzień</span>
              <Icon name="bolt" size="xs" />
            </div>
            <span class="text-[9px] font-normal text-slate-400">
              {unavailableSet.size === totalSlots ? "kliknij: odblokuj cały tydzień" : "kliknij: zablokuj cały tydzień"}
            </span>
          </button>
        </th>

        <!-- Day headers (column toggles) -->
        {#each days as day}
          {@const dayBlocked = isDayFullyBlocked(day.id)}
          <th class="p-1 border-r border-slate-200 last:border-r-0">
            <button
              type="button"
              onclick={() => onToggleWholeDay(day.id)}
              class={`w-full py-2 px-2 rounded-lg border font-bold text-xs transition-all flex flex-col items-center justify-center gap-0.5 cursor-pointer ${
                dayBlocked
                  ? "bg-rose-50 text-rose-800 border-rose-300 hover:bg-rose-100"
                  : "bg-white text-slate-800 border-slate-200 hover:border-slate-300 hover:bg-slate-50"
              }`}
              title={`Kliknij, aby przełączyć cały dzień: ${day.name}`}
            >
              <div class="flex items-center gap-1">
                <span>{day.name}</span>
                {#if dayBlocked}
                  <Icon name="block" size="xs" class="text-rose-600" />
                {/if}
              </div>
              <span class={`text-[9px] font-normal ${dayBlocked ? "text-rose-600 font-semibold" : "text-slate-400"}`}>
                {dayBlocked ? "cały zablokowany" : "kliknij: cały dzień"}
              </span>
            </button>
          </th>
        {/each}
      </tr>
    </thead>
    <tbody class="divide-y divide-slate-200">
      {#each Array.from({ length: periodsPerDay }, (_, i) => i + 1) as period}
        {@const periodBlocked = isPeriodFullyBlocked(period)}
        <tr class="hover:bg-slate-50/60 transition-colors">
          <!-- Period header (row toggle on the left side) -->
          <td class="p-1 border-r border-slate-200 bg-slate-50/80">
            <button
              type="button"
              onclick={() => onToggleWholePeriod(period)}
              class={`w-full py-2 px-2.5 rounded-lg border font-bold text-xs text-left transition-all flex flex-col justify-center cursor-pointer ${
                periodBlocked
                  ? "bg-rose-50 text-rose-800 border-rose-300 hover:bg-rose-100"
                  : "bg-white text-slate-700 border-slate-200 hover:border-slate-300 hover:bg-slate-50"
              }`}
              title={`Kliknij, aby zablokować/odblokować lekcję ${period} we wszystkich dniach roboczych`}
            >
              <div class="flex items-center justify-between gap-1">
                <span>Lekcja {period}</span>
                {#if periodBlocked}
                  <Icon name="block" size="xs" class="text-rose-600" />
                {/if}
              </div>
              <span class={`text-[9px] font-normal ${periodBlocked ? "text-rose-600 font-semibold" : "text-slate-400"}`}>
                {periodBlocked ? "zablokowana we wszystkich dniach" : "kliknij: cały rząd"}
              </span>
            </button>
          </td>

          <!-- Individual day x period cells -->
          {#each days as day}
            {@const unavailable = isSlotUnavailable(day.id, period)}
            <td class="p-1 border-r border-slate-200 last:border-r-0">
              <button
                type="button"
                onclick={() => onToggleSlot(day.id, period)}
                class={`w-full py-2.5 px-2 rounded-lg font-medium text-xs transition-all flex flex-col items-center justify-center gap-0.5 cursor-pointer ${
                  unavailable
                    ? "bg-rose-100 hover:bg-rose-200 text-rose-800 border border-rose-300 shadow-2xs"
                    : "bg-white hover:bg-emerald-50/60 text-slate-700 hover:text-emerald-800 border border-slate-100 hover:border-emerald-200"
                }`}
              >
                {#if unavailable}
                  <Icon name="block" size="xs" class="text-rose-700" />
                  <span class="text-[10px] font-bold">Niedostępny</span>
                {:else}
                  <Icon name="check" size="xs" class="text-slate-300 group-hover:text-emerald-500" />
                  <span class="text-[10px] text-slate-400">Dostępny</span>
                {/if}
              </button>
            </td>
          {/each}
        </tr>
      {/each}
    </tbody>
  </table>
</div>

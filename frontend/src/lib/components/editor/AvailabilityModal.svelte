<script lang="ts">
  import AvailabilityGrid from "./availability/AvailabilityGrid.svelte";
  import AvailabilityAdvancedSettings from "./availability/AvailabilityAdvancedSettings.svelte";
  import AvailabilityContextDrawer from "./availability/AvailabilityContextDrawer.svelte";
  import AvailabilityQuickToolbar from "./availability/AvailabilityQuickToolbar.svelte";

  export interface AvailabilityExtraParams {
    maxDaysPerWeek?: number | null;
    minDaysPerWeek?: number | null;
    maxDailyLessons?: number | null;
    maxGapsPerWeek?: number | null;
    minDailyLessons?: number | null;
  }

  export interface ResourceContextInfo {
    totalAssignedHours?: number;
    workload?: number;
    classes?: { classId: string; className: string; subject: string; hours: number }[];
    lessons?: { subject: string; teacherName: string; hours: number }[];
  }

  interface Props {
    resourceId: string;
    resourceName: string;
    resourceType: "teacher" | "room" | "class";
    days: { id: string; name: string }[];
    periodsPerDay: number;
    initialUnavailableTimes: string[];
    initialIsSoft?: boolean;
    initialWeight?: number;
    initialMaxDaysPerWeek?: number | null;
    initialMinDaysPerWeek?: number | null;
    initialMaxDailyLessons?: number | null;
    initialMaxGapsPerWeek?: number | null;
    initialMinDailyLessons?: number | null;
    contextInfo?: ResourceContextInfo;
    onSave: (
      unavailableTimes: string[],
      isSoft: boolean,
      weight: number,
      extra?: AvailabilityExtraParams
    ) => void;
    onClose: () => void;
  }

  let {
    resourceId,
    resourceName,
    resourceType,
    days,
    periodsPerDay,
    initialUnavailableTimes,
    initialIsSoft = false,
    initialWeight = 10,
    initialMaxDaysPerWeek = null,
    initialMinDaysPerWeek = null,
    initialMaxDailyLessons = null,
    initialMaxGapsPerWeek = null,
    initialMinDailyLessons = null,
    contextInfo = undefined,
    onSave,
    onClose,
  }: Props = $props();

  let unavailableSet = $state<Set<string>>(new Set());
  let isSoft = $state(false);
  let weight = $state(10);
  let maxDaysPerWeek = $state<number | null>(null);
  let minDaysPerWeek = $state<number | null>(null);
  let maxDailyLessons = $state<number | null>(null);
  let maxGapsPerWeek = $state<number | null>(null);
  let minDailyLessons = $state<number | null>(null);

  $effect(() => {
    unavailableSet = new Set(initialUnavailableTimes);
    isSoft = initialIsSoft;
    weight = initialWeight;
    maxDaysPerWeek = initialMaxDaysPerWeek ?? null;
    minDaysPerWeek = initialMinDaysPerWeek ?? null;
    maxDailyLessons = initialMaxDailyLessons ?? null;
    maxGapsPerWeek = initialMaxGapsPerWeek ?? null;
    minDailyLessons = initialMinDailyLessons ?? null;
  });

  function getSlotId(dayId: string, period: number): string {
    return `${dayId}_${period}`;
  }

  function toggleSlot(dayId: string, period: number) {
    const slotId = getSlotId(dayId, period);
    const next = new Set(unavailableSet);
    if (next.has(slotId)) next.delete(slotId); else next.add(slotId);
    unavailableSet = next;
  }

  function toggleWholePeriod(period: number) {
    const next = new Set(unavailableSet);
    const allBlocked = days.length > 0 && days.every((d) => next.has(getSlotId(d.id, period)));

    days.forEach((d) => {
      const slotId = getSlotId(d.id, period);
      if (allBlocked) next.delete(slotId); else next.add(slotId);
    });
    unavailableSet = next;
  }

  function toggleWholeDay(dayId: string) {
    const next = new Set(unavailableSet);
    let allBlocked = true;
    for (let p = 1; p <= periodsPerDay; p++) {
      if (!next.has(getSlotId(dayId, p))) {
        allBlocked = false;
        break;
      }
    }

    for (let p = 1; p <= periodsPerDay; p++) {
      const slotId = getSlotId(dayId, p);
      if (allBlocked) next.delete(slotId); else next.add(slotId);
    }
    unavailableSet = next;
  }

  function toggleAllSlots() {
    if (unavailableSet.size === totalSlots) {
      unavailableSet = new Set();
    } else {
      const next = new Set<string>();
      days.forEach((d) => {
        for (let p = 1; p <= periodsPerDay; p++) next.add(getSlotId(d.id, p));
      });
      unavailableSet = next;
    }
  }

  function clearAll() {
    unavailableSet = new Set();
  }

  function blockAll() {
    const next = new Set<string>();
    days.forEach((d) => {
      for (let p = 1; p <= periodsPerDay; p++) next.add(getSlotId(d.id, p));
    });
    unavailableSet = next;
  }

  function invertSelection() {
    const next = new Set<string>();
    days.forEach((d) => {
      for (let p = 1; p <= periodsPerDay; p++) {
        const slot = getSlotId(d.id, p);
        if (!unavailableSet.has(slot)) next.add(slot);
      }
    });
    unavailableSet = next;
  }

  function handleSave() {
    onSave(Array.from(unavailableSet), isSoft, weight, {
      maxDaysPerWeek: maxDaysPerWeek !== null && String(maxDaysPerWeek) !== "" ? Number(maxDaysPerWeek) : null,
      minDaysPerWeek: minDaysPerWeek !== null && String(minDaysPerWeek) !== "" ? Number(minDaysPerWeek) : null,
      maxDailyLessons: maxDailyLessons !== null && String(maxDailyLessons) !== "" ? Number(maxDailyLessons) : null,
      maxGapsPerWeek: maxGapsPerWeek !== null && String(maxGapsPerWeek) !== "" ? Number(maxGapsPerWeek) : null,
      minDailyLessons: minDailyLessons !== null && String(minDailyLessons) !== "" ? Number(minDailyLessons) : null,
    });
    onClose();
  }

  let totalSlots = $derived(days.length * periodsPerDay);
  let blockedCount = $derived(unavailableSet.size);
  let availableCount = $derived(totalSlots - blockedCount);
  let availabilityPercent = $derived(
    totalSlots > 0 ? Math.round((availableCount / totalSlots) * 100) : 100
  );
</script>

<!-- Backdrop -->
<div
  class="fixed inset-0 z-50 bg-slate-900/60 backdrop-blur-xs flex items-center justify-center p-3 sm:p-5"
  role="dialog"
  aria-modal="true"
  aria-labelledby="availability-title"
>
  <div class="bg-white rounded-2xl border border-slate-200 shadow-2xl w-full max-w-5xl max-h-[92vh] flex flex-col overflow-hidden animate-in fade-in zoom-in-95 duration-150">
    <!-- Modal Header -->
    <div class="px-6 py-4 border-b border-slate-100 flex items-center justify-between shrink-0 bg-slate-50/50">
      <div class="flex items-center gap-3">
        <div class="w-10 h-10 rounded-xl bg-blue-50 border border-blue-200 text-blue-700 flex items-center justify-center text-xl shrink-0">
          {#if resourceType === "teacher"}
            👨‍🏫
          {:else if resourceType === "room"}
            🚪
          {:else}
            👥
          {/if}
        </div>
        <div>
          <div class="flex items-center gap-2">
            <h3 id="availability-title" class="text-base font-bold text-slate-900">
              Dostępność i wykluczenia czasowe: {resourceName}
            </h3>
            <span class="text-[11px] font-semibold px-2 py-0.5 rounded-full bg-slate-100 text-slate-700 border border-slate-200">
              {resourceType === "teacher" ? "Nauczyciel" : resourceType === "room" ? "Sala lekcyjna" : "Oddział"}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5">
            Kliknij na poszczególne kratki, całe godziny lekcyjne (po lewej) lub całe dni (u góry), aby zaznaczyć niedostępność.
          </p>
        </div>
      </div>

      <button
        type="button"
        onclick={onClose}
        class="text-slate-400 hover:text-slate-700 text-lg w-8 h-8 rounded-lg flex items-center justify-center hover:bg-slate-100 transition-colors cursor-pointer"
        title="Zamknij"
      >
        ✕
      </button>
    </div>

    <!-- Quick Actions Toolbar -->
    <AvailabilityQuickToolbar
      {availableCount}
      {totalSlots}
      {availabilityPercent}
      onClearAll={clearAll}
      onBlockAll={blockAll}
      onInvertSelection={invertSelection}
    />

    <!-- Main Content Layout (Left: Grid & Settings, Right: Context Drawer) -->
    <div class="flex-1 overflow-hidden flex flex-col md:flex-row">
      <!-- Left Column: Matrix & Sliders -->
      <div class="p-4 sm:p-5 overflow-y-auto flex-1 flex flex-col gap-4">
        <AvailabilityGrid
          {days}
          {periodsPerDay}
          {unavailableSet}
          {totalSlots}
          onToggleSlot={toggleSlot}
          onToggleWholeDay={toggleWholeDay}
          onToggleWholePeriod={toggleWholePeriod}
          onToggleAllSlots={toggleAllSlots}
        />

        <AvailabilityAdvancedSettings
          {resourceType}
          {days}
          bind:isSoft
          bind:weight
          bind:maxDaysPerWeek
          bind:minDaysPerWeek
          bind:maxDailyLessons
          bind:maxGapsPerWeek
          bind:minDailyLessons
        />
      </div>

      <!-- Right Column: Context Drawer -->
      {#if contextInfo}
        <AvailabilityContextDrawer
          {resourceType}
          {contextInfo}
          {availableCount}
          {totalSlots}
        />
      {/if}
    </div>

    <!-- Footer Controls -->
    <div class="px-6 py-3.5 bg-slate-50 border-t border-slate-200 flex items-center justify-between shrink-0">
      <div class="text-xs text-slate-500">
        {#if blockedCount > 0}
          <span class="text-rose-700 font-semibold">
            Wybrano {blockedCount} {blockedCount === 1 ? "niedostępną godzinę" : "niedostępnych godzin"} • {isSoft ? "Preferencja osobista" : "Wymóg bezwzględny"}
          </span>
        {:else}
          <span class="text-emerald-700 font-semibold">
            Brak wykluczeń — zasób w pełni dyspozycyjny
          </span>
        {/if}
      </div>

      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={onClose}
          class="px-4 py-2 bg-white hover:bg-slate-100 border border-slate-200 text-slate-700 text-xs font-semibold rounded-xl transition-all cursor-pointer"
        >
          Anuluj
        </button>
        <button
          type="button"
          onclick={handleSave}
          class="px-5 py-2 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
        >
          <span>💾</span>
          <span>Zastosuj dostępność</span>
        </button>
      </div>
    </div>
  </div>
</div>

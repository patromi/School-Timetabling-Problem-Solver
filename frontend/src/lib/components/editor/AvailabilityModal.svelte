<script lang="ts">
  interface Props {
    resourceId: string;
    resourceName: string;
    resourceType: "teacher" | "room" | "class";
    days: { id: string; name: string }[];
    periodsPerDay: number;
    initialUnavailableTimes: string[];
    initialIsSoft?: boolean;
    initialWeight?: number;
    onSave: (unavailableTimes: string[], isSoft: boolean, weight: number) => void;
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
    onSave,
    onClose,
  }: Props = $props();

  let unavailableSet = $state<Set<string>>(new Set());
  let isSoft = $state(false);
  let weight = $state(10);

  $effect(() => {
    unavailableSet = new Set(initialUnavailableTimes);
    isSoft = initialIsSoft;
    weight = initialWeight;
  });

  // Helper to format slot ID
  function getSlotId(dayId: string, period: number): string {
    return `${dayId}_${period}`;
  }

  function isSlotUnavailable(dayId: string, period: number): boolean {
    return unavailableSet.has(getSlotId(dayId, period));
  }

  function toggleSlot(dayId: string, period: number) {
    const slotId = getSlotId(dayId, period);
    const next = new Set(unavailableSet);
    if (next.has(slotId)) {
      next.delete(slotId);
    } else {
      next.add(slotId);
    }
    unavailableSet = next;
  }

  // Row-level toggle: all days for a specific lesson period
  function toggleWholePeriod(period: number) {
    const next = new Set(unavailableSet);
    const allBlocked = days.length > 0 && days.every((d) => next.has(getSlotId(d.id, period)));

    days.forEach((d) => {
      const slotId = getSlotId(d.id, period);
      if (allBlocked) {
        next.delete(slotId);
      } else {
        next.add(slotId);
      }
    });
    unavailableSet = next;
  }

  function isPeriodFullyBlocked(period: number): boolean {
    return days.length > 0 && days.every((d) => unavailableSet.has(getSlotId(d.id, period)));
  }

  // Column-level toggle: all periods for a specific day
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
      if (allBlocked) {
        next.delete(slotId);
      } else {
        next.add(slotId);
      }
    }
    unavailableSet = next;
  }

  function isDayFullyBlocked(dayId: string): boolean {
    if (periodsPerDay === 0) return false;
    for (let p = 1; p <= periodsPerDay; p++) {
      if (!unavailableSet.has(getSlotId(dayId, p))) return false;
    }
    return true;
  }

  // Corner global toggle: all slots in the week
  function toggleAllSlots() {
    if (unavailableSet.size === totalSlots) {
      unavailableSet = new Set();
    } else {
      const next = new Set<string>();
      days.forEach((d) => {
        for (let p = 1; p <= periodsPerDay; p++) {
          next.add(getSlotId(d.id, p));
        }
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
      for (let p = 1; p <= periodsPerDay; p++) {
        next.add(getSlotId(d.id, p));
      }
    });
    unavailableSet = next;
  }

  function invertSelection() {
    const next = new Set<string>();
    days.forEach((d) => {
      for (let p = 1; p <= periodsPerDay; p++) {
        const slot = getSlotId(d.id, p);
        if (!unavailableSet.has(slot)) {
          next.add(slot);
        }
      }
    });
    unavailableSet = next;
  }

  function handleSave() {
    onSave(Array.from(unavailableSet), isSoft, weight);
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
  class="fixed inset-0 z-50 bg-slate-900/60 backdrop-blur-xs flex items-center justify-center p-4 overflow-y-auto"
  role="dialog"
  aria-modal="true"
  aria-labelledby="availability-title"
>
  <div
    class="bg-white rounded-2xl border border-slate-200 shadow-2xl w-full max-w-4xl max-h-[92vh] flex flex-col overflow-hidden animate-in fade-in zoom-in-95 duration-150"
  >
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
            Kliknij na pojedyncze komórki, całe godziny lekcyjne (po lewej) lub całe dni (u góry), aby zablokować godziny (AvoidUnavailableTimesConstraint).
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
    <div class="px-6 py-2.5 bg-slate-100/70 border-b border-slate-200/80 flex flex-wrap items-center justify-between gap-3 text-xs shrink-0">
      <div class="flex items-center gap-1.5 flex-wrap">
        <span class="text-slate-500 font-semibold text-[11px] mr-1">Narzędzia globalne:</span>
        <button
          type="button"
          onclick={clearAll}
          class="px-2.5 py-1 bg-white hover:bg-emerald-50 border border-slate-200 hover:border-emerald-200 text-emerald-700 font-medium rounded-lg transition-colors cursor-pointer flex items-center gap-1"
          title="Odblokowuje wszystkie godziny w tygodniu"
        >
          <span>✨</span>
          <span>Odblokuj wszystko</span>
        </button>
        <button
          type="button"
          onclick={blockAll}
          class="px-2.5 py-1 bg-white hover:bg-rose-50 border border-slate-200 hover:border-rose-200 text-rose-700 font-medium rounded-lg transition-colors cursor-pointer flex items-center gap-1"
          title="Blokuje wszystkie godziny w tygodniu"
        >
          <span>🚫</span>
          <span>Zablokuj wszystko</span>
        </button>
        <button
          type="button"
          onclick={invertSelection}
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

    <!-- Main Grid (Scrollable) -->
    <div class="p-6 overflow-y-auto flex-1">
      <div class="border border-slate-200 rounded-xl overflow-hidden shadow-2xs">
        <table class="w-full text-center border-collapse text-xs select-none">
          <thead>
            <tr class="bg-slate-100/90 text-slate-700 border-b border-slate-200">
              <!-- Corner header with all-slots toggle -->
              <th class="p-1 text-left w-36 border-r border-slate-200 bg-slate-100/90">
                <button
                  type="button"
                  onclick={toggleAllSlots}
                  class="w-full py-2 px-2 text-[11px] font-bold rounded-lg border transition-all flex flex-col justify-center cursor-pointer hover:bg-white text-slate-700 border-slate-200"
                  title="Kliknij, aby przełączyć cały tydzień naraz"
                >
                  <div class="flex items-center justify-between gap-1">
                    <span>Godzina \ Dzień</span>
                    <span>⚡</span>
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
                    onclick={() => toggleWholeDay(day.id)}
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
                        <span class="text-[11px] text-rose-600 font-bold">🚫</span>
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
                    onclick={() => toggleWholePeriod(period)}
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
                        <span class="text-[11px] text-rose-600 font-bold">🚫</span>
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
                      onclick={() => toggleSlot(day.id, period)}
                      class={`w-full py-2.5 px-2 rounded-lg font-medium text-xs transition-all flex flex-col items-center justify-center gap-0.5 cursor-pointer ${
                        unavailable
                          ? "bg-rose-100 hover:bg-rose-200 text-rose-800 border border-rose-300 shadow-2xs"
                          : "bg-white hover:bg-emerald-50/60 text-slate-700 hover:text-emerald-800 border border-slate-100 hover:border-emerald-200"
                      }`}
                    >
                      {#if unavailable}
                        <span class="text-xs">🚫</span>
                        <span class="text-[10px] font-bold">Niedostępny</span>
                      {:else}
                        <span class="text-xs text-slate-300 group-hover:text-emerald-500">✓</span>
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

      <!-- Constraint Rigor / Priority Options -->
      <div class="mt-5 p-4 bg-slate-50 rounded-xl border border-slate-200/80 flex flex-col sm:flex-row sm:items-center justify-between gap-4 text-xs">
        <div>
          <span class="font-bold text-slate-900 block">Rygor ograniczenia niedostępności (XHSTT):</span>
          <p class="text-slate-500 text-[11px] mt-0.5">
            Czy zaznaczone wykluczenia są kategorycznym zakazem czy silną preferencją?
          </p>
        </div>

        <div class="flex items-center gap-4">
          <label class="flex items-center gap-2 cursor-pointer">
            <input
              type="radio"
              name="rigor"
              checked={!isSoft}
              onchange={() => (isSoft = false)}
              class="w-4 h-4 accent-rose-600"
            />
            <span class="font-bold text-rose-700">Wymóg bezwzględny (HARD)</span>
          </label>

          <label class="flex items-center gap-2 cursor-pointer">
            <input
              type="radio"
              name="rigor"
              checked={isSoft}
              onchange={() => (isSoft = true)}
              class="w-4 h-4 accent-amber-600"
            />
            <span class="font-bold text-amber-700">Preferencja jakościowa (SOFT)</span>
          </label>

          {#if isSoft}
            <div class="flex items-center gap-1.5 ml-2">
              <span class="text-slate-500 text-[11px]">Waga kary:</span>
              <input
                type="number"
                min="1"
                max="100"
                bind:value={weight}
                class="w-14 px-2 py-1 text-center bg-white border border-slate-300 rounded font-bold text-slate-800"
              />
            </div>
          {/if}
        </div>
      </div>
    </div>

    <!-- Footer Controls -->
    <div class="px-6 py-4 bg-slate-50 border-t border-slate-200 flex items-center justify-between shrink-0">
      <div class="text-xs text-slate-500">
        {#if blockedCount > 0}
          <span class="text-rose-700 font-semibold">
            Wybrano {blockedCount} {blockedCount === 1 ? "zablokowaną godzinę" : "zablokowanych godzin"} ({isSoft ? "SOFT" : "HARD"})
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

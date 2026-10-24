<script lang="ts">
  import LessonTimeSlotsGrid from "./preferences/LessonTimeSlotsGrid.svelte";

  interface Props {
    lessonId: string;
    lessonSubject: string;
    className: string;
    days: { id: string; name: string }[];
    periodsPerDay: number;
    initialPreference?: "any" | "morning" | "afternoon" | "custom";
    initialPreferredTimes?: string[];
    initialIsSoft?: boolean;
    initialWeight?: number;
    onSave: (
      pref: "any" | "morning" | "afternoon" | "custom",
      times: string[],
      isSoft: boolean,
      weight: number
    ) => void;
    onClose: () => void;
  }

  let {
    lessonId,
    lessonSubject,
    className,
    days,
    periodsPerDay,
    initialPreference = "any",
    initialPreferredTimes = [],
    initialIsSoft = true,
    initialWeight = 4,
    onSave,
    onClose,
  }: Props = $props();

  function buildInitialSet(): Set<string> {
    if (initialPreferredTimes && initialPreferredTimes.length > 0) {
      return new Set(initialPreferredTimes);
    }
    if (initialPreference === "custom") {
      const all: string[] = [];
      days.forEach((d) => {
        for (let p = 1; p <= periodsPerDay; p++) {
          all.push(`${d.id}_${p}`);
        }
      });
      return new Set(all);
    }
    return new Set();
  }

  let preference = $state<"any" | "morning" | "afternoon" | "custom">("any");
  let preferredSet = $state<Set<string>>(new Set());
  let isSoft = $state(true);
  let weight = $state(4);

  $effect(() => {
    preference = initialPreference;
    preferredSet = buildInitialSet();
    isSoft = initialIsSoft ?? true;
    weight = initialWeight ?? 4;
  });

  function getSlotId(dayId: string, period: number): string {
    return `${dayId}_${period}`;
  }

  function isSlotPreferred(dayId: string, period: number): boolean {
    if (preference === "any") return true;
    if (preference === "morning") return period <= 4;
    if (preference === "afternoon") return period >= 5;
    return preferredSet.has(getSlotId(dayId, period));
  }

  function selectPreset(type: "any" | "morning" | "afternoon" | "custom") {
    preference = type;
    if (type === "morning") {
      const morningSlots = new Set<string>();
      days.forEach((d) => {
        for (let p = 1; p <= Math.min(4, periodsPerDay); p++) {
          morningSlots.add(getSlotId(d.id, p));
        }
      });
      preferredSet = morningSlots;
    } else if (type === "afternoon") {
      const afternoonSlots = new Set<string>();
      days.forEach((d) => {
        for (let p = 5; p <= periodsPerDay; p++) {
          afternoonSlots.add(getSlotId(d.id, p));
        }
      });
      preferredSet = afternoonSlots;
    } else if (type === "custom") {
      if (preferredSet.size === 0) {
        selectAll();
      }
    } else {
      preferredSet = new Set();
    }
  }

  function toggleCustomSlot(dayId: string, period: number) {
    if (preference !== "custom") {
      const current = new Set<string>();
      days.forEach((d) => {
        for (let p = 1; p <= periodsPerDay; p++) {
          if (isSlotPreferred(d.id, p)) {
            current.add(getSlotId(d.id, p));
          }
        }
      });
      preferredSet = current;
      preference = "custom";
    }

    const slotId = getSlotId(dayId, period);
    const next = new Set(preferredSet);
    if (next.has(slotId)) {
      next.delete(slotId);
    } else {
      next.add(slotId);
    }
    preferredSet = next;
  }

  function toggleDay(dayId: string) {
    if (preference !== "custom") preference = "custom";
    const daySlotIds = Array.from({ length: periodsPerDay }, (_, i) => getSlotId(dayId, i + 1));
    const allSelected = daySlotIds.every((id) => preferredSet.has(id));
    const next = new Set(preferredSet);
    if (allSelected) {
      daySlotIds.forEach((id) => next.delete(id));
    } else {
      daySlotIds.forEach((id) => next.add(id));
    }
    preferredSet = next;
  }

  function togglePeriod(period: number) {
    if (preference !== "custom") preference = "custom";
    const periodSlotIds = days.map((d) => getSlotId(d.id, period));
    const allSelected = periodSlotIds.every((id) => preferredSet.has(id));
    const next = new Set(preferredSet);
    if (allSelected) {
      periodSlotIds.forEach((id) => next.delete(id));
    } else {
      periodSlotIds.forEach((id) => next.add(id));
    }
    preferredSet = next;
  }

  function selectAll() {
    preference = "custom";
    const all: string[] = [];
    days.forEach((d) => {
      for (let p = 1; p <= periodsPerDay; p++) {
        all.push(getSlotId(d.id, p));
      }
    });
    preferredSet = new Set(all);
  }

  function clearAll() {
    preference = "custom";
    preferredSet = new Set();
  }

  function handleSave() {
    let resultTimes: string[] = [];
    if (preference === "morning") {
      days.forEach((d) => {
        for (let p = 1; p <= Math.min(4, periodsPerDay); p++) {
          resultTimes.push(getSlotId(d.id, p));
        }
      });
    } else if (preference === "afternoon") {
      days.forEach((d) => {
        for (let p = 5; p <= periodsPerDay; p++) {
          resultTimes.push(getSlotId(d.id, p));
        }
      });
    } else if (preference === "custom") {
      resultTimes = Array.from(preferredSet);
    }
    onSave(preference, resultTimes, isSoft, weight);
  }
</script>

<div
  class="fixed inset-0 bg-slate-900/60 backdrop-blur-xs z-50 flex items-center justify-center p-4 overflow-y-auto"
  role="dialog"
  aria-modal="true"
  aria-labelledby="lesson-pref-title"
>
  <div class="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-2xl w-full overflow-hidden flex flex-col my-auto animate-in fade-in zoom-in-95 duration-150">
    <!-- Header -->
    <div class="px-6 py-4 bg-gradient-to-r from-slate-900 to-indigo-950 text-white flex items-center justify-between">
      <div class="flex items-center gap-3">
        <span class="w-10 h-10 rounded-xl bg-white/10 flex items-center justify-center text-xl">
          ⭐
        </span>
        <div>
          <h3 id="lesson-pref-title" class="text-base font-bold leading-tight">
            Preferencje godzinowe dla lekcji
          </h3>
          <p class="text-xs text-slate-300 mt-0.5">
            Przedmiot: <strong class="text-white">{lessonSubject}</strong> • Klasa: <strong class="text-white">{className}</strong>
          </p>
        </div>
      </div>
      <button
        type="button"
        onclick={onClose}
        class="text-white/60 hover:text-white p-1 rounded-lg hover:bg-white/10 transition-colors cursor-pointer text-lg"
        aria-label="Zamknij"
      >
        ✕
      </button>
    </div>

    <!-- Body -->
    <div class="p-6 flex flex-col gap-5 overflow-y-auto max-h-[75vh]">
      <!-- Quick Presets -->
      <div>
        <span class="block text-xs font-bold uppercase tracking-wider text-slate-500 mb-2">
          Wybierz preferowaną porę zajęć (PreferTimesConstraint)
        </span>
        <div class="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
          <!-- Any -->
          <button
            type="button"
            onclick={() => selectPreset("any")}
            class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference === 'any' ? 'bg-blue-50/80 border-blue-500 ring-2 ring-blue-500/20' : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
          >
            <div class="flex items-center justify-between">
              <span class="text-base">🕒</span>
              {#if preference === "any"}<span class="w-2 h-2 rounded-full bg-blue-600"></span>{/if}
            </div>
            <div class="mt-2">
              <span class="text-xs font-bold text-slate-900 block">Dowolna pora</span>
              <span class="text-[10px] text-slate-500 leading-tight block mt-0.5">Brak preferencji godzinowych</span>
            </div>
          </button>

          <!-- Morning -->
          <button
            type="button"
            onclick={() => selectPreset("morning")}
            class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference === 'morning' ? 'bg-amber-50/80 border-amber-500 ring-2 ring-amber-500/20' : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
          >
            <div class="flex items-center justify-between">
              <span class="text-base">🌅</span>
              {#if preference === 'morning'}<span class="w-2 h-2 rounded-full bg-amber-600"></span>{/if}
            </div>
            <div class="mt-2">
              <span class="text-xs font-bold text-slate-900 block">Poranne lekcje</span>
              <span class="text-[10px] text-slate-500 leading-tight block mt-0.5">Lekcje 1–4 (np. 8:00–11:30)</span>
            </div>
          </button>

          <!-- Afternoon -->
          <button
            type="button"
            onclick={() => selectPreset("afternoon")}
            class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference === 'afternoon' ? 'bg-indigo-50/80 border-indigo-500 ring-2 ring-indigo-500/20' : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
          >
            <div class="flex items-center justify-between">
              <span class="text-base">🌇</span>
              {#if preference === 'afternoon'}<span class="w-2 h-2 rounded-full bg-indigo-600"></span>{/if}
            </div>
            <div class="mt-2">
              <span class="text-xs font-bold text-slate-900 block">Popołudniowe</span>
              <span class="text-[10px] text-slate-500 leading-tight block mt-0.5">Lekcje 5+ (od 11:45)</span>
            </div>
          </button>

          <!-- Custom -->
          <button
            type="button"
            onclick={() => selectPreset("custom")}
            class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference === 'custom' ? 'bg-emerald-50/80 border-emerald-500 ring-2 ring-emerald-500/20' : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
          >
            <div class="flex items-center justify-between">
              <span class="text-base">🎯</span>
              {#if preference === 'custom'}<span class="w-2 h-2 rounded-full bg-emerald-600"></span>{/if}
            </div>
            <div class="mt-2">
              <span class="text-xs font-bold text-slate-900 block">Własna siatka</span>
              <span class="text-[10px] text-slate-500 leading-tight block mt-0.5">Zaznacz konkretne sloty</span>
            </div>
          </button>
        </div>
      </div>

      <!-- Matrix Preview / Custom editor -->
      <LessonTimeSlotsGrid
        {days}
        {periodsPerDay}
        {preference}
        {preferredSet}
        onToggleSlot={toggleCustomSlot}
        onToggleDay={toggleDay}
        onTogglePeriod={togglePeriod}
        onSelectAll={selectAll}
        onClearAll={clearAll}
      />

      <!-- Constraint Enforcement (HARD vs SOFT) -->
      {#if preference !== 'any'}
        <div class="p-4 bg-indigo-50/50 rounded-xl border border-indigo-100 flex flex-col gap-3">
          <div class="flex items-center justify-between">
            <div>
              <span class="text-xs font-bold text-indigo-950">Charakter reguły preferencji</span>
              <p class="text-[11px] text-indigo-700/80 mt-0.5">
                Określ, czy solver musi bezwzględnie umieścić lekcję w tych godzinach, czy może naruszyć regułę w zamian za karę.
              </p>
            </div>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-indigo-200 text-xs">
              <button
                type="button"
                onclick={() => (isSoft = true)}
                class="px-3 py-1 rounded-md font-semibold transition-all cursor-pointer {isSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
              >
                SOFT (Preferencja)
              </button>
              <button
                type="button"
                onclick={() => (isSoft = false)}
                class="px-3 py-1 rounded-md font-semibold transition-all cursor-pointer {!isSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
              >
                HARD (Wymóg)
              </button>
            </div>
          </div>

          {#if isSoft}
            <div class="pt-3 border-t border-indigo-100/70 flex items-center justify-between gap-4">
              <span class="text-xs text-indigo-900 font-medium">
                Waga kary za umieszczenie poza preferowanymi godzinami:
              </span>
              <div class="flex items-center gap-2">
                <input
                  type="range"
                  min="1"
                  max="20"
                  bind:value={weight}
                  class="w-32 accent-indigo-600 cursor-pointer"
                />
                <span class="text-xs font-bold text-indigo-950 w-6 text-center">{weight}</span>
              </div>
            </div>
          {/if}
        </div>
      {/if}
    </div>

    <!-- Footer -->
    <div class="p-4 bg-slate-50 border-t border-slate-200 flex items-center justify-between">
      <button
        type="button"
        onclick={onClose}
        class="px-4 py-2 text-xs font-semibold text-slate-600 hover:text-slate-900 transition-colors cursor-pointer"
      >
        Anuluj
      </button>

      <button
        type="button"
        onclick={handleSave}
        class="px-5 py-2 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
      >
        <span>Zapisz preferencje</span>
      </button>
    </div>
  </div>
</div>

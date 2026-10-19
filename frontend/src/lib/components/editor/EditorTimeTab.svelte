<script lang="ts">
  import type { GroupItem, TimeItem } from "../../types";

  interface Props {
    timeGroups: GroupItem[];
    times: TimeItem[];
    onChangeTimeGroups: (groups: GroupItem[]) => void;
    onChangeTimes: (times: TimeItem[]) => void;
  }

  let { timeGroups, times, onChangeTimeGroups, onChangeTimes }: Props = $props();

  // Generator states
  let genDaysCount = $state(5);
  let genPeriodsPerDay = $state(7);

  // Manual addition states
  let newDayId = $state("");
  let newDayName = $state("");
  let newTimeId = $state("");
  let newTimeName = $state("");
  let newTimeDayRef = $state("");

  const DEFAULT_DAY_NAMES = ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota", "Niedziela"];
  const DEFAULT_DAY_IDS = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];

  function handleQuickGenerate() {
    const newGroups: GroupItem[] = [];
    const newTimesList: TimeItem[] = [];

    const numDays = Math.min(Math.max(1, genDaysCount), 7);
    const numPeriods = Math.min(Math.max(1, genPeriodsPerDay), 12);

    for (let d = 0; d < numDays; d++) {
      const dayId = DEFAULT_DAY_IDS[d] || `Day_${d + 1}`;
      const dayName = DEFAULT_DAY_NAMES[d] || `Dzień ${d + 1}`;
      newGroups.push({ id: dayId, name: dayName, kind: "Day" });

      for (let p = 1; p <= numPeriods; p++) {
        const timeId = `${dayId}_${p}`;
        const timeName = `${dayName} - lekcja ${p}`;
        newTimesList.push({
          id: timeId,
          name: timeName,
          group_refs: [dayId],
        });
      }
    }

    onChangeTimeGroups(newGroups);
    onChangeTimes(newTimesList);
  }

  function addManualDay() {
    if (!newDayId.trim()) return;
    const dayId = newDayId.trim();
    if (timeGroups.some((g) => g.id === dayId)) return;
    const name = newDayName.trim() || dayId;
    onChangeTimeGroups([...timeGroups, { id: dayId, name, kind: "Day" }]);
    newDayId = "";
    newDayName = "";
  }

  function removeDay(dayId: string) {
    onChangeTimeGroups(timeGroups.filter((g) => g.id !== dayId));
    // Also remove or clean up times in this day
    onChangeTimes(
      times
        .filter((t) => !t.group_refs.includes(dayId))
        .map((t) => ({ ...t, group_refs: t.group_refs.filter((g) => g !== dayId) }))
    );
  }

  function addManualTime() {
    if (!newTimeId.trim()) return;
    const timeId = newTimeId.trim();
    if (times.some((t) => t.id === timeId)) return;
    const name = newTimeName.trim() || timeId;
    const groupRefs = newTimeDayRef ? [newTimeDayRef] : [];
    onChangeTimes([...times, { id: timeId, name, group_refs: groupRefs }]);
    newTimeId = "";
    newTimeName = "";
  }

  function removeTime(timeId: string) {
    onChangeTimes(times.filter((t) => t.id !== timeId));
  }
</script>

<div class="flex flex-col gap-6 font-mono text-xs text-zinc-200">
  <!-- Quick Grid Generator Card -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-4">
    <div class="flex items-center justify-between border-b border-zinc-800 pb-2">
      <div>
        <h4 class="font-bold text-white text-sm">⚡ Szybki Generator Siatki Tygodnia</h4>
        <p class="text-zinc-400 text-xs mt-0.5 font-sans">
          Wygeneruj typową strukturę szkolną (np. 5 dni roboczych po 7 godzin lekcyjnych dziennie).
        </p>
      </div>
      <button
        type="button"
        onclick={handleQuickGenerate}
        class="px-3 py-1.5 bg-amber-500 hover:bg-amber-400 text-zinc-950 font-bold rounded transition-colors cursor-pointer"
      >
        Wygeneruj siatkę ({genDaysCount * genPeriodsPerDay} slotów)
      </button>
    </div>

    <div class="flex flex-wrap items-center gap-6">
      <div class="flex items-center gap-2">
        <label for="gen-days" class="text-zinc-400">Liczba dni:</label>
        <input
          id="gen-days"
          type="number"
          min="1"
          max="7"
          bind:value={genDaysCount}
          class="w-16 px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-center text-white font-bold"
        />
        <span class="text-zinc-500 text-[11px]">(np. 5 = Pon-Pt)</span>
      </div>

      <div class="flex items-center gap-2">
        <label for="gen-periods" class="text-zinc-400">Lekcji dziennie:</label>
        <input
          id="gen-periods"
          type="number"
          min="1"
          max="12"
          bind:value={genPeriodsPerDay}
          class="w-16 px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-center text-white font-bold"
        />
        <span class="text-zinc-500 text-[11px]">(np. 7 = lekcje 1..7)</span>
      </div>
    </div>
  </div>

  <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
    <!-- Days Column -->
    <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex items-center justify-between">
        <h4 class="font-bold text-white">Dni Tygodnia ({timeGroups.length})</h4>
      </div>

      <!-- Add Day Form -->
      <div class="flex items-center gap-1.5">
        <input
          type="text"
          placeholder="ID (np. Mon)"
          bind:value={newDayId}
          class="w-24 px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500"
        />
        <input
          type="text"
          placeholder="Nazwa (np. Poniedziałek)"
          bind:value={newDayName}
          class="flex-1 px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500 font-sans"
        />
        <button
          type="button"
          onclick={addManualDay}
          class="px-2.5 py-1 bg-zinc-800 hover:bg-zinc-700 text-amber-400 rounded border border-zinc-700 cursor-pointer font-bold"
        >
          +
        </button>
      </div>

      <!-- Days List -->
      <div class="flex flex-col gap-1.5 max-h-72 overflow-y-auto divide-y divide-zinc-800/60">
        {#if timeGroups.length === 0}
          <p class="text-zinc-500 text-xs italic py-2">Brak zdefiniowanych dni</p>
        {:else}
          {#each timeGroups as day}
            <div class="pt-1.5 pb-1 flex items-center justify-between text-xs">
              <div class="flex items-center gap-2">
                <span class="text-amber-400 font-bold">{day.id}</span>
                <span class="text-zinc-300 font-sans">{day.name || day.id}</span>
              </div>
              <button
                type="button"
                onclick={() => removeDay(day.id)}
                class="text-zinc-500 hover:text-rose-400 cursor-pointer px-1.5 py-0.5"
                title="Usuń dzień i powiązane sloty"
              >
                ✕
              </button>
            </div>
          {/each}
        {/if}
      </div>
    </div>

    <!-- Times Table (2 cols) -->
    <div class="lg:col-span-2 bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex items-center justify-between">
        <h4 class="font-bold text-white">Sloty Czasowe ({times.length})</h4>
        <span class="text-zinc-500 text-[11px]">Łączny czas planu: {times.length} godzin</span>
      </div>

      <!-- Add Time Form -->
      <div class="flex flex-wrap items-center gap-2 bg-zinc-950 p-2.5 rounded border border-zinc-800">
        <input
          type="text"
          placeholder="ID (np. Mon_1)"
          bind:value={newTimeId}
          class="w-28 px-2 py-1 bg-zinc-900 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
        />
        <input
          type="text"
          placeholder="Etykieta (np. Poniedziałek 8:00)"
          bind:value={newTimeName}
          class="flex-1 px-2 py-1 bg-zinc-900 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none font-sans"
        />
        <select
          bind:value={newTimeDayRef}
          class="px-2 py-1 bg-zinc-900 border border-zinc-800 rounded text-zinc-200"
        >
          <option value="">(Wybierz dzień)</option>
          {#each timeGroups as g}
            <option value={g.id}>{g.name || g.id}</option>
          {/each}
        </select>
        <button
          type="button"
          onclick={addManualTime}
          class="px-3 py-1 bg-zinc-800 hover:bg-zinc-700 text-amber-400 font-bold rounded border border-zinc-700 cursor-pointer"
        >
          + Dodaj slot
        </button>
      </div>

      <!-- Times List / Table -->
      <div class="max-h-80 overflow-y-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-1 w-12">#</th>
              <th class="pb-1">ID</th>
              <th class="pb-1">Etykieta</th>
              <th class="pb-1">Dzień</th>
              <th class="pb-1 text-right w-12">Usuń</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if times.length === 0}
              <tr>
                <td colspan="5" class="py-4 text-center text-zinc-500 italic">
                  Brak slotów czasowych. Wygeneruj siatkę lub dodaj ręcznie.
                </td>
              </tr>
            {:else}
              {#each times as t, idx}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-1 text-zinc-500">{idx + 1}</td>
                  <td class="py-1 text-amber-400 font-bold">{t.id}</td>
                  <td class="py-1 text-zinc-200 font-sans">{t.name || "—"}</td>
                  <td class="py-1 text-zinc-400">
                    {#each t.group_refs as gr}
                      <span class="px-1.5 py-0.2 bg-zinc-950 border border-zinc-800 rounded text-[10px]">
                        {gr}
                      </span>
                    {/each}
                  </td>
                  <td class="py-1 text-right">
                    <button
                      type="button"
                      onclick={() => removeTime(t.id)}
                      class="text-zinc-500 hover:text-rose-400 cursor-pointer px-1"
                    >
                      ✕
                    </button>
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    </div>
  </div>
</div>

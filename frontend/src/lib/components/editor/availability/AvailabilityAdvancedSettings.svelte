<!-- frontend/src/lib/components/editor/availability/AvailabilityAdvancedSettings.svelte -->
<script lang="ts">
  interface Props {
    resourceType: "teacher" | "room" | "class";
    days: { id: string; name: string }[];
    isSoft: boolean;
    weight: number;
    maxDaysPerWeek: number | null;
    minDaysPerWeek: number | null;
    maxDailyLessons: number | null;
    maxGapsPerWeek: number | null;
    minDailyLessons: number | null;
  }

  let {
    resourceType,
    days,
    isSoft = $bindable(),
    weight = $bindable(),
    maxDaysPerWeek = $bindable(),
    minDaysPerWeek = $bindable(),
    maxDailyLessons = $bindable(),
    maxGapsPerWeek = $bindable(),
    minDailyLessons = $bindable(),
  }: Props = $props();
</script>

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

<!-- Parametry obciążenia i czasu pracy dla Nauczycieli -->
{#if resourceType === "teacher"}
  <div class="mt-4 p-4 bg-white rounded-xl border border-indigo-100 shadow-2xs flex flex-col gap-3 text-xs">
    <div class="flex items-center justify-between border-b border-indigo-50/80 pb-2">
      <div class="flex items-center gap-2">
        <span class="text-sm">⚡</span>
        <span class="font-bold text-slate-900">Parametry czasu pracy i obciążenia (Faza 2)</span>
        <span class="text-[10px] font-semibold px-2 py-0.5 bg-indigo-50 text-indigo-700 rounded-full border border-indigo-200">
          Indywidualne preferencje
        </span>
      </div>
      <span class="text-[11px] text-slate-400">Nadpisuje domyślne reguły ogólnoszkolne</span>
    </div>

    <div class="grid grid-cols-1 sm:grid-cols-3 gap-3">
      <!-- Maks dni roboczych (ClusterBusyTimes) -->
      <div class="p-3 bg-slate-50/80 rounded-lg border border-slate-200/60 flex flex-col justify-between">
        <div>
          <label for="teacher-max-days-input" class="font-semibold text-slate-800 flex items-center gap-1.5 mb-1">
            <span>📅</span>
            <span>Dni w tygodniu:</span>
          </label>
          <p class="text-[11px] text-slate-500 leading-snug">
            Maks. liczba aktywnych dni (np. dla niepełnego etatu).
          </p>
        </div>
        <div class="mt-2 flex items-center gap-2">
          <select
            id="teacher-max-days-input"
            bind:value={maxDaysPerWeek}
            class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 focus:outline-none focus:border-indigo-500"
          >
            <option value={null}>Brak limitu (cały tydzień)</option>
            {#each days as d, i}
              {#if i + 1 < days.length}
                <option value={i + 1}>Maks. {i + 1} {i === 0 ? "dzień" : i < 4 ? "dni" : "dni"}</option>
              {/if}
            {/each}
          </select>
        </div>
      </div>

      <!-- Maks lekcji dziennie (LimitBusyTimes) -->
      <div class="p-3 bg-slate-50/80 rounded-lg border border-slate-200/60 flex flex-col justify-between">
        <div>
          <label for="teacher-max-daily-input" class="font-semibold text-slate-800 flex items-center gap-1.5 mb-1">
            <span>⏱️</span>
            <span>Lekcji dziennie:</span>
          </label>
          <p class="text-[11px] text-slate-500 leading-snug">
            Maks. liczba zajęć w jednym dniu roboczym.
          </p>
        </div>
        <div class="mt-2 flex items-center gap-2">
          <select
            id="teacher-max-daily-input"
            bind:value={maxDailyLessons}
            class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 focus:outline-none focus:border-indigo-500"
          >
            <option value={null}>Domyślny limit ze szkoły</option>
            <option value={3}>Maks. 3 lekcje</option>
            <option value={4}>Maks. 4 lekcje</option>
            <option value={5}>Maks. 5 lekcji</option>
            <option value={6}>Maks. 6 lekcji</option>
            <option value={7}>Maks. 7 lekcji</option>
            <option value={8}>Maks. 8 lekcji</option>
          </select>
        </div>
      </div>

      <!-- Maks okienek w tygodniu (LimitIdleTimes) -->
      <div class="p-3 bg-slate-50/80 rounded-lg border border-slate-200/60 flex flex-col justify-between">
        <div>
          <label for="teacher-max-gaps-input" class="font-semibold text-slate-800 flex items-center gap-1.5 mb-1">
            <span>☕</span>
            <span>Okienka (tydzień):</span>
          </label>
          <p class="text-[11px] text-slate-500 leading-snug">
            Dopuszczalna liczba wolnych godzin między lekcjami.
          </p>
        </div>
        <div class="mt-2 flex items-center gap-2">
          <select
            id="teacher-max-gaps-input"
            bind:value={maxGapsPerWeek}
            class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 focus:outline-none focus:border-indigo-500"
          >
            <option value={null}>Domyślny limit ze szkoły</option>
            <option value={0}>0 okienek (ciągły plan)</option>
            <option value={1}>Maks. 1 okienko</option>
            <option value={2}>Maks. 2 okienka</option>
            <option value={3}>Maks. 3 okienka</option>
            <option value={4}>Maks. 4 okienka</option>
          </select>
        </div>
      </div>
    </div>
  </div>

<!-- Parametry obciążenia dla Klas -->
{:else if resourceType === "class"}
  <div class="mt-4 p-4 bg-white rounded-xl border border-indigo-100 shadow-2xs flex flex-col gap-3 text-xs">
    <div class="flex items-center justify-between border-b border-indigo-50/80 pb-2">
      <div class="flex items-center gap-2">
        <span class="text-sm">⚖️</span>
        <span class="font-bold text-slate-900">Dzienne ramy lekcyjne oddziału (LimitBusyTimes)</span>
        <span class="text-[10px] font-semibold px-2 py-0.5 bg-indigo-50 text-indigo-700 rounded-full border border-indigo-200">
          Higiena nauki
        </span>
      </div>
      <span class="text-[11px] text-slate-400">Nadpisuje domyślne reguły ogólnoszkolne</span>
    </div>

    <div class="grid grid-cols-1 sm:grid-cols-2 gap-3">
      <!-- Min lekcji dziennie -->
      <div class="p-3 bg-slate-50/80 rounded-lg border border-slate-200/60 flex flex-col justify-between">
        <div>
          <label for="class-min-daily-input" class="font-semibold text-slate-800 flex items-center gap-1.5 mb-1">
            <span>📉</span>
            <span>Minimalna liczba lekcji w dniu:</span>
          </label>
          <p class="text-[11px] text-slate-500 leading-snug">
            Zapobiega przychodzeniu uczniów do szkoły na zbyt małą liczbę lekcji.
          </p>
        </div>
        <div class="mt-2 flex items-center gap-2">
          <select
            id="class-min-daily-input"
            bind:value={minDailyLessons}
            class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 focus:outline-none focus:border-indigo-500"
          >
            <option value={null}>Domyślny limit ze szkoły</option>
            <option value={2}>Min. 2 lekcje</option>
            <option value={3}>Min. 3 lekcje</option>
            <option value={4}>Min. 4 lekcje</option>
            <option value={5}>Min. 5 lekcji</option>
          </select>
        </div>
      </div>

      <!-- Maks lekcji dziennie -->
      <div class="p-3 bg-slate-50/80 rounded-lg border border-slate-200/60 flex flex-col justify-between">
        <div>
          <label for="class-max-daily-input" class="font-semibold text-slate-800 flex items-center gap-1.5 mb-1">
            <span>📈</span>
            <span>Maksymalna liczba lekcji w dniu:</span>
          </label>
          <p class="text-[11px] text-slate-500 leading-snug">
            Chroni uczniów przed nadmiernym zmęczeniem zbyt długim dniem nauki.
          </p>
        </div>
        <div class="mt-2 flex items-center gap-2">
          <select
            id="class-max-daily-input"
            bind:value={maxDailyLessons}
            class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 focus:outline-none focus:border-indigo-500"
          >
            <option value={null}>Domyślny limit ze szkoły</option>
            <option value={5}>Maks. 5 lekcji</option>
            <option value={6}>Maks. 6 lekcji</option>
            <option value={7}>Maks. 7 lekcji</option>
            <option value={8}>Maks. 8 lekcji</option>
            <option value={9}>Maks. 9 lekcji</option>
          </select>
        </div>
      </div>
    </div>
  </div>
{/if}

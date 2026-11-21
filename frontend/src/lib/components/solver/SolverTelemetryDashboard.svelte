<script lang="ts">
  import type { SolverStatus } from "../../types";
  import { getDownloadSolutionUrl } from "../../api";
  import Icon from "../common/Icon.svelte";

  interface Props {
    solverStatus: SolverStatus | null;
    customIterations: number;
    progressPercent: number;
    sparklinePoints: string;
    onViewTimetable: () => void;
  }

  let {
    solverStatus,
    customIterations,
    progressPercent,
    sparklinePoints,
    onViewTimetable,
  }: Props = $props();
</script>

<div class="flex flex-col gap-5">
  <!-- Status Box -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900">
          Postęp optymalizacji w czasie rzeczywistym
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Stan: 
          <strong class="
            {solverStatus?.status === 'running' ? 'text-blue-600' :
             solverStatus?.status === 'completed' ? 'text-emerald-700' :
             solverStatus?.status === 'stopped' ? 'text-amber-700' :
             solverStatus?.status === 'error' ? 'text-rose-600' : 'text-slate-700'}">
            {solverStatus?.status === 'running' ? 'Przeszukiwanie przestrzeni rozwiązań...' :
             solverStatus?.status === 'completed' ? 'Zakończono pomyślnie' :
             solverStatus?.status === 'stopped' ? 'Zatrzymano przez użytkownika' :
             solverStatus?.status === 'error' ? 'Wystąpił błąd optymalizacji' : 'Oczekuje na uruchomienie'}
          </strong>
        </p>
      </div>

      {#if solverStatus?.status === "running"}
        <span class="w-3 h-3 rounded-full bg-blue-600 animate-ping"></span>
      {:else if solverStatus?.status === "error"}
        <span class="w-3 h-3 rounded-full bg-rose-600"></span>
      {/if}
    </div>

    {#if solverStatus?.status === "error"}
      <div class="p-3.5 bg-rose-50 border border-rose-200 rounded-xl text-xs text-rose-800 flex flex-col gap-1.5 shadow-2xs">
        <span class="font-bold text-rose-900 flex items-center gap-1.5">
          <Icon name="warning" size="xs" class="text-rose-600" />
          <span>Komunikat błędu silnika:</span>
        </span>
        <p class="font-mono text-[11px] bg-white/80 p-2 rounded border border-rose-200 text-rose-700 break-words">
          {solverStatus.error || "Wystąpił nieoczekiwany problem podczas optymalizacji."}
        </p>
      </div>
    {/if}

    <!-- Progress Bar -->
    <div class="flex flex-col gap-1.5">
      <div class="flex items-center justify-between text-xs font-semibold text-slate-600">
        <span>Wykonano: {solverStatus?.iteration || 0} / {solverStatus?.total_iterations || customIterations} prób</span>
        <span>{progressPercent}%</span>
      </div>
      <div class="w-full h-3 bg-slate-100 rounded-full overflow-hidden border border-slate-200">
        <div
          class="h-full bg-blue-600 transition-all duration-200 rounded-full"
          style={`width: ${progressPercent}%`}
        ></div>
      </div>
    </div>

    <!-- 3 KPIs -->
    <div class="grid grid-cols-3 gap-3 text-xs">
      <!-- Infeasibility -->
      <div class="p-3.5 rounded-xl border flex flex-col justify-between
        {solverStatus && solverStatus.infeasibility === 0 && solverStatus.status !== 'idle'
          ? 'bg-emerald-50 border-emerald-200 text-emerald-950'
          : 'bg-slate-50 border-slate-200 text-slate-800'}">
        <span class="text-[11px] font-semibold text-slate-500">Wymogi bezwzględne (Brak kolizji):</span>
        <div class="mt-2 flex items-baseline gap-1">
          {#if solverStatus?.status === "idle"}
            <span class="text-xl font-bold text-slate-400">—</span>
          {:else if solverStatus?.infeasibility === 0}
            <span class="text-xl font-bold text-emerald-700">0 konfliktów</span>
          {:else}
            <span class="text-xl font-bold text-rose-600">{solverStatus?.infeasibility}</span>
            <span class="text-rose-600 text-xs">naruszeń</span>
          {/if}
        </div>
        <span class="text-[10px] text-slate-400 mt-1">
          {solverStatus?.infeasibility === 0 ? 'Plan w 100% wykonalny' : 'Konieczna eliminacja kolizji'}
        </span>
      </div>

      <!-- Objective / Comfort -->
      <div class="p-3.5 bg-slate-50 rounded-xl border border-slate-200 flex flex-col justify-between">
        <span class="text-[11px] font-semibold text-slate-500">Wskaźnik wygody (Okienka):</span>
        <div class="mt-2 flex items-baseline gap-1">
          {#if solverStatus?.status === "idle"}
            <span class="text-xl font-bold text-slate-400">—</span>
          {:else}
            <span class="text-xl font-bold text-slate-900">{solverStatus?.objective}</span>
            <span class="text-slate-500 text-xs">pkt kary</span>
          {/if}
        </div>
        <span class="text-[10px] text-slate-400 mt-1">
          Im niższy, tym lepszy komfort planu
        </span>
      </div>

      <!-- Speed & Time -->
      <div class="p-3.5 bg-slate-50 rounded-xl border border-slate-200 flex flex-col justify-between">
        <span class="text-[11px] font-semibold text-slate-500">Wydajność solvera:</span>
        <div class="mt-2 flex items-baseline gap-1">
          <span class="text-xl font-bold text-slate-900">{solverStatus?.rate || 0}</span>
          <span class="text-slate-500 text-xs">prób/s</span>
        </div>
        <span class="text-[10px] text-slate-400 mt-1">
          Czas pracy: {solverStatus?.elapsed || 0} s
        </span>
      </div>
    </div>

    <!-- Convergence Sparkline Chart -->
    {#if sparklinePoints}
      <div class="flex flex-col gap-1.5 pt-3 border-t border-slate-100">
        <div class="flex items-center justify-between text-xs text-slate-500">
          <span class="font-semibold">Krzywa zbieżności algorytmu (Koszt w kolejnych próbach):</span>
          <span class="text-[11px] font-mono">Prób: {solverStatus?.history.length} punktów</span>
        </div>
        <div class="w-full h-24 bg-slate-50 border border-slate-200 rounded-xl p-2 flex items-center justify-center overflow-hidden">
          <svg viewBox="0 0 300 80" class="w-full h-full stroke-blue-600 fill-none" preserveAspectRatio="none">
            <polyline
              fill="none"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
              points={sparklinePoints}
            />
          </svg>
        </div>
      </div>
    {/if}
  </div>

  <!-- Result Banner when finished -->
  {#if solverStatus?.has_result}
    <div class="bg-emerald-50 border border-emerald-200/90 rounded-2xl p-6 shadow-xs flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
      <div class="flex items-center gap-3.5">
        <div class="w-10 h-10 rounded-xl bg-emerald-100 text-emerald-700 flex items-center justify-center shrink-0">
          <Icon name="task_alt" size="lg" />
        </div>
        <div>
          <h4 class="text-sm font-bold text-emerald-950">
            Plan lekcji został pomyślnie wygenerowany!
          </h4>
          <p class="text-xs text-emerald-800 mt-0.5">
            Wszystkie zajęcia zostały obsadzone w siatce tygodniowej. Wynik jest gotowy do przeglądania i wydruku.
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2 shrink-0">
        <a
          href={getDownloadSolutionUrl()}
          download={`${solverStatus.instance_id}_solution.xml`}
          class="px-3.5 py-2 bg-white hover:bg-slate-50 text-slate-700 text-xs font-semibold rounded-xl border border-emerald-300 transition-all cursor-pointer shadow-2xs flex items-center gap-1.5"
        >
          <Icon name="download" size="xs" class="text-slate-600" />
          <span>Pobierz XML rozwiązania</span>
        </a>
        <button
          type="button"
          onclick={onViewTimetable}
          class="px-4 py-2 bg-emerald-700 hover:bg-emerald-800 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
        >
          <Icon name="calendar_month" size="xs" class="text-white" />
          <span>Wyświetl plan lekcji</span>
          <Icon name="arrow_forward" size="xs" class="text-white" />
        </button>
      </div>
    </div>
  {/if}
</div>

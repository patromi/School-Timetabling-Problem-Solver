<script lang="ts">
  import { onMount, onDestroy } from "svelte";
  import type { InstanceListItem, SolverHistoryPoint, SolverStatus } from "../types";
  import {
    fetchInstances,
    getDownloadSolutionUrl,
    getSolverStatus,
    startSolver,
    stopSolver,
  } from "../api";

  interface Props {
    selectedInstanceId?: string | null;
    onViewTimetable: () => void;
  }

  let { selectedInstanceId = null, onViewTimetable }: Props = $props();

  let instances = $state<InstanceListItem[]>([]);
  let chosenInstanceId = $state<string>("");
  let solverStatus = $state<SolverStatus | null>(null);
  let isPolling = $state(false);
  let pollInterval: any = null;

  $effect(() => {
    if (selectedInstanceId && !chosenInstanceId) {
      chosenInstanceId = selectedInstanceId;
    }
  });

  // Preset profiles for non-technical users
  type Profile = "quick" | "standard" | "deep";
  let chosenProfile = $state<Profile>("standard");

  let customIterations = $state<number>(20000);
  let chosenSelector = $state<string>("ucb");
  let chosenAcceptance = $state<string>("lahc");
  let chosenSeed = $state<number>(42);
  let showAdvancedSettings = $state(false);

  let errorMessage = $state<string | null>(null);

  function getIterationsForProfile(p: Profile): number {
    if (p === "quick") return 5000;
    if (p === "deep") return 50000;
    return 20000;
  }

  $effect(() => {
    customIterations = getIterationsForProfile(chosenProfile);
  });

  async function loadSchoolList() {
    try {
      const data = await fetchInstances();
      instances = data;
      if (!chosenInstanceId && data.length > 0) {
        const preferred = data.find((i: InstanceListItem) => i.id === "LO_14_Poznan") || data.find((i: InstanceListItem) => i.id === "AU-BG-98") || data[0];
        chosenInstanceId = preferred.id;
      }
    } catch (err: any) {
      errorMessage = err.message || "Błąd wczytywania listy szkół";
    }
  }

  async function checkStatus() {
    try {
      const status = await getSolverStatus();
      solverStatus = status;

      if (status.status === "running") {
        if (!isPolling) startPolling();
      } else {
        if (isPolling) stopPolling();
      }
    } catch {
      // API boot
    }
  }

  function startPolling() {
    isPolling = true;
    if (pollInterval) clearInterval(pollInterval);
    pollInterval = setInterval(async () => {
      try {
        const status = await getSolverStatus();
        solverStatus = status;
        if (status.status !== "running") {
          stopPolling();
        }
      } catch {
        stopPolling();
      }
    }, 250);
  }

  function stopPolling() {
    isPolling = false;
    if (pollInterval) {
      clearInterval(pollInterval);
      pollInterval = null;
    }
  }

  async function handleStart() {
    if (!chosenInstanceId) {
      errorMessage = "Wybierz szkołę, dla której chcesz wygenerować plan.";
      return;
    }
    errorMessage = null;

    try {
      await startSolver({
        instance_id: chosenInstanceId,
        iterations: customIterations,
        selector: chosenSelector,
        acceptance: chosenAcceptance,
        seed: chosenSeed,
        history_length: 30,
      });
      await checkStatus();
      startPolling();
    } catch (err: any) {
      errorMessage = err.message || "Błąd podczas uruchamiania solvera.";
    }
  }

  async function handleStop() {
    try {
      await stopSolver();
      await checkStatus();
    } catch (err: any) {
      errorMessage = err.message || "Błąd podczas zatrzymywania solvera.";
    }
  }

  onMount(() => {
    loadSchoolList();
    checkStatus();
  });

  onDestroy(() => {
    stopPolling();
  });

  let progressPercent = $derived(
    solverStatus && solverStatus.total_iterations > 0
      ? Math.min(100, Math.round((solverStatus.iteration / solverStatus.total_iterations) * 100))
      : 0
  );

  let currentSchool = $derived(
    instances.find((i: InstanceListItem) => i.id === chosenInstanceId)
  );

  let sparklinePoints = $derived.by(() => {
    if (!solverStatus || !solverStatus.history || solverStatus.history.length < 2) return "";
    const hist = solverStatus.history;
    const maxCost = Math.max(...hist.map((p: SolverHistoryPoint) => p.infeasibility * 1000 + p.objective), 1);
    const minCost = Math.min(...hist.map((p: SolverHistoryPoint) => p.infeasibility * 1000 + p.objective), 0);
    const range = maxCost - minCost || 1;

    return hist
      .map((p: SolverHistoryPoint, idx: number) => {
        const x = (idx / (hist.length - 1)) * 300;
        const cost = p.infeasibility * 1000 + p.objective;
        const y = 80 - ((cost - minCost) / range) * 70;
        return `${x.toFixed(1)},${y.toFixed(1)}`;
      })
      .join(" ");
  });
</script>

<div class="p-6 max-w-5xl mx-auto flex flex-col gap-6 text-slate-800 antialiased">
  <!-- Header Card -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
    <div class="flex items-center gap-3.5">
      <div class="w-12 h-12 rounded-2xl bg-blue-50 text-blue-700 border border-blue-100 flex items-center justify-center font-bold text-xl shadow-2xs">
        ⚡
      </div>
      <div>
        <h2 class="text-lg font-bold text-slate-900 leading-tight">
          Generator Planu Lekcji (Silnik Optymalizacyjny)
        </h2>
        <p class="text-xs text-slate-500 mt-0.5">
          Automatyczne rozmieszczanie zajęć z eliminacją konfliktów i minimalizacją okienek
        </p>
      </div>
    </div>

    {#if solverStatus?.has_result}
      <button
        type="button"
        onclick={onViewTimetable}
        class="px-4 py-2 bg-emerald-600 hover:bg-emerald-700 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-2"
      >
        <span>📅 Zobacz gotowy plan</span>
        <span>→</span>
      </button>
    {/if}
  </div>

  {#if errorMessage}
    <div class="p-4 bg-rose-50 border border-rose-200 rounded-2xl text-xs text-rose-800 flex items-center justify-between shadow-2xs">
      <span>{errorMessage}</span>
      <button type="button" class="font-bold cursor-pointer" onclick={() => (errorMessage = null)}>✕</button>
    </div>
  {/if}

  <!-- Setup & Controls Grid -->
  <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
    <!-- Left Column: School & Mode Selection -->
    <div class="md:col-span-1 flex flex-col gap-5 bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs">
      <div>
        <label for="school-select" class="block text-xs font-bold text-slate-800 mb-1.5">
          1. Wybierz szkołę do ułożenia:
        </label>
        <select
          id="school-select"
          bind:value={chosenInstanceId}
          disabled={solverStatus?.status === "running"}
          class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-xl text-slate-900 font-medium focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 cursor-pointer"
        >
          {#each instances as inst}
            <option value={inst.id}>
              {inst.name || inst.id} ({inst.is_custom ? 'Własna' : 'Wzorcowa'})
            </option>
          {/each}
        </select>

        {#if currentSchool}
          <div class="mt-2 p-2.5 bg-slate-50 rounded-xl border border-slate-100 text-[11px] text-slate-500 flex flex-col gap-1">
            <span class="font-semibold text-slate-700">{currentSchool.name || currentSchool.id}</span>
            <span>📚 {currentSchool.events_count} lekcji • 👥 {currentSchool.resources_count} osób/sal</span>
          </div>
        {/if}
      </div>

      <!-- Optimization Profile -->
      <div>
        <span class="block text-xs font-bold text-slate-800 mb-2">
          2. Tryb optymalizacji:
        </span>
        <div class="flex flex-col gap-2">
          <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
            <input
              type="radio"
              name="profile"
              value="standard"
              bind:group={chosenProfile}
              disabled={solverStatus?.status === "running"}
              class="mt-0.5 accent-blue-600"
            />
            <div>
              <span class="text-xs font-bold text-slate-900">🚀 Standardowy (Zalecany)</span>
              <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
                20 000 prób optymalizacji (~3 sekundy). Znakomity balans jakości i szybkości.
              </p>
            </div>
          </label>

          <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
            <input
              type="radio"
              name="profile"
              value="quick"
              bind:group={chosenProfile}
              disabled={solverStatus?.status === "running"}
              class="mt-0.5 accent-blue-600"
            />
            <div>
              <span class="text-xs font-bold text-slate-900">⚡ Szybki test</span>
              <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
                5 000 prób (~1 sekunda). Do błyskawicznej weryfikacji poprawności siatki.
              </p>
            </div>
          </label>

          <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
            <input
              type="radio"
              name="profile"
              value="deep"
              bind:group={chosenProfile}
              disabled={solverStatus?.status === "running"}
              class="mt-0.5 accent-blue-600"
            />
            <div>
              <span class="text-xs font-bold text-slate-900">🎯 Głęboka optymalizacja</span>
              <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
                50 000 prób (~8 sekund). Maksymalny komfort planu i eliminacja trudnych okienek.
              </p>
            </div>
          </label>
        </div>
      </div>

      <!-- Advanced Toggle -->
      <div class="pt-2 border-t border-slate-100">
        <button
          type="button"
          onclick={() => (showAdvancedSettings = !showAdvancedSettings)}
          class="text-xs font-semibold text-slate-500 hover:text-slate-800 cursor-pointer flex items-center gap-1"
        >
          <span>{showAdvancedSettings ? 'Ukryj parametry zaawansowane' : 'Pokaż parametry zaawansowane'}</span>
          <span>{showAdvancedSettings ? '▲' : '▼'}</span>
        </button>

        {#if showAdvancedSettings}
          <div class="mt-3 flex flex-col gap-2.5 text-xs bg-slate-50 p-3 rounded-xl border border-slate-200">
            <div>
              <label for="custom-iterations-input" class="block text-[10px] font-mono text-slate-500 mb-0.5">Liczba iteracji:</label>
              <input
                id="custom-iterations-input"
                type="number"
                bind:value={customIterations}
                class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono"
              />
            </div>
            <div>
              <label for="chosen-selector-select" class="block text-[10px] font-mono text-slate-500 mb-0.5">Heurystyka wyboru (Selector):</label>
              <select id="chosen-selector-select" bind:value={chosenSelector} class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono">
                <option value="ucb">UCB (Multi-Armed Bandit)</option>
                <option value="epsilon-greedy">Epsilon-Greedy</option>
                <option value="random">Random Selector</option>
              </select>
            </div>
            <div>
              <label for="chosen-seed-input" class="block text-[10px] font-mono text-slate-500 mb-0.5">Ziarno losowe (Seed):</label>
              <input
                id="chosen-seed-input"
                type="number"
                bind:value={chosenSeed}
                class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono"
              />
            </div>
          </div>
        {/if}
      </div>

      <!-- Start / Stop Button -->
      <div class="mt-auto pt-4 border-t border-slate-100">
        {#if solverStatus?.status === "running"}
          <button
            type="button"
            onclick={handleStop}
            class="w-full py-3 bg-rose-600 hover:bg-rose-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center justify-center gap-2 animate-pulse"
          >
            <span>⏹ Zatrzymaj solver</span>
          </button>
        {:else}
          <button
            type="button"
            onclick={handleStart}
            class="w-full py-3 bg-blue-600 hover:bg-blue-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center justify-center gap-2"
          >
            <span>▶ Rozpocznij generowanie planu</span>
          </button>
        {/if}
      </div>
    </div>

    <!-- Right 2 Columns: Live Telemetry & Quality Dashboard -->
    <div class="md:col-span-2 flex flex-col gap-5">
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
                 solverStatus?.status === 'stopped' ? 'text-amber-700' : 'text-slate-700'}">
                {solverStatus?.status === 'running' ? 'Przeszukiwanie przestrzeni rozwiązań...' :
                 solverStatus?.status === 'completed' ? 'Zakończono pomyślnie' :
                 solverStatus?.status === 'stopped' ? 'Zatrzymano przez użytkownika' : 'Oczekuje na uruchomienie'}
              </strong>
            </p>
          </div>

          {#if solverStatus?.status === "running"}
            <span class="w-3 h-3 rounded-full bg-blue-600 animate-ping"></span>
          {/if}
        </div>

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
          <div class="flex items-center gap-3">
            <span class="text-3xl">🎉</span>
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
              class="px-3.5 py-2 bg-white hover:bg-slate-50 text-slate-700 text-xs font-semibold rounded-xl border border-emerald-300 transition-all cursor-pointer shadow-2xs"
            >
              📥 Pobierz XML rozwiązania
            </a>
            <button
              type="button"
              onclick={onViewTimetable}
              class="px-4 py-2 bg-emerald-700 hover:bg-emerald-800 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer"
            >
              📅 Wyświetl plan lekcji →
            </button>
          </div>
        </div>
      {/if}
    </div>
  </div>
</div>

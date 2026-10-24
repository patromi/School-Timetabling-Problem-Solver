<script lang="ts">
  import { onMount, onDestroy } from "svelte";
  import type { InstanceListItem, SolverHistoryPoint, SolverStatus } from "../types";
  import {
    fetchInstances,
    getSolverStatus,
    startSolver,
    stopSolver,
  } from "../api";
  import SolverConfigPanel, { type Profile } from "./solver/SolverConfigPanel.svelte";
  import SolverTelemetryDashboard from "./solver/SolverTelemetryDashboard.svelte";

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

  let chosenProfile = $state<Profile>("standard");
  let customIterations = $state<number>(20000);
  let chosenSelector = $state<string>("ucb");
  let chosenAcceptance = $state<string>("lahc");
  let chosenSeed = $state<number>(42);
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
    <div class="md:col-span-1">
      <SolverConfigPanel
        {instances}
        bind:chosenInstanceId
        bind:chosenProfile
        bind:customIterations
        bind:chosenSelector
        bind:chosenSeed
        {solverStatus}
        {currentSchool}
        onStart={handleStart}
        onStop={handleStop}
      />
    </div>

    <!-- Right 2 Columns: Live Telemetry & Quality Dashboard -->
    <div class="md:col-span-2">
      <SolverTelemetryDashboard
        {solverStatus}
        {customIterations}
        {progressPercent}
        {sparklinePoints}
        {onViewTimetable}
      />
    </div>
  </div>
</div>

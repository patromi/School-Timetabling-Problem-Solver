<script lang="ts">
  import { onMount } from "svelte";
  import type { TimetableCellEntry, TimetableResult } from "../types";
  import { getDownloadSolutionUrl, getSolverResult } from "../api";
  import TimetableGridTable from "./timetable/TimetableGridTable.svelte";
  import TimetableAuditSection from "./timetable/TimetableAuditSection.svelte";

  interface Props {
    onGoToSolver: () => void;
  }

  let { onGoToSolver }: Props = $props();

  let loading = $state(true);
  let error = $state<string | null>(null);
  let result = $state<TimetableResult | null>(null);

  // Perspective: "class" | "teacher" | "room"
  let perspective = $state<"class" | "teacher" | "room">("class");
  let selectedResourceId = $state<string>("");

  // Diagnostics filter
  let selectedViolationConstraint = $state<string | null>(null);

  async function loadResult() {
    loading = true;
    error = null;
    try {
      const data = await getSolverResult();
      result = data;

      // Select first available resource in grids
      const allResourceIds = Object.keys(data.grids);
      if (allResourceIds.length > 0 && !selectedResourceId) {
        // Try to pick a class first
        const foundClass = allResourceIds.find((id) => /^[0-9]+[a-z]?$/i.test(id) || id.includes("1A") || id.toLowerCase().includes("class"));
        selectedResourceId = foundClass || allResourceIds[0];
      }
    } catch (err: any) {
      error = err.message || "Brak dostępnego rozwiązania planu lekcji.";
      result = null;
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    loadResult();
  });

  // Resource categorization
  let resourcesList = $derived.by(() => {
    if (!result) return { classes: [], teachers: [], rooms: [] };
    const allIds = Object.keys(result.grids);

    const classes: string[] = [];
    const teachers: string[] = [];
    const rooms: string[] = [];

    for (const id of allIds) {
      const lower = id.toLowerCase();
      if (lower.startsWith("s") && (lower.includes("sala") || lower.includes("gym") || lower.includes("lab") || lower.includes("it") || lower.startsWith("s10") || lower.startsWith("s20"))) {
        rooms.push(id);
      } else if (/^[0-9]+[a-z]?$/i.test(id) || lower.includes("klas") || lower.includes("class") || lower.startsWith("c_")) {
        classes.push(id);
      } else {
        teachers.push(id);
      }
    }

    return { classes, teachers, rooms };
  });

  // Switch perspective and select default
  function handlePerspectiveChange(newP: "class" | "teacher" | "room") {
    perspective = newP;
    const list = resourcesList;
    if (newP === "class" && list.classes.length > 0) {
      selectedResourceId = list.classes[0];
    } else if (newP === "teacher" && list.teachers.length > 0) {
      selectedResourceId = list.teachers[0];
    } else if (newP === "room" && list.rooms.length > 0) {
      selectedResourceId = list.rooms[0];
    }
  }

  // Active grid matrix for selected resource
  let activeGrid = $derived.by(() => {
    if (!result || !selectedResourceId || !result.grids[selectedResourceId]) {
      return {} as Record<string, Record<number, TimetableCellEntry[]>>;
    }

    const matrix: Record<string, Record<number, TimetableCellEntry[]>> = {};
    for (const d of result.days) {
      matrix[d.id] = {};
      for (let p = 0; p < d.periods.length; p++) {
        matrix[d.id][p] = [];
      }
    }

    const entries = result.grids[selectedResourceId] || [];
    for (const e of entries) {
      if (!matrix[e.day_id]) matrix[e.day_id] = {};
      if (!matrix[e.day_id][e.period_idx]) matrix[e.day_id][e.period_idx] = [];
      matrix[e.day_id][e.period_idx].push(e);
    }

    return matrix;
  });

  // Max periods across all days
  let maxPeriodsCount = $derived(
    result ? Math.max(...result.days.map((d) => d.periods.length), 0) : 0
  );

  function handlePrint() {
    window.print();
  }
</script>

<div class="p-6 max-w-7xl mx-auto flex flex-col gap-6 text-slate-800 antialiased">
  {#if loading}
    <div class="bg-white border border-slate-200/90 rounded-2xl p-16 text-center shadow-xs flex flex-col items-center justify-center gap-3">
      <div class="w-8 h-8 border-3 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
      <span class="text-xs text-slate-500 font-medium">Wczytywanie gotowego planu lekcji...</span>
    </div>
  {:else if error || !result}
    <!-- Empty State -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-12 text-center shadow-xs flex flex-col items-center justify-center gap-4 max-w-xl mx-auto">
      <div class="w-16 h-16 rounded-3xl bg-blue-50 text-blue-600 flex items-center justify-center text-3xl shadow-2xs">
        📅
      </div>
      <div>
        <h3 class="text-base font-bold text-slate-900">Brak gotowego planu do wyświetlenia</h3>
        <p class="text-xs text-slate-500 mt-1 leading-relaxed">
          Aby zobaczyć siatkę zajęć, przejdź do generatora planu i uruchom solver dla wybranej szkoły.
        </p>
      </div>
      <button
        type="button"
        onclick={onGoToSolver}
        class="mt-2 px-5 py-2.5 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-2"
      >
        <span>⚡ Uruchom generator planu</span>
        <span>→</span>
      </button>
    </div>
  {:else}
    <!-- Top Bar with Quality KPIs & Actions -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
      <div>
        <div class="flex items-center gap-2">
          <h2 class="text-lg font-bold text-slate-900 tracking-tight">
            Tygodniowy Plan Zajęć Szkolnych
          </h2>
          <span class="px-2.5 py-0.5 rounded-full text-xs font-semibold
            {result.infeasibility === 0 ? 'bg-emerald-50 text-emerald-800 border border-emerald-200' : 'bg-rose-50 text-rose-800 border border-rose-200'}">
            {result.infeasibility === 0 ? '✅ 0 konfliktów (Plan w 100% poprawny)' : `⚠️ ${result.infeasibility} konfliktów`}
          </span>
        </div>
        <p class="text-xs text-slate-500 mt-1">
          Wskaźnik ergonomii i wygody planu: <strong>{result.objective} pkt kary</strong> • {result.days.length} dni w tygodniu • {maxPeriodsCount} lekcji dziennie
        </p>
      </div>

      <!-- Action buttons -->
      <div class="flex items-center gap-2 shrink-0">
        <button
          type="button"
          onclick={handlePrint}
          class="px-3.5 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-xl border border-slate-300/80 transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs"
        >
          <span>🖨️ Drukuj plan</span>
        </button>
        <a
          href={getDownloadSolutionUrl()}
          download="plan_lekcji_rozwiazanie.xml"
          class="px-3.5 py-2 bg-white hover:bg-slate-50 text-slate-700 text-xs font-semibold rounded-xl border border-slate-300/80 transition-all cursor-pointer flex items-center gap-1.5 shadow-2xs"
        >
          <span>📥 Pobierz plik XML</span>
        </a>
      </div>
    </div>

    <!-- Perspective & Selector Bar -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-4 shadow-xs flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-4">
      <!-- Perspective tabs -->
      <div class="flex items-center gap-1 bg-slate-100 p-1 rounded-xl text-xs font-semibold text-slate-600">
        <button
          type="button"
          onclick={() => handlePerspectiveChange("class")}
          class={`px-3.5 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            perspective === "class" ? "bg-white text-slate-900 shadow-xs font-bold" : "hover:text-slate-900"
          }`}
        >
          <span>👥 Plan dla Klasy</span>
          <span class="text-[10px] bg-slate-200 px-1.5 py-0.2 rounded-full">{resourcesList.classes.length}</span>
        </button>

        <button
          type="button"
          onclick={() => handlePerspectiveChange("teacher")}
          class={`px-3.5 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            perspective === "teacher" ? "bg-white text-slate-900 shadow-xs font-bold" : "hover:text-slate-900"
          }`}
        >
          <span>👨‍🏫 Plan dla Nauczyciela</span>
          <span class="text-[10px] bg-slate-200 px-1.5 py-0.2 rounded-full">{resourcesList.teachers.length}</span>
        </button>

        <button
          type="button"
          onclick={() => handlePerspectiveChange("room")}
          class={`px-3.5 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            perspective === "room" ? "bg-white text-slate-900 shadow-xs font-bold" : "hover:text-slate-900"
          }`}
        >
          <span>🚪 Obłożenie Sali</span>
          <span class="text-[10px] bg-slate-200 px-1.5 py-0.2 rounded-full">{resourcesList.rooms.length}</span>
        </button>
      </div>

      <!-- Resource Selector Dropdown -->
      <div class="flex items-center gap-2">
        <span class="text-xs font-semibold text-slate-500">Wybrany podmiot:</span>
        <select
          bind:value={selectedResourceId}
          class="px-3.5 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-xl text-slate-900 font-bold focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 cursor-pointer min-w-[200px]"
        >
          {#if perspective === "class"}
            {#each resourcesList.classes as c}
              <option value={c}>Oddział {c}</option>
            {/each}
          {:else if perspective === "teacher"}
            {#each resourcesList.teachers as t}
              <option value={t}>{t}</option>
            {/each}
          {:else}
            {#each resourcesList.rooms as r}
              <option value={r}>{r}</option>
            {/each}
          {/if}
        </select>
      </div>
    </div>

    <!-- Timetable Grid Matrix -->
    <TimetableGridTable
      days={result.days}
      {maxPeriodsCount}
      {activeGrid}
      {selectedViolationConstraint}
      violationMap={result.violation_map}
    />

    <!-- Constraint Audit & Diagnostics Section -->
    <TimetableAuditSection
      scores={result.scores}
      {selectedViolationConstraint}
      onToggleViolation={(cId) => (selectedViolationConstraint = selectedViolationConstraint === cId ? null : cId)}
      onClearViolation={() => (selectedViolationConstraint = null)}
    />
  {/if}
</div>

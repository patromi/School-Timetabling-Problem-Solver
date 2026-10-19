<script lang="ts">
  import { onMount } from "svelte";
  import type { TimetableCellEntry, TimetableDay, TimetableResult } from "../types";
  import { getDownloadSolutionUrl, getSolverResult } from "../api";
  import { CONSTRAINT_FRIENDLY_MAP } from "../school_helpers";

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

  // Subject color palettes (tasteful pastel colors)
  function getSubjectColor(subjectName: string): { bg: string; text: string; border: string } {
    const s = (subjectName || "").toLowerCase();
    if (s.includes("mat")) {
      return { bg: "bg-blue-50/90", text: "text-blue-900", border: "border-blue-200" };
    }
    if (s.includes("polsk") || s.includes("hist")) {
      return { bg: "bg-amber-50/90", text: "text-amber-950", border: "border-amber-200" };
    }
    if (s.includes("angiel") || s.includes("niemiec") || s.includes("język")) {
      return { bg: "bg-purple-50/90", text: "text-purple-950", border: "border-purple-200" };
    }
    if (s.includes("fizyk") || s.includes("chem") || s.includes("biol")) {
      return { bg: "bg-teal-50/90", text: "text-teal-950", border: "border-teal-200" };
    }
    if (s.includes("wf") || s.includes("fizycz")) {
      return { bg: "bg-emerald-50/90", text: "text-emerald-950", border: "border-emerald-200" };
    }
    if (s.includes("inform")) {
      return { bg: "bg-cyan-50/90", text: "text-cyan-950", border: "border-cyan-200" };
    }
    return { bg: "bg-slate-50", text: "text-slate-900", border: "border-slate-200" };
  }

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
    <div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden print:border-none print:shadow-none">
      <div class="overflow-x-auto">
        <table class="w-full border-collapse text-left">
          <thead>
            <tr class="bg-slate-50 border-b border-slate-200 text-xs text-slate-700 font-bold">
              <th class="p-3 w-28 text-center border-r border-slate-200/80">Godzina</th>
              {#each result.days as day}
                <th class="p-3 text-center border-r border-slate-200/80 last:border-r-0">
                  <div class="font-bold text-slate-900">{day.name || day.id}</div>
                  <div class="text-[10px] text-slate-400 font-normal">{day.periods.length} lekcji</div>
                </th>
              {/each}
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-100">
            {#each Array(maxPeriodsCount) as _, periodIdx}
              <tr class="hover:bg-slate-50/40 transition-colors">
                <!-- Period number column -->
                <td class="p-2.5 bg-slate-50/60 border-r border-slate-200 text-center font-mono text-xs">
                  <span class="font-bold text-slate-800">Lekcja {periodIdx + 1}</span>
                  <div class="text-[10px] text-slate-400 mt-0.5">
                    {8 + Math.floor(periodIdx * 50 / 60)}:{(periodIdx * 50 % 60).toString().padStart(2, '0')}
                  </div>
                </td>

                <!-- Days columns -->
                {#each result.days as day}
                  {@const cellEntries = activeGrid[day.id]?.[periodIdx] || []}
                  <td class="p-2 border-r border-slate-200/80 last:border-r-0 align-top h-20 min-w-[170px]">
                    {#if cellEntries.length === 0}
                      <!-- Empty slot -->
                      <div class="h-full flex items-center justify-center text-slate-300 text-xs select-none">
                        —
                      </div>
                    {:else}
                      <!-- Lesson Cards -->
                      <div class="flex flex-col gap-1.5 h-full">
                        {#each cellEntries as entry}
                          {@const colors = getSubjectColor(entry.event_name)}
                          {@const isViolation = selectedViolationConstraint && result.violation_map[selectedViolationConstraint]?.includes(entry.event_ref)}
                          <div class="p-2 rounded-xl border text-xs flex flex-col justify-between transition-all shadow-2xs
                            {entry.is_clash
                              ? 'bg-rose-50 border-rose-300 text-rose-950 animate-pulse'
                              : isViolation
                              ? 'bg-amber-50 border-amber-400 ring-2 ring-amber-400/50 text-amber-950'
                              : `${colors.bg} ${colors.border} ${colors.text}`}">
                            
                            <!-- Subject & Duration -->
                            <div class="flex items-start justify-between gap-1">
                              <span class="font-bold leading-snug">
                                {entry.event_name}
                              </span>
                              {#if entry.duration > 1}
                                <span class="text-[9px] px-1.5 py-0.2 bg-white/80 rounded font-medium shrink-0 border border-current">
                                  Blok {entry.duration}h
                                </span>
                              {/if}
                            </div>

                            <!-- Clash Warning -->
                            {#if entry.is_clash}
                              <div class="text-[10px] font-bold text-rose-700 flex items-center gap-1 mt-1">
                                <span>⚠️ Kolizja terminów!</span>
                              </div>
                            {/if}

                            <!-- Other resources (Teacher / Room / Class) -->
                            <div class="text-[11px] text-slate-600 mt-1 flex flex-wrap items-center gap-1">
                              {#each entry.other_resources as other}
                                <span class="bg-white/80 px-1.5 py-0.5 rounded text-[10px] font-medium border border-slate-200/60">
                                  {other}
                                </span>
                              {/each}
                            </div>
                          </div>
                        {/each}
                      </div>
                    {/if}
                  </td>
                {/each}
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>

    <!-- Constraint Audit & Diagnostics Section -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
      <div class="flex items-center justify-between">
        <div>
          <h3 class="text-sm font-bold text-slate-900">
            Audyt spełnienia reguł szkolnych (Diagnostyka ograniczeń)
          </h3>
          <p class="text-xs text-slate-500 mt-0.5">
            Zweryfikowano {result.scores.length} reguł organizacyjnych dla tej szkoły:
          </p>
        </div>
        {#if selectedViolationConstraint}
          <button
            type="button"
            onclick={() => (selectedViolationConstraint = null)}
            class="text-xs font-semibold text-blue-600 hover:text-blue-700 cursor-pointer"
          >
            ✕ Wyłącz podświetlenie reguły
          </button>
        {/if}
      </div>

      <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
        {#each result.scores as s}
          {@const friendly = CONSTRAINT_FRIENDLY_MAP[s.id] || { title: s.name || s.id, description: "Zasada organizacji planu." }}
          {@const hasViolations = s.cost > 0}
          <button
            type="button"
            onclick={() => {
              if (hasViolations) {
                selectedViolationConstraint = selectedViolationConstraint === s.id ? null : s.id;
              }
            }}
            disabled={!hasViolations}
            class={`p-3.5 rounded-xl border text-left transition-all ${
              hasViolations
                ? selectedViolationConstraint === s.id
                  ? 'bg-amber-50 border-amber-400 ring-2 ring-amber-400/40 cursor-pointer'
                  : 'bg-slate-50/90 border-slate-200 hover:border-amber-300 cursor-pointer'
                : 'bg-slate-50/50 border-slate-100 cursor-default opacity-80'
            }`}
          >
            <div class="flex items-start justify-between gap-2">
              <span class="text-xs font-bold text-slate-900 leading-snug">
                {friendly.title}
              </span>
              <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full shrink-0
                {hasViolations ? 'bg-amber-100 text-amber-800' : 'bg-emerald-100 text-emerald-800'}">
                {hasViolations ? `Kara: ${s.cost} pkt` : '✓ 100%'}
              </span>
            </div>
            <p class="text-[11px] text-slate-500 mt-1 leading-snug">
              {friendly.description}
            </p>
            {#if hasViolations}
              <div class="mt-2 text-[10px] font-semibold text-amber-800 flex items-center gap-1">
                <span>🔍 Kliknij, aby podświetlić w siatce</span>
              </div>
            {/if}
          </button>
        {/each}
      </div>
    </div>
  {/if}
</div>

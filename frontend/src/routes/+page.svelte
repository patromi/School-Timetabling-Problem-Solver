<!-- frontend/src/routes/+page.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail, InstanceListItem } from "#lib";
  import { fetchInstanceDetails, fetchInstances } from "#lib";
  import InstanceStudio from "../lib/components/editor/studio/InstanceStudio.svelte";
  import SolverView from "../lib/components/SolverView.svelte";
  import TimetableView from "../lib/components/TimetableView.svelte";

  let instances = $state<InstanceListItem[]>([]);
  let selectedInstanceId = $state<string | null>(null);
  let instanceDetail = $state<InstanceDetail | null>(null);
  let loadingList = $state(true);
  let loadingDetail = $state(false);
  let errorMessage = $state<string | null>(null);

  // 3-Step Main Pipeline: "studio" | "solver" | "timetable"
  let mainView = $state<"studio" | "solver" | "timetable">("studio");

  async function loadInstancesList() {
    loadingList = true;
    errorMessage = null;
    try {
      const data = await fetchInstances();
      instances = data;
      if (data.length > 0 && !selectedInstanceId) {
        // Prefer custom LO_14_Poznan if available, otherwise AU-BG-98 or first
        const defaultInst =
          data.find((i: InstanceListItem) => i.id === "LO_14_Poznan") ||
          data.find((i: InstanceListItem) => i.id === "AU-BG-98") ||
          data[0];
        selectInstance(defaultInst.id);
      }
    } catch (err: any) {
      errorMessage = err.message || "Błąd wczytywania listy szkół";
    } finally {
      loadingList = false;
    }
  }

  async function selectInstance(id: string) {
    selectedInstanceId = id;
    loadingDetail = true;
    errorMessage = null;
    try {
      const detail = await fetchInstanceDetails(id);
      instanceDetail = detail;
    } catch (err: any) {
      errorMessage = err.message || `Błąd wczytywania szczegółów szkoły ${id}`;
      instanceDetail = null;
    } finally {
      loadingDetail = false;
    }
  }

  function handleUploaded(newIds: string[]) {
    loadInstancesList().then(() => {
      if (newIds.length > 0) {
        selectInstance(newIds[0]);
      }
    });
  }

  function handleNewSchool() {
    selectedInstanceId = null;
    instanceDetail = null;
    mainView = "studio";
  }

  function handleSolveCurrentSchool(schoolId?: string) {
    if (schoolId) {
      selectedInstanceId = schoolId;
    }
    mainView = "solver";
  }

  function handleSchoolSaved(savedId: string) {
    loadInstancesList().then(() => {
      selectInstance(savedId);
    });
  }

  onMount(() => {
    loadInstancesList();
    if (typeof window !== "undefined") {
      const urlParams = new URLSearchParams(window.location.search);
      const v = urlParams.get("view");
      if (v === "solver" || v === "timetable" || v === "studio") {
        mainView = v as any;
      } else if (v === "editor" || v === "explorer") {
        mainView = "studio";
      }
    }
  });
</script>

<svelte:head>
  <title>
    {mainView === "studio"
      ? instanceDetail?.name
        ? `${instanceDetail.name} — Pulpit Szkoły`
        : "Pulpit Szkoły — Studio Zasobów"
      : mainView === "solver"
      ? "Generator Planu (Solver) — Plan Lekcji"
      : "Gotowy Plan Lekcji — Plan Lekcji"}
  </title>
</svelte:head>

<div class="h-screen w-screen overflow-hidden flex flex-col bg-slate-50 font-sans text-slate-800 antialiased">
  <!-- Top Application Bar (Workflow Steps) -->
  <header class="h-14 border-b border-slate-200/90 bg-white px-6 flex items-center justify-between shrink-0 select-none shadow-2xs">
    <div class="flex items-center gap-4">
      <div class="flex items-center gap-3">
        <div class="w-8 h-8 rounded-lg bg-blue-600 text-white flex items-center justify-center font-bold text-base shadow-xs">
          📅
        </div>
        <div>
          <h1 class="text-sm font-bold text-slate-900 leading-none">
            Harmonogram Szkolny
          </h1>
          <p class="text-[11px] text-slate-400 mt-0.5">
            System optymalizacji i układania planów lekcji (XHSTT Solver)
          </p>
        </div>
      </div>

      <span class="text-slate-300">/</span>

      <!-- 3 Clear Workflow Steps -->
      <div class="flex items-center gap-1 bg-slate-100 p-0.5 rounded-lg text-xs font-semibold text-slate-600">
        <button
          type="button"
          onclick={() => (mainView = "studio")}
          class={`px-3 py-1.5 rounded-md transition-all cursor-pointer flex items-center gap-1.5 ${
            mainView === "studio"
              ? "bg-white text-blue-700 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>🏫 1. Szkoła i Dane</span>
        </button>

        <button
          type="button"
          onclick={() => (mainView = "solver")}
          class={`px-3 py-1.5 rounded-md transition-all cursor-pointer flex items-center gap-1.5 ${
            mainView === "solver"
              ? "bg-white text-blue-700 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>⚡ 2. Generator Solver</span>
        </button>

        <button
          type="button"
          onclick={() => (mainView = "timetable")}
          class={`px-3 py-1.5 rounded-md transition-all cursor-pointer flex items-center gap-1.5 ${
            mainView === "timetable"
              ? "bg-white text-emerald-700 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>📅 3. Gotowy Plan Lekcji</span>
        </button>
      </div>
    </div>

    <!-- Status -->
    <div class="flex items-center gap-2 text-xs text-slate-500 font-medium">
      <span class="w-2 h-2 rounded-full bg-emerald-500"></span>
      <span>Silnik solvera aktywny (:8000)</span>
    </div>
  </header>

  <!-- Main Pipeline Content -->
  {#if mainView === "studio"}
    <div class="flex-1 overflow-hidden">
      {#if loadingDetail && !instanceDetail}
        <div class="h-full flex flex-col items-center justify-center gap-3 text-slate-400 text-sm">
          <div class="w-7 h-7 border-2 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
          <span>Wczytywanie arkusza organizacyjnego szkoły...</span>
        </div>
      {:else}
        <InstanceStudio
          initialInstance={instanceDetail}
          {instances}
          {selectedInstanceId}
          {loadingList}
          onSelectInstance={selectInstance}
          onUploaded={handleUploaded}
          onNewSchool={handleNewSchool}
          onSaved={handleSchoolSaved}
          onSolve={handleSolveCurrentSchool}
        />
      {/if}
    </div>
  {:else if mainView === "solver"}
    <div class="flex-1 overflow-y-auto">
      <SolverView
        selectedInstanceId={selectedInstanceId}
        onViewTimetable={() => (mainView = "timetable")}
      />
    </div>
  {:else if mainView === "timetable"}
    <div class="flex-1 overflow-y-auto">
      <TimetableView
        onGoToSolver={() => (mainView = "solver")}
      />
    </div>
  {/if}
</div>

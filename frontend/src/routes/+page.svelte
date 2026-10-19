<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail, InstanceListItem } from "#lib";
  import { fetchInstanceDetails, fetchInstances } from "#lib";
  import InstanceSidebar from "../lib/components/InstanceSidebar.svelte";
  import InstanceHeader from "../lib/components/InstanceHeader.svelte";
  import OverviewTab from "../lib/components/OverviewTab.svelte";
  import ResourcesTab from "../lib/components/ResourcesTab.svelte";
  import EventsTab from "../lib/components/EventsTab.svelte";
  import ConstraintsTab from "../lib/components/ConstraintsTab.svelte";
  import TechnicalTab from "../lib/components/TechnicalTab.svelte";
  import InstanceEditor from "../lib/components/editor/InstanceEditor.svelte";

  let instances = $state<InstanceListItem[]>([]);
  let selectedInstanceId = $state<string | null>(null);
  let instanceDetail = $state<InstanceDetail | null>(null);
  let loadingList = $state(true);
  let loadingDetail = $state(false);
  let errorMessage = $state<string | null>(null);
  let activeTab = $state<string>("overview");
  let selectedResourceFilter = $state<string>("");

  // Main View: "explorer" | "editor"
  let mainView = $state<"explorer" | "editor">("explorer");
  let editorInitialInstance = $state<InstanceDetail | null>(null);

  async function loadInstancesList() {
    loadingList = true;
    errorMessage = null;
    try {
      const data = await fetchInstances();
      instances = data;
      if (data.length > 0 && !selectedInstanceId) {
        // Prefer AU-BG-98 or first
        const defaultInst = data.find((i: InstanceListItem) => i.id === "AU-BG-98") || data[0];
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
    selectedResourceFilter = "";
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

  function handleResourceSelectedForEvents(resourceId: string) {
    selectedResourceFilter = resourceId;
    activeTab = "events";
  }

  function handleNewSchool() {
    editorInitialInstance = null;
    mainView = "editor";
  }

  function handleEditCurrentSchool() {
    editorInitialInstance = instanceDetail;
    mainView = "editor";
  }

  function handleSchoolSaved(savedId: string) {
    loadInstancesList().then(() => {
      selectInstance(savedId);
      mainView = "explorer";
    });
  }

  function handleCancelEditor() {
    mainView = "explorer";
  }

  onMount(() => {
    loadInstancesList();
  });
</script>

<svelte:head>
  <title>
    {mainView === "editor"
      ? "Kreator Szkoły — Plan Lekcji"
      : instanceDetail
        ? `${instanceDetail.name || instanceDetail.id} — Plan Lekcji`
        : "Eksplorator Planów Lekcji"}
  </title>
</svelte:head>

<div class="h-screen w-screen overflow-hidden flex flex-col bg-slate-50 font-sans text-slate-800 antialiased">
  <!-- Top Navigation Bar (Clean, Dignified, Human-friendly) -->
  <header class="h-14 border-b border-slate-200/90 bg-white px-6 flex items-center justify-between shrink-0 select-none shadow-2xs">
    <div class="flex items-center gap-4">
      <div class="flex items-center gap-3">
        <div class="w-8 h-8 rounded-lg bg-blue-600 text-white flex items-center justify-center font-bold text-base shadow-xs">
          📅
        </div>
        <div>
          <h1 class="text-sm font-bold text-slate-900 leading-none">
            Eksplorator Planów Lekcji
          </h1>
          <p class="text-[11px] text-slate-400 mt-0.5">
            System analizy i optymalizacji tygodniowych planów zajęć szkolnych
          </p>
        </div>
      </div>

      <span class="text-slate-300">/</span>

      <!-- View Switcher -->
      <div class="flex items-center gap-1 bg-slate-100 p-0.5 rounded-lg text-xs font-semibold text-slate-600">
        <button
          type="button"
          onclick={() => (mainView = "explorer")}
          class={`px-3 py-1.5 rounded-md transition-all cursor-pointer flex items-center gap-1.5 ${
            mainView === "explorer"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>🔍 Przeglądarka szkół</span>
        </button>

        <button
          type="button"
          onclick={handleNewSchool}
          class={`px-3 py-1.5 rounded-md transition-all cursor-pointer flex items-center gap-1.5 ${
            mainView === "editor"
              ? "bg-white text-blue-600 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>✏️ Kreator / Edytor</span>
        </button>
      </div>
    </div>

    <!-- Status -->
    <div class="flex items-center gap-2 text-xs text-slate-500 font-medium">
      <span class="w-2 h-2 rounded-full bg-emerald-500"></span>
      <span>Silnik solvera gotowy (:8000)</span>
    </div>
  </header>

  <!-- Main Content Layout -->
  {#if mainView === "editor"}
    <div class="flex-1 overflow-hidden">
      <InstanceEditor
        initialInstance={editorInitialInstance}
        onSaved={handleSchoolSaved}
        onCancel={handleCancelEditor}
      />
    </div>
  {:else}
    <div class="flex-1 flex overflow-hidden">
      <!-- Left Sidebar: Instance Selection & Upload -->
      <InstanceSidebar
        {instances}
        selectedId={selectedInstanceId}
        loading={loadingList}
        onSelect={selectInstance}
        onUploaded={handleUploaded}
        onNewInstance={handleNewSchool}
      />

      <!-- Right Workspace -->
      <main class="flex-1 flex flex-col overflow-hidden bg-slate-50">
        {#if loadingDetail}
          <div class="flex-1 flex flex-col items-center justify-center gap-3 text-slate-400 text-sm">
            <div class="w-6 h-6 border-2 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
            <span>Wczytywanie arkusza organizacyjnego...</span>
          </div>
        {:else if errorMessage}
          <div class="flex-1 flex flex-col items-center justify-center p-8 text-center">
            <div class="bg-rose-50 border border-rose-200 p-6 rounded-2xl max-w-md flex flex-col gap-3 text-xs shadow-xs">
              <span class="text-rose-700 font-bold text-sm">Wystąpił problem z wczytaniem danych</span>
              <p class="text-slate-600">{errorMessage}</p>
              <button
                type="button"
                onclick={() => selectedInstanceId && selectInstance(selectedInstanceId)}
                class="mt-2 px-4 py-2 bg-rose-600 hover:bg-rose-700 text-white font-semibold rounded-lg transition-all cursor-pointer"
              >
                Spróbuj ponownie
              </button>
            </div>
          </div>
        {:else if instanceDetail}
          <!-- Instance Header with KPIs & Clean Tabs -->
          <InstanceHeader
            instance={instanceDetail}
            {activeTab}
            onTabChange={(tab) => (activeTab = tab)}
            onEdit={handleEditCurrentSchool}
          />

          <!-- Tab Content Body (Scrollable) -->
          <div class="flex-1 overflow-y-auto">
            {#if activeTab === "overview"}
              <OverviewTab
                instance={instanceDetail}
                onGoToTab={(tab) => (activeTab = tab)}
              />
            {:else if activeTab === "resources"}
              <ResourcesTab
                instance={instanceDetail}
                onSelectResourceForEvents={handleResourceSelectedForEvents}
              />
            {:else if activeTab === "events"}
              <EventsTab
                instance={instanceDetail}
                {selectedResourceFilter}
              />
            {:else if activeTab === "constraints"}
              <ConstraintsTab instance={instanceDetail} />
            {:else if activeTab === "technical"}
              <TechnicalTab instance={instanceDetail} />
            {/if}
          </div>
        {:else}
          <div class="flex-1 flex items-center justify-center text-slate-400 text-sm">
            Wybierz szkołę z listy po lewej stronie, aby wyświetlić jej arkusz organizacyjny.
          </div>
        {/if}
      </main>
    </div>
  {/if}
</div>

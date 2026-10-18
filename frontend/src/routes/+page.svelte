<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail, InstanceListItem } from "#lib";
  import { fetchInstanceDetails, fetchInstances } from "#lib";
  import InstanceSidebar from "../lib/components/InstanceSidebar.svelte";
  import InstanceHeader from "../lib/components/InstanceHeader.svelte";
  import OverviewTab from "../lib/components/OverviewTab.svelte";
  import TimesTab from "../lib/components/TimesTab.svelte";
  import ResourcesTab from "../lib/components/ResourcesTab.svelte";
  import EventsTab from "../lib/components/EventsTab.svelte";
  import ConstraintsTab from "../lib/components/ConstraintsTab.svelte";
  import GroupsTab from "../lib/components/GroupsTab.svelte";

  let instances = $state<InstanceListItem[]>([]);
  let selectedInstanceId = $state<string | null>(null);
  let instanceDetail = $state<InstanceDetail | null>(null);
  let loadingList = $state(true);
  let loadingDetail = $state(false);
  let errorMessage = $state<string | null>(null);
  let activeTab = $state<string>("overview");
  let selectedResourceFilter = $state<string>("");

  async function loadInstancesList() {
    loadingList = true;
    errorMessage = null;
    try {
      const data = await fetchInstances();
      instances = data;
      if (data.length > 0 && !selectedInstanceId) {
        // Prefer AU-BG-98 if present, otherwise first instance
        const defaultInst = data.find((i: InstanceListItem) => i.id === "AU-BG-98") || data[0];
        selectInstance(defaultInst.id);
      }
    } catch (err: any) {
      errorMessage = err.message || "Błąd wczytywania listy instancji";
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
      errorMessage = err.message || `Błąd wczytywania instancji ${id}`;
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

  onMount(() => {
    loadInstancesList();
  });
</script>

<svelte:head>
  <title>
    {instanceDetail ? `${instanceDetail.id} — XHSTT Solver` : "XHSTT Solver Studio"}
  </title>
</svelte:head>

<div class="h-screen w-screen overflow-hidden flex flex-col bg-zinc-950 font-sans text-zinc-100">
  <!-- Top Navigation Bar -->
  <header class="h-12 border-b border-zinc-800 bg-zinc-900/90 px-4 flex items-center justify-between shrink-0 select-none">
    <div class="flex items-center gap-3">
      <div class="flex items-center gap-2">
        <span class="w-3 h-3 rounded-full bg-amber-500 animate-pulse"></span>
        <span class="font-mono font-bold text-sm tracking-tight text-white">XHSTT Solver Studio</span>
      </div>
      <span class="text-zinc-600">/</span>
      <div class="flex items-center gap-1 font-mono text-xs">
        <span class="px-2 py-0.5 bg-zinc-800 text-amber-400 font-semibold rounded border border-zinc-700">
          Eksplorator Instancji
        </span>
      </div>
    </div>

    <div class="flex items-center gap-4 text-xs font-mono">
      <div class="flex items-center gap-2 text-zinc-400">
        <span class="w-2 h-2 rounded-full bg-emerald-500"></span>
        <span>Backend API aktywny (:8000)</span>
      </div>
    </div>
  </header>

  <!-- Main Content Layout -->
  <div class="flex-1 flex overflow-hidden">
    <!-- Left Sidebar: Instance Selection & Upload -->
    <InstanceSidebar
      {instances}
      selectedId={selectedInstanceId}
      loading={loadingList}
      onSelect={selectInstance}
      onUploaded={handleUploaded}
    />

    <!-- Right Workspace -->
    <main class="flex-1 flex flex-col overflow-hidden bg-zinc-950">
      {#if loadingDetail}
        <div class="flex-1 flex flex-col items-center justify-center gap-3 text-zinc-400 font-mono text-sm">
          <div class="w-6 h-6 border-2 border-amber-500 border-t-transparent rounded-full animate-spin"></div>
          <span>Wczytywanie szczegółów instancji {selectedInstanceId}...</span>
        </div>
      {:else if errorMessage}
        <div class="flex-1 flex flex-col items-center justify-center p-8 text-center">
          <div class="bg-rose-950/40 border border-rose-800 p-6 rounded-lg max-w-md flex flex-col gap-3 font-mono text-xs">
            <span class="text-rose-400 font-bold text-sm">Wystąpił błąd</span>
            <p class="text-zinc-300">{errorMessage}</p>
            <button
              onclick={() => selectedInstanceId && selectInstance(selectedInstanceId)}
              class="mt-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-white rounded transition-colors cursor-pointer border border-zinc-700"
            >
              Spróbuj ponownie
            </button>
          </div>
        </div>
      {:else if instanceDetail}
        <!-- Instance Header with KPIs & Navigation Tabs -->
        <InstanceHeader
          instance={instanceDetail}
          {activeTab}
          onTabChange={(tab) => (activeTab = tab)}
        />

        <!-- Tab Content Body (Scrollable) -->
        <div class="flex-1 overflow-y-auto">
          {#if activeTab === "overview"}
            <OverviewTab
              instance={instanceDetail}
              onGoToTab={(tab) => (activeTab = tab)}
            />
          {:else if activeTab === "times"}
            <TimesTab instance={instanceDetail} />
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
          {:else if activeTab === "groups"}
            <GroupsTab instance={instanceDetail} />
          {/if}
        </div>
      {:else}
        <div class="flex-1 flex items-center justify-center text-zinc-500 font-mono text-sm">
          Wybierz instancję z listy po lewej stronie, aby rozpocząć przeglądanie.
        </div>
      {/if}
    </main>
  </div>
</div>

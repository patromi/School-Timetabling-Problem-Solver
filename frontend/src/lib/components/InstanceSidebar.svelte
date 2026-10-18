<script lang="ts">
  import type { InstanceListItem } from "../types";
  import { uploadInstanceXml } from "../api";

  interface Props {
    instances: InstanceListItem[];
    selectedId: string | null;
    loading: boolean;
    onSelect: (id: string) => void;
    onUploaded: (ids: string[]) => void;
  }

  let { instances, selectedId, loading, onSelect, onUploaded }: Props = $props();

  let searchQuery = $state("");
  let filterType = $state<"all" | "benchmark" | "custom">("all");
  let isUploading = $state(false);
  let uploadError = $state<string | null>(null);
  let fileInputRef: HTMLInputElement | null = $state(null);

  let filteredInstances = $derived(
    instances.filter((inst) => {
      const matchesSearch =
        inst.id.toLowerCase().includes(searchQuery.toLowerCase()) ||
        inst.name.toLowerCase().includes(searchQuery.toLowerCase());
      if (!matchesSearch) return false;
      if (filterType === "benchmark") return !inst.is_custom;
      if (filterType === "custom") return inst.is_custom;
      return true;
    })
  );

  async function handleFileUpload(event: Event) {
    const target = event.target as HTMLInputElement;
    if (!target.files || target.files.length === 0) return;
    const file = target.files[0];
    isUploading = true;
    uploadError = null;
    try {
      const res = await uploadInstanceXml(file);
      onUploaded(res.imported_ids);
      if (fileInputRef) fileInputRef.value = "";
    } catch (err: any) {
      uploadError = err.message || "Błąd podczas wgrywania pliku";
    } finally {
      isUploading = false;
    }
  }
</script>

<aside class="w-80 shrink-0 bg-zinc-900 border-r border-zinc-800 flex flex-col h-full select-none text-zinc-100">
  <!-- Sidebar Header -->
  <div class="p-3 border-b border-zinc-800 bg-zinc-900/90 flex flex-col gap-2">
    <div class="flex items-center justify-between">
      <span class="text-xs font-semibold uppercase tracking-wider text-zinc-400">Instancje XHSTT</span>
      <span class="text-xs bg-zinc-800 text-zinc-300 px-2 py-0.5 rounded font-mono">
        {filteredInstances.length} / {instances.length}
      </span>
    </div>

    <!-- Upload Button & File Input -->
    <div>
      <input
        type="file"
        accept=".xml"
        bind:this={fileInputRef}
        onchange={handleFileUpload}
        class="hidden"
        id="xml-upload-input"
      />
      <label
        for="xml-upload-input"
        class="w-full flex items-center justify-center gap-2 py-1.5 px-3 bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-xs font-medium rounded border border-zinc-700 cursor-pointer transition-colors"
      >
        <span>+ Wgraj instancję XML</span>
        {#if isUploading}
          <span class="text-amber-400 animate-spin">⏳</span>
        {/if}
      </label>
      {#if uploadError}
        <p class="mt-1 text-[11px] text-red-400 font-mono leading-tight">{uploadError}</p>
      {/if}
    </div>

    <!-- Search Input -->
    <div>
      <input
        type="text"
        bind:value={searchQuery}
        placeholder="Filtruj instancje (ID lub nazwa)..."
        class="w-full px-2.5 py-1.5 text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 focus:outline-none focus:border-zinc-500 font-mono"
      />
    </div>

    <!-- Category Filters -->
    <div class="grid grid-cols-3 gap-1 text-[11px]">
      <button
        onclick={() => (filterType = "all")}
        class={`py-1 rounded text-center transition-colors ${
          filterType === "all" ? "bg-zinc-700 text-white font-medium" : "bg-zinc-800/60 text-zinc-400 hover:text-zinc-200"
        }`}
      >
        Wszystkie
      </button>
      <button
        onclick={() => (filterType = "benchmark")}
        class={`py-1 rounded text-center transition-colors ${
          filterType === "benchmark" ? "bg-zinc-700 text-white font-medium" : "bg-zinc-800/60 text-zinc-400 hover:text-zinc-200"
        }`}
      >
        Benchmark
      </button>
      <button
        onclick={() => (filterType = "custom")}
        class={`py-1 rounded text-center transition-colors ${
          filterType === "custom" ? "bg-zinc-700 text-white font-medium" : "bg-zinc-800/60 text-zinc-400 hover:text-zinc-200"
        }`}
      >
        Własne
      </button>
    </div>
  </div>

  <!-- Instances List -->
  <div class="flex-1 overflow-y-auto divide-y divide-zinc-800/60 font-sans">
    {#if loading && instances.length === 0}
      <div class="p-4 text-center text-xs text-zinc-500">Wczytywanie instancji...</div>
    {:else if filteredInstances.length === 0}
      <div class="p-4 text-center text-xs text-zinc-500">Brak pasujących instancji</div>
    {:else}
      {#each filteredInstances as inst (inst.id)}
        <button
          onclick={() => onSelect(inst.id)}
          class={`w-full text-left p-2.5 transition-colors cursor-pointer flex flex-col gap-1 border-l-2 ${
            selectedId === inst.id
              ? "bg-zinc-800 border-amber-500 text-white"
              : "hover:bg-zinc-800/50 border-transparent text-zinc-300"
          }`}
        >
          <div class="flex items-center justify-between gap-1">
            <span class="font-mono text-xs font-bold text-amber-400 tracking-tight">{inst.id}</span>
            {#if inst.is_custom}
              <span class="text-[9px] uppercase px-1 py-0.2 bg-emerald-950 text-emerald-400 border border-emerald-800 rounded">
                Własna
              </span>
            {/if}
          </div>

          <div class="text-xs truncate text-zinc-200 font-medium" title={inst.name}>
            {inst.name || inst.id}
          </div>

          <!-- Metrics Pills -->
          <div class="flex items-center gap-1.5 text-[10px] font-mono text-zinc-400 mt-0.5">
            <span title="Zdarzenia (Events)" class="bg-zinc-950 px-1.5 py-0.5 rounded border border-zinc-800">
              E: <strong class="text-zinc-200">{inst.events_count}</strong>
            </span>
            <span title="Zasoby (Resources)" class="bg-zinc-950 px-1.5 py-0.5 rounded border border-zinc-800">
              R: <strong class="text-zinc-200">{inst.resources_count}</strong>
            </span>
            <span title="Czasy (Times)" class="bg-zinc-950 px-1.5 py-0.5 rounded border border-zinc-800">
              T: <strong class="text-zinc-200">{inst.times_count}</strong>
            </span>
            <span title="Ograniczenia (Constraints)" class="bg-zinc-950 px-1.5 py-0.5 rounded border border-zinc-800">
              C: <strong class="text-zinc-200">{inst.constraints_count}</strong>
            </span>
          </div>
        </button>
      {/each}
    {/if}
  </div>
</aside>

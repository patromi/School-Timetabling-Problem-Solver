<script lang="ts">
  import type { InstanceListItem } from "../types";
  import { uploadInstanceXml } from "../api";

  interface Props {
    instances: InstanceListItem[];
    selectedId: string | null;
    loading: boolean;
    onSelect: (id: string) => void;
    onUploaded: (ids: string[]) => void;
    onNewInstance?: () => void;
  }

  let { instances, selectedId, loading, onSelect, onUploaded, onNewInstance }: Props = $props();

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
      uploadError = err.message || "Błąd podczas wgrywania pliku XML";
    } finally {
      isUploading = false;
    }
  }
</script>

<aside class="w-80 shrink-0 bg-white border-r border-slate-200/80 flex flex-col h-full select-none text-slate-800 shadow-xs">
  <!-- Sidebar Header -->
  <div class="p-4 border-b border-slate-100 flex flex-col gap-3 bg-white">
    <div class="flex items-center justify-between">
      <span class="text-xs font-bold uppercase tracking-wider text-slate-500">
        Wybór placówki
      </span>
      <span class="text-xs bg-slate-100 text-slate-600 px-2 py-0.5 rounded-full font-medium">
        {filteredInstances.length} {filteredInstances.length === 1 ? "szkoła" : "szkół"}
      </span>
    </div>

    <!-- Actions (New School + Upload) -->
    <div class="flex flex-col gap-1.5">
      {#if onNewInstance}
        <button
          type="button"
          onclick={onNewInstance}
          class="w-full flex items-center justify-center gap-2 py-2 px-3 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-lg transition-all cursor-pointer shadow-xs active:scale-[0.99]"
        >
          <span>+ Utwórz nową szkołę</span>
        </button>
      {/if}

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
        class="w-full flex items-center justify-center gap-2 py-2 px-3 bg-slate-50 hover:bg-slate-100 text-slate-700 text-xs font-semibold rounded-lg border border-slate-200 cursor-pointer transition-all active:scale-[0.99]"
      >
        <span>📁 Wgraj plik XML szkoły</span>
        {#if isUploading}
          <span class="text-blue-600 animate-spin">⏳</span>
        {/if}
      </label>
      {#if uploadError}
        <p class="mt-1.5 text-xs text-rose-600 leading-tight bg-rose-50 p-2 rounded border border-rose-100">{uploadError}</p>
      {/if}
    </div>
    </div>

    <!-- Search Input -->
    <div class="relative">
      <input
        type="text"
        bind:value={searchQuery}
        placeholder="Szukaj po nazwie lub symbolu..."
        class="w-full pl-8 pr-3 py-1.5 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all"
      />
      <span class="absolute left-2.5 top-2 text-slate-400 text-xs">🔍</span>
    </div>

    <!-- Category Filters -->
    <div class="grid grid-cols-3 gap-1 bg-slate-100 p-0.5 rounded-lg text-xs font-medium text-slate-600">
      <button
        onclick={() => (filterType = "all")}
        class={`py-1 rounded-md text-center transition-all cursor-pointer ${
          filterType === "all" ? "bg-white text-slate-900 shadow-xs font-semibold" : "hover:text-slate-900"
        }`}
      >
        Wszystkie
      </button>
      <button
        onclick={() => (filterType = "benchmark")}
        class={`py-1 rounded-md text-center transition-all cursor-pointer ${
          filterType === "benchmark" ? "bg-white text-slate-900 shadow-xs font-semibold" : "hover:text-slate-900"
        }`}
      >
        Wzorcowe
      </button>
      <button
        onclick={() => (filterType = "custom")}
        class={`py-1 rounded-md text-center transition-all cursor-pointer ${
          filterType === "custom" ? "bg-white text-slate-900 shadow-xs font-semibold" : "hover:text-slate-900"
        }`}
      >
        Własne
      </button>
    </div>
  </div>

  <!-- Instances List -->
  <div class="flex-1 overflow-y-auto divide-y divide-slate-100 font-sans">
    {#if loading && instances.length === 0}
      <div class="p-6 text-center text-xs text-slate-400">Wczytywanie listy szkół...</div>
    {:else if filteredInstances.length === 0}
      <div class="p-6 text-center text-xs text-slate-400">Brak szkół spełniających kryteria</div>
    {:else}
      {#each filteredInstances as inst (inst.id)}
        <button
          onclick={() => onSelect(inst.id)}
          class={`w-full text-left p-3.5 transition-all cursor-pointer flex flex-col gap-1 border-l-4 ${
            selectedId === inst.id
              ? "bg-blue-50/70 border-blue-600 text-slate-900 shadow-2xs"
              : "hover:bg-slate-50/80 border-transparent text-slate-700"
          }`}
        >
          <div class="flex items-center justify-between gap-1">
            <span class="text-sm font-semibold text-slate-900 truncate" title={inst.name || inst.id}>
              {inst.name || inst.id}
            </span>
            {#if inst.is_custom}
              <span class="text-[10px] font-medium px-1.5 py-0.5 bg-emerald-100 text-emerald-800 rounded-full">
                Własna
              </span>
            {/if}
          </div>

          <!-- Human-friendly description line -->
          <div class="flex items-center gap-2 text-xs text-slate-500 font-normal">
            <span>📚 {inst.events_count} lekcji</span>
            <span>•</span>
            <span>👥 {inst.resources_count} zasobów</span>
          </div>

          <!-- Technical ID sub-tag (discrete) -->
          <div class="text-[10px] text-slate-400 font-mono mt-0.5">
            Identyfikator: {inst.id}
          </div>
        </button>
      {/each}
    {/if}
  </div>
</aside>

<!-- frontend/src/lib/components/editor/studio/SchoolDrawer.svelte -->
<script lang="ts">
  import type { InstanceListItem } from "../../../types";
  import { uploadInstanceXml } from "../../../api";
  import Icon from "../../common/Icon.svelte";

  interface Props {
    open: boolean;
    instances: InstanceListItem[];
    selectedId: string | null;
    loading: boolean;
    onClose: () => void;
    onSelect: (id: string) => void;
    onUploaded: (ids: string[]) => void;
    onNewSchool: () => void;
  }

  let {
    open = false,
    instances = [],
    selectedId = null,
    loading = false,
    onClose,
    onSelect,
    onUploaded,
    onNewSchool,
  }: Props = $props();

  let searchQuery = $state("");
  let filterType = $state<"all" | "benchmark" | "custom">("all");
  let isUploading = $state(false);
  let uploadError = $state<string | null>(null);
  let fileInputRef: HTMLInputElement | null = $state(null);
  let isDragging = $state(false);

  let filteredInstances = $derived(
    instances.filter((inst) => {
      const q = searchQuery.toLowerCase();
      const matchesSearch =
        inst.id.toLowerCase().includes(q) || (inst.name || "").toLowerCase().includes(q);
      if (!matchesSearch) return false;
      if (filterType === "benchmark") return !inst.is_custom;
      if (filterType === "custom") return inst.is_custom;
      return true;
    })
  );

  let customCount = $derived(instances.filter((i) => i.is_custom).length);
  let benchmarkCount = $derived(instances.filter((i) => !i.is_custom).length);

  async function processFileUpload(file: File) {
    if (!file.name.endsWith(".xml")) {
      uploadError = "Proszę wybrać plik w formacie XML (.xml)";
      return;
    }
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

  function handleFileChange(event: Event) {
    const target = event.target as HTMLInputElement;
    if (!target.files || target.files.length === 0) return;
    processFileUpload(target.files[0]);
  }

  function handleDrop(event: DragEvent) {
    event.preventDefault();
    isDragging = false;
    if (event.dataTransfer?.files && event.dataTransfer.files.length > 0) {
      processFileUpload(event.dataTransfer.files[0]);
    }
  }

  function handleDragOver(event: DragEvent) {
    event.preventDefault();
    isDragging = true;
  }

  function handleDragLeave() {
    isDragging = false;
  }

  function handleKeydown(event: KeyboardEvent) {
    if (event.key === "Escape" && open) {
      onClose();
    }
  }
</script>

<svelte:window onkeydown={handleKeydown} />

{#if open}
  <!-- Backdrop -->
  <button
    type="button"
    class="fixed inset-0 bg-slate-900/40 backdrop-blur-xs z-40 transition-opacity animate-in fade-in duration-200 cursor-default border-none"
    onclick={onClose}
    aria-label="Zamknij panel szkół"
  ></button>

  <!-- Drawer Container (Slide-over from left) -->
  <aside
    class="fixed inset-y-0 left-0 w-96 max-w-[90vw] bg-white shadow-2xl z-50 flex flex-col font-sans text-slate-800 animate-in slide-in-from-left duration-250 select-none border-r border-slate-200"
    aria-label="Panel wyboru szkoły"
  >
    <!-- Drawer Header -->
    <div class="p-5 border-b border-slate-100 flex items-center justify-between bg-slate-50/70">
      <div class="flex items-center gap-2.5">
        <div class="w-8 h-8 rounded-xl bg-blue-600 text-white flex items-center justify-center font-bold text-sm shadow-xs">
          <Icon name="school" size="sm" class="text-white" />
        </div>
        <div>
          <h2 class="text-sm font-bold text-slate-900 leading-none">
            Wybór placówki
          </h2>
          <p class="text-[11px] text-slate-400 mt-1">
            Zarządzaj arkuszami organizacyjnymi szkół
          </p>
        </div>
      </div>

      <button
        type="button"
        onclick={onClose}
        class="w-7 h-7 rounded-lg text-slate-400 hover:text-slate-700 hover:bg-slate-100 flex items-center justify-center transition-all cursor-pointer text-base"
        title="Zamknij (Esc)"
      >
        <Icon name="close" size="sm" />
      </button>
    </div>

    <!-- Actions Area (+ Nowa szkoła, Wgraj XML) -->
    <div class="p-4 border-b border-slate-100 flex flex-col gap-2.5 bg-white">
      <button
        type="button"
        onclick={() => {
          onNewSchool();
          onClose();
        }}
        class="w-full flex items-center justify-center gap-2 py-2.5 px-3 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-xl transition-all cursor-pointer shadow-xs active:scale-[0.99]"
      >
        <Icon name="add" size="sm" class="text-white" />
        <span>Utwórz nową szkołę</span>
      </button>

      <!-- Drag & Drop Upload Zone -->
      <div
        role="region"
        aria-label="Obszar wgrywania pliku XML"
        ondrop={handleDrop}
        ondragover={handleDragOver}
        ondragleave={handleDragLeave}
        class={`border border-dashed rounded-xl p-2.5 transition-all text-center flex flex-col items-center justify-center gap-1 ${
          isDragging
            ? "border-blue-500 bg-blue-50/60"
            : "border-slate-300 hover:border-slate-400 bg-slate-50/60"
        }`}
      >
        <input
          type="file"
          accept=".xml"
          bind:this={fileInputRef}
          onchange={handleFileChange}
          class="hidden"
          id="school-drawer-xml-upload"
        />
        <label
          for="school-drawer-xml-upload"
          class="w-full flex items-center justify-center gap-2 cursor-pointer text-xs font-semibold text-slate-700 hover:text-blue-600 transition-colors py-1"
        >
          <Icon name="upload_file" size="sm" class="text-slate-500" />
          <span>Wgraj plik XML (XHSTT)</span>
          {#if isUploading}
            <Icon name="sync" size="sm" class="text-blue-600 animate-spin" />
          {/if}
        </label>
        <span class="text-[10px] text-slate-400">lub przeciągnij plik tutaj</span>

        {#if uploadError}
          <div class="w-full mt-1.5 text-[11px] text-rose-600 bg-rose-50 border border-rose-200 p-2 rounded-lg text-left leading-tight">
            {uploadError}
          </div>
        {/if}
      </div>

      <!-- Search Input -->
      <div class="relative mt-1">
        <input
          type="text"
          bind:value={searchQuery}
          placeholder="Szukaj po nazwie lub identyfikatorze..."
          class="w-full pl-8 pr-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-xl text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all"
        />
        <span class="absolute left-2.5 top-2.5 text-slate-400">
          <Icon name="search" size="xs" />
        </span>
        {#if searchQuery}
          <button
            type="button"
            onclick={() => (searchQuery = "")}
            class="absolute right-2.5 top-2.5 text-slate-400 hover:text-slate-600 text-xs"
          >
            <Icon name="close" size="xs" />
          </button>
        {/if}
      </div>

      <!-- Category Filter Pills -->
      <div class="grid grid-cols-3 gap-1 bg-slate-100 p-0.5 rounded-xl text-xs font-semibold text-slate-600">
        <button
          type="button"
          onclick={() => (filterType = "all")}
          class={`py-1.5 rounded-lg text-center transition-all cursor-pointer ${
            filterType === "all"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          Wszystkie ({instances.length})
        </button>
        <button
          type="button"
          onclick={() => (filterType = "custom")}
          class={`py-1.5 rounded-lg text-center transition-all cursor-pointer ${
            filterType === "custom"
              ? "bg-white text-emerald-800 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          Własne ({customCount})
        </button>
        <button
          type="button"
          onclick={() => (filterType = "benchmark")}
          class={`py-1.5 rounded-lg text-center transition-all cursor-pointer ${
            filterType === "benchmark"
              ? "bg-white text-blue-800 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          XHSTT ({benchmarkCount})
        </button>
      </div>
    </div>

    <!-- School List Area -->
    <div class="flex-1 overflow-y-auto divide-y divide-slate-100 p-2">
      {#if loading && instances.length === 0}
        <div class="p-8 text-center text-xs text-slate-400 flex flex-col items-center gap-2">
          <div class="w-5 h-5 border-2 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
          <span>Wczytywanie listy szkół...</span>
        </div>
      {:else if filteredInstances.length === 0}
        <div class="p-8 text-center text-xs text-slate-400">
          Nie znaleziono szkół spełniających kryteria.
        </div>
      {:else}
        {#each filteredInstances as inst (inst.id)}
          <button
            type="button"
            onclick={() => {
              onSelect(inst.id);
              onClose();
            }}
            class={`w-full text-left p-3 rounded-xl transition-all cursor-pointer flex flex-col gap-1.5 border mb-1.5 ${
              selectedId === inst.id
                ? "bg-blue-50/80 border-blue-400 text-slate-900 shadow-xs ring-1 ring-blue-500/30"
                : "border-slate-100 hover:border-slate-200 hover:bg-slate-50 text-slate-700"
            }`}
          >
            <div class="flex items-start justify-between gap-2">
              <span class="text-xs font-bold text-slate-900 leading-snug line-clamp-2" title={inst.name || inst.id}>
                {inst.name || inst.id}
              </span>
              {#if inst.is_custom}
                <span class="text-[9px] font-bold px-1.5 py-0.5 bg-emerald-100 text-emerald-800 rounded-md shrink-0">
                  Własna
                </span>
              {:else}
                <span class="text-[9px] font-medium px-1.5 py-0.5 bg-slate-100 text-slate-600 rounded-md shrink-0 font-mono">
                  Benchmark
                </span>
              {/if}
            </div>

            <div class="flex items-center gap-2 text-[11px] text-slate-500">
              <span class="flex items-center gap-1"><Icon name="menu_book" size="xs" class="text-slate-400" /> <strong>{inst.events_count}</strong> lekcji</span>
              <span>•</span>
              <span class="flex items-center gap-1"><Icon name="groups" size="xs" class="text-slate-400" /> <strong>{inst.resources_count}</strong> zasobów</span>
            </div>

            <div class="flex items-center justify-between text-[10px] text-slate-400 font-mono mt-0.5">
              <span>ID: {inst.id}</span>
              {#if selectedId === inst.id}
                <span class="text-blue-600 font-bold font-sans flex items-center gap-1 text-[11px]">
                  <span>● Aktywna</span>
                </span>
              {/if}
            </div>
          </button>
        {/each}
      {/if}
    </div>

    <!-- Drawer Footer -->
    <div class="p-3 border-t border-slate-100 bg-slate-50/70 text-center text-[11px] text-slate-400">
      Wybierz szkołę, aby natychmiast załadować jej arkusz do Studia
    </div>
  </aside>
{/if}

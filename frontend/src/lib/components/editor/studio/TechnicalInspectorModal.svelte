<!-- frontend/src/lib/components/editor/studio/TechnicalInspectorModal.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail } from "../../../types";
  import { getExportInstanceUrl } from "../../../api";

  interface Props {
    open: boolean;
    instance: InstanceDetail;
    onClose: () => void;
  }

  let { open = false, instance, onClose }: Props = $props();

  let activeTab = $state<"xml" | "json" | "times" | "groups">("xml");
  let xmlContent = $state<string>("");
  let loadingXml = $state(false);
  let xmlError = $state<string | null>(null);
  let copyFeedback = $state<string | null>(null);

  $effect(() => {
    if (open && activeTab === "xml" && instance.id && !xmlContent && !loadingXml) {
      loadXml();
    }
  });

  async function loadXml() {
    loadingXml = true;
    xmlError = null;
    try {
      const url = getExportInstanceUrl(instance.id);
      const res = await fetch(url);
      if (res.ok) {
        xmlContent = await res.text();
      } else {
        xmlError = "Plik XML będzie dostępny po zapisaniu bieżącej wersji szkoły.";
      }
    } catch (e: any) {
      xmlError = "Nie udało się pobrać pliku XML (zapisz szkołę, aby wygenerować plik).";
    } finally {
      loadingXml = false;
    }
  }

  function copyToClipboard(text: string, label: string) {
    if (!text) return;
    navigator.clipboard.writeText(text).then(() => {
      copyFeedback = `Skopiowano ${label}!`;
      setTimeout(() => {
        copyFeedback = null;
      }, 2000);
    });
  }

  function handleKeydown(e: KeyboardEvent) {
    if (e.key === "Escape" && open) {
      onClose();
    }
  }
</script>

<svelte:window onkeydown={handleKeydown} />

{#if open}
  <!-- Backdrop -->
  <button
    type="button"
    class="fixed inset-0 bg-slate-900/50 backdrop-blur-xs z-50 transition-opacity animate-in fade-in duration-200 cursor-default border-none"
    onclick={onClose}
    aria-label="Zamknij podgląd techniczny"
  ></button>

  <!-- Modal Dialog -->
  <div
    class="fixed inset-6 sm:inset-10 md:inset-16 max-w-5xl mx-auto bg-white rounded-2xl shadow-2xl z-50 flex flex-col font-sans text-slate-800 animate-in zoom-in-95 duration-200 border border-slate-200 overflow-hidden"
    role="dialog"
    aria-modal="true"
    aria-label="Podgląd techniczny XHSTT"
  >
    <!-- Modal Header -->
    <div class="px-6 py-4 border-b border-slate-200 bg-slate-50 flex items-center justify-between shrink-0">
      <div class="flex items-center gap-3">
        <div class="w-8 h-8 rounded-xl bg-slate-900 text-white flex items-center justify-center font-mono text-xs font-bold shadow-xs">
          XML
        </div>
        <div>
          <h2 class="text-sm font-bold text-slate-900 leading-tight flex items-center gap-2">
            <span>Inspekcja techniczna XHSTT</span>
            <span class="text-[10px] font-mono px-2 py-0.5 rounded-full bg-slate-200 text-slate-700">
              {instance.id}
            </span>
          </h2>
          <p class="text-[11px] text-slate-500">
            Podgląd surowego pliku XML, struktury API oraz grup ograniczeń solvera
          </p>
        </div>
      </div>

      <div class="flex items-center gap-2">
        {#if copyFeedback}
          <span class="text-xs text-emerald-600 font-bold bg-emerald-50 px-2.5 py-1 rounded-lg border border-emerald-200 animate-in fade-in">
            ✓ {copyFeedback}
          </span>
        {/if}

        <button
          type="button"
          onclick={onClose}
          class="w-8 h-8 rounded-xl text-slate-400 hover:text-slate-700 hover:bg-slate-200/70 flex items-center justify-center transition-all cursor-pointer text-base"
        >
          ✕
        </button>
      </div>
    </div>

    <!-- Navigation Tabs inside Modal -->
    <div class="px-6 py-2.5 border-b border-slate-200 bg-white flex items-center justify-between shrink-0">
      <div class="flex items-center gap-1 bg-slate-100 p-0.5 rounded-xl text-xs font-semibold text-slate-600">
        <button
          type="button"
          onclick={() => {
            activeTab = "xml";
            if (!xmlContent) loadXml();
          }}
          class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            activeTab === "xml"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>📄 Plik XHSTT XML</span>
        </button>

        <button
          type="button"
          onclick={() => (activeTab = "json")}
          class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            activeTab === "json"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>🔍 Model JSON API</span>
        </button>

        <button
          type="button"
          onclick={() => (activeTab = "times")}
          class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            activeTab === "times"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>⏰ Siatka slotów ({instance.times?.length || 0})</span>
        </button>

        <button
          type="button"
          onclick={() => (activeTab = "groups")}
          class={`px-3 py-1.5 rounded-lg transition-all cursor-pointer flex items-center gap-1.5 ${
            activeTab === "groups"
              ? "bg-white text-slate-900 shadow-xs font-bold"
              : "hover:text-slate-900"
          }`}
        >
          <span>🏷️ Grupy reguł ({(instance.time_groups?.length || 0) + (instance.resource_groups?.length || 0)})</span>
        </button>
      </div>

      <!-- Action buttons depending on active tab -->
      <div class="flex items-center gap-2">
        {#if activeTab === "xml"}
          <button
            type="button"
            onclick={() => copyToClipboard(xmlContent, "plik XML")}
            disabled={!xmlContent}
            class="px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-800 text-xs font-semibold rounded-lg transition-all cursor-pointer flex items-center gap-1.5 disabled:opacity-50"
          >
            <span>📋 Kopiuj XML</span>
          </button>
          <a
            href={getExportInstanceUrl(instance.id)}
            download={`${instance.id}.xml`}
            class="px-3 py-1.5 bg-blue-600 hover:bg-blue-700 text-white text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-1.5 shadow-xs"
          >
            <span>📥 Pobierz .xml</span>
          </a>
        {:else if activeTab === "json"}
          <button
            type="button"
            onclick={() => copyToClipboard(JSON.stringify(instance, null, 2), "obiekt JSON")}
            class="px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-800 text-xs font-semibold rounded-lg transition-all cursor-pointer flex items-center gap-1.5"
          >
            <span>📋 Kopiuj JSON</span>
          </button>
        {/if}
      </div>
    </div>

    <!-- Modal Content Body -->
    <div class="flex-1 overflow-y-auto p-6 bg-slate-50/50">
      {#if activeTab === "xml"}
        <div class="h-full flex flex-col">
          {#if loadingXml}
            <div class="h-64 flex flex-col items-center justify-center gap-2 text-slate-400 text-xs">
              <div class="w-6 h-6 border-2 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
              <span>Generowanie i pobieranie pliku XML z silnika...</span>
            </div>
          {:else if xmlError}
            <div class="bg-amber-50 border border-amber-200 rounded-xl p-5 text-xs text-amber-900 flex flex-col gap-2">
              <span class="font-bold">Informacja:</span>
              <p>{xmlError}</p>
              <p class="text-slate-600">
                Możesz przejrzeć pełną strukturę danych w zakładce <strong>Model JSON API</strong> lub zapisać szkołę, aby pobrać sformatowany plik XML zgodny ze specyfikacją XHSTT.
              </p>
            </div>
          {:else}
            <pre class="bg-slate-900 text-emerald-400 p-4 rounded-xl text-xs font-mono overflow-auto max-h-[60vh] border border-slate-800 shadow-inner select-text">{xmlContent}</pre>
          {/if}
        </div>
      {:else if activeTab === "json"}
        <pre class="bg-slate-900 text-slate-100 p-4 rounded-xl text-xs font-mono overflow-auto max-h-[60vh] border border-slate-800 shadow-inner select-text">{JSON.stringify(instance, null, 2)}</pre>
      {:else if activeTab === "times"}
        <div class="flex flex-col gap-4">
          <h3 class="text-xs font-bold uppercase tracking-wider text-slate-500">
            Zdefiniowane sloty czasowe ({instance.times?.length || 0})
          </h3>
          <div class="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-6 gap-2">
            {#each instance.times || [] as t}
              <div class="p-2.5 bg-white border border-slate-200 rounded-xl flex flex-col text-xs shadow-2xs">
                <span class="font-bold text-slate-900">{t.name || t.id}</span>
                <span class="text-[10px] text-slate-400 font-mono mt-0.5">ID: {t.id}</span>
              </div>
            {/each}
          </div>
        </div>
      {:else if activeTab === "groups"}
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          <!-- Time groups -->
          <div class="bg-white border border-slate-200 rounded-xl p-4 shadow-2xs flex flex-col gap-2">
            <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
              Grupy czasowe ({instance.time_groups?.length || 0})
            </h4>
            <div class="flex flex-col gap-1 max-h-80 overflow-y-auto">
              {#each instance.time_groups || [] as g}
                <div class="p-2 bg-slate-50 rounded-lg text-xs flex justify-between">
                  <span class="font-semibold text-slate-800">{g.name || g.id}</span>
                  <span class="text-slate-400 font-mono text-[10px]">{g.kind}</span>
                </div>
              {/each}
            </div>
          </div>

          <!-- Resource groups -->
          <div class="bg-white border border-slate-200 rounded-xl p-4 shadow-2xs flex flex-col gap-2">
            <h4 class="text-xs font-bold uppercase tracking-wider text-slate-500">
              Grupy zasobów ({instance.resource_groups?.length || 0})
            </h4>
            <div class="flex flex-col gap-1 max-h-80 overflow-y-auto">
              {#each instance.resource_groups || [] as g}
                <div class="p-2 bg-slate-50 rounded-lg text-xs flex justify-between">
                  <span class="font-semibold text-slate-800">{g.name || g.id}</span>
                  <span class="text-slate-400 font-mono text-[10px]">{g.kind}</span>
                </div>
              {/each}
            </div>
          </div>
        </div>
      {/if}
    </div>

    <!-- Modal Footer -->
    <div class="px-6 py-3 border-t border-slate-200 bg-white flex items-center justify-between text-xs text-slate-500 shrink-0">
      <span>Specyfikacja formatu: XHSTT 2014 / 2024 XML Schema</span>
      <button
        type="button"
        onclick={onClose}
        class="px-4 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-800 font-semibold rounded-lg transition-all cursor-pointer"
      >
        Zamknij
      </button>
    </div>
  </div>
{/if}

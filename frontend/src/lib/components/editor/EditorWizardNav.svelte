<script lang="ts">
  import type { EditorState } from "./editorState.svelte";

  interface Props {
    editor: EditorState;
    currentStep: number;
  }

  let { editor, currentStep = $bindable() }: Props = $props();
</script>

<nav class="bg-white border-b border-slate-200/70 px-6 py-2.5 flex items-center justify-between overflow-x-auto gap-2">
  <div class="flex items-center gap-1 sm:gap-2">
    <!-- Step 1 -->
    <button
      type="button"
      onclick={() => (currentStep = 1)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 1
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 1 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">1</span>
      <span>🏫 Szkoła i Tydzień</span>
    </button>

    <span class="text-slate-300">›</span>

    <!-- Step 2 -->
    <button
      type="button"
      onclick={() => (currentStep = 2)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 2
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 2 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">2</span>
      <span>👥 Klasy i Kadra</span>
      {#if currentStep === 2}
        <span class="text-[10px] px-1.5 py-0.5 bg-blue-100 text-blue-800 rounded font-normal">
          {editor.step2ActiveTab === 'classes' ? 'Klasy' : 'Nauczyciele'}
        </span>
      {/if}
    </button>

    <span class="text-slate-300">›</span>

    <!-- Step 3 -->
    <button
      type="button"
      onclick={() => (currentStep = 3)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 3
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 3 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">3</span>
      <span>🚪 Sale lekcyjne</span>
    </button>

    <span class="text-slate-300">›</span>

    <!-- Step 4 -->
    <button
      type="button"
      onclick={() => (currentStep = 4)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 4
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 4 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">4</span>
      <span>📚 Siatka lekcji</span>
    </button>

    <span class="text-slate-300">›</span>

    <!-- Step 5 -->
    <button
      type="button"
      onclick={() => (currentStep = 5)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 5
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 5 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">5</span>
      <span>⚖️ Zasady planu</span>
    </button>

    <span class="text-slate-300">›</span>

    <!-- Step 6 -->
    <button
      type="button"
      onclick={() => (currentStep = 6)}
      class={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
        currentStep === 6
          ? "bg-blue-50 text-blue-700 border border-blue-200/70 shadow-2xs"
          : "text-slate-600 hover:bg-slate-50"
      }`}
    >
      <span class="w-5 h-5 rounded-full flex items-center justify-center text-[10px] {currentStep === 6 ? 'bg-blue-600 text-white' : 'bg-slate-100 text-slate-700'}">6</span>
      <span>✅ Podsumowanie</span>
    </button>
  </div>

  <!-- Health indicator -->
  <div class="text-xs font-semibold flex items-center gap-2">
    {#if editor.isSchoolValid}
      <span class="px-2.5 py-1 bg-emerald-50 text-emerald-800 rounded-full border border-emerald-200/70 text-[11px] flex items-center gap-1.5 font-bold">
        <span>●</span>
        <span>Arkusz gotowy</span>
      </span>
    {:else}
      <button
        type="button"
        onclick={() => {
          if (editor.validationIssues.length > 0) {
            currentStep = editor.validationIssues[0].step;
          }
        }}
        class="px-2.5 py-1 bg-amber-50 hover:bg-amber-100 text-amber-800 rounded-full border border-amber-200/70 text-[11px] flex items-center gap-1.5 cursor-pointer transition-colors font-medium shadow-2xs"
        title="Kliknij, aby przejść do brakujących danych"
      >
        <span>●</span>
        <span>Wymaga uzupełnienia ({editor.validationIssues.length})</span>
      </button>
    {/if}
  </div>
</nav>

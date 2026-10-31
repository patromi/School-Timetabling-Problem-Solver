<!-- frontend/src/lib/components/editor/steps/constraints/RelationsAndOrderRules.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-3.5">
  <!-- Reguła 1: Zajęcia jednoczesne i grupy międzyoddziałowe -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">🔀</span>
      <div>
        <div class="flex items-center gap-2">
          <span class="text-xs font-bold text-slate-900">Zajęcia jednoczesne i grupy międzyoddziałowe</span>
          {#if editor.linkedGroups.length > 0}
            <span class="text-[10px] font-bold px-2 py-0.5 bg-purple-100 text-purple-800 rounded-full border border-purple-200">
              {editor.linkedGroups.length} grup powiązanych
            </span>
          {/if}
        </div>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Wymusza odbywanie się wybranych lekcji dokładnie w tym samym czasie (np. podział na grupy językowe z klas 1A i 1B lub łączony WF).
          {#if editor.linkedGroups.length > 0}
            <span class="text-purple-900 font-medium block mt-1">
              Aktywne grupy: {editor.linkedGroups.map((g) => g.name).join(", ")}.
            </span>
          {:else}
            <span class="text-slate-400 italic block mt-1">
              (Brak powiązań — kliknij przycisk obok, aby skonfigurować grupy odbywające się w tej samej godzinie).
            </span>
          {/if}
        </p>
      </div>
    </div>
    <button
      type="button"
      onclick={() => (editor.isLinkedModalOpen = true)}
      class="text-[11px] font-semibold px-3 py-1.5 bg-purple-50 hover:bg-purple-100 text-purple-700 rounded-xl border border-purple-200 shrink-0 cursor-pointer transition-colors flex items-center gap-1.5 shadow-2xs"
    >
      <span>🔗</span>
      <span>Zarządzaj powiązaniami</span>
    </button>
  </div>

  <!-- Reguła 2: Następstwo i kolejność lekcji -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleOrderLessons ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">📐</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Kolejność i chronologia zajęć</span>
            {#if editor.orderRules.length > 0}
              <span class="text-[10px] font-bold px-2 py-0.5 bg-blue-100 text-blue-800 rounded-full border border-blue-200">
                {editor.orderRules.length} zdefiniowanych reguł
              </span>
            {/if}
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Egzekwuje porządek chronologiczny (np. wykład teoretyczny musi odbyć się przed ćwiczeniami praktycznymi lub pracownią).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleOrderLessons} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleOrderLessons}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-3 text-xs">
        <!-- Add new order pair -->
        <div class="p-3 bg-slate-50 rounded-xl border border-slate-200/80 flex flex-col gap-2.5">
          <span class="font-bold text-slate-800 text-[11px] uppercase tracking-wider">
            Dodaj regułę chronologiczną (Lekcja A musi odbyć się PRZED Lekcją B):
          </span>
          <div class="grid grid-cols-1 sm:grid-cols-3 gap-2">
            <div>
              <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja wcześniejsza (1):</span>
              <select
                bind:value={editor.newOrderFirstLesson}
                class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs text-slate-800"
              >
                <option value="">Wybierz pierwszą lekcję...</option>
                {#each editor.lessons as l}
                  <option value={l.id}>{l.subject} ({editor.classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                {/each}
              </select>
            </div>

            <div>
              <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja następująca (2):</span>
              <select
                bind:value={editor.newOrderSecondLesson}
                class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs text-slate-800"
              >
                <option value="">Wybierz kolejną lekcję...</option>
                {#each editor.lessons as l}
                  <option value={l.id}>{l.subject} ({editor.classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                {/each}
              </select>
            </div>

            <div class="flex items-end gap-2">
              <div class="flex-1">
                <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Wymagany odstęp:</span>
                <select
                  bind:value={editor.newOrderSeparation}
                  class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-xs text-slate-800"
                >
                  <option value={0}>Wcześniej w tym samym tygodniu</option>
                  <option value={editor.periodsPerDay}>Co najmniej 1 dzień odstępu</option>
                </select>
              </div>

              <button
                type="button"
                onclick={() => editor.addOrderRule()}
                class="px-3.5 py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-bold rounded-lg text-xs cursor-pointer transition-colors shadow-2xs"
              >
                + Dodaj
              </button>
            </div>
          </div>
        </div>

        <!-- Existing order pairs -->
        {#if editor.orderRules.length > 0}
          <div class="divide-y divide-slate-100 border border-slate-200 rounded-xl overflow-hidden bg-white">
            {#each editor.orderRules as rule}
              <div class="p-2.5 flex items-center justify-between text-xs hover:bg-slate-50 transition-colors">
                <div class="flex items-center gap-2">
                  <span class="w-5 h-5 rounded-full bg-blue-100 text-blue-800 font-bold flex items-center justify-center text-[10px]">1</span>
                  <span class="font-bold text-slate-900">{editor.lessons.find((l) => l.id === rule.firstLessonId)?.subject || rule.firstLessonId}</span>
                  <span class="text-slate-400">➔</span>
                  <span class="w-5 h-5 rounded-full bg-blue-100 text-blue-800 font-bold flex items-center justify-center text-[10px]">2</span>
                  <span class="font-bold text-slate-900">{editor.lessons.find((l) => l.id === rule.secondLessonId)?.subject || rule.secondLessonId}</span>
                  <span class="text-slate-500 text-[11px] ml-1">
                    ({rule.minSeparation > 0 ? "min. 1 dzień odstępu" : "przed kolejną"})
                  </span>
                </div>
                <button
                  type="button"
                  onclick={() => editor.removeOrderRule(rule.id)}
                  class="text-rose-500 hover:text-rose-700 font-bold p-1 rounded-md hover:bg-rose-50 cursor-pointer transition-colors"
                  title="Usuń tę regułę"
                >
                  ✕
                </button>
              </div>
            {/each}
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 3: Indywidualne niedostępności zasobów -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">🗓️</span>
      <div>
        <div class="flex items-center gap-2">
          <span class="text-xs font-bold text-slate-900">Indywidualne wykluczenia czasowe (dyspozycyjność)</span>
          {#if editor.totalUnavailabilitiesCount > 0}
            <span class="text-[10px] font-bold px-2 py-0.5 bg-rose-100 text-rose-800 rounded-full border border-rose-200">
              {editor.totalUnavailabilitiesCount} zablokowanych godzin
            </span>
          {/if}
        </div>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Uwzględnia zdefiniowane w krokach 2 i 3 blokady godzin dla nauczycieli, sal i klas (urlopy, zebrania, wyłączenia pracowni).
          {#if editor.resourcesWithUnavailability.length > 0}
            <span class="text-slate-700 font-medium block mt-1">
              Zasoby z ograniczeniami ({editor.resourcesWithUnavailability.length}): {editor.resourcesWithUnavailability.map((r) => r.name).slice(0, 4).join(", ")}{editor.resourcesWithUnavailability.length > 4 ? ` i ${editor.resourcesWithUnavailability.length - 4} innych` : ""}.
            </span>
          {:else}
            <span class="text-slate-400 italic block mt-1">
              (Brak wykluczeń — wszyscy nauczyciele, sale i oddziały są w pełni dyspozycyjne).
            </span>
          {/if}
        </p>
      </div>
    </div>
    <span class="text-[11px] font-semibold px-2.5 py-1 bg-blue-50 text-blue-700 rounded-full border border-blue-200 shrink-0">
      {editor.resourcesWithUnavailability.length} zasobów
    </span>
  </div>
</div>

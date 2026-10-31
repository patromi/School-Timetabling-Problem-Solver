<!-- frontend/src/lib/components/editor/studio/GlobalRulesStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let showAdvancedMorning = $state(false);
  let showAdvancedAfternoon = $state(false);
  let showAdvancedStudentGaps = $state(false);
</script>

<div class="flex flex-col gap-6">
  <!-- Header -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-2">
    <h3 class="text-base font-bold text-slate-900 flex items-center gap-2">
      <span>⚖️</span>
      <span>Ogólne zasady i standardy dydaktyczne szkoły</span>
    </h3>
    <p class="text-xs text-slate-500">
      Zasady obowiązujące globalnie w całej placówce. Ograniczenia specyficzne dla konkretnych pedagogów (np. dni pracy) lub oddziałów (ramy dzienne) konfiguruje się bezpośrednio w ich profilach.
    </p>
  </div>

  <!-- Standardy bezwzględne (Nienaruszalne fundamenty) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
      Wymogi nienaruszalne (fundamenty planu):
    </span>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
      <!-- Reguła 1: Brak nakładania się lekcji -->
      <div class="p-4 bg-slate-50 rounded-xl border border-slate-200/70 flex items-start justify-between gap-3 text-xs">
        <div class="flex items-start gap-3">
          <span class="text-xl">🛡️</span>
          <div>
            <span class="font-bold text-slate-900 block">Brak nakładania się zajęć</span>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Żaden nauczyciel, oddział ani sala nie mogą mieć dwóch lekcji jednocześnie.
            </p>
          </div>
        </div>
        <span class="text-[10px] font-bold px-2 py-0.5 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
          Bezwzględny
        </span>
      </div>

      <!-- Reguła 2: Kompletny plan -->
      <div class="p-4 bg-slate-50 rounded-xl border border-slate-200/70 flex items-start justify-between gap-3 text-xs">
        <div class="flex items-start gap-3">
          <span class="text-xl">⏱️</span>
          <div>
            <span class="font-bold text-slate-900 block">Kompletny plan zajęć</span>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Każda wprowadzona godzina lekcyjna musi mieć przydzielony termin w tygodniu.
            </p>
          </div>
        </div>
        <span class="text-[10px] font-bold px-2 py-0.5 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
          Bezwzględny
        </span>
      </div>
    </div>
  </div>

  <!-- Higiena i pory dnia -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
      Optymalizacja dydaktyczna i pory dnia:
    </span>

    <div class="flex flex-col gap-3.5 text-xs">
      <!-- Brak okienek dla uczniów -->
      <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
        <div class="flex items-start justify-between gap-4">
          <div class="flex items-start gap-3">
            <span class="text-xl">☕</span>
            <div>
              <div class="flex items-center gap-2">
                <span class="font-bold text-slate-900">Brak okienek dla uczniów (lekcje ciągiem)</span>
                <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-rose-50 text-rose-700 border-rose-200">
                  {!editor.ruleStudentNoGapsSoft ? "Wymóg bezwzględny" : "Wysoki priorytet"}
                </span>
              </div>
              <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
                Uczniowie mają zajęcia ciągiem od pierwszej do ostatniej godziny, bez pustych przerw w środku dnia.
              </p>
            </div>
          </div>
          <input type="checkbox" bind:checked={editor.ruleStudentNoGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
        </div>

        {#if editor.ruleStudentNoGaps}
          <div class="pt-2 border-t border-slate-200/60 flex items-center justify-between text-xs">
            <span class="text-slate-500 text-[11px]">Rygor reguły:</span>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.ruleStudentNoGapsSoft = false)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold {!editor.ruleStudentNoGapsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
              >
                🔴 Bezwzględny
              </button>
              <button
                type="button"
                onclick={() => { editor.ruleStudentNoGapsSoft = true; editor.studentGapsWeight = 10; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold {editor.ruleStudentNoGapsSoft ? 'bg-amber-600 text-white' : 'text-slate-600'}"
              >
                🟡 Wysoki priorytet
              </button>
            </div>
          </div>
        {/if}
      </div>

      <!-- Przedmioty ścisłe rano -->
      <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
        <div class="flex items-start justify-between gap-4">
          <div class="flex items-start gap-3">
            <span class="text-xl">🌅</span>
            <div>
              <div class="flex items-center gap-2">
                <span class="font-bold text-slate-900">Przedmioty wymagające skupienia rano</span>
                <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-amber-50 text-amber-800 border-amber-200">
                  Lekcje 1–4
                </span>
              </div>
              <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
                Planuje matematykę, fizykę, chemię, język polski i biologię na pierwszych 4 godzinach lekcyjnych.
              </p>
            </div>
          </div>
          <input type="checkbox" bind:checked={editor.ruleMorningCoreSubjects} class="w-4 h-4 accent-amber-600 cursor-pointer mt-1" />
        </div>
      </div>

      <!-- WF po południu -->
      <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
        <div class="flex items-start justify-between gap-4">
          <div class="flex items-start gap-3">
            <span class="text-xl">🌇</span>
            <div>
              <div class="flex items-center gap-2">
                <span class="font-bold text-slate-900">WF i zajęcia artystyczne w godzinach popołudniowych</span>
                <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-blue-50 text-blue-800 border-blue-200">
                  Od 5. lekcji
                </span>
              </div>
              <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
                Kieruje wychowanie fizyczne, plastykę, muzykę i etykę na godziny popołudniowe.
              </p>
            </div>
          </div>
          <input type="checkbox" bind:checked={editor.ruleAfternoonFlexSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
        </div>
      </div>
    </div>
  </div>

  <!-- Relacje i kolejność -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex items-center justify-between">
      <div>
        <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
          Zajęcia jednoczesne i grupy międzyoddziałowe ({editor.linkedGroups.length}):
        </span>
        <p class="text-xs text-slate-500 mt-0.5">
          Wymusza odbywanie się wybranych lekcji w tym samym slocie czasowym (np. grupy językowe z klas 1A i 1B).
        </p>
      </div>

      <button
        type="button"
        onclick={() => (editor.isLinkedModalOpen = true)}
        class="px-3 py-1.5 bg-slate-900 hover:bg-slate-800 text-white rounded-xl text-xs font-semibold cursor-pointer shadow-xs transition-colors"
      >
        🔗 Zarządzaj grupami
      </button>
    </div>

    {#if editor.linkedGroups.length > 0}
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-200 text-xs text-slate-700">
        Aktywne grupy: <strong>{editor.linkedGroups.map((g) => g.name).join(", ")}</strong>
      </div>
    {/if}
  </div>
</div>


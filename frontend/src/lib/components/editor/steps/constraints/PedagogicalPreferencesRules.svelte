<!-- frontend/src/lib/components/editor/steps/constraints/PedagogicalPreferencesRules.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let showAdvancedSpread = $state(false);
  let showAdvancedMorning = $state(false);
  let showAdvancedAfternoon = $state(false);
</script>

<div class="flex flex-col gap-3.5">
  <!-- Reguła 1: Rozkład przedmiotów w tygodniu -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleSpreadSubjects ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🗓️</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Równomierny rozkład przedmiotów w tygodniu</span>
            <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
              !editor.ruleSpreadSubjectsSoft
                ? "bg-rose-50 text-rose-700 border-rose-200"
                : editor.spreadSubjectsWeight >= 8
                  ? "bg-indigo-50 text-indigo-700 border-indigo-200"
                  : "bg-slate-100 text-slate-700 border-slate-200"
            }`}>
              {!editor.ruleSpreadSubjectsSoft ? "🔴 Wymóg bezwzględny" : editor.spreadSubjectsWeight >= 8 ? "🟡 Wysoki priorytet" : "⚪ Standardowa preferencja"}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Zapobiega kumulowaniu wielu godzin tego samego przedmiotu (np. 4h matematyki) w jeden dzień.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleSpreadSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleSpreadSubjects}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <div class="flex items-center gap-2">
            <span class="text-slate-600 font-medium text-[11px]">Dopuszczalna liczba lekcji dziennie:</span>
            <select
              bind:value={editor.spreadSubjectsMode}
              class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-xl text-xs font-bold text-slate-800 cursor-pointer"
            >
              <option value="single">Ściśle 1 lekcja dziennie (np. 4h = 4 różne dni)</option>
              <option value="blocks">Dopuszczalne bloki 2-godzinne (maks. 2h/dzień)</option>
            </select>
          </div>

          <div class="flex items-center bg-slate-100 p-0.5 rounded-xl border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => { editor.ruleSpreadSubjectsSoft = false; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {!editor.ruleSpreadSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🔴 Bezwzględny
            </button>
            <button
              type="button"
              onclick={() => { editor.ruleSpreadSubjectsSoft = true; editor.spreadSubjectsWeight = 10; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.ruleSpreadSubjectsSoft && editor.spreadSubjectsWeight >= 8 ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🟡 Wysoki
            </button>
            <button
              type="button"
              onclick={() => { editor.ruleSpreadSubjectsSoft = true; editor.spreadSubjectsWeight = 4; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.ruleSpreadSubjectsSoft && editor.spreadSubjectsWeight < 8 ? 'bg-slate-700 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              ⚪ Standardowy
            </button>
          </div>
        </div>

        {#if editor.ruleSpreadSubjectsSoft}
          <div class="flex items-center justify-between pt-1">
            <button
              type="button"
              onclick={() => (showAdvancedSpread = !showAdvancedSpread)}
              class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
            >
              {showAdvancedSpread ? "▾ Ukryj zaawansowaną wagę" : "▸ Dostosuj dokładną wagę matematyczną"}
            </button>
            {#if showAdvancedSpread}
              <div class="flex items-center gap-1.5 animate-in fade-in duration-100">
                <span class="text-slate-500 text-[11px]">Waga kary solvera:</span>
                <input
                  type="number"
                  min="1"
                  max="20"
                  bind:value={editor.spreadSubjectsWeight}
                  class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
                />
              </div>
            {/if}
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 2: Ciągłość bloków -->
  <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
    <div class="flex items-start gap-3">
      <span class="text-xl">🔗</span>
      <div>
        <span class="text-xs font-bold text-slate-900">Ciągłość bloków lekcyjnych (zajęcia 2-godzinne)</span>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Zajęcia 2-godzinne (np. laboratorium chemiczne, 2h wychowania fizycznego) odbywają się pod rząd w tym samym dniu.
        </p>
      </div>
    </div>
    <input type="checkbox" bind:checked={editor.ruleContinuousBlocks} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
  </label>

  <!-- Reguła 3: Poranne lekcje przedmiotów ścisłych i maturalnych -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleMorningCoreSubjects ? 'border-amber-300 ring-2 ring-amber-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🌅</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Przedmioty wymagające skupienia w godzinach porannych</span>
            <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
              !editor.morningCoreSubjectsSoft
                ? "bg-rose-50 text-rose-700 border-rose-200"
                : editor.morningCoreSubjectsWeight >= 6
                  ? "bg-amber-50 text-amber-800 border-amber-200"
                  : "bg-slate-100 text-slate-700 border-slate-200"
            }`}>
              {!editor.morningCoreSubjectsSoft ? "🔴 Wymóg bezwzględny" : editor.morningCoreSubjectsWeight >= 6 ? "🟡 Wysoki priorytet" : "⚪ Standardowa preferencja"}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Planuje przedmioty ścisłe i egzaminacyjne (Matematyka, Fizyka, Chemia, Polski, Biologia) na porannych godzinach (lekcje 1–4, 8:00–11:30).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleMorningCoreSubjects} class="w-4 h-4 accent-amber-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleMorningCoreSubjects}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <span class="text-slate-500 text-[11px]">
            Preferowane lekcje 1–4 każdego dnia roboczego.
          </span>

          <div class="flex items-center bg-slate-100 p-0.5 rounded-xl border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => { editor.morningCoreSubjectsSoft = false; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {!editor.morningCoreSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🔴 Bezwzględny
            </button>
            <button
              type="button"
              onclick={() => { editor.morningCoreSubjectsSoft = true; editor.morningCoreSubjectsWeight = 8; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.morningCoreSubjectsSoft && editor.morningCoreSubjectsWeight >= 6 ? 'bg-amber-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🟡 Wysoki (zalecane)
            </button>
            <button
              type="button"
              onclick={() => { editor.morningCoreSubjectsSoft = true; editor.morningCoreSubjectsWeight = 3; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.morningCoreSubjectsSoft && editor.morningCoreSubjectsWeight < 6 ? 'bg-slate-700 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              ⚪ Standardowy
            </button>
          </div>
        </div>

        {#if editor.morningCoreSubjectsSoft}
          <div class="flex items-center justify-between pt-1">
            <button
              type="button"
              onclick={() => (showAdvancedMorning = !showAdvancedMorning)}
              class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
            >
              {showAdvancedMorning ? "▾ Ukryj zaawansowaną wagę" : "▸ Dostosuj dokładną wagę matematyczną"}
            </button>
            {#if showAdvancedMorning}
              <div class="flex items-center gap-1.5 animate-in fade-in duration-100">
                <span class="text-slate-500 text-[11px]">Waga kary:</span>
                <input
                  type="number"
                  min="1"
                  max="20"
                  bind:value={editor.morningCoreSubjectsWeight}
                  class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
                />
              </div>
            {/if}
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 4: WF i zajęcia artystyczne po południu -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleAfternoonFlexSubjects ? 'border-indigo-300 ring-2 ring-indigo-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🌇</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">WF i zajęcia artystyczne w godzinach popołudniowych</span>
            <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
              !editor.afternoonFlexSubjectsSoft
                ? "bg-rose-50 text-rose-700 border-rose-200"
                : editor.afternoonFlexSubjectsWeight >= 6
                  ? "bg-indigo-50 text-indigo-700 border-indigo-200"
                  : "bg-slate-100 text-slate-700 border-slate-200"
            }`}>
              {!editor.afternoonFlexSubjectsSoft ? "🔴 Wymóg bezwzględny" : editor.afternoonFlexSubjectsWeight >= 6 ? "🟡 Wysoki priorytet" : "⚪ Standardowa preferencja"}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Kieruje wychowanie fizyczne, plastykę, muzykę i etykę na późniejsze godziny lekcyjne (od 5. lekcji, od 11:45).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleAfternoonFlexSubjects} class="w-4 h-4 accent-indigo-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleAfternoonFlexSubjects}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <span class="text-slate-500 text-[11px]">
            Preferowane godziny od 5. lekcji wzwyż.
          </span>

          <div class="flex items-center bg-slate-100 p-0.5 rounded-xl border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => { editor.afternoonFlexSubjectsSoft = false; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {!editor.afternoonFlexSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🔴 Bezwzględny
            </button>
            <button
              type="button"
              onclick={() => { editor.afternoonFlexSubjectsSoft = true; editor.afternoonFlexSubjectsWeight = 6; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.afternoonFlexSubjectsSoft && editor.afternoonFlexSubjectsWeight >= 5 ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🟡 Wysoki
            </button>
            <button
              type="button"
              onclick={() => { editor.afternoonFlexSubjectsSoft = true; editor.afternoonFlexSubjectsWeight = 3; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.afternoonFlexSubjectsSoft && editor.afternoonFlexSubjectsWeight < 5 ? 'bg-slate-700 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              ⚪ Standardowy (zalecane)
            </button>
          </div>
        </div>

        {#if editor.afternoonFlexSubjectsSoft}
          <div class="flex items-center justify-between pt-1">
            <button
              type="button"
              onclick={() => (showAdvancedAfternoon = !showAdvancedAfternoon)}
              class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
            >
              {showAdvancedAfternoon ? "▾ Ukryj zaawansowaną wagę" : "▸ Dostosuj dokładną wagę matematyczną"}
            </button>
            {#if showAdvancedAfternoon}
              <div class="flex items-center gap-1.5 animate-in fade-in duration-100">
                <span class="text-slate-500 text-[11px]">Waga kary:</span>
                <input
                  type="number"
                  min="1"
                  max="20"
                  bind:value={editor.afternoonFlexSubjectsWeight}
                  class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
                />
              </div>
            {/if}
          </div>
        {/if}
      </div>
    {/if}
  </div>
</div>

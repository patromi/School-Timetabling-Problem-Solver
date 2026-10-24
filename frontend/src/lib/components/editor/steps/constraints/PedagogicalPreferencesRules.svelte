<!-- frontend/src/lib/components/editor/steps/constraints/PedagogicalPreferencesRules.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-3">
  <!-- Reguła 6: Rozkład przedmiotów (SpreadEventsConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleSpreadSubjects ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🗓️</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Równomierny rozkład przedmiotów w tygodniu (SpreadEventsConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {editor.ruleSpreadSubjectsSoft ? 'bg-indigo-50 text-indigo-700 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
              {editor.ruleSpreadSubjectsSoft ? `SOFT (waga ${editor.spreadSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
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
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <div class="flex items-center gap-2">
          <span class="text-slate-600 font-medium text-[11px]">Dopuszczalna liczba lekcji dziennie:</span>
          <select
            bind:value={editor.spreadSubjectsMode}
            class="px-2 py-1 bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800 cursor-pointer"
          >
            <option value="single">Ściśle 1 lekcja dziennie (maks. 1h/dzień)</option>
            <option value="blocks">Dopuszczalne bloki 2-godzinne (maks. 2h/dzień)</option>
          </select>
        </div>

        <div class="flex items-center gap-3">
          <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => (editor.ruleSpreadSubjectsSoft = true)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {editor.ruleSpreadSubjectsSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              SOFT
            </button>
            <button
              type="button"
              onclick={() => (editor.ruleSpreadSubjectsSoft = false)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!editor.ruleSpreadSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              HARD
            </button>
          </div>

          {#if editor.ruleSpreadSubjectsSoft}
            <div class="flex items-center gap-1.5">
              <span class="text-slate-500 text-[11px]">Waga:</span>
              <input type="number" min="1" max="20" bind:value={editor.spreadSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
            </div>
          {/if}
        </div>
      </div>
    {/if}
  </div>

  <!-- Reguła 7: Ciągłość bloków -->
  <label class="p-4 bg-white rounded-xl border border-slate-200 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-all">
    <div class="flex items-start gap-3">
      <span class="text-xl">🔗</span>
      <div>
        <span class="text-xs font-bold text-slate-900">Ciągłość bloków lekcyjnych (bloki 2h)</span>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Zajęcia 2-godzinne (np. laboratorium, 2h WF-u) odbywają się pod rząd tego samego dnia.
        </p>
      </div>
    </div>
    <input type="checkbox" bind:checked={editor.ruleContinuousBlocks} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
  </label>

  <!-- Reguła 8: Poranne lekcje przedmiotów ścisłych i maturalnych (PreferTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleMorningCoreSubjects ? 'border-amber-300 ring-2 ring-amber-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🌅</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Przedmioty ścisłe i maturalne rano (PreferTimesConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {editor.morningCoreSubjectsSoft ? 'bg-amber-50 text-amber-800 border border-amber-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
              {editor.morningCoreSubjectsSoft ? `SOFT (waga ${editor.morningCoreSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Kieruje przedmioty wymagające koncentracji (Matematyka, Fizyka, Chemia, Polski, Biologia) na poranne godziny (lekcje 1–4).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleMorningCoreSubjects} class="w-4 h-4 accent-amber-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleMorningCoreSubjects}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <span class="text-slate-500 text-[11px]">
          Preferowane lekcje 1–4 każdego dnia (8:00–11:30).
        </span>
        <div class="flex items-center gap-3">
          <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => (editor.morningCoreSubjectsSoft = true)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {editor.morningCoreSubjectsSoft ? 'bg-amber-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              SOFT
            </button>
            <button
              type="button"
              onclick={() => (editor.morningCoreSubjectsSoft = false)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!editor.morningCoreSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              HARD
            </button>
          </div>
          {#if editor.morningCoreSubjectsSoft}
            <div class="flex items-center gap-1.5">
              <span class="text-slate-500 text-[11px]">Waga:</span>
              <input type="number" min="1" max="20" bind:value={editor.morningCoreSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
            </div>
          {/if}
        </div>
      </div>
    {/if}
  </div>

  <!-- Reguła 9: Popołudniowe lekcje WF i przedmiotów artystycznych (PreferTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleAfternoonFlexSubjects ? 'border-indigo-300 ring-2 ring-indigo-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🌇</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">WF i zajęcia artystyczne po południu (PreferTimesConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full {editor.afternoonFlexSubjectsSoft ? 'bg-indigo-50 text-indigo-800 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
              {editor.afternoonFlexSubjectsSoft ? `SOFT (waga ${editor.afternoonFlexSubjectsWeight})` : 'HARD (Wymóg bezwzględny)'}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Kieruje zajęcia sportowe i artystyczne (Wychowanie Fizyczne, Plastyka, Muzyka, Religia, Etyka) na godziny popołudniowe (od 5. lekcji).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleAfternoonFlexSubjects} class="w-4 h-4 accent-indigo-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleAfternoonFlexSubjects}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <span class="text-slate-500 text-[11px]">
          Preferowane lekcje 5+ każdego dnia (od 11:45).
        </span>
        <div class="flex items-center gap-3">
          <div class="flex items-center bg-slate-50 p-0.5 rounded-lg border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => (editor.afternoonFlexSubjectsSoft = true)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {editor.afternoonFlexSubjectsSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              SOFT
            </button>
            <button
              type="button"
              onclick={() => (editor.afternoonFlexSubjectsSoft = false)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold transition-all cursor-pointer {!editor.afternoonFlexSubjectsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600'}"
            >
              HARD
            </button>
          </div>
          {#if editor.afternoonFlexSubjectsSoft}
            <div class="flex items-center gap-1.5">
              <span class="text-slate-500 text-[11px]">Waga:</span>
              <input type="number" min="1" max="20" bind:value={editor.afternoonFlexSubjectsWeight} class="w-12 px-1.5 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
            </div>
          {/if}
        </div>
      </div>
    {/if}
  </div>
</div>

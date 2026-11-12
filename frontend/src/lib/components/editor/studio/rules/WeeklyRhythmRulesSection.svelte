<!-- frontend/src/lib/components/editor/studio/rules/WeeklyRhythmRulesSection.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import WeightAdjuster from "./WeightAdjuster.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
  <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
    3. Rytm tygodnia i preferowane pory dnia:
  </span>

  <div class="flex flex-col gap-3.5 text-xs">
    <!-- Równomierny rozkład przedmiotów w tygodniu -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">🗓️</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Równomierne rozłożenie przedmiotów w tygodniu</span>
              {#if editor.ruleSpreadSubjects}
                <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
                  !editor.ruleSpreadSubjectsSoft
                    ? "bg-rose-50 text-rose-700 border-rose-200"
                    : editor.spreadSubjectsWeight >= 8
                      ? "bg-amber-50 text-amber-800 border-amber-200"
                      : "bg-slate-100 text-slate-700 border-slate-200"
                }`}>
                  {!editor.ruleSpreadSubjectsSoft ? "Wymóg bezwzględny" : editor.spreadSubjectsWeight >= 8 ? "Ważna zasada" : "Preferencja"}
                </span>
              {:else}
                <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-slate-100 text-slate-500 border-slate-200">
                  Wyłączone
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Zapobiega kumulowaniu wielu godzin tego samego przedmiotu (np. 4h matematyki) w jednym dniu.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleSpreadSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleSpreadSubjects}
        <div class="pt-2.5 border-t border-slate-200/60 flex flex-col gap-2">
          <div class="flex flex-wrap items-center justify-between gap-3">
            <div class="flex items-center gap-2">
              <label for="global-spread-mode" class="text-slate-500 text-[11px]">Zasada dzienna:</label>
              <select
                id="global-spread-mode"
                bind:value={editor.spreadSubjectsMode}
                class="px-2.5 py-1 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
              >
                <option value="single">Maks. 1 lekcja tego przedmiotu dziennie</option>
                <option value="blocks">Dopuszczalne bloki 2h (maks. 2 lekcje dziennie)</option>
              </select>
            </div>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.ruleSpreadSubjectsSoft = false)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {!editor.ruleSpreadSubjectsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
              >
                🔴 Bezwzględny
              </button>
              <button
                type="button"
                onclick={() => { editor.ruleSpreadSubjectsSoft = true; editor.spreadSubjectsWeight = 10; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.ruleSpreadSubjectsSoft && editor.spreadSubjectsWeight >= 8 ? 'bg-amber-600 text-white' : 'text-slate-600'}"
              >
                🟡 Ważna zasada
              </button>
              <button
                type="button"
                onclick={() => { editor.ruleSpreadSubjectsSoft = true; editor.spreadSubjectsWeight = 5; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.ruleSpreadSubjectsSoft && editor.spreadSubjectsWeight < 8 ? 'bg-slate-700 text-white' : 'text-slate-600'}"
              >
                ⚪ Preferencja
              </button>
            </div>
          </div>

          {#if editor.ruleSpreadSubjectsSoft}
            <WeightAdjuster label="Waga kary:" bind:value={editor.spreadSubjectsWeight} />
          {/if}
        </div>
      {/if}
    </div>

    <!-- Ciągłość bloków 2-godzinnych -->
    <label class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex items-start justify-between gap-4 cursor-pointer hover:border-slate-300 transition-colors">
      <div class="flex items-start gap-3">
        <span class="text-xl">🧱</span>
        <div>
          <span class="font-bold text-slate-900">Łącz zajęcia 2-godzinne w ciągłe bloki</span>
          <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
            Dla przedmiotów oznaczonych jako lekcje 2-godzinne tworzy nierozdzielny blok dwóch kolejnych godzin w jednym dniu (np. pracownia, 2h WF).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleContinuousBlocks} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </label>

    <!-- Przedmioty ścisłe rano -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">🌅</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Przedmioty wymagające skupienia rano</span>
              <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-amber-50 text-amber-800 border-amber-200">
                Lekcje 1–4
              </span>
              {#if editor.ruleMorningCoreSubjects}
                <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
                  !editor.morningCoreSubjectsSoft ? "bg-rose-50 text-rose-700 border-rose-200" : "bg-slate-100 text-slate-700 border-slate-200"
                }`}>
                  {!editor.morningCoreSubjectsSoft ? "Bezwzględny" : "Preferencja"}
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Kieruje przedmioty zawierające w nazwie: matematyka, fizyka, chemia, język polski lub biologia na pierwsze 4 lekcje dnia.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleMorningCoreSubjects} class="w-4 h-4 accent-amber-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleMorningCoreSubjects}
        <div class="pt-2.5 border-t border-slate-200/60 flex flex-col gap-2">
          <div class="flex flex-wrap items-center justify-between gap-2">
            <span class="text-slate-500 text-[11px]">Rygor reguły:</span>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.morningCoreSubjectsSoft = false)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {!editor.morningCoreSubjectsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
              >
                🔴 Bezwzględny
              </button>
              <button
                type="button"
                onclick={() => { editor.morningCoreSubjectsSoft = true; editor.morningCoreSubjectsWeight = 4; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.morningCoreSubjectsSoft ? 'bg-amber-600 text-white' : 'text-slate-600'}"
              >
                ⚪ Preferencja
              </button>
            </div>
          </div>
          {#if editor.morningCoreSubjectsSoft}
            <WeightAdjuster label="Waga:" bind:value={editor.morningCoreSubjectsWeight} />
          {/if}
        </div>
      {/if}
    </div>

    <!-- WF po południu -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">🌇</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">WF i zajęcia artystyczne w godzinach popołudniowych</span>
              <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-blue-50 text-blue-800 border-blue-200">
                Od 5. lekcji
              </span>
              {#if editor.ruleAfternoonFlexSubjects}
                <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
                  !editor.afternoonFlexSubjectsSoft ? "bg-rose-50 text-rose-700 border-rose-200" : "bg-slate-100 text-slate-700 border-slate-200"
                }`}>
                  {!editor.afternoonFlexSubjectsSoft ? "Bezwzględny" : "Preferencja"}
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Kieruje wychowanie fizyczne (WF), plastykę, muzykę, religię i etykę na późniejsze godziny (od 5. lekcji).
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleAfternoonFlexSubjects} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleAfternoonFlexSubjects}
        <div class="pt-2.5 border-t border-slate-200/60 flex flex-col gap-2">
          <div class="flex flex-wrap items-center justify-between gap-2">
            <span class="text-slate-500 text-[11px]">Rygor reguły:</span>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.afternoonFlexSubjectsSoft = false)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {!editor.afternoonFlexSubjectsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
              >
                🔴 Bezwzględny
              </button>
              <button
                type="button"
                onclick={() => { editor.afternoonFlexSubjectsSoft = true; editor.afternoonFlexSubjectsWeight = 3; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.afternoonFlexSubjectsSoft ? 'bg-blue-600 text-white' : 'text-slate-600'}"
              >
                ⚪ Preferencja
              </button>
            </div>
          </div>
          {#if editor.afternoonFlexSubjectsSoft}
            <WeightAdjuster label="Waga:" bind:value={editor.afternoonFlexSubjectsWeight} />
          {/if}
        </div>
      {/if}
    </div>
  </div>
</div>


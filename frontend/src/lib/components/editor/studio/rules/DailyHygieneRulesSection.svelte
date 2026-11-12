<!-- frontend/src/lib/components/editor/studio/rules/DailyHygieneRulesSection.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import WeightAdjuster from "./WeightAdjuster.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
  let showWeightDailyLimits = $state(false);
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
  <div>
    <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
      2. Higiena dnia i okienka (domyślne standardy szkoły):
    </span>
    <p class="text-[11px] text-slate-500 mt-0.5">
      Określają komfort planu dla uczniów i nauczycieli. Własne limity wpisane w kartach klas/nauczycieli mają pierwszeństwo przed ustawieniami domyślnymi.
    </p>
  </div>

  <div class="flex flex-col gap-3.5 text-xs">
    <!-- Brak okienek dla uczniów -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">☕</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Brak okienek dla uczniów (lekcje ciągiem)</span>
              {#if editor.ruleStudentNoGaps}
                <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
                  !editor.ruleStudentNoGapsSoft
                    ? "bg-rose-50 text-rose-700 border-rose-200"
                    : editor.studentGapsWeight >= 10
                      ? "bg-amber-50 text-amber-800 border-amber-200"
                      : "bg-slate-100 text-slate-700 border-slate-200"
                }`}>
                  {!editor.ruleStudentNoGapsSoft ? "Wymóg bezwzględny" : editor.studentGapsWeight >= 10 ? "Ważna zasada" : "Preferencja"}
                </span>
              {:else}
                <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full border bg-slate-100 text-slate-500 border-slate-200">
                  Wyłączone
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Uczniowie mają zajęcia ciągiem od pierwszej do ostatniej godziny, bez pustych przerw w środku dnia.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleStudentNoGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleStudentNoGaps}
        <div class="pt-2.5 border-t border-slate-200/60 flex flex-col gap-2">
          <div class="flex flex-wrap items-center justify-between gap-2">
            <span class="text-slate-500 text-[11px]">Rygor reguły:</span>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.ruleStudentNoGapsSoft = false)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {!editor.ruleStudentNoGapsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
              >
                🔴 Bezwzględny
              </button>
              <button
                type="button"
                onclick={() => { editor.ruleStudentNoGapsSoft = true; editor.studentGapsWeight = 10; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.ruleStudentNoGapsSoft && editor.studentGapsWeight >= 10 ? 'bg-amber-600 text-white' : 'text-slate-600'}"
              >
                🟡 Ważna zasada
              </button>
              <button
                type="button"
                onclick={() => { editor.ruleStudentNoGapsSoft = true; editor.studentGapsWeight = 5; }}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.ruleStudentNoGapsSoft && editor.studentGapsWeight < 10 ? 'bg-slate-700 text-white' : 'text-slate-600'}"
              >
                ⚪ Preferencja
              </button>
            </div>
          </div>

          {#if editor.ruleStudentNoGapsSoft}
            <WeightAdjuster label="Waga za 1 okienko:" bind:value={editor.studentGapsWeight} />
          {/if}
        </div>
      {/if}
    </div>

    <!-- Domyślny limit okienek dla nauczycieli -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">👨‍🏫</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Domyślny limit okienek dla nauczycieli</span>
              {#if editor.ruleTeacherMaxGaps}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
                  Maks. {editor.teacherMaxGapsLimit} / tydzień
                </span>
              {:else}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-slate-100 text-slate-500 rounded-full border border-slate-200">
                  Brak limitu domyślnego
                </span>
              {/if}
              {#if editor.teachersWithCustomGaps.length > 0}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                  {editor.teachersWithCustomGaps.length} z limitem własnym
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Ogranicza puste godziny między lekcjami dla kadry, która nie ma własnego limitu w swoim profilu.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleTeacherMaxGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleTeacherMaxGaps}
        <div class="pt-2.5 border-t border-slate-200/60 flex flex-col gap-2">
          <div class="flex flex-wrap items-center justify-between gap-3">
            <div class="flex items-center gap-2">
              <label for="global-teacher-gaps" class="text-slate-500 text-[11px]">Maksymalnie w tygodniu:</label>
              <select
                id="global-teacher-gaps"
                bind:value={editor.teacherMaxGapsLimit}
                class="px-2.5 py-1 bg-white border border-slate-200 rounded-lg text-xs font-semibold text-slate-800 cursor-pointer"
              >
                <option value={0}>0 okienek (ciągły plan)</option>
                <option value={1}>Maks. 1 okienko</option>
                <option value={2}>Maks. 2 okienka (zalecane)</option>
                <option value={3}>Maks. 3 okienka</option>
                <option value={4}>Maks. 4 okienka</option>
              </select>
            </div>
            <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
              <button
                type="button"
                onclick={() => (editor.teacherGapsWeight = 10)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.teacherGapsWeight >= 8 ? 'bg-amber-600 text-white' : 'text-slate-600'}"
              >
                🟡 Ważna zasada
              </button>
              <button
                type="button"
                onclick={() => (editor.teacherGapsWeight = 5)}
                class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer {editor.teacherGapsWeight < 8 ? 'bg-slate-700 text-white' : 'text-slate-600'}"
              >
                ⚪ Preferencja
              </button>
            </div>
          </div>

          <WeightAdjuster label="Waga za okienko ponad limit:" bind:value={editor.teacherGapsWeight} />
        </div>
      {/if}
    </div>

    <!-- Dzienne limity lekcji (klasy i nauczyciele) -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <!-- Dla klas -->
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">📊</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Domyślne dzienne ramy lekcyjne klas</span>
              {#if editor.ruleMaxLessonsDaily}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-emerald-50 text-emerald-700 rounded-full border border-emerald-200">
                  {editor.studentMinDailyLimit}–{editor.studentMaxDailyLimit} lekcji / dzień
                </span>
              {:else}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-slate-100 text-slate-500 rounded-full border border-slate-200">
                  Brak limitu domyślnego
                </span>
              {/if}
              {#if editor.classesWithCustomDailyLimit.length > 0}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                  {editor.classesWithCustomDailyLimit.length} klas z limitem własnym
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Zapobiega dniom zbyt krótkim (np. 1–2 lekcje) lub przeładowanym dla oddziałów bez indywidualnych ustawień.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleMaxLessonsDaily}
        <div class="pl-8 flex flex-wrap items-center gap-4 text-xs">
          <div class="flex items-center gap-2">
            <label for="global-student-min" class="text-slate-600 text-[11px]">Min. lekcji dziennie:</label>
            <input id="global-student-min" type="number" min="1" max="8" bind:value={editor.studentMinDailyLimit} class="w-14 px-2 py-1 text-center bg-white border border-slate-200 rounded-lg text-xs font-bold text-slate-800" />
          </div>
          <div class="flex items-center gap-2">
            <label for="global-student-max" class="text-slate-600 text-[11px]">Maks. lekcji dziennie:</label>
            <input id="global-student-max" type="number" min="3" max="12" bind:value={editor.studentMaxDailyLimit} class="w-14 px-2 py-1 text-center bg-white border border-slate-200 rounded-lg text-xs font-bold text-slate-800" />
          </div>
        </div>
      {/if}

      <div class="border-t border-slate-200/70 pt-3 flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">🧑‍🏫</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Domyślne maksymalne obciążenie dzienne nauczycieli</span>
              {#if editor.ruleTeacherMaxDaily}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-emerald-50 text-emerald-700 rounded-full border border-emerald-200">
                  Maks. {editor.teacherMaxDailyLimit} lekcji / dzień
                </span>
              {:else}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-slate-100 text-slate-500 rounded-full border border-slate-200">
                  Brak limitu domyślnego
                </span>
              {/if}
              {#if editor.teachersWithCustomDailyLimit.length > 0}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                  {editor.teachersWithCustomDailyLimit.length} z limitem własnym
                </span>
              {/if}
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Chroni nauczycieli przed prowadzeniem zbyt wielu godzin w jednym dniu.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleTeacherMaxDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleTeacherMaxDaily}
        <div class="pl-8 flex flex-wrap items-center gap-4 text-xs">
          <div class="flex items-center gap-2">
            <label for="global-teacher-max-daily" class="text-slate-600 text-[11px]">Maks. lekcji nauczyciela w dniu:</label>
            <input id="global-teacher-max-daily" type="number" min="2" max="12" bind:value={editor.teacherMaxDailyLimit} class="w-14 px-2 py-1 text-center bg-white border border-slate-200 rounded-lg text-xs font-bold text-slate-800" />
          </div>
        </div>
      {/if}

      {#if editor.ruleMaxLessonsDaily || editor.ruleTeacherMaxDaily}
        <div class="pt-1 flex items-center justify-between">
          <button
            type="button"
            onclick={() => (showWeightDailyLimits = !showWeightDailyLimits)}
            class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
          >
            {showWeightDailyLimits ? "▾ Ukryj wagi liczbowe" : "▸ Dostosuj wagi liczbowe limitów dziennych"}
          </button>
          {#if showWeightDailyLimits}
            <div class="flex items-center gap-3">
              {#if editor.ruleMaxLessonsDaily}
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga (klasy):</span>
                  <input type="number" min="1" max="50" bind:value={editor.studentDailyWeight} class="w-14 px-2 py-0.5 text-center bg-white border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              {/if}
              {#if editor.ruleTeacherMaxDaily}
                <div class="flex items-center gap-1.5">
                  <span class="text-slate-500 text-[11px]">Waga (kadra):</span>
                  <input type="number" min="1" max="50" bind:value={editor.teacherDailyWeight} class="w-14 px-2 py-0.5 text-center bg-white border border-slate-200 rounded text-xs font-bold text-slate-800" />
                </div>
              {/if}
            </div>
          {/if}
        </div>
      {/if}
    </div>

    <!-- Kompaktowy tydzień pracy kadry -->
    <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">📅</span>
          <div>
            <div class="flex flex-wrap items-center gap-2">
              <span class="font-bold text-slate-900">Kompaktowy tydzień pracy (dni robocze kadry)</span>
              <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                {editor.teachersWithCustomDays.length} z limitem dni
              </span>
            </div>
            <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
              Uwzględnia limity dni w tygodniu ustawione indywidualnie w profilach nauczycieli (np. niepełny etat lub dojazdy).
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleTeacherClusterDays} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleTeacherClusterDays}
        <div class="pt-2 border-t border-slate-200/60 flex flex-col gap-2">
          {#if editor.teachersWithCustomDays.length > 0}
            <div class="p-2 bg-purple-50/70 rounded-lg border border-purple-100 text-[11px] text-purple-900">
              Aktywne dla: {editor.teachersWithCustomDays.map((t) => `${t.name} (maks. ${t.maxDaysPerWeek} dni)`).join(", ")}
            </div>
          {/if}
          <WeightAdjuster label="Waga reguły:" bind:value={editor.teacherClusterWeight} />
        </div>
      {/if}
    </div>
  </div>
</div>


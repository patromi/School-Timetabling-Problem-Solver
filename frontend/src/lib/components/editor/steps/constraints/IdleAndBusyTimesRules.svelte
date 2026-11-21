<!-- frontend/src/lib/components/editor/steps/constraints/IdleAndBusyTimesRules.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let showAdvancedStudentGaps = $state(false);
  let showAdvancedTeacherGaps = $state(false);
  let showAdvancedClusterDays = $state(false);
</script>

<div class="flex flex-col gap-3.5">
  <!-- Reguła 1: Brak nakładania się lekcji -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">🛡️</span>
      <div>
        <div class="flex items-center gap-2">
          <span class="text-xs font-bold text-slate-900">Brak konfliktów i nakładania się zajęć</span>
        </div>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Żaden nauczyciel, oddział klasowy ani sala lekcyjna nie mogą brać udziału w dwóch lekcjach jednocześnie.
        </p>
      </div>
    </div>
    <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
      🔴 Wymóg bezwzględny
    </span>
  </div>

  <!-- Reguła 2: Wszystkie lekcje w planie -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">⏱️</span>
      <div>
        <div class="flex items-center gap-2">
          <span class="text-xs font-bold text-slate-900">Kompletny plan (przydział wszystkich lekcji)</span>
        </div>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Każda zaplanowana godzina lekcyjna wprowadzona w arkuszu musi znaleźć termin w planie tygodniowym.
        </p>
      </div>
    </div>
    <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
      🔴 Wymóg bezwzględny
    </span>
  </div>

  <!-- Reguła 3: Okienka uczniów -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleStudentNoGaps ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">☕</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Brak okienek dla uczniów (lekcje ciągiem)</span>
            <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
              !editor.ruleStudentNoGapsSoft
                ? "bg-rose-50 text-rose-700 border-rose-200"
                : editor.studentGapsWeight >= 10
                  ? "bg-amber-50 text-amber-800 border-amber-200"
                  : "bg-slate-100 text-slate-700 border-slate-200"
            }`}>
              {!editor.ruleStudentNoGapsSoft ? "🔴 Wymóg bezwzględny" : editor.studentGapsWeight >= 10 ? "🟡 Wysoki priorytet" : "⚪ Standardowa preferencja"}
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Uczniowie mają zajęcia ciągiem od pierwszej do ostatniej lekcji, bez pustych przerw w środku dnia.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleStudentNoGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleStudentNoGaps}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <span class="text-slate-600 font-medium text-[11px]">Priorytet reguły dla algorytmu:</span>
          <div class="flex items-center bg-slate-100 p-0.5 rounded-xl border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => { editor.ruleStudentNoGapsSoft = false; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {!editor.ruleStudentNoGapsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🔴 Bezwzględny zakaz
            </button>
            <button
              type="button"
              onclick={() => { editor.ruleStudentNoGapsSoft = true; editor.studentGapsWeight = 15; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.ruleStudentNoGapsSoft && editor.studentGapsWeight >= 10 ? 'bg-amber-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🟡 Wysoki priorytet
            </button>
            <button
              type="button"
              onclick={() => { editor.ruleStudentNoGapsSoft = true; editor.studentGapsWeight = 5; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.ruleStudentNoGapsSoft && editor.studentGapsWeight < 10 ? 'bg-slate-700 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              ⚪ Preferencja
            </button>
          </div>
        </div>

        {#if editor.ruleStudentNoGapsSoft}
          <div class="flex items-center justify-between pt-1">
            <button
              type="button"
              onclick={() => (showAdvancedStudentGaps = !showAdvancedStudentGaps)}
              class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
            >
              {showAdvancedStudentGaps ? "▾ Ukryj zaawansowaną wagę" : "▸ Dostosuj dokładną wagę matematyczną"}
            </button>
            {#if showAdvancedStudentGaps}
              <div class="flex items-center gap-1.5 animate-in fade-in duration-100">
                <span class="text-slate-500 text-[11px]">Waga kary za 1 okienko:</span>
                <input
                  type="number"
                  min="1"
                  max="50"
                  bind:value={editor.studentGapsWeight}
                  class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
                />
              </div>
            {/if}
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 4: Okienka nauczycieli -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleTeacherMaxGaps ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">👨‍🏫</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Ograniczenie okienek dla nauczycieli</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
              Maks. {editor.teacherMaxGapsLimit} {editor.teacherMaxGapsLimit === 1 ? "okienko" : "okienka"} w tyg.
            </span>
            {#if editor.teachersWithCustomGaps.length > 0}
              <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                {editor.teachersWithCustomGaps.length} indywidualnych
              </span>
            {/if}
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Nauczyciele nie powinni bezczynnie czekać w szkole między swoimi lekcjami w ciągu tygodnia pracy.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleTeacherMaxGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleTeacherMaxGaps}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <div class="flex items-center gap-2">
            <span class="text-slate-600 text-[11px] font-medium">Dopuszczalna liczba wolnych godzin w tygodniu:</span>
            <select
              bind:value={editor.teacherMaxGapsLimit}
              class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-xl text-xs font-semibold text-slate-800 cursor-pointer"
            >
              <option value={0}>0 okienek w tygodniu (ściśle ciągły plan)</option>
              <option value={1}>Maks. 1 okienko w tygodniu</option>
              <option value={2}>Maks. 2 okienka w tygodniu (zalecane)</option>
              <option value={3}>Maks. 3 okienka w tygodniu</option>
              <option value={4}>Maks. 4 okienka w tygodniu</option>
            </select>
          </div>

          <div class="flex items-center bg-slate-100 p-0.5 rounded-xl border border-slate-200 text-xs">
            <button
              type="button"
              onclick={() => { editor.teacherGapsWeight = 10; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.teacherGapsWeight >= 8 ? 'bg-blue-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              🟡 Wysoki priorytet
            </button>
            <button
              type="button"
              onclick={() => { editor.teacherGapsWeight = 4; }}
              class="px-2.5 py-1 rounded-lg text-[11px] font-semibold transition-all cursor-pointer {editor.teacherGapsWeight < 8 ? 'bg-slate-700 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
            >
              ⚪ Standardowy
            </button>
          </div>
        </div>

        <div class="flex items-center justify-between pt-1">
          <button
            type="button"
            onclick={() => (showAdvancedTeacherGaps = !showAdvancedTeacherGaps)}
            class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
          >
            {showAdvancedTeacherGaps ? "▾ Ukryj zaawansowaną wagę" : "▸ Dostosuj dokładną wagę kary"}
          </button>
          {#if showAdvancedTeacherGaps}
            <div class="flex items-center gap-1.5 animate-in fade-in duration-100">
              <span class="text-slate-500 text-[11px]">Waga kary za każde dodatkowe okienko:</span>
              <input
                type="number"
                min="1"
                max="50"
                bind:value={editor.teacherGapsWeight}
                class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
              />
            </div>
          {/if}
        </div>

        {#if editor.teachersWithCustomGaps.length > 0}
          <div class="p-2.5 bg-cyan-50/70 rounded-xl border border-cyan-100 text-[11px] text-cyan-900 flex items-center gap-2">
            <span>ℹ️</span>
            <span>Nauczyciele z indywidualnym limitem okienek: {editor.teachersWithCustomGaps.map(t => `${t.name} (maks. ${t.maxGapsPerWeek})`).join(", ")}</span>
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 5: Dzienne obciążenie klas i kadry -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleMaxLessonsDaily ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">📊</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Dzienny limit godzin lekcyjnych (higiena pracy)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-emerald-50 text-emerald-700 rounded-full border border-emerald-200">
              Klasy: {editor.studentMaxDailyLimit}h • Kadra: {editor.teacherMaxDailyLimit}h
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Chroni uczniów i kadrę pedagogiczną przed nadmiernym zmęczeniem zbyt dużą liczbą lekcji w jednym dniu.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleMaxLessonsDaily}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-4 text-xs">
        <div class="flex items-center gap-2.5">
          <span class="text-slate-600 font-medium text-[11px]">Maksymalnie dla klasy w jednym dniu:</span>
          <div class="flex items-center gap-1.5">
            <input
              type="number"
              min="4"
              max="10"
              bind:value={editor.studentMaxDailyLimit}
              class="w-14 px-2 py-1 text-center bg-slate-50 border border-slate-200 rounded-lg text-xs font-bold text-slate-800"
            />
            <span class="text-slate-400 text-[11px]">godz./dzień</span>
          </div>
        </div>

        <div class="flex items-center gap-2.5">
          <span class="text-slate-600 font-medium text-[11px]">Maksymalnie dla nauczyciela w jednym dniu:</span>
          <div class="flex items-center gap-1.5">
            <input
              type="number"
              min="4"
              max="10"
              bind:value={editor.teacherMaxDailyLimit}
              class="w-14 px-2 py-1 text-center bg-slate-50 border border-slate-200 rounded-lg text-xs font-bold text-slate-800"
            />
            <span class="text-slate-400 text-[11px]">godz./dzień</span>
          </div>
        </div>
      </div>
    {/if}
  </div>

  <!-- Reguła 6: Kompaktowy tydzień pracy kadry -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleTeacherClusterDays ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">📅</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Kompaktowy tydzień pracy (dni wolne dla kadry)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
              {editor.teachersWithCustomDays.length} z limitem dni
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Skupia lekcje w wyznaczoną liczbę dni roboczych dla nauczycieli niepełnoetatowych lub dojeżdżających.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleTeacherClusterDays} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleTeacherClusterDays}
      <div class="pt-3 border-t border-slate-100 flex flex-col gap-2.5 text-xs">
        <div class="flex flex-wrap items-center justify-between gap-3">
          <span class="text-slate-500 text-[11px]">
            Liczbę dni pracy ustala się indywidualnie przy każdym nauczycielu w Kroku 2 (przycisk Dostępność).
          </span>
          <div class="flex items-center gap-1.5">
            <span class="text-slate-500 text-[11px]">Waga reguły:</span>
            <input
              type="number"
              min="1"
              max="50"
              bind:value={editor.teacherClusterWeight}
              class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800"
            />
          </div>
        </div>

        {#if editor.teachersWithCustomDays.length > 0}
          <div class="p-2.5 bg-purple-50/70 rounded-xl border border-purple-100 text-[11px] text-purple-900 flex items-center gap-2">
            <span>ℹ️</span>
            <span>Nauczyciele z limitem dni: {editor.teachersWithCustomDays.map(t => `${t.name} (maks. ${t.maxDaysPerWeek} dni)`).join(", ")}</span>
          </div>
        {/if}
      </div>
    {/if}
  </div>
</div>

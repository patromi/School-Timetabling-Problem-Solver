<!-- frontend/src/lib/components/editor/steps/constraints/IdleAndBusyTimesRules.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-3">
  <!-- Reguła 1: Brak nakładania się lekcji -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">🛡️</span>
      <div>
        <span class="text-xs font-bold text-slate-900">Brak nakładania się lekcji (AvoidClashes)</span>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Żaden nauczyciel, oddział klasowy ani sala lekcyjna nie mogą brać udziału w dwóch zajęciach jednocześnie.
        </p>
      </div>
    </div>
    <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
      Wymóg bezwzględny
    </span>
  </div>

  <!-- Reguła 2: Wszystkie lekcje w planie -->
  <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
    <div class="flex items-start gap-3">
      <span class="text-xl">⏱️</span>
      <div>
        <span class="text-xs font-bold text-slate-900">Wszystkie lekcje w planie (AssignTime)</span>
        <p class="text-xs text-slate-500 mt-0.5 leading-snug">
          Każda godzina zajęć wprowadzona w siatce musi mieć przydzielony termin w tygodniu.
        </p>
      </div>
    </div>
    <span class="text-[11px] font-semibold px-2.5 py-1 bg-rose-50 text-rose-700 rounded-full border border-rose-200 shrink-0">
      Wymóg bezwzględny
    </span>
  </div>

  <!-- Reguła 3: Okienka uczniów (LimitIdleTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">☕</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Brak okienek dla uczniów (LimitIdleTimesConstraint)</span>
            <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${
              !editor.ruleStudentNoGapsSoft ? "bg-rose-50 text-rose-700 border-rose-200" : "bg-amber-50 text-amber-700 border-amber-200"
            }`}>
              {!editor.ruleStudentNoGapsSoft ? "Wymóg HARD" : `SOFT (waga ${editor.studentGapsWeight})`}
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
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <div class="flex items-center gap-4">
          <span class="text-slate-500 text-[11px] font-medium">Rygor ograniczenia:</span>
          <label class="flex items-center gap-1.5 cursor-pointer">
            <input type="radio" name="studentGapsRigor" checked={!editor.ruleStudentNoGapsSoft} onchange={() => (editor.ruleStudentNoGapsSoft = false)} class="w-3.5 h-3.5 accent-rose-600" />
            <span class="font-semibold text-rose-700 text-[11px]">HARD (Zakaz bezwzględny)</span>
          </label>
          <label class="flex items-center gap-1.5 cursor-pointer">
            <input type="radio" name="studentGapsRigor" checked={editor.ruleStudentNoGapsSoft} onchange={() => (editor.ruleStudentNoGapsSoft = true)} class="w-3.5 h-3.5 accent-amber-600" />
            <span class="font-semibold text-amber-700 text-[11px]">SOFT (Preferencja z wagą)</span>
          </label>
        </div>
        {#if editor.ruleStudentNoGapsSoft}
          <div class="flex items-center gap-1.5">
            <span class="text-slate-500 text-[11px]">Waga kary:</span>
            <input type="number" min="1" max="50" bind:value={editor.studentGapsWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
          </div>
        {/if}
      </div>
    {/if}
  </div>

  <!-- Reguła 4: Okienka nauczycieli (LimitIdleTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">👨‍🏫</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Minimalizacja okienek dla nauczycieli (LimitIdleTimesConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
              Maks. {editor.teacherMaxGapsLimit} {editor.teacherMaxGapsLimit === 1 ? "okienko" : "okienka"} (waga {editor.teacherGapsWeight})
            </span>
            {#if editor.teachersWithCustomGaps.length > 0}
              <span class="text-[10px] font-semibold px-2 py-0.5 bg-cyan-50 text-cyan-700 rounded-full border border-cyan-200">
                {editor.teachersWithCustomGaps.length} indywidualnych
              </span>
            {/if}
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Nauczyciele nie powinni czekać bezczynnie w szkole między lekcjami w ciągu tygodnia roboczego.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleTeacherMaxGaps} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleTeacherMaxGaps}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <div class="flex items-center gap-3">
          <span class="text-slate-500 text-[11px] font-medium">Dopuszczalny limit ogólny:</span>
          <select bind:value={editor.teacherMaxGapsLimit} class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-semibold text-slate-800">
            <option value={0}>0 okienek w tygodniu (ciągły plan)</option>
            <option value={1}>Maks. 1 okienko w tygodniu</option>
            <option value={2}>Maks. 2 okienka w tygodniu</option>
            <option value={3}>Maks. 3 okienka w tygodniu</option>
            <option value={4}>Maks. 4 okienka w tygodniu</option>
          </select>
        </div>
        <div class="flex items-center gap-1.5">
          <span class="text-slate-500 text-[11px]">Waga kary (kara/okienko):</span>
          <input type="number" min="1" max="50" bind:value={editor.teacherGapsWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
        </div>
      </div>
      {#if editor.teachersWithCustomGaps.length > 0}
        <div class="p-2 bg-cyan-50/60 rounded-lg border border-cyan-100 text-[11px] text-cyan-900">
          Nauczyciele z indywidualnym limitem: {editor.teachersWithCustomGaps.map(t => `${t.name} (max ${t.maxGapsPerWeek})`).join(", ")}
        </div>
      {/if}
    {/if}
  </div>

  <!-- Reguła 5: Dzienne obciążenie klas (LimitBusyTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">📊</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Maksymalny wymiar godzin dziennie (LimitBusyTimesConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-emerald-50 text-emerald-700 rounded-full border border-emerald-200">
              Klasy: {editor.studentMaxDailyLimit}h, Kadra: {editor.teacherMaxDailyLimit}h
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Chroni uczniów i kadrę przed przeciążeniem nadmierną liczbą lekcji w ciągu jednego dnia.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleMaxLessonsDaily}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-4 text-xs">
        <div class="flex items-center gap-2">
          <span class="text-slate-500 text-[11px]">Maks. dla oddziału:</span>
          <input type="number" min="4" max="10" bind:value={editor.studentMaxDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
          <span class="text-slate-400 text-[11px]">godz./dzień</span>
        </div>
        <div class="flex items-center gap-2">
          <span class="text-slate-500 text-[11px]">Maks. dla nauczyciela:</span>
          <input type="number" min="4" max="10" bind:value={editor.teacherMaxDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
          <span class="text-slate-400 text-[11px]">godz./dzień</span>
        </div>
      </div>
    {/if}
  </div>

  <!-- Reguła: Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint) -->
  <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">📅</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint)</span>
            <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
              {editor.teachersWithCustomDays.length} z limitem dni
            </span>
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Egzekwuje ograniczenie liczby dni obecności w szkole dla nauczycieli na niepełnym etacie lub dojeżdżających.
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleTeacherClusterDays} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleTeacherClusterDays}
      <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
        <span class="text-slate-500 text-[11px]">
          Limity dni konfiguruje się w kroku 2 przy każdym pedagogu (przycisk Dostępność).
        </span>
        <div class="flex items-center gap-1.5">
          <span class="text-slate-500 text-[11px]">Waga kary:</span>
          <input type="number" min="1" max="50" bind:value={editor.teacherClusterWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
        </div>
      </div>
      {#if editor.teachersWithCustomDays.length > 0}
        <div class="p-2 bg-purple-50/60 rounded-lg border border-purple-100 text-[11px] text-purple-900">
          Nauczyciele z limitem dni: {editor.teachersWithCustomDays.map(t => `${t.name} (maks. ${t.maxDaysPerWeek} dni)`).join(", ")}
        </div>
      {/if}
    {/if}
  </div>
</div>

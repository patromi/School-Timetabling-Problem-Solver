<!-- frontend/src/lib/components/editor/steps/Step5Constraints.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-6">
  <div>
    <h3 class="text-base font-bold text-slate-900">Standardy dydaktyczne i zasady szkoły</h3>
    <p class="text-xs text-slate-500 mt-0.5">
      Zaznacz reguły, które algorytm optymalizacyjny powinien wymusić lub preferować podczas układania planu.
    </p>
  </div>

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
          <span class="text-xl">⚖️</span>
          <div>
            <div class="flex items-center gap-2">
              <span class="text-xs font-bold text-slate-900">Dzienne ramy lekcyjne dla oddziałów (LimitBusyTimesConstraint)</span>
              <span class="text-[10px] font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-200">
                {editor.studentMinDailyLimit} - {editor.studentMaxDailyLimit} lekcji/dzień
              </span>
              {#if editor.classesWithCustomDailyLimit.length > 0}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                  {editor.classesWithCustomDailyLimit.length} indywidualnych
                </span>
              {/if}
            </div>
            <p class="text-xs text-slate-500 mt-0.5 leading-snug">
              Higiena nauki: zapobiega przychodzeniu uczniów na pojedyncze lekcje oraz przeciążeniu zbyt długim dniem nauki.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleMaxLessonsDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleMaxLessonsDaily}
        <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
          <div class="flex items-center gap-3">
            <div class="flex items-center gap-1.5">
              <span class="text-slate-500 text-[11px]">Min. lekcji dziennie:</span>
              <input type="number" min="1" max="6" bind:value={editor.studentMinDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
            </div>
            <div class="flex items-center gap-1.5">
              <span class="text-slate-500 text-[11px]">Maks. lekcji dziennie:</span>
              <input type="number" min="4" max="10" bind:value={editor.studentMaxDailyLimit} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
            </div>
          </div>
          <div class="flex items-center gap-1.5">
            <span class="text-slate-500 text-[11px]">Waga kary:</span>
            <input type="number" min="1" max="50" bind:value={editor.studentDailyWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
          </div>
        </div>
      {/if}
    </div>

    <!-- Reguła 5b: Maksymalne obciążenie dzienne nauczycieli (LimitBusyTimesConstraint) -->
    <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">⏱️</span>
          <div>
            <div class="flex items-center gap-2">
              <span class="text-xs font-bold text-slate-900">Maksymalne obciążenie dzienne nauczycieli (LimitBusyTimesConstraint)</span>
              <span class="text-[10px] font-semibold px-2 py-0.5 bg-amber-50 text-amber-700 rounded-full border border-amber-200">
                Maks. {editor.teacherMaxDailyLimit} lekcji/dzień
              </span>
              {#if editor.teachersWithCustomDailyLimit.length > 0}
                <span class="text-[10px] font-semibold px-2 py-0.5 bg-purple-50 text-purple-700 rounded-full border border-purple-200">
                  {editor.teachersWithCustomDailyLimit.length} indywidualnych
                </span>
              {/if}
            </div>
            <p class="text-xs text-slate-500 mt-0.5 leading-snug">
              Chroni kadrę pedagogiczną przed kumulacją zbyt wielu godzin dydaktycznych w jednym dniu.
            </p>
          </div>
        </div>
        <input type="checkbox" bind:checked={editor.ruleTeacherMaxDaily} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
      </div>

      {#if editor.ruleTeacherMaxDaily}
        <div class="pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
          <div class="flex items-center gap-3">
            <span class="text-slate-500 text-[11px] font-medium">Maks. lekcji w dniu:</span>
            <select bind:value={editor.teacherMaxDailyLimit} class="px-2.5 py-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-semibold text-slate-800">
              <option value={4}>Maks. 4 lekcje dziennie</option>
              <option value={5}>Maks. 5 lekcji dziennie</option>
              <option value={6}>Maks. 6 lekcji dziennie</option>
              <option value={7}>Maks. 7 lekcji dziennie</option>
              <option value={8}>Maks. 8 lekcji dziennie</option>
            </select>
          </div>
          <div class="flex items-center gap-1.5">
            <span class="text-slate-500 text-[11px]">Waga kary:</span>
            <input type="number" min="1" max="50" bind:value={editor.teacherDailyWeight} class="w-14 px-2 py-0.5 text-center bg-slate-50 border border-slate-200 rounded text-xs font-bold text-slate-800" />
          </div>
        </div>
        {#if editor.teachersWithCustomDailyLimit.length > 0}
          <div class="p-2 bg-amber-50/60 rounded-lg border border-amber-100 text-[11px] text-amber-900">
            Nauczyciele z indywidualnym limitem: {editor.teachersWithCustomDailyLimit.map(t => `${t.name} (max ${t.maxDailyLessons}h)`).join(", ")}
          </div>
        {/if}
      {/if}
    </div>

    <!-- Reguła 5c: Kompaktowy tydzień pracy kadry (ClusterBusyTimesConstraint) -->
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

    <!-- Reguła 6: Rozkład przedmiotów (SpreadEventsConstraint - Faza 3) -->
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

    <!-- Reguła 8: Poranne lekcje przedmiotów ścisłych i maturalnych (PreferTimesConstraint - Faza 3) -->
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

    <!-- Reguła 9: Popołudniowe lekcje WF i przedmiotów artystycznych (PreferTimesConstraint - Faza 3) -->
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

    <!-- Reguła 10: Lekcje równoległe i jednoczesne (LinkEventsConstraint - Faza 3) -->
    <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🔗</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Lekcje równoległe i jednoczesne (LinkEventsConstraint)</span>
            {#if editor.linkedGroups.length > 0}
              <span class="text-[10px] font-bold px-2 py-0.5 bg-purple-100 text-purple-800 rounded-full border border-purple-200">
                {editor.linkedGroups.length} grup powiązanych
              </span>
            {/if}
          </div>
          <p class="text-xs text-slate-500 mt-0.5 leading-snug">
            Wymusza odbywanie się wybranych lekcji w tym samym slocie czasowym (np. grupy językowe z klas 1A i 1B).
            {#if editor.linkedGroups.length > 0}
              <span class="text-purple-900 font-medium block mt-1">
                Aktywne powiązania: {editor.linkedGroups.map((g) => g.name).join(", ")}.
              </span>
            {:else}
              <span class="text-slate-400 italic block mt-1">
                (Brak powiązań — możesz je skonfigurować w kroku 4 przyciskiem „Lekcje równoległe”).
              </span>
            {/if}
          </p>
        </div>
      </div>
      <button
        type="button"
        onclick={() => (editor.isLinkedModalOpen = true)}
        class="text-[11px] font-semibold px-2.5 py-1 bg-purple-50 hover:bg-purple-100 text-purple-700 rounded-lg border border-purple-200 shrink-0 cursor-pointer transition-colors"
      >
        Zarządzaj powiązaniami
      </button>
    </div>

    <!-- Reguła 11: Następstwo i kolejność lekcji (OrderEventsConstraint - Faza 3) -->
    <div class="p-4 bg-white rounded-xl border border-slate-200 flex flex-col gap-3 transition-all {editor.ruleOrderLessons ? 'border-blue-300 ring-2 ring-blue-500/10' : ''}">
      <div class="flex items-start justify-between gap-4">
        <div class="flex items-start gap-3">
          <span class="text-xl">📐</span>
          <div>
            <div class="flex items-center gap-2">
              <span class="text-xs font-bold text-slate-900">Następstwo i kolejność lekcji (OrderEventsConstraint)</span>
              {#if editor.orderRules.length > 0}
                <span class="text-[10px] font-bold px-2 py-0.5 bg-blue-100 text-blue-800 rounded-full border border-blue-200">
                  {editor.orderRules.length} reguł kolejności
                </span>
              {/if}
            </div>
            <p class="text-xs text-slate-500 mt-0.5 leading-snug">
              Egzekwuje kolejność pomiędzy zajęciami (np. wykład teoretyczny musi odbyć się przed ćwiczeniami/laboratorium).
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
              Dodaj parę sekwencji (Lekcja 1 musi być PRZED Lekcją 2):
            </span>
            <div class="grid grid-cols-1 sm:grid-cols-3 gap-2">
              <div>
                <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja pierwsza:</span>
                <select
                  bind:value={editor.newOrderFirstLesson}
                  class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                >
                  <option value="">Wybierz lekcję 1...</option>
                  {#each editor.lessons as l}
                    <option value={l.id}>{l.subject} ({editor.classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                  {/each}
                </select>
              </div>

              <div>
                <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Lekcja następująca:</span>
                <select
                  bind:value={editor.newOrderSecondLesson}
                  class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                >
                  <option value="">Wybierz lekcję 2...</option>
                  {#each editor.lessons as l}
                    <option value={l.id}>{l.subject} ({editor.classes.find((c) => c.id === l.classId)?.name || l.classId})</option>
                  {/each}
                </select>
              </div>

              <div class="flex items-end gap-2">
                <div class="flex-1">
                  <span class="block text-[10px] font-semibold text-slate-600 mb-0.5">Min. odstęp:</span>
                  <select
                    bind:value={editor.newOrderSeparation}
                    class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs text-slate-800"
                  >
                    <option value={0}>W tym samym tygodniu (wcześniej)</option>
                    <option value={editor.periodsPerDay}>Min. 1 dzień odstępu</option>
                  </select>
                </div>

                <button
                  type="button"
                  onclick={() => editor.addOrderRule()}
                  class="px-3 py-1 bg-slate-900 hover:bg-slate-800 text-white font-bold rounded text-xs cursor-pointer transition-colors"
                >
                  + Dodaj
                </button>
              </div>
            </div>
          </div>

          <!-- Existing order pairs -->
          {#if editor.orderRules.length > 0}
            <div class="divide-y divide-slate-100 border border-slate-200 rounded-lg overflow-hidden bg-white">
              {#each editor.orderRules as rule}
                <div class="p-2.5 flex items-center justify-between text-xs hover:bg-slate-50">
                  <div class="flex items-center gap-2">
                    <span class="text-blue-600 font-bold">1.</span>
                    <span class="font-bold text-slate-900">{editor.lessons.find((l) => l.id === rule.firstLessonId)?.subject || rule.firstLessonId}</span>
                    <span class="text-slate-400">➔</span>
                    <span class="text-blue-600 font-bold">2.</span>
                    <span class="font-bold text-slate-900">{editor.lessons.find((l) => l.id === rule.secondLessonId)?.subject || rule.secondLessonId}</span>
                    <span class="text-slate-400 text-[11px]">
                      ({rule.minSeparation > 0 ? "min. 1 dzień odstępu" : "przed"})
                    </span>
                  </div>
                  <button
                    type="button"
                    onclick={() => editor.removeOrderRule(rule.id)}
                    class="text-slate-400 hover:text-rose-600 p-1 cursor-pointer"
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

    <!-- Reguła 12: Indywidualne niedostępności zasobów (AvoidUnavailableTimesConstraint) -->
    <div class="p-4 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <span class="text-xl">🗓️</span>
        <div>
          <div class="flex items-center gap-2">
            <span class="text-xs font-bold text-slate-900">Indywidualne wykluczenia czasowe (AvoidUnavailableTimesConstraint)</span>
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
                (Brak wykluczeń — wszyscy nauczyciele, sale i klasy są w pełni dyspozycyjni).
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
</div>

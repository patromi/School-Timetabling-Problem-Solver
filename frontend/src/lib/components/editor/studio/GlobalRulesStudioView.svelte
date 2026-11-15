<!-- frontend/src/lib/components/editor/studio/GlobalRulesStudioView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import DailyHygieneRulesSection from "./rules/DailyHygieneRulesSection.svelte";
  import WeeklyRhythmRulesSection from "./rules/WeeklyRhythmRulesSection.svelte";
  import RelationsAndOrderStudioSection from "./rules/RelationsAndOrderStudioSection.svelte";
  import Icon from "../../common/Icon.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <!-- Nagłówek -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-2">
    <h3 class="text-base font-bold text-slate-900 flex items-center gap-2">
      <Icon name="balance" size="base" class="text-slate-700" />
      <span>Ogólne zasady i standardy dydaktyczne szkoły</span>
    </h3>
    <p class="text-xs text-slate-500">
      Wszystkie aktywne reguły ogólnoszkolne brane pod uwagę przy układaniu planu. Indywidualne wyjątki (np. dni pracy konkretnego pedagoga czy dzienne ramy wybranej klasy) ustawia się bezpośrednio w profilach nauczycieli i oddziałów.
    </p>
  </div>

  <!-- 1. Standardy bezwzględne (Nienaruszalne fundamenty) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
      1. Wymogi nienaruszalne (fundamenty planu):
    </span>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-3.5">
      <div class="p-4 bg-slate-50 rounded-xl border border-slate-200/70 flex items-start justify-between gap-3 text-xs">
        <div class="flex items-start gap-3">
          <Icon name="shield" size="lg" class="text-slate-700" />
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

      <div class="p-4 bg-slate-50 rounded-xl border border-slate-200/70 flex items-start justify-between gap-3 text-xs">
        <div class="flex items-start gap-3">
          <Icon name="timer" size="lg" class="text-slate-700" />
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

  <!-- 2. Higiena dnia i okienka -->
  <DailyHygieneRulesSection {editor} />

  <!-- 3. Rytm tygodnia i pory dnia -->
  <WeeklyRhythmRulesSection {editor} />

  <!-- 4. Relacje i kolejność -->
  <RelationsAndOrderStudioSection {editor} />
</div>

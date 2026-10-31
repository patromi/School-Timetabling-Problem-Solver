<!-- frontend/src/lib/components/editor/steps/Step6SummaryExport.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
    isSaving: boolean;
    onSave: () => void;
    onNavigateStep?: (step: number) => void;
  }

  let { editor, isSaving, onSave, onNavigateStep }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between pb-4 border-b border-slate-100">
      <div>
        <h3 class="text-lg font-bold text-slate-900">{editor.schoolName || "Nowa szkoła (brak nazwy)"}</h3>
        <p class="text-xs text-slate-500">Identyfikator instancji: {editor.schoolId || "niezdefiniowany"}</p>
      </div>
      {#if editor.isSchoolValid}
        <span class="px-3 py-1 bg-emerald-50 text-emerald-800 rounded-full font-semibold text-xs border border-emerald-200 flex items-center gap-1.5">
          <span>✓</span>
          <span>Gotowa do zapisu</span>
        </span>
      {:else}
        <span class="px-3 py-1 bg-rose-50 text-rose-800 rounded-full font-semibold text-xs border border-rose-200 flex items-center gap-1.5">
          <span>⚠️</span>
          <span>Do uzupełnienia ({editor.validationIssues.length})</span>
        </span>
      {/if}
    </div>

    <!-- Lista brakujących elementów do uzupełnienia przed zapisem -->
    {#if editor.validationIssues.length > 0}
      <div class="p-4 bg-rose-50 border border-rose-200 rounded-2xl flex flex-col gap-3 text-xs">
        <div class="flex items-center gap-2 font-bold text-rose-900">
          <span>⚠️</span>
          <span>Przed zapisem arkusza należy uzupełnić następujące dane:</span>
        </div>
        <div class="grid grid-cols-1 sm:grid-cols-2 gap-2">
          {#each editor.validationIssues as issue}
            <div class="p-3 bg-white border border-rose-200/80 rounded-xl flex items-center justify-between gap-2 shadow-2xs">
              <div>
                <span class="font-bold text-slate-900 block">{issue.text}</span>
                <span class="text-[10px] text-slate-400">Krok {issue.step}</span>
              </div>
              {#if onNavigateStep}
                <button
                  type="button"
                  onclick={() => onNavigateStep(issue.step)}
                  class="px-3 py-1 bg-rose-100 hover:bg-rose-200 text-rose-800 text-[11px] font-bold rounded-lg transition-colors cursor-pointer shrink-0"
                >
                  Przejdź →
                </button>
              {/if}
            </div>
          {/each}
        </div>
      </div>
    {/if}

    <!-- Bilans -->
    <div class="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs">
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
        <span class="text-slate-500">Oddziały klasowe:</span>
        <span class="text-xl font-bold text-slate-900 mt-1">{editor.classes.length} oddziałów</span>
      </div>
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
        <span class="text-slate-500">Kadra pedagogiczna:</span>
        <span class="text-xl font-bold text-slate-900 mt-1">{editor.teachers.length} nauczycieli</span>
      </div>
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
        <span class="text-slate-500">Sale lekcyjne:</span>
        <span class="text-xl font-bold text-slate-900 mt-1">{editor.rooms.length} pomieszczeń</span>
      </div>
      <div class="p-3 bg-slate-50 rounded-xl border border-slate-100 flex flex-col">
        <span class="text-slate-500">Wymiar tygodniowy:</span>
        <span class="text-xl font-bold text-slate-900 mt-1">{editor.totalHoursAssigned} godzin</span>
      </div>
    </div>

    <!-- Checklist -->
    <div class="mt-2 flex flex-col gap-2 text-xs">
      <div class="flex items-center gap-2 text-emerald-800">
        <span>✓</span>
        <span>Struktura tygodnia: {editor.selectedDays.length} dni roboczych ({editor.periodsPerDay} lekcji dziennie)</span>
      </div>
      {#if editor.classes.length > 0 && editor.classesWithoutLessons.length === 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>Wszystkie klasy mają przypisane lekcje i pedagogów ({editor.classes.length} oddziałów)</span>
        </div>
      {:else if editor.classes.length > 0}
        <div class="flex items-center gap-2 text-amber-800 font-semibold">
          <span>⚠️</span>
          <span>Klasy bez przypisanych zajęć: {editor.classesWithoutLessons.map((c) => c.name).join(", ")}</span>
        </div>
      {/if}
      <div class="flex items-center gap-2 text-emerald-800">
        <span>✓</span>
        <span>Wymogi bezwzględne oraz preferencje dydaktyczne zostały poprawnie skonfigurowane</span>
      </div>
      {#if editor.totalUnavailabilitiesCount > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Uwzględniono {editor.totalUnavailabilitiesCount} godzin wykluczeń czasowych (dyspozycyjność dla {editor.resourcesWithUnavailability.length} osób/sal)
          </span>
        </div>
      {/if}
      {#if editor.teachersWithCustomDays.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Zdefiniowano kompaktowy tydzień pracy dla {editor.teachersWithCustomDays.length} pedagogów (niepełny etat / dojazdy)
          </span>
        </div>
      {/if}
      {#if editor.teachersWithCustomDailyLimit.length > 0 || editor.classesWithCustomDailyLimit.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Skonfigurowano indywidualne limity obciążenia dziennego klas i kadry
          </span>
        </div>
      {/if}
      {#if editor.linkedGroups.length > 0}
        <div class="flex items-center gap-2 text-purple-800">
          <span>✓</span>
          <span>
            Skonfigurowano {editor.linkedGroups.length} grup lekcji równoległych i jednoczesnych
          </span>
        </div>
      {/if}
      {#if editor.ruleOrderLessons && editor.orderRules.length > 0}
        <div class="flex items-center gap-2 text-blue-800">
          <span>✓</span>
          <span>
            Wymuszono reguły chronologicznej kolejności dla {editor.orderRules.length} par zajęć
          </span>
        </div>
      {/if}
      {#if editor.ruleMorningCoreSubjects || editor.ruleAfternoonFlexSubjects}
        <div class="flex items-center gap-2 text-amber-800">
          <span>✓</span>
          <span>
            Włączono preferencje pory dnia (np. przedmioty ścisłe rano, WF po południu)
          </span>
        </div>
      {/if}
    </div>

    <!-- Save CTA -->
    <div class="mt-4 pt-5 border-t border-slate-100 flex items-center justify-between">
      <p class="text-xs text-slate-500">
        Kliknij poniższy przycisk, aby zapisać szkołę i przejść do eksploratora.
      </p>
      <button
        type="button"
        onclick={onSave}
        disabled={isSaving || !editor.isSchoolValid}
        class="px-6 py-2.5 bg-blue-600 hover:bg-blue-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-2"
      >
        {#if isSaving}
          <span class="animate-spin">⏳</span>
          <span>Zapisywanie w toku...</span>
        {:else}
          <span>💾 Zapisz szkołę do bazy</span>
        {/if}
      </button>
    </div>
  </div>
</div>

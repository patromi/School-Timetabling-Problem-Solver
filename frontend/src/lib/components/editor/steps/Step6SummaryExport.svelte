<!-- frontend/src/lib/components/editor/steps/Step6SummaryExport.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
    isSaving: boolean;
    onSave: () => void;
  }

  let { editor, isSaving, onSave }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between pb-4 border-b border-slate-100">
      <div>
        <h3 class="text-lg font-bold text-slate-900">{editor.schoolName}</h3>
        <p class="text-xs text-slate-500">Identyfikator instancji: {editor.schoolId}</p>
      </div>
      <span class="px-3 py-1 bg-emerald-50 text-emerald-800 rounded-full font-semibold text-xs border border-emerald-200">
        Gotowa do zapisu
      </span>
    </div>

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
      <div class="flex items-center gap-2 text-emerald-800">
        <span>✓</span>
        <span>Wszystkie klasy mają przypisane lekcje i pedagogów</span>
      </div>
      <div class="flex items-center gap-2 text-emerald-800">
        <span>✓</span>
        <span>Wymogi bezwzględne oraz preferencje dydaktyczne zostały poprawnie skonfigurowane</span>
      </div>
      {#if editor.totalUnavailabilitiesCount > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Uwzględniono {editor.totalUnavailabilitiesCount} godzin wykluczeń czasowych dla {editor.resourcesWithUnavailability.length} zasobów (AvoidUnavailableTimesConstraint)
          </span>
        </div>
      {/if}
      {#if editor.teachersWithCustomDays.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Zdefiniowano kompaktowy tydzień pracy (ClusterBusyTimes) dla {editor.teachersWithCustomDays.length} pedagogów
          </span>
        </div>
      {/if}
      {#if editor.teachersWithCustomDailyLimit.length > 0 || editor.classesWithCustomDailyLimit.length > 0}
        <div class="flex items-center gap-2 text-emerald-800">
          <span>✓</span>
          <span>
            Skonfigurowano indywidualne limity obciążenia dziennego (LimitBusyTimes)
          </span>
        </div>
      {/if}
      {#if editor.linkedGroups.length > 0}
        <div class="flex items-center gap-2 text-purple-800">
          <span>✓</span>
          <span>
            Skonfigurowano {editor.linkedGroups.length} grup lekcji równoległych / jednoczesnych (LinkEventsConstraint)
          </span>
        </div>
      {/if}
      {#if editor.ruleOrderLessons && editor.orderRules.length > 0}
        <div class="flex items-center gap-2 text-blue-800">
          <span>✓</span>
          <span>
            Wymuszono reguły kolejności dla {editor.orderRules.length} par zajęć (OrderEventsConstraint)
          </span>
        </div>
      {/if}
      {#if editor.ruleMorningCoreSubjects || editor.ruleAfternoonFlexSubjects}
        <div class="flex items-center gap-2 text-amber-800">
          <span>✓</span>
          <span>
            Włączono dydaktyczne preferencje pory dnia dla przedmiotów (PreferTimesConstraint)
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

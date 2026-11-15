<!-- frontend/src/lib/components/editor/studio/rules/RelationsAndOrderStudioSection.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import WeightAdjuster from "./WeightAdjuster.svelte";
  import Icon from "../../../common/Icon.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  function lessonLabel(lessonId: string): string {
    const l = editor.lessons.find((item) => item.id === lessonId);
    if (!l) return lessonId;
    const cls = editor.classes.find((c) => c.id === l.classId);
    return `${l.subject} (${cls?.name || l.classId})`;
  }
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
  <span class="text-xs font-bold text-slate-800 uppercase tracking-wider block">
    4. Relacje między zajęciami i chronologia:
  </span>

  <!-- Zajęcia jednoczesne -->
  <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3">
    <div class="flex items-center justify-between gap-4">
      <div>
        <span class="text-xs font-bold text-slate-900 block">
          Zajęcia jednoczesne i grupy międzyoddziałowe ({editor.linkedGroups.length})
        </span>
        <p class="text-[11px] text-slate-500 mt-0.5">
          Wymusza odbywanie się wybranych lekcji w tym samym slocie czasowym (np. grupy językowe z klas 1A i 1B lub łączony WF).
        </p>
      </div>

      <button
        type="button"
        onclick={() => (editor.isLinkedModalOpen = true)}
        class="px-3 py-1.5 bg-slate-900 hover:bg-slate-800 text-white rounded-xl text-xs font-semibold cursor-pointer shadow-xs transition-colors shrink-0 flex items-center gap-1.5"
      >
        <Icon name="link" size="xs" class="text-white" />
        <span>Zarządzaj grupami</span>
      </button>
    </div>

    {#if editor.linkedGroups.length > 0}
      <div class="p-2.5 bg-white rounded-xl border border-slate-200 text-xs text-slate-700">
        Aktywne grupy: <strong>{editor.linkedGroups.map((g) => g.name).join(", ")}</strong>
      </div>
    {/if}
  </div>

  <!-- Kolejność lekcji (OrderEventsConstraint) -->
  <div class="p-4 bg-slate-50/70 rounded-xl border border-slate-200/80 flex flex-col gap-3 text-xs">
    <div class="flex items-start justify-between gap-4">
      <div class="flex items-start gap-3">
        <Icon name="low_priority" size="lg" class="text-blue-600 shrink-0 mt-0.5" />
        <div>
          <div class="flex flex-wrap items-center gap-2">
            <span class="font-bold text-slate-900">Kolejność lekcji (np. wykład przed ćwiczeniami)</span>
            {#if editor.orderRules.length > 0}
              <span class="text-[10px] font-bold px-2 py-0.5 bg-blue-100 text-blue-800 rounded-full border border-blue-200">
                {editor.orderRules.length} par
              </span>
            {/if}
          </div>
          <p class="text-slate-500 text-[11px] mt-0.5 leading-snug">
            Egzekwuje chronologię między wybranymi lekcjami w tygodniu (Lekcja 1 musi wypaść przed Lekcją 2).
          </p>
        </div>
      </div>
      <input type="checkbox" bind:checked={editor.ruleOrderLessons} class="w-4 h-4 accent-blue-600 cursor-pointer mt-1" />
    </div>

    {#if editor.ruleOrderLessons}
      <div class="pt-3 border-t border-slate-200/60 flex flex-col gap-3">
        <!-- Rygor reguły -->
        <div class="flex flex-wrap items-center justify-between gap-2">
          <span class="text-slate-500 text-[11px]">Rygor kolejności:</span>
          <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
            <button
              type="button"
              onclick={() => (editor.newOrderIsSoft = false)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer flex items-center gap-1 {!editor.newOrderIsSoft ? 'bg-rose-600 text-white' : 'text-slate-600'}"
            >
              <Icon name="lock" size="xs" class={!editor.newOrderIsSoft ? 'text-white' : 'text-rose-600'} />
              <span>Bezwzględny</span>
            </button>
            <button
              type="button"
              onclick={() => (editor.newOrderIsSoft = true)}
              class="px-2.5 py-0.5 rounded text-[11px] font-semibold cursor-pointer flex items-center gap-1 {editor.newOrderIsSoft ? 'bg-amber-600 text-white' : 'text-slate-600'}"
            >
              <Icon name="tune" size="xs" class={editor.newOrderIsSoft ? 'text-white' : 'text-slate-500'} />
              <span>Preferencja</span>
            </button>
          </div>
        </div>

        {#if editor.newOrderIsSoft}
          <WeightAdjuster label="Waga kary:" bind:value={editor.newOrderWeight} />
        {/if}

        <!-- Dodawanie pary -->
        <div class="p-3 bg-white rounded-xl border border-slate-200/80 flex flex-col gap-2.5">
          <span class="font-bold text-slate-700 text-[11px]">
            Dodaj nową parę chronologiczną (Lekcja 1 musi odbyć się przed Lekcją 2):
          </span>
          <div class="grid grid-cols-1 sm:grid-cols-3 gap-2">
            <div>
              <label for="order-first-select" class="block text-[10px] font-semibold text-slate-500 mb-0.5">Lekcja wcześniejsza (1):</label>
              <select
                id="order-first-select"
                bind:value={editor.newOrderFirstLesson}
                class="w-full px-2 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800"
              >
                <option value="">Wybierz pierwszą lekcję...</option>
                {#each editor.lessons as l}
                  <option value={l.id}>{lessonLabel(l.id)}</option>
                {/each}
              </select>
            </div>

            <div>
              <label for="order-second-select" class="block text-[10px] font-semibold text-slate-500 mb-0.5">Lekcja późniejsza (2):</label>
              <select
                id="order-second-select"
                bind:value={editor.newOrderSecondLesson}
                class="w-full px-2 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800"
              >
                <option value="">Wybierz kolejną lekcję...</option>
                {#each editor.lessons as l}
                  <option value={l.id}>{lessonLabel(l.id)}</option>
                {/each}
              </select>
            </div>

            <div class="flex items-end gap-2">
              <div class="flex-1">
                <label for="order-sep-select" class="block text-[10px] font-semibold text-slate-500 mb-0.5">Wymagany odstęp:</label>
                <select
                  id="order-sep-select"
                  bind:value={editor.newOrderSeparation}
                  class="w-full px-2 py-1.5 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800"
                >
                  <option value={0}>Bezpośrednio lub później</option>
                  <option value={editor.periodsPerDay}>Co najmniej 1 dzień przerwy</option>
                </select>
              </div>
              <button
                type="button"
                onclick={() => editor.addOrderRule()}
                class="px-3.5 py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-bold rounded-lg text-xs cursor-pointer transition-colors shrink-0 flex items-center gap-1"
              >
                <Icon name="add" size="xs" class="text-white" />
                <span>Dodaj</span>
              </button>
            </div>
          </div>
        </div>

        <!-- Lista par -->
        {#if editor.orderRules.length > 0}
          <div class="divide-y divide-slate-100 border border-slate-200 rounded-xl overflow-hidden bg-white">
            {#each editor.orderRules as rule}
              <div class="p-2.5 flex items-center justify-between text-xs hover:bg-slate-50 transition-colors">
                <div class="flex flex-wrap items-center gap-2">
                  <span class="w-5 h-5 rounded-full bg-blue-100 text-blue-800 font-bold flex items-center justify-center text-[10px]">1</span>
                  <span class="font-bold text-slate-900">{lessonLabel(rule.firstLessonId)}</span>
                  <Icon name="arrow_forward" size="xs" class="text-slate-400" />
                  <span class="w-5 h-5 rounded-full bg-blue-100 text-blue-800 font-bold flex items-center justify-center text-[10px]">2</span>
                  <span class="font-bold text-slate-900">{lessonLabel(rule.secondLessonId)}</span>
                  <span class="text-slate-500 text-[11px]">
                    ({rule.minSeparation > 0 ? `odstęp min. ${rule.minSeparation} godz.` : "wcześniej w tygodniu"})
                  </span>
                </div>
                <button
                  type="button"
                  onclick={() => editor.removeOrderRule(rule.id)}
                  class="text-rose-500 hover:text-rose-700 font-bold p-1 rounded-md hover:bg-rose-50 cursor-pointer transition-colors"
                  title="Usuń tę regułę"
                >
                  <Icon name="close" size="xs" />
                </button>
              </div>
            {/each}
          </div>
        {/if}
      </div>
    {/if}
  </div>
</div>

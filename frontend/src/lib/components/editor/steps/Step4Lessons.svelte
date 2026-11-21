<!-- frontend/src/lib/components/editor/steps/Step4Lessons.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import AllLessonsTab from "./lessons/AllLessonsTab.svelte";
  import ByClassTab from "./lessons/ByClassTab.svelte";
  import ByTeacherTab from "./lessons/ByTeacherTab.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-5">
  <!-- Pasek zakładek Krok 4 -->
  <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
    <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
      <button
        type="button"
        onclick={() => (editor.step4ActiveTab = "lessons")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "lessons"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>📋 Wszystkie lekcje</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step4ActiveTab === 'lessons' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.lessons.length}
        </span>
      </button>
      <button
        type="button"
        onclick={() => (editor.step4ActiveTab = "byClass")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "byClass"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>🏫 Widok klas i roczników</span>
      </button>
      <button
        type="button"
        onclick={() => {
          editor.step4ActiveTab = "byTeacher";
          if (!editor.selectedTeacherViewId && editor.teachers[0]) {
            editor.selectedTeacherViewId = editor.teachers[0].id;
          }
        }}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step4ActiveTab === "byTeacher"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👨‍🏫 Widok nauczycieli</span>
      </button>
    </div>

    <div class="text-xs text-slate-500 px-3 py-1">
      {#if editor.step4ActiveTab === "lessons"}
        Łącznie zaplanowano: <strong>{editor.totalHoursAssigned} godzin lekcyjnych</strong> w tygodniu.
      {:else if editor.step4ActiveTab === "byClass"}
        Przeglądaj plany poszczególnych klas, edytuj siatki i przypisuj przedmioty do roczników.
      {:else}
        Podgląd klas każdego nauczyciela oraz weryfikacja zasady spójności przydziału.
      {/if}
    </div>
  </div>

  {#if editor.step4ActiveTab === "lessons"}
    <AllLessonsTab {editor} />
  {:else if editor.step4ActiveTab === "byClass"}
    <ByClassTab {editor} />
  {:else}
    <ByTeacherTab {editor} />
  {/if}
</div>

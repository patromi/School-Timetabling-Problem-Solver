<!-- frontend/src/lib/components/editor/steps/Step2Classes.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import ClassesTab from "./classes/ClassesTab.svelte";
  import TeachersTab from "./classes/TeachersTab.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-5">
  <!-- Pasek zakładek Krok 2 -->
  <div class="flex flex-wrap items-center justify-between gap-3 bg-white p-2 border border-slate-200/90 rounded-2xl shadow-xs">
    <div class="flex items-center gap-1.5 p-1 bg-slate-100/80 rounded-xl">
      <button
        type="button"
        onclick={() => (editor.step2ActiveTab = "classes")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step2ActiveTab === "classes"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👥 Klasy i roczniki</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step2ActiveTab === 'classes' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.classes.length}
        </span>
      </button>
      <button
        type="button"
        onclick={() => (editor.step2ActiveTab = "teachers")}
        class={`px-4 py-2 text-xs font-bold rounded-lg transition-all cursor-pointer flex items-center gap-2 ${
          editor.step2ActiveTab === "teachers"
            ? "bg-white text-blue-700 shadow-xs"
            : "text-slate-600 hover:text-slate-900"
        }`}
      >
        <span>👨‍🏫 Kadra i pensum</span>
        <span class="px-1.5 py-0.5 rounded-full text-[10px] {editor.step2ActiveTab === 'teachers' ? 'bg-blue-100 text-blue-800' : 'bg-slate-200 text-slate-600'}">
          {editor.teachers.length}
        </span>
      </button>
    </div>

    <div class="text-xs text-slate-500 px-3 py-1">
      {#if editor.step2ActiveTab === "classes"}
        Zarządzaj poziomami klas, edytuj nazwy, powielaj siatki godzin i ustalaj ograniczenia dzienne.
      {:else}
        Ustalaj pensum dydaktyczne (etat), edytuj dane pedagogów i monitoruj zbilansowanie godzin kadry.
      {/if}
    </div>
  </div>

  {#if editor.step2ActiveTab === "classes"}
    <ClassesTab editor={editor} />
  {:else}
    <TeachersTab editor={editor} />
  {/if}
</div>

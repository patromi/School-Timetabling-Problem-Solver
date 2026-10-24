<!-- frontend/src/lib/components/editor/steps/lessons/AddLessonBar.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col gap-3">
  <span class="text-xs font-bold text-slate-800">Dodaj nową pozycję do planu:</span>
  <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-6 gap-2.5 text-xs">
    <!-- Przedmiot -->
    <div>
      <label for="lesson-subject-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Przedmiot</label>
      <input
        id="lesson-subject-input"
        type="text"
        bind:value={editor.lessonSubject}
        placeholder="np. Matematyka"
        class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
      />
    </div>

    <!-- Klasa -->
    <div>
      <label for="lesson-class-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Oddział / Klasa</label>
      <select
        id="lesson-class-input"
        bind:value={editor.lessonClassId}
        class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
      >
        {#each editor.classes as c}
          <option value={c.id}>{c.name}</option>
        {/each}
      </select>
    </div>

    <!-- Nauczyciel -->
    <div>
      <label for="lesson-teacher-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Nauczyciel</label>
      <select
        id="lesson-teacher-input"
        bind:value={editor.lessonTeacherId}
        class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
      >
        {#each editor.teachers as t}
          <option value={t.id}>{t.name} ({t.subject})</option>
        {/each}
      </select>
    </div>

    <!-- Sala -->
    <div>
      <label for="lesson-room-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Sala</label>
      <select
        id="lesson-room-input"
        bind:value={editor.lessonRoomId}
        class="w-full py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs text-slate-800 focus:outline-none focus:border-blue-500"
      >
        <option value="">Dowolna</option>
        {#each editor.rooms as r}
          <option value={r.id}>{r.name}</option>
        {/each}
      </select>
    </div>

    <!-- Wymiar godzin -->
    <div>
      <label for="lesson-count-input" class="block text-[10px] uppercase font-bold text-slate-400 mb-1">Godz./tydzień</label>
      <div class="flex items-center gap-1.5">
        <input
          id="lesson-count-input"
          type="number"
          min="1"
          max="10"
          bind:value={editor.lessonCount}
          class="w-12 py-1.5 px-1 bg-slate-50 border border-slate-200 rounded-lg text-xs font-bold text-center text-slate-800"
        />
        <label class="flex items-center gap-1 text-[11px] text-slate-500 cursor-pointer">
          <input type="checkbox" bind:checked={editor.lessonIsBlock} class="accent-blue-600 rounded" />
          <span>Blok 2h</span>
        </label>
      </div>
    </div>

    <!-- Przycisk dodaj -->
    <div class="flex items-end">
      <button
        type="button"
        onclick={() => editor.addCustomLesson()}
        class="w-full py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-semibold rounded-lg transition-all cursor-pointer text-center"
      >
        + Dodaj lekcję
      </button>
    </div>
  </div>
</div>

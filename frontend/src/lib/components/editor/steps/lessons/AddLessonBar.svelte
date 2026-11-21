<!-- frontend/src/lib/components/editor/steps/lessons/AddLessonBar.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";
  import SubjectCombobox from "../../common/SubjectCombobox.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  function handleSubjectChange(sub: string) {
    editor.lessonSubject = sub;
    if (sub.trim()) {
      editor.registerSubject(sub.trim());
      // Jeśli wybrany nauczyciel nie uczy tego przedmiotu, spróbuj podpowiedzieć odpowiedniego pedagoga
      const matchingTeacher = editor.teachers.find(
        (t) => t.subject?.toLowerCase() === sub.toLowerCase()
      );
      if (matchingTeacher) {
        editor.lessonTeacherId = matchingTeacher.id;
      }
    }
  }
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-4 sm:p-5 shadow-xs flex flex-col gap-3.5">
  <div class="flex items-center justify-between">
    <div class="flex items-center gap-2">
      <span class="w-6 h-6 rounded-lg bg-blue-50 text-blue-700 flex items-center justify-center font-bold text-xs">
        +
      </span>
      <span class="text-xs font-bold text-slate-900">Dodaj nową lekcję do planu zajęć:</span>
    </div>
    <span class="text-[11px] text-slate-400 hidden sm:inline">
      Prowadzący i sala zostaną automatycznie dopasowani do wybranego przedmiotu
    </span>
  </div>

  <!-- Row 1: Przedmiot, Oddział, Prowadzący, Sala -->
  <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-12 gap-3 text-xs">
    <!-- Przedmiot (4 cols) -->
    <div class="lg:col-span-4">
      <label for="lesson-subject-input" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Przedmiot <span class="text-slate-400 font-normal">(wybierz lub wpisz nowy)</span>:
      </label>
      <SubjectCombobox
        id="lesson-subject-input"
        bind:value={editor.lessonSubject}
        subjects={editor.allKnownSubjects}
        onSelect={handleSubjectChange}
        onAddNew={(newSub) => editor.registerSubject(newSub)}
        placeholder="Wybierz lub wpisz przedmiot..."
      />
    </div>

    <!-- Oddział / Klasa (2 cols) -->
    <div class="lg:col-span-2">
      <label for="lesson-class-input" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Oddział / Klasa:
      </label>
      <select
        id="lesson-class-input"
        bind:value={editor.lessonClassId}
        class="w-full py-2 px-2.5 bg-slate-50 border border-slate-200 rounded-xl text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 focus:bg-white transition-all cursor-pointer"
      >
        {#each editor.classes as c}
          <option value={c.id}>{c.name}</option>
        {/each}
      </select>
    </div>

    <!-- Nauczyciel (4 cols - dużo miejsca na pełne imię, nazwisko i specjalizację) -->
    <div class="lg:col-span-4">
      <label for="lesson-teacher-input" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Nauczyciel prowadzący:
      </label>
      <select
        id="lesson-teacher-input"
        bind:value={editor.lessonTeacherId}
        class="w-full py-2 px-2.5 bg-slate-50 border border-slate-200 rounded-xl text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 focus:bg-white transition-all cursor-pointer"
      >
        {#each editor.teachers as t}
          <option value={t.id}>{t.name} ({t.subject})</option>
        {/each}
      </select>
    </div>

    <!-- Sala (2 cols) -->
    <div class="lg:col-span-2">
      <label for="lesson-room-input" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Dedykowana sala:
      </label>
      <select
        id="lesson-room-input"
        bind:value={editor.lessonRoomId}
        class="w-full py-2 px-2.5 bg-slate-50 border border-slate-200 rounded-xl text-xs font-medium text-slate-800 focus:outline-none focus:border-blue-500 focus:bg-white transition-all cursor-pointer"
      >
        <option value="">Dowolna sala</option>
        {#each editor.rooms as r}
          <option value={r.id}>{r.name}</option>
        {/each}
      </select>
    </div>
  </div>

  <!-- Row 2: Wymiar godzin, łączenie w bloki i przycisk zatwierdzenia -->
  <div class="pt-2 border-t border-slate-100 flex flex-wrap items-center justify-between gap-3 text-xs">
    <div class="flex flex-wrap items-center gap-4">
      <!-- Wymiar godzin -->
      <div class="flex items-center gap-2">
        <label for="lesson-count-input" class="text-[11px] font-semibold text-slate-700">
          Liczba godzin w tygodniu:
        </label>
        <div class="flex items-center gap-1.5">
          <input
            id="lesson-count-input"
            type="number"
            min="1"
            max="12"
            bind:value={editor.lessonCount}
            class="w-14 py-1.5 px-2 bg-slate-50 border border-slate-200 rounded-lg text-xs font-bold text-center text-slate-800 focus:bg-white focus:outline-none focus:border-blue-500"
          />
          <span class="text-slate-500 text-[11px]">godz./tydzień</span>
        </div>
      </div>

      <!-- Blok 2-godzinny -->
      <label class="flex items-center gap-2 text-xs font-medium text-slate-700 cursor-pointer select-none bg-slate-50 hover:bg-slate-100 px-3 py-1.5 rounded-xl border border-slate-200/80 transition-colors">
        <input
          type="checkbox"
          bind:checked={editor.lessonIsBlock}
          class="w-3.5 h-3.5 accent-blue-600 rounded cursor-pointer"
        />
        <span>Łącz w blok dwugodzinny (2h pod rząd)</span>
      </label>
    </div>

    <!-- Przycisk dodania -->
    <button
      type="button"
      onclick={() => editor.addCustomLesson()}
      class="px-5 py-2 bg-slate-900 hover:bg-blue-600 text-white font-bold rounded-xl transition-all shadow-xs cursor-pointer flex items-center gap-1.5"
    >
      <span>+</span>
      <span>Dodaj lekcję do siatki</span>
    </button>
  </div>
</div>

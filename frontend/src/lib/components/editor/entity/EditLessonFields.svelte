<script lang="ts">
  import type { SimpleClass, SimpleTeacher, SimpleRoom } from "../types";
  import SubjectCombobox from "../common/SubjectCombobox.svelte";

  interface Props {
    lessonSubject: string;
    lessonClassId: string;
    lessonTeacherId: string;
    lessonRoomId: string;
    lessonWeeklyCount: number;
    lessonDuration: number;
    classes: SimpleClass[];
    teachers: SimpleTeacher[];
    rooms: SimpleRoom[];
    subjects?: string[];
  }

  let {
    lessonSubject = $bindable(),
    lessonClassId = $bindable(),
    lessonTeacherId = $bindable(),
    lessonRoomId = $bindable(),
    lessonWeeklyCount = $bindable(),
    lessonDuration = $bindable(),
    classes,
    teachers,
    rooms,
    subjects = [],
  }: Props = $props();
</script>

<div>
  <label for="edit-lesson-subject" class="block font-bold text-slate-700 mb-1">Nazwa przedmiotu:</label>
  <SubjectCombobox
    id="edit-lesson-subject"
    bind:value={lessonSubject}
    {subjects}
    placeholder="Wybierz lub wpisz..."
  />
</div>

<div class="grid grid-cols-2 gap-3">
  <div>
    <label for="edit-lesson-class" class="block font-bold text-slate-700 mb-1">Oddział (klasa):</label>
    <select
      id="edit-lesson-class"
      bind:value={lessonClassId}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    >
      {#each classes as c}
        <option value={c.id}>{c.name || c.id}</option>
      {/each}
    </select>
  </div>
  <div>
    <label for="edit-lesson-teacher" class="block font-bold text-slate-700 mb-1">Nauczyciel prowadzący:</label>
    <select
      id="edit-lesson-teacher"
      bind:value={lessonTeacherId}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    >
      {#each teachers as t}
        <option value={t.id}>{t.name || t.id}</option>
      {/each}
    </select>
  </div>
</div>

<div class="grid grid-cols-3 gap-3">
  <div>
    <label for="edit-lesson-room" class="block font-bold text-slate-700 mb-1">Preferowana sala:</label>
    <select
      id="edit-lesson-room"
      bind:value={lessonRoomId}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    >
      <option value="">(Dowolna / brak)</option>
      {#each rooms as r}
        <option value={r.id}>{r.name || r.id}</option>
      {/each}
    </select>
  </div>
  <div>
    <label for="edit-lesson-count" class="block font-bold text-slate-700 mb-1">Liczba godzin/tydz:</label>
    <input
      id="edit-lesson-count"
      type="number"
      min="1"
      max="15"
      bind:value={lessonWeeklyCount}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
    />
  </div>
  <div>
    <label for="edit-lesson-duration" class="block font-bold text-slate-700 mb-1">Format zajęć:</label>
    <select
      id="edit-lesson-duration"
      bind:value={lessonDuration}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    >
      <option value={1}>Pojedyncza (45 min)</option>
      <option value={2}>Blok (90 min)</option>
    </select>
  </div>
</div>

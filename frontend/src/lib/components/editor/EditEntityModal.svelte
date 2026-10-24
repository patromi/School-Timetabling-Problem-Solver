<!-- frontend/src/lib/components/editor/EditEntityModal.svelte -->
<script lang="ts">
  import type { EditorState } from "./editorState.svelte";
  import type { RoomType } from "./types";

  interface Props {
    editor: EditorState;
    target: {
      type: "class" | "teacher" | "room" | "lesson";
      data: any;
    };
    onClose: () => void;
  }

  let { editor, target, onClose }: Props = $props();

  // Pola robocze
  let name = $state("");
  let id = $state("");
  let grade = $state<number>(1);
  let subject = $state("");
  let workload = $state<number>(18);
  let roomType = $state<RoomType>("general");

  // Dla lekcji
  let lessonSubject = $state("");
  let lessonClassId = $state("");
  let lessonTeacherId = $state("");
  let lessonRoomId = $state("");
  let lessonWeeklyCount = $state<number>(1);
  let lessonDuration = $state<number>(1);

  $effect(() => {
    name = target.data.name || "";
    id = target.data.id || "";
    grade = target.data.grade || 1;
    subject = target.data.subject || "";
    workload = target.data.workload || 18;
    roomType = target.data.type || "general";
    lessonSubject = target.data.subject || "";
    lessonClassId = target.data.classId || "";
    lessonTeacherId = target.data.teacherId || "";
    lessonRoomId = target.data.roomId || "";
    lessonWeeklyCount = target.data.weeklyCount || 1;
    lessonDuration = target.data.duration || 1;
  });

  function handleSave() {
    if (target.type === "class") {
      if (!name.trim()) {
        alert("Podaj nazwę klasy!");
        return;
      }
      editor.updateClass(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        grade: Number(grade) || 1,
      });
    } else if (target.type === "teacher") {
      if (!name.trim()) {
        alert("Podaj imię i nazwisko nauczyciela!");
        return;
      }
      editor.updateTeacher(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        subject: subject.trim() || "Przedmiot ogólny",
        workload: Number(workload) || 18,
      });
    } else if (target.type === "room") {
      if (!name.trim()) {
        alert("Podaj nazwę sali!");
        return;
      }
      editor.updateRoom(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        type: roomType,
      });
    } else if (target.type === "lesson") {
      if (!lessonSubject.trim() || !lessonClassId || !lessonTeacherId) {
        alert("Wypełnij wymagane pola lekcji!");
        return;
      }
      editor.updateLesson(target.data.id, {
        subject: lessonSubject.trim(),
        classId: lessonClassId,
        teacherId: lessonTeacherId,
        roomId: lessonRoomId,
        weeklyCount: Number(lessonWeeklyCount) || 1,
        duration: Number(lessonDuration) || 1,
      });
    }

    onClose();
  }
</script>

<div
  class="fixed inset-0 bg-slate-900/60 backdrop-blur-xs flex items-center justify-center p-4 z-50 animate-in fade-in duration-150"
  role="dialog"
  aria-modal="true"
>
  <div class="bg-white rounded-2xl border border-slate-200 shadow-2xl w-full max-w-lg flex flex-col overflow-hidden animate-in zoom-in-95 duration-150">
    <!-- Header -->
    <div class="px-6 py-4 border-b border-slate-100 flex items-center justify-between shrink-0 bg-slate-50/60">
      <div class="flex items-center gap-2.5">
        <span class="text-xl">
          {#if target.type === 'class'}👥{:else if target.type === 'teacher'}👨‍🏫{:else if target.type === 'room'}🏫{:else}📚{/if}
        </span>
        <div>
          <h3 class="text-sm font-bold text-slate-900">
            {#if target.type === 'class'}
              Edycja oddziału klasowego
            {:else if target.type === 'teacher'}
              Edycja nauczyciela
            {:else if target.type === 'room'}
              Edycja sali lekcyjnej
            {:else}
              Edycja lekcji
            {/if}
          </h3>
          <p class="text-[11px] text-slate-500">
            Zaktualizuj dane i nazwy w instancji
          </p>
        </div>
      </div>
      <button
        type="button"
        onclick={onClose}
        class="w-8 h-8 rounded-lg hover:bg-slate-200/60 text-slate-400 hover:text-slate-600 flex items-center justify-center text-sm font-bold transition-all cursor-pointer"
      >
        ✕
      </button>
    </div>

    <!-- Body -->
    <div class="p-6 flex flex-col gap-4 text-xs">
      {#if target.type === 'class'}
        <!-- KLASA -->
        <div>
          <label for="edit-class-name" class="block font-bold text-slate-700 mb-1">Nazwa oddziału:</label>
          <input
            id="edit-class-name"
            type="text"
            bind:value={name}
            placeholder="np. Klasa 1A (Mat-Fiz)"
            class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label for="edit-class-grade" class="block font-bold text-slate-700 mb-1">Rocznik (klasa):</label>
            <input
              id="edit-class-grade"
              type="number"
              min="1"
              max="8"
              bind:value={grade}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
            />
          </div>
          <div>
            <label for="edit-class-id" class="block font-bold text-slate-700 mb-1">Kod ID:</label>
            <input
              id="edit-class-id"
              type="text"
              bind:value={id}
              placeholder="np. 1A"
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono"
            />
          </div>
        </div>

      {:else if target.type === 'teacher'}
        <!-- NAUCZYCIEL -->
        <div>
          <label for="edit-teacher-name" class="block font-bold text-slate-700 mb-1">Imię i nazwisko nauczyciela:</label>
          <input
            id="edit-teacher-name"
            type="text"
            bind:value={name}
            placeholder="np. Anna Nowak"
            class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label for="edit-teacher-subject" class="block font-bold text-slate-700 mb-1">Główny przedmiot:</label>
            <input
              id="edit-teacher-subject"
              type="text"
              bind:value={subject}
              placeholder="np. Matematyka"
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
            />
          </div>
          <div>
            <label for="edit-teacher-workload" class="block font-bold text-slate-700 mb-1">Pensum etatowe (h/tydz.):</label>
            <input
              id="edit-teacher-workload"
              type="number"
              min="1"
              max="40"
              bind:value={workload}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
            />
          </div>
        </div>

        <div>
          <label for="edit-teacher-id" class="block font-bold text-slate-700 mb-1">Identyfikator (ID):</label>
          <input
            id="edit-teacher-id"
            type="text"
            bind:value={id}
            placeholder="np. Nowak_A"
            class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono text-xs"
          />
        </div>

      {:else if target.type === 'room'}
        <!-- SALA -->
        <div>
          <label for="edit-room-name" class="block font-bold text-slate-700 mb-1">Nazwa pomieszczenia:</label>
          <input
            id="edit-room-name"
            type="text"
            bind:value={name}
            placeholder="np. Sala 101 (Ogólna)"
            class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label for="edit-room-type" class="block font-bold text-slate-700 mb-1">Przeznaczenie (typ):</label>
            <select
              id="edit-room-type"
              bind:value={roomType}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
            >
              <option value="general">Ogólna (dydaktyczna)</option>
              <option value="gym">Gimnastyczna (WF)</option>
              <option value="lab">Laboratorium / Chem-Biol</option>
              <option value="it">Informatyczna (IT)</option>
            </select>
          </div>
          <div>
            <label for="edit-room-id" class="block font-bold text-slate-700 mb-1">Kod ID:</label>
            <input
              id="edit-room-id"
              type="text"
              bind:value={id}
              placeholder="np. s101"
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono"
            />
          </div>
        </div>

      {:else if target.type === 'lesson'}
        <!-- LEKCJA -->
        <div>
          <label for="edit-lesson-subject" class="block font-bold text-slate-700 mb-1">Przedmiot:</label>
          <input
            id="edit-lesson-subject"
            type="text"
            bind:value={lessonSubject}
            class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-semibold"
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label for="edit-lesson-class" class="block font-bold text-slate-700 mb-1">Klasa:</label>
            <select
              id="edit-lesson-class"
              bind:value={lessonClassId}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
            >
              {#each editor.classes as c}
                <option value={c.id}>{c.name}</option>
              {/each}
            </select>
          </div>

          <div>
            <label for="edit-lesson-teacher" class="block font-bold text-slate-700 mb-1">Nauczyciel:</label>
            <select
              id="edit-lesson-teacher"
              bind:value={lessonTeacherId}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
            >
              {#each editor.teachers as t}
                <option value={t.id}>{t.name} ({t.subject})</option>
              {/each}
            </select>
          </div>
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label for="edit-lesson-room" class="block font-bold text-slate-700 mb-1">Sala lekcyjna:</label>
            <select
              id="edit-lesson-room"
              bind:value={lessonRoomId}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
            >
              <option value="">Dowolna</option>
              {#each editor.rooms as r}
                <option value={r.id}>{r.name}</option>
              {/each}
            </select>
          </div>

          <div>
            <label for="edit-lesson-weekly-count" class="block font-bold text-slate-700 mb-1">Wymiar (godz./tydzień):</label>
            <input
              id="edit-lesson-weekly-count"
              type="number"
              min="1"
              max="15"
              bind:value={lessonWeeklyCount}
              class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
            />
          </div>
        </div>

        <div>
          <label class="flex items-center gap-2 cursor-pointer font-medium text-slate-700 mt-1">
            <input
              type="checkbox"
              checked={lessonDuration === 2}
              onchange={(e) => (lessonDuration = e.currentTarget.checked ? 2 : 1)}
              class="w-4 h-4 rounded text-blue-600 focus:ring-blue-500"
            />
            <span>Lekcja łączona w blok 2-godzinny (podwójna lekcja)</span>
          </label>
        </div>
      {/if}
    </div>

    <!-- Footer -->
    <div class="px-6 py-3.5 border-t border-slate-100 bg-slate-50 flex items-center justify-end gap-2.5 shrink-0">
      <button
        type="button"
        onclick={onClose}
        class="px-4 py-2 bg-slate-200 hover:bg-slate-300 text-slate-700 font-semibold rounded-xl text-xs transition-colors cursor-pointer"
      >
        Anuluj
      </button>
      <button
        type="button"
        onclick={handleSave}
        class="px-5 py-2 bg-blue-600 hover:bg-blue-700 text-white font-bold rounded-xl text-xs transition-colors cursor-pointer shadow-xs"
      >
        💾 Zapisz zmiany
      </button>
    </div>
  </div>
</div>

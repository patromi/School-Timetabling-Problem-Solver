<!-- frontend/src/lib/components/editor/EditEntityModal.svelte -->
<script lang="ts">
  import type { EditorState } from "./editorState.svelte";
  import type { RoomType } from "./types";
  import EditClassFields from "./entity/EditClassFields.svelte";
  import EditTeacherFields from "./entity/EditTeacherFields.svelte";
  import EditRoomFields from "./entity/EditRoomFields.svelte";
  import EditLessonFields from "./entity/EditLessonFields.svelte";

  interface Props {
    editor: EditorState;
    target: {
      type: "class" | "teacher" | "room" | "lesson";
      data: any;
    };
    onClose: () => void;
  }

  let { editor, target, onClose }: Props = $props();

  let name = $state("");
  let id = $state("");
  let grade = $state<number>(1);
  let subject = $state("");
  let workload = $state<number>(18);
  let roomType = $state<RoomType>("general");

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
      if (!name.trim()) return alert("Podaj nazwę klasy!");
      editor.updateClass(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        grade: Number(grade) || 1,
      });
    } else if (target.type === "teacher") {
      if (!name.trim()) return alert("Podaj imię i nazwisko nauczyciela!");
      editor.updateTeacher(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        subject: subject.trim() || "Przedmiot ogólny",
        workload: Number(workload) || 18,
      });
    } else if (target.type === "room") {
      if (!name.trim()) return alert("Podaj nazwę sali!");
      editor.updateRoom(target.data.id, {
        name: name.trim(),
        id: id.trim() || target.data.id,
        type: roomType,
      });
    } else if (target.type === "lesson") {
      if (!lessonSubject.trim() || !lessonClassId || !lessonTeacherId) {
        return alert("Wypełnij wymagane pola lekcji!");
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
        <EditClassFields bind:name bind:grade bind:id />
      {:else if target.type === 'teacher'}
        <EditTeacherFields bind:name bind:subject bind:workload bind:id subjects={editor.allKnownSubjects} />
      {:else if target.type === 'room'}
        <EditRoomFields bind:name bind:roomType bind:id />
      {:else if target.type === 'lesson'}
        <EditLessonFields
          bind:lessonSubject
          bind:lessonClassId
          bind:lessonTeacherId
          bind:lessonRoomId
          bind:lessonWeeklyCount
          bind:lessonDuration
          classes={editor.classes}
          teachers={editor.teachers}
          rooms={editor.rooms}
          subjects={editor.allKnownSubjects}
        />
      {/if}
    </div>

    <!-- Footer -->
    <div class="px-6 py-3.5 bg-slate-50 border-t border-slate-100 flex items-center justify-between shrink-0">
      <span class="text-[11px] text-slate-400">
        ID: <strong class="font-mono text-slate-600">{target.data.id}</strong>
      </span>
      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={onClose}
          class="px-3.5 py-1.5 text-xs text-slate-600 hover:bg-slate-200/60 rounded-xl font-semibold transition-all cursor-pointer"
        >
          Anuluj
        </button>
        <button
          type="button"
          onclick={handleSave}
          class="px-4 py-1.5 text-xs font-bold bg-blue-600 hover:bg-blue-700 text-white rounded-xl shadow-xs transition-all cursor-pointer"
        >
          Zapisz zmiany
        </button>
      </div>
    </div>
  </div>
</div>

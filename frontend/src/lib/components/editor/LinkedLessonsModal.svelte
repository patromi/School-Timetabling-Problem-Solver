<script lang="ts">
  import type { LinkedGroupItem } from "./types";
  import LinkedGroupsList from "./linked/LinkedGroupsList.svelte";
  import NewLinkedGroupForm from "./linked/NewLinkedGroupForm.svelte";

  export type { LinkedGroupItem };

  interface Props {
    lessons: Array<{
      id: string;
      subject: string;
      classId: string;
      teacherId: string;
      weeklyCount: number;
    }>;
    classes: Array<{ id: string; name: string }>;
    teachers: Array<{ id: string; name: string }>;
    initialGroups: LinkedGroupItem[];
    onSave: (groups: LinkedGroupItem[]) => void;
    onClose: () => void;
  }

  let {
    lessons,
    classes,
    teachers,
    initialGroups = [],
    onSave,
    onClose,
  }: Props = $props();

  let groups = $state<LinkedGroupItem[]>([]);

  $effect(() => {
    groups = JSON.parse(JSON.stringify(initialGroups || []));
  });

  function handleAddGroup(newGroup: LinkedGroupItem) {
    groups = [...groups, newGroup];
  }

  function handleRemoveGroup(groupId: string) {
    groups = groups.filter((g) => g.id !== groupId);
  }

  function handleSaveAll() {
    onSave(groups);
  }
</script>

<div
  class="fixed inset-0 bg-slate-900/60 backdrop-blur-xs z-50 flex items-center justify-center p-4 overflow-y-auto"
  role="dialog"
  aria-modal="true"
  aria-labelledby="linked-lessons-title"
>
  <div class="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-3xl w-full overflow-hidden flex flex-col my-auto animate-in fade-in zoom-in-95 duration-150">
    <!-- Header -->
    <div class="px-6 py-4 bg-gradient-to-r from-purple-900 to-indigo-950 text-white flex items-center justify-between">
      <div class="flex items-center gap-3">
        <span class="w-10 h-10 rounded-xl bg-white/10 flex items-center justify-center text-xl">
          🔗
        </span>
        <div>
          <h3 id="linked-lessons-title" class="text-base font-bold leading-tight">
            Lekcje równoległe i grupy jednoczesne
          </h3>
          <p class="text-xs text-purple-200 mt-0.5">
            Synchronizacja godzinowa lekcji między klasami (grupy językowe, łączony WF, wspólne wykłady).
          </p>
        </div>
      </div>
      <button
        type="button"
        onclick={onClose}
        class="text-white/60 hover:text-white p-1 rounded-lg hover:bg-white/10 transition-colors cursor-pointer text-lg"
        aria-label="Zamknij"
      >
        ✕
      </button>
    </div>

    <!-- Body -->
    <div class="p-6 flex flex-col gap-6 overflow-y-auto max-h-[75vh]">
      <!-- Existing groups list -->
      <LinkedGroupsList
        {groups}
        {lessons}
        {classes}
        {teachers}
        onRemoveGroup={handleRemoveGroup}
      />

      <!-- Add new linked group form -->
      <NewLinkedGroupForm
        {lessons}
        {classes}
        {teachers}
        onAddGroup={handleAddGroup}
      />
    </div>

    <!-- Footer -->
    <div class="px-6 py-3.5 bg-slate-50 border-t border-slate-200 flex items-center justify-between">
      <span class="text-xs text-slate-500">
        Po zapisaniu grupy zostaną dodane do ograniczeń instancji.
      </span>
      <div class="flex items-center gap-2">
        <button
          type="button"
          onclick={onClose}
          class="px-4 py-2 text-xs font-semibold text-slate-700 hover:bg-slate-200/60 rounded-xl transition-all cursor-pointer"
        >
          Anuluj
        </button>
        <button
          type="button"
          onclick={handleSaveAll}
          class="px-5 py-2 bg-purple-700 hover:bg-purple-800 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer"
        >
          Zapisz powiązania
        </button>
      </div>
    </div>
  </div>
</div>

<script lang="ts">
  import type { LinkedGroupItem } from "../types";

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
    onAddGroup: (newGroup: LinkedGroupItem) => void;
  }

  let { lessons, classes, teachers, onAddGroup }: Props = $props();

  let newGroupName = $state("");
  let selectedLessonIds = $state<Set<string>>(new Set());
  let newGroupIsSoft = $state(false);
  let newGroupWeight = $state(10);
  let filterClass = $state("");
  let filterSubject = $state("");

  function getClassName(classId: string): string {
    const c = classes.find((x) => x.id === classId);
    return c ? c.name : classId;
  }

  function getTeacherName(teacherId: string): string {
    const t = teachers.find((x) => x.id === teacherId);
    return t ? t.name : teacherId;
  }

  function toggleLessonSelection(lessonId: string) {
    const next = new Set(selectedLessonIds);
    if (next.has(lessonId)) {
      next.delete(lessonId);
    } else {
      next.add(lessonId);
    }
    selectedLessonIds = next;
  }

  function presetLanguageGroup() {
    newGroupName = "Język angielski międzyoddziałowy";
    const englishLessons = lessons.filter((l) =>
      l.subject.toLowerCase().includes("angiel")
    );
    selectedLessonIds = new Set(englishLessons.map((l) => l.id));
  }

  function presetPeGroup() {
    newGroupName = "Wychowanie Fizyczne równoległe";
    const peLessons = lessons.filter((l) =>
      l.subject.toLowerCase().includes("fizycz") || l.subject.toLowerCase().includes("wf")
    );
    selectedLessonIds = new Set(peLessons.map((l) => l.id));
  }

  function handleCreate() {
    if (!newGroupName.trim() || selectedLessonIds.size < 2) return;

    onAddGroup({
      id: `link_${Date.now()}_${Math.random().toString(36).substring(2, 6)}`,
      name: newGroupName.trim(),
      lessonIds: Array.from(selectedLessonIds),
      isSoft: newGroupIsSoft,
      weight: newGroupWeight,
    });

    newGroupName = "";
    selectedLessonIds = new Set();
    newGroupIsSoft = false;
    newGroupWeight = 10;
  }

  let filteredLessons = $derived(
    lessons.filter((l) => {
      if (filterClass && l.classId !== filterClass) return false;
      if (filterSubject && !l.subject.toLowerCase().includes(filterSubject.toLowerCase())) return false;
      return true;
    })
  );
</script>

<div class="bg-slate-50/80 p-5 rounded-2xl border border-slate-200 flex flex-col gap-4">
  <div class="flex items-center justify-between">
    <h4 class="text-xs font-bold uppercase tracking-wider text-slate-800">
      Utwórz nową grupę lekcji równoległych
    </h4>

    <!-- Presets -->
    <div class="flex items-center gap-1.5">
      <span class="text-[10px] text-slate-500 font-medium">Szybki preset:</span>
      <button
        type="button"
        onclick={presetLanguageGroup}
        class="px-2 py-0.5 bg-white border border-slate-200 hover:border-purple-300 text-purple-800 rounded text-[10px] font-semibold cursor-pointer transition-all"
      >
        🗣️ Języki obce
      </button>
      <button
        type="button"
        onclick={presetPeGroup}
        class="px-2 py-0.5 bg-white border border-slate-200 hover:border-purple-300 text-purple-800 rounded text-[10px] font-semibold cursor-pointer transition-all"
      >
        🏃 WF równoległy
      </button>
    </div>
  </div>

  <!-- Name and enforcement -->
  <div class="grid grid-cols-1 sm:grid-cols-2 gap-3 text-xs">
    <div>
      <label for="link-group-name" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Nazwa grupy powiązania:
      </label>
      <input
        id="link-group-name"
        type="text"
        bind:value={newGroupName}
        placeholder="np. Języki obce 1A i 1B (grupy)"
        class="w-full px-3 py-2 bg-white border border-slate-200 rounded-xl text-slate-900 focus:outline-none focus:ring-2 focus:ring-purple-500/20 focus:border-purple-500"
      />
    </div>

    <div>
      <label for="link-group-type" class="block text-[11px] font-semibold text-slate-700 mb-1">
        Rygor ograniczenia:
      </label>
      <div class="flex items-center gap-2">
        <select
          id="link-group-type"
          bind:value={newGroupIsSoft}
          class="flex-1 px-3 py-2 bg-white border border-slate-200 rounded-xl text-slate-900 focus:outline-none focus:ring-2 focus:ring-purple-500/20 focus:border-purple-500"
        >
          <option value={false}>Wymóg twardy (HARD - bezwzględny)</option>
          <option value={true}>Preferencja miękka (SOFT - kara)</option>
        </select>
        {#if newGroupIsSoft}
          <input
            type="number"
            min="1"
            max="100"
            bind:value={newGroupWeight}
            class="w-16 px-2 py-2 bg-white border border-slate-200 rounded-xl text-center font-bold"
            title="Waga kary"
          />
        {/if}
      </div>
    </div>
  </div>

  <!-- Lesson selection filter -->
  <div>
    <div class="flex items-center justify-between mb-2">
      <span class="text-[11px] font-semibold text-slate-700">
        Wybierz co najmniej 2 lekcje do zsynchronizowania (zaznaczono: <strong>{selectedLessonIds.size}</strong>):
      </span>
      <div class="flex items-center gap-2">
        <input
          type="text"
          placeholder="Filtruj przedmiot..."
          bind:value={filterSubject}
          class="px-2 py-1 text-[11px] bg-white border border-slate-200 rounded-lg w-32"
        />
        <select
          bind:value={filterClass}
          class="px-2 py-1 text-[11px] bg-white border border-slate-200 rounded-lg"
        >
          <option value="">Wszystkie klasy</option>
          {#each classes as c}
            <option value={c.id}>{c.name}</option>
          {/each}
        </select>
      </div>
    </div>

    <!-- Lesson cards grid -->
    <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-2 max-h-48 overflow-y-auto p-1 bg-white border border-slate-200 rounded-xl">
      {#each filteredLessons as l}
        {@const isSelected = selectedLessonIds.has(l.id)}
        <button
          type="button"
          onclick={() => toggleLessonSelection(l.id)}
          class={`p-2.5 rounded-lg border text-left transition-all cursor-pointer flex items-start gap-2 ${
            isSelected
              ? "bg-purple-50 border-purple-300 ring-2 ring-purple-400/40"
              : "bg-slate-50/50 border-slate-200/80 hover:bg-slate-100/60"
          }`}
        >
          <input
            type="checkbox"
            checked={isSelected}
            tabindex={-1}
            class="mt-0.5 accent-purple-600 rounded cursor-pointer"
          />
          <div class="flex flex-col min-w-0">
            <span class="font-bold text-xs text-slate-900 truncate">{l.subject}</span>
            <span class="text-[10px] text-slate-500">
              Oddział: {getClassName(l.classId)}
            </span>
            <span class="text-[10px] text-purple-700 truncate">
              {getTeacherName(l.teacherId)}
            </span>
          </div>
        </button>
      {/each}
    </div>
  </div>

  <div class="flex justify-end pt-2">
    <button
      type="button"
      onclick={handleCreate}
      disabled={!newGroupName.trim() || selectedLessonIds.size < 2}
      class={`px-4 py-2 text-xs font-bold rounded-xl transition-all cursor-pointer flex items-center gap-1.5 ${
        newGroupName.trim() && selectedLessonIds.size >= 2
          ? "bg-purple-700 hover:bg-purple-800 text-white shadow-xs"
          : "bg-slate-200 text-slate-400 cursor-not-allowed"
      }`}
    >
      <span>➕ Połącz wybrane lekcje w grupę</span>
    </button>
  </div>
</div>

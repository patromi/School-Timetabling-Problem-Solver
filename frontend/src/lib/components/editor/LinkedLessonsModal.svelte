<script lang="ts">
  export interface LinkedGroupItem {
    id: string;
    name: string;
    lessonIds: string[];
    isSoft?: boolean;
    weight?: number;
  }

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
  let newGroupName = $state("");
  let selectedLessonIds = $state<Set<string>>(new Set());
  let newGroupIsSoft = $state(false); // LinkEvents usually HARD requirement (simultaneous classes)
  let newGroupWeight = $state(10);
  let filterClass = $state("");
  let filterSubject = $state("");

  $effect(() => {
    groups = JSON.parse(JSON.stringify(initialGroups || []));
  });

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

  function addGroup() {
    if (!newGroupName.trim() || selectedLessonIds.size < 2) return;

    const newGroup: LinkedGroupItem = {
      id: `link_${Date.now()}_${Math.random().toString(36).substring(2, 6)}`,
      name: newGroupName.trim(),
      lessonIds: Array.from(selectedLessonIds),
      isSoft: newGroupIsSoft,
      weight: newGroupWeight,
    };

    groups = [...groups, newGroup];
    newGroupName = "";
    selectedLessonIds = new Set();
    newGroupIsSoft = false;
    newGroupWeight = 10;
  }

  function removeGroup(groupId: string) {
    groups = groups.filter((g) => g.id !== groupId);
  }

  function handleSaveAll() {
    onSave(groups);
  }

  // Pre-fill helpers for common scenarios
  function presetLanguageGroup() {
    newGroupName = "Język angielski międzyoddziałowy";
    // Find all English lessons
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

  let filteredLessons = $derived(
    lessons.filter((l) => {
      if (filterClass && l.classId !== filterClass) return false;
      if (filterSubject && !l.subject.toLowerCase().includes(filterSubject.toLowerCase())) return false;
      return true;
    })
  );
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
            Lekcje równoległe i jednoczesne (LinkEventsConstraint)
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
      <div>
        <div class="flex items-center justify-between mb-3">
          <span class="text-xs font-bold uppercase tracking-wider text-slate-500">
            Zdefiniowane grupy lekcji równoległych ({groups.length})
          </span>
          <span class="text-[11px] text-slate-400">
            Wszystkie lekcje w grupie odbędą się w tym samym czasie
          </span>
        </div>

        {#if groups.length === 0}
          <div class="p-6 rounded-xl border border-dashed border-slate-200 bg-slate-50 text-center flex flex-col items-center justify-center gap-1.5">
            <span class="text-2xl">🔗</span>
            <span class="text-xs font-bold text-slate-700">Brak powiązanych lekcji</span>
            <p class="text-[11px] text-slate-400 max-w-md">
              Utwórz nową grupę poniżej, aby połączyć lekcje z różnych oddziałów (np. podział na grupy językowe z klas 1A i 1B w tym samym slocie).
            </p>
          </div>
        {:else}
          <div class="divide-y divide-slate-100 border border-slate-200 rounded-xl overflow-hidden">
            {#each groups as g}
              <div class="p-4 bg-white hover:bg-slate-50/70 transition-colors flex items-start justify-between gap-4">
                <div class="flex flex-col gap-1.5 flex-1">
                  <div class="flex items-center gap-2">
                    <span class="font-bold text-slate-900 text-xs">{g.name}</span>
                    <span class="px-2 py-0.5 rounded-full text-[10px] font-bold {g.isSoft ? 'bg-indigo-50 text-indigo-700 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
                      {g.isSoft ? `SOFT (waga ${g.weight || 10})` : 'HARD (Wymóg bezwzględny)'}
                    </span>
                    <span class="text-[11px] text-slate-400">
                      • {g.lessonIds.length} powiązanych lekcji
                    </span>
                  </div>

                  <!-- Member badges -->
                  <div class="flex flex-wrap gap-1.5 mt-1">
                    {#each g.lessonIds as lid}
                      {@const l = lessons.find((x) => x.id === lid)}
                      {#if l}
                        <span class="inline-flex items-center gap-1 px-2.5 py-1 bg-purple-50 border border-purple-200 text-purple-900 rounded-lg text-[11px] font-medium">
                          <span>📚</span>
                          <strong>{l.subject}</strong>
                          <span class="text-purple-600">({getClassName(l.classId)} - {getTeacherName(l.teacherId)})</span>
                        </span>
                      {/if}
                    {/each}
                  </div>
                </div>

                <button
                  type="button"
                  onclick={() => removeGroup(g.id)}
                  class="text-slate-400 hover:text-rose-600 p-1 rounded-lg hover:bg-rose-50 transition-colors cursor-pointer text-xs"
                  title="Usuń grupę"
                >
                  Usuń
                </button>
              </div>
            {/each}
          </div>
        {/if}
      </div>

      <!-- Add new linked group form -->
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
              placeholder="np. Angielski międzyoddziałowy 1A i 1B"
              class="w-full px-3 py-1.5 bg-white border border-slate-200 rounded-lg text-slate-900 text-xs focus:outline-none focus:border-purple-500"
            />
          </div>

          <div class="flex items-end gap-3">
            <div class="flex-1">
              <span class="block text-[11px] font-semibold text-slate-700 mb-1">
                Charakter wymogu:
              </span>
              <div class="flex items-center bg-white p-0.5 rounded-lg border border-slate-200">
                <button
                  type="button"
                  onclick={() => (newGroupIsSoft = false)}
                  class="flex-1 py-1 text-center font-bold text-[11px] rounded-md transition-all cursor-pointer {!newGroupIsSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                >
                  HARD (Ścisła synchronizacja)
                </button>
                <button
                  type="button"
                  onclick={() => (newGroupIsSoft = true)}
                  class="flex-1 py-1 text-center font-bold text-[11px] rounded-md transition-all cursor-pointer {newGroupIsSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
                >
                  SOFT (Preferencja)
                </button>
              </div>
            </div>

            {#if newGroupIsSoft}
              <div class="w-20">
                <label for="link-group-weight" class="block text-[11px] font-semibold text-slate-700 mb-1">Waga kary:</label>
                <input
                  id="link-group-weight"
                  type="number"
                  min="1"
                  max="50"
                  bind:value={newGroupWeight}
                  class="w-full px-2 py-1.5 bg-white border border-slate-200 rounded-lg text-center font-bold text-xs"
                />
              </div>
            {/if}
          </div>
        </div>

        <!-- Filter and Lesson selector -->
        <div class="flex flex-col gap-2">
          <div class="flex items-center justify-between text-xs">
            <span class="font-semibold text-slate-700">
              Wybierz lekcje do synchronizacji ({selectedLessonIds.size} zaznaczonych — wymagane min. 2):
            </span>

            <div class="flex items-center gap-2">
              <select
                bind:value={filterClass}
                class="px-2 py-1 bg-white border border-slate-200 rounded text-[11px] text-slate-700 focus:outline-none"
              >
                <option value="">Wszystkie klasy</option>
                {#each classes as c}
                  <option value={c.id}>{c.name}</option>
                {/each}
              </select>

              <input
                type="text"
                placeholder="Filtruj przedmiot..."
                bind:value={filterSubject}
                class="px-2 py-1 bg-white border border-slate-200 rounded text-[11px] text-slate-700 focus:outline-none w-36"
              />
            </div>
          </div>

          <div class="max-h-56 overflow-y-auto divide-y divide-slate-200/60 border border-slate-200 bg-white rounded-xl">
            {#each filteredLessons as l}
              {@const selected = selectedLessonIds.has(l.id)}
              <label class="p-2.5 flex items-center justify-between hover:bg-slate-50 cursor-pointer text-xs {selected ? 'bg-purple-50/60' : ''}">
                <div class="flex items-center gap-3">
                  <input
                    type="checkbox"
                    checked={selected}
                    onchange={() => toggleLessonSelection(l.id)}
                    class="w-4 h-4 accent-purple-600 rounded cursor-pointer"
                  />
                  <div>
                    <span class="font-bold text-slate-900">{l.subject}</span>
                    <span class="text-slate-500 text-[11px] ml-1.5">
                      (Klasa: <strong>{getClassName(l.classId)}</strong>, Pedagog: {getTeacherName(l.teacherId)})
                    </span>
                  </div>
                </div>

                <span class="text-[11px] font-semibold text-slate-500">
                  {l.weeklyCount}h/tydz.
                </span>
              </label>
            {/each}
          </div>
        </div>

        <!-- Add group button -->
        <div class="flex justify-end">
          <button
            type="button"
            onclick={addGroup}
            disabled={!newGroupName.trim() || selectedLessonIds.size < 2}
            class="px-4 py-2 bg-purple-700 hover:bg-purple-800 disabled:opacity-40 disabled:cursor-not-allowed text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
          >
            <span>+ Dodaj grupę do planu ({selectedLessonIds.size} lekcji)</span>
          </button>
        </div>
      </div>
    </div>

    <!-- Footer -->
    <div class="p-4 bg-slate-50 border-t border-slate-200 flex items-center justify-between">
      <button
        type="button"
        onclick={onClose}
        class="px-4 py-2 text-xs font-semibold text-slate-600 hover:text-slate-900 transition-colors cursor-pointer"
      >
        Anuluj
      </button>

      <button
        type="button"
        onclick={handleSaveAll}
        class="px-5 py-2 bg-purple-700 hover:bg-purple-800 text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
      >
        <span>Zapisz konfigurację powiązań</span>
      </button>
    </div>
  </div>
</div>

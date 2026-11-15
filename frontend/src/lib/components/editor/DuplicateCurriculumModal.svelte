<script lang="ts">
  import Icon from "../common/Icon.svelte";

  interface SimpleClass {
    id: string;
    name: string;
    grade?: number;
  }

  interface Props {
    sourceClass: SimpleClass;
    classes: SimpleClass[];
    sourceHours: number;
    sourceLessonsCount: number;
    onDuplicate: (targetClassIds: string[]) => void;
    onClose: () => void;
  }

  let {
    sourceClass,
    classes,
    sourceHours,
    sourceLessonsCount,
    onDuplicate,
    onClose,
  }: Props = $props();

  let selectedTargetIds = $state<Set<string>>(new Set());

  function getClassGrade(cls: SimpleClass): number {
    if (cls.grade !== undefined && cls.grade !== null) return cls.grade;
    const match = cls.name.match(/^(\d+)/) || cls.id.match(/^(\d+)/);
    return match ? parseInt(match[1], 10) : 1;
  }

  const sourceGrade = $derived(getClassGrade(sourceClass));
  const otherClasses = $derived(classes.filter((c) => c.id !== sourceClass.id));
  const sameGradeClasses = $derived(otherClasses.filter((c) => getClassGrade(c) === sourceGrade));

  function toggleTarget(id: string) {
    const next = new Set(selectedTargetIds);
    if (next.has(id)) next.delete(id);
    else next.add(id);
    selectedTargetIds = next;
  }

  function selectAllSameGrade() {
    selectedTargetIds = new Set(sameGradeClasses.map((c) => c.id));
  }

  function selectAll() {
    selectedTargetIds = new Set(otherClasses.map((c) => c.id));
  }

  function clearSelection() {
    selectedTargetIds = new Set();
  }

  function handleConfirm() {
    if (selectedTargetIds.size === 0) return;
    onDuplicate(Array.from(selectedTargetIds));
  }
</script>

<div
  class="fixed inset-0 bg-slate-900/60 backdrop-blur-xs z-50 flex items-center justify-center p-4 overflow-y-auto"
  role="dialog"
  aria-modal="true"
  aria-labelledby="duplicate-modal-title"
>
  <div class="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-lg w-full overflow-hidden flex flex-col my-auto animate-in fade-in zoom-in-95 duration-150">
    <!-- Header -->
    <div class="px-6 py-4 bg-gradient-to-r from-blue-900 to-indigo-950 text-white flex items-center justify-between">
      <div class="flex items-center gap-3">
        <span class="w-10 h-10 rounded-xl bg-white/10 flex items-center justify-center">
          <Icon name="content_copy" size="base" class="text-white" />
        </span>
        <div>
          <h3 id="duplicate-modal-title" class="text-base font-bold leading-tight">
            Duplikowanie siatki zajęć
          </h3>
          <p class="text-xs text-blue-200 mt-0.5">
            Skopiuj plan przedmiotów i wymiar godzin do innych klas.
          </p>
        </div>
      </div>
      <button
        type="button"
        onclick={onClose}
        class="text-white/60 hover:text-white p-1 rounded-lg hover:bg-white/10 transition-colors cursor-pointer flex items-center justify-center"
        aria-label="Zamknij"
      >
        <Icon name="close" size="xs" />
      </button>
    </div>

    <!-- Body -->
    <div class="p-6 flex flex-col gap-4">
      <!-- Source class summary -->
      <div class="p-4 bg-blue-50/70 border border-blue-200 rounded-xl flex items-center justify-between text-xs">
        <div>
          <span class="text-slate-500 block text-[11px]">Klasa źródłowa:</span>
          <span class="font-bold text-blue-950 text-sm">{sourceClass.name}</span>
          <span class="text-blue-700 block mt-0.5">Rocznik: Poziom {sourceGrade}</span>
        </div>
        <div class="text-right">
          <span class="px-2.5 py-1 bg-blue-600 text-white font-bold rounded-lg text-xs">
            {sourceHours} godz./tydz.
          </span>
          <span class="text-[11px] text-slate-500 block mt-1">
            {sourceLessonsCount} pozycji zajęć
          </span>
        </div>
      </div>

      <!-- Quick actions -->
      <div>
        <div class="flex items-center justify-between mb-2">
          <span class="text-xs font-bold uppercase tracking-wider text-slate-600">
            Wybierz klasy docelowe ({selectedTargetIds.size} zaznaczonych):
          </span>
          <div class="flex items-center gap-1.5">
            {#if sameGradeClasses.length > 0}
              <button
                type="button"
                onclick={selectAllSameGrade}
                class="px-2 py-0.5 bg-blue-50 border border-blue-200 hover:bg-blue-100 text-blue-800 rounded text-[10px] font-semibold cursor-pointer transition-all"
              >
                Rocznik {sourceGrade} ({sameGradeClasses.length})
              </button>
            {/if}
            <button
              type="button"
              onclick={selectAll}
              class="px-2 py-0.5 bg-slate-100 border border-slate-200 hover:bg-slate-200 text-slate-700 rounded text-[10px] font-semibold cursor-pointer transition-all"
            >
              Wszystkie
            </button>
            <button
              type="button"
              onclick={clearSelection}
              class="px-2 py-0.5 bg-slate-100 border border-slate-200 hover:bg-slate-200 text-slate-500 rounded text-[10px] cursor-pointer transition-all"
            >
              Wyczyść
            </button>
          </div>
        </div>

        {#if otherClasses.length === 0}
          <div class="p-6 text-center text-xs text-slate-400 bg-slate-50 rounded-xl border border-dashed border-slate-200">
            Brak innych klas w szkole. Dodaj najpierw inne oddziały, aby skopiować plan.
          </div>
        {:else}
          <div class="max-h-60 overflow-y-auto divide-y divide-slate-100 border border-slate-200 rounded-xl bg-white">
            {#each otherClasses as cls}
              {@const isSameGrade = getClassGrade(cls) === sourceGrade}
              {@const isSelected = selectedTargetIds.has(cls.id)}
              <label class="p-3 flex items-center justify-between hover:bg-slate-50/80 cursor-pointer text-xs {isSelected ? 'bg-blue-50/40' : ''}">
                <div class="flex items-center gap-3">
                  <input
                    type="checkbox"
                    checked={isSelected}
                    onchange={() => toggleTarget(cls.id)}
                    class="w-4 h-4 accent-blue-600 rounded cursor-pointer"
                  />
                  <div>
                    <span class="font-bold text-slate-900">{cls.name}</span>
                    <span class="text-slate-500 text-[11px] ml-1.5">
                      (Rocznik {getClassGrade(cls)})
                    </span>
                  </div>
                </div>

                {#if isSameGrade}
                  <span class="px-2 py-0.5 bg-emerald-50 text-emerald-700 border border-emerald-200 rounded text-[10px] font-semibold">
                    Ten sam rocznik ({sourceGrade})
                  </span>
                {/if}
              </label>
            {/each}
          </div>
        {/if}
      </div>

      <p class="text-[11px] text-slate-500 leading-snug flex items-start gap-1">
        <Icon name="warning" size="xs" class="text-amber-600 shrink-0 mt-0.5" />
        <span><strong>Uwaga:</strong> Skopiowanie siatki zastąpi dotychczasowe lekcje w wybranych klasach docelowych dokładnym zestawem godzin z klasy <strong>{sourceClass.name}</strong> ({sourceHours}h w tygodniu).</span>
      </p>
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
        onclick={handleConfirm}
        disabled={selectedTargetIds.size === 0}
        class="px-5 py-2 bg-blue-600 hover:bg-blue-700 disabled:opacity-40 disabled:cursor-not-allowed text-white text-xs font-bold rounded-xl shadow-xs transition-all cursor-pointer flex items-center gap-1.5"
      >
        <Icon name="content_copy" size="xs" class="text-white" />
        <span>Skopiuj siatkę do {selectedTargetIds.size} klas</span>
      </button>
    </div>
  </div>
</div>

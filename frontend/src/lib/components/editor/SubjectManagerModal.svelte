<!-- frontend/src/lib/components/editor/SubjectManagerModal.svelte -->
<script lang="ts">
  import type { EditorState } from "./editorState.svelte";

  interface Props {
    editor: EditorState;
    onClose: () => void;
  }

  let { editor, onClose }: Props = $props();

  let renamingSubject = $state<string | null>(null);
  let newSubjectName = $state("");

  let mergingSubject = $state<string | null>(null);
  let mergeTargetSubject = $state("");

  let newCustomSubjectName = $state("");

  function startRename(name: string) {
    renamingSubject = name;
    newSubjectName = name;
    mergingSubject = null;
  }

  function applyRename() {
    if (!renamingSubject || !newSubjectName.trim()) return;
    editor.renameSubject(renamingSubject, newSubjectName.trim());
    renamingSubject = null;
    newSubjectName = "";
  }

  function startMerge(name: string) {
    mergingSubject = name;
    mergeTargetSubject = editor.uniqueSubjects.find((s) => s.name !== name)?.name || "";
    renamingSubject = null;
  }

  function applyMerge() {
    if (!mergingSubject || !mergeTargetSubject || mergingSubject === mergeTargetSubject) return;
    if (confirm(`Czy na pewno chcesz przenieść wszystkie lekcje i przydziały z przedmiotu "${mergingSubject}" do "${mergeTargetSubject}"?`)) {
      editor.mergeSubjects(mergingSubject, mergeTargetSubject);
      mergingSubject = null;
      mergeTargetSubject = "";
    }
  }

  function addNewSubject() {
    if (!newCustomSubjectName.trim()) return;
    const name = newCustomSubjectName.trim();
    if (editor.uniqueSubjects.some((s) => s.name.toLowerCase() === name.toLowerCase())) {
      alert("Przedmiot o tej nazwie już istnieje!");
      return;
    }
    editor.batchGradeSubject = name;
    editor.lessonSubject = name;
    newCustomSubjectName = "";
  }
</script>

<div
  class="fixed inset-0 bg-slate-900/60 backdrop-blur-xs flex items-center justify-center p-4 z-50 animate-in fade-in duration-150"
  role="dialog"
  aria-modal="true"
>
  <div class="bg-white rounded-2xl border border-slate-200 shadow-2xl w-full max-w-3xl max-h-[90vh] flex flex-col overflow-hidden animate-in zoom-in-95 duration-150">
    <!-- Header -->
    <div class="px-6 py-4 border-b border-slate-100 flex items-center justify-between shrink-0 bg-slate-50/60">
      <div class="flex items-center gap-3">
        <span class="w-9 h-9 rounded-xl bg-purple-100 text-purple-700 flex items-center justify-center font-bold text-lg">
          📚
        </span>
        <div>
          <h3 class="text-sm font-bold text-slate-900">Menedżer Przedmiotów Szkolnych</h3>
          <p class="text-xs text-slate-500">
            Zmieniaj nazwy przedmiotów z automatyczną kaskadą do lekcji, nauczycieli i reguł
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
    <div class="p-6 overflow-y-auto flex-1 flex flex-col gap-5">
      <!-- Dodawanie nowego przedmiotu -->
      <div class="p-3.5 bg-purple-50/50 border border-purple-200/70 rounded-xl flex flex-col sm:flex-row items-stretch sm:items-center gap-2.5">
        <input
          type="text"
          bind:value={newCustomSubjectName}
          placeholder="Wpisz nazwę nowego przedmiotu (np. Filozofia, Rysunek techniczny)..."
          class="flex-1 px-3 py-1.5 text-xs bg-white border border-purple-200 rounded-lg text-slate-800 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-purple-500/20"
        />
        <button
          type="button"
          onclick={addNewSubject}
          class="px-4 py-1.5 bg-purple-600 hover:bg-purple-700 text-white font-semibold text-xs rounded-lg transition-colors cursor-pointer shrink-0"
        >
          + Dodaj przedmiot
        </button>
      </div>

      <!-- Lista przedmiotów -->
      <div class="border border-slate-200 rounded-xl overflow-hidden shadow-2xs">
        <div class="p-3 bg-slate-50 border-b border-slate-200 flex items-center justify-between text-xs font-bold text-slate-600">
          <span>Nazwa przedmiotu ({editor.uniqueSubjects.length})</span>
          <div class="flex items-center gap-6">
            <span>Przydział / Obciążenie</span>
            <span>Akcje</span>
          </div>
        </div>

        <div class="divide-y divide-slate-100 max-h-96 overflow-y-auto">
          {#if editor.uniqueSubjects.length === 0}
            <div class="p-8 text-center text-slate-400 text-xs">
              Brak zdefiniowanych przedmiotów w szkole.
            </div>
          {:else}
            {#each editor.uniqueSubjects as sub}
              <div class="p-3 hover:bg-slate-50/80 flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs transition-colors">
                <!-- Informacja o przedmiocie -->
                <div class="flex-1">
                  {#if renamingSubject === sub.name}
                    <!-- Inline Rename Form -->
                    <div class="flex items-center gap-2">
                      <input
                        type="text"
                        bind:value={newSubjectName}
                        class="px-2.5 py-1 text-xs bg-white border border-blue-400 rounded-lg font-bold text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20"
                      />
                      <button
                        type="button"
                        onclick={applyRename}
                        class="px-2.5 py-1 bg-emerald-600 hover:bg-emerald-700 text-white font-bold text-xs rounded-lg transition-colors cursor-pointer"
                      >
                        ✓ Zapisz
                      </button>
                      <button
                        type="button"
                        onclick={() => (renamingSubject = null)}
                        class="px-2 py-1 bg-slate-200 hover:bg-slate-300 text-slate-700 text-xs rounded-lg transition-colors cursor-pointer"
                      >
                        Anuluj
                      </button>
                    </div>
                  {:else if mergingSubject === sub.name}
                    <!-- Inline Merge Form -->
                    <div class="flex items-center gap-2">
                      <span class="text-slate-500 text-xs">Scal z:</span>
                      <select
                        bind:value={mergeTargetSubject}
                        class="px-2 py-1 text-xs bg-white border border-amber-400 rounded-lg text-slate-800"
                      >
                        {#each editor.uniqueSubjects.filter((s) => s.name !== sub.name) as target}
                          <option value={target.name}>{target.name}</option>
                        {/each}
                      </select>
                      <button
                        type="button"
                        onclick={applyMerge}
                        class="px-2.5 py-1 bg-amber-600 hover:bg-amber-700 text-white font-bold text-xs rounded-lg transition-colors cursor-pointer"
                      >
                        ✓ Połącz
                      </button>
                      <button
                        type="button"
                        onclick={() => (mergingSubject = null)}
                        class="px-2 py-1 bg-slate-200 hover:bg-slate-300 text-slate-700 text-xs rounded-lg transition-colors cursor-pointer"
                      >
                        Anuluj
                      </button>
                    </div>
                  {:else}
                    <div class="flex items-center gap-2">
                      <span class="font-bold text-slate-900 text-sm">{sub.name}</span>
                    </div>
                    <div class="text-[11px] text-slate-500 flex flex-wrap items-center gap-2 mt-0.5">
                      <span>Nauczyciele: <strong>{sub.teachers.length > 0 ? sub.teachers.join(", ") : "Brak"}</strong></span>
                      <span>•</span>
                      <span>Klasy: {sub.classes.length > 0 ? sub.classes.slice(0, 4).join(", ") + (sub.classes.length > 4 ? "..." : "") : "Brak"}</span>
                    </div>
                  {/if}
                </div>

                <!-- Obciążenie i Akcje -->
                <div class="flex items-center gap-3 shrink-0">
                  <span class="px-2.5 py-1 bg-slate-100 text-slate-700 rounded-full font-semibold text-[11px]">
                    {sub.totalHours}h / tydz. ({sub.lessonCount} lekcji)
                  </span>

                  {#if renamingSubject !== sub.name && mergingSubject !== sub.name}
                    <div class="flex items-center gap-1.5">
                      <button
                        type="button"
                        onclick={() => startRename(sub.name)}
                        class="px-2 py-1 bg-blue-50 hover:bg-blue-100 text-blue-700 font-semibold rounded-lg text-xs transition-colors cursor-pointer flex items-center gap-1"
                        title="Zmień nazwę przedmiotu we wszystkich lekcjach i nauczycielach"
                      >
                        <span>✏️</span>
                        <span>Zmień nazwę</span>
                      </button>
                      {#if editor.uniqueSubjects.length > 1}
                        <button
                          type="button"
                          onclick={() => startMerge(sub.name)}
                          class="px-2 py-1 bg-amber-50 hover:bg-amber-100 text-amber-800 font-semibold rounded-lg text-xs transition-colors cursor-pointer flex items-center gap-1"
                          title="Scal ten przedmiot z innym (przenieś wszystkie lekcje)"
                        >
                          <span>🔗</span>
                          <span>Scal</span>
                        </button>
                      {/if}
                    </div>
                  {/if}
                </div>
              </div>
            {/each}
          {/if}
        </div>
      </div>
    </div>

    <!-- Footer -->
    <div class="px-6 py-3.5 border-t border-slate-100 bg-slate-50 flex items-center justify-end shrink-0">
      <button
        type="button"
        onclick={onClose}
        class="px-5 py-2 bg-slate-900 hover:bg-slate-800 text-white font-semibold rounded-xl text-xs transition-colors cursor-pointer"
      >
        Zamknij
      </button>
    </div>
  </div>
</div>

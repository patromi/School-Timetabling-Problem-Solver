<!-- frontend/src/lib/components/editor/steps/classes/ClassesTab.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();

  let filteredClasses = $derived(
    editor.selectedGradeFilter === null
      ? editor.classes
      : editor.classes.filter((c) => editor.getClassGrade(c) === editor.selectedGradeFilter)
  );
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
  <!-- Header & Grade filter -->
  <div class="flex flex-col md:flex-row md:items-center justify-between gap-3 pb-4 border-b border-slate-100">
    <div>
      <h3 class="text-sm font-bold text-slate-900">Oddziały klasowe według roczników</h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Możesz edytować nazwy klas, duplikować oddziały lub kopiować cały program nauczania.
      </p>
    </div>

    <!-- Filtry roczników -->
    <div class="flex flex-wrap items-center gap-1.5">
      <span class="text-xs font-semibold text-slate-500 mr-1">Rocznik:</span>
      <button
        type="button"
        onclick={() => (editor.selectedGradeFilter = null)}
        class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
          editor.selectedGradeFilter === null
            ? "bg-slate-900 text-white border-slate-900 shadow-2xs"
            : "bg-slate-50 text-slate-600 border-slate-200 hover:bg-slate-100"
        }`}
      >
        Wszystkie ({editor.classes.length})
      </button>
      {#each editor.availableGrades as gr}
        {@const count = editor.classes.filter((c) => editor.getClassGrade(c) === gr).length}
        <button
          type="button"
          onclick={() => (editor.selectedGradeFilter = gr)}
          class={`px-2.5 py-1 text-xs rounded-lg font-semibold transition-all cursor-pointer border ${
            editor.selectedGradeFilter === gr
              ? "bg-blue-600 text-white border-blue-600 shadow-2xs"
              : "bg-slate-50 text-slate-600 border-slate-200 hover:bg-slate-100"
          }`}
        >
          Klasy {gr}. ({count})
        </button>
      {/each}
    </div>
  </div>

  <!-- Add class input -->
  <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
    <div class="flex-1 min-w-[200px]">
      <input
        type="text"
        bind:value={editor.newClassName}
        placeholder="Nazwa nowej klasy (np. 1C lub 4D biol-chem)..."
        onkeydown={(e) => e.key === "Enter" && editor.addClass()}
        class="w-full px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
      />
    </div>
    <div class="flex items-center gap-1.5">
      <span class="text-xs text-slate-500 font-medium">Rocznik:</span>
      <select
        bind:value={editor.newClassGrade}
        class="px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
      >
        <option value={1}>Klasa 1</option>
        <option value={2}>Klasa 2</option>
        <option value={3}>Klasa 3</option>
        <option value={4}>Klasa 4</option>
        <option value={5}>Klasa 5</option>
      </select>
    </div>
    <button
      type="button"
      onclick={() => editor.addClass()}
      class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all shadow-2xs"
    >
      + Dodaj klasę
    </button>
  </div>

  <!-- Classes grid -->
  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
    {#each filteredClasses as cls}
      {@const weeklyHours = editor.getClassWeeklyHours(cls.id)}
      {@const clsLessons = editor.getClassLessons(cls.id)}
      {@const grade = editor.getClassGrade(cls)}
      <div class="p-4 bg-slate-50 rounded-2xl border border-slate-200/80 flex flex-col justify-between gap-3 text-xs hover:border-slate-300 transition-all shadow-2xs">
        <div>
          <div class="flex items-start justify-between gap-2">
            <div>
              <div class="flex items-center gap-2">
                <span class="font-bold text-slate-900 text-sm">{cls.name}</span>
                <span class="text-[10px] px-2 py-0.5 rounded-full font-bold bg-blue-50 text-blue-700 border border-blue-200">
                  Rocznik {grade}
                </span>
              </div>
              <div class="text-[11px] text-slate-500 mt-0.5">
                Łącznie: <strong>{weeklyHours} godz.</strong> / tydz. ({clsLessons.length} przedmiotów)
              </div>
            </div>

            <div class="flex items-center gap-1">
              <!-- Przycisk Edycji / Zmiany nazwy klasy -->
              <button
                type="button"
                onclick={() => (editor.editEntityTarget = { type: "class", data: cls })}
                class="text-[11px] px-2 py-1 bg-white hover:bg-slate-100 text-slate-700 rounded-lg border border-slate-200 flex items-center gap-1 transition-all cursor-pointer font-medium"
                title="Edytuj nazwę, rocznik lub ID oddziału"
              >
                <span>✏️</span>
                <span>Edytuj</span>
              </button>

              <button
                type="button"
                onclick={() => editor.openAvailabilityModal(cls.id, cls.name, "class")}
                class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                  (cls.unavailableTimes?.length || 0) > 0 || cls.minDailyLessons || cls.maxDailyLessons
                    ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                    : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                }`}
                title="Ustal dostępność klasy oraz dzienne limity lekcji"
              >
                <span>🗓️</span>
                <span>{cls.unavailableTimes?.length ? `${cls.unavailableTimes.length} wykl.` : "Dostępność"}</span>
              </button>

              <button
                type="button"
                onclick={() => editor.removeClass(cls.id)}
                class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                title="Usuń oddział klasowy"
              >
                ✕
              </button>
            </div>
          </div>

          <!-- Mini podgląd siatki przedmiotów -->
          {#if clsLessons.length > 0}
            <div class="mt-3 flex flex-wrap gap-1">
              {#each clsLessons.slice(0, 5) as l}
                <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-700 rounded-md">
                  {l.subject} ({l.weeklyCount}h)
                </span>
              {/each}
              {#if clsLessons.length > 5}
                <span class="text-[10px] px-1.5 py-0.5 text-slate-400">
                  +{clsLessons.length - 5} więcej
                </span>
              {/if}
            </div>
          {:else}
            <p class="mt-2 text-[11px] text-slate-400 italic">
              Brak przypisanych lekcji. Użyj przycisku poniżej, aby skopiować siatkę z innej klasy.
            </p>
          {/if}
        </div>

        <!-- Akcje dla klasy: duplikowanie -->
        <div class="pt-2 border-t border-slate-200/60 flex items-center justify-between gap-2 text-xs">
          <button
            type="button"
            onclick={() => (editor.duplicateModalTarget = cls)}
            class="px-2.5 py-1 bg-white hover:bg-blue-50 text-blue-700 border border-slate-200 hover:border-blue-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1.5 text-[11px]"
            title="Skopiuj wszystkie lekcje z tej klasy do innych klas"
          >
            <span>📋 Duplikuj siatkę do...</span>
          </button>

          <button
            type="button"
            onclick={() => editor.duplicateClass(cls.id)}
            class="px-2.5 py-1 bg-white hover:bg-slate-100 text-slate-700 border border-slate-200 rounded-lg font-semibold transition-all cursor-pointer flex items-center gap-1 text-[11px]"
            title="Utwórz nowy oddział na bazie tej klasy z identyczną siatką zajęć"
          >
            <span>➕ Nowy oddział</span>
          </button>
        </div>
      </div>
    {/each}
  </div>
</div>

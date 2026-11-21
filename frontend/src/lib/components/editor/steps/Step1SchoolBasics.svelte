<!-- frontend/src/lib/components/editor/steps/Step1SchoolBasics.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <!-- Pasek wyboru: Czysty start vs Gotowy szablon -->
  <div class="bg-gradient-to-r from-blue-50 to-indigo-50 border border-blue-200/90 rounded-2xl p-4 shadow-xs flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs">
    <div class="flex items-center gap-3">
      <span class="text-2xl">📋</span>
      <div>
        <span class="font-bold text-slate-900 block">Tryb tworzenia arkusza organizacyjnego</span>
        <p class="text-slate-600 text-[11px] mt-0.5">
          {#if editor.classes.length === 0 && editor.teachers.length === 0}
            Tworzysz szkołę od zera na pustym arkuszu.
          {:else}
            Arkusz zawiera {editor.classes.length} klas, {editor.teachers.length} pedagogów i {editor.lessons.length} pozycji siatki.
          {/if}
        </p>
      </div>
    </div>

    <div class="flex items-center gap-2 shrink-0">
      {#if editor.classes.length > 0 || editor.teachers.length > 0}
        <button
          type="button"
          onclick={() => {
            if (confirm("Czy na pewno chcesz wyczyścić wszystkie dane i zacząć od pustego arkusza?")) {
              editor.clearToEmptySchool();
            }
          }}
          class="px-3 py-1.5 bg-white hover:bg-rose-50 text-rose-700 border border-rose-200 rounded-xl font-semibold transition-all cursor-pointer shadow-2xs"
          title="Usuwa wszystkie klasy, kadrę i lekcje, pozostawiając czysty formularz"
        >
          🧹 Wyczyść i zacznij od zera
        </button>
      {/if}
      <button
        type="button"
        onclick={() => editor.loadHighSchoolPreset()}
        class="px-3 py-1.5 bg-blue-600 hover:bg-blue-700 text-white rounded-xl font-bold transition-all cursor-pointer shadow-xs"
        title="Wstawia przykładowy zestaw klas, nauczycieli i przedmiotów dla liceum"
      >
        ⚡ Załaduj wzorzec liceum
      </button>
    </div>
  </div>

  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div>
      <h3 class="text-base font-bold text-slate-900">Krok 1: Podstawowe informacje o placówce</h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Wprowadź oficjalną nazwę szkoły oraz określ organizację tygodnia pracy.
      </p>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
      <div>
        <label for="school-name-input" class="block text-xs font-semibold text-slate-700 mb-1">Nazwa szkoły:</label>
        <input
          id="school-name-input"
          type="text"
          bind:value={editor.schoolName}
          oninput={() => {
            if (!editor.schoolId || editor.schoolId.startsWith("Szkola") || editor.schoolId.startsWith("LO_") || editor.schoolId === "Nowa_Szkola") {
              editor.schoolId = editor.schoolName
                .trim()
                .replace(/\s+/g, "_")
                .replace(/[^a-zA-Z0-9_ąćęłńóśźżĄĆĘŁŃÓŚŹŻ-]/g, "")
                .slice(0, 30);
            }
          }}
          placeholder="np. Liceum Ogólnokształcące nr 1"
          class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
        />
      </div>
      <div>
        <div class="flex items-center justify-between mb-1">
          <label for="school-id-input" class="block text-xs font-semibold text-slate-700">Identyfikator systemowy:</label>
          <span class="text-[10px] text-slate-400 font-medium">generowany automatycznie</span>
        </div>
        <input
          id="school-id-input"
          type="text"
          bind:value={editor.schoolId}
          placeholder="np. LO_1_Wzorcowe"
          class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-lg font-mono text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500"
        />
        <p class="text-[10px] text-slate-400 mt-1">Używany w plikach XML instancji</p>
      </div>
    </div>
  </div>

  <!-- Tydzień pracy -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <h3 class="text-sm font-bold text-slate-900">Dni nauki w tygodniu</h3>
    <div class="flex flex-wrap gap-2">
      {#each ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota", "Niedziela"] as day}
        {@const isChecked = editor.selectedDays.includes(day)}
        <button
          type="button"
          onclick={() => {
            if (isChecked) {
              if (editor.selectedDays.length > 1) {
                editor.selectedDays = editor.selectedDays.filter((d: string) => d !== day);
              }
            } else {
              editor.selectedDays = [...editor.selectedDays, day];
            }
          }}
          class={`px-4 py-2 rounded-xl text-xs font-semibold border transition-all cursor-pointer flex items-center gap-2 ${
            isChecked
              ? "bg-blue-50 border-blue-300 text-blue-900 shadow-2xs"
              : "bg-slate-50 border-slate-200 text-slate-500 hover:bg-slate-100"
          }`}
        >
          <span>{isChecked ? "✓" : "+"}</span>
          <span>{day}</span>
        </button>
      {/each}
    </div>

    <div class="mt-2 pt-4 border-t border-slate-100 grid grid-cols-1 md:grid-cols-2 gap-4">
      <div>
        <label for="periods-per-day-input" class="block text-xs font-semibold text-slate-700 mb-1">
          Liczba godzin lekcyjnych dziennie:
        </label>
        <div class="flex items-center gap-3">
          <input
            id="periods-per-day-input"
            type="range"
            min="4"
            max="16"
            bind:value={editor.periodsPerDay}
            class="flex-1 cursor-pointer accent-blue-600"
          />
          <span class="w-16 text-center font-bold text-slate-900 text-sm bg-slate-100 py-1 rounded-lg">
            {editor.periodsPerDay} lekcji
          </span>
        </div>
        <p class="text-[11px] text-slate-400 mt-1">
          Typowo: 7–8 lekcji dziennie (jednozmianowa), do 16 lekcji przy pracy dwuzmianowej
        </p>
      </div>

      <div class="bg-slate-50 p-3 rounded-xl border border-slate-200/80 flex flex-col justify-between">
        <span class="text-xs text-slate-500 font-medium">Pojemność tygodniowa:</span>
        <div class="flex items-baseline gap-1 mt-1">
          <span class="text-2xl font-bold text-slate-900">{editor.weeklySlotsCapacity}</span>
          <span class="text-xs text-slate-500">dostępnych slotów na klasę</span>
        </div>
        <span class="text-[11px] text-slate-400">
          {editor.selectedDays.length} dni × {editor.periodsPerDay} lekcji = {editor.weeklySlotsCapacity} godzin lekcyjnych
        </span>
      </div>
    </div>
  </div>
</div>

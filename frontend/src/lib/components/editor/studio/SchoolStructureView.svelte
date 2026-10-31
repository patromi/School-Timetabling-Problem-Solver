<!-- frontend/src/lib/components/editor/studio/SchoolStructureView.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <!-- Status & Quick Action Card (Flat, No Gradients) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col sm:flex-row sm:items-center justify-between gap-4 text-xs">
    <div class="flex items-center gap-3">
      <div class="w-10 h-10 rounded-xl bg-blue-50 text-blue-700 flex items-center justify-center text-xl shrink-0 font-bold border border-blue-200/60">
        🏫
      </div>
      <div>
        <span class="font-bold text-slate-900 block text-sm">Organizacja i parametry szkoły</span>
        <p class="text-slate-500 text-xs mt-0.5">
          {#if editor.classes.length === 0 && editor.teachers.length === 0}
            Arkusz jest czysty — możesz zacząć od zera lub załadować wzorzec liceum.
          {:else}
            Zarejestrowano {editor.classes.length} oddziałów, {editor.teachers.length} pedagogów i {editor.rooms.length} sal lekcyjnych.
          {/if}
        </p>
      </div>
    </div>

    <div class="flex items-center gap-2 shrink-0">
      {#if editor.classes.length > 0 || editor.teachers.length > 0}
        <button
          type="button"
          onclick={() => {
            if (confirm("Czy na pewno chcesz wyczyścić cały arkusz i zacząć od zera?")) {
              editor.clearToEmptySchool();
            }
          }}
          class="px-3.5 py-2 bg-slate-50 hover:bg-rose-50 text-rose-700 border border-slate-200 hover:border-rose-200 rounded-xl font-semibold transition-all cursor-pointer shadow-2xs"
        >
          🧹 Wyczyść dane
        </button>
      {/if}
      <button
        type="button"
        onclick={() => editor.loadHighSchoolPreset()}
        class="px-4 py-2 bg-slate-900 hover:bg-slate-800 text-white rounded-xl font-bold transition-all cursor-pointer shadow-xs"
      >
        ⚡ Załaduj wzorzec liceum
      </button>
    </div>
  </div>

  <!-- School Information Form -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div>
      <h3 class="text-sm font-bold text-slate-900">Podstawowe dane placówki</h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Wprowadź oficjalną nazwę szkoły oraz techniczny identyfikator systemowy.
      </p>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
      <div>
        <label for="studio-school-name" class="block text-xs font-semibold text-slate-700 mb-1">
          Oficjalna nazwa szkoły:
        </label>
        <input
          id="studio-school-name"
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
          class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-xl text-slate-900 focus:bg-white focus:outline-none focus:border-blue-500"
        />
      </div>

      <div>
        <div class="flex items-center justify-between mb-1">
          <label for="studio-school-id" class="block text-xs font-semibold text-slate-700">
            Identyfikator instancji:
          </label>
          <span class="text-[10px] text-slate-400 font-medium">auto-slug</span>
        </div>
        <input
          id="studio-school-id"
          type="text"
          bind:value={editor.schoolId}
          placeholder="np. LO_1_Warszawa"
          class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-xl font-mono text-slate-900 focus:bg-white focus:outline-none focus:border-blue-500"
        />
        <p class="text-[10px] text-slate-400 mt-1">Używany w identyfikacji plików i wynikach solvera</p>
      </div>
    </div>
  </div>

  <!-- Weekly Structure -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div>
      <h3 class="text-sm font-bold text-slate-900">Organizacja tygodnia nauki</h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Zaznacz aktywne dni nauki oraz liczbę lekcji w ciągu jednego dnia roboczego.
      </p>
    </div>

    <div class="flex flex-wrap gap-2.5">
      {#each ["Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota"] as day}
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
        <label for="studio-periods-range" class="block text-xs font-semibold text-slate-700 mb-1">
          Liczba godzin lekcyjnych w dniu:
        </label>
        <div class="flex items-center gap-3">
          <input
            id="studio-periods-range"
            type="range"
            min="5"
            max="10"
            bind:value={editor.periodsPerDay}
            class="flex-1 cursor-pointer accent-blue-600"
          />
          <span class="w-14 text-center font-bold text-slate-900 text-sm bg-slate-100 py-1.5 rounded-lg border border-slate-200">
            {editor.periodsPerDay} lekcji
          </span>
        </div>
        <p class="text-[11px] text-slate-400 mt-1">
          Standardowo 8 lekcji (np. 8:00 - 15:30)
        </p>
      </div>

      <div class="bg-slate-50 p-4 rounded-xl border border-slate-200/80 flex flex-col justify-between text-xs">
        <span class="text-slate-500 font-medium">Pojemność siatki:</span>
        <div class="flex items-baseline gap-1.5 my-1">
          <span class="text-2xl font-extrabold text-slate-900">{editor.weeklySlotsCapacity}</span>
          <span class="text-xs text-slate-500">dostępnych godzin w tygodniu</span>
        </div>
        <span class="text-[11px] text-slate-400">
          {editor.selectedDays.length} dni × {editor.periodsPerDay} lekcji na dobę
        </span>
      </div>
    </div>
  </div>
</div>

<!-- frontend/src/lib/components/editor/steps/classes/TeachersTab.svelte -->
<script lang="ts">
  import type { EditorState } from "../../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
  <!-- Globalny pasek bilansu pensum -->
  <div class="p-4 bg-gradient-to-r from-blue-50/80 to-indigo-50/80 border border-blue-200/80 rounded-2xl flex flex-wrap items-center justify-between gap-4">
    <div class="flex items-center gap-4">
      <div class="w-10 h-10 rounded-xl bg-blue-600 text-white flex items-center justify-center text-lg font-black shadow-xs">
        ⚖️
      </div>
      <div>
        <h4 class="text-xs font-bold uppercase tracking-wider text-blue-950">
          Bilans pensum i etaty kadry pedagogicznej
        </h4>
        <p class="text-xs text-blue-800 mt-0.5">
          Możesz bezpośrednio edytować pensum każdego nauczyciela i śledzić jego obciążenie.
        </p>
      </div>
    </div>

    <div class="flex items-center gap-6 text-xs">
      <div>
        <span class="text-slate-500 block text-[11px]">Łączne pensum:</span>
        <span class="font-black text-slate-900 text-sm">{editor.totalTeacherWorkload} godz.</span>
      </div>
      <div>
        <span class="text-slate-500 block text-[11px]">Przydzielone lekcje:</span>
        <span class="font-black text-blue-700 text-sm">{editor.totalHoursAssigned} godz.</span>
      </div>
      <div>
        <span class="text-slate-500 block text-[11px]">Bilans:</span>
        {#if editor.totalHoursAssigned === editor.totalTeacherWorkload}
          <span class="font-black text-emerald-600 text-sm">Zrównoważony (0h)</span>
        {:else if editor.totalHoursAssigned > editor.totalTeacherWorkload}
          <span class="font-black text-rose-600 text-sm">+{editor.totalHoursAssigned - editor.totalTeacherWorkload}h (nadgodziny)</span>
        {:else}
          <span class="font-black text-amber-600 text-sm">-{editor.totalTeacherWorkload - editor.totalHoursAssigned}h (wakat)</span>
        {/if}
      </div>
    </div>
  </div>

  <!-- Add teacher input -->
  <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
    <input
      type="text"
      bind:value={editor.newTeacherName}
      placeholder="Imię i nazwisko nauczyciela (np. Jan Kowalski)..."
      onkeydown={(e) => e.key === "Enter" && editor.addTeacher()}
      class="flex-1 min-w-[200px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
    />
    <input
      type="text"
      bind:value={editor.newTeacherSubject}
      placeholder="Domyślny przedmiot (np. Matematyka)..."
      class="w-48 px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
    />
    <div class="flex items-center gap-1 bg-white px-2.5 py-1 rounded-lg border border-slate-200">
      <span class="text-[11px] text-slate-500">Pensum:</span>
      <input
        type="number"
        min="1"
        max="40"
        bind:value={editor.newTeacherWorkload}
        class="w-12 text-xs font-bold text-center text-slate-900 focus:outline-none"
      />
      <span class="text-[11px] text-slate-400">h</span>
    </div>
    <button
      type="button"
      onclick={() => editor.addTeacher()}
      class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all shadow-2xs"
    >
      + Dodaj nauczyciela
    </button>
  </div>

  <!-- Teachers grid -->
  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
    {#each editor.teachers as t}
      {@const assigned = editor.getTeacherAssignedHours(t.id)}
      {@const tClasses = editor.getTeacherClasses(t.id)}
      {@const workloadDiff = assigned - (t.workload || 18)}
      <div class="p-4 bg-slate-50 rounded-2xl border border-slate-200/80 flex flex-col justify-between gap-3 text-xs hover:border-slate-300 transition-all shadow-2xs">
        <div>
          <div class="flex items-start justify-between gap-2">
            <div>
              <div class="font-bold text-slate-900 text-sm">{t.name}</div>
              <div class="text-[11px] text-slate-500 flex items-center gap-1.5 mt-0.5">
                <span>Przedmiot: <strong class="text-slate-700">{t.subject}</strong></span>
              </div>

              <!-- Edycja pensum wprost z karty -->
              <div class="flex items-center gap-2 mt-2">
                <span class="text-xs text-slate-600 font-semibold">Pensum:</span>
                <input
                  type="number"
                  min="1"
                  max="60"
                  value={t.workload}
                  oninput={(e) => editor.updateTeacherWorkload(t.id, parseInt(e.currentTarget.value, 10))}
                  class="w-14 px-2 py-0.5 text-xs bg-white border border-slate-300 rounded text-slate-900 font-bold text-center focus:outline-none focus:ring-1 focus:ring-blue-500"
                  title="Edytuj wymiar pensum nauczyciela"
                />
                <span class="text-xs text-slate-500">h / tydzień</span>

                <!-- Wskaźnik zbilansowania -->
                <span class={`text-[10px] px-2 py-0.5 rounded-full font-bold ${
                  workloadDiff === 0
                    ? "bg-emerald-50 text-emerald-700 border border-emerald-200"
                    : workloadDiff > 0
                    ? "bg-rose-50 text-rose-700 border-rose-200"
                    : "bg-amber-50 text-amber-700 border-amber-200"
                }`}>
                  {assigned} / {t.workload}h ({workloadDiff === 0 ? "100%" : workloadDiff > 0 ? `+${workloadDiff}h` : `${workloadDiff}h`})
                </span>
              </div>
            </div>

            <div class="flex items-center gap-1">
              <!-- Przycisk Edycji / Zmiany nazwiska/przedmiotu -->
              <button
                type="button"
                onclick={() => (editor.editEntityTarget = { type: "teacher", data: t })}
                class="text-[11px] px-2 py-1 bg-white hover:bg-slate-100 text-slate-700 rounded-lg border border-slate-200 flex items-center gap-1 transition-all cursor-pointer font-medium"
                title="Zmień nazwisko, nauczany przedmiot lub pensum"
              >
                <span>✏️</span>
                <span>Edytuj</span>
              </button>

              <button
                type="button"
                onclick={() => editor.openAvailabilityModal(t.id, t.name, "teacher")}
                class={`text-[11px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                  (t.unavailableTimes?.length || 0) > 0 || t.maxDaysPerWeek || t.maxDailyLessons || (t.maxGapsPerWeek !== null && t.maxGapsPerWeek !== undefined)
                    ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                    : "bg-white text-slate-600 border-slate-200 hover:border-slate-300"
                }`}
                title="Ustal dostępność nauczyciela, dni pracy i okienka"
              >
                <span>🗓️</span>
                <span>{t.unavailableTimes?.length ? `${t.unavailableTimes.length} wykl.` : "Dostępność"}</span>
              </button>

              <button
                type="button"
                onclick={() => editor.removeTeacher(t.id)}
                class="text-slate-400 hover:text-rose-600 text-xs p-1 rounded cursor-pointer"
                title="Usuń nauczyciela"
              >
                ✕
              </button>
            </div>
          </div>

          <!-- Podgląd przypisanych klas -->
          <div class="mt-3">
            <span class="text-[10px] font-bold uppercase tracking-wider text-slate-400 block mb-1">
              Przydzielone oddziały ({tClasses.length}):
            </span>
            {#if tClasses.length > 0}
              <div class="flex flex-wrap gap-1">
                {#each tClasses as tc}
                  <span class="text-[10px] px-2 py-0.5 bg-white border border-slate-200 text-slate-800 rounded-md font-medium">
                    {tc.className} ({tc.subject}: {tc.hours}h)
                  </span>
                {/each}
              </div>
            {:else}
              <span class="text-[11px] text-slate-400 italic">
                Brak przypisanych klas (0 godzin).
              </span>
            {/if}
          </div>
        </div>
      </div>
    {/each}
  </div>
</div>

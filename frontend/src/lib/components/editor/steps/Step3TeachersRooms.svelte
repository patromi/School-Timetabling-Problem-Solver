<!-- frontend/src/lib/components/editor/steps/Step3TeachersRooms.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="flex flex-col gap-6">
  <!-- Nagłówek bazy lokalowej -->
  <div class="bg-gradient-to-r from-slate-900 to-slate-800 text-white rounded-2xl p-5 shadow-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div class="flex items-center gap-3.5">
      <div class="w-11 h-11 rounded-2xl bg-white/10 flex items-center justify-center text-xl shrink-0">
        🚪
      </div>
      <div>
        <div class="flex items-center gap-2">
          <h3 class="text-sm font-bold text-white">Baza lokalowa szkoły ({editor.rooms.length} sal)</h3>
          <span class="text-[10px] px-2 py-0.5 rounded-full font-bold bg-white/20 text-white">
            Krok 3 z 6
          </span>
        </div>
        <p class="text-xs text-slate-300 mt-0.5">
          Zdefiniuj sale ogólne, pracownie profilowane (laboratoria, sale komputerowe) oraz obiekty sportowe.
        </p>
      </div>
    </div>

    <div class="flex items-center gap-2 text-xs bg-white/10 px-3.5 py-2 rounded-xl border border-white/10 text-slate-200">
      <span>Łączna liczba sal:</span>
      <strong class="text-white text-sm font-bold">{editor.rooms.length}</strong>
    </div>
  </div>

  <!-- Baza lokalowa szkoły -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-5">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900">Baza lokalowa szkoły ({editor.rooms.length} sal)</h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Zdefiniuj dostępne sale ogólne, pracownie specjalistyczne oraz hale sportowe.
        </p>
      </div>
    </div>

    <!-- Add room input -->
    <div class="flex flex-wrap items-center gap-2 bg-slate-50 p-3 rounded-xl border border-slate-200/70">
      <input
        type="text"
        bind:value={editor.newRoomName}
        placeholder="Nazwa sali (np. Sala 105 lub Pracownia fizyczna)"
        onkeydown={(e) => e.key === "Enter" && editor.addRoom()}
        class="flex-1 min-w-[200px] px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:border-blue-500"
      />
      <select
        bind:value={editor.newRoomType}
        class="px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-800 cursor-pointer focus:outline-none focus:border-blue-500"
      >
        <option value="general">Sala ogólna / dydaktyczna</option>
        <option value="gym">Sala gimnastyczna / WF</option>
        <option value="lab">Laboratorium / Pracownia przyrodnicza</option>
        <option value="it">Pracownia komputerowa</option>
      </select>
      <button
        type="button"
        onclick={() => editor.addRoom()}
        class="px-4 py-1.5 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg cursor-pointer transition-all"
      >
        + Dodaj salę
      </button>
    </div>

    <!-- Rooms grid -->
    <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-3">
      {#each editor.rooms as room}
        <div class="p-3.5 bg-slate-50 rounded-xl border border-slate-100 flex items-center justify-between text-xs hover:border-slate-300 transition-colors">
          <div class="flex items-center gap-2.5">
            <span class="text-lg">
              {#if room.type === 'gym'}🏀
              {:else if room.type === 'lab'}🔬
              {:else if room.type === 'it'}💻
              {:else}🚪{/if}
            </span>
            <div>
              <div class="font-bold text-slate-900">{room.name}</div>
              <div class="text-[10px] text-slate-500 capitalize">Typ: {room.type}</div>
            </div>
          </div>
          <div class="flex items-center gap-1.5">
            <!-- Przycisk Edycji / Zmiany nazwy sali -->
            <button
              type="button"
              onclick={() => (editor.editEntityTarget = { type: "room", data: room })}
              class="text-[10px] px-2 py-1 bg-white hover:bg-slate-100 text-slate-700 rounded-lg border border-slate-200 flex items-center gap-1 transition-all cursor-pointer font-medium"
              title="Edytuj nazwę lub typ sali"
            >
              <span>✏️</span>
            </button>

            <button
              type="button"
              onclick={() => editor.openAvailabilityModal(room.id, room.name, "room")}
              class={`text-[10px] px-2 py-1 rounded-lg border flex items-center gap-1 transition-all cursor-pointer ${
                (room.unavailableTimes?.length || 0) > 0
                  ? "bg-rose-50 text-rose-700 border-rose-200 font-semibold"
                  : "bg-white text-slate-500 border-slate-200 hover:border-slate-300"
              }`}
              title="Ustal dostępność sali w tygodniu (AvoidUnavailableTimesConstraint)"
            >
              <span>🗓️</span>
              <span>{room.unavailableTimes?.length ? `${room.unavailableTimes.length} wykl.` : "Dostępność"}</span>
            </button>
            <button
              type="button"
              onclick={() => editor.removeRoom(room.id)}
              class="text-slate-400 hover:text-rose-600 text-xs px-1.5 py-0.5 rounded cursor-pointer"
              title="Usuń salę"
            >
              ✕
            </button>
          </div>
        </div>
      {/each}
    </div>
  </div>
</div>

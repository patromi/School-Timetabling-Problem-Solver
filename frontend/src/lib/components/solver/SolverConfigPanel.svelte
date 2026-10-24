<script lang="ts">
  import type { InstanceListItem, SolverStatus } from "../../types";

  export type Profile = "quick" | "standard" | "deep";

  interface Props {
    instances: InstanceListItem[];
    chosenInstanceId: string;
    chosenProfile: Profile;
    customIterations: number;
    chosenSelector: string;
    chosenSeed: number;
    solverStatus: SolverStatus | null;
    currentSchool?: InstanceListItem;
    onStart: () => void;
    onStop: () => void;
  }

  let {
    instances,
    chosenInstanceId = $bindable(),
    chosenProfile = $bindable(),
    customIterations = $bindable(),
    chosenSelector = $bindable(),
    chosenSeed = $bindable(),
    solverStatus,
    currentSchool,
    onStart,
    onStop,
  }: Props = $props();

  let showAdvancedSettings = $state(false);
</script>

<div class="flex flex-col gap-5 bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs">
  <div>
    <label for="school-select" class="block text-xs font-bold text-slate-800 mb-1.5">
      1. Wybierz szkołę do ułożenia:
    </label>
    <select
      id="school-select"
      bind:value={chosenInstanceId}
      disabled={solverStatus?.status === "running"}
      class="w-full px-3 py-2 text-xs bg-slate-50 border border-slate-200 rounded-xl text-slate-900 font-medium focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 cursor-pointer"
    >
      {#each instances as inst}
        <option value={inst.id}>
          {inst.name || inst.id} ({inst.is_custom ? 'Własna' : 'Wzorcowa'})
        </option>
      {/each}
    </select>

    {#if currentSchool}
      <div class="mt-2 p-2.5 bg-slate-50 rounded-xl border border-slate-100 text-[11px] text-slate-500 flex flex-col gap-1">
        <span class="font-semibold text-slate-700">{currentSchool.name || currentSchool.id}</span>
        <span>📚 {currentSchool.events_count} lekcji • 👥 {currentSchool.resources_count} osób/sal</span>
      </div>
    {/if}
  </div>

  <!-- Optimization Profile -->
  <div>
    <span class="block text-xs font-bold text-slate-800 mb-2">
      2. Tryb optymalizacji:
    </span>
    <div class="flex flex-col gap-2">
      <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
        <input
          type="radio"
          name="profile"
          value="standard"
          bind:group={chosenProfile}
          disabled={solverStatus?.status === "running"}
          class="mt-0.5 accent-blue-600"
        />
        <div>
          <span class="text-xs font-bold text-slate-900">🚀 Standardowy (Zalecany)</span>
          <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
            20 000 prób optymalizacji (~3 sekundy). Znakomity balans jakości i szybkości.
          </p>
        </div>
      </label>

      <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
        <input
          type="radio"
          name="profile"
          value="quick"
          bind:group={chosenProfile}
          disabled={solverStatus?.status === "running"}
          class="mt-0.5 accent-blue-600"
        />
        <div>
          <span class="text-xs font-bold text-slate-900">⚡ Szybki test</span>
          <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
            5 000 prób (~1 sekunda). Do błyskawicznej weryfikacji poprawności siatki.
          </p>
        </div>
      </label>

      <label class="p-3 bg-slate-50 rounded-xl border border-slate-200 cursor-pointer hover:border-blue-300 transition-all flex items-start gap-2.5">
        <input
          type="radio"
          name="profile"
          value="deep"
          bind:group={chosenProfile}
          disabled={solverStatus?.status === "running"}
          class="mt-0.5 accent-blue-600"
        />
        <div>
          <span class="text-xs font-bold text-slate-900">🎯 Głęboka optymalizacja</span>
          <p class="text-[11px] text-slate-500 leading-snug mt-0.5">
            50 000 prób (~8 sekund). Maksymalny komfort planu i eliminacja trudnych okienek.
          </p>
        </div>
      </label>
    </div>
  </div>

  <!-- Advanced Settings Accordion -->
  <div class="pt-2 border-t border-slate-100">
    <button
      type="button"
      onclick={() => (showAdvancedSettings = !showAdvancedSettings)}
      class="text-xs font-semibold text-slate-500 hover:text-slate-800 cursor-pointer flex items-center gap-1"
    >
      <span>{showAdvancedSettings ? 'Ukryj parametry zaawansowane' : 'Pokaż parametry zaawansowane'}</span>
      <span>{showAdvancedSettings ? '▲' : '▼'}</span>
    </button>

    {#if showAdvancedSettings}
      <div class="mt-3 flex flex-col gap-2.5 text-xs bg-slate-50 p-3 rounded-xl border border-slate-200">
        <div>
          <label for="custom-iterations-input" class="block text-[10px] font-mono text-slate-500 mb-0.5">Liczba iteracji:</label>
          <input
            id="custom-iterations-input"
            type="number"
            bind:value={customIterations}
            class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono"
          />
        </div>
        <div>
          <label for="chosen-selector-select" class="block text-[10px] font-mono text-slate-500 mb-0.5">Heurystyka wyboru (Selector):</label>
          <select id="chosen-selector-select" bind:value={chosenSelector} class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono">
            <option value="ucb">UCB (Multi-Armed Bandit)</option>
            <option value="epsilon-greedy">Epsilon-Greedy</option>
            <option value="random">Random Selector</option>
          </select>
        </div>
        <div>
          <label for="chosen-seed-input" class="block text-[10px] font-mono text-slate-500 mb-0.5">Ziarno losowe (Seed):</label>
          <input
            id="chosen-seed-input"
            type="number"
            bind:value={chosenSeed}
            class="w-full px-2 py-1 bg-white border border-slate-200 rounded text-xs font-mono"
          />
        </div>
      </div>
    {/if}
  </div>

  <!-- Start / Stop Button -->
  <div class="mt-auto pt-4 border-t border-slate-100">
    {#if solverStatus?.status === "running"}
      <button
        type="button"
        onclick={onStop}
        class="w-full py-3 bg-rose-600 hover:bg-rose-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center justify-center gap-2 animate-pulse"
      >
        <span>⏹ Zatrzymaj solver</span>
      </button>
    {:else}
      <button
        type="button"
        onclick={onStart}
        class="w-full py-3 bg-blue-600 hover:bg-blue-700 text-white font-bold text-xs rounded-xl shadow-xs transition-all cursor-pointer flex items-center justify-center gap-2"
      >
        <span>▶ Rozpocznij generowanie planu</span>
      </button>
    {/if}
  </div>
</div>

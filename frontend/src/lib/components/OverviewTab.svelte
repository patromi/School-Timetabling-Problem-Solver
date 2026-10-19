<script lang="ts">
  import type { InstanceDetail } from "../types";
  import {
    computeResourceWorkloads,
    CONSTRAINT_FRIENDLY_MAP,
    getResourceCategory,
  } from "../school_helpers";

  interface Props {
    instance: InstanceDetail;
    onGoToTab: (tab: string) => void;
  }

  let { instance, onGoToTab }: Props = $props();

  let daysCount = $derived(
    instance.time_groups.filter((g) => g.kind.toLowerCase() === "day").length ||
      instance.time_groups.length || 5
  );

  let totalDuration = $derived(
    instance.events.reduce((sum, e) => sum + (e.duration || 1), 0)
  );

  let teachers = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "teacher")
  );

  let classes = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "class")
  );

  let rooms = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "room")
  );

  let workloads = $derived(computeResourceWorkloads(instance));

  let avgTeacherLoad = $derived(
    teachers.length > 0 ? Math.round(totalDuration / teachers.length) : 0
  );

  let avgClassLoad = $derived(
    classes.length > 0 ? Math.round(totalDuration / classes.length) : 0
  );

  let hardConstraints = $derived(
    instance.constraints.filter((c) => c.required).length
  );

  let softConstraints = $derived(
    instance.constraints.filter((c) => !c.required).length
  );

  // Główne typy reguł występujące w tej szkole
  let uniqueConstraintTypes = $derived(
    Array.from(new Set(instance.constraints.map((c) => c.type)))
  );
</script>

<div class="p-6 max-w-6xl mx-auto flex flex-col gap-6 text-slate-800">
  <!-- Welcome / School Introduction Banner -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
    <div class="flex flex-col gap-1.5 max-w-2xl">
      <div class="flex items-center gap-2">
        <span class="text-xs font-semibold px-2 py-0.5 bg-blue-50 text-blue-700 rounded-full border border-blue-100">
          Gotowy do układania planu
        </span>
        <span class="text-xs text-slate-400">•</span>
        <span class="text-xs text-slate-500">Arkusz zweryfikowany</span>
      </div>
      <h2 class="text-lg font-bold text-slate-900 tracking-tight">
        Podsumowanie organizacyjne: {instance.name || instance.id}
      </h2>
      <p class="text-xs text-slate-600 leading-relaxed">
        Poniżej przedstawiono główne parametry planu lekcji. Tydzień szkolny obejmuje <strong>{daysCount} dni</strong> oraz łącznie <strong>{instance.times.length} godzin lekcyjnych</strong> w siatce. Wszystkie zajęcia wymagają optymalnego rozłożenia w czasie i przestrzeni.
      </p>
    </div>

    <!-- Quick action links -->
    <div class="flex flex-row md:flex-col gap-2 shrink-0 w-full md:w-auto">
      <button
        onclick={() => onGoToTab("resources")}
        class="flex-1 md:flex-initial px-4 py-2 bg-slate-900 hover:bg-slate-800 text-white text-xs font-semibold rounded-lg transition-all cursor-pointer text-center"
      >
        👥 Przejdź do kadry i klas
      </button>
      <button
        onclick={() => onGoToTab("events")}
        class="flex-1 md:flex-initial px-4 py-2 bg-white hover:bg-slate-50 text-slate-700 text-xs font-semibold rounded-lg border border-slate-200 transition-all cursor-pointer text-center"
      >
        📚 Zobacz wykaz lekcji
      </button>
    </div>
  </div>

  <!-- 3 Organization Cards -->
  <div class="grid grid-cols-1 md:grid-cols-3 gap-5">
    <!-- Card 1: Kadra i Uczniowie -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col justify-between">
      <div>
        <div class="flex items-center justify-between pb-3 border-b border-slate-100">
          <span class="text-xs font-bold uppercase tracking-wider text-slate-500">👥 Społeczność szkolna</span>
          <button type="button" class="text-xs font-semibold text-blue-600 cursor-pointer hover:underline bg-transparent border-0 p-0" onclick={() => onGoToTab("resources")}>Szczegóły →</button>
        </div>
        <div class="mt-4 flex flex-col gap-3">
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Nauczyciele prowadzący:</span>
            <span class="font-bold text-slate-900">{teachers.length || 0} osób</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Średnie pensum nauczyciela:</span>
            <span class="font-semibold text-slate-900">{avgTeacherLoad} godz./tydz.</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Oddziały klasowe:</span>
            <span class="font-bold text-slate-900">{classes.length || instance.resources.length} oddziałów</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Średni wymiar lekcji klasy:</span>
            <span class="font-semibold text-slate-900">{avgClassLoad} godz./tydz.</span>
          </div>
        </div>
      </div>
      <div class="mt-4 pt-3 border-t border-slate-100 text-[11px] text-slate-400">
        Łącznie {instance.resources.length} zdefiniowanych podmiotów
      </div>
    </div>

    <!-- Card 2: Baza lokalowa i Tydzień -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col justify-between">
      <div>
        <div class="flex items-center justify-between pb-3 border-b border-slate-100">
          <span class="text-xs font-bold uppercase tracking-wider text-slate-500">🚪 Sale i Harmonogram</span>
          <span class="text-xs font-semibold text-slate-400 font-mono">{instance.times.length} slotów</span>
        </div>
        <div class="mt-4 flex flex-col gap-3">
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Sale i pracownie dydaktyczne:</span>
            <span class="font-bold text-slate-900">{rooms.length || 0} sal</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Dni pracy w tygodniu:</span>
            <span class="font-bold text-slate-900">{daysCount} dni (Pon-Pt)</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Lekcji dziennie w siatce:</span>
            <span class="font-semibold text-slate-900">ok. {Math.round(instance.times.length / daysCount)} godzin</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Tygodniowy wolumen lekcji:</span>
            <span class="font-bold text-slate-900">{totalDuration} godzin</span>
          </div>
        </div>
      </div>
      <div class="mt-4 pt-3 border-t border-slate-100 text-[11px] text-slate-400">
        Średnie obciążenie sal: {rooms.length > 0 ? Math.round(totalDuration / rooms.length) : 0} godz./tydz.
      </div>
    </div>

    <!-- Card 3: Reguły Jakościowe i Wymogi -->
    <div class="bg-white border border-slate-200/90 rounded-2xl p-5 shadow-xs flex flex-col justify-between">
      <div>
        <div class="flex items-center justify-between pb-3 border-b border-slate-100">
          <span class="text-xs font-bold uppercase tracking-wider text-slate-500">⚖️ Standardy planu</span>
          <button type="button" class="text-xs font-semibold text-blue-600 cursor-pointer hover:underline bg-transparent border-0 p-0" onclick={() => onGoToTab("constraints")}>Zobacz reguły →</button>
        </div>
        <div class="mt-4 flex flex-col gap-3">
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Wymogi bezwzględne (konieczne):</span>
            <span class="font-bold text-rose-700 bg-rose-50 px-2 py-0.5 rounded-full">{hardConstraints} warunków</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Preferencje (wygoda uczniów i kadry):</span>
            <span class="font-bold text-amber-800 bg-amber-50 px-2 py-0.5 rounded-full">{softConstraints} reguł</span>
          </div>
          <div class="flex items-center justify-between text-xs">
            <span class="text-slate-600">Główne rodzaje kryteriów:</span>
            <span class="font-semibold text-slate-900">{uniqueConstraintTypes.length} standardów</span>
          </div>
        </div>
      </div>
      <div class="mt-4 pt-3 border-t border-slate-100 text-[11px] text-slate-400">
        Solver automatycznie optymalizuje spełnienie obu kryteriów
      </div>
    </div>
  </div>

  <!-- Friendly Rules Section (Przejrzyste zasady dla człowieka) -->
  <div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
    <div class="flex items-center justify-between">
      <div>
        <h3 class="text-sm font-bold text-slate-900">
          Zasady i cele organizacji planu lekcji w tej szkole
        </h3>
        <p class="text-xs text-slate-500 mt-0.5">
          Algorytm optymalizacyjny uwzględnia następujące standardy dydaktyczne i higieny nauki:
        </p>
      </div>
      <button
        onclick={() => onGoToTab("constraints")}
        class="text-xs font-semibold text-blue-600 hover:text-blue-700 cursor-pointer"
      >
        Pełna lista ({instance.constraints.length}) →
      </button>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-3 mt-1">
      {#each uniqueConstraintTypes.slice(0, 6) as type}
        {@const info = CONSTRAINT_FRIENDLY_MAP[type] || { title: type, description: "Zasada organizacji zajęć w placówce.", isHardByDefault: true }}
        <div class="p-3.5 bg-slate-50/80 rounded-xl border border-slate-200/70 flex items-start gap-3">
          <div class="w-8 h-8 rounded-lg bg-white border border-slate-200 flex items-center justify-center shrink-0 text-sm shadow-2xs">
            {#if type.includes("Clash")}
              🛡️
            {:else if type.includes("Time")}
              ⏱️
            {:else if type.includes("Gap")}
              ☕
            {:else if type.includes("Day") || type.includes("Busy")}
              ⚖️
            {:else if type.includes("Spread")}
              🗓️
            {:else}
              📋
            {/if}
          </div>
          <div class="flex flex-col gap-1">
            <span class="text-xs font-bold text-slate-900">{info.title}</span>
            <p class="text-[11px] text-slate-600 leading-snug">{info.description}</p>
          </div>
        </div>
      {/each}
    </div>
  </div>
</div>

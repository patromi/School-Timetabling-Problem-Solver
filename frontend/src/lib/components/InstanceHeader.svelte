<script lang="ts">
  import type { InstanceDetail } from "../types";
  import { getExportInstanceUrl } from "../api";
  import { getResourceCategory } from "../school_helpers";

  interface Props {
    instance: InstanceDetail;
    activeTab: string;
    onTabChange: (tab: string) => void;
    onEdit?: () => void;
  }

  let { instance, activeTab, onTabChange, onEdit }: Props = $props();

  let daysCount = $derived(
    instance.time_groups.filter((g) => g.kind.toLowerCase() === "day").length ||
      instance.time_groups.length || 5
  );

  let totalDuration = $derived(
    instance.events.reduce((sum, e) => sum + (e.duration || 1), 0)
  );

  let teachersCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "teacher").length
  );

  let classesCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "class").length
  );

  let roomsCount = $derived(
    instance.resources.filter((r) => getResourceCategory(r, instance) === "room").length
  );

  let hardConstraints = $derived(
    instance.constraints.filter((c) => c.required).length
  );

  let softConstraints = $derived(
    instance.constraints.filter((c) => !c.required).length
  );
</script>

<div class="bg-white border-b border-slate-200/90 px-6 pt-5 pb-0 text-slate-800 flex flex-col gap-5 shadow-2xs">
  <!-- Title & Action -->
  <div class="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
    <div>
      <div class="flex items-center gap-2">
        <h1 class="text-xl font-bold text-slate-900 tracking-tight">
          {instance.name || instance.id}
        </h1>
        <span class="text-xs px-2 py-0.5 bg-slate-100 text-slate-600 rounded font-mono border border-slate-200">
          {instance.id}
        </span>
      </div>
      <p class="text-xs text-slate-500 mt-1">
        Tygodniowy plan zajęć dydaktycznych • Organizacja: {daysCount} dni w tygodniu • Łącznie {instance.times.length} godzin lekcyjnych
      </p>
    </div>

    <div class="flex items-center gap-2">
      {#if onEdit}
        <button
          type="button"
          onclick={onEdit}
          class="flex items-center gap-1.5 px-3 py-1.5 bg-blue-50 hover:bg-blue-100 text-blue-800 text-xs font-semibold rounded-lg border border-blue-200 transition-all cursor-pointer shadow-2xs"
          title="Otwórz arkusz tej szkoły w edytorze"
        >
          <span>✏️ Edytuj tę szkołę</span>
        </button>
      {/if}
      <a
        href={getExportInstanceUrl(instance.id)}
        download={`${instance.id}.xml`}
        class="flex items-center gap-1.5 px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg border border-slate-300/80 transition-all cursor-pointer shadow-2xs"
      >
        <span>📥 Pobierz plik XML</span>
      </a>
    </div>
  </div>

  <!-- 4 Key Metric Cards (Clean, Human, High Contrast) -->
  <div class="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs">
    <!-- Classes / Students -->
    <div class="bg-slate-50/70 p-3.5 rounded-xl border border-slate-200 flex flex-col justify-between">
      <div class="flex items-center justify-between">
        <span class="text-xs font-semibold text-slate-600">👥 Klasy i Oddziały</span>
        <span class="text-[10px] text-slate-400 font-medium">Uczniowie</span>
      </div>
      <div class="mt-2 flex items-baseline gap-1.5">
        <span class="text-2xl font-bold text-slate-900">{classesCount || instance.resources.length}</span>
        <span class="text-slate-500 text-xs">oddziałów</span>
      </div>
      <span class="text-[11px] text-slate-500 mt-1">
        Zarejestrowane grupy klasowe
      </span>
    </div>

    <!-- Teachers -->
    <div class="bg-slate-50/70 p-3.5 rounded-xl border border-slate-200 flex flex-col justify-between">
      <div class="flex items-center justify-between">
        <span class="text-xs font-semibold text-slate-600">👨‍🏫 Nauczyciele</span>
        <span class="text-[10px] text-slate-400 font-medium">Kadra</span>
      </div>
      <div class="mt-2 flex items-baseline gap-1.5">
        <span class="text-2xl font-bold text-slate-900">{teachersCount || 0}</span>
        <span class="text-slate-500 text-xs">pedagogów</span>
      </div>
      <span class="text-[11px] text-slate-500 mt-1">
        Prowadzący zajęcia dydaktyczne
      </span>
    </div>

    <!-- Classrooms / Rooms -->
    <div class="bg-slate-50/70 p-3.5 rounded-xl border border-slate-200 flex flex-col justify-between">
      <div class="flex items-center justify-between">
        <span class="text-xs font-semibold text-slate-600">🚪 Sale lekcyjne</span>
        <span class="text-[10px] text-slate-400 font-medium">Baza</span>
      </div>
      <div class="mt-2 flex items-baseline gap-1.5">
        <span class="text-2xl font-bold text-slate-900">{roomsCount || 0}</span>
        <span class="text-slate-500 text-xs">sal i pracowni</span>
      </div>
      <span class="text-[11px] text-slate-500 mt-1">
        Dostępne przestrzenie nauczania
      </span>
    </div>

    <!-- Total Lessons / Volume -->
    <div class="bg-slate-50/70 p-3.5 rounded-xl border border-slate-200 flex flex-col justify-between">
      <div class="flex items-center justify-between">
        <span class="text-xs font-semibold text-slate-600">📚 Zajęcia w tygodniu</span>
        <span class="text-[10px] text-slate-400 font-medium">Wymiar</span>
      </div>
      <div class="mt-2 flex items-baseline gap-1.5">
        <span class="text-2xl font-bold text-slate-900">{totalDuration}</span>
        <span class="text-slate-500 text-xs">godzin lekcyjnych</span>
      </div>
      <span class="text-[11px] text-slate-500 mt-1">
        W {instance.events.length} pozycjach planu
      </span>
    </div>
  </div>

  <!-- Clean, Non-Technical Navigation Tabs -->
  <div class="flex items-center gap-1 border-b border-slate-200 -mb-px text-xs font-medium overflow-x-auto">
    <button
      onclick={() => onTabChange("overview")}
      class={`px-4 py-2.5 border-b-2 transition-all cursor-pointer shrink-0 flex items-center gap-1.5 ${
        activeTab === "overview"
          ? "border-blue-600 text-blue-600 font-semibold"
          : "border-transparent text-slate-500 hover:text-slate-800"
      }`}
    >
      <span>📋 Podsumowanie szkoły</span>
    </button>

    <button
      onclick={() => onTabChange("resources")}
      class={`px-4 py-2.5 border-b-2 transition-all cursor-pointer shrink-0 flex items-center gap-1.5 ${
        activeTab === "resources"
          ? "border-blue-600 text-blue-600 font-semibold"
          : "border-transparent text-slate-500 hover:text-slate-800"
      }`}
    >
      <span>👥 Nauczyciele i Klasy</span>
      <span class="px-1.5 py-0.2 bg-slate-100 text-slate-600 rounded-full text-[10px]">
        {instance.resources.length}
      </span>
    </button>

    <button
      onclick={() => onTabChange("events")}
      class={`px-4 py-2.5 border-b-2 transition-all cursor-pointer shrink-0 flex items-center gap-1.5 ${
        activeTab === "events"
          ? "border-blue-600 text-blue-600 font-semibold"
          : "border-transparent text-slate-500 hover:text-slate-800"
      }`}
    >
      <span>📚 Wykaz lekcji</span>
      <span class="px-1.5 py-0.2 bg-slate-100 text-slate-600 rounded-full text-[10px]">
        {instance.events.length}
      </span>
    </button>

    <button
      onclick={() => onTabChange("constraints")}
      class={`px-4 py-2.5 border-b-2 transition-all cursor-pointer shrink-0 flex items-center gap-1.5 ${
        activeTab === "constraints"
          ? "border-blue-600 text-blue-600 font-semibold"
          : "border-transparent text-slate-500 hover:text-slate-800"
      }`}
    >
      <span>⚖️ Zasady i wymagania</span>
      <span class="px-1.5 py-0.2 bg-slate-100 text-slate-600 rounded-full text-[10px]">
        {hardConstraints + softConstraints}
      </span>
    </button>

    <button
      onclick={() => onTabChange("technical")}
      class={`px-4 py-2.5 border-b-2 transition-all cursor-pointer shrink-0 flex items-center gap-1.5 ml-auto ${
        activeTab === "technical"
          ? "border-slate-800 text-slate-900 font-semibold"
          : "border-transparent text-slate-400 hover:text-slate-600"
      }`}
    >
      <span>⚙️ Szczegóły techniczne</span>
    </button>
  </div>
</div>

<!-- frontend/src/lib/components/editor/availability/AvailabilityContextDrawer.svelte -->
<script lang="ts">
  import type { ResourceContextInfo } from "../AvailabilityModal.svelte";
  import Icon from "../../common/Icon.svelte";

  interface Props {
    resourceType: "teacher" | "room" | "class";
    contextInfo: ResourceContextInfo;
    availableCount: number;
    totalSlots: number;
  }

  let {
    resourceType,
    contextInfo,
    availableCount,
    totalSlots,
  }: Props = $props();
</script>

<div class="w-full md:w-80 shrink-0 border-t md:border-t-0 md:border-l border-slate-200 bg-slate-50/70 p-5 flex flex-col gap-4 overflow-y-auto">
  <div class="flex items-center justify-between pb-3 border-b border-slate-200">
    <h4 class="text-xs font-bold uppercase tracking-wider text-slate-800 flex items-center gap-1.5">
      <Icon name="bar_chart" size="xs" />
      <span>Podgląd obciążenia</span>
    </h4>
    <span class="text-[10px] px-2 py-0.5 rounded-full font-bold bg-blue-100 text-blue-800">
      {resourceType === "teacher" ? "Pedagog" : resourceType === "class" ? "Oddział" : "Sala"}
    </span>
  </div>

  <!-- KPI Summary Card -->
  <div class="p-3.5 bg-white rounded-xl border border-slate-200/90 shadow-2xs flex flex-col gap-2.5 text-xs">
    {#if resourceType === "teacher"}
      <div class="flex items-center justify-between">
        <span class="text-slate-500">Pensum etatowe:</span>
        <span class="font-bold text-slate-900">{contextInfo.workload ?? 18} godz./tydz.</span>
      </div>
      <div class="flex items-center justify-between">
        <span class="text-slate-500">Przydzielone lekcje:</span>
        <span class="font-bold text-blue-700">{contextInfo.totalAssignedHours ?? 0} godz.</span>
      </div>
      <div class="flex items-center justify-between pt-2 border-t border-slate-100">
        <span class="text-slate-500">Dostępne sloty:</span>
        <span class="font-bold {availableCount < (contextInfo.totalAssignedHours ?? 0) ? 'text-rose-600' : 'text-emerald-700'}">
          {availableCount} / {totalSlots}
        </span>
      </div>
    {:else if resourceType === "class"}
      <div class="flex items-center justify-between">
        <span class="text-slate-500">Lekcje w tygodniu:</span>
        <span class="font-bold text-blue-700">{contextInfo.totalAssignedHours ?? 0} godz.</span>
      </div>
      <div class="flex items-center justify-between pt-2 border-t border-slate-100">
        <span class="text-slate-500">Dostępne sloty:</span>
        <span class="font-bold {availableCount < (contextInfo.totalAssignedHours ?? 0) ? 'text-rose-600' : 'text-emerald-700'}">
          {availableCount} / {totalSlots}
        </span>
      </div>
    {:else}
      <div class="flex items-center justify-between">
        <span class="text-slate-500">Zajętość sali:</span>
        <span class="font-bold text-blue-700">{contextInfo.totalAssignedHours ?? 0} godz./tydz.</span>
      </div>
    {/if}
  </div>

  <!-- Conflict Live Warning -->
  {#if (contextInfo.totalAssignedHours ?? 0) > availableCount}
    <div class="p-3 bg-rose-50 border border-rose-200 rounded-xl text-rose-900 text-xs flex items-start gap-2">
      <Icon name="warning" size="xs" class="text-rose-600 shrink-0 mt-0.5" />
      <div class="leading-snug">
        <strong class="font-bold block">Zbyt wiele wykluczeń!</strong>
        Przydział wymaga <strong>{contextInfo.totalAssignedHours} godz.</strong>, a odblokowano tylko <strong>{availableCount} slotów</strong>.
      </div>
    </div>
  {/if}

  <!-- Assigned Classes / Lessons List -->
  {#if resourceType === "teacher"}
    <div>
      <span class="text-[11px] font-bold uppercase tracking-wider text-slate-500 block mb-2">
        Przydzielone oddziały ({contextInfo.classes?.length || 0}):
      </span>
      {#if !contextInfo.classes || contextInfo.classes.length === 0}
        <p class="text-xs text-slate-400 italic bg-white p-3 rounded-xl border border-dashed border-slate-200 text-center">
          Brak przypisanych klas.
        </p>
      {:else}
        <div class="flex flex-col gap-1.5 max-h-56 overflow-y-auto">
          {#each contextInfo.classes as c}
            <div class="p-2.5 bg-white border border-slate-200/90 rounded-xl flex items-center justify-between text-xs">
              <div>
                <span class="font-bold text-slate-900 block">{c.className}</span>
                <span class="text-[10px] text-slate-500">{c.subject}</span>
              </div>
              <span class="px-2 py-0.5 bg-blue-50 text-blue-700 font-bold rounded-md text-[11px]">
                {c.hours}h
              </span>
            </div>
          {/each}
        </div>
      {/if}
    </div>
  {:else if resourceType === "class"}
    <div>
      <span class="text-[11px] font-bold uppercase tracking-wider text-slate-500 block mb-2">
        Przedmioty i nauczyciele ({contextInfo.lessons?.length || 0}):
      </span>
      {#if !contextInfo.lessons || contextInfo.lessons.length === 0}
        <p class="text-xs text-slate-400 italic bg-white p-3 rounded-xl border border-dashed border-slate-200 text-center">
          Brak przypisanych lekcji.
        </p>
      {:else}
        <div class="flex flex-col gap-1.5 max-h-56 overflow-y-auto">
          {#each contextInfo.lessons as l}
            <div class="p-2.5 bg-white border border-slate-200/90 rounded-xl flex items-center justify-between text-xs">
              <div>
                <span class="font-bold text-slate-900 block">{l.subject}</span>
                <span class="text-[10px] text-slate-500">{l.teacherName}</span>
              </div>
              <span class="px-2 py-0.5 bg-blue-50 text-blue-700 font-bold rounded-md text-[11px]">
                {l.hours}h
              </span>
            </div>
          {/each}
        </div>
      {/if}
    </div>
  {/if}
</div>

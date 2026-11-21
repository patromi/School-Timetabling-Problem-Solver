<script lang="ts">
  import type { TimetableCellEntry, TimetableDay } from "../../types";
  import { getSubjectColor } from "./timetableHelpers";
  import Icon from "../common/Icon.svelte";

  interface Props {
    days: TimetableDay[];
    maxPeriodsCount: number;
    activeGrid: Record<string, Record<number, TimetableCellEntry[]>>;
    selectedViolationConstraint: string | null;
    violationMap: Record<string, string[]>;
  }

  let {
    days,
    maxPeriodsCount,
    activeGrid,
    selectedViolationConstraint,
    violationMap,
  }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl shadow-xs overflow-hidden print:border-none print:shadow-none">
  <div class="overflow-x-auto">
    <table class="w-full border-collapse text-left">
      <thead>
        <tr class="bg-slate-50 border-b border-slate-200 text-xs text-slate-700 font-bold">
          <th class="p-3 w-28 text-center border-r border-slate-200/80">Godzina</th>
          {#each days as day}
            <th class="p-3 text-center border-r border-slate-200/80 last:border-r-0">
              <div class="font-bold text-slate-900">{day.name || day.id}</div>
              <div class="text-[10px] text-slate-400 font-normal">{day.periods.length} lekcji</div>
            </th>
          {/each}
        </tr>
      </thead>
      <tbody class="divide-y divide-slate-100">
        {#each Array(maxPeriodsCount) as _, periodIdx}
          <tr class="hover:bg-slate-50/40 transition-colors">
            <!-- Period number column -->
            <td class="p-2.5 bg-slate-50/60 border-r border-slate-200 text-center font-mono text-xs">
              <span class="font-bold text-slate-800">Lekcja {periodIdx + 1}</span>
              <div class="text-[10px] text-slate-400 mt-0.5">
                {8 + Math.floor(periodIdx * 50 / 60)}:{(periodIdx * 50 % 60).toString().padStart(2, '0')}
              </div>
            </td>

            <!-- Days columns -->
            {#each days as day}
              {@const cellEntries = activeGrid[day.id]?.[periodIdx] || []}
              <td class="p-2 border-r border-slate-200/80 last:border-r-0 align-top h-20 min-w-[170px]">
                {#if cellEntries.length === 0}
                  <!-- Empty slot -->
                  <div class="h-full flex items-center justify-center text-slate-300 text-xs select-none">
                    —
                  </div>
                {:else}
                  <!-- Lesson Cards -->
                  <div class="flex flex-col gap-1.5 h-full">
                    {#each cellEntries as entry}
                      {@const colors = getSubjectColor(entry.event_name)}
                      {@const isViolation = selectedViolationConstraint && violationMap[selectedViolationConstraint]?.includes(entry.event_ref)}
                      <div class="p-2 rounded-xl border text-xs flex flex-col justify-between transition-all shadow-2xs
                        {entry.is_clash
                          ? 'bg-rose-50 border-rose-300 text-rose-950 animate-pulse'
                          : isViolation
                          ? 'bg-amber-50 border-amber-400 ring-2 ring-amber-400/50 text-amber-950'
                          : `${colors.bg} ${colors.border} ${colors.text}`}">
                        
                        <!-- Subject & Duration -->
                        <div class="flex items-start justify-between gap-1">
                          <span class="font-bold leading-snug">
                            {entry.event_name}
                          </span>
                          {#if entry.duration > 1}
                            <span class="text-[9px] px-1.5 py-0.2 bg-white/80 rounded font-medium shrink-0 border border-current">
                              Blok {entry.duration}h
                            </span>
                          {/if}
                        </div>

                        <!-- Clash Warning -->
                        {#if entry.is_clash}
                          <div class="text-[10px] font-bold text-rose-700 flex items-center gap-1 mt-1">
                            <Icon name="warning" size="xs" class="text-rose-600" />
                            <span>Kolizja terminów!</span>
                          </div>
                        {/if}

                        <!-- Other resources (Teacher / Room / Class) -->
                        <div class="text-[11px] text-slate-600 mt-1 flex flex-wrap items-center gap-1">
                          {#each entry.other_resources as other}
                            <span class="bg-white/80 px-1.5 py-0.5 rounded text-[10px] font-medium border border-slate-200/60">
                              {other}
                            </span>
                          {/each}
                        </div>
                      </div>
                    {/each}
                  </div>
                {/if}
              </td>
            {/each}
          </tr>
        {/each}
      </tbody>
    </table>
  </div>
</div>

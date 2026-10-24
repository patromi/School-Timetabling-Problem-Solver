<script lang="ts">
  import type { TimetableScore } from "../../types";
  import { CONSTRAINT_FRIENDLY_MAP } from "../../school_helpers";

  interface Props {
    scores: TimetableScore[];
    selectedViolationConstraint: string | null;
    onToggleViolation: (constraintId: string) => void;
    onClearViolation: () => void;
  }

  let {
    scores,
    selectedViolationConstraint,
    onToggleViolation,
    onClearViolation,
  }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-4">
  <div class="flex items-center justify-between">
    <div>
      <h3 class="text-sm font-bold text-slate-900">
        Audyt spełnienia reguł szkolnych (Diagnostyka ograniczeń)
      </h3>
      <p class="text-xs text-slate-500 mt-0.5">
        Zweryfikowano {scores.length} reguł organizacyjnych dla tej szkoły:
      </p>
    </div>
    {#if selectedViolationConstraint}
      <button
        type="button"
        onclick={onClearViolation}
        class="text-xs font-semibold text-blue-600 hover:text-blue-700 cursor-pointer"
      >
        ✕ Wyłącz podświetlenie reguły
      </button>
    {/if}
  </div>

  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
    {#each scores as s}
      {@const friendly = CONSTRAINT_FRIENDLY_MAP[s.id] || { title: s.name || s.id, description: "Zasada organizacji planu." }}
      {@const hasViolations = s.cost > 0}
      <button
        type="button"
        onclick={() => {
          if (hasViolations) {
            onToggleViolation(s.id);
          }
        }}
        disabled={!hasViolations}
        class={`p-3.5 rounded-xl border text-left transition-all ${
          hasViolations
            ? selectedViolationConstraint === s.id
              ? 'bg-amber-50 border-amber-400 ring-2 ring-amber-400/40 cursor-pointer'
              : 'bg-slate-50/90 border-slate-200 hover:border-amber-300 cursor-pointer'
            : 'bg-slate-50/50 border-slate-100 cursor-default opacity-80'
        }`}
      >
        <div class="flex items-start justify-between gap-2">
          <span class="text-xs font-bold text-slate-900 leading-snug">
            {friendly.title}
          </span>
          <span class="text-[10px] font-semibold px-2 py-0.5 rounded-full shrink-0
            {hasViolations ? 'bg-amber-100 text-amber-800' : 'bg-emerald-100 text-emerald-800'}">
            {hasViolations ? `Kara: ${s.cost} pkt` : '✓ 100%'}
          </span>
        </div>
        <p class="text-[11px] text-slate-500 mt-1 leading-snug">
          {friendly.description}
        </p>
        {#if hasViolations}
          <div class="mt-2 text-[10px] font-semibold text-amber-800 flex items-center gap-1">
            <span>🔍 Kliknij, aby podświetlić w siatce</span>
          </div>
        {/if}
      </button>
    {/each}
  </div>
</div>

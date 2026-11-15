<script lang="ts">
  import type { ConstraintScore } from "../../types";
  import { getConstraintScoreFriendlyInfo } from "../../school_helpers";
  import Icon from "../common/Icon.svelte";

  interface Props {
    scores: ConstraintScore[];
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
        class="text-xs font-semibold text-blue-600 hover:text-blue-700 cursor-pointer inline-flex items-center gap-1"
      >
        <Icon name="close" size="xs" />
        <span>Wyłącz podświetlenie reguły</span>
      </button>
    {/if}
  </div>

  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
    {#each scores as s}
      {@const friendly = getConstraintScoreFriendlyInfo(s)}
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
              ? s.required
                ? 'bg-rose-50 border-rose-400 ring-2 ring-rose-400/40 cursor-pointer'
                : 'bg-amber-50 border-amber-400 ring-2 ring-amber-400/40 cursor-pointer'
              : s.required
                ? 'bg-rose-50/40 border-rose-200 hover:border-rose-300 cursor-pointer'
                : 'bg-slate-50/90 border-slate-200 hover:border-amber-300 cursor-pointer'
            : 'bg-slate-50/50 border-slate-100 cursor-default opacity-80'
        }`}
      >
        <div class="flex items-start justify-between gap-2">
          <span class="text-xs font-bold text-slate-900 leading-snug inline-flex items-center gap-1.5">
            <Icon
              name={friendly.category === 'uczniowie' ? 'school' : friendly.category === 'nauczyciele' ? 'person' : friendly.category === 'sale' ? 'meeting_room' : 'event'}
              size="xs"
              class="text-slate-400 shrink-0"
            />
            <span>{friendly.title}</span>
          </span>
          <span class={`text-[10px] font-semibold px-2 py-0.5 rounded-full shrink-0 ${
            hasViolations
              ? s.required
                ? 'bg-rose-100 text-rose-800'
                : 'bg-amber-100 text-amber-800'
              : 'bg-emerald-100 text-emerald-800 inline-flex items-center gap-0.5'
          }`}>
            {#if hasViolations}
              Kara: {s.cost} pkt
            {:else}
              <Icon name="check" size="xs" class="text-emerald-700" />
              <span>100%</span>
            {/if}
          </span>
        </div>
        <div class="mt-1 flex items-center gap-1.5">
          <span class={`text-[10px] font-semibold px-1.5 py-0.5 rounded border ${
            s.required
              ? 'bg-rose-50 text-rose-700 border-rose-200'
              : 'bg-slate-100 text-slate-600 border-slate-200'
          }`}>
            {s.required ? 'Wymóg bezwzględny' : `Preferencja (waga ${s.weight})`}
          </span>
        </div>
        <p class="text-[11px] text-slate-500 mt-1.5 leading-snug">
          {friendly.description}
        </p>
        {#if hasViolations}
          <div class={`mt-2 text-[10px] font-semibold flex items-center gap-1 ${
            s.required ? 'text-rose-800' : 'text-amber-800'
          }`}>
            <Icon name="search" size="xs" />
            <span>Kliknij, aby podświetlić w siatce</span>
          </div>
        {/if}
      </button>
    {/each}
  </div>
</div>

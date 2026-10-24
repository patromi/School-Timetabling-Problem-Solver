<script lang="ts">
  interface Props {
    isSoft: boolean;
    weight: number;
  }

  let { isSoft = $bindable(), weight = $bindable() }: Props = $props();
</script>

<div class="p-4 bg-indigo-50/50 rounded-xl border border-indigo-100 flex flex-col gap-3">
  <div class="flex items-center justify-between">
    <div>
      <span class="text-xs font-bold text-indigo-950">Charakter reguły preferencji</span>
      <p class="text-[11px] text-indigo-700/80 mt-0.5">
        Określ, czy solver musi bezwzględnie umieścić lekcję w tych godzinach, czy może naruszyć regułę w zamian za karę.
      </p>
    </div>
    <div class="flex items-center bg-white p-0.5 rounded-lg border border-indigo-200 text-xs">
      <button
        type="button"
        onclick={() => (isSoft = true)}
        class="px-3 py-1 rounded-md font-semibold transition-all cursor-pointer {isSoft ? 'bg-indigo-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
      >
        SOFT (Preferencja)
      </button>
      <button
        type="button"
        onclick={() => (isSoft = false)}
        class="px-3 py-1 rounded-md font-semibold transition-all cursor-pointer {!isSoft ? 'bg-rose-600 text-white shadow-xs' : 'text-slate-600 hover:text-slate-900'}"
      >
        HARD (Wymóg)
      </button>
    </div>
  </div>

  {#if isSoft}
    <div class="pt-3 border-t border-indigo-100/70 flex items-center justify-between gap-4">
      <span class="text-xs text-indigo-900 font-medium">
        Waga kary za umieszczenie poza preferowanymi godzinami:
      </span>
      <div class="flex items-center gap-2">
        <input
          type="range"
          min="1"
          max="20"
          bind:value={weight}
          class="w-32 accent-indigo-600 cursor-pointer"
        />
        <span class="text-xs font-bold text-indigo-950 w-6 text-center">{weight}</span>
      </div>
    </div>
  {/if}
</div>

<!-- frontend/src/lib/components/editor/studio/rules/WeightAdjuster.svelte -->
<script lang="ts">
  interface Props {
    label?: string;
    value: number;
    min?: number;
    max?: number;
  }

  let { label = "Waga kary:", value = $bindable(), min = 1, max = 50 }: Props = $props();
  let open = $state(false);
</script>

<div class="flex items-center justify-between">
  <button
    type="button"
    onclick={() => (open = !open)}
    class="text-[11px] text-blue-600 hover:text-blue-800 font-medium cursor-pointer"
  >
    {open ? "▾ Ukryj wagę liczbową" : "▸ Dostosuj wagę liczbową"}
  </button>
  {#if open}
    <div class="flex items-center gap-1.5">
      <span class="text-slate-500 text-[11px]">{label}</span>
      <input
        type="number"
        {min}
        {max}
        bind:value
        class="w-14 px-2 py-0.5 text-center bg-white border border-slate-200 rounded text-xs font-bold text-slate-800"
      />
    </div>
  {/if}
</div>


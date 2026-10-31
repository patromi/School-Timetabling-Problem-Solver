<script lang="ts">
  interface Props {
    preference: "any" | "morning" | "afternoon" | "custom";
    onSelect: (type: "any" | "morning" | "afternoon" | "custom") => void;
  }

  let { preference, onSelect }: Props = $props();
</script>

<div>
  <span
    class="block text-xs font-bold uppercase tracking-wider text-slate-500 mb-2"
  >
    porę zajęć
  </span>
  <div class="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
    <!-- Any -->
    <button
      type="button"
      onclick={() => onSelect("any")}
      class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference ===
      'any'
        ? 'bg-blue-50/80 border-blue-500 ring-2 ring-blue-500/20'
        : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
    >
      <div class="flex items-center justify-between">
        <span class="text-base">🕒</span>
        {#if preference === "any"}<span class="w-2 h-2 rounded-full bg-blue-600"
          ></span>{/if}
      </div>
      <div class="mt-2">
        <span class="text-xs font-bold text-slate-900 block">Dowolna pora</span>
        <span class="text-[10px] text-slate-500 leading-tight block mt-0.5"
          >Brak preferencji godzinowych</span
        >
      </div>
    </button>

    <!-- Morning -->
    <button
      type="button"
      onclick={() => onSelect("morning")}
      class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference ===
      'morning'
        ? 'bg-amber-50/80 border-amber-500 ring-2 ring-amber-500/20'
        : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
    >
      <div class="flex items-center justify-between">
        <span class="text-base">🌅</span>
        {#if preference === "morning"}<span
            class="w-2 h-2 rounded-full bg-amber-600"
          ></span>{/if}
      </div>
      <div class="mt-2">
        <span class="text-xs font-bold text-slate-900 block"
          >Poranne lekcje</span
        >
        <span class="text-[10px] text-slate-500 leading-tight block mt-0.5"
          >Lekcje 1–4 (np. 8:00–11:30)</span
        >
      </div>
    </button>

    <!-- Afternoon -->
    <button
      type="button"
      onclick={() => onSelect("afternoon")}
      class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference ===
      'afternoon'
        ? 'bg-indigo-50/80 border-indigo-500 ring-2 ring-indigo-500/20'
        : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
    >
      <div class="flex items-center justify-between">
        <span class="text-base">🌇</span>
        {#if preference === "afternoon"}<span
            class="w-2 h-2 rounded-full bg-indigo-600"
          ></span>{/if}
      </div>
      <div class="mt-2">
        <span class="text-xs font-bold text-slate-900 block">Popołudniowe</span>
        <span class="text-[10px] text-slate-500 leading-tight block mt-0.5"
          >Lekcje 5+ (od 11:45)</span
        >
      </div>
    </button>

    <!-- Custom -->
    <button
      type="button"
      onclick={() => onSelect("custom")}
      class="p-3 rounded-xl border text-left transition-all cursor-pointer flex flex-col justify-between {preference ===
      'custom'
        ? 'bg-emerald-50/80 border-emerald-500 ring-2 ring-emerald-500/20'
        : 'bg-slate-50 border-slate-200 hover:bg-slate-100'}"
    >
      <div class="flex items-center justify-between">
        <span class="text-base">🎯</span>
        {#if preference === "custom"}<span
            class="w-2 h-2 rounded-full bg-emerald-600"
          ></span>{/if}
      </div>
      <div class="mt-2">
        <span class="text-xs font-bold text-slate-900 block">Własna siatka</span
        >
        <span class="text-[10px] text-slate-500 leading-tight block mt-0.5"
          >Zaznacz konkretne sloty</span
        >
      </div>
    </button>
  </div>
</div>

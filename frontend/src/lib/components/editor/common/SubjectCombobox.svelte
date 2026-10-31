<!-- frontend/src/lib/components/editor/common/SubjectCombobox.svelte -->
<script lang="ts">
  import { onMount } from "svelte";

  interface Props {
    value?: string;
    subjects?: string[];
    placeholder?: string;
    id?: string;
    disabled?: boolean;
    inputClass?: string;
    onSelect?: (val: string) => void;
    onAddNew?: (val: string) => void;
  }

  let {
    value = $bindable(""),
    subjects = [],
    placeholder = "Wybierz lub wpisz przedmiot...",
    id,
    disabled = false,
    inputClass = "",
    onSelect,
    onAddNew,
  }: Props = $props();

  let isOpen = $state(false);
  let highlightedIndex = $state<number>(-1);
  let containerRef: HTMLDivElement | null = $state(null);
  let inputRef: HTMLInputElement | null = $state(null);

  // Filtrowane pozycje na podstawie wpisanego tekstu
  const query = $derived((value || "").trim().toLowerCase());

  const filteredSubjects = $derived(
    subjects.filter((s) => s.toLowerCase().includes(query))
  );

  const exactMatchExists = $derived(
    subjects.some((s) => s.toLowerCase() === query)
  );

  const canAddNew = $derived(
    query.length > 0 && !exactMatchExists
  );

  function selectSubject(name: string) {
    value = name;
    isOpen = false;
    highlightedIndex = -1;
    onSelect?.(name);
  }

  function createAndSelect(name: string) {
    const trimmed = name.trim();
    if (!trimmed) return;
    value = trimmed;
    isOpen = false;
    highlightedIndex = -1;
    onAddNew?.(trimmed);
    onSelect?.(trimmed);
  }

  function handleInputFocus() {
    if (!disabled) {
      isOpen = true;
    }
  }

  function handleInputChange() {
    isOpen = true;
    highlightedIndex = -1;
    // Wywołanie onSelect z aktualną wartością, by stan nadrzędny był zsynchronizowany
    if (value) {
      onSelect?.(value);
    }
  }

  function handleKeyDown(e: KeyboardEvent) {
    if (!isOpen) {
      if (e.key === "ArrowDown" || e.key === "ArrowUp") {
        e.preventDefault();
        isOpen = true;
        return;
      }
    }

    if (e.key === "Escape") {
      isOpen = false;
      highlightedIndex = -1;
      return;
    }

    const totalItems = filteredSubjects.length + (canAddNew ? 1 : 0);

    if (e.key === "ArrowDown") {
      e.preventDefault();
      highlightedIndex = (highlightedIndex + 1) % (totalItems || 1);
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      highlightedIndex = (highlightedIndex - 1 + totalItems) % (totalItems || 1);
    } else if (e.key === "Enter") {
      e.preventDefault();
      if (canAddNew && highlightedIndex === 0) {
        createAndSelect(value);
      } else {
        const itemIdx = canAddNew ? highlightedIndex - 1 : highlightedIndex;
        if (itemIdx >= 0 && itemIdx < filteredSubjects.length) {
          selectSubject(filteredSubjects[itemIdx]);
        } else if (value.trim()) {
          createAndSelect(value);
        }
      }
    }
  }

  function handleClickOutside(event: MouseEvent) {
    if (containerRef && !containerRef.contains(event.target as Node)) {
      isOpen = false;
      highlightedIndex = -1;
    }
  }

  onMount(() => {
    document.addEventListener("click", handleClickOutside);
    return () => {
      document.removeEventListener("click", handleClickOutside);
    };
  });
</script>

<div class="relative w-full" bind:this={containerRef}>
  <div class="relative flex items-center">
    <input
      {id}
      type="text"
      bind:this={inputRef}
      bind:value
      {placeholder}
      {disabled}
      autocomplete="off"
      onfocus={handleInputFocus}
      oninput={handleInputChange}
      onkeydown={handleKeyDown}
      class={`w-full py-1.5 pl-3 pr-8 text-xs bg-slate-50 border border-slate-200 rounded-lg text-slate-900 placeholder:text-slate-400 focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-colors ${inputClass}`}
    />

    <div class="absolute inset-y-0 right-0 pr-1.5 flex items-center gap-0.5">
      {#if value && !disabled}
        <button
          type="button"
          tabindex="-1"
          onclick={() => {
            value = "";
            onSelect?.("");
            inputRef?.focus();
          }}
          class="p-1 text-slate-400 hover:text-slate-600 rounded-md transition-colors cursor-pointer text-[10px]"
          title="Wyczyść"
        >
          ✕
        </button>
      {/if}
      <button
        type="button"
        tabindex="-1"
        {disabled}
        onclick={() => {
          if (!disabled) {
            isOpen = !isOpen;
            if (isOpen) inputRef?.focus();
          }
        }}
        class="p-1 text-slate-400 hover:text-slate-700 transition-colors cursor-pointer text-xs"
        title="Rozwiń listę przedmiotów"
      >
        <span class="inline-block transition-transform duration-150 {isOpen ? 'rotate-180' : ''}">▾</span>
      </button>
    </div>
  </div>

  {#if isOpen}
    <div
      class="absolute z-50 left-0 right-0 mt-1 bg-white border border-slate-200 rounded-xl shadow-xl max-h-60 overflow-y-auto py-1 animate-in fade-in zoom-in-95 duration-100"
      role="listbox"
    >
      <!-- Opcja dodania nowego przedmiotu, gdy wpisano nazwę, której nie ma w bazie -->
      {#if canAddNew}
        <button
          type="button"
          onclick={() => createAndSelect(value)}
          class={`w-full text-left px-3 py-2 text-xs flex items-center gap-2 cursor-pointer transition-colors border-b border-slate-100 bg-blue-50/70 hover:bg-blue-100 text-blue-900 font-semibold ${
            highlightedIndex === 0 ? 'bg-blue-100 text-blue-950 ring-1 ring-inset ring-blue-300' : ''
          }`}
          role="option"
          aria-selected={highlightedIndex === 0}
        >
          <span class="w-5 h-5 rounded-md bg-blue-600 text-white flex items-center justify-center text-[10px] font-bold shrink-0">
            +
          </span>
          <span class="truncate">
            Dodaj nowy przedmiot: <strong class="text-blue-950 font-bold">"{value.trim()}"</strong>
          </span>
        </button>
      {/if}

      <!-- Lista istniejących przedmiotów -->
      {#if filteredSubjects.length === 0 && !canAddNew}
        <div class="px-3 py-3 text-center text-xs text-slate-400">
          Brak przedmiotów pasujących do frazy. Wpisz nazwę, aby dodać.
        </div>
      {:else}
        {#each filteredSubjects as sub, i}
          {@const adjustedIdx = canAddNew ? i + 1 : i}
          {@const isSelected = sub.toLowerCase() === (value || "").trim().toLowerCase()}
          <button
            type="button"
            onclick={() => selectSubject(sub)}
            class={`w-full text-left px-3 py-1.5 text-xs flex items-center justify-between cursor-pointer transition-colors ${
              isSelected ? 'font-bold text-blue-700 bg-blue-50/50' : 'text-slate-800'
            } ${
              highlightedIndex === adjustedIdx ? 'bg-slate-100 text-slate-900' : 'hover:bg-slate-50'
            }`}
            role="option"
            aria-selected={isSelected}
          >
            <span class="truncate">{sub}</span>
            {#if isSelected}
              <span class="text-blue-600 text-xs">✓</span>
            {/if}
          </button>
        {/each}
      {/if}
    </div>
  {/if}
</div>


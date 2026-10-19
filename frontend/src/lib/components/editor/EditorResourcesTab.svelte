<script lang="ts">
  import type { GroupItem, ResourceItem, ResourceTypeItem } from "../../types";

  interface Props {
    resourceTypes: ResourceTypeItem[];
    resourceGroups: GroupItem[];
    resources: ResourceItem[];
    onChangeResourceTypes: (types: ResourceTypeItem[]) => void;
    onChangeResources: (resources: ResourceItem[]) => void;
  }

  let {
    resourceTypes,
    resourceGroups,
    resources,
    onChangeResourceTypes,
    onChangeResources,
  }: Props = $props();

  // New resource state
  let resId = $state("");
  let resName = $state("");
  let resType = $state("Teacher");
  let resGroup = $state("");

  $effect(() => {
    if (resourceTypes.length > 0 && !resourceTypes.some((rt) => rt.id === resType)) {
      resType = resourceTypes[0].id;
    }
  });

  // New type state
  let newTypeId = $state("");
  let newTypeName = $state("");

  // Filter state
  let filterType = $state("all");
  let searchQuery = $state("");

  // Quick preset counts
  let teachersCount = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("teach")).length
  );
  let classesCount = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("class")).length
  );
  let roomsCount = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("room")).length
  );

  let filteredResources = $derived.by(() => {
    return resources.filter((r) => {
      if (filterType !== "all" && r.resource_type_ref !== filterType) return false;
      if (
        searchQuery &&
        !r.id.toLowerCase().includes(searchQuery.toLowerCase()) &&
        !r.name.toLowerCase().includes(searchQuery.toLowerCase())
      ) {
        return false;
      }
      return true;
    });
  });

  function addResource() {
    if (!resId.trim()) return;
    const id = resId.trim();
    if (resources.some((r) => r.id === id)) {
      alert(`Zasób o ID ${id} już istnieje!`);
      return;
    }
    const name = resName.trim() || id;
    const groupRefs = resGroup.trim() ? [resGroup.trim()] : [];
    onChangeResources([
      ...resources,
      {
        id,
        name,
        resource_type_ref: resType,
        group_refs: groupRefs,
      },
    ]);
    resId = "";
    resName = "";
  }

  function removeResource(id: string) {
    onChangeResources(resources.filter((r) => r.id !== id));
  }

  function addResourceType() {
    if (!newTypeId.trim()) return;
    const id = newTypeId.trim();
    if (resourceTypes.some((rt) => rt.id === id)) return;
    const name = newTypeName.trim() || id;
    onChangeResourceTypes([...resourceTypes, { id, name }]);
    newTypeId = "";
    newTypeName = "";
    resType = id;
  }

  function addQuickSampleResources() {
    const samples: ResourceItem[] = [
      { id: "T_KOWALSKI", name: "Jan Kowalski (Matematyka)", resource_type_ref: "Teacher", group_refs: ["Sci"] },
      { id: "T_NOWAK", name: "Anna Nowak (Polski)", resource_type_ref: "Teacher", group_refs: ["Hum"] },
      { id: "T_WISNIEWSKI", name: "Marek Wiśniewski (Fizyka)", resource_type_ref: "Teacher", group_refs: ["Sci"] },
      { id: "T_ZIELINSKA", name: "Katarzyna Zielińska (Angielski)", resource_type_ref: "Teacher", group_refs: ["Lang"] },
      { id: "C_1A", name: "Klasa 1A (Mat-Fiz)", resource_type_ref: "Class", group_refs: ["G1"] },
      { id: "C_1B", name: "Klasa 1B (Human)", resource_type_ref: "Class", group_refs: ["G1"] },
      { id: "R_101", name: "Sala 101 (Ogólna)", resource_type_ref: "Room", group_refs: [] },
      { id: "R_102", name: "Sala 102 (Fizyczna)", resource_type_ref: "Room", group_refs: [] },
      { id: "R_201", name: "Sala 201 (Językowa)", resource_type_ref: "Room", group_refs: [] },
    ];
    // Keep existing types ensured
    const typesToEnsure = ["Teacher", "Class", "Room"];
    let updatedTypes = [...resourceTypes];
    for (const t of typesToEnsure) {
      if (!updatedTypes.some((rt) => rt.id === t)) {
        updatedTypes.push({ id: t, name: t });
      }
    }
    onChangeResourceTypes(updatedTypes);

    const existingIds = new Set(resources.map((r) => r.id));
    const toAdd = samples.filter((s) => !existingIds.has(s.id));
    onChangeResources([...resources, ...toAdd]);
  }
</script>

<div class="flex flex-col gap-6 font-mono text-xs text-zinc-200">
  <!-- Top Stats & Presets -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div class="flex items-center gap-4">
      <div class="flex items-center gap-2">
        <span class="text-zinc-400">Podsumowanie:</span>
        <span class="px-2 py-0.5 bg-indigo-950 text-indigo-300 border border-indigo-800 rounded">
          Nauczyciele: <strong>{teachersCount}</strong>
        </span>
        <span class="px-2 py-0.5 bg-emerald-950 text-emerald-300 border border-emerald-800 rounded">
          Klasy: <strong>{classesCount}</strong>
        </span>
        <span class="px-2 py-0.5 bg-amber-950 text-amber-300 border border-amber-800 rounded">
          Sale: <strong>{roomsCount}</strong>
        </span>
        <span class="px-2 py-0.5 bg-zinc-800 text-zinc-300 border border-zinc-700 rounded">
          Łącznie: <strong>{resources.length}</strong>
        </span>
      </div>
    </div>

    <button
      type="button"
      onclick={addQuickSampleResources}
      class="px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-amber-400 font-bold rounded border border-zinc-700 transition-colors cursor-pointer"
      title="Wstawia przykładowych nauczycieli, klasy i sale"
    >
      ⚡ Wstaw przykładowe zasoby szkolne
    </button>
  </div>

  <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
    <!-- Left: Resource Add Form & Types -->
    <div class="flex flex-col gap-4">
      <!-- Add Resource Form -->
      <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
        <h4 class="font-bold text-white text-sm border-b border-zinc-800 pb-2">
          + Nowy Zasób
        </h4>

        <div class="flex flex-col gap-2">
          <div>
            <label for="res-id" class="text-[11px] text-zinc-400">Identyfikator (ID):</label>
            <input
              id="res-id"
              type="text"
              placeholder="np. T_KOWALSKI, C_1A, R_101"
              bind:value={resId}
              class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500 font-bold"
            />
          </div>

          <div>
            <label for="res-name" class="text-[11px] text-zinc-400">Pełna nazwa / Imię i nazwisko:</label>
            <input
              id="res-name"
              type="text"
              placeholder="np. Jan Kowalski (Matematyka)"
              bind:value={resName}
              class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none focus:border-zinc-500 font-sans"
            />
          </div>

          <div class="grid grid-cols-2 gap-2">
            <div>
              <label for="res-type" class="text-[11px] text-zinc-400">Typ zasobu:</label>
              <select
                id="res-type"
                bind:value={resType}
                class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
              >
                {#each resourceTypes as rt}
                  <option value={rt.id}>{rt.name || rt.id}</option>
                {/each}
              </select>
            </div>

            <div>
              <label for="res-grp" class="text-[11px] text-zinc-400">Grupa (opcjonalnie):</label>
              <input
                id="res-grp"
                type="text"
                placeholder="np. Sci, Klasy_1"
                bind:value={resGroup}
                class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
              />
            </div>
          </div>

          <button
            type="button"
            onclick={addResource}
            class="mt-2 w-full py-2 bg-amber-500 hover:bg-amber-400 text-zinc-950 font-bold rounded transition-colors cursor-pointer"
          >
            + Dodaj zasób
          </button>
        </div>
      </div>

      <!-- Resource Types Configuration -->
      <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
        <h4 class="font-bold text-white text-xs border-b border-zinc-800 pb-2">
          Kategorie zasobów (ResourceTypes)
        </h4>

        <div class="flex flex-wrap gap-1.5">
          {#each resourceTypes as rt}
            <span class="px-2 py-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-300 text-[11px]">
              {rt.name || rt.id}
            </span>
          {/each}
        </div>

        <div class="flex items-center gap-1.5 mt-1">
          <input
            type="text"
            placeholder="Nowy typ (np. Sprzęt)"
            bind:value={newTypeId}
            class="flex-1 px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
          />
          <button
            type="button"
            onclick={addResourceType}
            class="px-2.5 py-1 bg-zinc-800 hover:bg-zinc-700 text-amber-400 rounded border border-zinc-700 cursor-pointer font-bold"
          >
            +
          </button>
        </div>
      </div>
    </div>

    <!-- Right: Resources Table (2 cols) -->
    <div class="lg:col-span-2 bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 border-b border-zinc-800 pb-3">
        <h4 class="font-bold text-white">
          Zdefiniowane Zasoby ({filteredResources.length} / {resources.length})
        </h4>

        <div class="flex items-center gap-2 w-full sm:w-auto">
          <!-- Type Filter -->
          <select
            bind:value={filterType}
            class="px-2 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
          >
            <option value="all">Wszystkie typy</option>
            {#each resourceTypes as rt}
              <option value={rt.id}>{rt.name || rt.id}</option>
            {/each}
          </select>

          <!-- Search Filter -->
          <input
            type="text"
            placeholder="Filtruj zasoby..."
            bind:value={searchQuery}
            class="px-2.5 py-1 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
          />
        </div>
      </div>

      <!-- Table -->
      <div class="max-h-96 overflow-y-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-1 w-12">#</th>
              <th class="pb-1">ID</th>
              <th class="pb-1">Nazwa</th>
              <th class="pb-1">Kategoria</th>
              <th class="pb-1">Grupy</th>
              <th class="pb-1 text-right w-12">Usuń</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if resources.length === 0}
              <tr>
                <td colspan="6" class="py-4 text-center text-zinc-500 italic">
                  Brak zasobów. Dodaj nauczycieli, klasy i sale powyższym formularzem lub wstaw szablon.
                </td>
              </tr>
            {:else if filteredResources.length === 0}
              <tr>
                <td colspan="6" class="py-4 text-center text-zinc-500 italic">
                  Brak zasobów spełniających kryteria wyszukiwania.
                </td>
              </tr>
            {:else}
              {#each filteredResources as r, idx}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2 text-zinc-500">{idx + 1}</td>
                  <td class="py-2 text-amber-400 font-bold">{r.id}</td>
                  <td class="py-2 text-zinc-200 font-sans font-medium">{r.name || "—"}</td>
                  <td class="py-2">
                    <span class="px-1.5 py-0.5 rounded text-[10px] bg-zinc-950 border border-zinc-800 text-zinc-300">
                      {r.resource_type_ref}
                    </span>
                  </td>
                  <td class="py-2">
                    {#if r.group_refs.length > 0}
                      <span class="text-zinc-400">{r.group_refs.join(", ")}</span>
                    {:else}
                      <span class="text-zinc-600">—</span>
                    {/if}
                  </td>
                  <td class="py-2 text-right">
                    <button
                      type="button"
                      onclick={() => removeResource(r.id)}
                      class="text-zinc-500 hover:text-rose-400 cursor-pointer px-1 text-sm"
                      title="Usuń zasób"
                    >
                      ✕
                    </button>
                  </td>
                </tr>
              {/each}
            {/if}
          </tbody>
        </table>
      </div>
    </div>
  </div>
</div>

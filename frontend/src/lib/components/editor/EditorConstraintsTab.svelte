<script lang="ts">
  import type { AppliesToItem, ConstraintItem, EventItem, GroupItem, ResourceItem, TimeItem } from "../../types";

  interface Props {
    constraints: ConstraintItem[];
    resources: ResourceItem[];
    events: EventItem[];
    times: TimeItem[];
    timeGroups: GroupItem[];
    onChangeConstraints: (constraints: ConstraintItem[]) => void;
  }

  let {
    constraints,
    resources,
    events,
    times,
    timeGroups,
    onChangeConstraints,
  }: Props = $props();

  const CONSTRAINT_CATALOG = [
    { type: "AvoidClashesConstraint", name: "Brak nakładania się lekcji (AvoidClashes)", desc: "Zasób nie może być w dwóch miejscach jednocześnie" },
    { type: "AssignTimeConstraint", name: "Wymóg przypisania czasu (AssignTime)", desc: "Lekcja musi mieć przydzielony slot czasowy" },
    { type: "AssignResourceConstraint", name: "Wymóg przypisania zasobu (AssignResource)", desc: "Lekcja musi mieć przydzielonego nauczyciela / salę" },
    { type: "LimitIdleTimesConstraint", name: "Ograniczenie okienek (LimitIdleTimes)", desc: "Brak lub limit dziur w planie w ciągu dnia" },
    { type: "LimitBusyTimesConstraint", name: "Limit lekcji dziennie (LimitBusyTimes)", desc: "Minimalna i maksymalna liczba lekcji w danym dniu" },
    { type: "AvoidUnavailableTimesConstraint", name: "Niedostępność zasobów (AvoidUnavailableTimes)", desc: "Blokada godzin urlopowych, rad pedagogicznych itp." },
    { type: "ClusterBusyTimesConstraint", name: "Zwartość planu / dni wolne (ClusterBusyTimes)", desc: "Maksymalna liczba dni obecności w tygodniu" },
    { type: "SpreadEventsConstraint", name: "Równomierny rozkład lekcji (SpreadEvents)", desc: "Np. maksymalnie 1 lekcja przedmiotu dziennie" },
    { type: "DistributeSplitEventsConstraint", name: "Rozkład podzielonych lekcji (DistributeSplitEvents)", desc: "Dzielenie kursu na różne dni" },
    { type: "PreferTimesConstraint", name: "Preferencje godzinowe (PreferTimes)", desc: "Unikanie trudnych lekcji na ostatnich godzinach" },
    { type: "PreferResourcesConstraint", name: "Preferencje sal/nauczycieli (PreferResources)", desc: "Preferowane sale dla konkretnych przedmiotów" },
    { type: "LinkEventsConstraint", name: "Lekcje w tym samym czasie (LinkEvents)", desc: "Równoległe lektoraty lub laboratoria" },
    { type: "OrderEventsConstraint", name: "Kolejność lekcji (OrderEvents)", desc: "Zdarzenie A musi odbyć się przed zdarzeniem B" },
    { type: "SplitEventsConstraint", name: "Zasady podziału zajęć (SplitEvents)", desc: "Maksymalna i minimalna długość części lekcji" },
    { type: "AvoidSplitAssignmentsConstraint", name: "Jeden nauczyciel na kurs (AvoidSplitAssignments)", desc: "Zakaz zmiany nauczyciela między częściami" },
    { type: "LimitWorkloadConstraint", name: "Limit obciążenia (LimitWorkload)", desc: "Maksymalna liczba godzin pracy w tygodniu" },
  ];

  // Form states
  let cId = $state("");
  let cType = $state("AvoidClashesConstraint");
  let cRequired = $state(true);
  let cWeight = $state(1);
  let cCostFunc = $state("Linear");
  let cTargetScope = $state<"all_teachers" | "all_classes" | "all_rooms" | "all_events" | "custom">("all_teachers");

  // Custom params
  let paramMin = $state<number | undefined>(undefined);
  let paramMax = $state<number | undefined>(undefined);
  let selectedUnavailableTime = $state("");

  let teachers = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("teach"))
  );
  let classes = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("class"))
  );
  let rooms = $derived(
    resources.filter((r) => r.resource_type_ref.toLowerCase().includes("room"))
  );

  function addConstraint() {
    const id = cId.trim() || `${cType}_${Date.now().toString().slice(-4)}`;
    if (constraints.some((c) => c.id === id)) {
      alert(`Ograniczenie o ID ${id} już istnieje!`);
      return;
    }

    const appliesTo: AppliesToItem = {
      event_groups: [],
      events: [],
      resource_groups: [],
      resources: [],
      time_groups: [],
      times: [],
      event_pairs: [],
    };

    if (cTargetScope === "all_teachers") {
      appliesTo.resources = teachers.map((t) => t.id);
    } else if (cTargetScope === "all_classes") {
      appliesTo.resources = classes.map((c) => c.id);
    } else if (cTargetScope === "all_rooms") {
      appliesTo.resources = rooms.map((r) => r.id);
    } else if (cTargetScope === "all_events") {
      appliesTo.events = events.map((e) => e.id);
    }

    const params: Record<string, any> = {};

    if (cType === "LimitBusyTimesConstraint") {
      if (paramMin !== undefined) params["Minimum"] = paramMin;
      if (paramMax !== undefined) params["Maximum"] = paramMax;
      params["TimeGroups"] = timeGroups.map((g) => ({ reference: g.id }));
    } else if (cType === "LimitIdleTimesConstraint") {
      params["Minimum"] = 0;
      params["Maximum"] = paramMax !== undefined ? paramMax : 0;
      params["TimeGroups"] = timeGroups.map((g) => ({ reference: g.id }));
    } else if (cType === "ClusterBusyTimesConstraint") {
      params["Minimum"] = paramMin !== undefined ? paramMin : 1;
      params["Maximum"] = paramMax !== undefined ? paramMax : 5;
      params["TimeGroups"] = timeGroups.map((g) => ({ reference: g.id }));
    } else if (cType === "AvoidUnavailableTimesConstraint" && selectedUnavailableTime) {
      appliesTo.times = [selectedUnavailableTime];
    }

    const newConstraint: ConstraintItem = {
      id,
      name: `${cType} (${cTargetScope})`,
      type: cType,
      required: cRequired,
      weight: Number(cWeight) || 1,
      cost_function: cCostFunc,
      applies_to: appliesTo,
      params,
    };

    onChangeConstraints([...constraints, newConstraint]);

    cId = "";
  }

  function removeConstraint(id: string) {
    onChangeConstraints(constraints.filter((c) => c.id !== id));
  }

  function addStandardSchoolPreset() {
    const defaultList: ConstraintItem[] = [];

    // 1. Avoid Clashes for Teachers (Hard)
    if (teachers.length > 0) {
      defaultList.push({
        id: "Hard_Clashes_Teachers",
        name: "Brak konfliktów nauczycieli",
        type: "AvoidClashesConstraint",
        required: true,
        weight: 1,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: teachers.map((t) => t.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {},
      });
    }

    // 2. Avoid Clashes for Classes (Hard)
    if (classes.length > 0) {
      defaultList.push({
        id: "Hard_Clashes_Classes",
        name: "Brak konfliktów klas",
        type: "AvoidClashesConstraint",
        required: true,
        weight: 1,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: classes.map((c) => c.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {},
      });
    }

    // 3. Avoid Clashes for Rooms (Hard)
    if (rooms.length > 0) {
      defaultList.push({
        id: "Hard_Clashes_Rooms",
        name: "Brak konfliktów sal",
        type: "AvoidClashesConstraint",
        required: true,
        weight: 1,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: rooms.map((r) => r.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {},
      });
    }

    // 4. Assign Time to all Events (Hard)
    if (events.length > 0) {
      defaultList.push({
        id: "Hard_Assign_Time",
        name: "Wymóg alokacji czasu lekcji",
        type: "AssignTimeConstraint",
        required: true,
        weight: 1,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: events.map((e) => e.id),
          resource_groups: [],
          resources: [],
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {},
      });
    }

    // 5. Limit Idle Times for Classes (Soft, max 0 okienek)
    if (classes.length > 0) {
      defaultList.push({
        id: "Soft_Idle_Classes",
        name: "Brak okienek dla uczniów",
        type: "LimitIdleTimesConstraint",
        required: false,
        weight: 5,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: classes.map((c) => c.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Minimum: 0,
          Maximum: 0,
          TimeGroups: timeGroups.map((g) => ({ reference: g.id })),
        },
      });
    }

    // 6. Limit Busy Times for Classes (Soft, max 6-7 lekcji/dzień)
    if (classes.length > 0) {
      defaultList.push({
        id: "Soft_Busy_Classes",
        name: "Limit godzin w dniu dla uczniów",
        type: "LimitBusyTimesConstraint",
        required: false,
        weight: 2,
        cost_function: "Linear",
        applies_to: {
          event_groups: [],
          events: [],
          resource_groups: [],
          resources: classes.map((c) => c.id),
          time_groups: [],
          times: [],
          event_pairs: [],
        },
        params: {
          Minimum: 3,
          Maximum: 7,
          TimeGroups: timeGroups.map((g) => ({ reference: g.id })),
        },
      });
    }

    // Append non-duplicate constraints
    const existingIds = new Set(constraints.map((c) => c.id));
    const toAdd = defaultList.filter((c) => !existingIds.has(c.id));
    onChangeConstraints([...constraints, ...toAdd]);
  }
</script>

<div class="flex flex-col gap-6 font-mono text-xs text-zinc-200">
  <!-- Top Banner & Preset -->
  <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
    <div class="flex items-center gap-3">
      <span class="text-zinc-400">Podsumowanie reguł:</span>
      <span class="px-2 py-0.5 bg-rose-950 text-rose-300 border border-rose-800 rounded font-bold">
        Twarde (Hard): {constraints.filter((c) => c.required).length}
      </span>
      <span class="px-2 py-0.5 bg-amber-950 text-amber-300 border border-amber-800 rounded font-bold">
        Miękkie (Soft): {constraints.filter((c) => !c.required).length}
      </span>
      <span class="px-2 py-0.5 bg-zinc-800 text-zinc-300 border border-zinc-700 rounded">
        Łącznie: {constraints.length}
      </span>
    </div>

    <button
      type="button"
      onclick={addStandardSchoolPreset}
      class="px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-amber-400 font-bold rounded border border-zinc-700 transition-colors cursor-pointer"
      title="Dodaje standardowe reguły unikania kolizji, braku okienek i limitów godzin"
    >
      ⚡ Wstaw standardowy zestaw reguł szkolnych
    </button>
  </div>

  <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
    <!-- Form Column -->
    <div class="bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <h4 class="font-bold text-white text-sm border-b border-zinc-800 pb-2">
        + Nowa Reguła / Ograniczenie
      </h4>

      <div class="flex flex-col gap-2.5">
        <div>
          <label for="c-type" class="text-[11px] text-zinc-400">Typ ograniczenia (16 XHSTT):</label>
          <select
            id="c-type"
            bind:value={cType}
            class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-amber-300 font-bold focus:outline-none"
          >
            {#each CONSTRAINT_CATALOG as cat}
              <option value={cat.type}>{cat.name}</option>
            {/each}
          </select>
          <p class="text-[10px] text-zinc-500 mt-0.5 font-sans">
            {CONSTRAINT_CATALOG.find((cat) => cat.type === cType)?.desc}
          </p>
        </div>

        <div>
          <label for="c-id" class="text-[11px] text-zinc-400">Identyfikator (ID opcjonalne):</label>
          <input
            id="c-id"
            type="text"
            placeholder="np. Regula_Nauczyciele_Kolizje"
            bind:value={cId}
            class="w-full px-2.5 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-600 focus:outline-none"
          />
        </div>

        <!-- Hard / Soft & Weight -->
        <div class="grid grid-cols-2 gap-2 bg-zinc-950 p-2.5 rounded border border-zinc-800">
          <div>
            <span class="text-[11px] text-zinc-400 block mb-1">Status reguły:</span>
            <label class="flex items-center gap-1.5 cursor-pointer">
              <input type="checkbox" bind:checked={cRequired} class="rounded accent-amber-500" />
              <span class={cRequired ? "text-rose-400 font-bold" : "text-amber-400 font-bold"}>
                {cRequired ? "HARD (Wymóg)" : "SOFT (Jakość)"}
              </span>
            </label>
          </div>

          <div>
            <label for="c-weight" class="text-[11px] text-zinc-400">Waga naruszenia:</label>
            <input
              id="c-weight"
              type="number"
              min="1"
              max="1000"
              bind:value={cWeight}
              class="w-full px-2 py-1 mt-0.5 bg-zinc-900 border border-zinc-800 rounded text-white text-center font-bold"
            />
          </div>
        </div>

        <!-- Scope -->
        <div>
          <label for="c-scope" class="text-[11px] text-zinc-400">Zakres obiektów (AppliesTo):</label>
          <select
            id="c-scope"
            bind:value={cTargetScope}
            class="w-full px-2 py-1.5 mt-0.5 bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none"
          >
            <option value="all_teachers">Wszyscy nauczyciele ({teachers.length})</option>
            <option value="all_classes">Wszystkie klasy ({classes.length})</option>
            <option value="all_rooms">Wszystkie sale ({rooms.length})</option>
            <option value="all_events">Wszystkie lekcje ({events.length})</option>
          </select>
        </div>

        <!-- Type-specific parameters inputs -->
        {#if cType === "LimitBusyTimesConstraint"}
          <div class="grid grid-cols-2 gap-2 bg-zinc-950 p-2 rounded border border-zinc-800">
            <div>
              <label for="param-min-busy" class="text-[10px] text-zinc-400">Min. lekcji/dzień:</label>
              <input
                id="param-min-busy"
                type="number"
                min="0"
                max="10"
                bind:value={paramMin}
                class="w-full px-2 py-1 bg-zinc-900 border border-zinc-800 rounded text-center"
              />
            </div>
            <div>
              <label for="param-max-busy" class="text-[10px] text-zinc-400">Maks. lekcji/dzień:</label>
              <input
                id="param-max-busy"
                type="number"
                min="1"
                max="12"
                bind:value={paramMax}
                class="w-full px-2 py-1 bg-zinc-900 border border-zinc-800 rounded text-center"
              />
            </div>
          </div>
        {:else if cType === "LimitIdleTimesConstraint"}
          <div class="bg-zinc-950 p-2 rounded border border-zinc-800">
            <label for="param-max-idle" class="text-[10px] text-zinc-400">Maks. dopuszczalnych okienek dziennie:</label>
            <input
              id="param-max-idle"
              type="number"
              min="0"
              max="5"
              bind:value={paramMax}
              placeholder="0 (brak okienek)"
              class="w-full px-2 py-1 mt-0.5 bg-zinc-900 border border-zinc-800 rounded text-center"
            />
          </div>
        {:else if cType === "AvoidUnavailableTimesConstraint"}
          <div class="bg-zinc-950 p-2 rounded border border-zinc-800">
            <label for="param-unavail-slot" class="text-[10px] text-zinc-400">Zablokowany slot czasowy:</label>
            <select
              id="param-unavail-slot"
              bind:value={selectedUnavailableTime}
              class="w-full px-2 py-1 mt-0.5 bg-zinc-900 border border-zinc-800 rounded text-zinc-200"
            >
              <option value="">(Wybierz slot niedostępności)</option>
              {#each times as t}
                <option value={t.id}>{t.name || t.id}</option>
              {/each}
            </select>
          </div>
        {/if}

        <button
          type="button"
          onclick={addConstraint}
          class="mt-2 w-full py-2 bg-amber-500 hover:bg-amber-400 text-zinc-950 font-bold rounded transition-colors cursor-pointer"
        >
          + Dodaj ograniczenie
        </button>
      </div>
    </div>

    <!-- Right: Table of Constraints (2 cols) -->
    <div class="lg:col-span-2 bg-zinc-900 border border-zinc-800 rounded-lg p-4 flex flex-col gap-3">
      <div class="flex items-center justify-between border-b border-zinc-800 pb-3">
        <h4 class="font-bold text-white">
          Aktywne Ograniczenia w Instancji ({constraints.length})
        </h4>
      </div>

      <div class="max-h-96 overflow-y-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead class="text-zinc-500 border-b border-zinc-800 text-[11px]">
            <tr>
              <th class="pb-1 w-12">#</th>
              <th class="pb-1 w-20">Status</th>
              <th class="pb-1">Typ reguły</th>
              <th class="pb-1">ID</th>
              <th class="pb-1 w-12 text-right">Waga</th>
              <th class="pb-1">Zakres</th>
              <th class="pb-1 text-right w-12">Usuń</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-zinc-800/60">
            {#if constraints.length === 0}
              <tr>
                <td colspan="7" class="py-4 text-center text-zinc-500 italic">
                  Brak ograniczeń. Użyj przycisku wstawiania standardowego zestawu reguł lub dodaj reguły ręcznie.
                </td>
              </tr>
            {:else}
              {#each constraints as c, idx}
                <tr class="hover:bg-zinc-800/40">
                  <td class="py-2 text-zinc-500">{idx + 1}</td>
                  <td class="py-2">
                    {#if c.required}
                      <span class="px-1.5 py-0.2 bg-rose-950 text-rose-300 border border-rose-800 rounded text-[10px] font-bold">
                        HARD
                      </span>
                    {:else}
                      <span class="px-1.5 py-0.2 bg-amber-950 text-amber-300 border border-amber-800 rounded text-[10px] font-bold">
                        SOFT
                      </span>
                    {/if}
                  </td>
                  <td class="py-2 font-bold text-zinc-200">{c.type}</td>
                  <td class="py-2 text-amber-400">{c.id}</td>
                  <td class="py-2 text-right text-zinc-300 font-bold">{c.weight}</td>
                  <td class="py-2 text-zinc-400 text-[11px]">
                    {#if c.applies_to.resources.length > 0}
                      <span>{c.applies_to.resources.length} zasobów</span>
                    {:else if c.applies_to.events.length > 0}
                      <span>{c.applies_to.events.length} lekcji</span>
                    {:else if c.applies_to.times.length > 0}
                      <span>{c.applies_to.times.length} slotów</span>
                    {:else}
                      <span class="text-zinc-600">—</span>
                    {/if}
                  </td>
                  <td class="py-2 text-right">
                    <button
                      type="button"
                      onclick={() => removeConstraint(c.id)}
                      class="text-zinc-500 hover:text-rose-400 cursor-pointer px-1 text-sm"
                      title="Usuń regułę"
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

<!-- frontend/src/lib/components/editor/studio/InstanceStudio.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import type { InstanceDetail } from "../../../types";
  import { saveInstance } from "../../../api";
  import { EditorState } from "../editorState.svelte";
  import EditorModalsHost from "../EditorModalsHost.svelte";
  import TeacherStudioView from "./TeacherStudioView.svelte";
  import ClassStudioView from "./ClassStudioView.svelte";
  import RoomStudioView from "./RoomStudioView.svelte";
  import SchoolStructureView from "./SchoolStructureView.svelte";
  import GlobalRulesStudioView from "./GlobalRulesStudioView.svelte";
  import SummaryStudioView from "./SummaryStudioView.svelte";

  interface Props {
    initialInstance?: InstanceDetail | null;
    onSaved: (savedId: string) => void;
    onCancel?: () => void;
  }

  let { initialInstance = null, onSaved, onCancel }: Props = $props();

  const editor = new EditorState();

  let isSaving = $state(false);
  let saveError = $state<string | null>(null);
  let saveSuccess = $state<string | null>(null);

  // Quick addition fields in left sidebar
  let quickAddName = $state("");

  onMount(() => {
    if (typeof window !== "undefined") {
      const params = new URLSearchParams(window.location.search);
      const sec = params.get("section");
      if (sec && ["teachers", "classes", "rooms", "structure", "rules", "summary"].includes(sec)) {
        editor.studioSection = sec as any;
      }
      const tId = params.get("teacher");
      if (tId) editor.selectedStudioTeacherId = tId;
      const cId = params.get("class");
      if (cId) editor.selectedStudioClassId = cId;
      const rId = params.get("room");
      if (rId) editor.selectedStudioRoomId = rId;
    }
  });

  $effect(() => {
    if (initialInstance) {
      editor.loadFromExistingInstance(initialInstance);
    } else if (editor.lessons.length === 0) {
      editor.generateDefaultCurriculum();
    }
  });

  // Ensure an item is always selected when browsing lists
  $effect(() => {
    if (editor.studioSection === "teachers" && editor.teachers.length > 0 && !editor.selectedStudioTeacherId) {
      editor.selectedStudioTeacherId = editor.teachers[0].id;
    }
    if (editor.studioSection === "classes" && editor.classes.length > 0 && !editor.selectedStudioClassId) {
      editor.selectedStudioClassId = editor.classes[0].id;
    }
    if (editor.studioSection === "rooms" && editor.rooms.length > 0 && !editor.selectedStudioRoomId) {
      editor.selectedStudioRoomId = editor.rooms[0].id;
    }
  });

  let selectedTeacher = $derived(editor.teachers.find((t) => t.id === editor.selectedStudioTeacherId) || editor.teachers[0]);
  let selectedClass = $derived(editor.classes.find((c) => c.id === editor.selectedStudioClassId) || editor.classes[0]);
  let selectedRoom = $derived(editor.rooms.find((r) => r.id === editor.selectedStudioRoomId) || editor.rooms[0]);

  // Sidebar filtering
  let filteredTeachers = $derived(
    editor.teachers.filter((t) =>
      t.name.toLowerCase().includes(editor.studioSearchQuery.toLowerCase()) ||
      t.subject.toLowerCase().includes(editor.studioSearchQuery.toLowerCase())
    )
  );

  let filteredClasses = $derived(
    editor.classes.filter((c) =>
      c.name.toLowerCase().includes(editor.studioSearchQuery.toLowerCase())
    )
  );

  let filteredRooms = $derived(
    editor.rooms.filter((r) =>
      r.name.toLowerCase().includes(editor.studioSearchQuery.toLowerCase())
    )
  );

  function handleQuickAdd() {
    const val = quickAddName.trim();
    if (!val) return;
    if (editor.studioSection === "teachers") {
      editor.newTeacherName = val;
      editor.newTeacherSubject = "Matematyka";
      editor.newTeacherWorkload = 18;
      editor.addTeacher();
      const added = editor.teachers[editor.teachers.length - 1];
      if (added) editor.selectedStudioTeacherId = added.id;
    } else if (editor.studioSection === "classes") {
      editor.newClassName = val;
      editor.newClassGrade = 1;
      editor.addClass();
      const added = editor.classes[editor.classes.length - 1];
      if (added) editor.selectedStudioClassId = added.id;
    } else if (editor.studioSection === "rooms") {
      editor.newRoomName = val;
      editor.newRoomType = "general";
      editor.addRoom();
      const added = editor.rooms[editor.rooms.length - 1];
      if (added) editor.selectedStudioRoomId = added.id;
    }
    quickAddName = "";
  }

  async function handleSaveSchool() {
    if (!editor.isSchoolValid) {
      saveError = "Proszę uzupełnić nazwę szkoły oraz upewnić się, że dodano klasy, nauczycieli i lekcje.";
      return;
    }

    isSaving = true;
    saveError = null;
    saveSuccess = null;

    try {
      const payload = editor.buildXhsttPayload();
      await saveInstance(payload);
      saveSuccess = `Szkoła ${payload.name} została pomyślnie zapisana!`;
      setTimeout(() => {
        onSaved(payload.id);
      }, 700);
    } catch (err: any) {
      saveError = err.message || "Błąd podczas zapisywania szkoły w bazie.";
    } finally {
      isSaving = false;
    }
  }
</script>

<div class="h-full flex flex-col bg-slate-100/70 font-sans text-slate-800 antialiased overflow-hidden select-none">
  <!-- Top Application Bar (Clean, Flat, Professional) -->
  <header class="h-16 border-b border-slate-200 bg-white px-6 flex items-center justify-between shrink-0 shadow-2xs z-20">
    <div class="flex items-center gap-4">
      {#if onCancel}
        <button
          type="button"
          onclick={onCancel}
          class="px-3 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-xl transition-all cursor-pointer flex items-center gap-1.5"
        >
          <span>←</span>
          <span>Wróć do przeglądarki</span>
        </button>
      {/if}

      <div class="border-l border-slate-200 pl-4 flex items-center gap-3">
        <div>
          <h2 class="text-sm font-bold text-slate-900 leading-tight flex items-center gap-2">
            <span>{editor.schoolName || "Nowa szkoła"}</span>
            <span class="text-[10px] font-normal px-2 py-0.5 rounded-full bg-slate-100 text-slate-600 font-mono">
              Studio Zasobów
            </span>
          </h2>
          <p class="text-[11px] text-slate-500">
            Swobodne zarządzanie kadrą, oddziałami, salami i planem zajęć
          </p>
        </div>
      </div>
    </div>

    <!-- Center/Right Stats & Actions -->
    <div class="flex items-center gap-4">
      <div class="hidden md:flex items-center gap-3 text-xs text-slate-500 border-r border-slate-200 pr-4">
        <span>Godzin: <strong class="text-blue-700 font-bold">{editor.totalHoursAssigned}</strong></span>
        <span>•</span>
        <span>Kadra: <strong class="text-slate-800">{editor.teachers.length}</strong></span>
        <span>•</span>
        <span>Oddziały: <strong class="text-slate-800">{editor.classes.length}</strong></span>
        <span>•</span>
        <span>Sale: <strong class="text-slate-800">{editor.rooms.length}</strong></span>
      </div>

      {#if !editor.isSchoolValid}
        <button
          type="button"
          onclick={() => { editor.studioSection = "summary"; }}
          class="px-2.5 py-1.5 text-xs text-amber-800 bg-amber-50 hover:bg-amber-100 border border-amber-200 rounded-xl transition-all cursor-pointer flex items-center gap-1.5 font-medium"
          title="Kliknij, aby zobaczyć brakujące elementy"
        >
          <span>⚠️</span>
          <span>Do uzupełnienia ({editor.validationIssues.length})</span>
        </button>
      {/if}

      <button
        type="button"
        onclick={handleSaveSchool}
        disabled={isSaving || !editor.isSchoolValid}
        class={`px-4 py-2 text-xs font-bold rounded-xl transition-all shadow-xs flex items-center gap-1.5 cursor-pointer ${
          editor.isSchoolValid
            ? "bg-blue-600 hover:bg-blue-700 text-white active:scale-[0.99] shadow-blue-500/20"
            : "bg-slate-200 text-slate-400 cursor-not-allowed"
        }`}
      >
        {#if isSaving}
          <span class="animate-spin">⏳</span>
          <span>Zapisywanie...</span>
        {:else}
          <span>💾</span>
          <span>Zapisz szkołę</span>
        {/if}
      </button>
    </div>
  </header>

  <!-- Section Switcher Bar (Modern Navigation Rail) -->
  <nav class="bg-white border-b border-slate-200/90 px-6 py-2 flex items-center justify-between shrink-0 overflow-x-auto gap-2">
    <div class="flex items-center gap-1 text-xs">
      <!-- 1. Kadra -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "teachers"; }}
        class={`px-3.5 py-1.5 rounded-xl font-bold transition-all cursor-pointer flex items-center gap-2 ${
          editor.studioSection === "teachers"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>👨‍🏫</span>
        <span>Kadra pedagogiczna</span>
        <span class={`text-[10px] px-1.5 py-0.2 rounded-full font-semibold ${
          editor.studioSection === "teachers" ? "bg-slate-800 text-slate-200" : "bg-slate-100 text-slate-600"
        }`}>
          {editor.teachers.length}
        </span>
      </button>

      <!-- 2. Oddziały -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "classes"; }}
        class={`px-3.5 py-1.5 rounded-xl font-bold transition-all cursor-pointer flex items-center gap-2 ${
          editor.studioSection === "classes"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>👥</span>
        <span>Oddziały klasowe</span>
        <span class={`text-[10px] px-1.5 py-0.2 rounded-full font-semibold ${
          editor.studioSection === "classes" ? "bg-slate-800 text-slate-200" : "bg-slate-100 text-slate-600"
        }`}>
          {editor.classes.length}
        </span>
      </button>

      <!-- 3. Sale -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "rooms"; }}
        class={`px-3.5 py-1.5 rounded-xl font-bold transition-all cursor-pointer flex items-center gap-2 ${
          editor.studioSection === "rooms"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>🚪</span>
        <span>Sale lekcyjne</span>
        <span class={`text-[10px] px-1.5 py-0.2 rounded-full font-semibold ${
          editor.studioSection === "rooms" ? "bg-slate-800 text-slate-200" : "bg-slate-100 text-slate-600"
        }`}>
          {editor.rooms.length}
        </span>
      </button>

      <!-- Divider -->
      <div class="h-4 w-px bg-slate-200 mx-1"></div>

      <!-- 4. Struktura i tydzień -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "structure"; }}
        class={`px-3 py-1.5 rounded-xl font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          editor.studioSection === "structure"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>🏫</span>
        <span>Struktura i Tydzień</span>
      </button>

      <!-- 5. Zasady globalne -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "rules"; }}
        class={`px-3 py-1.5 rounded-xl font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          editor.studioSection === "rules"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>⚖️</span>
        <span>Zasady globalne</span>
      </button>

      <!-- 6. Podsumowanie -->
      <button
        type="button"
        onclick={() => { editor.studioSection = "summary"; }}
        class={`px-3 py-1.5 rounded-xl font-semibold transition-all cursor-pointer flex items-center gap-1.5 ${
          editor.studioSection === "summary"
            ? "bg-slate-900 text-white shadow-xs"
            : "text-slate-600 hover:text-slate-900 hover:bg-slate-100"
        }`}
      >
        <span>📋</span>
        <span>Podsumowanie i Zapis</span>
      </button>
    </div>
  </nav>

  <!-- Alerts -->
  {#if saveError}
    <div class="mx-6 mt-3 p-3 bg-rose-50 border border-rose-200 rounded-xl text-xs text-rose-800 flex items-center justify-between">
      <span>{saveError}</span>
      <button type="button" class="font-bold cursor-pointer" onclick={() => (saveError = null)}>✕</button>
    </div>
  {/if}
  {#if saveSuccess}
    <div class="mx-6 mt-3 p-3 bg-emerald-50 border border-emerald-200 rounded-xl text-xs text-emerald-800 flex items-center justify-between">
      <span>{saveSuccess}</span>
    </div>
  {/if}

  <!-- Main Content Layout (Desktop Full Width) -->
  <main class="flex-1 overflow-hidden flex flex-col md:flex-row">
    <!-- For standalone views: structure, rules, summary -->
    {#if editor.studioSection === "structure"}
      <div class="flex-1 overflow-y-auto p-6 max-w-5xl mx-auto w-full">
        <SchoolStructureView {editor} />
      </div>
    {:else if editor.studioSection === "rules"}
      <div class="flex-1 overflow-y-auto p-6 max-w-5xl mx-auto w-full">
        <GlobalRulesStudioView {editor} />
      </div>
    {:else if editor.studioSection === "summary"}
      <div class="flex-1 overflow-y-auto p-6 max-w-5xl mx-auto w-full">
        <SummaryStudioView {editor} {isSaving} onSave={handleSaveSchool} />
      </div>

    <!-- Master-Detail 2-Column Split: for teachers, classes, rooms -->
    {:else}
      <!-- Left Master Column (Resources list with search & quick add) -->
      <aside class="w-full md:w-80 lg:w-96 border-r border-slate-200/90 bg-white flex flex-col shrink-0 overflow-hidden shadow-2xs">
        <!-- Search bar -->
        <div class="p-3.5 border-b border-slate-100 bg-slate-50/50 flex flex-col gap-2.5">
          <div class="relative">
            <span class="absolute left-2.5 top-2 text-slate-400 text-xs">🔍</span>
            <input
              type="text"
              bind:value={editor.studioSearchQuery}
              placeholder={`Szukaj w sekcji ${editor.studioSection === 'teachers' ? 'kadry' : editor.studioSection === 'classes' ? 'oddziałów' : 'sal'}...`}
              class="w-full pl-8 pr-3 py-1.5 text-xs bg-white border border-slate-200 rounded-xl text-slate-800 focus:outline-none focus:border-blue-500 shadow-2xs"
            />
          </div>

          <!-- Quick Add in Master Sidebar -->
          <form
            onsubmit={(e) => { e.preventDefault(); handleQuickAdd(); }}
            class="flex items-center gap-1.5"
          >
            <input
              type="text"
              bind:value={quickAddName}
              placeholder={`+ Nowy ${editor.studioSection === 'teachers' ? 'nauczyciel' : editor.studioSection === 'classes' ? 'oddział (np. 1C)' : 'sala (np. s201)'}...`}
              class="flex-1 px-3 py-1.5 text-xs bg-white border border-slate-200 rounded-xl text-slate-800 focus:outline-none focus:border-blue-500"
            />
            <button
              type="submit"
              class="px-3 py-1.5 bg-slate-900 hover:bg-slate-800 text-white font-bold text-xs rounded-xl cursor-pointer transition-colors shadow-2xs shrink-0"
            >
              + Dodaj
            </button>
          </form>
        </div>

        <!-- Scrollable Resource List -->
        <div class="flex-1 overflow-y-auto p-2 divide-y divide-slate-100">
          {#if editor.studioSection === "teachers"}
            {#each filteredTeachers as teacher}
              {@const tAssigned = editor.getTeacherAssignedHours(teacher.id)}
              {@const tWorkload = teacher.workload || 18}
              {@const isSelected = editor.selectedStudioTeacherId === teacher.id}
              <button
                type="button"
                onclick={() => { editor.selectedStudioTeacherId = teacher.id; }}
                class={`w-full p-3 rounded-xl text-left transition-all flex items-center justify-between cursor-pointer group mb-1 ${
                  isSelected
                    ? "bg-slate-900 text-white shadow-xs"
                    : "hover:bg-slate-50 text-slate-800"
                }`}
              >
                <div class="flex items-center gap-2.5 overflow-hidden">
                  <span class="text-base shrink-0">👨‍🏫</span>
                  <div class="overflow-hidden">
                    <span class="font-bold text-xs truncate block">{teacher.name}</span>
                    <span class={`text-[10px] block truncate ${isSelected ? 'text-slate-300' : 'text-slate-400'}`}>
                      {teacher.subject}
                    </span>
                  </div>
                </div>

                <div class="text-right shrink-0">
                  <span class={`text-[11px] font-bold px-2 py-0.5 rounded-full ${
                    isSelected
                      ? 'bg-slate-800 text-slate-200'
                      : tAssigned === tWorkload
                        ? 'bg-emerald-50 text-emerald-800 border border-emerald-200'
                        : 'bg-slate-100 text-slate-700'
                  }`}>
                    {tAssigned}/{tWorkload}h
                  </span>
                </div>
              </button>
            {/each}

          {:else if editor.studioSection === "classes"}
            {#each filteredClasses as cls}
              {@const cHours = editor.getClassWeeklyHours(cls.id)}
              {@const isSelected = editor.selectedStudioClassId === cls.id}
              <button
                type="button"
                onclick={() => { editor.selectedStudioClassId = cls.id; }}
                class={`w-full p-3 rounded-xl text-left transition-all flex items-center justify-between cursor-pointer group mb-1 ${
                  isSelected
                    ? "bg-slate-900 text-white shadow-xs"
                    : "hover:bg-slate-50 text-slate-800"
                }`}
              >
                <div class="flex items-center gap-2.5 overflow-hidden">
                  <span class="text-base shrink-0">👥</span>
                  <div class="overflow-hidden">
                    <span class="font-bold text-xs truncate block">{cls.name}</span>
                    <span class={`text-[10px] block truncate ${isSelected ? 'text-slate-300' : 'text-slate-400'}`}>
                      Klasa {cls.grade || editor.getClassGrade(cls)}
                    </span>
                  </div>
                </div>

                <div class="text-right shrink-0">
                  <span class={`text-[11px] font-bold px-2 py-0.5 rounded-full ${
                    isSelected
                      ? 'bg-slate-800 text-slate-200'
                      : cHours === 0
                        ? 'bg-amber-50 text-amber-800 border border-amber-200'
                        : 'bg-blue-50 text-blue-800 border border-blue-200'
                  }`}>
                    {cHours}h
                  </span>
                </div>
              </button>
            {/each}

          {:else if editor.studioSection === "rooms"}
            {#each filteredRooms as room}
              {@const rHours = editor.getRoomAssignedHours(room.id)}
              {@const isSelected = editor.selectedStudioRoomId === room.id}
              <button
                type="button"
                onclick={() => { editor.selectedStudioRoomId = room.id; }}
                class={`w-full p-3 rounded-xl text-left transition-all flex items-center justify-between cursor-pointer group mb-1 ${
                  isSelected
                    ? "bg-slate-900 text-white shadow-xs"
                    : "hover:bg-slate-50 text-slate-800"
                }`}
              >
                <div class="flex items-center gap-2.5 overflow-hidden">
                  <span class="text-base shrink-0">🚪</span>
                  <div class="overflow-hidden">
                    <span class="font-bold text-xs truncate block">{room.name}</span>
                    <span class={`text-[10px] block truncate ${isSelected ? 'text-slate-300' : 'text-slate-400'}`}>
                      {room.type}
                    </span>
                  </div>
                </div>

                <div class="text-right shrink-0">
                  <span class={`text-[11px] font-bold px-2 py-0.5 rounded-full ${
                    isSelected ? 'bg-slate-800 text-slate-200' : 'bg-slate-100 text-slate-700'
                  }`}>
                    {rHours}h
                  </span>
                </div>
              </button>
            {/each}
          {/if}
        </div>
      </aside>

      <!-- Right Detail Column (Full width, 360° Object Studio View) -->
      <section class="flex-1 overflow-y-auto p-6 lg:p-8">
        <div class="max-w-6xl mx-auto w-full">
          {#if editor.studioSection === "teachers" && selectedTeacher}
            <TeacherStudioView
              {editor}
              teacher={selectedTeacher}
              onNavigateToClass={(classId) => editor.navigateToClass(classId)}
              onNavigateToRoom={(roomId) => editor.navigateToRoom(roomId)}
            />
          {:else if editor.studioSection === "classes" && selectedClass}
            <ClassStudioView
              {editor}
              cls={selectedClass}
              onNavigateToTeacher={(teacherId) => editor.navigateToTeacher(teacherId)}
              onNavigateToRoom={(roomId) => editor.navigateToRoom(roomId)}
            />
          {:else if editor.studioSection === "rooms" && selectedRoom}
            <RoomStudioView
              {editor}
              room={selectedRoom}
              onNavigateToClass={(classId) => editor.navigateToClass(classId)}
              onNavigateToTeacher={(teacherId) => editor.navigateToTeacher(teacherId)}
            />
          {:else}
            <div class="p-12 text-center bg-white rounded-2xl border border-slate-200 text-slate-400 text-xs">
              Wybierz element z listy po lewej stronie, aby otworzyć jego pełny widok.
            </div>
          {/if}
        </div>
      </section>
    {/if}
  </main>

  <!-- Auxiliary Modals Host -->
  <EditorModalsHost {editor} />
</div>


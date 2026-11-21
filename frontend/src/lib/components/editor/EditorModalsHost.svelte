<script lang="ts">
  import type { EditorState } from "./editorState.svelte";
  import AvailabilityModal from "./AvailabilityModal.svelte";
  import LessonPreferencesModal from "./LessonPreferencesModal.svelte";
  import LinkedLessonsModal from "./LinkedLessonsModal.svelte";
  import DuplicateCurriculumModal from "./DuplicateCurriculumModal.svelte";
  import SubjectManagerModal from "./SubjectManagerModal.svelte";
  import EditEntityModal from "./EditEntityModal.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<!-- Modal Availability Grid (Phase 1 & 2) -->
{#if editor.activeAvailabilityTarget}
  <AvailabilityModal
    resourceId={editor.activeAvailabilityTarget.id}
    resourceName={editor.activeAvailabilityTarget.name}
    resourceType={editor.activeAvailabilityTarget.type}
    days={editor.currentDaysFormatted}
    periodsPerDay={editor.periodsPerDay}
    initialUnavailableTimes={editor.activeAvailabilityTarget.unavailableTimes}
    initialIsSoft={editor.activeAvailabilityTarget.isSoft}
    initialWeight={editor.activeAvailabilityTarget.weight}
    initialMaxDaysPerWeek={editor.activeAvailabilityTarget.maxDaysPerWeek}
    initialMinDaysPerWeek={editor.activeAvailabilityTarget.minDaysPerWeek}
    initialMaxDailyLessons={editor.activeAvailabilityTarget.maxDailyLessons}
    initialMaxGapsPerWeek={editor.activeAvailabilityTarget.maxGapsPerWeek}
    initialMinDailyLessons={editor.activeAvailabilityTarget.minDailyLessons}
    contextInfo={editor.activeAvailabilityTarget.contextInfo}
    onSave={(times, isSoft, weight, extra) => editor.handleSaveAvailability(times, isSoft, weight, extra)}
    onClose={() => (editor.activeAvailabilityTarget = null)}
  />
{/if}

<!-- Modal Preferencji Godzinowych dla Lekcji (Phase 3) -->
{#if editor.activeLessonPrefTarget}
  <LessonPreferencesModal
    lessonId={editor.activeLessonPrefTarget.lessonId}
    lessonSubject={editor.activeLessonPrefTarget.subject}
    className={editor.activeLessonPrefTarget.className}
    days={editor.currentDaysFormatted}
    periodsPerDay={editor.periodsPerDay}
    initialPreference={editor.activeLessonPrefTarget.preference}
    initialPreferredTimes={editor.activeLessonPrefTarget.preferredTimes}
    initialIsSoft={editor.activeLessonPrefTarget.isSoft}
    initialWeight={editor.activeLessonPrefTarget.weight}
    onSave={(pref, times, isSoft, weight) => editor.handleSaveLessonPref(pref, times, isSoft, weight)}
    onClose={() => (editor.activeLessonPrefTarget = null)}
  />
{/if}

<!-- Modal Lekcji Równoległych / Jednoczesnych (Phase 3) -->
{#if editor.isLinkedModalOpen}
  <LinkedLessonsModal
    lessons={editor.lessons}
    classes={editor.classes}
    teachers={editor.teachers}
    initialGroups={editor.linkedGroups}
    onSave={(updatedGroups) => {
      editor.linkedGroups = updatedGroups;
      editor.isLinkedModalOpen = false;
    }}
    onClose={() => (editor.isLinkedModalOpen = false)}
  />
{/if}

<!-- Modal Duplikowania Siatki Zajęć (Krok 2 & Krok 4) -->
{#if editor.duplicateModalTarget}
  {@const target = editor.duplicateModalTarget}
  <DuplicateCurriculumModal
    sourceClass={target}
    classes={editor.classes}
    sourceHours={editor.getClassWeeklyHours(target.id)}
    sourceLessonsCount={editor.getClassLessons(target.id).length}
    onDuplicate={(targetIds) => editor.duplicateCurriculum(target.id, targetIds)}
    onClose={() => (editor.duplicateModalTarget = null)}
  />
{/if}

<!-- Modal Menedżera Przedmiotów (Renaming & Merging) -->
{#if editor.isSubjectManagerOpen}
  <SubjectManagerModal
    editor={editor}
    onClose={() => (editor.isSubjectManagerOpen = false)}
  />
{/if}

<!-- Modal Edycji Encji (Klasa, Nauczyciel, Sala, Lekcja) -->
{#if editor.editEntityTarget}
  <EditEntityModal
    editor={editor}
    target={editor.editEntityTarget}
    onClose={() => (editor.editEntityTarget = null)}
  />
{/if}

<script lang="ts">
  import type { LinkedGroupItem } from "../types";

  interface Props {
    groups: LinkedGroupItem[];
    lessons: Array<{
      id: string;
      subject: string;
      classId: string;
      teacherId: string;
      weeklyCount: number;
    }>;
    classes: Array<{ id: string; name: string }>;
    teachers: Array<{ id: string; name: string }>;
    onRemoveGroup: (id: string) => void;
  }

  let { groups, lessons, classes, teachers, onRemoveGroup }: Props = $props();

  function getClassName(classId: string): string {
    const c = classes.find((x) => x.id === classId);
    return c ? c.name : classId;
  }

  function getTeacherName(teacherId: string): string {
    const t = teachers.find((x) => x.id === teacherId);
    return t ? t.name : teacherId;
  }
</script>

<div>
  <div class="flex items-center justify-between mb-3">
    <span class="text-xs font-bold uppercase tracking-wider text-slate-500">
      Zdefiniowane grupy lekcji równoległych ({groups.length})
    </span>
    <span class="text-[11px] text-slate-400">
      Wszystkie lekcje w grupie odbędą się w tym samym czasie
    </span>
  </div>

  {#if groups.length === 0}
    <div class="p-6 rounded-xl border border-dashed border-slate-200 bg-slate-50 text-center flex flex-col items-center justify-center gap-1.5">
      <span class="text-2xl">🔗</span>
      <span class="text-xs font-bold text-slate-700">Brak powiązanych lekcji</span>
      <p class="text-[11px] text-slate-400 max-w-md">
        Utwórz nową grupę poniżej, aby połączyć lekcje z różnych oddziałów (np. podział na grupy językowe z klas 1A i 1B w tym samym slocie).
      </p>
    </div>
  {:else}
    <div class="divide-y divide-slate-100 border border-slate-200 rounded-xl overflow-hidden">
      {#each groups as g}
        <div class="p-4 bg-white hover:bg-slate-50/70 transition-colors flex items-start justify-between gap-4">
          <div class="flex flex-col gap-1.5 flex-1">
            <div class="flex items-center gap-2">
              <span class="font-bold text-slate-900 text-xs">{g.name}</span>
              <span class="px-2 py-0.5 rounded-full text-[10px] font-bold {g.isSoft ? 'bg-indigo-50 text-indigo-700 border border-indigo-200' : 'bg-rose-50 text-rose-700 border border-rose-200'}">
                {g.isSoft ? `SOFT (waga ${g.weight || 10})` : 'HARD (Wymóg bezwzględny)'}
              </span>
              <span class="text-[11px] text-slate-400">
                • {g.lessonIds.length} powiązanych lekcji
              </span>
            </div>

            <!-- Member badges -->
            <div class="flex flex-wrap gap-1.5 mt-1">
              {#each g.lessonIds as lid}
                {@const l = lessons.find((x) => x.id === lid)}
                {#if l}
                  <span class="inline-flex items-center gap-1 px-2.5 py-1 bg-purple-50 border border-purple-200 text-purple-900 rounded-lg text-[11px] font-medium">
                    <span>📚</span>
                    <strong>{l.subject}</strong>
                    <span class="text-purple-600">({getClassName(l.classId)} - {getTeacherName(l.teacherId)})</span>
                  </span>
                {/if}
              {/each}
            </div>
          </div>

          <button
            type="button"
            onclick={() => onRemoveGroup(g.id)}
            class="text-slate-400 hover:text-rose-600 p-1 rounded-lg hover:bg-rose-50 transition-colors cursor-pointer text-xs"
            title="Usuń grupę"
          >
            Usuń
          </button>
        </div>
      {/each}
    </div>
  {/if}
</div>

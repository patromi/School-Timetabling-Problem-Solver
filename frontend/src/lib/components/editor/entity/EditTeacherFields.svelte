<script lang="ts">
  interface Props {
    name: string;
    subject: string;
    workload: number;
    id: string;
  }

  let { name = $bindable(), subject = $bindable(), workload = $bindable(), id = $bindable() }: Props = $props();
</script>

<div>
  <label for="edit-teacher-name" class="block font-bold text-slate-700 mb-1">Imię i nazwisko nauczyciela:</label>
  <input
    id="edit-teacher-name"
    type="text"
    bind:value={name}
    placeholder="np. Anna Nowak"
    class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
  />
</div>

<div class="grid grid-cols-2 gap-3">
  <div>
    <label for="edit-teacher-subject" class="block font-bold text-slate-700 mb-1">Główny przedmiot:</label>
    <input
      id="edit-teacher-subject"
      type="text"
      bind:value={subject}
      placeholder="np. Matematyka"
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    />
  </div>
  <div>
    <label for="edit-teacher-workload" class="block font-bold text-slate-700 mb-1">Pensum etatowe (h/tydz.):</label>
    <input
      id="edit-teacher-workload"
      type="number"
      min="1"
      max="40"
      bind:value={workload}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
    />
  </div>
</div>

<div>
  <label for="edit-teacher-id" class="block font-bold text-slate-700 mb-1">Identyfikator (ID):</label>
  <input
    id="edit-teacher-id"
    type="text"
    bind:value={id}
    placeholder="np. Nowak_A"
    class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono text-xs"
  />
</div>

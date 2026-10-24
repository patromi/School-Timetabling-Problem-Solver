<script lang="ts">
  interface Props {
    name: string;
    grade: number;
    id: string;
  }

  let { name = $bindable(), grade = $bindable(), id = $bindable() }: Props = $props();
</script>

<div>
  <label for="edit-class-name" class="block font-bold text-slate-700 mb-1">Nazwa oddziału:</label>
  <input
    id="edit-class-name"
    type="text"
    bind:value={name}
    placeholder="np. Klasa 1A (Mat-Fiz)"
    class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
  />
</div>

<div class="grid grid-cols-2 gap-3">
  <div>
    <label for="edit-class-grade" class="block font-bold text-slate-700 mb-1">Rocznik (klasa):</label>
    <input
      id="edit-class-grade"
      type="number"
      min="1"
      max="8"
      bind:value={grade}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-bold"
    />
  </div>
  <div>
    <label for="edit-class-id" class="block font-bold text-slate-700 mb-1">Kod ID:</label>
    <input
      id="edit-class-id"
      type="text"
      bind:value={id}
      placeholder="np. 1A"
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono"
    />
  </div>
</div>

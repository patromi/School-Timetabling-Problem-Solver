<script lang="ts">
  import type { RoomType } from "../types";

  interface Props {
    name: string;
    roomType: RoomType;
    id: string;
  }

  let { name = $bindable(), roomType = $bindable(), id = $bindable() }: Props = $props();
</script>

<div>
  <label for="edit-room-name" class="block font-bold text-slate-700 mb-1">Nazwa pomieszczenia:</label>
  <input
    id="edit-room-name"
    type="text"
    bind:value={name}
    placeholder="np. Sala 101 (Ogólna)"
    class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
  />
</div>

<div class="grid grid-cols-2 gap-3">
  <div>
    <label for="edit-room-type" class="block font-bold text-slate-700 mb-1">Przeznaczenie (typ):</label>
    <select
      id="edit-room-type"
      bind:value={roomType}
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900"
    >
      <option value="general">Ogólna (dydaktyczna)</option>
      <option value="gym">Gimnastyczna (WF)</option>
      <option value="lab">Laboratorium / Chem-Biol</option>
      <option value="it">Informatyczna (IT)</option>
    </select>
  </div>
  <div>
    <label for="edit-room-id" class="block font-bold text-slate-700 mb-1">Kod ID sali:</label>
    <input
      id="edit-room-id"
      type="text"
      bind:value={id}
      placeholder="np. s101"
      class="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-xl focus:bg-white focus:outline-none focus:ring-2 focus:ring-blue-500/20 text-slate-900 font-mono text-xs"
    />
  </div>
</div>

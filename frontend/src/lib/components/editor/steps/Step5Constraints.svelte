<!-- frontend/src/lib/components/editor/steps/Step5Constraints.svelte -->
<script lang="ts">
  import type { EditorState } from "../editorState.svelte";
  import IdleAndBusyTimesRules from "./constraints/IdleAndBusyTimesRules.svelte";
  import PedagogicalPreferencesRules from "./constraints/PedagogicalPreferencesRules.svelte";
  import RelationsAndOrderRules from "./constraints/RelationsAndOrderRules.svelte";

  interface Props {
    editor: EditorState;
  }

  let { editor }: Props = $props();
</script>

<div class="bg-white border border-slate-200/90 rounded-2xl p-6 shadow-xs flex flex-col gap-6">
  <div>
    <h3 class="text-base font-bold text-slate-900">Standardy dydaktyczne i zasady szkoły</h3>
    <p class="text-xs text-slate-500 mt-0.5">
      Zaznacz reguły, które algorytm optymalizacyjny powinien wymusić lub preferować podczas układania planu.
    </p>
  </div>

  <IdleAndBusyTimesRules {editor} />
  <PedagogicalPreferencesRules {editor} />
  <RelationsAndOrderRules {editor} />
</div>

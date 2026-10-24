// frontend/src/lib/components/editor/state/orderRulesActions.ts
import type { SimpleLesson, OrderRuleItem } from "../types";

export function createOrderRule(
  lessons: SimpleLesson[],
  firstLessonId: string,
  secondLessonId: string,
  minSeparation: number,
  isSoft: boolean,
  weight: number
): { rule?: OrderRuleItem; error?: string } {
  if (!firstLessonId || !secondLessonId || firstLessonId === secondLessonId) {
    return { error: "Wybierz dwie różne lekcje do określenia kolejności!" };
  }
  const l1 = lessons.find((l) => l.id === firstLessonId);
  const l2 = lessons.find((l) => l.id === secondLessonId);
  const ruleName = `${l1?.subject || "Lekcja 1"} przed ${l2?.subject || "Lekcja 2"}`;
  return {
    rule: {
      id: `order_${Date.now()}_${Math.random().toString(36).substring(2, 6)}`,
      name: ruleName,
      firstLessonId,
      secondLessonId,
      minSeparation: Number(minSeparation) || 0,
      isSoft,
      weight,
    },
  };
}

export function removeOrderRuleFromList(rules: OrderRuleItem[], ruleId: string): OrderRuleItem[] {
  return rules.filter((r) => r.id !== ruleId);
}

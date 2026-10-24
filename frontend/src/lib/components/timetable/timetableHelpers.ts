// frontend/src/lib/components/timetable/timetableHelpers.ts

export function getSubjectColor(subjectName: string): { bg: string; text: string; border: string } {
  const s = (subjectName || "").toLowerCase();
  if (s.includes("mat")) {
    return { bg: "bg-blue-50/90", text: "text-blue-900", border: "border-blue-200" };
  }
  if (s.includes("polsk") || s.includes("hist")) {
    return { bg: "bg-amber-50/90", text: "text-amber-950", border: "border-amber-200" };
  }
  if (s.includes("angiel") || s.includes("niemiec") || s.includes("język")) {
    return { bg: "bg-purple-50/90", text: "text-purple-950", border: "border-purple-200" };
  }
  if (s.includes("fizyk") || s.includes("chem") || s.includes("biol")) {
    return { bg: "bg-teal-50/90", text: "text-teal-950", border: "border-teal-200" };
  }
  if (s.includes("wf") || s.includes("fizycz")) {
    return { bg: "bg-emerald-50/90", text: "text-emerald-950", border: "border-emerald-200" };
  }
  if (s.includes("inform")) {
    return { bg: "bg-cyan-50/90", text: "text-cyan-950", border: "border-cyan-200" };
  }
  return { bg: "bg-slate-50", text: "text-slate-900", border: "border-slate-200" };
}

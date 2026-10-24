// frontend/src/lib/components/editor/state/initialEditorDefaults.ts
import type { SimpleClass, SimpleTeacher, SimpleRoom, BellTime } from "../types";

export function createDefaultBellTimes(): BellTime[] {
  return [
    { period: 1, start: "08:00", end: "08:45" },
    { period: 2, start: "08:55", end: "09:40" },
    { period: 3, start: "09:50", end: "10:35" },
    { period: 4, start: "10:50", end: "11:35" },
    { period: 5, start: "11:55", end: "12:40" },
    { period: 6, start: "12:55", end: "13:40" },
    { period: 7, start: "13:50", end: "14:35" },
    { period: 8, start: "14:45", end: "15:30" },
  ];
}

export function createDefaultClasses(): SimpleClass[] {
  return [
    { id: "1A", name: "Klasa 1A (Mat-Fiz)", grade: 1 },
    { id: "1B", name: "Klasa 1B (Biol-Chem)", grade: 1 },
    { id: "2A", name: "Klasa 2A (Mat-Fiz)", grade: 2 },
    { id: "2B", name: "Klasa 2B (Humanistyczna)", grade: 2 },
    { id: "3A", name: "Klasa 3A (Mat-Geo)", grade: 3 },
    { id: "3B", name: "Klasa 3B (Biol-Chem)", grade: 3 },
    { id: "4A", name: "Klasa 4A (Mat-Fiz)", grade: 4 },
    { id: "4B", name: "Klasa 4B (Językowa)", grade: 4 },
  ];
}

export function createDefaultTeachers(): SimpleTeacher[] {
  return [
    { id: "Kowalski_J", name: "Jan Kowalski", subject: "Matematyka", workload: 18 },
    { id: "Nowak_A", name: "Anna Nowak", subject: "Język polski", workload: 18 },
    { id: "Wisniewski_P", name: "Piotr Wiśniewski", subject: "Wychowanie Fizyczne", workload: 18 },
    { id: "Wojcik_M", name: "Marta Wójcik", subject: "Język angielski", workload: 16 },
    { id: "Kaminski_T", name: "Tomasz Kamiński", subject: "Historia", workload: 15, maxDaysPerWeek: 4 },
    { id: "Lewandowska_E", name: "Ewa Lewandowska", subject: "Biologia", workload: 14, maxDaysPerWeek: 3 },
    { id: "Zieliński_K", name: "Krzysztof Zieliński", subject: "Fizyka", workload: 14, maxDaysPerWeek: 3 },
    { id: "Szymanska_B", name: "Beata Szymańska", subject: "Chemia", workload: 14, maxDailyLessons: 5 },
    { id: "Dabrowski_R", name: "Robert Dąbrowski", subject: "Informatyka", workload: 14, maxGapsPerWeek: 1 },
    { id: "Kozlowska_M", name: "Magdalena Kozłowska", subject: "Geografia", workload: 14, maxGapsPerWeek: 1 },
  ];
}

export function createDefaultRooms(): SimpleRoom[] {
  return [
    { id: "s101", name: "Sala 101 (Ogólna)", type: "general" },
    { id: "s102", name: "Sala 102 (Ogólna)", type: "general" },
    { id: "s103", name: "Sala 103 (Ogólna)", type: "general" },
    { id: "s104", name: "Sala 104 (Ogólna)", type: "general" },
    { id: "s201", name: "Sala 201 (Ogólna)", type: "general" },
    { id: "s202", name: "Sala 202 (Ogólna)", type: "general" },
    { id: "sgym", name: "Sala gimnastyczna (WF)", type: "gym" },
    { id: "slab", name: "Pracownia chemiczno-biol.", type: "lab" },
    { id: "sit", name: "Pracownia informatyczna", type: "it" },
  ];
}

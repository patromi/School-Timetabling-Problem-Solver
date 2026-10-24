import type { ConstraintItem, EventItem, InstanceDetail, ResourceItem } from "./types";

/**
 * Rozpoznaje kategorię zasobu (nauczyciel, oddział/klasa, sala, inny)
 * na podstawie identyfikatora lub typu zasobu.
 */
export function getResourceCategory(
  resource: ResourceItem,
  instance: InstanceDetail
): "teacher" | "class" | "room" | "other" {
  const typeRef = (resource.resource_type_ref || "").toLowerCase();
  const id = resource.id.toLowerCase();
  const name = (resource.name || "").toLowerCase();

  // Sprawdzanie po typie zasobu
  if (typeRef.includes("teach") || typeRef.includes("nauczyciel") || typeRef.includes("prof")) {
    return "teacher";
  }
  if (typeRef.includes("class") || typeRef.includes("student") || typeRef.includes("oddz") || typeRef.includes("klas")) {
    return "class";
  }
  if (typeRef.includes("room") || typeRef.includes("sala") || typeRef.includes("pracown")) {
    return "room";
  }

  // Heurystyka po nazwie/ID jeśli typ jest generyczny
  if (id.startsWith("t_") || id.startsWith("te_") || name.startsWith("nauczyciel") || name.includes("mgr") || name.includes("dr")) {
    return "teacher";
  }
  if (id.startsWith("c_") || id.startsWith("cl_") || id.startsWith("k_") || /^[0-9]+[a-z]?$/i.test(id)) {
    return "class";
  }
  if (id.startsWith("r_") || id.startsWith("ro_") || id.startsWith("s_") || name.includes("sala") || name.includes("lab")) {
    return "room";
  }

  return "other";
}

/**
 * Oblicza sumaryczną liczbę godzin lekcyjnych dla każdego zasobu w tygodniu.
 */
export function computeResourceWorkloads(instance: InstanceDetail): Record<string, number> {
  const workloads: Record<string, number> = {};
  for (const r of instance.resources) {
    workloads[r.id] = 0;
  }

  for (const e of instance.events) {
    const dur = e.duration || 1;
    for (const er of e.resources) {
      if (er.resource_ref && workloads[er.resource_ref] !== undefined) {
        workloads[er.resource_ref] += dur;
      }
    }
  }

  return workloads;
}

/**
 * Przyjazny dla człowieka opis ograniczeń XHSTT
 */
export interface ConstraintFriendlyInfo {
  title: string;
  category: "organizacja" | "uczniowie" | "nauczyciele" | "sale";
  description: string;
  isHardByDefault: boolean;
}

export const CONSTRAINT_FRIENDLY_MAP: Record<string, ConstraintFriendlyInfo> = {
  AssignTimeConstraint: {
    title: "Obsadzenie terminów w planie",
    category: "organizacja",
    description: "Każda lekcja z siatki programowej musi mieć wyznaczony konkretny dzień i godzinę w tygodniu.",
    isHardByDefault: true,
  },
  AssignResourceConstraint: {
    title: "Obsadzenie nauczycieli i sal",
    category: "organizacja",
    description: "Zajęcia muszą mieć przydzielonego konkretnego pedagoga i odpowiednią salę lekcyjną.",
    isHardByDefault: true,
  },
  AvoidClashesConstraint: {
    title: "Brak konfliktów terminów (nakładanie się)",
    category: "organizacja",
    description: "Żaden nauczyciel, klasa ani sala nie mogą mieć dwóch różnych lekcji w tym samym czasie.",
    isHardByDefault: true,
  },
  AvoidSplitWeeksConstraint: {
    title: "Ciągłość bloków lekcyjnych",
    category: "organizacja",
    description: "Zajęcia wielogodzinne (np. laboratorium, blok 2h) powinny odbywać się jako jedna całość.",
    isHardByDefault: false,
  },
  SpreadEventsConstraint: {
    title: "Równomierne rozłożenie w tygodniu",
    category: "organizacja",
    description: "Lekcje z jednego przedmiotu powinny być równomiernie podzielone na różne dni tygodnia.",
    isHardByDefault: false,
  },
  LimitIdleTimesConstraint: {
    title: "Ograniczenie okienek w planie (puste godziny)",
    category: "uczniowie",
    description: "Eliminacja lub minimalizacja przerw między zajęciami w ciągu dnia szkolnego.",
    isHardByDefault: true,
  },
  LimitBusyTimesConstraint: {
    title: "Limit liczby lekcji dziennie (higiena nauki)",
    category: "uczniowie",
    description: "Zapewnienie zrównoważonego obciążenia i nieprzekraczanie maksymalnej liczby lekcji na dzień.",
    isHardByDefault: false,
  },
  AvoidUnavailableTimesConstraint: {
    title: "Niedostępność w wybrane dni i godziny",
    category: "organizacja",
    description: "Uwzględnienie godzin wyłączonych z planu (np. zebrania rady, urlopy, brak dostępności sali).",
    isHardByDefault: true,
  },
  PreferTimesConstraint: {
    title: "Preferowane pory zajęć",
    category: "organizacja",
    description: "Umieszczanie trudniejszych przedmiotów w godzinach porannych, a luźniejszych na koniec dnia.",
    isHardByDefault: false,
  },
  PreferResourcesConstraint: {
    title: "Preferowany przydział sal/pracowni",
    category: "sale",
    description: "Lekcje przedmiotowe powinny w miarę możliwości odbywać się w dedykowanych pracowniach.",
    isHardByDefault: false,
  },
  ClusterBusyTimesConstraint: {
    title: "Grupowanie godzin pracy",
    category: "nauczyciele",
    description: "Zajęcia danego dnia powinny tworzyć zwarty, ciągły blok bez niepotrzebnych przerw.",
    isHardByDefault: false,
  },
  OrderEventsConstraint: {
    title: "Kolejność lekcji (np. wykład przed ćwiczeniami)",
    category: "organizacja",
    description: "Zachowanie wymaganej kolejności pomiędzy powiązanymi zajęciami.",
    isHardByDefault: false,
  },
};

export function getConstraintFriendlyInfo(constraint: ConstraintItem): ConstraintFriendlyInfo {
  const found = CONSTRAINT_FRIENDLY_MAP[constraint.type];
  if (found) return found;

  return {
    title: constraint.name || constraint.type,
    category: "organizacja",
    description: `Specjalna reguła organizacyjna instancji (${constraint.type}).`,
    isHardByDefault: constraint.required,
  };
}

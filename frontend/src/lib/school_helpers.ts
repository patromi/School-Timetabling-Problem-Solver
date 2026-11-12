import type { ConstraintItem, ConstraintScore, EventItem, InstanceDetail, ResourceItem } from "./types";

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
  AvoidSplitAssignmentsConstraint: {
    title: "Stały nauczyciel / sala dla przedmiotu",
    category: "organizacja",
    description: "Wszystkie godziny tego samego przedmiotu w klasie powinien prowadzić ten sam nauczyciel (lub odbywać się w jednej sali).",
    isHardByDefault: true,
  },
  SplitEventsConstraint: {
    title: "Podział zajęć na bloki i godziny",
    category: "organizacja",
    description: "Określa dozwoloną liczbę i długość bloków, na jakie dzielony jest przedmiot w tygodniu.",
    isHardByDefault: true,
  },
  DistributeSplitEventsConstraint: {
    title: "Rozkład długości bloków lekcyjnych",
    category: "organizacja",
    description: "Wymaga odpowiedniej liczby pojedynczych godzin lub bloków 2-godzinnych w ramach przedmiotu.",
    isHardByDefault: false,
  },
  SpreadEventsConstraint: {
    title: "Równomierne rozłożenie w tygodniu",
    category: "organizacja",
    description: "Lekcje z jednego przedmiotu powinny być równomiernie podzielone na różne dni tygodnia.",
    isHardByDefault: false,
  },
  LinkEventsConstraint: {
    title: "Zajęcia jednoczesne (grupy łączone)",
    category: "organizacja",
    description: "Powiązane zajęcia (np. grupy językowe lub międzyoddziałowe) muszą odbywać się dokładnie w tym samym czasie.",
    isHardByDefault: true,
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
    description: "Zapewnienie zrównoważonego obciążenia i nieprzekraczanie dziennych ram lekcyjnych.",
    isHardByDefault: false,
  },
  LimitWorkloadConstraint: {
    title: "Limit łącznego obciążenia (pensum)",
    category: "nauczyciele",
    description: "Pilnuje, aby łączna tygodniowa liczba godzin pracy mieściła się w ustalonym przedziale.",
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
    description: "Umieszczanie wskazanych przedmiotów w preferowanych godzinach (np. ścisłe rano, WF po południu).",
    isHardByDefault: false,
  },
  PreferResourcesConstraint: {
    title: "Preferowany przydział nauczycieli lub sal",
    category: "sale",
    description: "Zajęcia powinny być przydzielone wyłącznie wskazanym nauczycielom lub odpowiednim pracowniom.",
    isHardByDefault: false,
  },
  ClusterBusyTimesConstraint: {
    title: "Kompaktowy tydzień pracy (limit dni)",
    category: "nauczyciele",
    description: "Zajęcia powinny skupiać się w ustalonej liczbie dni roboczych w tygodniu.",
    isHardByDefault: false,
  },
  OrderEventsConstraint: {
    title: "Kolejność lekcji (chronologia)",
    category: "organizacja",
    description: "Zachowanie wymaganej kolejności pomiędzy powiązanymi zajęciami (np. wykład przed ćwiczeniami).",
    isHardByDefault: false,
  },
};

export function getConstraintFriendlyInfo(constraint: ConstraintItem): ConstraintFriendlyInfo {
  const found = CONSTRAINT_FRIENDLY_MAP[constraint.type];
  if (found) return found;

  return {
    title: constraint.name || constraint.type,
    category: "organizacja",
    description: `Reguła organizacyjna instancji (${constraint.type}).`,
    isHardByDefault: constraint.required,
  };
}

export function getConstraintScoreFriendlyInfo(score: ConstraintScore): ConstraintFriendlyInfo {
  const found =
    (score.type ? CONSTRAINT_FRIENDLY_MAP[score.type] : undefined) ||
    CONSTRAINT_FRIENDLY_MAP[score.id];

  if (found) {
    return {
      ...found,
      title: score.name || found.title,
    };
  }

  return {
    title: score.name || score.id,
    category: "organizacja",
    description: score.type
      ? `Reguła organizacyjna (${score.type}).`
      : "Zasada organizacji planu.",
    isHardByDefault: score.required,
  };
}

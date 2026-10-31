// frontend/src/lib/components/editor/types.ts

export type RoomType = "general" | "gym" | "lab" | "it" | string;

export interface SimpleClass {
  id: string;
  name: string;
  grade?: number;
  unavailableTimes?: string[];
  unavailableSoft?: boolean;
  unavailableWeight?: number;
  minDailyLessons?: number | null;
  maxDailyLessons?: number | null;
}

export interface SimpleTeacher {
  id: string;
  name: string;
  subject: string;
  subjects?: string[];
  workload: number;
  unavailableTimes?: string[];
  unavailableSoft?: boolean;
  unavailableWeight?: number;
  maxDaysPerWeek?: number | null;
  minDaysPerWeek?: number | null;
  maxDailyLessons?: number | null;
  maxGapsPerWeek?: number | null;
}

export interface SimpleRoom {
  id: string;
  name: string;
  type: string;
  unavailableTimes?: string[];
  unavailableSoft?: boolean;
  unavailableWeight?: number;
}

export interface SimpleLesson {
  id: string;
  subject: string;
  classId: string;
  teacherId: string; // puste "" oznacza wakat / nieprzypisany
  roomId: string;
  duration: number; // 1 or 2
  weeklyCount: number; // lessons per week
  timePreference?: "any" | "morning" | "afternoon" | "custom";
  preferredTimes?: string[];
  preferenceSoft?: boolean;
  preferenceWeight?: number;
}

export interface LinkedGroupItem {
  id: string;
  name: string;
  lessonIds: string[];
  isSoft?: boolean;
  weight?: number;
}

export interface OrderRuleItem {
  id: string;
  name: string;
  firstLessonId: string;
  secondLessonId: string;
  minSeparation: number;
  isSoft?: boolean;
  weight?: number;
}

export interface DayFormatted {
  id: string;
  name: string;
}

export interface BellTime {
  period: number;
  start: string;
  end: string;
}

export interface SubjectSummary {
  name: string;
  lessonCount: number;
  totalHours: number;
  teacherCount: number;
  teachers: string[];
  classes: string[];
}

export interface ResourceContextInfo {
  workload?: number;
  totalAssignedHours?: number;
  classes?: { classId: string; className: string; subject: string; hours: number }[];
  lessons?: { subject: string; teacherName: string; hours: number }[];
}

export interface ActiveAvailabilityTarget {
  id: string;
  name: string;
  type: "teacher" | "room" | "class";
  unavailableTimes: string[];
  isSoft: boolean;
  weight: number;
  maxDaysPerWeek?: number | null;
  minDaysPerWeek?: number | null;
  maxDailyLessons?: number | null;
  maxGapsPerWeek?: number | null;
  minDailyLessons?: number | null;
  contextInfo?: ResourceContextInfo;
}

export interface ActiveLessonPrefTarget {
  lessonId: string;
  subject: string;
  className: string;
  preference: "any" | "morning" | "afternoon" | "custom";
  preferredTimes: string[];
  isSoft: boolean;
  weight: number;
}

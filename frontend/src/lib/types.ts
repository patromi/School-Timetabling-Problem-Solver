export interface InstanceListItem {
  id: string;
  name: string;
  events_count: number;
  times_count: number;
  resources_count: number;
  constraints_count: number;
  is_custom: boolean;
}

export interface GroupItem {
  id: string;
  name: string;
  kind: string;
}

export interface TimeItem {
  id: string;
  name: string;
  group_refs: string[];
}

export interface ResourceTypeItem {
  id: string;
  name: string;
}

export interface ResourceItem {
  id: string;
  name: string;
  resource_type_ref: string;
  group_refs: string[];
}

export interface EventResourceItem {
  role: string;
  resource_ref: string | null;
  resource_type_ref: string | null;
  workload: number | null;
}

export interface EventItem {
  id: string;
  name: string;
  duration: number;
  course_ref: string | null;
  time_ref: string | null;
  workload: number | null;
  resources: EventResourceItem[];
  group_refs: string[];
}

export interface EventPairItem {
  first_event: string;
  second_event: string;
  min_separation: number;
  max_separation: number | null;
}

export interface AppliesToItem {
  event_groups: string[];
  events: string[];
  resource_groups: string[];
  resources: string[];
  time_groups: string[];
  times: string[];
  event_pairs: EventPairItem[];
}

export interface ConstraintItem {
  type: string;
  id: string;
  name: string;
  required: boolean;
  weight: number;
  cost_function: string;
  applies_to: AppliesToItem;
  params: Record<string, unknown>;
}

export interface InstanceDetail {
  id: string;
  name: string;
  time_groups: GroupItem[];
  times: TimeItem[];
  resource_types: ResourceTypeItem[];
  resource_groups: GroupItem[];
  resources: ResourceItem[];
  event_groups: GroupItem[];
  events: EventItem[];
  constraints: ConstraintItem[];
}

// Solver types
export interface SolveRequest {
  instance_id: string;
  iterations?: number;
  selector?: string;
  acceptance?: string;
  seed?: number;
  time_limit?: number | null;
  history_length?: number;
  sa_t_start?: number;
  sa_t_end?: number;
  sa_cooling?: number;
}

export interface SolverHistoryPoint {
  iteration: number;
  infeasibility: number;
  objective: number;
  elapsed: number;
}

export interface SolverStatus {
  status: "idle" | "running" | "completed" | "stopped" | "error";
  instance_id: string | null;
  iteration: number;
  total_iterations: number;
  infeasibility: number;
  objective: number;
  best_cost: number;
  rate: number;
  elapsed: number;
  error: string | null;
  history: SolverHistoryPoint[];
  has_result: boolean;
}

export interface TimetablePeriod {
  id: string;
  name: string;
}

export interface TimetableDay {
  id: string;
  name: string;
  periods: TimetablePeriod[];
}

export interface TimetableCellEntry {
  day_id: string;
  period_idx: number;
  event_ref: string;
  event_name: string;
  duration: number;
  other_resources: string[];
  is_clash: boolean;
}

export interface ConstraintScore {
  id: string;
  name: string;
  type?: string;
  required: boolean;
  weight: number;
  cost: number;
}

export interface TimetableResult {
  days: TimetableDay[];
  grids: Record<string, TimetableCellEntry[]>;
  scores: ConstraintScore[];
  violation_map: Record<string, string[]>;
  infeasibility: number;
  objective: number;
}

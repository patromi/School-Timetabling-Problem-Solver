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

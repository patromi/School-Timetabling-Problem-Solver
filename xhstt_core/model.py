from dataclasses import dataclass, field


@dataclass
class Group:
    """A TimeGroup/ResourceGroup/EventGroup (or one of their named subtypes,
    e.g. <Day>, <Week>, <Course> — `kind` records the actual XML tag)."""

    id: str
    name: str
    kind: str


@dataclass
class Time:
    id: str
    name: str
    group_refs: list[str] = field(default_factory=list)


@dataclass
class ResourceType:
    id: str
    name: str


@dataclass
class Resource:
    id: str
    name: str
    resource_type_ref: str
    group_refs: list[str] = field(default_factory=list)


@dataclass
class EventResource:
    """<ResourceType> is only mandatory when the resource slot is left
    unassigned (resource_ref is None) — if a concrete Resource is
    referenced, its type is already declared on that Resource."""

    role: str
    resource_ref: str | None = None
    resource_type_ref: str | None = None
    workload: float | None = None


@dataclass
class Event:
    id: str
    name: str
    duration: int
    course_ref: str | None = None
    resources: list[EventResource] = field(default_factory=list)
    group_refs: list[str] = field(default_factory=list)


@dataclass
class EventPair:
    first_event: str
    second_event: str
    min_separation: int = 0
    max_separation: int | None = None  # None = unbounded


@dataclass
class AppliesTo:
    event_groups: list[str] = field(default_factory=list)
    events: list[str] = field(default_factory=list)
    resource_groups: list[str] = field(default_factory=list)
    resources: list[str] = field(default_factory=list)
    time_groups: list[str] = field(default_factory=list)
    times: list[str] = field(default_factory=list)
    event_pairs: list[EventPair] = field(default_factory=list)


@dataclass
class Constraint:
    """Generic representation of any of the 16 XHSTT constraint types.

    `type` is the XML tag (e.g. "ClusterBusyTimesConstraint"). Parameters
    specific to a constraint type (Role, Duration, Minimum, Maximum,
    ResourceGroups, TimeGroups, ...) live in `params` rather than as typed
    fields, so new/rare constraint types need no model changes — the XHSTT
    spec explicitly allows modular extension of constraints."""

    type: str
    id: str
    name: str
    required: bool
    weight: int
    cost_function: str
    applies_to: AppliesTo = field(default_factory=AppliesTo)
    params: dict = field(default_factory=dict)


@dataclass
class SolutionEventResource:
    role: str
    resource_ref: str


@dataclass
class SolutionEvent:
    """One assignment for an Event. If SplitEventsConstraint applies, the
    same event_ref can appear multiple times — once per split occurrence —
    each with its own duration/time (see BrazilInstance1's solutions)."""

    event_ref: str
    time_ref: str | None = None
    duration: int | None = None
    resources: list[SolutionEventResource] = field(default_factory=list)


@dataclass
class Solution:
    instance_ref: str
    events: list[SolutionEvent] = field(default_factory=list)


@dataclass
class SolutionGroup:
    id: str
    solutions: list[Solution] = field(default_factory=list)


@dataclass
class Instance:
    id: str
    name: str
    time_groups: list[Group] = field(default_factory=list)
    times: list[Time] = field(default_factory=list)
    resource_types: list[ResourceType] = field(default_factory=list)
    resource_groups: list[Group] = field(default_factory=list)
    resources: list[Resource] = field(default_factory=list)
    event_groups: list[Group] = field(default_factory=list)
    events: list[Event] = field(default_factory=list)
    constraints: list[Constraint] = field(default_factory=list)

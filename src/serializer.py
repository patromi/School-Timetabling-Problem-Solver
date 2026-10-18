from typing import Any

from src.model import (
    AppliesTo,
    Constraint,
    Event,
    EventPair,
    EventResource,
    Group,
    Instance,
    Resource,
    ResourceType,
    Time,
)


def instance_to_dict(inst: Instance) -> dict[str, Any]:
    """Converts an Instance dataclass to a plain dictionary suitable for JSON serialization."""
    return {
        "id": inst.id,
        "name": inst.name,
        "time_groups": [
            {"id": g.id, "name": g.name, "kind": g.kind} for g in inst.time_groups
        ],
        "times": [
            {"id": t.id, "name": t.name, "group_refs": list(t.group_refs)}
            for t in inst.times
        ],
        "resource_types": [
            {"id": rt.id, "name": rt.name} for rt in inst.resource_types
        ],
        "resource_groups": [
            {"id": rg.id, "name": rg.name, "kind": rg.kind}
            for rg in inst.resource_groups
        ],
        "resources": [
            {
                "id": r.id,
                "name": r.name,
                "resource_type_ref": r.resource_type_ref,
                "group_refs": list(r.group_refs),
            }
            for r in inst.resources
        ],
        "event_groups": [
            {"id": eg.id, "name": eg.name, "kind": eg.kind} for eg in inst.event_groups
        ],
        "events": [
            {
                "id": e.id,
                "name": e.name,
                "duration": e.duration,
                "course_ref": e.course_ref,
                "time_ref": e.time_ref,
                "workload": e.workload,
                "resources": [
                    {
                        "role": er.role,
                        "resource_ref": er.resource_ref,
                        "resource_type_ref": er.resource_type_ref,
                        "workload": er.workload,
                    }
                    for er in e.resources
                ],
                "group_refs": list(e.group_refs),
            }
            for e in inst.events
        ],
        "constraints": [
            {
                "type": c.type,
                "id": c.id,
                "name": c.name,
                "required": c.required,
                "weight": c.weight,
                "cost_function": c.cost_function,
                "applies_to": {
                    "event_groups": list(c.applies_to.event_groups),
                    "events": list(c.applies_to.events),
                    "resource_groups": list(c.applies_to.resource_groups),
                    "resources": list(c.applies_to.resources),
                    "time_groups": list(c.applies_to.time_groups),
                    "times": list(c.applies_to.times),
                    "event_pairs": [
                        {
                            "first_event": ep.first_event,
                            "second_event": ep.second_event,
                            "min_separation": ep.min_separation,
                            "max_separation": ep.max_separation,
                        }
                        for ep in c.applies_to.event_pairs
                    ],
                },
                "params": c.params,
            }
            for c in inst.constraints
        ],
    }


def dict_to_instance(data: dict[str, Any]) -> Instance:
    """Converts a dictionary (e.g. from JSON payload) back to an Instance dataclass."""
    time_groups = [
        Group(
            id=g["id"],
            name=g.get("name", ""),
            kind=g.get("kind", "TimeGroup"),
        )
        for g in data.get("time_groups", [])
    ]
    times = [
        Time(
            id=t["id"],
            name=t.get("name", ""),
            group_refs=list(t.get("group_refs", [])),
        )
        for t in data.get("times", [])
    ]
    resource_types = [
        ResourceType(id=rt["id"], name=rt.get("name", ""))
        for rt in data.get("resource_types", [])
    ]
    resource_groups = [
        Group(
            id=rg["id"],
            name=rg.get("name", ""),
            kind=rg.get("kind", "ResourceGroup"),
        )
        for rg in data.get("resource_groups", [])
    ]
    resources = [
        Resource(
            id=r["id"],
            name=r.get("name", ""),
            resource_type_ref=r.get("resource_type_ref", ""),
            group_refs=list(r.get("group_refs", [])),
        )
        for r in data.get("resources", [])
    ]
    event_groups = [
        Group(
            id=eg["id"],
            name=eg.get("name", ""),
            kind=eg.get("kind", "EventGroup"),
        )
        for eg in data.get("event_groups", [])
    ]
    events = [
        Event(
            id=e["id"],
            name=e.get("name", ""),
            duration=int(e.get("duration", 1)),
            course_ref=e.get("course_ref"),
            time_ref=e.get("time_ref"),
            workload=e.get("workload"),
            resources=[
                EventResource(
                    role=er.get("role", ""),
                    resource_ref=er.get("resource_ref"),
                    resource_type_ref=er.get("resource_type_ref"),
                    workload=float(er["workload"])
                    if er.get("workload") is not None
                    else None,
                )
                for er in e.get("resources", [])
            ],
            group_refs=list(e.get("group_refs", [])),
        )
        for e in data.get("events", [])
    ]

    constraints = []
    for c in data.get("constraints", []):
        applies_data = c.get("applies_to", {})
        pairs = [
            EventPair(
                first_event=ep["first_event"],
                second_event=ep["second_event"],
                min_separation=int(ep.get("min_separation", 0)),
                max_separation=int(ep["max_separation"])
                if ep.get("max_separation") is not None
                else None,
            )
            for ep in applies_data.get("event_pairs", [])
        ]
        applies_to = AppliesTo(
            event_groups=list(applies_data.get("event_groups", [])),
            events=list(applies_data.get("events", [])),
            resource_groups=list(applies_data.get("resource_groups", [])),
            resources=list(applies_data.get("resources", [])),
            time_groups=list(applies_data.get("time_groups", [])),
            times=list(applies_data.get("times", [])),
            event_pairs=pairs,
        )
        constraints.append(
            Constraint(
                type=c["type"],
                id=c["id"],
                name=c.get("name", ""),
                required=bool(c.get("required", False)),
                weight=int(c.get("weight", 1)),
                cost_function=c.get("cost_function", "Linear"),
                applies_to=applies_to,
                params=dict(c.get("params", {})),
            )
        )

    return Instance(
        id=data["id"],
        name=data.get("name", data["id"]),
        time_groups=time_groups,
        times=times,
        resource_types=resource_types,
        resource_groups=resource_groups,
        resources=resources,
        event_groups=event_groups,
        events=events,
        constraints=constraints,
    )

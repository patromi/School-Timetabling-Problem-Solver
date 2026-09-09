import xml.etree.ElementTree as ET

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
    Solution,
    SolutionEvent,
    SolutionEventResource,
    SolutionGroup,
    Time,
)

_CONSTRAINT_META_TAGS = {"Name", "Required", "Weight", "CostFunction", "AppliesTo"}


def _text(node: ET.Element, tag: str, default: str | None = None) -> str | None:
    child = node.find(tag)
    return child.text if child is not None else default


def _find(node: ET.Element | None, tag: str) -> ET.Element | None:
    return node.find(tag) if node is not None else None


def _parse_groups(container: ET.Element | None) -> list[Group]:
    """Parses a *definition* container (e.g. <TimeGroups>) whose children are
    Groups (or named subtypes like <Day>/<Week>/<Course>) with Id + Name."""
    if container is None:
        return []
    return [
        Group(id=child.attrib["Id"], name=_text(child, "Name", ""), kind=child.tag)
        for child in container
    ]


def _collect_group_refs(node: ET.Element) -> list[str]:
    """Collects group references from a node's children: direct reference
    tags (e.g. <Day Reference="X"/>) and reference containers
    (e.g. <TimeGroups><TimeGroup Reference="X"/></TimeGroups>)."""
    refs = []
    for child in node:
        if child.tag == "Name":
            continue
        if "Reference" in child.attrib:
            refs.append(child.attrib["Reference"])
        else:
            refs.extend(
                sub.attrib["Reference"] for sub in child if "Reference" in sub.attrib
            )
    return refs


def _parse_times(times_node: ET.Element | None) -> list[Time]:
    if times_node is None:
        return []
    return [
        Time(
            id=t.attrib["Id"],
            name=_text(t, "Name", ""),
            group_refs=_collect_group_refs(t),
        )
        for t in times_node.findall("Time")
    ]


def _parse_resource_types(container: ET.Element | None) -> list[ResourceType]:
    if container is None:
        return []
    return [
        ResourceType(id=rt.attrib["Id"], name=_text(rt, "Name", ""))
        for rt in container.findall("ResourceType")
    ]


def _parse_resources(resources_node: ET.Element | None) -> list[Resource]:
    if resources_node is None:
        return []
    resources = []
    for r in resources_node.findall("Resource"):
        groups_node = r.find("ResourceGroups")
        resources.append(
            Resource(
                id=r.attrib["Id"],
                name=_text(r, "Name", ""),
                resource_type_ref=r.find("ResourceType").attrib["Reference"],
                group_refs=_collect_group_refs(groups_node)
                if groups_node is not None
                else [],
            )
        )
    return resources


def _parse_event_resources(resources_node: ET.Element | None) -> list[EventResource]:
    if resources_node is None:
        return []
    resources = []
    for r in resources_node.findall("Resource"):
        type_node = r.find("ResourceType")
        workload_text = _text(r, "Workload")
        resources.append(
            EventResource(
                role=_text(r, "Role", ""),
                resource_ref=r.attrib.get("Reference"),
                resource_type_ref=type_node.attrib["Reference"]
                if type_node is not None
                else None,
                workload=float(workload_text) if workload_text is not None else None,
            )
        )
    return resources


def _parse_events(events_node: ET.Element | None) -> list[Event]:
    if events_node is None:
        return []
    events = []
    for e in events_node.findall("Event"):
        course_node = e.find("Course")
        time_node = e.find("Time")
        event_groups_node = e.find("EventGroups")
        workload_text = _text(e, "Workload")
        events.append(
            Event(
                id=e.attrib["Id"],
                name=_text(e, "Name", ""),
                duration=int(_text(e, "Duration", "1")),
                course_ref=course_node.attrib["Reference"]
                if course_node is not None
                else None,
                time_ref=time_node.attrib["Reference"]
                if time_node is not None
                else None,
                workload=int(workload_text) if workload_text is not None else None,
                resources=_parse_event_resources(e.find("Resources")),
                group_refs=_collect_group_refs(event_groups_node)
                if event_groups_node is not None
                else [],
            )
        )
    return events


def _parse_refs_list(container: ET.Element | None, ref_tag: str) -> list[str]:
    if container is None:
        return []
    return [x.attrib["Reference"] for x in container.findall(ref_tag)]


def _parse_event_pairs(container: ET.Element | None) -> list[EventPair]:
    if container is None:
        return []
    pairs = []
    for ep in container.findall("EventPair"):
        max_sep_text = _text(ep, "MaxSeparation")
        pairs.append(
            EventPair(
                first_event=ep.find("FirstEvent").attrib["Reference"],
                second_event=ep.find("SecondEvent").attrib["Reference"],
                min_separation=int(_text(ep, "MinSeparation", "0")),
                max_separation=int(max_sep_text) if max_sep_text is not None else None,
            )
        )
    return pairs


def _parse_applies_to(node: ET.Element | None) -> AppliesTo:
    if node is None:
        return AppliesTo()
    return AppliesTo(
        event_groups=_parse_refs_list(node.find("EventGroups"), "EventGroup"),
        events=_parse_refs_list(node.find("Events"), "Event"),
        resource_groups=_parse_refs_list(node.find("ResourceGroups"), "ResourceGroup"),
        resources=_parse_refs_list(node.find("Resources"), "Resource"),
        time_groups=_parse_refs_list(node.find("TimeGroups"), "TimeGroup"),
        times=_parse_refs_list(node.find("Times"), "Time"),
        event_pairs=_parse_event_pairs(node.find("EventPairs")),
    )


def _parse_constraint_param(
    node: ET.Element,
) -> str | list[dict[str, str | None]] | None:
    if len(node) == 0:
        return node.text
    entries: list[dict[str, str | None]] = []
    for sub in node:
        if "Reference" not in sub.attrib:
            continue
        entry = {"reference": sub.attrib["Reference"]}
        entry.update({grand.tag: grand.text for grand in sub})
        entries.append(entry)
    return entries


def _parse_constraints(container: ET.Element | None) -> list[Constraint]:
    if container is None:
        return []
    constraints = []
    for c in container:
        params = {
            child.tag: _parse_constraint_param(child)
            for child in c
            if child.tag not in _CONSTRAINT_META_TAGS
        }
        constraints.append(
            Constraint(
                type=c.tag,
                id=c.attrib["Id"],
                name=_text(c, "Name", ""),
                required=_text(c, "Required", "false") == "true",
                weight=int(_text(c, "Weight", "0")),
                cost_function=_text(c, "CostFunction", "Linear"),
                applies_to=_parse_applies_to(c.find("AppliesTo")),
                params=params,
            )
        )
    return constraints


def _parse_solution_event_resources(
    resources_node: ET.Element | None,
) -> list[SolutionEventResource]:
    if resources_node is None:
        return []
    return [
        SolutionEventResource(
            role=_text(r, "Role", ""),
            resource_ref=r.attrib["Reference"],
        )
        for r in resources_node.findall("Resource")
    ]


def _parse_solution(solution_node: ET.Element) -> Solution:
    events = []
    events_node = solution_node.find("Events")
    if events_node is not None:
        for e in events_node.findall("Event"):
            time_node = e.find("Time")
            duration_text = _text(e, "Duration")
            events.append(
                SolutionEvent(
                    event_ref=e.attrib["Reference"],
                    time_ref=time_node.attrib["Reference"]
                    if time_node is not None
                    else None,
                    duration=int(duration_text) if duration_text is not None else None,
                    resources=_parse_solution_event_resources(e.find("Resources")),
                )
            )
    return Solution(instance_ref=solution_node.attrib["Reference"], events=events)


def parse_solution_groups(xml_text: str) -> list[SolutionGroup]:
    root = ET.fromstring(xml_text)
    container = root.find("SolutionGroups")
    if container is None:
        return []
    return [
        SolutionGroup(
            id=group_node.attrib["Id"],
            solutions=[_parse_solution(s) for s in group_node.findall("Solution")],
        )
        for group_node in container.findall("SolutionGroup")
    ]


def parse_archive(xml_text: str) -> list[Instance]:
    root = ET.fromstring(xml_text)
    instances = []
    for instance_node in root.find("Instances").findall("Instance"):
        times_node = instance_node.find("Times")
        resources_node = instance_node.find("Resources")
        events_node = instance_node.find("Events")
        instances.append(
            Instance(
                id=instance_node.attrib["Id"],
                name=_text(instance_node.find("MetaData"), "Name"),
                time_groups=_parse_groups(_find(times_node, "TimeGroups")),
                times=_parse_times(times_node),
                resource_types=_parse_resource_types(
                    _find(resources_node, "ResourceTypes")
                ),
                resource_groups=_parse_groups(_find(resources_node, "ResourceGroups")),
                resources=_parse_resources(resources_node),
                event_groups=_parse_groups(_find(events_node, "EventGroups")),
                events=_parse_events(events_node),
                constraints=_parse_constraints(instance_node.find("Constraints")),
            )
        )
    return instances

# Repair strategy: ClusterBusyTimesConstraint

## What this constraint means
A resource must be active (have at least one event) in a number of distinct
TimeGroups (typically days) that falls within [Minimum, Maximum].  "Active"
means the resource attends at least one event whose full duration span touches
any time slot in that group — not just the start time.

Violation: the resource is active in too many or too few days.

## Why it is hard to fix with generic moves
`move_random` does not reason about which days a resource is active on.
Moving a single event randomly may leave the resource still active on the
same days if it has other events there.  A targeted move must check whether
relocating an event actually changes the set of active days.

## Suggested algorithmic approaches

1. **Eliminate an extra active day**
   - Count how many distinct TimeGroups the resource is active in.
   - If count > Maximum: find a TimeGroup where the resource has exactly one
     event (removing it would make the resource inactive there).
   - Move that event to a slot inside a TimeGroup the resource is already
     active in (so the active-day count drops by one).

2. **Activate a missing day**
   - If count < Minimum: find a TimeGroup the resource is currently inactive
     in.
   - Pick any event of that resource and move it to a slot in that TimeGroup.

3. **Consolidation swap**
   - If count > Maximum: pick two events of the resource on two different
     "extra" days.
   - Try swapping them so both end up on days the resource was already
     active on.

## Useful data available in `instance` and `solution`
```python
# Map time_id -> list of group_ids (including Day groups)
time_to_groups = {t.id: t.group_refs for t in instance.times}

# Which days is a resource currently active on?
active_days = set()
for se in solution.events:
    for er in se.resources:
        if er.resource_ref == target_resource_id and se.time_ref is not None:
            active_days.update(instance.times[...].group_refs)

# Valid start times for an event of given duration
slots = valid_start_time_ids(instance, duration)  # already in globals
```

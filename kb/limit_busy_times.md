# Repair strategy: LimitBusyTimesConstraint

## What this constraint means
A resource (teacher, student group) must be busy for a number of time slots
within each specified TimeGroup (e.g. each day) that falls within [Minimum,
Maximum].  Violation: too many or too few events on a given day.

The deviation is the SUM across all target TimeGroups of the per-group
shortfall/excess, and the cost function is applied once to that total.

## Why it is hard to fix with generic moves
`move_random` redistributes events randomly — it may fix one day while
breaking another.  A targeted move that explicitly counts busy slots per
TimeGroup and moves events away from over-loaded (or toward under-loaded)
groups is far more effective.

## Suggested algorithmic approaches

1. **Over-loaded day relief**
   - For each resource in the constraint's AppliesTo, count busy time slots
     per TimeGroup.
   - Find a TimeGroup where the count exceeds Maximum.
   - Pick one event assigned to that group at random.
   - Move it to a slot in a TimeGroup that is below Maximum (or below Minimum
     if the resource is under-loaded there).

2. **Under-loaded day fill**
   - Find a TimeGroup where the resource's busy count is below Minimum.
   - Among all events assigned to that resource, find one that is NOT in
     that TimeGroup.
   - Move it to a slot inside the under-loaded TimeGroup.

3. **Swap across days**
   - Find two events assigned to the same resource: one in an over-loaded
     TimeGroup, one in an under-loaded one.
   - Swap their time slots if both new slots are valid.

## Useful data available in `instance` and `solution`
```python
# Map time_id -> list of group_ids it belongs to
time_to_groups = {t.id: t.group_refs for t in instance.times}

# Events assigned to a resource
for se in solution.events:
    for er in se.resources:
        if er.resource_ref == target_resource_id:
            # se.time_ref is the assigned start slot
            ...

# Valid start times for an event of given duration
slots = valid_start_time_ids(instance, duration)  # already in globals
```

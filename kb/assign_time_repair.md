# Repair strategy: AssignTimeConstraint

## What this constraint means
One or more events have no assigned start time (time_ref is None).  Every
unassigned duration unit contributes to the infeasibility cost.

## Suggested algorithmic approaches

1. **Greedy assignment**
   - Find all solution events where `se.time_ref is None`.
   - For each, call `valid_start_time_ids(instance, se.duration)` and pick the
     slot that minimises the total cost (or randomly if cost is not checked).

2. **Least-loaded slot**
   - Count how many events are already assigned to each time slot.
   - Assign unassigned events to the slot with the fewest existing assignments.

## Useful data
```python
unassigned = [se for se in solution.events if se.time_ref is None]
```

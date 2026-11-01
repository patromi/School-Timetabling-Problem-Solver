# Repair strategy: AvoidClashesConstraint

## What this constraint means
A resource (teacher, classroom, student group) is assigned to more than one
event at the same time.  The penalty equals the number of "extra" simultaneous
assignments summed across every overlapping time slot.

## Why it is hard to fix with generic moves
`move_random` picks an event uniformly at random — most events do not
participate in a clash, so most moves are wasted.  A targeted repair that
*first identifies a clashing event* and *then moves it* is far more efficient.

## Suggested algorithmic approaches

1. **Clash-targeted reassignment**
   - Find all resources that appear in more than one event at any time slot.
   - Pick one such resource at random.
   - Among its conflicting events, pick one at random.
   - Move that event to a time slot where the resource is free (sample from
     `valid_start_time_ids` filtered to times when the resource has no other
     assignment).

2. **Conflict graph local search**
   - Build a conflict graph: nodes = events, edge = shared resource at
     overlapping time.
   - Pick the event with the highest degree (most conflicts).
   - Relocate it to its best available slot (lowest cost after move).

3. **Swap-based repair**
   - Find two events that clash on resource R.
   - Try swapping their time slots: the clash on R disappears if their new
     times do not overlap.
   - Only accept the swap if it does not introduce a *new* clash for either
     event.

## Useful data available in `instance` and `solution`
```python
# All time ids in order
time_ids = [t.id for t in instance.times]

# Events assigned to a resource: iterate solution.events and check resources
for se in solution.events:
    for er in se.resources:
        if er.resource_ref == target_resource_id:
            ...

# Valid start times for an event of given duration
from src.evaluator_ref import valid_start_time_ids  # already in globals
slots = valid_start_time_ids(instance, duration)
```

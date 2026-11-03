# Repair strategy: LinkEventsConstraint

## What this constraint means
All events in an event group must occupy exactly the same set of time slots.
Deviation: the number of time slots that appear in at least one event's
occupied span but not in all of them.

Example: if event A occupies {Mon_1, Mon_2} and event B occupies {Mon_1, Tue_3},
the deviation is 2 (Mon_2 is in A but not B; Tue_3 is in B but not A).

## Why it is hard to fix with generic moves
`move_random` moves one event to a random slot — it has no concept of
aligning events within a group.  The repair must identify the disagreement
between events and explicitly synchronise their time assignments.

## Suggested algorithmic approaches

1. **Align to the majority slot**
   - For each event group under this constraint, collect the occupied time
     slots of every event.
   - Find the set of slots that appears in the most events (the "consensus").
   - For each event whose slots differ from the consensus, move it to match.

2. **Pairwise alignment**
   - Pick two events from the group whose occupied slots differ.
   - Move the second event to the same start time as the first (if the
     duration matches and the slot is valid).

3. **Copy-slot repair**
   - Pick one event in the group as the reference (e.g. the one with the
     lowest index).
   - For every other event in the group, move it to the reference event's
     start time (only if `valid_start_time_ids` allows it and durations match).

## Useful data available in `instance` and `solution`
```python
# Occupied slots for a solution event of duration d starting at time_ref t:
time_ids = [t.id for t in instance.times]
pos = {t: i for i, t in enumerate(time_ids)}
start = pos[se.time_ref]
occupied = set(time_ids[start : start + se.duration])

# Valid start times for an event of given duration
slots = valid_start_time_ids(instance, duration)  # already in globals
```

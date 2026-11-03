# Repair strategy: OrderEventsConstraint

## What this constraint means
For each event pair (A, B), the gap between the end of A's last sub-event and
the start of B's first sub-event must fall within [MinSeparation, MaxSeparation].
Gap = (position of B's earliest start) - (position of A's latest end) - 1.

Violation: gap < MinSeparation (B starts too soon after A) or
           gap > MaxSeparation (B starts too late after A).

## Why it is hard to fix with generic moves
`move_random` ignores the ordering relationship between event pairs entirely.
The repair must know which event in the pair to move and in which direction.

## Suggested algorithmic approaches

1. **Close the gap (gap too large)**
   - If gap > MaxSeparation: move event B earlier (toward A) until the gap
     is within [MinSeparation, MaxSeparation].
   - Try start times for B in decreasing order (earlier slots first) and
     pick the first valid one that satisfies the constraint.

2. **Widen the gap (gap too small)**
   - If gap < MinSeparation: move event B later (away from A) until the gap
     is within range.
   - Alternatively, move event A earlier so there is more room before B.

3. **Binary search over valid slots**
   - Enumerate `valid_start_time_ids` for B.
   - Filter to those whose position relative to A's end gives a gap in
     [MinSeparation, MaxSeparation].
   - Pick the slot with the lowest total cost among the filtered candidates.

## Useful data available in `instance` and `solution`
```python
# Position (integer index) of a time slot in the global order
time_ids = [t.id for t in instance.times]
pos = {t: i for i, t in enumerate(time_ids)}

# End position of an event (last slot it occupies, 0-indexed)
end_pos = pos[se.time_ref] + se.duration - 1

# Gap between event A (end_pos_a) and event B (start of B)
gap = pos[se_b.time_ref] - end_pos_a - 1

# Valid start times for an event of given duration
slots = valid_start_time_ids(instance, duration)  # already in globals
```

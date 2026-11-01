# Soft objective improvement strategy (feasible, low stagnation)

## Context
The solution is already feasible (no hard constraint violations).  The goal is
to reduce the soft (objective) cost — e.g. spread events more evenly across the
week, minimise idle time for teachers, cluster events for student groups.

## Suggested algorithmic approaches

1. **Idle-time minimisation**
   - For each teacher resource, identify their daily schedule.
   - If there are gaps (idle periods between events), try moving an event from
     a gap edge to a slot adjacent to another existing event.

2. **Event spreading**
   - For event groups that should be spread across days
     (SpreadEventsConstraint), pick one group.
   - Find days with too many events from that group.
   - Move one event to a day that has fewer.

3. **Workload balancing**
   - Count how many events each teacher has per day.
   - Move an event from an overloaded teacher/day to a lighter one.

4. **Greedy soft-cost descent**
   - Pick a random event.
   - Try all its valid start times.
   - Move to the one that gives the lowest total cost (even if improvement is
     tiny) — only soft cost changes since the solution is feasible.

## Notes
Moves must remain feasible.  After applying the move, verify that infeasibility
is still 0 (or accept only if the full cost is lower).  The 4th approach above
(greedy descent) is safe because it picks the globally cheapest slot.

# Improve strategy: SpreadEventsConstraint

## What this constraint means
Events in an event group must be spread across at least a minimum number of
distinct days (or time groups).  Violation: too many events from the group
land on the same day.

## Suggested algorithmic approaches

1. **Over-loaded day relief**
   - For each event group, count how many events land on each day.
   - Find the day with the most events from that group.
   - Move one of those events to a day that has fewer events from the group.

2. **Balanced spread**
   - Target a uniform distribution of the group's events across all days.
   - Iteratively move events from over-represented days to under-represented
     ones, checking that the move is valid.

# Repair/improve strategy: LimitIdleTimesConstraint

## What this constraint means
A resource has too many or too few idle time slots between their first and
last assignment on a given day.  Typical form: "teachers should have at most
2 idle periods per day."

## Suggested algorithmic approaches

1. **Compact teacher schedule**
   - For each teacher, find their earliest and latest assigned slot per day.
   - Count idle slots between them.
   - If idle count exceeds the limit, move an event from a day edge to fill
     a gap (i.e. move the earliest event later, or the latest event earlier).

2. **Day boundary contraction**
   - Pick the teacher with the highest idle-time cost.
   - Move their earliest or latest daily event inward to reduce span.

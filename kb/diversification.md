# Diversification strategy (high stagnation, feasible solution)

## Context
The solver has been feasible for many iterations without improving the best
known solution.  It is likely stuck in a local optimum.  Standard single-event
moves are insufficient — a larger perturbation that breaks the current structure
is needed.

## Suggested algorithmic approaches

1. **Ruin-and-recreate (medium scale)**
   - Randomly select 5–15% of all events.
   - Reassign each to a completely random valid time slot (ignoring cost).
   - Recreate them one by one in a random order, each to its locally best slot.

2. **Day-based shuffle**
   - Pick a random day (time group of kind "Day").
   - Collect all events assigned to that day.
   - Randomly shuffle their time slots among themselves (swap within the day).

3. **Kempe-chain perturbation**
   - Pick two random time slots T1 and T2.
   - Find all events at T1 that share a resource with any event at T2.
   - Swap T1↔T2 for that connected component.

4. **Resource-group scatter**
   - Pick a resource group (e.g. all classes of a year).
   - For each resource in the group, move all its events to new random slots.

## Notes
After a large perturbation the solver may temporarily worsen — this is
intentional.  LAHC's history window will accept the worsened state as long as
it is no worse than the solution from L steps ago.

# Temporal Pruning of Insertion Positions in DARP Insertion Heuristic

## Goal

Speed up the insertion heuristic (IH) for long vehicle plans in DARP instances with a long time horizon and short maximum passenger delay.

In the current IH, for each request–plan combination, all possible pickup and drop-off insertion-position pairs are evaluated. When plans become long, this is expensive even though a newly processed request can usually only be inserted close to actions occurring in a similar time interval.

The optimization should preserve the behavior of the existing IH: it must only remove insertion positions that are provably infeasible.

---

## Background

A vehicle plan is an ordered sequence of pickup and drop-off actions:

\[
p = (a_1, a_2, \ldots, a_n).
\]

When a new request is inserted, the existing actions are **not reordered**. Only two new actions are inserted:

- pickup action \(o_r\),
- drop-off action \(d_r\), after \(o_r\).

The baseline IH evaluates all pairs of insertion positions:

\[
j \in \{1,\ldots,n+1\},
\qquad
k \in \{j+1,\ldots,n+2\}.
\]

Therefore, the number of candidate pairs for one request–plan combination is quadratic in the plan length.

---

## Key Observation

For a feasible fixed-order plan, every existing action has an **effective feasible time interval**

\[
[E_i, L_i],
\]

where:

- \(E_i\) is the earliest time at which action \(a_i\) can be performed while respecting all constraints of the existing plan;
- \(L_i\) is the latest time at which action \(a_i\) can be performed while respecting all constraints of the existing plan.

These must be the effective bounds under the current fixed action order, not only the original request-level time-window bounds.

Because the plan order is fixed, the effective action bounds are chronologically monotone:

\[
E_1 \le E_2 \le \cdots \le E_n,
\]

\[
L_1 \le L_2 \le \cdots \le L_n.
\]

Travel time and service time, if present, make these inequalities stricter; they do not break the monotonicity.

---

## Safe Pruning Rule for a New Action

Consider inserting one new action \(x\) with admissible time interval

\[
[e_x, l_x].
\]

### Lower index bound

If an existing action \(a_i\) satisfies

\[
L_i < e_x,
\]

then \(x\) cannot be inserted before \(a_i\). Since all earlier actions also satisfy \(L_h \le L_i < e_x\), no insertion position before or at this temporal prefix can be feasible.

Therefore, all insertion positions before the first possibly overlapping part of the plan can be skipped.

### Upper index bound

If an existing action \(a_i\) satisfies

\[
E_i > l_x,
\]

then \(x\) cannot be inserted after \(a_i\). Since all later actions also satisfy \(E_h \ge E_i > l_x\), no insertion position after this temporal suffix can be feasible.

Therefore, all insertion positions after the last possibly overlapping part of the plan can be skipped.

### Consequence

The candidate insertion positions for action \(x\) form one contiguous interval of plan positions. Its endpoints can be found using binary search on the monotone arrays of effective bounds.

---

## Application to Request Insertion

For each request–plan combination:

1. Obtain the admissible time interval of the new pickup action \(o_r\).
2. Use binary search over the effective action bounds of the current plan to obtain a candidate interval of pickup insertion positions.
3. Obtain the admissible time interval of the new drop-off action \(d_r\).
4. Use binary search to obtain a candidate interval of drop-off insertion positions.
5. Enumerate only pickup/drop-off insertion pairs that:
   - lie inside these candidate intervals;
   - preserve pickup-before-drop-off ordering;
   - follow the existing index semantics after pickup insertion.
6. Run the current full feasibility and cost evaluation unchanged on each retained pair.

The binary-search pruning is only a necessary-condition filter. A retained candidate pair may still be infeasible after actual insertion because the two inserted actions can tighten the feasible timing of surrounding actions. Therefore, the existing full feasibility test must remain authoritative.

---

## Correctness Requirement

The optimized IH must return exactly the same result as the baseline IH, assuming identical tie-breaking.

This follows because the optimization removes only candidates where at least one inserted action is temporally impossible relative to the fixed-order effective time bounds of the current plan. Every insertion pair that could be feasible under the baseline enumeration remains evaluated by the optimized method.

Implementation should include tests comparing the baseline and optimized variants on the same instances, verifying:

- same selected plan/vehicle and insertion positions, or at least same solution under existing tie-breaking;
- same final objective/cost;
- same feasibility result.

---

## Effective Bound Maintenance

The optimization assumes that each current plan provides effective earliest/latest action times. If these are not already maintained by the implementation, they should be computed for a feasible fixed-order plan using forward/backward propagation.

Conceptually, for action \(a_i\) with raw feasible window \([e_i,l_i]\), travel time \(t(a_i,a_{i+1})\), and service time \(s_i\):

\[
E_{i+1}
=
\max\left(e_{i+1}, E_i + s_i + t(a_i,a_{i+1})\right),
\]

\[
L_i
=
\min\left(l_i, L_{i+1} - s_i - t(a_i,a_{i+1})\right).
\]

The codebase may already store equivalent quantities under different names. Reuse existing propagated timing data where possible rather than adding duplicate scheduling logic.

Effective bounds need to be updated only when a vehicle plan is actually changed by an accepted insertion, not for every rejected request–plan trial.

---

## Complexity Motivation

For a plan with \(n\) existing actions, the baseline insertion loop considers \(O(n^2)\) pickup/drop-off index pairs.

Let \(w_o\) and \(w_d\) be the numbers of candidate pickup and drop-off positions remaining after temporal pruning. The optimized enumeration examines approximately

\[
O(\log n + w_o w_d)
\]

candidates/search operations for the request–plan combination, in addition to the unchanged feasibility evaluation of retained insertion pairs.

In long-horizon instances with short maximum delay, \(w_o\) and \(w_d\) should be much smaller than \(n\), because a request can interact temporally with only a small neighborhood of actions in a long plan.



## Optional Activation Threshold

For short plans, the overhead of binary searches and additional branching may not improve runtime. The implementation may therefore apply temporal pruning only when a plan contains at least a configurable number of actions.

A reasonable initial experimental threshold is:

\[
n_{\min} = 32 \text{ actions}.
\]

This is not a theoretical requirement. It should be validated by benchmarks and may be moved, removed, or replaced by an always-enabled implementation if the overhead is negligible.



## Implementation Cautions

- Use **effective propagated bounds of actions in the current plan**, not unpropagated raw request windows.
- Preserve all current feasibility checks.
- Preserve current cost evaluation and tie-breaking behavior.
- Carefully handle the index shift caused by inserting pickup before considering drop-off.
- Treat the plan's existing action order as immutable during each insertion trial.
- Prefer reusing timing information already stored by the feasibility-checking logic.

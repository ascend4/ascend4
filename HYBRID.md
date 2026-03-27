# Hybrid DAE Design Notes

## Purpose

This note records the current design direction and implementation status for
hybrid DAE support in ASCEND.

The implemented feature set currently centres on:

- `pre(x)` as an event-iteration value operator
- `REINIT(...)` for event-time state/discrete updates
- event iteration and same-time restart handling in IDA
- selector-style state variables with `WHEN(mode)` and `SWITCH TO ... IF ...`

The note is intentionally compact. It keeps only the rationale and outstanding
issues that still matter for current implementation and review.

## Current Status

Implemented and passing for the tested IDA cases:

- `pre(x)` parsing and evaluation in event-time `REINIT(...)`
- `REINIT(...)` inside `WHEN` cases
- solver-side lowering of `REINIT(...)` onto `when_case` action lists
- event iteration at a fixed time point, including advancing `pre(x)` between
  event iterations
- continuous real state resets
- inferred discrete real event-memory updated through `REINIT(...)`
- boolean, integer, and symbol discrete updates through `REINIT(...)`
- selector-like state syntax:
  - `mode IS_A selector OF modes DEFAULT '...'`
  - `WHEN(mode) ... CASE 'state': ... END WHEN`
  - `SWITCH TO 'state' IF guard;`
- direct integer/symbol-controlled `WHEN(...)` dispatch in the runtime path
- simple Zeno/event-accumulation detection for IDA restart loops

Primary regression and example coverage:

- [test_ida.c](./ascend/integrator/test/test_ida.c)
- [reinit.a4c](./models/test/ida/reinit.a4c)
- [reinit_discrete.a4c](./models/test/ida/reinit_discrete.a4c)
- [reinit_bool.a4c](./models/test/ida/reinit_bool.a4c)
- [reinit_bool_cascade.a4c](./models/test/ida/reinit_bool_cascade.a4c)
- [multi_boundary.a4c](./models/test/ida/multi_boundary.a4c)
- [switchto.a4c](./models/test/ida/switchto.a4c)
- [ideal_rebound.a4c](./models/johnpye/dyn/ideal_rebound.a4c)
- [lengthening_sawtooth.a4c](./models/johnpye/dyn/lengthening_sawtooth.a4c)

At the time of writing:

- the IDA hybrid/event regressions pass
- LSODE rejects evented models rather than trying to run them
- CMSlv can ignore `REINIT(...)` where it is irrelevant to its algebraic
  conditional solve semantics
- selector state is exposed in the browser/`ascpy` object view, although the
  selector domain/default metadata are not yet richly represented as their own
  inspectable child structure

## Design Decisions

### 1. `REINIT(x, expr)` is the current reset syntax

The chosen spelling is:

```ascend
REINIT(v, -e * pre(v));
```

not:

```ascend
REINIT v := -e * pre(v);
REINIT v = -e * pre(v);
```

Reasons:

- `:=` already means ordinary assignment in ASCEND
- `=` already means a relation statement
- `REINIT(x, expr)` avoids pretending that a reset is either an imperative
  assignment or a continuously active equation

For current ASCEND, `REINIT(...)` is the right surface form even though later
Phase 2 work may introduce a cleaner split between continuous reset and other
discrete transition actions.

### 2. `pre(x)` is an event-iteration operator

`pre(x)` is currently parsed as an expression operator, but it is only
semantically valid during event processing.

Current rule:

- `pre(x)` is accepted only inside `REINIT(...)`
- outside event-time `REINIT(...)`, it is rejected

The implemented event semantics deliberately advance `pre(x)` across
same-time event iterations. That means:

- most event iterations still see `x = pre(x)`
- if an earlier event action in the same cascade changes `x`, a later
  `REINIT(...)` sees that updated value through `pre(x)`

This matches the intended fixed-point event-iteration semantics better than a
frozen single pre-event snapshot.

### 3. Event semantics remain declarative

Even where the surface syntax looks action-like, the semantics remain
declarative.

The current intended event flow is:

1. detect a boundary/event
2. determine active discrete configuration
3. apply newly active transition actions
4. solve a same-time consistency restart
5. repeat until the discrete configuration is stable
6. continue continuous integration

This is not source-ordered imperative execution.

### 4. Current target classes for `REINIT(...)`

`REINIT(...)` now supports three broad target classes:

- continuous real integrator states
- inferred discrete real event-memory variables
- discrete boolean/integer/symbol variables

Real algebraic variables are rejected explicitly as `REINIT(...)` targets,
because any such value would only be overwritten by the post-event DAE
consistency solve.

### 5. Discrete real event-memory is inferred, not flagged manually

Early prototypes used an explicit `.discrete := TRUE` flag on real atoms.
That has now been removed.

Current rule:

- if a real `REINIT(...)` target is a differential/integrator state, it is
  treated as such
- otherwise, if it is a real `solver_var` not in the continuous DAE relation
  set, it is inferred as a discrete real event-memory variable

That keeps the user syntax cleaner and avoids inventing a third parallel family
of real atom types.

### 6. Selector state is explicit

The chosen Phase 2 direction is an ASCEND-style selector declaration:

```ascend
modes IS_A set OF symbol_constant;
modes :== ['free', 'impact'];

mode IS_A selector OF modes DEFAULT 'free';
```

and selector-driven control:

```ascend
WHEN(mode)
    CASE 'free':
        SWITCH TO 'impact' IF hit_floor AND descending;
    CASE 'impact':
        REINIT(v, -e * pre(v));
        SWITCH TO 'free' IF TRUE;
END WHEN;
```

This preserves the existing `WHEN ... CASE` shell while adding the gPROMS-like
`SWITCH TO ... IF ...` syntax that keeps source state, guard, and target state
together.

### 7. Selector transition syntax is dynamic, not steady-state MIP semantics

The current selector form is fundamentally transition-based:

- `CASE 'state'` describes the equations/actions active while *in* that state
- `SWITCH TO 'other' IF ...` describes the condition for *leaving* that state

That is appropriate for hybrid DAE/event simulation, but it is not the same
thing as CMSlv/MIP-style configuration semantics, where the discrete logic
describes which state is currently admissible at steady state.

So:

- current selector/`SWITCH TO` syntax is meaningful for dynamic simulation
- it can also be used in steady-state only when the selector value is already
  fixed and the `WHEN` just dispatches the active equations
- it is **not** yet a direct front-end for solver-chosen LP/MIP mode
  selection

If ASCEND later wants true steady-state optimisation over selector states, it
will likely need a separate configuration/disjunction interpretation above the
current dynamic transition semantics, even if selector declarations and
state-local equation blocks are shared.

## Current Working Syntax

### Minimal event/reset syntax

The Phase 1 style remains valid:

```ascend
CONDITIONAL
    boundary: y <= r;
END CONDITIONAL;

switch == SATISFIED(boundary, 1e-8 {m});

WHEN(switch)
    CASE TRUE:
        REINIT(v, -e * pre(v));
    CASE FALSE:
        ...
END WHEN;
```

### Current best selector-style syntax

The best current selector-style form is:

```ascend
modes IS_A set OF symbol_constant;
modes :== ['free', 'impact'];

mode IS_A selector OF modes DEFAULT 'free';

WHEN(mode)
    CASE 'free':
        SWITCH TO 'impact' IF hit_floor AND descending;
    CASE 'impact':
        REINIT(v, -e * pre(v));
        REINIT(t_last_event, t);
        SWITCH TO 'free' IF TRUE;
END WHEN;
```

Current guard support is now split into two working tiers:

- `SWITCH TO ... IF ...` can consume rich boolean-valued expressions over
  already-declared discrete conditions
- simple active-case continuous comparison guards such as `h >= h_weir` are
  now also exposed directly as IDA root functions

So this works today:

```ascend
SWITCH TO 'running' IF start_cmd AND cooldown_ok AND NOT tripped;
```

and this now works too:

```ascend
SWITCH TO 'aboveweir' IF h > h_weir;
```

What is still *not* first-class yet is the automatic lowering of arbitrarily
complex continuous/logical combinations into event sources. For those cases,
explicit `CONDITIONAL`, `SATISFIED(...)`, and/or logrelations are still the
right path.

State-local equations inside selector cases now work in a first useful slice.
For example, [overflowing_weir.a4c](./models/johnpye/dyn/overflowing_weir.a4c)
uses:

- globally active mass-balance equations outside the selector block
- selector-local overflow equations inside sibling `CASE` branches
- `SWITCH TO` transitions driven by an explicitly declared boundary boolean

## Event Iteration and Restart Semantics

The IDA path now performs same-time event iteration rather than a single
"apply one batch of resets and restart" step.

Current loop:

- settle logical/discrete configuration with LRSlv where relevant
- apply any newly active `REINIT(...)` or `SWITCH TO ...` actions
- perform one same-time consistency restart
- repeat until no new discrete changes or newly active actions remain

Important implemented details:

- `pre(x)` advances between event iterations
- simultaneous same-direction boundary crossings from different sources are
  combined into one logical event
- mixed TRUE/FALSE simultaneous target sets are still rejected
- IDA root-direction handling was corrected so rebound-style models do not
  immediately refire the opposite event on restart

## Current Examples

Two user-facing examples under [models/johnpye/dyn](./models/johnpye/dyn) are
kept readable rather than purely regression-oriented:

- [ideal_rebound.a4c](./models/johnpye/dyn/ideal_rebound.a4c)
  - ideal bounce without contact kinetics
  - now uses selector-style syntax
  - intentionally does not settle to rest, so it remains a useful Zeno test
- [resting_rebound.a4c](./models/johnpye/dyn/resting_rebound.a4c)
  - settling counterpart to `ideal_rebound`
  - demonstrates a working `'free'` / `'impact'` / `'rest'` selector model
  - shows that a resting contact mode is now possible when the active-case
    formulation preserves a coherent first-order DAE shape
- [lengthening_sawtooth.a4c](./models/johnpye/dyn/lengthening_sawtooth.a4c)
  - repeated resets using discrete real event-memory
- [overflowing_weir.a4c](./models/johnpye/dyn/overflowing_weir.a4c)
  - selector-driven switching with equations local to sibling selector cases
  - now uses direct `SWITCH TO ... IF V >= A * h_weir` style guards

## Zeno / Event Accumulation

IDA now has a simple event-accumulation stop to avoid wasting CPU in obvious
Zeno/chattering cases.

Current mechanism:

- keep a fixed ring buffer of recent event times
- parameters:
  - `zeno_ncycles`
  - `zeno_duration`
- if `zeno_ncycles` events occur within `zeno_duration`, stop cleanly with an
  event-accumulation diagnostic

This is intentionally the simple pragmatic form:

- it does not try to classify event source signatures
- it is only meant to stop pathological same-time or near-same-time cycling
  promptly

The current `ideal_rebound` model is intentionally kept non-settling so this
behavior remains exercised and visible.

## Current Limitations

### 1. State-local equations are only implemented in a first cut

The major remaining syntax/runtime gap is not "can we put equations in cases"
any more. That first slice now works. The remaining issue is to harden and
generalize the semantics.

What is wanted eventually:

```ascend
WHEN(mode)
    CASE 'free':
        F_net = F_g;
        SWITCH TO 'impact' IF hit_floor AND descending;
    CASE 'impact':
        REINIT(v, -e * pre(v));
        SWITCH TO 'rest' IF settling;
        SWITCH TO 'free' IF NOT settling;
    CASE 'rest':
        y = r;
        v = 0 {m/s};
END WHEN;
```

What is possible now:

- globally active equations outside the `WHEN`
- equations directly inside selector `CASE` bodies
- optional `USE ...` inside cases where that is still convenient
- transition logic and actions inside the cases
- sibling case-local relations already replace each other at the ACTIVE-bit
  level; only the selected case contributes its local relations

What is **not** yet hardened enough:

- the validation and diagnostics around more complex replacing case families
- the full replacing semantics for more complex state-local equation sets
- nested/richer combinations that would let us express arbitrary resting or
  contact states without worrying about the active-case DAE shape

So the work has moved from "syntax missing" to "semantics need hardening".

Recent probing clarified the current practical boundary:

- a settling bouncing-ball `'rest'` mode now works in `resting_rebound.a4c`
- the key was to keep the active-case formulation close to the already-working
  first-order DAE shape from `ideal_rebound.a4c`
- earlier attempts that introduced an extra algebraic support-force balance or
  moved too much of the derivative structure inside the cases were rejected by
  IDA analysis

So the lesson is now clearer:

- a resting/contact state is possible with the current machinery
- but it still needs a carefully chosen post-switch formulation that preserves
  a coherent first-order DAE shape for IDA
- the old alternative-structure checker is part of this story, and has already
  been tightened to correctly detect at least one previously missed mismatch:
  same number of local equations, same overall incident-variable set, but a
  different per-relation incidence grouping across sibling cases

### 2. Common equations should not need duplication

The language should allow:

- globally active equations outside the selector block
- state-local transitions and actions inside the selector block

It should **not** force duplicated `USE free_flight;`-style clauses when the
continuous equations are actually common across states.

### 3. Direct continuous `SWITCH TO` guards are implemented narrowly

The current direct-guard path is deliberately narrow:

- simple active-case comparison guards such as `h >= h_weir` now become extra
  IDA root functions
- rich boolean combinations over already-defined discrete conditions also work

What is still missing is automatic event-source generation for arbitrary mixed
continuous/logical guard expressions. For example, something like

```ascend
SWITCH TO 'running' IF start_cmd AND storage > storage_min AND t - t_last > deadtime;
```

still benefits from explicit `CONDITIONAL` / `SATISFIED(...)` plumbing today.

So the remaining gap is no longer "no direct continuous guards"; it is
"general composite guard lowering is not there yet".

The LRSlv-backed path is now working again:

- selector/integer/symbol `SWITCH TO` driven by an explicit discrete trigger
  boolean works in the `switchto_*_mode` regressions
- the newer direct continuous-guard selector path still works too

The important repair there was more reliable same-time discrete-change
detection during boundary/event iteration, not a second transition mechanism.

### 4. Some lower-bound chatter still appears near impacts

The current ideal-bounce path can still emit noisy lower-bound messages during
root localisation near impacts, even though the sampled trajectory is correct.

This is mainly a runtime polish issue rather than a semantic gap.

### 5. Opt-in hybrid trace now exists for debugging

The IDA wrapper now has a lightweight opt-in event trace:

- set `ASCEND_HYBRID_TRACE=1`
- the solver will emit checkpoint lines around event iteration and
  continuation
- current output includes:
  - checkpoint label and time
  - currently selected solver
  - discrete variables participating in `WHEN`s
  - active `WHEN` cases

This is still developer-oriented rather than polished user-facing output, but
it is already useful for diagnosing event-iteration and reconfiguration bugs.

## Future Work

Near term:

1. define and harden the replacing semantics between sibling selector cases
   now that the first-cut state-local equation path exists
   - treat this mainly as a validation/diagnostics problem now, because the
     basic ACTIVE-bit replacement is already in place
2. extend the `models/johnpye/dyn` examples from first-cut working cases to
   richer settling/mode-holding examples
3. improve selector exhaustiveness diagnostics and browser/tree presentation
4. add transition-conflict diagnostics when multiple outgoing transitions from
   one state are simultaneously enabled
5. widen direct-guard lowering from simple comparisons to richer mixed
   continuous/logical expressions where worthwhile
6. refine the current hybrid trace into clearer user-facing event diagnostics

After that:

7. revisit surface syntax for richer discrete reassignment if it becomes
   clearly worthwhile beyond current widened `REINIT(...)`
8. refine event/logical syntax so explicit condition/logrel declarations and
   selector transitions work together naturally rather than competing

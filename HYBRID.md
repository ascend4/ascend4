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

Important current limitation:

- `SWITCH TO ... IF ...` can already consume rich boolean-valued expressions
- but continuous event sources still need to be defined separately through
  `CONDITIONAL`, `SATISFIED(...)`, and/or logrelations

So this works today:

```ascend
SWITCH TO 'running' IF start_cmd AND cooldown_ok AND NOT tripped;
```

but this is not yet a first-class event source on its own:

```ascend
SWITCH TO 'aboveweir' IF h > h_weir;
```

For now, the latter still needs explicit old-style condition plumbing.

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
- [lengthening_sawtooth.a4c](./models/johnpye/dyn/lengthening_sawtooth.a4c)
  - repeated resets using discrete real event-memory

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

### 1. State-local equations are not yet true replacing cases

The major remaining syntax/runtime gap is state-local equation selection.

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
- optional `USE ...` inside cases
- transition logic and actions inside the cases

What is **not** yet possible cleanly:

- equations inside sibling selector cases that replace one another structurally

Current case-local activation is effectively additive. That is why a proper
`rest` mode for the bouncing ball still cannot be expressed cleanly without
overconstraining the model.

### 2. Common equations should not need duplication

The language should allow:

- globally active equations outside the selector block
- state-local transitions and actions inside the selector block

It should **not** force duplicated `USE free_flight;`-style clauses when the
continuous equations are actually common across states.

### 3. Direct continuous `SWITCH TO` guards are not yet event sources

As noted above, current `SWITCH TO ... IF ...` guards can consume rich boolean
logic, but the condition definitions themselves still need the older explicit
condition/logrel layer.

That is a real remaining Phase 2 gap, not merely a cosmetic syntax issue.

### 4. Some lower-bound chatter still appears near impacts

The current ideal-bounce path can still emit noisy lower-bound messages during
root localisation near impacts, even though the sampled trajectory is correct.

This is mainly a runtime polish issue rather than a semantic gap.

## Future Work

Near term:

1. add true state-local equation support in selector `CASE` bodies
2. define replacing semantics between sibling state branches
3. add direct continuous/event-source generation for `SWITCH TO ... IF ...`
   guards
4. improve selector exhaustiveness diagnostics and browser/tree presentation

After that:

5. revisit surface syntax for richer discrete reassignment if it becomes
   clearly worthwhile beyond current widened `REINIT(...)`
6. refine event/logical syntax so explicit condition/logrel declarations and
   selector transitions work together naturally rather than competing

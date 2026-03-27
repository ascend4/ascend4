# Integration Language Plan

## Scope

This note captures the intended METHOD-language support for dynamic runs,
reported trajectories, and the relationship between solver-side `STUDY`
requests and integrator-side reporting.

It is separate from:

- [`DER.md`](DER.md) for derivative and index-analysis work
- [`HYBRID.md`](HYBRID.md) for hybrid/event semantics

## Design Direction

The intended user model is parallel to the existing:

- `SOLVER`
- `OPTION`
- `SOLVE`
- `STUDY`

family.

For dynamic runs, the target family is:

- `INTEGRATOR`
- `OPTION`
- `OBSERVE`
- `INTEGRATE`

with `OBSERVE` shared between `STUDY` and `INTEGRATE`.

## Focus Rule

To keep plain `OPTION` unambiguous, the GUI/CLI and METHOD language should
treat either a solver or an integrator as being "in focus" at a given time.

Initial working rule:

- `SOLVER name;` sets solver focus
- `INTEGRATOR name;` sets integrator focus
- `OPTION foo value;` applies to the currently focused target

This is a front-end/language rule, not a claim that libascend never needs
both solver and integrator-side machinery internally.

Current planning assumption:

- unqualified `OPTION` remains acceptable
- whichever of solver or integrator is most recently selected becomes the
  current focus
- explicit `OPTION OF ...` syntax is not needed in the first pass

## Observation Lists

`obs_id` remains a useful legacy/default mechanism, but it should not be the
main long-term way of selecting reported variables.

The preferred language mechanism is:

```ascend
OBSERVE y, v, mode, t_last_event;
OBSERVE y, v AS zoom;
```

Meaning:

- unnamed `OBSERVE` sets the current default observation list
- named `OBSERVE ... AS name;` stores a reusable named list

The same observed-instance machinery should be used by both:

- `STUDY`
- `INTEGRATE`

First-pass error rules:

- it is an error to run `STUDY` with no explicit observed list and no
  default `OBSERVE` list
- it will likewise be an error to run `INTEGRATE` with no explicit/default
  observed list

## `STUDY` Evolution

Current legacy syntax:

```ascend
STUDY y, x VARY x FROM 1 TO 10 STEPS 9;
```

Target transitional syntax:

```ascend
OBSERVE y, x;
STUDY VARY x FROM 1 TO 10 STEPS 9;
```

Rules:

- explicit observed variables on `STUDY` still work
- if `STUDY` omits its observed-var list, it uses the current default
  `OBSERVE` list
- it is an error to run `STUDY` with neither an explicit list nor a default
  `OBSERVE` list

Longer-term, the explicit observed-var list on `STUDY` may be deprecated if
the shared `OBSERVE` mechanism proves satisfactory.

## `INTEGRATE` Target Syntax

The current target syntax is:

```ascend
INTEGRATOR IDA;
OPTION rtol 1e-6;
OPTION maxstep 1 {s};

OBSERVE y, v, mode, t_last_event;

INTEGRATE FROM 0 {s} TO 3600 {s} STEPS 10;
INTEGRATE FROM 3600 {s} TO 3610 {s} STEPS 100;
```

Possible later extensions:

- `OBSERVE zoom` selection on a specific `INTEGRATE`
- `MICROSTATES NONE|ENDPOINTS|ALL`
- output file naming
- `NOW`
- engine selection overrides

The intended CLI family should mirror this:

- `./a4 study ...`
- `./a4 run --integrate ...`

with plotting remaining an optional CLI add-on for both.

For compatibility and convenience, `./a4 int ...` can remain as a shorthand
alias for `./a4 run --integrate ...`.

## Why `OBSERVE` Is Procedural

Observation lists need to be usable:

- inside `METHOD`s
- inside loops
- after conditionals
- after changes in solver/integrator focus

So `OBSERVE` should be a METHOD statement, not just declarative metadata.

## Typed Reporting

Shared observed-instance machinery should eventually support:

- real
- boolean
- integer
- symbol / selector

CLI and file reporters can show all of these.
Plots should remain numeric-only by default.

The GUI integrator/observer widgets will need extending so that selector,
boolean, integer, and symbol observations can be shown sensibly even when
they are not plottable.

## Current Implementation Status

Implemented now:

- `OBSERVE var1, var2, ... [AS name];` is parsed and executed as a METHOD statement
- unnamed `OBSERVE` establishes a default observed-var list in the hook layer
- `STUDY` may omit its observed-var list and fall back to that default list
- `STUDY` now errors if neither explicit nor default observed vars exist
- `INTEGRATOR name;` is parsed and executed as a METHOD statement
- unqualified `OPTION ...;` now follows current focus:
  `SOLVER ...;` routes options to the solver, `INTEGRATOR ...;` routes options
  to the integrator
- `INTEGRATE FROM ... TO ... STEPS ...;` is parsed and executed as a METHOD statement
- `INTEGRATE` uses the current default `OBSERVE` list
- `INTEGRATE` now errors if no default observed vars exist
- the lower-level integrator API now has a typed observed-instance path
- CLI and hook-driven `STUDY` / `INTEGRATE` output support real, boolean,
  integer, symbol, and selector observations
- GTK `Observer` / integrator-reporter tables now display mixed typed observed
  values, with plotting restricted to real-valued columns

Current first-pass limitations:

- named observe-set selection by `STUDY` or `INTEGRATE`
- GTK `Observer` / integrator-reporter widgets now accept mixed real/boolean/
  integer/symbol/selector columns in their tabular output, but plotting remains
  limited to real-valued columns
- plotting remains numeric-only
- GUI/editor support for named observation sets

## Open Design Points

### Integrator Selection Syntax

Resolved for the first pass:

- use a dedicated `INTEGRATOR name;` METHOD statement

### Lifetime Of Options And Observe Sets

Still open:

- if a user selects integrator `A`, sets options, then selects integrator `B`,
  then returns to `A`, should `A`'s earlier options still be remembered?
- should named/default `OBSERVE` lists persist across solver/integrator focus
  changes within the same METHOD execution context?

Current working assumption:

- default and named `OBSERVE` lists should persist in the current simulation
  context
- solver/integrator-specific options should be stored per selected engine and
  restored when that engine is re-selected

## First Implementation Sequence

1. `OBSERVE` statement and default observe storage.
2. `STUDY` fallback to default observe list.
3. `INTEGRATOR` selection via slvreq-style hooks.
4. `INTEGRATE` execution request via slvreq-style hooks.
5. typed observation output for integrator reporters.
6. GUI/CLI exposure of named observe sets and dynamic reporting options.

The repository is now between steps 5 and 6:

- METHOD `OBSERVE`, `STUDY`, and `INTEGRATE` now accept/report real,
  boolean, integer, symbol, and selector observations in the hook-driven
  CLI/console path
- the lower-level integrator API now has a typed observed-instance path
  alongside the legacy `double *` observation API
- the GTK study/integrator viewers still need to be moved onto that typed
  path

# DER Design Notes

## Purpose

This note records the current design direction and implementation status for
dynamic derivatives in ASCEND.

The main goal is to support equation-level derivative syntax

$$
\mathrm{der}(x)
$$

as a first-class construct, while preserving compatibility for existing models
that still use `DER(...)` and legacy ODE metadata.

The note is intentionally compact. Superseded exploratory detail has been
removed.

## Current Status

The core first-order `der(x)` path is implemented and working for the tested
cases.

Implemented and passing:

- parser support for lower-case `der(fname)` in equations
- equation instantiation preserving `der(...)` as a distinct expression form
- analysis-side dynamic registry
- derivative pseudo-instances, exposed as dynamic pseudo-children such as
  `x.der`
- qlfdid support for both:
  - `der(model.part.var)`
  - `model.part.var.der`
- METHOD-time manipulation of derivatives through pseudo-instances
- browser and ascxx/Python instance-view access to derivative pseudo-children
- compatibility retention for legacy `DER(...)`
- QRSlv semantics for derivative pseudo-instances
- HiGHS regression coverage for derivative-containing linear models

Primary regression coverage:

- [test_der.c](./ascend/system/test/test_der.c)
- [alias_der_wLINK.a4c](./models/test/ida/alias_der_wLINK.a4c)
- [check_alias_der.py](./models/test/ida/check_alias_der.py)
- [test_lsode.c](./ascend/integrator/test/test_lsode.c)
- [test_ida.c](./ascend/integrator/test/test_ida.c)
- [test_highs.c](./ascend/solver/test/test_highs.c)

At the time of writing:

- `./a4 cutest system_der -v` passes
- existing LSODE and IDA regressions, including the new `INITIAL` startup
  cases, pass
- `tcltk` and `ascxx` compile successfully in this environment

## Design Decisions

### 1. Canonical language syntax is `der(x)`

The preferred user-facing derivative syntax is:

$$
\mathrm{der}(x)
$$

This is the form to use in equations and, where practical, in METHODs and
other user-facing references.

The tree-oriented path form

$$
x.\mathrm{der}
$$

is treated as an object-model alias for the same quantity, not as the primary
language surface.

For textual diagnostics and generated names, derivative pseudo-instances are
now rendered in the canonical form `der(x)` rather than `x.der`.

### 2. `DER(...)` remains compatibility syntax

Legacy syntax:

$$
\mathrm{DER}(\dot x, x)
$$

is still supported.

Its role is now compatibility and optional explicit materialisation of a user
declared derivative variable. It is not the preferred modern surface syntax.

### 3. Derivatives are exposed as pseudo-instances

The implementation no longer treats derivatives as solver-only hidden objects.
Instead, when needed, a derivative is materialised as a runtime pseudo-instance
associated with a base variable.

Example conceptual mapping:

$$
\mathrm{der}(y) \leftrightarrow y.\mathrm{der}
$$

Important constraints:

- derivative pseudo-instances are runtime-generated
- they are not ordinary declared source-language children
- they are visible through dynamic-child APIs, not through ordinary structural
  child traversal
- they are runtime-owned, not solver-owned; repeated solver/system rebuilds
  must reuse them rather than destroying them

### 4. Do not overload ordinary structural child traversal

Derivative pseudo-instances are intentionally **not** ordinary structural
children of the instance tree.

Current split:

- structural child APIs remain unchanged
- dynamic pseudo-children are accessed through dedicated APIs

This keeps compiler/system tree walking stable while still allowing browser and
interactive tooling to expose derivatives.

## Current Object Model

The public derivative API is currently centred around:

- [derivinst.h](./ascend/compiler/derivinst.h)

Main instance-centric entry points:

- `InstanceHasDerivative`
- `InstanceGetDerivative`
- `InstanceEnsureDerivative`
- `IsDerivativeInstance`
- `DerivativeInstanceBase`
- `DerivativeInstanceOrder`
- `DerivativeInstanceIndependent`

Dynamic-child view:

- `InstanceDynamicChildCount`
- `InstanceDynamicChild`
- `InstanceDynamicChildName`
- `InstanceDynamicChildByChar`
- `InstanceDynamicChildIndex`

This is the current "half-citizen" design:

- derivatives are real runtime objects
- they are visible to interactive tooling
- they are not yet ordinary structural children

## qlfdid and User Reference Forms

Current supported equivalent lookups include:

$$
\mathrm{der}(\mathrm{cell}.y)
$$

and

$$
\mathrm{cell}.y.\mathrm{der}
$$

The first form is the canonical user-facing derivative syntax.
The second form is the tree/object path alias.

The intent is:

- equations and user documentation prefer `der(model.part.var)`
- browser and object-path tooling may naturally expose `model.part.var.der`
- transitional METHOD-time attribute access currently uses the tree/object
  path form, eg `x.der.obs_id := 1`
- canonical attribute syntax such as `der(x).obs_id := 1` is not yet
  implemented

We do **not** currently intend to support a mixed form such as:

$$
\mathrm{model}.\mathrm{part}.\mathrm{der}(x)
$$

because it muddies the distinction between expression syntax and path syntax.

## METHOD and Interactive Semantics

### Current state

METHOD operations on derivatives now resolve directly to derivative
pseudo-instances during `Initialize(...)`.

That means:

- `FIX der(x)`
- `FREE der(x)`
- `der(x) := ...`

now act on a real runtime object at method time. The older pending-operation
mechanism has been removed.

This is materially better aligned with ASCEND METHOD semantics.

### Remaining caveat

The object model is improved, but not yet fully settled for all GUI actions.
The core browser/path plumbing now works, but some broader GUI interactions
have not yet been exercised end-to-end.

### Observation metadata

Derivative pseudo-instances now support METHOD-time metadata assignment
through the transitional tree-style syntax:

- `x.der.obs_id := 1`
- `x.der.ode_id := 2`
- `x.der.ode_type := 1`

This is sufficient for current integrator observation workflows.

The source of truth for what the integrator observes remains `obs_id` on the
runtime instances. Observer tabs are presentation state, not authoritative
observation-definition state.

## Solver Semantics

### IDA

For IDA, derivative pseudo-instances map naturally onto true dynamic
derivative quantities.

Conceptually:

- base state $x$ belongs to the state vector
- pseudo-instance $x.\mathrm{der}$ corresponds to the derivative slot for $x$

So for IDA, the pseudo-instance is not merely decorative. It is a user-visible
handle on a genuine solver quantity.

### QRSlv

QRSlv semantics are now implemented and tested for the current first-order
derivative model.

Current behavior:

- derivative pseudo-instances should appear to QRSlv as ordinary variables
- they should be fixed to zero by default
- if the user frees them, QRSlv should solve for them like any other free
  variable
- if the user fixes them to a nonzero value, equation evaluation should use
  that value

In other words, for QRSlv they behave like normal atom instances from the
solver's perspective, with one extra defaulting rule for untouched
derivatives.

The important implementation detail is:

- an untouched derivative pseudo-instance uses an algebraic default
- QRSlv treats that default as "fixed at zero"
- an explicit `FREE` clears that default
- a plain assignment to `der(x)` updates the pseudo-instance value without
  implicitly fixing it
- an explicit `FIX der(x) := value` records a fixed derivative value before any
  solver is loaded

Explicit mutation is now handled consistently across:

- METHOD-time `FIX`, `FREE`, and assignment
- ascxx/Python edits
- Tcl/Tk browser/unit-setting edits

### HiGHS

HiGHS now has explicit regression coverage for derivative-containing linear
models.

Current verified behavior:

- a model containing `der(x)` is eligible for HiGHS provided it also has an
  objective relation
- a trivial objective such as `MINIMIZE 0*x` is sufficient to satisfy that
  requirement
- with no explicit derivative mutation, `der(x)` follows the algebraic default
  and behaves as fixed-at-zero
- if `der(x)` is explicitly freed, it can participate as an ordinary LP
  variable

## Aliasing and Identity

Canonical derivative identity is not the same thing as the base variable
itself.

Important rule:

- `der(x)` is **not** in the same clique as `x`

Instead, derivative identity is inferred from:

- the canonical base variable identity
- derivative order
- the canonical independent variable

This is the basis for ensuring that if two variables are aliased or otherwise
merged, their derivatives resolve consistently as the same dynamic quantity.

## Derivative Chains and Named Derivatives

There is an important distinction between:

- an equation such as `v = der(x)`
- true identity / aliasing of `v` with the derivative quantity of `x`
- structural derivative-chain information for dynamic analysis

These are **not** the same thing.

### Current rule

Relations of the form

- `v = der(x)`
- `der(x) = v`

are currently treated as ordinary equations again.

They are **not** currently reinterpreted as:

- `ALIASES`
- `ARE_THE_SAME`
- hidden derivative-binding metadata

This is deliberate. The current design is that equations remain equations and
should not be silently co-opted into a different semantic role during system
analysis.

### What this means

- `v = der(x)` may still be a useful and natural modelling equation
- it does **not** currently make `v` the canonical runtime derivative
  identity of `x`
- it does **not** currently add `v` into a maintained derivative alias clique
- but simple equations such as `v = der(x)` may now be used by the advisory
  Pantelides pass as structural named-derivative representatives, without
  changing the runtime identity model

So at present:

- `der(x)` has a canonical runtime pseudo-instance identity
- `v` may be constrained equal to that quantity by equation
- but `v` is not thereby identical to `der(x)`
- and structural analysis may still choose to follow `v` as the named first
  derivative representative of `x` in limited cases

### Why this is a design concern

This matters for future work such as:

- Pantelides-style structural index analysis
- index reduction
- higher-order derivative chains
- consistent initialization of more difficult DAEs

If `v = der(x)` is treated only as an equation, then chain information may be
harder to recover structurally.

If `v = der(x)` is treated as hidden aliasing metadata, then equation semantics
are violated.

So the open problem is:

- how to preserve honest equation semantics
- while still detecting useful derivative-chain structure for structural DAE
  algorithms

That is now an explicit next-phase design issue.

### Current conservative position

- keep `v = der(x)` as an equation
- do not currently allow true aliasing such as `v ALIASES der(x)`
- keep higher-order intent explicit as `der(der(x))`, `der(der(der(x)))`, etc

This avoids ambiguity in current first-order IDA/LSODE handling, especially
around accidentally treating a named derivative alias as an ordinary state.

The long-term solution may involve either:

- explicit structural inference from equations like `v = der(x)` without
  removing them from the active problem, or
- a distinct explicit derivative-identity syntax separate from equation
  equality

That design has not yet been settled.

## Pantelides Reference Cases

Two classic reference problems are now recorded under
[models/test/pantelides](./models/test/pantelides):

- [reactor.a4c](./models/test/pantelides/reactor.a4c)
- [pendulum.a4c](./models/test/pantelides/pendulum.a4c)

These began as discussion/reference models and are now also used as explicit
high-index / non-reduced structural regression cases in:

- [test_lsode.c](./ascend/integrator/test/test_lsode.c)
- [test_ida.c](./ascend/integrator/test/test_ida.c)

Current expected behavior is rejection during analysis, with user-facing
messages indicating that the models are not in first-order ODE/DAE form and
that index reduction may be required.

They are important because they make the current derivative-chain design issue
concrete:

- the models use canonical `der(...)` equations directly, such as
  `vx = der(x)` and `der(C) = ...`
- those equations should remain equations
- but Pantelides-style structural analysis may still need to infer derivative
  chain structure from equations of the form `v = der(x)` when users choose to
  introduce named first-derivative variables

So these examples strengthen the current conclusion:

- `v = der(x)` should not be silently reinterpreted as aliasing or hidden
  metadata
- but future structural analysis may still need to infer chain structure from
  such equations, without removing them from the active problem

The reactor example is now written in a more ASCEND-style physical form with
dimensioned temperatures, molar densities, and rate constants, but it is still
intended as a structural reference case rather than a calibrated reactor model.

These two models are the current reference starting point for future work on:

- structural index detection
- Pantelides algorithm design
- index reduction in the presence of `der(...)`
- understanding how `INITIAL` equations should participate in higher-index
  dynamic problems

There is now also a first-pass advisory Pantelides reporter in
[pantelides.c](./ascend/integrator/pantelides.c), exposed through
`integrator_pantelides_advisory(...)` and now also wrapped at system stage
through `Simulation.getPantelidesReport()` for ascxx/Python/GUI use. This
analysis is solver-neutral and read-only:

- it works from the active `slv_system_t`
- it uses the current `diffvars` view plus active solver relations
- it reports the derivative chains and equations that ASCEND currently sees
- it does **not** yet create symbolic differentiated equations or mutate the
  working problem
- it can be captured to a file or string at the C layer and is now available
  to Python scripts and the GTK browser

That advisory pass now gives two distinct useful results on the reference
models:

- for `pendulum.a4c`, it now infers the named derivative representatives
  `vx = der(x)` and `vy = der(y)` and advises differentiating the holonomic
  constraint `eq5` twice
- for `reactor.a4c`, it no longer hangs; instead it stops with an explicit
  advisory-analysis limit note, which is a safer and more honest failure mode
  for the current prototype

This sharpens the chain-inference use-case. The real structural question is not
whether `der(x)` is a derivative of `x` (that is already explicit), but whether
an ordinary variable such as `vx` should be recognised as the named
representative of that derivative quantity for structural analysis purposes.

The current concrete gap is therefore:

- if a future structural algorithm such as Pantelides needs to follow chains
  through named derivative variables, it will need some structural notion of
  `v` being the first derivative representative of `x`
- that structural notion should not require treating `v = der(x)` as aliasing
  or deleting the equation from the active system

The advisory pass now has enough structural information to make progress on the
pendulum example, but it still lacks:

- symbolic differentiated-relation generation
- stronger stopping rules / reformulation logic for cases like the reactor
- a broader structural treatment of named derivative representatives beyond the
  simplest `v = der(x)` form

Those are the next steps before automatic index reduction can be attempted.

## Compatibility and Transition

The transition policy is:

1. keep legacy `DER(...)` working
2. prefer `der(x)` in new models
3. keep old ODE metadata working as compatibility input
4. avoid a flag-day rewrite of old models

So:

- compatibility is preserved
- deprecation, not immediate removal, is the current plan

## Hybrid / Event Scope

The current `der(x)` work is primarily about smooth ODE/DAE support.

Future hybrid/event support will also need:

- `INITIAL`
- `WHEN` / `CONDITIONAL`
- `REINIT`
- `pre(x)`

The current derivative design should leave room for those features, but it does
not complete them.

Conservative first-phase assumptions remain sensible:

- `WHEN` may change active equations
- `WHEN` should not yet change the canonical differential state set
- guards should not depend on `der(...)` initially

### Proposed Hybrid Surface Syntax

The most practical path is to stage hybrid syntax in two layers:

1. add the missing hybrid semantics on top of the existing backend
2. add a lighter surface syntax afterward as sugar

That keeps the current `CONDITIONAL` / `SATISFIED(...)` / `WHEN ... CASE`
machinery as the execution model, while giving users something less awkward to
write.

#### Phase 1: minimal semantics-first extension

Add:

- `pre(x)` as an event-time left-limit operator
- `REINIT(x, expr);` as a new statement allowed inside a `WHEN` case body

Example:

```ascend
CONDITIONAL
    boundary: y <= r;
END CONDITIONAL;

switch == SATISFIED(boundary, 1e-8{m});

WHEN(switch)
    CASE TRUE:
        REINIT(v, -e * pre(v));
        USE bouncing;
    CASE FALSE:
        USE airborn;
END WHEN;
```

This is intentionally close to the existing implementation model:

- the guard still comes from a conditional relation
- the boolean selector still comes from `SATISFIED(...)`
- the active equation set still switches through `USE ...`
- only the missing event-memory and state-reset semantics are added

This is the lowest-risk route because current `WHEN` checking already exists,
and only needs to be widened to allow `REINIT(...)` in addition to `USE`,
nested `WHEN`, and `FOR`.

#### `REINIT` spelling

For ASCEND, the best spelling is:

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
- `REINIT(x, expr)` avoids pretending that a reset is either a procedural
  assignment or an always-active algebraic equation
- the function-style spelling matches the intended event-time semantics and
  keeps parsing simple

`REINIT` should therefore be treated as its own statement form, not as
syntactic sugar for assignment.

#### Declarative meaning

Even if the syntax later becomes `WHEN ... DO`, the meaning should stay
declarative.

A `REINIT` is not "execute this assignment in source order". It is a
reinitialisation condition that becomes active when an event fires. The event
handler should:

1. detect a guard crossing
2. determine the active branch
3. form the post-event system
4. apply the `REINIT(...)` conditions
5. solve for a consistent restarted state

That keeps the language aligned with ASCEND's equation-based semantics and
avoids making event behavior depend on statement ordering.

#### Phase 2: user-facing sugar

Once `pre(x)` and `REINIT(...)` exist, a lighter surface form can be added and
lowered to the same backend:

```ascend
WHEN y <= r DO
    REINIT(v, -e * pre(v));
    USE bouncing;
ELSE
    USE airborn;
END WHEN;
```

This should be treated as sugar for:

- an implicit conditional relation
- an implicit `SATISFIED(...)` selector
- the existing `WHEN (...) CASE ... END WHEN` machinery

So `WHEN ... DO` is not the semantic foundation. It is only a better front
end.

#### Sensible v1 restrictions

For a first implementation, the following limits are sensible:

- `REINIT(...)` is only legal inside `WHEN` cases
- Phase 1A should restrict the `REINIT` target to continuous real
  differential/integrator states only
- event-memory variables such as `t_last_event` are important, but should come
  later as a separate extension rather than being mixed into the first working
  reset implementation
- `pre(x)` should parse as a normal expression operator, but in Phase 1A it is
  only semantically legal inside `REINIT(...)`
- outside active event/reinitialisation processing, `pre(x)` should not be
  treated as having a meaningful runtime value
- Phase 1A semantics should be defined solver-neutrally as "apply explicit
  post-event values to selected continuous states, then compute a consistent
  restart". For IDA this is expected to map onto the existing restart /
  `IDACalcIC` path, effectively with `YA_YDP`-style behavior for the reset
  states.
- the event/reset data should be lowered onto solver-side `when_case`
  structures in `slv_system_t`, analogous to existing `WHEN`/boundary
  lowering, rather than having solver engines reach back into compiler
  `Statement` trees at runtime
- in Phase 1A, IDA is the intended execution path for `REINIT`; LSODE should
  reject models that require `WHEN`/boundary event handling, and CMSlv may
  safely ignore `REINIT` actions since they are not part of its algebraic
  conditional solve semantics
- guards should remain free of `der(...)` in v1
- branch switching may change active equations, but should not change the
  canonical differential state set

For the later event-memory extension, it is preferable not to create a third
parallel family of real atom types. A better direction is likely to be:

- keep the existing `IS_A` variable declarations
- add an extra classification flag on real atom instances for
  discrete/event-memory behavior
- filter such flagged variables out of continuous solver-variable/state lists
  while still allowing them to store values between events

This keeps the implementation closer to the existing `solver_var` machinery and
avoids duplicating every measure/type into a new discrete-real family.

Phase 1B now follows that direction in a minimal way:

- `solver_var` carries a `discrete` boolean child, currently intended to be set
  from methods such as `on_load`
- discrete real variables are treated as fixed between events for solver
  purposes
- they are excluded from the continuous integrator state vectors
- they may nevertheless be targeted by `REINIT(...)` so they can act as simple
  real-valued event memory
- IDA now carries a regression exercising repeated event-memory updates with a
  lengthening-period sawtooth, which depends on reinitialising IDA rootfinding
  state after each event restart
- simultaneous boundary crossings from different sources now need to be
  treated as one combined logical event, not silently truncated to the first
  crossed boundary reported by IDA
- the current implementation now combines simultaneous crossings when they all
  imply the same target truth value, but still rejects mixed TRUE/FALSE target
  sets because LRSlv's current perturb interface only carries one target truth
  mode for the whole solve

There are now also small documented example models for the current feature set
under `models/johnpye/dyn`:

- `ideal_rebound.a4c` shows a clean "bounce without contact kinetics" using
  `REINIT(v, -e * pre(v));`
- `lengthening_sawtooth.a4c` shows repeated resets plus discrete real
  event-memory updates

Phase 1C now extends this one step further for discrete state:

- `REINIT(...)` may also target boolean discrete variables
- this is enough to support simple latched boolean event memory and
  state-selection booleans in some cases
- the implementation should update both the atom instance and the solver-side
  `dis_discrete` record so that LRSlv / event iteration sees the new value
  immediately
- a small regression now exercises this with a boundary-triggered boolean
  latch in `models/test/ida/reinit_bool.a4c`

There is still an important limitation to keep in mind:

- boolean `REINIT(...)` is now workable when the follow-on `WHEN` structure is
  either unchanged or structurally equivalent
- but if that freshly reassigned boolean immediately drives a second
  structurally different `WHEN` reconfiguration, the current IDA restart path
  can still become non-square
- that is therefore a real follow-on problem for later hybrid/state-machine
  work, not something Phase 1C fully solves by itself

The current IDA event handling is now one step closer to the intended hybrid
semantics:

- event handling is no longer just "apply one batch of `REINIT(...)` then
  restart"
- instead, IDA now performs a same-time event-iteration loop:
  - settle logical/discrete configuration with LRSlv
  - apply any newly active `REINIT(...)` actions that have not yet fired in the
    current event
  - perform one same-time consistency restart
  - repeat until no new discrete changes or newly active `REINIT(...)` actions
    remain
- within this loop, `pre(x)` now advances with each event iteration rather than
  staying frozen at the value from immediately before the first event action

That "advancing `pre(x)`" choice is deliberate. It means:

- most event iterations still see `x = pre(x)`
- but if an earlier event action in the same cascade changes `x`, a later
  `REINIT(...)` in that same cascade will see the updated value through
  `pre(x)`

A regression for this now exists in `models/test/ida/reinit_bool_cascade.a4c`:

- one `WHEN` increments a discrete real stage counter and latches a boolean
- a second `WHEN`, activated only after that latch, increments the stage
  counter again
- the final stage value of `2` confirms that `pre(stage)` advanced between the
  first and second event iterations

There is also one important practical detail in the IDA restart path:

- the post-event root-direction suppression had to be corrected to match the
  actual semantics of `IDASetRootDirection`
- otherwise a rebound-style model could immediately fire a second event on the
  way back out of the boundary and undo the intended `REINIT(...)`

These examples are intended to be readable rather than merely minimal solver
regressions, and they are also covered by IDA tests so they should remain
executable.

Modelica and gPROMS both keep a semantic distinction here:

- Modelica `reinit(x, expr)` is for continuous `Real` states, while ordinary
  discrete/event-memory variables are updated directly in `when` equations
- gPROMS `REINITIAL ... WITH ...` is likewise aimed at differential variables,
  while other discontinuous value changes are handled through separate
  mechanisms such as `REASSIGN`

For ASCEND Phase 1B it is still reasonable to keep a single `REINIT(...)`
surface form for both continuous-state resets and discrete real event-memory
updates, provided the backend continues to distinguish those two target classes.
This keeps the first implementation small, while leaving open the option of a
cleaner split in surface syntax later if it proves worthwhile.

Expression-level event generation, more in the style of Modelica, may also be
desirable later. However, that should be treated as a future event-source layer
above the same backend semantics; it is not required for Phase 1A.

#### Phase 2 plan: gPROMS-like state transitions

The next major step should be to move from "event actions inside existing
`WHEN` cases" toward an explicit state-transition notation, while still
lowering onto the current `CONDITIONAL` / `SATISFIED(...)` / `WHEN` backend.

The main missing capability is not real-valued reset any more. Phase 1B
already extended `REINIT(...)` so it can update:

- continuous real states
- discrete real event-memory variables

What is still missing is true `REASSIGN`-like behavior for discrete non-real
state:

- boolean mode/state variables
- integer selector variables
- symbolic / enumerated selector variables

That is why Phase 2 still needs something beyond the current widened
`REINIT(...)`. We do not yet have a clean way to say "the active state becomes
`running` now" unless that state can be encoded indirectly through existing
condition booleans.

The recommended Phase 2 semantic target is:

- explicit selector/mode memory
- state-local equations
- state-local outgoing transitions
- optional transition actions (`REINIT`, later `REASSIGN`)
- post-transition event iteration until the discrete configuration is stable
- explicit priority / exclusivity rules for multiple enabled outgoing
  transitions

A syntax direction closer to gPROMS would be appropriate, for example:

```ascend
SELECTOR mode IS_A symbol OF [off, running];

CASE mode OF
    WHEN off DO
        USE off_equations;
        TRANSITION TO running IF cooldown_ok AND storage_ok;
    END WHEN;

    WHEN running DO
        USE running_equations;
        TRANSITION TO off IF shutdown_cmd;
    END WHEN;
END CASE;
```

The important point is not the exact spelling, but that the source state, the
guard, and the target state appear together in one place. That is the key
readability advantage of the gPROMS style.

ASCEND already has existing `SELECT` and `SWITCH` statements in the grammar,
so Phase 2 should copy the gPROMS structural idea more than the exact keyword
spelling. The surface syntax may therefore need to use `STATE` /
`TRANSITION`, or some other non-conflicting form, even if the underlying
semantics are very similar to gPROMS `SELECTOR` / `CASE` / `SWITCH TO IF`.

The existing backend can still do most of the heavy lifting:

- transition guards lower to `CONDITIONAL` relations / logrelations
- guard truth values lower through `SATISFIED(...)` or logical relations
- active-state equation selection lowers to current `WHEN` / `CASE`
- transition actions lower to solver-side action lists attached to the active
  case

What Phase 2 needs in addition is a first-class selector variable and
transition-action model.

##### User-interface / instance-tree implications

If Phase 2 is only lowered silently to today's `WHEN` / `CASE` nodes, the GTK
instance tree will become hard to read. The user will see a forest of boundary
booleans and cases, but not the higher-level state machine they wrote.

So the compiler-side representation should preserve state/transition structure
for the UI, even if the solver backend still receives lowered `WHEN` data.

A sensible tree shape would be:

- selector node
- state nodes beneath the selector
- transition nodes beneath each state
- under each transition:
  - guard
  - target state
  - action list (`REINIT`, later `REASSIGN`)

This could be done either with new instance kinds or with synthetic child nodes
attached to an enriched `WHEN` instance. The important requirement is that the
browser view should show the source-state-local transitions directly.

##### Example-driven requirements

The following examples are good anchors for Phase 2 semantics:

- overflowing weir:
  two-mode structure (`below_weir` / `overflowing`), mostly equation
  switching, little or no reset logic
- bursting disc:
  one-way transition (`intact -> burst`) that never returns; requires explicit
  mode memory
- safety relief valve:
  open / closed transitions with hysteresis and possible priorities if several
  valve conditions are present
- adaptive cruise control:
  multiple control modes, mode-local equations, and source-state-dependent
  transition availability
- bouncing ball:
  transition action semantics (`REINIT(v, -e * pre(v))`) plus post-event
  settling

Together these examples imply that Phase 2 must support:

- reversible transitions
- one-shot transitions
- hysteresis
- state-conditioned guards
- transition-local actions
- multiple outgoing transitions from the same state
- explicit conflict resolution when several outgoing transitions are enabled

##### Concrete implementation slices

To keep Phase 2 reviewable and incremental, it should probably be split into
small semantic slices rather than attempted as one parser rewrite:

- Phase 2A: first-class selector / state syntax in the compiler
  - add source-level state / transition syntax
  - preserve that structure in the compiler representation for UI display
  - lower it to today's `WHEN` / `CASE` machinery for execution
- Phase 2B: explicit transition actions for discrete non-real state
  - add `REASSIGN`-like actions for boolean, integer, and symbolic selector
    targets
  - keep `REINIT(...)` for continuous-state resets and real-valued event
    memory
- Phase 2C: runtime transition semantics
  - define transition priority / exclusivity
  - define event iteration when a transition action enables another transition
  - define one-shot vs reversible transition behavior
- Phase 2D: UI / instance-tree presentation
  - show selectors, states, transitions, guards, and actions directly in the
    browser tree
  - avoid exposing only the lowered forest of `SATISFIED(...)` booleans and
    backend `WHEN` nodes

The recommended implementation order is:

1. preserve a high-level state/transition structure in the compiler and UI
2. lower that structure onto existing solver-side `WHEN` / `CASE` support
3. add discrete non-real transition actions
4. then refine priority and event-iteration rules once the basic models are
   executable

##### Why widened `REINIT(...)` is not enough

Phase 1B intentionally stretched `REINIT(...)` further than either Modelica or
gPROMS by also allowing discrete real event-memory targets.

That still does **not** make it a full replacement for `REASSIGN`, because
Phase 1B does not yet cover:

- boolean state updates
- integer selector updates
- symbolic / enumerated mode updates
- general discrete assignment semantics outside real-valued storage

So the missing Phase 2 capability is not "resetting something discontinuously"
in the abstract. The missing part is "changing named discrete state in a way
that the source language can express directly and the UI can display
intelligibly".

##### What current syntax still cannot cover cleanly

Current Phase 1 syntax can express many present-time conditions, but it still
cannot cleanly express:

- rising-edge detection on a calculated boolean, e.g.
  `ready AND NOT pre(ready)`
- a latched mode transition such as
  "if state is `off` and cooldown has expired and storage is sufficient, move
  to `running` and stay there until a different stop transition fires"
- symbolic selector updates such as `mode := running`
- transition priorities between several enabled target states from the same
  source state

Those are exactly the gaps Phase 2 should close.

## `INITIAL`

`INITIAL` is a declarative section inside `MODEL`, positioned between the main
declarative statement list and the `METHODS` section:

```ascend
MODEL foo;
    ...
    r1: der(x) = -x;

INITIAL
    x = 1;
    der(x) = 0;

METHODS
    METHOD on_load;
        ...
    END on_load;
END foo;
```

It is intentionally **not** a METHOD. It is a second declarative equation
section.

### Semantics

`INITIAL` equations are additional equations that hold only in the
initialization problem at

$$
t = t_0
$$

and are not included in the normal simulation problem for

$$
t > t_0.
$$

The intended initialization system is therefore:

$$
\text{normal model equations at } t_0
\;+\;
\text{INITIAL equations}
$$

This follows the same broad semantic direction as Modelica
`initial equation` and gPROMS `INITIAL`: initialization is a separate
declarative equation set, not imperative setup code.

`METHODS` and `INITIAL` should have distinct roles.

`INITIAL` is for:

- equations that are true only at initialization
- initial algebraic constraints
- steady-state initialization constraints such as `der(x) = 0`

`METHODS` remain for:

- guesses
- bounds
- `FIX/FREE`
- solver/integrator selection
- orchestration such as `SOLVE`

The user should not need to emulate `INITIAL` by writing METHOD code that
manually toggles relation `included` flags. Internally, ASCEND may still use
the existing `included` machinery to realise the initialization split, but
that remains an implementation detail.

For v1, `INITIAL` is deliberately narrow.

Allowed:

- relation statements
- logical relation statements if needed for initialization consistency
- `FOR`
- parameter-structural `IF`

Not yet allowed:

- `RUN`
- `FIX`
- `FREE`
- `SOLVE`
- `STUDY`
- `OPTION`
- `WHEN`
- `CONDITIONAL`

That keeps `INITIAL` as an equation section, not a second procedural language.

During initialization, `der(x)` should be usable as an ordinary algebraic
quantity in equations.

Examples:

```ascend
INITIAL
    x = 1;
    der(x) = 0;
```

or

```ascend
INITIAL
    x + y = 3;
    der(x) + der(y) = 0;
```

That is the key behavior needed for steady-state initialization and consistent
DAE startup.

`INITIAL` composes hierarchically in the same way as ordinary model equations:

- a child model's `INITIAL` equations belong wherever that child is
  instantiated
- a parent may add cross-component initialization constraints in its own
  `INITIAL` section
- build mode determines whether instantiated `INITIAL` relations are included

This is preferable to a flat, simulation-root-only interpretation.

### Implementation status

Implemented so far:

- `INITIAL` parses as a distinct model section
- `TypeDescription` stores a separate `initstats` list
- statements in `INITIAL` carry `context_INITIAL`
- child-list derivation sees names declared in both the normal body and
  `INITIAL`
- executable statement traversal for model instantiation runs over both normal
  statements and `INITIAL`
- relation/logrelation definitions now carry an `initial` boolean child
- relations/logrelations instantiated from `INITIAL` are marked with
  `initial := TRUE`
- those equations default to `included := FALSE`
- `SetInitialRelationInclusion(root, active)` toggles all initial equations in
  a tree
- this already composes through hierarchy
- `SYSTEM_BUILD_NORMAL` and `SYSTEM_BUILD_INITIAL` now exist

The v1 semantic fence is also in place:

- `INITIAL` accepts only equation-building declarative constructs
- non-equation body constructs in `INITIAL` are rejected during typelint

### Out of scope for v1

The following should be treated as later work:

- `initial algorithm`-style procedural initialization
- `pre(x)`
- `REINIT`
- event-triggered reinitialization
- changing the differential state set during initialization

Those features belong to the broader hybrid/event roadmap, not to the minimal
equation-based `INITIAL` section.

### Integrator architecture

The correct architectural split is now:

- `integrator_analyse` remains structural and solver-independent
- startup initialization, if any, is engine-owned and occurs from
  `integrator_solve` via an optional `initialisefn` hook

Integrator families are currently split as follows:

- LSODE, DOPRI5, and RADAU5 share the generic ODE analysis path and already
  depend on an algebraic solver during stepping
- IDA has its own analyse function and its own IC machinery

So:

- LSODE/DOPRI5/RADAU5 share one ODE-family initialization hook
- IDA has a separate initialization hook in its own engine module

This replaced an earlier attempt to run a QRSlv-based initialization solve
from `integrator_analyse`, which was the wrong layer.

### Current status

The startup path now works for the new focused regressions:

- [deriv.a4c](./models/test/lsode/deriv.a4c) `initial_decay`
- [initial.a4c](./models/test/ida/initial.a4c) `ida_initial_decay`
- hierarchical startup cases in both LSODE and IDA
- mixed differential/algebraic `INITIAL` cases in IDA
- overdetermined/conflicting `INITIAL` startup cases now fail early with a
  clear user-facing initialization message

The key fixes were:

- derivative pseudo-instances are no longer destroyed during system teardown
- repeated system builds rebuild dynamic derivative metadata from already bound
  relations
- integrator startup reanalysis goes back through full
  `integrator_analyse(...)`, not only the engine-specific analyse function
- IDA startup/debug output has been cleaned up so developer trace uses `MSG`
  and end-user diagnostics remain on `error_reporter`
- the unsupported IDA `minstep` option now reports once per integrator
  instance, rather than on every internal reinitialisation
- temporary initialization-mode presolve now suppresses generic DOF/rank
  chatter and relies on one higher-level user-facing message such as
  `Initialization problem is not square...`
- bad-startup integrator tests now clean up without leaving process-exit memory
  leaks

So the remaining work is no longer basic startup correctness. It is refinement
around explicit initialization-mode use outside automatic integrator startup.

### QRSlv-based initialization exploration

For user-facing guidance, it is important to state clearly that QRSlv is not
being used here to "run the IVP". It is only being used to solve the
initialization problem at

$$
t = t_0
$$

for inspection and debugging.

The intended user model is:

1. write the dynamic model using normal equations plus `INITIAL`
2. use IDA or LSODE for actual time integration
3. if startup is troublesome, temporarily switch to QRSlv to solve the
   initialization problem explicitly
4. inspect and adjust startup values, derivative values, and FIX/FREE choices
5. switch back to IDA or LSODE for the transient run

So QRSlv should be documented as a startup-analysis tool in this context, not
as an alternative transient integrator.

More broadly, this sits inside a more important model-reuse goal:

- the same dynamic component model should behave sensibly in steady-state
  algebraic solves
- the same model should support explicit initialization solving
- the same model should support transient integration

That means the overall derivative/`INITIAL` design should be understood in
terms of three distinct problem modes, not just "QRSlv versus IDA".

That implies three distinct problem modes:

- normal algebraic solve
  - `INITIAL` equations are excluded
  - untouched `der(x)` behaves as zero, so the model can still be used
    rationally in a steady-state context
- initialization-mode algebraic solve
  - normal equations at `t0` plus `INITIAL`
- time integration
  - IDA or LSODE, possibly using an internal initialization solve first

Typical examples are:

- steady-state reuse of a dynamic model
  - leave normal mode active
  - untouched `der(x)` defaults to zero
- non-steady algebraic operating point
  - explicitly fix a derivative to a nonzero value, for example

```ascend
METHOD operating_point;
    SOLVER QRSlv;
    FIX der(E_stored) := 2 {kW};
    SOLVE;
END operating_point;
```

- startup solve with `INITIAL`
  - enter initialization mode, solve, inspect, then leave initialization mode

The wording in APIs and UI should reflect this. Prefer:

- "solve initialization problem"
- "enter initialization mode"
- "leave initialization mode"

and avoid wording like:

- "solve the DAE with QRSlv"

because it invites the wrong mental model.

This three-mode framing is important because it is what allows a user to write
one model and reuse it for:

- steady-state studies
- startup/IVP consistency analysis
- transient simulation

without maintaining separate model variants.

For the explicit QRSlv exploration path, the minimum practical workflow is:

- enter initialization mode
- rebuild the solver system
- solve with the currently selected algebraic solver
- inspect values and residuals while still in initialization mode
- leave initialization mode and rebuild the normal system when finished

The important usability point is that after a successful startup solve, the
user should normally remain in initialization mode long enough to inspect the
startup problem. A one-shot "solve and immediately switch back" workflow is
less useful for debugging.

Current implementation support is still partial here:

- the low-level build-mode machinery exists
- startup initialization through integrators exists
- explicit user-facing initialization-mode control now exists via built-in
  wrapper METHODS:
  - `enter_initial_mode`
  - `leave_initial_mode`
- these are implemented in `basemodel.a4l` using built-in external methods:
  - `set_initial_mode_on`
  - `set_initial_mode_off`
- those operations:
  - flip instantiated `INITIAL` relation inclusion on the current simulation
  - mark the simulation dirty
  - invalidate any current solver system so the next `SOLVE` rebuilds in the
    new mode

These helper methods are available in normal library usage because
`basemodel.a4l` augments the global `MODEL` definition using
`ADD METHODS IN DEFINITION MODEL;`.

This is important:

- models do **not** implicitly `REFINE basemodel`
- the helper methods are available once `basemodel.a4l` has been loaded
  because it mutates the base `MODEL` definition itself

So the current explicit workflow is:

```ascend
METHOD inspect_startup;
    SOLVER QRSlv;
    RUN enter_initial_mode;
    SOLVE;
    (* inspect/fix/free values *)
    RUN leave_initial_mode;
END inspect_startup;
```

The non-steady operating-point workflow in normal algebraic mode is equally
explicit:

```ascend
METHOD operating_point;
    SOLVER QRSlv;
    FIX der(x) := 5;
    SOLVE;
END operating_point;
```

That keeps `INITIAL` excluded while overriding the usual steady-state default
for the selected derivative quantity.

There is still no built-in one-shot `solve_initial` helper. That remains an
open UX question, but the explicit mode-switch workflow is now implemented and
tested.

## Python / Object-View Support

Current Python-facing access now includes:

- `inst.der`
- `ascpy.der(inst)`
- `sim.getPantelidesReport()`

These both resolve to the same derivative pseudo-instance.

This is useful because it keeps:

- a tree/object form: `inst.der`
- a language-like form: `ascpy.der(inst)`
- a scriptable system-stage Pantelides text report: `sim.getPantelidesReport()`

without introducing extra spelling variants.

## Current Squishy Bits

- `INITIAL` explicit solve workflow
  - focused integrator startup regressions now pass, including obvious
    overdetermined failure cases
  - broader semantics still need to be hardened for explicit
    initialization-mode solves outside the integrator path
- full GUI semantics
  - browser/object path is working
  - Pantelides advisory text is now exposed in the GTK browser
  - broader end-to-end GUI exercise is still useful
- hybrid/event semantics
  - `WHEN`, `pre(x)`, and `REINIT` are still future work
- higher derivatives
  - the APIs expose derivative order
  - practical implementation is still first-order only

## Recommended Next Steps

Near term:

1. continue documenting and testing explicit steady-state versus
   initialization-mode workflows with small example models
2. continue broadening `INITIAL` coverage only where it adds semantic value,
   not just more variants of already-covered startup cases
3. move on to `pre(x)` / `REINIT` / hybrid-event semantics once the example
   workflows feel stable

After that:

4. define `pre(x)` and `REINIT` semantics
5. expand hybrid/event support on top of the current derivative model

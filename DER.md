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

So the remaining work is no longer basic startup correctness. It is refinement
and cleanup around the new initialization mode.

## Python / Object-View Support

Current Python-facing access now includes:

- `inst.der`
- `ascpy.der(inst)`

These both resolve to the same derivative pseudo-instance.

This is useful because it keeps:

- a tree/object form: `inst.der`
- a language-like form: `ascpy.der(inst)`

without introducing extra spelling variants.

## Current Squishy Bits

- `INITIAL` explicit solve workflow
  - focused integrator startup regressions now pass
  - broader semantics still need to be hardened, especially around future
    explicit initialization-mode solves outside the integrator path
- full GUI semantics
  - browser/object path is working
  - broader end-to-end GUI exercise is still useful
- hybrid/event semantics
  - `WHEN`, `pre(x)`, and `REINIT` are still future work
- higher derivatives
  - the APIs expose derivative order
  - practical implementation is still first-order only

## Recommended Next Steps

Near term:

1. decide whether to expose convenience `METHOD`s for advanced QRSlv-based
   initialization exploration
2. exercise more hierarchical and multi-state `INITIAL` examples
3. clarify how non-integrator initialization-mode solves should be surfaced

After that:

4. define `pre(x)` and `REINIT` semantics
5. expand hybrid/event support on top of the current derivative model

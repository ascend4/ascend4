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

The core `der(x)` path is implemented and working for the tested cases.

Implemented:

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

Test coverage:

- [test_der.c](./ascend/system/test/test_der.c)
- [alias_der_wLINK.a4c](./models/test/ida/alias_der_wLINK.a4c)
- [check_alias_der.py](./models/test/ida/check_alias_der.py)

Current verified cases include:

- direct, nested, array, and implicit `der(...)` equations
- scalar `ALIASES`
- array `ALIASES`
- array `ARE_THE_SAME`
- METHOD-time `FIX`, `FREE`, and assignment on `der(x)`
- qlfdid resolution of both `der(x)` and `x.der`
- browser/ascxx exposure of dynamic pseudo-children

At the time of writing:

- `./a4 cutest system_der -v` passes
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

QRSlv semantics are the next concrete area to finish.

The intended direction is:

- derivative pseudo-instances should appear to QRSlv as ordinary variables
- they should be fixed to zero by default
- if the user frees them, QRSlv should solve for them like any other free
  variable
- if the user fixes them to a nonzero value, equation evaluation should use
  that value

In other words, for QRSlv they should behave like normal atom instances from
the solver's perspective.

This is a system-analysis problem, not a parsing problem.

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

## Python / Object-View Support

Current Python-facing access now includes:

- `inst.der`
- `ascpy.der(inst)`

These both resolve to the same derivative pseudo-instance.

This is useful because it keeps:

- a tree/object form: `inst.der`
- a language-like form: `ascpy.der(inst)`

without introducing extra spelling variants.

## What Is Solid Now

These parts now look like the right foundation:

- `der(x)` as the modern equation-level syntax
- compatibility retention for `DER(...)`
- analysis-side dynamic registry
- derivative pseudo-instances as runtime objects
- separate dynamic-child API instead of modifying ordinary structural child
  traversal
- qlfdid support for both canonical and tree-path derivative references
- METHOD-time direct manipulation of derivative pseudo-instances
- browser and ascxx exposure of derivative pseudo-children

## Current Squishy Bits

The main unresolved areas are now narrower.

### 1. Full GUI semantics

The browser/object path is working, but broader GUI editing/action paths still
need deliberate exercise against derivative pseudo-instances.

### 2. QRSlv semantics

The intended QRSlv behavior is clear, but the full system-analysis path still
needs to be implemented and tested.

### 3. Hybrid/event semantics

The derivative design is compatible with later work on events, but that work
has not yet been done.

### 4. Higher derivatives

The APIs expose derivative order, but the practical implementation focus is
still first-order derivatives.

## Recommended Next Steps

Near term:

1. complete QRSlv semantics for derivative pseudo-instances
2. exercise browser/GUI mutation paths against derivative pseudo-instances
3. keep `system_der` coverage growing as behavior is clarified

After that:

4. define `INITIAL`, `pre(x)`, and `REINIT` semantics
5. expand hybrid/event support on top of the current derivative model

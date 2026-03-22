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

## Proposed `INITIAL` Semantics

### Scope and placement

The proposed ASCEND syntax is a new declarative section inside `MODEL`,
positioned between the main declarative statement list and the `METHODS`
section:

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

This is intentionally **not** a METHOD. It is a second declarative equation
section.

### Core meaning

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

### Relationship with `METHODS`

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

So the user should not need to emulate `INITIAL` by writing METHOD code that
manually toggles relation `included` flags.

Internally, ASCEND may still use the existing `included` machinery to realise
the initialization/non-initialization split, but that should remain an
implementation detail.

### First implementation limits

For a first implementation, `INITIAL` should be deliberately narrow.

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

### Interaction with `der(...)`

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

### Implementation shape

The current compiler/runtime layout suggests a clean implementation path:

1. extend the grammar so `MODEL` contains:
   - main declarative statements
   - optional `INITIAL` statement list
   - optional `METHODS`
2. add a second declarative statement-list slot to `TypeDescription`
3. instantiate `INITIAL` statements as relation instances flagged as
   initialization-only
4. build an initialization-mode system that includes:
   - normal equations
   - `INITIAL` equations
5. keep ordinary system builds excluding the `INITIAL` equations

This is preferable to encoding initialization purely through METHOD-side edits
to `included`.

### Parser strategy: avoid duplicating equation grammar

The preferred parser strategy is to **reuse the existing declarative statement
grammar** rather than clone a parallel "initial equation" grammar.

Recommended approach:

1. add an optional `INITIAL` section in the `MODEL` grammar
2. parse its contents using the same statement-list machinery already used for
   the main declarative section
3. mark those statements with a new statement-context bit such as
   `context_INITIAL`
4. run a semantic validation pass that rejects statement types not permitted
   in `INITIAL`

This keeps all existing relation/logrelation/`FOR` lowering paths shared.

The context bit then becomes the key to later stages:

- instantiation knows the statement came from `INITIAL`
- relation instances created from it can be flagged initialization-only
- system-build mode can include or exclude them without re-parsing anything

This is much cleaner than carrying a parser-global mode that changes relation
construction implicitly, and it avoids duplicating large amounts of parser
logic.

### Hierarchical model semantics

`INITIAL` should compose through hierarchy in the same way as ordinary model
equations.

That means:

- if a submodel type has an `INITIAL` section, those initialization equations
  belong to that submodel wherever it is instantiated
- when a parent model is built in initialization mode, the active
  initialization problem includes:
  - the parent's normal equations
  - the parent's `INITIAL` equations
  - each instantiated child model's normal equations
  - each instantiated child model's `INITIAL` equations

So initialization is hierarchical and declarative, not local-only.

This also allows a parent model to add cross-component initialization
constraints, for example:

```ascend
INITIAL
    child1.x = child2.x;
```

without needing to modify the child types themselves.

The natural implementation model is therefore:

- `INITIAL` is attached to the type where it is declared
- instantiation propagates it exactly like ordinary statements
- build mode determines whether those instantiated relations are included

This is preferable to a flat, simulation-root-only interpretation of
initialization.

### Out of scope for `INITIAL` v1

The following should be treated as later work:

- `initial algorithm`-style procedural initialization
- `pre(x)`
- `REINIT`
- event-triggered reinitialization
- changing the differential state set during initialization

Those features belong to the broader hybrid/event roadmap, not to the minimal
equation-based `INITIAL` section.

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
- QRSlv treatment of derivative pseudo-instances as ordinary variables, fixed
  to zero by default until explicitly edited

## Current Squishy Bits

The main unresolved areas are now narrower.

### 1. Full GUI semantics

The browser/object path is working, and the edit paths now clear derivative
algebraic defaults consistently. More end-to-end interactive exercise is still
useful, but the core mutation semantics are no longer just intended behavior.

### 2. Broader non-DAE solver semantics

QRSlv is now the tested reference implementation for algebraic solves.
The remaining question is how broadly to encode the same policy for other
non-DAE solver contexts.

### 3. Hybrid/event semantics

The derivative design is compatible with later work on events, but that work
has not yet been done.

### 4. Higher derivatives

The APIs expose derivative order, but the practical implementation focus is
still first-order derivatives.

## Recommended Next Steps

Near term:

1. generalize the QRSlv derivative-default policy cleanly across other
   non-DAE solver contexts where appropriate
2. exercise browser/GUI mutation paths against derivative pseudo-instances
   more directly
3. implement `INITIAL` as a second declarative statement section on models
4. keep `system_der` coverage growing as behavior is clarified

After that:

5. define `pre(x)` and `REINIT` semantics
6. expand hybrid/event support on top of the current derivative model

## `INITIAL` Implementation Status

Current implemented pieces:

- `INITIAL` is now a parser-recognized section inside `MODEL`
- `TypeDescription` now stores a separate `initstats` statement list
- statements in the `INITIAL` section are marked with `context_INITIAL`
- child-list derivation now sees names declared in both the normal body and
  `INITIAL`
- executable-statement traversal for model instantiation now runs over:
  - normal declarative statements
  - then `INITIAL` statements

This is enough to make `INITIAL` real in the compiler/runtime structure:

- the syntax parses
- the statements are preserved separately on the type
- named relations in `INITIAL` instantiate as children

However, this is **not yet the final semantics**.

Current limitation:

- `INITIAL` relations/logrelations are still instantiated and executed like
  ordinary declarative equations
- there is not yet an initialization-only relation flag
- there is not yet a build-mode switch that includes/excludes `INITIAL`
  equations

So the current implementation should be understood as:

- parser/type/instantiation plumbing complete enough to build on
- initialization-mode semantics still pending

The next implementation step is therefore:

1. mark instantiated relations/logrelations created from `context_INITIAL`
   statements
2. add internal mode switching for:
   - normal build
   - initialization build
3. include `INITIAL` equations only in initialization mode

Until that is done, `INITIAL` is structurally present but not yet
semantically isolated.

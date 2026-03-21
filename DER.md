# DER Design Notes

## Purpose

This note records the evolving design concept for `DER(...)`, `INDEPENDENT`,
and future `der(x)` support in ASCEND.

The current implementation is still largely LINK-driven and instance-name
driven in places. The goal is to move toward one canonical internal
representation of dynamic structure that is robust under:

- nested models
- arrays
- `ALIASES`
- `ARE_THE_SAME`
- future index-reduction work

## Current Findings

### Current ODE metadata path

At present, dynamic structure is still reconstructed late during system
analysis:

- [analyze.c](./ascend/system/analyze.c) classifies solver variables
- it reads `ode_type`, `ode_id`, `obs_id` from child atoms if present
- otherwise it falls back to `getOdeType(...)` / `getOdeId(...)`
- those functions still depend on compiler LINK metadata
- [diffvars.c](./ascend/system/diffvars.c) then builds derivative chains from
  `(ode_id, ode_type)` pairs

This is workable, but too indirect and too fragile.

### Reproducer status

Minimal reproducer models and scripts are in:

- [alias_der_wLINK.a4c](./models/test/ida/alias_der_wLINK.a4c)
- [check_alias_der.py](./models/test/ida/check_alias_der.py)

Current observed behaviour:

- top-level scalar `DER(dy_dt, y)` works
- nested scalar `DER(cell.dy_dt, cell.y)` works
- scalar alias case works
- nested array direct case works
- array `ARE_THE_SAME` case works
- array `ALIASES` case still fails with empty derivative structure

This suggests that the remaining problem is not generic nested naming, but the
way array aliases interact with dynamic variable identity.

After the first deferred `der(x)` bridge was added, the picture is now:

- `der(y)` with explicit legacy `DER(dy_dt, y)` bridge works for direct and
  nested scalar models
- the old scalar `ALIASES` reproducer also analyses successfully
- the remaining known failure is still the legacy array `ALIASES` case
- that remaining failure now sits squarely in legacy `diffvars` discovery, not
  in `der(x)` relation compilation

### First `der(x)` compiler experiment

An initial compiler experiment has now been made:

- lower-case `der(fname)` is accepted in the parser as an expression-level
  construct
- it is carried as a distinct expression node rather than being collapsed to a
  plain variable name
- relation-building code was then trialled with an *early* bridge that tried
  to resolve `der(x)` immediately to an already-materialised derivative
  instance from an `ode` chain

That experiment is useful because it exposed an important architectural
constraint:

- declarative relations are checked and compiled during instantiation
- method-level dynamic setup is too late for that
- even declarative `DER(...)` / `INDEPENDENT` metadata is not a good long-term
  basis for relation-time resolution of `der(x)`

Practical conclusion:

- `der(x)` should survive instantiation as a derivative reference to base
  instance `x`
- it should not be forced to resolve immediately to an explicit derivative
  variable during relation compilation
- binding/materialisation must therefore move deeper, into a later dynamic
  analysis stage

This is a strong argument that the correct bridge is not "smarter LINK lookup"
but "deferred derivative binding".

Current implementation status:

- token relations can now carry an explicit `e_der` term kind whose payload is
  still the base-variable incidence entry
- this is enough for `der(x)` to survive parsing and relation instantiation
- a first deferred bridge now runs in [analyze.c](./ascend/system/analyze.c)
  before solver lists are built
- that bridge walks token relations, resolves each `e_der` term through the
  currently declared legacy `DER(...)` / `ode` chain, and rewrites the term
  onto the materialised derivative variable
- a first analysis-side dynamic registry now also exists in
  [analyze.c](./ascend/system/analyze.c)
- that registry is built once from legacy `ode` / `independent` LINK metadata
  before `classify_instance`
- `classify_instance` now consults the registry first and only falls back to
  `getOdeType(...)` / `getOdeId(...)` when the registry does not yet have a
  safe canonical answer

This is still transitional, but it is now happening at the correct phase:
after instantiation, before solver analysis.

Important qualification:

- the first registry pass is intentionally conservative
- it currently tracks scalar atom instances cleanly
- some array-`ALIASES` cases still collapse ambiguously in the legacy LINK
  resolution APIs
- those ambiguous cases are therefore left on the legacy fallback path for
  now, rather than being mis-registered in the new registry

### Current test status

A new CUnit suite now exists in:

- [test_der.c](./ascend/system/test/test_der.c)

It currently covers:

- direct scalar `der(y)` with legacy `DER(dy_dt, y)` bridge
- nested scalar `der(cell.y)` with legacy bridge
- scalar `ALIASES`
- array `ARE_THE_SAME`
- array `ALIASES` as an explicit known-gap regression

Current observed results:

- `der_expr_direct_ok`: passes system build and IDA analyse
- `der_expr_nested_ok`: passes system build and IDA analyse
- `alias_der_alias_fail`: now passes system build and IDA analyse
- `alias_der_array_same`: passes system build and IDA analyse
- `alias_der_array_alias_fail`: still fails IDA analyse with empty derivative
  structure

So the new registry is already improving the scalar and nested cases, but the
array-`ALIASES` case remains the main unresolved dynamic-identity gap.

### Conditional models and IDA reanalysis

The current IDA path already shares ASCEND's conditional-model machinery with
CMSlv.

Relevant observations from the current implementation:

- `configure_conditional_problem(...)` records per-case relation structure and
  per-case incident-variable sets
- `reanalyze_solver_lists(...)` is used to rebuild the active solver lists when
  discrete state changes alter the active conditional structure
- both CMSlv and IDA call into this machinery
- IDA boundary/event handling can trigger a full reanalysis of its solver-side
  variable and relation lists after a conditional change

This means that the active variable incidence pattern can already change when a
`WHEN` condition switches.

However, that should not be confused with full support for arbitrary changes in
dynamic state identity. The current reanalysis path is much more clearly aimed
at:

- changing which relations are active
- changing which variables are incident in the active configuration

than at:

- changing which variables are differential states
- changing derivative order requirements
- changing derivative-chain identity across branches

Working conservative rule for first `der(x)` implementation:

- permit `WHEN`-driven changes in active equations
- require the canonical dynamic state set to remain fixed across branches
- require the highest derivative order for each state to remain fixed across
  branches

This should be revisited later if hybrid DAE support is extended further.

## Working Design Direction

### Canonical dynamic identity

Dynamic quantities should not be keyed by:

- name strings
- path strings
- `interface_ptr`
- late LINK string matching

They should be keyed by a canonical variable identity, ideally:

- canonical base instance (clique-aware)
- canonical independent instance
- derivative order

Conceptually:

```text
dyn_quantity := (base_instance, indep_instance, order)
```

Examples:

```text
x         => (x, t, 0)
der(x)    => (x, t, 1)
der2(x)   => (x, t, 2)
```

### `der(x)` as first-class quantity

The long-term direction is to support expression-level derivatives:

```text
der(y) = x - y;
der(x) = y - 2*x;
```

Restriction for first implementation:

- allow only `der(variable)`
- do not allow `der(expression)`
- do not allow `der(...)` in `WHEN` guards initially

`der(x)` should be:

- a first-class dynamic quantity
- not necessarily a user-declared instance
- still available for fixing, observation, and initialization

This means it must have backend identity, even if it is not a literal child in
the instance tree.

### `DER(...)` is transitional only

The intended end state is that `DER(...)` disappears from the language.

Target user-facing syntax:

```text
INDEPENDENT t;

der(y) = x - y;
der(x) = y - 2*x;
```

If a user wants an explicit variable for a derivative, that should be optional:

```text
x_dot = der(x);
```

So explicit derivative instances are not the canonical dynamic objects. They are
ordinary variables that a user may constrain to equal a derivative quantity.

Compatibility interpretation:

```text
DER(x_dot, x)
```

should be viewed as transitional sugar for:

```text
x_dot = der(x)
```

plus any convenience metadata needed during migration.

### Do not model `der(x)` as `x.der` child

The notion of derivative as an "attribute" on an instance is useful
conceptually, but should probably not be represented as a normal tree child.

Reasons:

- it pollutes the user instance tree with backend artefacts
- arrays become awkward
- higher derivatives become awkward
- it entangles model structure with solver representation

Better approach:

- keep the user instance tree unchanged
- store derivative quantities in a dynamic registry keyed on canonical
  instances
- materialise hidden backend variables if a solver requires them

## Proposed Internal Representation

Possible backend structures:

```c
struct dyn_quantity {
    struct Instance *base;
    struct Instance *indep;
    unsigned order;
    struct var_variable *var; /* if materialised */
    unsigned flags;
};

struct dyn_chain {
    struct Instance *base;
    struct Instance *indep;
    unsigned max_order;
    struct dyn_quantity **q; /* q[0]=x, q[1]=der(x), ... */
};
```

`diffvars` should eventually become a solver-facing view of these canonical
chains, rather than the thing that discovers them.

For compatibility with existing solver backends, a `dyn_quantity` may be
materialised as a hidden solver variable even if there is no user-declared
instance for it.

## `INDEPENDENT`

Working rule:

- dynamic solve requires exactly one canonical independent variable
- algebraic solve may have zero
- if `der(...)` appears and no independent variable exists, this should be an
  error

Multiple `INDEPENDENT` declarations may be permitted if they collapse to one
canonical independent variable.

Useful interpretation:

- declaring multiple independent variables means the user intends them to be
  the same one
- this should be handled by the dynamic registry / canonicalization logic, not
  by interface hacks

## Interpretation of `DER(dy_dt, y)`

Transitional interpretation:

```text
DER(dy_dt, y)
```

is sugar for:

```text
dy_dt, der(y) ARE_THE_SAME
```

This is conceptually useful, with one caveat:

- `ARE_THE_SAME` normally works over instance identities already present in the
  tree
- `der(y)` is not yet a normal instance in the current design

So this should not be implemented literally as an instance-tree merge.

However, it remains a good semantic model:

- `DER(dy_dt, y)` states that user-declared `dy_dt` is the materialised
  representation of the dynamic quantity `der(y)`

That suggests the following lowering rule:

```text
DER(dy_dt, y)
=> register chain entry (base=y, order=1)
=> bind user instance dy_dt as the materialised variable for der(y)
```

This is probably the right compatibility meaning while `DER(...)` still exists.

Long term, the preferred user-written form should be:

$$
\dot y = \mathrm{der}(y)
$$

or simply direct use of $\mathrm{der}(y)$ in equations, with no `DER(...)`
statement at all.

## Hybrid / Event Considerations

The `der(x)` redesign should leave room for hybrid IVP/DAE models such as the
bouncing ball, but the first implementation should remain conservative.

Important distinctions:

- smooth DAE support and hybrid/event support are related but not identical
- event detection, state reset, and post-event consistent reinitialisation are
  separate concerns from smooth residual evaluation

Important first-phase restrictions:

- `der(...)` may appear in smooth equations
- `WHEN` may change active equation sets
- `WHEN` should not change the canonical differential state set
- `WHEN` should not change the highest derivative order required for a state
- guards should not depend on `der(...)` initially

This is compatible with many useful hybrid models while avoiding immediate
entanglement with full structural index-changing behaviour.

## Proposed Implementation Stages

### Stage 1

- keep `DER(...)` and `INDEPENDENT` syntax
- implement lower-case `der(fname)` parser support as an expression-level form
- restrict `fname` to ordinary variable references
- preserve `der(fname)` as an explicit derivative reference through
  instantiation, rather than requiring immediate resolution to a materialised
  derivative variable
- keep `DER(...)` working as a compatibility path during migration
- allow token relations to carry unresolved `e_der` terms through compilation
- add a late rewrite step that can bind `e_der(x)` onto an explicit derivative
  variable when a legacy `DER(...)` chain exists
- introduce a system-side dynamic registry
- canonicalize base/independent variables by clique-aware instance identity
  where the legacy LINK APIs provide enough information
- use the registry as the primary source for dynamic classification
- retain `getOdeType(...)` / `getOdeId(...)` only as fallback for cases not yet
  captured canonically

### Stage 2

- make `der(x)` the primary dynamic surface syntax
- treat `DER(...)` as deprecated compatibility syntax
- allow explicit derivative variables to be written as ordinary equations, eg
  $\dot x = \mathrm{der}(x)$
- build canonical derivative chains from explicit dynamic metadata instead of
  legacy LINK reconstruction
- move `diffvars` generation fully onto the canonical registry
- keep compatibility lowering only for legacy models during transition

### Stage 3

- allow mixed use of:
  - user-declared derivative instances
  - implicit `der(x)` quantities
- support direct fixing / observation / initialization of `der(x)`
- support explicit equations of the form
  $$
  \dot x = \mathrm{der}(x)
  $$
  as the normal way for users to expose a derivative as a variable

### Stage 4

- deprecate `DER(...)`
- retain only `der(x)` and ordinary equations
- use the same registry and derivative expression representation as the basis
  for Pantelides-style index reduction

## Open Questions

- How should canonical representative selection be implemented for aliased
  variables?
- Should materialised derivative variables always exist in the backend, or only
  on demand?
- How should initialization syntax expose `der(x)(t0)` cleanly?
- Where exactly should the dynamic registry live: compiler side, system side,
  or split across both?
- How much of the old `ode_id` / `ode_type` machinery should be retained as a
  compatibility layer during transition?

## Immediate Next Step

Short term, the architecture should be pushed toward:

- replacing the remaining array-`ALIASES` fallback with proper canonical array
  element identity in the dynamic registry
- then moving `diffvars` generation off raw `(ode_id, ode_type)` discovery and
  onto the registry itself

rather than continuing to patch name-based resolution edge cases.

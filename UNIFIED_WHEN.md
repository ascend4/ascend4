# Unified WHEN Syntax Design Notes

## Purpose

This note records a proposed direction for converging ASCEND syntax for:

- steady conditional models solved by CMSlv/CMSlv2
- hybrid DAE models solved by IDA
- future MINLP/GDP-style conditional optimisation, possibly through SCIP or
  another mixed-integer backend
- fixed model-variant selection using a selector value

The core observation is that the existing syntaxes for steady conditional
models and hybrid DAE state machines overlap but are not yet semantically
unified.

Current steady conditional models often use:

```ascend
CONDITIONAL
    boundary: x < x0;
END CONDITIONAL;

b == SATISFIED(boundary, eps);

WHEN(b)
    CASE TRUE:
        USE eq1;
    CASE FALSE:
        USE eq2;
END WHEN;
```

Current hybrid DAE models can use selector-style syntax:

```ascend
WHEN(mode)
    CASE 'free':
        USE free_eq;
        SWITCH TO 'impact' IF hit_floor;
    CASE 'impact':
        USE impact_eq;
        REINIT(v, -e * pre(v));
        SWITCH TO 'free' IF TRUE;
END WHEN;
```

These are related, but the meanings are not identical:

- steady conditional modelling needs region membership/admissibility
- hybrid DAE modelling needs transition/event semantics
- MINLP/GDP modelling needs disjunctive region predicates
- fixed variant selection needs simple selector dispatch

The proposed syntax keeps a single `WHEN(selector) ... CASE ... END WHEN`
surface form while distinguishing region predicates from transition guards.

## Proposed Surface Form

Best-so-far proposed form:

```ascend
WHEN(selector)
    CASE 'name1' [IF condition]:
        [APPLIES IF predicate;]
        USE ...
        [SWITCH TO 'name2' IF guard;]

    CASE 'name2' [IF condition]:
        ...

    OTHERWISE ['nameN']:
        ...
END WHEN;
```

The optional clauses have distinct meanings:

- `CASE ... IF condition` is priority cascade / classifier syntax.
- `APPLIES IF predicate` is explicit region admissibility syntax.
- `SWITCH TO ... IF guard` is dynamic transition syntax.
- `USE ...` or direct equations define the active equation set for the case.

The syntax intentionally does not force users to choose at model-build time
whether the same model will later be solved with CMSlv/CMSlv2, integrated with
IDA, or eventually transformed to a MINLP/GDP formulation. The stored model can
carry enough information for each runtime path to use the appropriate
interpretation.

The instance tree should remain the canonical presentation of what the user
wrote. Solver-specific lowering must not rewrite it. Instead, solvers should
request a derived view in the `slv_system_t` structures. That view can contain
lowered region predicates, transition roots, disjunctive encodings, or other
solver-oriented data without changing the model tree shared by the GUI and
other solvers.

## Priority Classifier Syntax

`CASE ... IF ...` is an IF/ELSEIF cascade with named regions.

Example:

```ascend
WHEN(delta_region)
    CASE 'slow' IF bubble_regime_ratio < 1:
        USE delta_eq_slow;

    CASE 'intermediate' IF bubble_regime_ratio < 5:
        USE delta_eq_intermediate;

    OTHERWISE 'fast':
        USE delta_eq_fast_or_vigorous;
END WHEN;
```

This is equivalent to a priority cascade:

```ascend
IF bubble_regime_ratio < 1 THEN
    USE delta_eq_slow;
ELSEIF bubble_regime_ratio < 5 THEN
    USE delta_eq_intermediate;
ELSE
    USE delta_eq_fast_or_vigorous;
END IF;
```

The case predicates are expanded internally as:

```text
R_slow         = bubble_regime_ratio < 1
R_intermediate = NOT R_slow AND bubble_regime_ratio < 5
R_fast         = NOT R_slow AND NOT (bubble_regime_ratio < 5)
```

The resulting selector value is first-class and inspectable:

```text
delta_region = 'slow' | 'intermediate' | 'fast'
```

This avoids the main weakness of a plain anonymous `IF` cascade: the selected
region is not automatically available as a model variable.

## Explicit Region Syntax

`APPLIES IF` gives the region predicate directly:

```ascend
WHEN(delta_region)
    CASE 'slow':
        APPLIES IF bubble_regime_ratio < 1;
        USE delta_eq_slow;

    CASE 'intermediate':
        APPLIES IF bubble_regime_ratio >= 1 AND bubble_regime_ratio < 5;
        USE delta_eq_intermediate;

    CASE 'fast':
        APPLIES IF bubble_regime_ratio >= 5;
        USE delta_eq_fast_or_vigorous;
END WHEN;
```

This is more verbose than `CASE ... IF ...`, but it is also more explicit. It
is the right form when region predicates are not naturally a priority cascade,
or when a steady-state admissibility region needs to be stated independently
from any dynamic transition logic.

## Dynamic Transition Syntax

`SWITCH TO ... IF ...` remains the dynamic state-machine mechanism:

```ascend
WHEN(mode)
    CASE 'off':
        USE off_eq;
        SWITCH TO 'on' IF T < T_low;

    CASE 'on':
        USE on_eq;
        SWITCH TO 'off' IF T > T_high;
END WHEN;
```

This expresses hysteresis. It cannot be reduced to a unique memoryless steady
classification in the band:

```text
T_low <= T <= T_high
```

In that range, the active state depends on history. That is exactly what
`SWITCH TO` is for.

## Compatibility Rules

The following rules keep the semantics tractable:

1. `CASE ... IF ...` and `APPLIES IF` are not mixed in the same `WHEN` block.
   - `CASE ... IF ...` means priority cascade.
   - `APPLIES IF` means explicit independent region predicates.

2. If any `SWITCH TO` appears, automatic transition inference is disabled.
   - The block is being written as a dynamic state machine.
   - Source-specific transitions, hysteresis, latches, and resets are allowed.

3. `SWITCH TO` should not be required in every case.
   - Absorbing, terminal, resting, and fault states are legitimate.
   - Requiring every case to switch would rule out useful state machines.

4. If no `SWITCH TO` appears, dynamic mode tracking can be inferred from the
   region predicates where needed by IDA.
   - From any current case, switch to the case whose region predicate is true.
   - This is valid only for memoryless classifier behaviour.

5. If no `APPLIES IF` appears, CMSlv/CMSlv2/MINLP region predicates are
   inferred from the `CASE ... IF ...` priority cascade.

6. If the selector is fixed to a constant value, the block can be used as fixed
   region dispatch.
   - The selected case is active.
   - Region predicates may be checked as consistency conditions.

7. `OTHERWISE` provides exhaustive classification for priority cascades.
   - Without `OTHERWISE`, uncovered regions must be diagnosed clearly.
   - A future syntax could explicitly permit "no active case", but it should
     not be the default.

## Solver Interpretations

### QRSlv

QRSlv does not solve the conditional/discrete selection problem. It can solve
an already-selected configuration of a `WHEN` model.

The active configuration is selected by the shared ASCEND conditional analysis
at system-build or reanalysis time:

```ascend
b := TRUE;
b.fixed := TRUE;

WHEN(b)
    CASE TRUE:
        USE true_eq;
    CASE FALSE:
        USE false_eq;
END WHEN;
```

For QRSlv, this means:

- the current selector values choose the active `CASE`;
- only active relations/variables are counted by QRSlv;
- changing a selector value requires rebuild/reanalysis before the active case
  changes in the solver system;
- nested `WHEN`s work for this fixed-configuration dispatch use, because the
  conditional analysis recursively applies nested active cases;
- QRSlv will not search for the correct selector values from
  `CONDITIONAL`/`SATISFIED` boundaries.

This is the relevant pattern for legacy models, including the collocation
models: a boolean or selector can be assigned by a method, fixed, and QRSlv can
solve the resulting active branch. It is not CMSlv-style conditional solving.

### CMSlv/CMSlv2

For steady conditional solving, the important information is:

```text
case name -> active equation set
case name -> region predicate
```

`CASE ... IF ...` and `APPLIES IF` both provide region predicates. `SWITCH TO`
does not define steady-region membership and should not be used as a substitute
for `APPLIES IF`.

Nested `WHEN`s need a precise support statement:

- active-case selection is shared with QRSlv and is recursive, so nested active
  branches can be selected;
- full conditional search/consistency over nested alternatives is not yet a
  complete, well-tested contract;
- the existing structural comparison code explicitly treats a `CASE` containing
  nested `WHEN`s as structure-changing rather than fully comparing every nested
  alternative;
- CMSlv2 inherits this limitation from the common conditional-analysis layer,
  even though CMSlv2's newer decomposition/scheduler work may make the
  limitation more visible.

Therefore nested `WHEN` should be considered supported for active
configuration dispatch, but still experimental for general CMSlv/CMSlv2
conditional-region solving until dedicated tests and structural analysis are
added.

Status on branch `unified-when`: fixed-selector nested `WHEN` dispatch now has
dedicated CUnit coverage:

- `solver_qrslv`: `nested_when_static_outer_true_inner_false` and
  `nested_when_static_outer_false_inner_true`
- `solver_cmslv`: `nested_when_static`
- `solver_cmslv2`: `cmslv2_nested_when_static`

These tests deliberately use fixed Boolean selectors. They prove recursive
active-case selection for already-selected configurations. They do not claim
that CMSlv/CMSlv2 can yet search arbitrary nested conditional alternatives.

### IDA

For hybrid DAE solving, the important information is:

```text
current selector value
active case equations
event sources / transition guards
transition actions such as REINIT
```

If explicit `SWITCH TO` is present, IDA uses those transitions.

If no explicit `SWITCH TO` is present, IDA may infer classifier-tracking
transitions from the region predicates:

```text
from any source case:
    switch to target case when target region predicate becomes true
```

This inference is appropriate for memoryless mode tracking. It is not
appropriate for hysteresis, latching, timers, counters, or reset-based models.

### MINLP/GDP

For future MINLP/GDP lowering, the important information is:

```text
disjunct label
disjunct equations
logical condition / region predicate
selector or binary encoding
```

`CASE ... IF ...` gives a priority-encoded disjunction. `APPLIES IF` gives
explicit disjunct predicates. `SWITCH TO` is dynamic and should generally not
be lowered to a steady MINLP formulation unless separate region predicates are
also supplied.

### Fixed Selector Dispatch

If the selector is fixed or otherwise set externally, the syntax can replace a
separate `SELECT` statement:

```ascend
WHEN(correlation)
    CASE 'linear':
        USE linear_eq;
    CASE 'parabolic':
        USE parabolic_eq;
END WHEN;
```

This remains useful even without `CASE ... IF`, `APPLIES IF`, or `SWITCH TO`.

## Relationship To WHEN(bool,...)

Existing Boolean tuple dispatch remains useful:

```ascend
WHEN(delta_use_slow, delta_use_intermediate_or_slow)
    CASE TRUE,TRUE:
        USE delta_eq_slow;
    CASE FALSE,TRUE:
        USE delta_eq_intermediate;
    CASE FALSE,FALSE:
        USE delta_eq_fast_or_vigorous;
END WHEN;
```

Adding don't-care patterns would make this more compact:

```ascend
WHEN(delta_use_slow, delta_use_intermediate_or_slow)
    CASE TRUE,*:
        USE delta_eq_slow;
    CASE FALSE,TRUE:
        USE delta_eq_intermediate;
    CASE FALSE,FALSE:
        USE delta_eq_fast_or_vigorous;
END WHEN;
```

An IF cascade can be lowered to such Boolean tuple dispatch by expanding each
branch into priority predicates:

```text
branch 1 = b1
branch 2 = NOT b1 AND b2
branch 3 = NOT b1 AND NOT b2 AND b3
branch 4 = NOT b1 AND NOT b2 AND NOT b3
```

Karnaugh-map, Quine-McCluskey, or BDD-style minimisation can then compact those
predicates into don't-care patterns.

The proposed `WHEN(selector) CASE ... IF ...` form is the named-region
equivalent of this transformation.

## Related Literature and Concepts

This topic overlaps several established bodies of work. The closest fit is not
ordinary finite-state-machine design alone, but multimode DAE structural
analysis: the combination of state graphs, guarded mode changes, active equation
sets, and restart/initialisation semantics.

### Multimode DAE structural analysis

The most directly relevant references are the recent Modelica/multimode DAE
papers by Benveniste, Caillaud, Malandain, and collaborators:

- Benveniste, Caillaud, Malandain, and Thibault, "Algorithms for the
  Structural Analysis of Multimode Modelica Models", *Electronics*, 2022.
- Benveniste, Caillaud, and Malandain, "Handling Multimode Models and Mode
  Changes in Modelica", *Proceedings of the 14th Modelica Conference*, 2021.
- Benveniste et al., "Structural Analysis of Multimode DAE Systems: summary of
  results", arXiv:2101.05702.

These works are relevant because they treat mode-dependent DAE structure as a
compiler problem, not just as a runtime event problem. They discuss structural
analysis across multiple modes, mode-change events, consistent initialization,
rewriting to solver-amenable forms, and impulsive behaviour at mode changes.

For ASCEND, the analogous presolve object would be something like:

```text
HybridCaseGraph:
    selector states
    active equations per state
    region predicates / invariants
    transition guards
    reset actions
    adjacency graph
    structural signatures per state
```

IDA would use this graph to decide which event roots to monitor and whether
adjacent mode changes have coherent DAE structure. CMSlv2/MINLP would use the
same graph to obtain region predicates and disjunctive equation sets.

### Hybrid automata

Hybrid automata provide the standard language of:

```text
modes
continuous dynamics per mode
invariants / admissible regions
guarded transitions
reset maps
reachability over the mode graph
```

This vocabulary maps well onto the proposed syntax:

```text
CASE body       -> mode-local equations
APPLIES IF      -> invariant / admissible region
SWITCH TO IF    -> guarded transition
REINIT          -> reset map
selector value  -> current mode
```

Hybrid automata reachability literature is probably more heavyweight than
ASCEND needs for the first implementation, but it is conceptually useful for:

- defining adjacency through guard sets
- distinguishing invariants from transition guards
- detecting unreachable modes
- reasoning about same-time transition cycles and Zeno/chattering behaviour
- understanding why hysteresis is genuinely history-dependent

### Boolean minimisation and don't-cares

For the steady conditional side, `IF` cascades and `WHEN(bool,...)` cases can be
viewed as Boolean region predicates. Classical Boolean minimisation ideas such
as Karnaugh maps, Quine-McCluskey minimisation, and BDDs are relevant for:

- expanding priority cascades into mutually exclusive branch predicates
- simplifying case predicates
- introducing don't-care patterns in `WHEN(bool,...)`
- checking overlap and coverage for finite Boolean control tuples

This does not solve the continuous guard problem by itself, but it provides a
good internal representation for the discrete/logical part of region
selection.

### Design implication

The proposed syntax should not be implemented as a shallow parser rewrite only.
The long-term useful internal representation is a case graph with guards,
regions, active equation sets, and structural signatures. Different solvers can
then ask different questions of the same graph:

```text
CMSlv2:
    Do the region predicates define a valid disjunction?

IDA:
    What roots and event sources are needed?
    Which states are adjacent?
    Are adjacent mode changes structurally admissible?

MINLP/GDP:
    What disjuncts, binaries/selectors, and logical implications are needed?

GUI:
    What state/mode graph should be shown to the user?
    Which states are unreachable, terminal, or ambiguous?
```

## Case For Migration

### 1. One syntax family for related concepts

ASCEND currently has overlapping ways to express conditional structure:

- Boolean condition variables and `WHEN(bool,...)`
- `CONDITIONAL` / `SATISFIED(...)`
- selector `WHEN(mode)` blocks
- dynamic `SWITCH TO ... IF ...`
- possible future MINLP/GDP disjunctions

A unified `WHEN(selector)` form gives users one visible structure for active
equation selection, while still distinguishing steady regions from dynamic
transitions.

### 2. First-class selected region

Plain `IF / ELSEIF / ELSE` syntax is readable, but it does not automatically
produce a model variable naming the chosen region.

`WHEN(selector) CASE ... IF ...` preserves IF-cascade readability while making
the selected region inspectable, fixable, reportable, and usable by the GUI.

### 3. Better GUI/runtime fit

In the GUI, the user may instantiate a model and later choose:

- a steady nonlinear solver
- CMSlv/CMSlv2
- IDA
- a future MINLP/GDP backend

ASCEND does not necessarily know the final runtime interpretation at compile
time. Storing both active-equation sets and region/transition metadata allows
the runtime path to choose the relevant semantics.

### 4. Cleaner steady conditional modelling

Many CMSlv-style models are naturally priority classifiers:

```ascend
IF r < 1 THEN slow
ELSEIF r < 5 THEN intermediate
ELSE fast
```

Writing these as Boolean tuple `WHEN(...)` blocks forces users to enumerate
intermediate Boolean combinations. The proposed syntax expresses the intended
logic directly and lets the compiler lower to Boolean/disjunctive form.

### 5. Cleaner hybrid DAE modelling

The current `WHEN(mode) CASE ... SWITCH TO ... IF ...` syntax already works
well for dynamic state machines. The unified syntax preserves that form and
does not force hybrid models into steady conditional syntax.

### 6. Explicit place for hysteresis and memory

Hysteresis, latches, trips, timers, counters, and resets cannot be represented
as memoryless steady classification without extra state. Keeping `SWITCH TO`
as explicit transition syntax makes that distinction visible.

### 7. Natural bridge to MINLP/GDP

`APPLIES IF` is close to the logical predicate of a disjunct. A future MINLP
or GDP lowering can consume:

```text
selector value
region predicate
active equations
```

without trying to reverse-engineer them from dynamic transitions.

### 8. Better diagnostics become possible

Once region predicates are first-class, ASCEND can diagnose:

- uncovered regions
- overlapping regions
- unreachable cases
- selector values without cases
- cases whose equations are structurally incompatible
- classifier blocks accidentally written with dynamic memory

Those diagnostics are much harder when the logic is hidden inside low-level
Boolean variables and separate `SATISFIED(...)` relations.

### 9. Supports fixed model variants

The same `WHEN(selector)` structure supports fixed selector dispatch. This can
replace a separate `SELECT` statement while preserving a natural path to solver
chosen regions later.

### 10. Incremental migration path

Existing syntax need not be removed. The new form can be added as a higher
level surface syntax that lowers to existing `WHEN`, active relation, and
conditional machinery.

## Case Against Migration

### 1. Semantic overload risk

One block form would cover:

- fixed dispatch
- steady region classification
- disjunctive optimisation
- inferred dynamic tracking
- explicit dynamic state machines

This is powerful, but it risks confusing users unless the distinctions between
`CASE IF`, `APPLIES IF`, and `SWITCH TO` are extremely clear.

### 2. Priority versus admissibility ambiguity

`CASE ... IF ...` is a priority cascade:

```text
second case means NOT first AND second condition
```

`APPLIES IF` is an explicit predicate:

```text
second case means exactly the predicate written
```

These are intentionally different. Users may expect them to be equivalent
unless documentation and diagnostics make the difference obvious.

### 3. Solver-dependent interpretation

The same model may be solved by CMSlv2 or integrated by IDA. That is a goal,
but it also means solver-specific interpretations can diverge.

For example, CMSlv2 needs region admissibility, while IDA needs event sources.
If automatic IDA tracking is inferred from region predicates, users may be
surprised by event behaviour that was not written as `SWITCH TO`.

### 4. General root generation is hard

Simple conditions such as:

```ascend
CASE 'high' IF h >= h0:
```

can be lowered to IDA root functions. Arbitrary Boolean combinations of
continuous and discrete predicates are much harder.

The implementation must be conservative. It should support simple cases first
and require explicit `CONDITIONAL` / `SATISFIED(...)` plumbing or explicit
guards for complex cases.

### 5. Not every dynamic state machine has steady meaning

Hysteresis, latches, trips, fault states, deadbands, and `REINIT(...)` actions
are genuinely dynamic. They cannot be reverse-lowered to a unique steady
conditional region without adding memory variables or extra modelling choices.

The unified syntax must not imply that all `WHEN(selector)` blocks are valid
steady conditional models.

### 6. MINLP/GDP lowering needs stronger checks

MINLP/GDP formulations need clean disjunctions. Overlapping or incomplete
region predicates may be acceptable in a dynamic model but invalid for a
steady mixed-integer formulation.

The compiler/runtime must know when to reject, warn, or require explicit
disambiguation.

### 7. Existing models and user habits

Existing CMSlv examples use `CONDITIONAL`, `SATISFIED`, Boolean variables, and
`WHEN(bool,...)`. Existing hybrid examples use selector `WHEN` and `SWITCH TO`.

Introducing a new preferred form may create a long period where multiple
styles coexist. That is manageable, but documentation and examples must avoid
making the language feel fragmented.

### 8. Parser and grammar complexity

`CASE ... IF`, `APPLIES IF`, optional `OTHERWISE 'name'`, direct equations in
cases, `USE`, `REINIT`, and `SWITCH TO` all need clean grammar rules.

Adding too much at once risks subtle parse ambiguities and poor error
messages.

### 9. Structural consistency remains difficult

Even with clean syntax, sibling cases may have incompatible equation
structures for IDA or CMSlv2. Syntax unification does not remove the need for
alternative-structure checks and solver-specific diagnostics.

### 10. Risk of premature commitment

ASCEND already has working pieces. A broad syntax migration could lock in a
surface form before the CMSlv2, IDA, and MINLP/GDP lowering requirements are
fully understood.

This argues for implementing the feature in phases and treating early syntax
as experimental.

## Inference and Memory Detection

A `WHEN(selector)` block without `SWITCH TO` can be treated as a memoryless
classifier if it has valid region predicates.

For `CASE ... IF`:

```text
R_1 = c_1
R_2 = NOT c_1 AND c_2
R_3 = NOT c_1 AND NOT c_2 AND c_3
R_otherwise = NOT c_1 AND NOT c_2 AND NOT c_3
```

For `APPLIES IF`, the `R_i` are written directly.

If explicit `SWITCH TO` is present, the block is not a memoryless classifier
unless separate region predicates are also supplied. The following should be
treated as dynamic memory and not reverse-lowered to steady conditional logic:

- `REINIT(...)`
- `pre(...)`
- timers
- counters
- event-memory variables
- one-way transitions
- absorbing states
- source-dependent transition guards
- hysteresis thresholds
- latches and trips

The implementation should be conservative:

```text
proved classifier      -> allow steady/dynamic classifier lowering
explicit dynamic       -> use SWITCH TO semantics
not proven             -> require APPLIES IF or reject for steady lowering
```

## Suggested Phasing

### Phase 1: Boolean tuple improvements

Add don't-care support to existing `WHEN(bool,...)`:

```ascend
CASE TRUE,*:
```

This improves current CMSlv/CMSlv2 models without changing selector semantics.

Status on branch `unified-when`: started. The parser now accepts `*` in set
lists as an alias for the existing `ANY` expression, so both forms are accepted:

```ascend
WHEN(b1,b2)
    CASE TRUE,ANY:
        USE eq1;
    CASE FALSE,*:
        USE eq2;
END WHEN;
```

The runtime already had support for per-position wildcard values in conditional
case matching. The branch adds instantiate coverage for both `ANY` and `*` in
multi-Boolean `WHEN` cases. Remaining Phase 1 follow-up is mostly language
policy: decide whether `*`/`ANY` should be advertised only for `WHEN(bool,...)`
or also as a general case-list wildcard for `SELECT`/`SWITCH`, where the
underlying machinery is also close to supporting it.

Wildcard cases should be understood as an ordered macro expansion. For example:

```ascend
WHEN(b1,b2)
    CASE TRUE,*:
        USE eq1;
    CASE TRUE,FALSE:
        USE eq2;
END WHEN;
```

is equivalent to writing the expanded cases in the same order:

```ascend
WHEN(b1,b2)
    CASE TRUE,TRUE:
        USE eq1;
    CASE TRUE,FALSE:
        USE eq1;
    CASE TRUE,FALSE:
        USE eq2;
END WHEN;
```

ASCEND's existing conditional analysis uses first matching case wins semantics,
and CMSlv/CMSlv2 consume that same conditional analysis path. Therefore
wildcard overlap is not a semantic change: later cases can be shadowed by
earlier wildcard cases just as later branches in an `IF` / `ELSEIF` cascade can
be shadowed. A future diagnostic pass could warn about exact duplicate cases,
fully shadowed cases, or suspicious partial overlaps, but Phase 1 should not
change selection semantics.

The current test model `when_ok_wildcard_collision_first_wins` records this
policy explicitly: `CASE *,FALSE` appears before `CASE TRUE,FALSE`, and the
first case is selected.

### Phase 2: Priority classifier syntax

Add proposed `CASE ... IF ...` syntax for `WHEN(selector)` and lower it to
explicit internal region predicates.

Initial implementation can target steady conditional solving only.

Status on branch `unified-when`: parser/AST/runtime-representation groundwork
started. `CASE ... IF` and `APPLIES IF` are now accepted by the grammar and
stored on each `WHEN` case as optional classifier/region predicates. They are
copied, destroyed, compared, and written with the statement tree, then carried
into the instantiated `Case` and solver `when_case` structures. A `WHEN` block
that mixes `CASE ... IF` and `APPLIES IF` is rejected during instantiation, as
planned. Conditional analysis deliberately rejects solver execution of either
predicate form until the lowering pass exists; this avoids
silently treating:

```ascend
WHEN(region)
    CASE 'slow' IF r < 1:
        USE slow_eq;
END WHEN;
```

as an ordinary selector-only case and losing the classifier predicate. The next
implementation step is to lower ordered `CASE IF` guards to explicit internal
region predicates:

This is a real lowering step, not just a relaxation of the current guardrail.
The conditional-analysis code can already choose active cases from selector
values, but it does not yet evaluate or solve over the classifier predicates.
Allowing solver execution before lowering would mean the predicate metadata is
present in the tree but semantically ignored.

Solver-side lowering groundwork is now present. `when_case` has an optional
lowered region predicate owned by the solver-system presentation, not by the
compiler instance tree. Solvers can call:

```c
when_lower_classifier_regions(when, WHEN_REGION_STEADY);
```

or, later:

```c
when_lower_classifier_regions(when, WHEN_REGION_DYNAMIC_CLASSIFIER);
```

The current implementation derives:

- `APPLIES IF p` -> region predicate `p`
- ordered `CASE IF` cascade -> mutually exclusive predicates
- `OTHERWISE` in a `CASE IF` cascade -> complement of all previous guards

This lowering is deliberately separate from solver consumption. Lowering is
part of the solver-system presentation, and different solvers will ask
different questions of the same stored syntax.

There is also now a `slv_system_t`-level handoff API:

```c
slv_has_classifier_whens(sys);
slv_lower_classifier_whens(sys, WHEN_REGION_STEADY);
slv_classifier_regions_lowered(sys, WHEN_REGION_STEADY);
```

`system_build` is allowed to construct a system containing `CASE IF` or
`APPLIES IF`. Ordinary selector-dispatch configuration skips those classifier
`WHEN`s, leaving their alternatives inactive until a solver consumes the
lowered region view.

CMSlv2 now has the first steady-state consumption path. During presolve it:

- requests `WHEN_REGION_STEADY` lowering for classifier `WHEN`s
- reanalyzes active equations using the lowered predicates
- evaluates those predicates against the current instance-tree values
- activates the first true lowered region, including nested `WHEN`s
- hands the resulting active system to its existing QRSlv/LRSlv machinery

This is intentionally a first, static steady-region path. It validates that the
lowered presentation can select equations for CMSlv2, but it is not yet a full
mixed complementarity/MINLP-style search over free algebraic region
predicates. A predicate such as `x < 1` is evaluated from the current value of
`x` at reanalysis time; future CMSlv2 work must decide how to make such
predicates part of the branch/search/boundary machinery when `x` is a solved
unknown.

QRSlv and CMSlv still reject direct classifier models at solver setup unless a
solver has already explicitly consumed the lowered steady presentation. IDA
also rejects classifier predicates until dynamic transition lowering exists.

IDA has the same guardrail. `when_case` now exposes a recursive predicate
detector, and IDA analysis, event-root refresh, and REINIT handling reject
classifier predicates explicitly. The current IDA tests
`ida_when_case_if_rejected` and `ida_when_applies_if_rejected` record that the
new syntax cannot reach dynamic solving until classifier-transition lowering is
implemented.

```text
R_1 = c_1
R_2 = NOT c_1 AND c_2
R_3 = NOT c_1 AND NOT c_2 AND c_3
R_otherwise = NOT c_1 AND NOT c_2 AND NOT c_3
```

Once that representation exists, CMSlv/CMSlv2 can consume the predicates as
steady regions and IDA can later infer classifier-tracking transitions where
the predicates are simple enough.

An implementation experiment now records the more compact, boundary-preserving
`CASE IF` lowering shape. Instead of immediately converting each final region
to a one-hot region predicate, ASCEND can treat each original `CASE IF` guard
as a natural generated Boolean:

```ascend
WHEN(region)
    CASE 'slow' IF g1:
        USE slow_eq;
    CASE 'medium' IF g2:
        USE medium_eq;
    OTHERWISE:
        USE fast_eq;
END WHEN;
```

is equivalent to the old-style tuple dispatch:

```text
generated guard booleans:
    b1 == g1
    b2 == g2

WHEN(b1, b2)
    CASE TRUE, *:
        USE slow_eq;
    CASE FALSE, TRUE:
        USE medium_eq;
    CASE FALSE, FALSE:
        USE fast_eq;
END WHEN
```

This preserves the original guard expressions `g1`, `g2` as the natural
boundary/logical predicates. For example, if `g2` is:

```ascend
a < 5 AND (c2 OR c1) AND y > x
```

then the lowering keeps that expression as the generated guard definition,
rather than hiding the natural boundary surfaces behind a derived one-hot
region such as `NOT g1 AND g2`. The tuple pattern carries priority semantics:

```text
case 1      -> TRUE, *
case 2      -> FALSE, TRUE
otherwise  -> FALSE, FALSE
```

The current C API experiment exposes this plan with:

```c
when_case_if_guard_count(when, &nguards);
when_case_if_guard(when, i);
when_case_if_pattern(when, wc, values, &nvalues);
when_case_if_materialization_plan(when, &plan);
```

`when_case_if_materialization_plan` records the generated-object shape without
yet allocating virtual solver objects. For the guard:

```ascend
a < 5 AND (c2 OR c1) AND y > x
```

the current test expects:

```text
guard_booleans      = 1
real_boundaries     = 2     # a = 5, y = x
logical_boundaries  = 0
boolean_ops         = 3     # AND, OR, AND
hidden_booleans     = 1
hidden_relations    = 2
hidden_logrelations = 1
```

This accounting should avoid generating unnecessary helper Booleans. A
`CASE IF` guard such as:

```ascend
CASE 'slow' IF use_slow:
```

where `use_slow` is already a named Boolean/conditional selector can be reused
directly as the compact guard slot. In contrast:

```ascend
CASE 'slow' IF a < 5:
```

requires generated boundary/logical machinery unless the user has already
named that boundary explicitly. The implementation should treat the compact
guard slots as a logical encoding, not as a mandate to allocate one new Boolean
for every guard.

The next implementation layer is a guard-artifact descriptor, still independent
of the final hidden-instance versus virtual-object decision. Each artifact is
one compact guard slot and records:

```text
guard index
source CASE / source guard expression
reuse existing Boolean, or generate helper Boolean
real boundary count
logical boundary count
Boolean operator count
reusable named Boolean terms inside compound guards
source module and line
```

Example classification:

```ascend
CASE 'slow' IF use_slow:
```

reuses `use_slow` as the guard slot. No generated Boolean/logrelation is
needed.

```ascend
CASE 'medium' IF a < 5:
```

requires a generated guard Boolean/logrelation plus one natural real boundary.

```ascend
CASE 'fast' IF a < 5 AND use_fast:
```

requires a generated guard Boolean/logrelation, one natural real boundary, and
can reuse `use_fast` as a named Boolean term inside the generated logical
definition.

This is not yet the full CMSlv2 materialisation step. Inspecting the old CMSlv
path shows that existing `rel_relation`, `logrel_relation`, and
`bnd_boundary` objects are strongly instance-backed: evaluation, residual
storage, direct solve, names, and boundary calculations all dereference
compiler `Instance` objects. Therefore the next implementation step needs an
explicit choice:

- compiler/instantiation rewrite: generate real hidden boolean/logrel/relation
  instances before `system_build`, preserving the existing solver object model
- solver-side virtual objects: extend `rel_relation`/`logrel_relation`/boundary
  evaluation to handle generated, non-instance-backed guard artifacts

Hidden instances are a real extra design issue, not just an allocation detail.
ASCEND already has a narrow hidden-instance mechanism for backend-created
dynamic derivative variables. Those instances are carried by `slv_system_t` so
their interface pointers can be cleared when the solver system is destroyed.
That mechanism does not currently provide a general hidden child namespace.

For generated `CASE IF` predicates, nameability matters. A generated real
boundary such as `a < 5` can plausibly become an internal conditional relation,
but a generated guard logical relation needs to refer to it as
`SATISFIED(boundary_name, tol)`. The existing compiler/logrelation path resolves
`SATISFIED(...)` through a `Name` in the instance tree. An unparented hidden
relation instance is therefore not enough by itself: either the generated
objects need stable internal names in a hidden instance scope, or the solver
side needs a virtual logical-expression representation that can point directly
to generated boundary objects without going back through `Name` resolution.

The current materialisation-plan API records this explicitly:

```text
hidden_boolean_instances = one per CASE IF guard
hidden_relation_instances = one per real comparison boundary
hidden_logrel_instances = one per generated guard definition
requires_named_instances = true for the instance-backed route
```

Generated artifacts must also preserve source provenance. Users should not see
diagnostics for anonymous objects such as:

```text
__when_guard_3_boundary_2 failed
```

unless that message is immediately tied back to visible model text:

```text
in WHEN(delta_region), CASE 'intermediate' IF ...
guard term: y > x
source: models/foo.a4c:123
```

This is a practical requirement, not just a UI nicety. Generated guard
booleans, generated conditional relations, and generated logical relations will
be the objects seen by CMSlv/CMSlv2, IDA event handling, decomposition, and
debug diagnostics. If they only have hidden internal names, solver errors will
point at objects the user cannot find or edit.

ASCEND already stores module and line information at statement level. That is
enough to report the enclosing `WHEN` statement, but it is probably not enough
for good `CASE IF` diagnostics because the current case/list structures do not
appear to carry per-case or per-guard source locations. The materialisation
step should therefore attach provenance records to every generated object:

```text
generated object
    internal name / pointer
    source WHEN instance
    source CASE label or OTHERWISE
    source guard index
    source guard expression text, where printable
    source boundary subexpression, where applicable
    module and line number, at least for the enclosing WHEN
```

The same principle applies if the virtual-object route is chosen instead of
hidden compiler instances: virtual boundaries and virtual logrelations still
need user-facing provenance. This also keeps later IDA/index-reduction work
manageable, because generated derivative, residual, or consistency conditions
can be reported against the original visible equations and guards rather than
against solver-created helper objects.

The materialisation plan keeps this decision isolated. It defines what must be
generated while preserving the natural boundaries; the remaining design
question is where those generated artifacts should live.

### Phase 3: Explicit region predicates

Status on branch `unified-when`: syntax/storage groundwork started. Remaining
work is lowering and diagnostics for:

- missing `OTHERWISE`
- overlapping predicates where detectable
- uncovered selector values
- mixing `CASE IF` and `APPLIES IF` across richer edge cases

### Phase 4: Dynamic classifier tracking

For IDA, infer transitions from classifier predicates only when:

- no explicit `SWITCH TO` appears
- predicates are simple enough to lower to event sources
- no memory/reset constructs are present

Complex predicates should require explicit `CONDITIONAL` / `SATISFIED(...)` or
explicit `SWITCH TO` guards.

### Open Issue: INITIAL Classifier WHENs

ASCEND already has separate normal and initial build modes:

```text
system_build(...)
system_build_with_mode(..., SYSTEM_BUILD_INITIAL)
```

`INITIAL` equations are therefore part of the generic system-building story,
not just an IDA detail. It is plausible that users will eventually want
classifier syntax inside initialization logic, for example:

```ascend
INITIAL
    WHEN(region)
        CASE 'cold' IF T < T0:
            ...
        CASE 'hot' IF T >= T0:
            ...
    END WHEN;
END INITIAL;
```

This is not currently supported as a solved conditional initialization path.
If the initial mode is itself selected by free continuous variables, then
initialization requires conditional solving machinery, most likely CMSlv/CMSlv2,
rather than a fixed-branch QRSlv-style initialization.

The current plan should therefore treat `INITIAL` classifier `WHEN`s as an
unresolved issue:

- generated guard/boundary artifacts should eventually be build-mode aware;
- normal and initial active-equation sets may differ while sharing classifier
  predicates;
- IDA initialization may need a CMSlv/CMSlv2-style conditional initialization
  solve before integration can begin;
- current implementation should reject or clearly diagnose unsupported
  classifier `WHEN`s in `INITIAL` until that workflow is designed.

### Phase 5: Mixed advanced semantics

Allow `APPLIES IF` plus explicit `SWITCH TO` for advanced models where:

- `APPLIES IF` describes steady admissibility
- `SWITCH TO` describes dynamic transition behaviour

This is the natural form for models with hysteresis that still need a steady
region interpretation.

### Phase 6: MINLP/GDP lowering

Use region predicates and active equation sets to generate a disjunctive or
mixed-integer formulation.

This should come after the region semantics and diagnostics are stable.

## Open Questions

- Should `CASE ... IF` be accepted without `OTHERWISE`, and if so does that
  mean "no active case" or a model error?
- Should a fixed selector value suppress region predicates or require them as
  consistency checks?
- How much Boolean simplification should ASCEND perform internally?
- Should primitive continuous inequalities be canonicalised so complements can
  be detected?
- How should selector domains be displayed in the GUI/browser?
- Should syntax be marked experimental until CMSlv2 and IDA both consume it?
- Should `WHEN(bool,...)` eventually be considered a lower-level form, with
  `WHEN(selector) CASE IF` preferred in documentation?

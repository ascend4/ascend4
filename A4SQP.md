# A4SQP Concept

## Purpose

A4SQP is a proposed SQP-based nonlinear solver for ASCEND. Its intended niche
is hard nonlinear algebraic and nonlinear programming problems where the
existing local equation-solving path can stall because of active bounds, poor
initialization, inconsistent local linearizations, or tight engineering
specifications.

The initial implementation target is an elastic line-search SQP method using
HiGHS as the quadratic programming (QP) solver. A later phase may add filter SQP
globalization.

This note also records an architectural tension: A4SQP should eventually be
usable outside ASCEND through an IPOPT-like C problem ABI, but an ASCEND-native
solver client can both consume and return richer information through
`slv_system_t`.

## Initial Solver Shape

The first implementation should be conservative:

- Implement A4SQP as a normal ASCEND solver client over `slv_system_t`.
- Build the SQP problem from ASCEND solver variable and relation lists.
- Use exact first derivatives from ASCEND relation derivative machinery.
- Use HiGHS for convex QP subproblems.
- Use a positive-semidefinite Hessian approximation, initially BFGS,
  limited-memory BFGS, diagonal, or identity.
- Add elastic variables to QP subproblems so local linearized infeasibility does
  not immediately abort the solve.
- Use an L1 merit-function line search for first-phase globalization.
- Report diagnostics that identify active bounds, elastic rows, limiting
  relations, and the point where progress stalled.

The goal is not to reproduce SNOPT, WORHP, or Knitro immediately. The goal is a
focused ASCEND prototype that can test whether active-set elastic SQP helps
models that fail with the current solver set.

## Current Prototype Status

As of May 2026, the prototype has moved past the initial registration and QP
spike stage:

- A4SQP is implemented as an ASCEND solver client and can build a reusable
  ASCEND-native problem view from `slv_system_t`.
- HiGHS-backed convex QP subproblems are assembled and solved through the A4SQP
  QP layer.
- Elastic row variables are included in the QP model so local linearized
  infeasibility does not immediately abort an iteration.
- The prototype currently uses exact first derivatives from ASCEND together with
  a positive-semidefinite step Hessian approximation.
- The current Hessian model is a damped dense BFGS update on the primal step
  block, initialized from scaled curvature information.
- Globalization is an L1 merit-function line search with Armijo-style
  backtracking.
- The current iterate path supports repeated `presolve` plus `iterate` calls
  from the ASCEND CLI and test harness.

The current prototype has been exercised on small benchmark models under
`models/test/a4sqp`, including:

- `hs3.a4c`, an objective-only Hock-Schittkowski problem with a strongly scaled
  shallow curvature direction, now solving successfully with the current
  objective-only BFGS step Hessian;
- `hs11.a4c`, a constrained Hock-Schittkowski problem with one nonlinear
  inequality, now solving successfully with the current BFGS step Hessian and
  linearized-row tolerance handling;
- `hs21.a4c`, a linearly constrained quadratic Hock-Schittkowski problem, now
  solving successfully as a compact constrained regression;
- `rosenbr.a4c`, the classical two-variable Rosenbrock objective-only problem,
  now present as an ASCEND benchmark model but not yet a passing A4SQP
  regression.

Recent implementation lessons from these benchmarks:

- Objective-only models are valid A4SQP cases and must not be treated as QP
  assembly failures when the relation count is zero.
- Convergence for optimization problems must check stationarity, not just
  feasibility.
- Merit-function acceptance should be driven by predicted decrease, without
  imposing an artificial absolute decrease floor on otherwise valid steps.
- Triangular Hessian storage passed to HiGHS must be reconstructed carefully
  when computing model-predicted objective values.

Recent constrained-solver stabilization work:

- QP row-displacement bounds that are already within A4SQP's scaled feasibility
  tolerance are snapped to zero before the HiGHS call, which avoids late
  numerically pointless correction QPs;
- the merit line search no longer suppresses small but still useful accepted
  steps before the convergence check has a chance to see the updated iterate.

Still deferred:

- exact Hessian support via ASCEND Hessian callbacks with PSD regularization for
  HiGHS;
- a reduced-memory Hessian option such as L-BFGS;
- filter globalization;
- richer warm-start state beyond the current prototype state.

## SQP Subproblem

At iterate `x_k`, A4SQP forms a QP step `p` from a local model:

```text
minimize    grad f(x_k)^T p + 1/2 p^T B_k p + rho * sum(elastic_slacks)

subject to  r_L - e_L <= r(x_k) + J_r(x_k) p <= r_U + e_U
            x_L <= x_k + p <= x_U
            e_L, e_U >= 0
```

For a pure feasibility solve, `f` may be zero or a residual merit objective. For
an optimization solve, `f` is the ASCEND objective relation or objective
variable.

`B_k` should be positive semidefinite for HiGHS QP. Early implementations should
therefore avoid unregularized exact Hessians. Exact Hessians can be added later,
with regularization.

## Why HiGHS

HiGHS is attractive because ASCEND already has a HiGHS integration for LP/MIP
models and HiGHS provides the QP machinery needed by SQP. A4SQP should not try
to implement an active-set QP solver itself.

Required HiGHS-facing work:

- Build QP Hessian, gradient, bounds, and sparse linear relation rows.
- Add elastic columns and penalties.
- Preserve row and column ordering across SQP iterations.
- Investigate HiGHS support for warm-starting QP state or active sets.
- Decide whether to extend the existing ASCEND HiGHS wrapper or call HiGHS
  directly from A4SQP.

## ASCEND Integration

A4SQP should initially be an ASCEND solver client, not only a generic external
library. This matters in two directions:

- `slv_system_t` exposes solver-analyzed problem structure that does not fit
  naturally through a minimal IPOPT-like callback ABI.
- The solver can report useful post-solve state back in ASCEND terms, which is
  essential when a solve fails.

### Information A4SQP Should Consume

The ASCEND plugin should use:

- solver variable lists from `slv_get_solvers_var_list`;
- solver relation lists from `slv_get_solvers_rel_list`;
- objective relation or objective variable data;
- variable and relation `sindex` mapping;
- fixed, active, incident, and included flags;
- `var_filter_t` and `rel_filter_t`;
- solver block information where useful;
- bounds, nominal values, scaling, and initialization data;
- relation residual, gradient, and optional Hessian routines;
- linearity information where available;
- system serial/change information to validate warm starts;
- ASCEND naming and diagnostics for variables and relations.

This means the ASCEND A4SQP client should be more than a thin wrapper around a
flat NLP callback interface. It should understand enough of `slv_system_t` to
respect the model configuration that ASCEND has already analyzed.

### Information A4SQP Should Return

For QRSlv, it is useful to know which blocks solved and which block stalled. SQP
will not present the same block-by-block story, but it should expose an
equivalent model-review view:

- final active lower and upper variable bounds;
- final active lower and upper row bounds;
- rows using elastic violation variables, including magnitude and side;
- variables with large bound multipliers;
- rows with large relation multipliers;
- rows contributing most to merit or infeasibility;
- variables receiving persistently tiny or clipped steps;
- QP subproblem status at the failed iteration;
- line-search failure reason, if any;
- current SQP iteration number and accepted step length;
- feasibility norm, complementarity measure, stationarity measure, and merit
  value at failure;
- block or structural region containing the worst residual or elastic row, when
  this can be mapped back to ASCEND block data;
- stable row and variable names for diagnostic printing and GUI review.

The user-facing goal is that an unsuccessful A4SQP solve should still answer:

```text
Where did the solver get stuck?
Which equations or bounds are blocking progress?
Which parts of the model are already effectively satisfied?
Was the failure due to QP infeasibility, line search, active bounds, poor
scaling, derivative errors, or apparent model infeasibility?
```

This argues for tight `slv_system_t` integration even if the core algorithm is
kept portable.

## Optimisation-Oriented Block Decomposition

A4SQP should explore block decomposition of optimisation problems, not just
block decomposition of square equation-solving problems.

Many ASCEND models contain calculations that are not part of the true nonlinear
optimisation core:

- upstream calculations that depend only on fixed variables, fixed parameters,
  or already-determined model state;
- downstream presentation calculations that depend on the optimisation result
  but do not feed back into the objective or active relations;
- accounting or reporting expressions, such as annual curtailment, average
  price, aggregate costs, or diagnostic totals;
- initialization-only subproblems that can be solved before the optimisation
  variables are released.

If these regions are included in the NLP sent to A4SQP, they can increase the
number of variables, rows, bounds, derivative entries, and possible failure
points without changing the mathematical optimisation problem. They can also
make the optimisation result sensitive to incidental model initialization or
analysis choices.

The proposed decomposition is:

```text
fixed inputs and fixed parameters
    -> pre-optimisation calculation blocks
        -> core optimisation block
            -> post-processing / presentation blocks
```

The pre-optimisation blocks should be solved before constructing the SQP
problem. Depending on their structure, they might be handled by direct solve,
QRSlv, or a feasibility-only A4SQP solve. Once solved, their outputs become
fixed inputs to the reduced optimisation problem.

The core optimisation block should contain only variables and relations that are
reachable from the unfixed decision variables and relevant to the objective or
active relations.

The post-processing blocks should be solved after the optimisation succeeds, or
after a failed solve if a diagnostic partial result is useful. These blocks
should not influence SQP step computation unless the user explicitly includes
them in the objective or relations.

### Dependency View

The useful decomposition is a dependency/reachability problem:

- Start from unfixed decision variables, the objective, and included
  optimisation relations.
- Identify relations and variables that participate in paths between decisions
  and the objective or optimisation relations.
- Identify upstream variables/relations that can be resolved entirely from fixed
  data before the optimisation.
- Identify downstream variables/relations that are reachable from the core
  solution but do not feed back to the objective or optimisation relations.
- Preserve enough mapping so diagnostics can still be reported in terms of the
  full ASCEND model.

This analysis is different from simply counting free variables and included
relations. A variable may be free in the model but irrelevant to the active
optimisation problem. A relation may be included but only define a reporting
quantity. Conversely, a relation that looks like a calculation may be essential
if it lies on a dependency path to the objective or an optimisation relation.

### Benefits

Optimisation-oriented decomposition could:

- reduce the size of the SQP and QP subproblems;
- reduce derivative evaluation cost;
- avoid optimization failures in reporting-only equations;
- make initialization less dependent on incidental post-processing variables;
- improve diagnostics by separating "core optimisation stuck" from
  "precalculation failed" or "post-processing failed";
- allow different solvers for different parts of the model;
- make warm starts more stable by keeping the core row/column set smaller and
  more semantically stable.

### Interaction With A4SQP

A4SQP should eventually expose decomposition-aware diagnostics:

- pre-optimisation blocks solved, skipped, or failed;
- core optimisation block size and selected rows/columns;
- post-processing blocks deferred;
- variables/relations excluded from the SQP problem and why;
- failed block identity if preprocessing prevents optimisation from starting;
- reduced-problem mapping back to full ASCEND names and indices.

This is another reason to keep the ASCEND integration richer than a bare
IPOPT-like ABI. The standalone A4SQP ABI can solve the reduced problem, but
ASCEND is the layer that knows how to find and explain the reduced problem.

### Visualisation

The optimisation-oriented decomposition should be inspectable in the same spirit
as ASCEND's current incidence matrix and incidence graph tooling.

Useful outputs would include:

- a colour-coded Graphviz incidence graph;
- a matrix-based view of variables versus relations;
- a reduced core matrix showing only rows and columns passed to A4SQP;
- before/after views comparing the full analysed system with the reduced
  optimisation problem;
- labels or legends explaining why each row/column was classified as pre-core,
  core, post-processing, fixed, excluded, or unresolved.

Possible colour classes:

- fixed input or parameter;
- pre-optimisation calculation;
- core optimisation variable or relation;
- active objective or active optimisation relation;
- post-processing or presentation calculation;
- excluded inactive relation or fixed/irrelevant variable;
- elastic or infeasible row after an A4SQP attempt;
- active bound after an A4SQP attempt.

The graph should make directionality visible where possible: upstream
calculation into the core, core into post-processing, and feedback paths that
prevent a row from being safely moved out of the core.

The matrix view should support the same classification using row and column
ordering, colour bands, or block annotations. This would help detect cases where
the decomposition is wrong or too conservative, for example:

- a reporting calculation unexpectedly feeds back into an optimisation relation;
- an apparently fixed pre-calculation actually depends on a decision variable;
- a relation is included but disconnected from the objective and relevant
  optimisation relations;
- a post-processing row is causing a solve failure even though it should have
  been deferred.

This visualisation should be useful before A4SQP is mature. Even a read-only
decomposition report could help users clean up optimisation models and understand
which parts of a large ASCEND model are actually involved in an optimisation.

## IPOPT-Like C ABI

A4SQP should still be designed with a future standalone C ABI in mind. That ABI
should be close to IPOPT's problem callback model:

- number of variables and relations;
- variable lower and upper bounds;
- relation lower and upper bounds;
- objective callback;
- objective gradient callback;
- relation residual callback;
- sparse relation Jacobian structure and values;
- optional Lagrangian Hessian structure and values;
- user data pointer;
- primal and dual solution arrays.

This would make A4SQP usable outside ASCEND and would let the existing ASCEND
IPOPT integration code inform the A4SQP implementation.

The important distinction is that the IPOPT-like ABI should be the portable
solver boundary, not necessarily the richest ASCEND boundary.

## Reuse Plan: IPOPT ABI, `slv_system_t`, and HiGHS

A4SQP should avoid duplicating ASCEND's existing solver-interface work. The
implementation should separate three concerns:

```text
ASCEND model/system integration
    -> sparse NLP problem view
        -> SQP/QP algorithm and HiGHS calls
```

The IPOPT wrapper, the HiGHS wrapper, and the `slv_system_t` layer each contain
pieces that are useful, but none of them alone is the right abstraction for the
whole A4SQP solver.

### Reusable From The IPOPT Integration

The current IPOPT wrapper already demonstrates how to expose an ASCEND model as
a sparse NLP:

- creation of a solver client with a `slv_system_t` back-link;
- use of solver variable and relation lists;
- filters for active, incident, included, solver variables, and non-fixed
  variables;
- objective relation handling;
- variable and relation counting for NLP dimensions;
- model update from a dense `x` vector back into ASCEND variables;
- objective evaluation;
- objective gradient evaluation;
- relation residual evaluation;
- sparse Jacobian structure and value callbacks;
- optional Hessian-of-the-Lagrangian callback;
- derivative safe-evaluation handling;
- parameter registration through ASCEND's solver parameter mechanism;
- solve status reporting through `slv_status_t`.

The most valuable reusable concept is not the IPOPT-specific `IpoptProblem`
object. It is the adapter pattern:

```text
ASCEND solver lists + relman/var data
    -> n, m, bounds, residuals, objective, sparse Jacobian, optional Hessian
```

A4SQP should factor this into a common sparse NLP view where practical, rather
than copying IPOPT callback code into a second solver.

Likely reusable or generalisable code areas:

- variable vector packing and unpacking;
- relation residual evaluation;
- objective evaluation and objective sense handling;
- Jacobian nonzero counting and sparsity extraction;
- Jacobian value evaluation via `relman_diff2_rev` or equivalent;
- optional Hessian evaluation via `relman_hess`;
- ASCEND solver parameter definitions for derivative safety, scaling, maximum
  iterations, tolerances, and output verbosity;
- status and diagnostic reporting patterns.

### Limits Of The IPOPT-Like ABI

The IPOPT-style problem ABI is a good portable minimum, but it does not naturally
represent all information that A4SQP may want from ASCEND:

- solver-list provenance and master-list provenance;
- block and ordering information;
- fixed/included/active/incident flag history;
- conditional-model reanalysis state;
- ASCEND-specific warm-start invalidation;
- per-row and per-variable diagnostic flags;
- user-facing mapping from reduced rows/columns back to model instances;
- decomposition decisions such as pre-core, core, post-processing, or excluded.

Therefore the IPOPT-like ABI should be used as the external compatibility layer,
while the ASCEND client should populate richer optional metadata when it is
available.

### Reusable From The HiGHS Integration

The current HiGHS wrapper already contains useful infrastructure for turning an
ASCEND system into a solver-ready linear model:

- HiGHS build detection and linking;
- HiGHS solver parameter handling;
- variable bound extraction and bound sanity checks;
- relation operator extraction;
- linear matrix assembly for LP/MIP models;
- nominal scaling support for rows and columns;
- variable type handling for LP/MIP;
- HiGHS option application and read-back checks;
- progress and interrupt callback handling;
- passing LP/MIP models to HiGHS;
- fetching primal and dual solution arrays;
- translating HiGHS solve status back into ASCEND status and diagnostics.

A4SQP should reuse the HiGHS configuration, option, progress, interrupt, status,
and solution-reporting patterns where possible.

The existing LP/MIP matrix assembly is useful as a reference, but A4SQP cannot
use it directly for the SQP subproblem because the SQP QP is rebuilt from a
linearization at each iterate.

### New Pieces Needed For HiGHS QP

A4SQP needs a new QP assembly layer. This layer should build a convex QP in
HiGHS terms from the current SQP iterate:

- step variables `p`, not just absolute model variables `x`;
- optional elastic variables for lower and upper row violations;
- linearized relation matrix `J_r(x_k)`;
- linearized row bounds `r_L - r(x_k)` and `r_U - r(x_k)`;
- step bounds `x_L - x_k <= p <= x_U - x_k`;
- QP linear objective `grad f(x_k)^T p`;
- QP Hessian `B_k`, initially diagonal, BFGS, or limited-memory-derived;
- elastic penalties in the linear objective;
- optional trust-region or maximum-step bounds;
- mapping from QP columns back to ASCEND variables and elastic rows;
- mapping from QP rows back to ASCEND relations;
- extraction of QP primal step, row duals, bound duals, and elastic values.

The QP layer also needs to handle solver mechanics:

- create and destroy a HiGHS model for each SQP iteration, or efficiently update
  an existing model if the HiGHS API makes that worthwhile;
- pass Hessian data to HiGHS in the format required by its QP interface;
- ensure the Hessian supplied to HiGHS is positive semidefinite or regularized;
- configure HiGHS for continuous convex QP, not LP/MIP;
- capture QP statuses separately from outer SQP statuses;
- decide whether and how HiGHS QP warm-start state can be reused.

### New Pieces Needed For SQP

The main A4SQP work is the SQP algorithm around the QP solver:

- SQP iteration driver;
- convergence tests for feasibility, stationarity, complementarity, step size,
  and objective/merit progress;
- elastic penalty update strategy;
- L1 merit-function line search;
- step acceptance and rollback into ASCEND variables;
- BFGS or limited-memory BFGS update;
- Hessian regularization;
- restoration or least-infeasible behavior when progress stalls;
- warm-start storage and invalidation;
- diagnostic accumulation for rows, variables, active bounds, elastic rows, and
  failed QP subproblems.

This algorithmic layer should not depend directly on `slv_system_t`. It should
depend on the A4SQP problem view. The ASCEND client and future IPOPT-like C ABI
should both be able to populate that view.

### Extra ASCEND-Specific Value

The ASCEND client can add value beyond the generic A4SQP core by:

- preserving meaningful row and variable identities;
- storing or exposing solver-state diagnostics in ASCEND terms;
- reporting failed rows, elastic rows, and active bounds by model name;
- using `slv_status_t` block/status fields where they fit;
- adding A4SQP-specific diagnostic structures where standard status fields are
  not expressive enough;
- supporting incidence graph and matrix visualisation of the reduced problem and
  failed solve state;
- deciding when the ASCEND system has changed enough to invalidate warm-start
  state.

This is the argument for a two-layer design rather than a pure IPOPT-like
callback implementation.

## Scaling

A4SQP should treat scaling as a first-class design issue. Poor scaling can make
the QP subproblem misleading, distort active-set decisions, and make elastic
penalties difficult to tune.

ASCEND already has useful scaling concepts:

- variable nominal values via `var_nominal`;
- relation nominal values via `rel_nominal`;
- relation nominal calculation via `relman_scale`;
- QRSlv options for variable nominal, relation nominal, row norm, and iterative
  scaling;
- HiGHS LP/MIP nominal scaling support in `lp_apply_nominal_scaling`.

The IPOPT wrapper appears to rely mainly on IPOPT's own NLP scaling behaviour
and does not obviously reuse QRSlv-style relation scaling. A4SQP should not
assume that is sufficient, because the QP backend and elastic penalty logic will
see the scaled or unscaled rows directly.

The first implementation should follow QRSlv rather than inventing a new scaling
vocabulary. QRSlv scales variables by nominal values and scales relation rows
using either row two-norms, relation nominals, optional Fourer-style iterative
scaling, or no scaling. A4SQP should expose a compatible subset first, then add
extra SQP-specific interpretation only where needed.

The proposed first-stage policy:

- store `var_scale` and `rel_scale` in `A4SqpMeta`;
- default variable scale from safe `var_nominal`;
- default relation scale according to the selected QRSlv-style mode: none,
  Jacobian row two-norm, relation nominal via `rel_nominal`/`relman_scale`, and
  later iterative scaling;
- assemble the SQP/QP problem in scaled coordinates or apply equivalent scaling
  consistently to bounds, residuals, Jacobian rows/columns, objective gradient,
  and multipliers;
- report diagnostics in both scaled and unscaled terms where useful;
- add solver parameters using QRSlv-compatible names or values where practical,
  so existing ASCEND users do not have to learn a second scaling language.

The exact convention should be fixed early. A reasonable convention is:

```text
x = S_x z
r_scaled(z) = S_r^{-1} r(S_x z)
J_scaled = S_r^{-1} J S_x
```

In this convention, the QP works with scaled step variables `p_z`. The ASCEND
adapter packs/unpacks physical `x` values, while the A4SQP core sees scaled
vectors. Multipliers must be converted back before reporting if they are exposed
in unscaled ASCEND terms.

Scaling should be tested independently using small problems where the same model
is written with deliberately different units and nominal values.

## Architectural Options

### Option A: IPOPT-Like Core First

In this approach, A4SQP is primarily a standalone solver library with an
IPOPT-like C ABI. ASCEND supplies callbacks through an adapter.

Advantages:

- Easier to package for third-party applications.
- Cleaner separation between ASCEND and A4SQP.
- Existing IPOPT integration code can be generalized.
- Unit tests can use small standalone callback problems.
- The solver design is less coupled to ASCEND internals.

Disadvantages:

- ASCEND-specific metadata is either lost or must be awkwardly reconstructed.
- Warm-start validation is harder if row and column identity are just callback
  indices.
- Block/decomposition information is not naturally represented.
- Diagnostics may be less meaningful unless names and source mappings are added
  separately.
- Conditional modeling, active/included flags, and solver-list semantics can
  become flattened too early.
- Returning useful post-failure state to ASCEND becomes an afterthought unless
  diagnostic extension hooks are designed from the beginning.

### Option B: ASCEND-Native Solver First

In this approach, A4SQP is first implemented as a normal ASCEND solver client
over `slv_system_t`. The standalone ABI is added after the ASCEND prototype is
working.

Advantages:

- Makes full use of ASCEND's existing system analysis.
- Preserves solver-list ordering, `sindex` mapping, flags, and names.
- Better diagnostics for failed feasibility and elastic rows.
- Easier to respect fixed/included/active state and conditional model changes.
- Warm-start invalidation can use ASCEND system identity and list state.
- More likely to produce useful results on real ASCEND models quickly.

Disadvantages:

- Higher risk of coupling A4SQP algorithm code to ASCEND internals.
- Third-party packaging may be delayed or more difficult.
- The solver core may accidentally depend on ASCEND data structures.
- More discipline is required to keep the algorithm portable.

### Option C: Two-Layer Design

This is the preferred direction.

```text
ASCEND slv_system_t
    -> A4SQP ASCEND client
        -> A4SQP problem view
            -> A4SQP core SQP engine
                -> HiGHS QP

External application
    -> IPOPT-like A4SQP C ABI
        -> A4SQP problem view
            -> A4SQP core SQP engine
                -> HiGHS QP
```

The A4SQP core should operate on a solver-neutral problem view. The ASCEND
client should populate that view from `slv_system_t`. The future IPOPT-like C
ABI should populate the same view from callbacks.

This gives ASCEND the benefit of tight integration while keeping the core solver
packagable.

## Optional Extensions Beyond IPOPT's Problem API

These extensions are not strictly required for SQP, but they may make A4SQP more
effective in ASCEND and easier to diagnose.

They should be optional metadata on the A4SQP problem view, not mandatory inputs
for the standalone C ABI.

Problem identity and mapping:

- Stable row and column identifiers, distinct from integer positions.
- Human-readable row and column names.
- Source mappings back to ASCEND variables, relations, objectives, and bounds.
- Warm-start generation identifiers.

Problem structure:

- Row and column scaling data.
- Nominal values and preferred perturbation scales.
- Fixed, active, incident, included, and invariant flags.
- Variable and relation type information.
- Linearity classification for rows and objective.
- Block partition information from ASCEND.
- Optimisation dependency partition: pre-calculation, core optimisation,
  post-processing, and excluded rows/columns.
- Tearing or ordering information, if available and useful.

Solver state and diagnostics:

- Previous active bound and active row state.
- Previous QP basis or active-set state, if available from HiGHS.
- Previous BFGS or L-BFGS state.
- Elastic row history and infeasibility diagnostics.
- Per-row and per-variable SQP diagnostic flags.
- Interrupt/progress callback hooks consistent with ASCEND solver reporting.

The key design question is which of these extensions materially improves solver
behavior, and which merely makes implementation more ASCEND-specific. Early
implementation should add only the extensions needed for diagnostics,
warm-start validity, scaling, and stable mapping.

## Warm Starts

SQP warm starts should preserve:

- primal variable values;
- relation multipliers;
- variable bound multipliers;
- active row and bound status;
- Hessian approximation state;
- QP solver state where available;
- penalty or filter state where appropriate.

Warm starts should be invalidated, partly or fully, when ASCEND changes the
solver lists, row or column ordering, included/fixed/active flags, sparsity
pattern, objective, or conditional model configuration.

A conservative policy is:

- keep primal values whenever dimensions still match;
- keep multipliers only when row and column identities match;
- keep active-set and QP state only when row/column identities, bounds, and
  sparsity are unchanged;
- keep Hessian approximation only when variable identities and scaling are
  unchanged.

### Warm Starts And The ASCEND Solver API

Warm start has two different meanings depending on whether A4SQP is being used
through ASCEND's `slv_system_t` API or through a future standalone C ABI.

In the ASCEND API:

- `slv_presolve` prepares a solver client for a fixed analysed system.
- `slv_iterate` asks the selected solver to perform one iteration and leave
  model-visible state available for inspection.
- `slv_solve` asks the solver to iterate until convergence, failure, or limits.
- `slv_resolve` is the efficient re-prepare path when only allowed quantities
  changed, such as variable values, variable nominals, variable bounds, or solver
  parameters.

For A4SQP, `slv_iterate` should mean one outer SQP major iteration, including
QP construction, QP solve, line search, model update, and diagnostic update. It
should write enough state back to `var_variable`, `rel_relation`, `slv_status_t`,
and/or A4SQP diagnostic storage for the user or GUI to inspect the current
iterate.

`slv_solve` should be implemented as the automated loop over the same iteration
logic. It should not be a separate algorithmic path. This keeps interactive
debugging and batch solving consistent:

```text
slv_solve(sys) ~= while ready_to_solve: slv_iterate(sys)
```

Within a single presolved solver client, repeated `slv_iterate` calls are not a
warm start in the usual sense. They are simply continuation of the same live SQP
session. The active set, BFGS/L-BFGS state, elastic penalty state, and QP
diagnostic state remain in the solver client token.

Warm start becomes relevant when a new solve starts from a previous solve's
state:

- the user solved once, changed allowed values/bounds/parameters, then called
  `slv_resolve` and `slv_solve`;
- the user is performing continuation or parameter sweeps;
- a solve failed, the user inspected diagnostics, adjusted initial values or
  bounds, then requested another solve without rebuilding the whole system;
- the future standalone C ABI caller explicitly passes previous primal/dual and
  active-set state to A4SQP.

For ASCEND users, the basic primal warm start is already natural: current
variable values in the model become the next starting point. The additional
A4SQP value is preserving non-primal state in the solver client:

- relation multipliers;
- variable bound multipliers;
- active relation and active bound state;
- BFGS/L-BFGS approximation state;
- elastic penalty state;
- possibly QP backend warm-start state.

A4SQP's `resolve` method should therefore be a key part of warm-start support.
It should inspect what changed since the previous solve and decide which pieces
of state remain valid. If `slv_presolve` is called after structural changes, the
safe assumption is that most warm-start state is invalid unless stable row and
column identities prove otherwise.

The future standalone C ABI should expose explicit warm-start calls because it
does not have ASCEND's persistent solver client and `slv_resolve` convention:

```text
A4SqpSetWarmStart(...)
A4SqpGetWarmStart(...)
```

Those functions are mainly for non-ASCEND callers and for tests. The ASCEND
path should make warm starts feel like normal solver reuse: preserve state
across `slv_resolve` when valid, and expose enough diagnostics for users to
understand when state was discarded.

## Phase Plan

### Phase 1: Elastic Line-Search SQP Prototype

- Implement A4SQP as an ASCEND solver client.
- Build a reusable A4SQP problem view from `slv_system_t`.
- Restrict Phase 1 to smooth continuous optimisation: no discrete variables,
  logical relations, conditional switching, or active model-structure changes.
  Unsupported structures should be rejected with clear diagnostics.
- Generate QP subproblems with elastic variables.
- Call HiGHS for QP solves.
- Use BFGS, L-BFGS, diagonal, or identity Hessian approximations.
- Use an L1 merit-function line search.
- Report active bounds, elastic relations, infeasibility, and progress.
- Use native ASCEND `.a4c` test models first, so that the initial tests exercise
  the real `slv_system_t` integration rather than a parallel standalone path.

The first coding milestone should be narrower than "solve NLPs": construct,
evaluate, scale, and validate an A4SQP problem view from `slv_system_t`. The
minimum acceptance criteria are:

- solver var and relation counts match the selected ASCEND solver lists;
- variable bounds, fixed flags, nominals, values, and names are captured;
- relation operators map to row bounds correctly;
- objective value and gradient are available;
- relation residuals and Jacobian sparsity/values are available;
- scaling vectors match the selected QRSlv-style scaling mode;
- unsupported model features produce user-facing diagnostics;
- progress, trace, and developer diagnostics can be emitted without a QP solve.

Suggested Phase 1 native test models:

- unconstrained scalar objective;
- equality-constrained small NLP;
- one-sided inequality relation;
- bounded variable with an active bound at the solution;
- deliberately unsupported discrete/logical/switching feature;
- small scaling/nominal regression case.

#### Phase 1 Coding Checklist

Phase 1 should be implemented as a sequence of narrow, testable steps. The
early steps should prove that A4SQP sees the same problem ASCEND sees before any
SQP algorithm behaviour is introduced.

1. Build-system skeleton.

   - Add `solvers/a4sqp/SConscript`.
   - Add the `a4sqp` solver directory to `solvers/SConscript`.
   - Add `A4SQP` to the top-level `WITH_SOLVERS` list.
   - Gate A4SQP on HiGHS availability, probably by requiring `WITH_HIGHS`.
   - Add a stub solver registration so ASCEND can list/select A4SQP.

2. Source skeleton.

   Initial files should be small and deliberately separated by responsibility:

   ```text
   solvers/a4sqp/a4sqp.c              ASCEND solver registration and slv hooks
   solvers/a4sqp/a4sqp.h              internal public declarations
   solvers/a4sqp/a4sqp_params.c       solver parameters
   solvers/a4sqp/a4sqp_params.h
   solvers/a4sqp/a4sqp_ascend.c       slv_system_t adapter
   solvers/a4sqp/a4sqp_ascend.h
   solvers/a4sqp/a4sqp_view.c         problem-view allocation/validation
   solvers/a4sqp/a4sqp_view.h
   solvers/a4sqp/a4sqp_scale.c        QRSlv-style scaling
   solvers/a4sqp/a4sqp_scale.h
   solvers/a4sqp/a4sqp_diag.c         diagnostics and progress reporting
   solvers/a4sqp/a4sqp_diag.h
   solvers/a4sqp/a4sqp_qp_highs.c     HiGHS QP backend experiments
   solvers/a4sqp/a4sqp_qp_highs.h
   ```

   BFGS, merit, and line-search files can wait until the problem view and QP
   backend are testable.

3. Minimal solver lifecycle.

   - Implement create/destroy/update hooks with no optimisation algorithm yet.
   - Implement parameter defaults, including safe calculation, scaling mode,
     verbosity, and progress reporting.
   - Implement `slv_presolve`/setup logic that builds the A4SQP problem view.
   - Make `slv_iterate` perform one "view evaluation" pass at first: evaluate
     residuals, objective, gradients/Jacobian, scaling, and diagnostics.
   - Make `slv_solve` call the same internal path rather than introduce a
     separate evaluation path.

4. Problem-view data model.

   Define an internal `A4SqpView` or `A4SqpProblemView` with:

   - solver variable count and relation count;
   - maps from A4SQP column/row index to ASCEND `sindex`;
   - pointers back to `var_variable` and `rel_relation` where appropriate;
   - variable values, lower bounds, upper bounds, nominals, and fixed flags;
   - relation residuals, lower row bounds, upper row bounds, operators, and
     included flags;
   - objective relation/index and objective gradient;
   - Jacobian sparsity and values;
   - scaling vectors for variables and relations;
   - diagnostic side tables for active bounds, elastic flags, worst residuals,
     evaluation failures, and unsupported features.

5. ASCEND relation-to-row-bound mapping.

   The adapter should map current ASCEND binary relation operators into the
   internal row-bound representation:

   ```text
   LHS =  RHS      r = LHS - RHS,    rel_l = 0,    rel_u = 0
   LHS <= RHS      r = LHS - RHS,    rel_l = -inf, rel_u = 0
   LHS >= RHS      r = LHS - RHS,    rel_l = 0,    rel_u = +inf
   LHS <  RHS      r = LHS - RHS,    rel_l = -inf, rel_u = 0
   LHS >  RHS      r = LHS - RHS,    rel_l = 0,    rel_u = +inf
   ```

   Strict inequalities should probably be accepted initially as their tolerant
   non-strict equivalents, with a diagnostic note if needed. `<>` should be
   rejected for Phase 1.

6. Derivative and residual evaluation.

   - Reuse relman routines for residual and Jacobian evaluation.
   - Preserve ASCEND's residual meaning: `LHS - RHS` regardless of comparison.
   - Store both raw residuals and scaled residuals.
   - Capture derivative evaluation failures by row and variable where possible.
   - Confirm Jacobian sparsity can be requested separately from numeric values.
   - Add a debug dump option for dimensions, bounds, residuals, and Jacobian
     nonzeros.

7. QRSlv-style scaling.

   - Implement `NONE`, variable nominal scaling, row two-norm relation scaling,
     and relation nominal scaling first.
   - Defer iterative/Fourer scaling until the basic scaling tests pass.
   - Use safe fallbacks for zero, negative, or non-finite nominals.
   - Report bad scaling through `ERROR_REPORTER_*` when user action may be
     needed, and through `CONSOLE_DEBUG` for detailed developer traces.
   - Test that scaled row bounds, residuals, and Jacobian entries are consistent
     with the selected convention.

8. Diagnostics and progress.

   - Add a small A4SQP diagnostic/report helper rather than scattering
     `ERROR_REPORTER_*` calls through all files.
   - Use `ERROR_REPORTER_*` for user-facing diagnostics.
   - Use `CONSOLE_DEBUG` for developer traces.
   - Use `slv_report_progress("A4SQP", message)` for progress messages.
   - Poll `slv_get_solver_interrupt()` in long loops.
   - Record why setup failed: unsupported relation, missing objective, bad
     derivative, non-finite residual, bad bounds, or unsupported discrete state.

9. Native ASCEND tests.

   Add tests under `ascend/solver/test/test_a4sqp.c` and models under
   `models/test/a4sqp/`. Initial tests should assert construction and
   evaluation, not optimisation success:

   - variable/relation counts and row-bound mapping;
   - objective and objective-gradient extraction;
   - residual values for `=`, `<=`, and `>=`;
   - Jacobian sparsity and numeric values on tiny models;
   - variable and relation scaling;
   - rejection of unsupported logical/discrete/switching cases;
   - progress callback can receive at least one A4SQP progress message.

10. Hand-built HiGHS QP spike.

    After the problem view is testable, add the smallest possible HiGHS QP call
    independent of the full SQP loop:

    - one convex quadratic objective;
    - variable bounds;
    - one linear row;
    - one elastic lower/upper slack pair;
    - extraction of primal step, row multipliers, bound multipliers, and elastic
      values where HiGHS exposes them.

    This should prove the QP backend interface before it is connected to
    nonlinear SQP iteration.

    Current spike status:

    - `a4sqp_qp_highs_spike` builds a tiny convex QP through the HiGHS C API.
    - The fixture has one step variable, two elastic slack variables, sparse
      column-wise row data, a triangular Hessian, variable bounds, row bounds,
      and linear elastic penalties.
    - The expected solution is deterministic: the step hits its upper bound and
      the remaining row infeasibility is represented by a lower elastic slack.
    - The CUnit test calls the exported function from the loaded A4SQP shared
      module, so the test exercises the same dynamic module and HiGHS link path
      that ASCEND will use.
    - Verified with `./a4 cutest solver_a4sqp`.

11. First SQP iteration.

    Only after the view and QP spike pass tests, implement one major SQP
    iteration:

    - assemble QP from current residuals and Jacobian;
    - use identity or diagonal Hessian approximation initially;
    - solve the QP with HiGHS;
    - compute candidate step and predicted reduction;
    - update diagnostics, but do not yet attempt a sophisticated line search.

    Current first-iteration status:

    - `A4SqpQp` now stores a reusable HiGHS-ready QP: column metadata, row
      metadata, costs, bounds, sparse column-wise matrix, triangular Hessian,
      primal/dual solution arrays, and HiGHS status fields.
    - `a4sqp_qp_build_from_view` assembles an elastic QP from the scaled
      `A4SqpView`.
    - Step-variable bounds are built as scaled variable bounds relative to the
      current scaled value.
    - Relation row bounds are built as scaled relation bounds relative to the
      current scaled residual.
    - Every relation currently receives lower and upper elastic slack columns
      with the default elastic penalty.
    - The initial Hessian approximation is identity on step variables and zero
      on elastic variables.
    - Scaled objective gradients are now extracted from ASCEND objective
      relations and used as step-variable linear costs.
    - `slv_iterate` now builds and solves this QP with HiGHS, then runs a
      minimal merit-decreasing line search and stores QP/line-search diagnostics
      in the A4SQP solver client state.
    - Repeated solve looping, basic convergence tests, and first-pass progress
      reporting are now implemented.
    - Predicted reduction, adaptive penalty updates, active-set/multiplier
      diagnostics, and richer failure reporting remain next steps.
    - Verified with `./a4 cutest solver_a4sqp`.

12. Minimal elastic line-search solve.

    The first real solve loop should add:

    - L1 merit function;
    - Armijo-style backtracking line search;
    - elastic penalty parameter and update rule;
    - convergence checks for scaled feasibility, stationarity proxy, step size,
      and iteration limit;
    - user-facing final status explaining success, infeasibility, derivative
      failure, QP failure, line-search failure, or interruption.

    Current line-search status:

    - The merit function is `objective + rho * scaled-row-violation` when an
      objective exists, and `rho * scaled-row-violation` for feasibility-only
      systems.
    - Trial steps are unpacked from scaled QP step variables back to physical
      ASCEND variable values using variable scales.
    - Each trial point rebuilds the A4SQP view through `slv_system_t`, so
      residuals, objective value, gradients, Jacobian, and scaling are refreshed
      at the trial point.
    - Rejected trial points are overwritten from saved original variable values;
      if the search fails, the original model state is restored and the view is
      rebuilt.
    - The line search now computes a predicted merit reduction from the QP
      model. The current model comparison is:
      `current_objective + rho * current_violation` minus
      `current_objective + QP_objective`, where `QP_objective` contains
      `g'p + 1/2 p'Bp + rho * linearized_elastic_violation`.
    - Step acceptance now uses an Armijo-style test:
      actual merit decrease must be at least
      `armijo_coeff * alpha * predicted_reduction`, with `merit_tol` as a
      numerical floor. If the QP does not predict meaningful reduction, A4SQP
      falls back to requiring a strict `merit_tol` decrease.
    - `basic_view.a4c` now exercises a nonzero objective-gradient QP step,
      backtracking acceptance, merit decrease, and updated objective value.
    - Solver parameters now include `max_iter`, `max_backtrack`, `feas_tol`,
      `step_tol`, `merit_tol`, `armijo_coeff`, and `elastic_penalty`, in
      addition to the earlier safe-evaluation, scaling, verbosity, progress,
      and view-dump controls.
    - `slv_solve` now loops over major SQP iterations until convergence,
      iteration limit, QP failure, or line-search failure. Convergence currently
      uses maximum scaled relation violation plus accepted physical step norm.
    - Iteration diagnostics are stored in the A4SQP solver client state:
      objective value, merit before/after, predicted reduction, model merit
      after, linearized elastic violation, violation sum, maximum violation,
      worst relation row, accepted step norm, accepted line-search alpha, and
      line-search failure flag.
    - Progress messages can be emitted through ASCEND progress callbacks and,
      optionally, through the error reporter.
    - The native CUnit test now includes a repeated `slv_solve` case for
      `basic_view.a4c`, checking final solver status, iteration count,
      feasibility, small-step convergence, and loose agreement with the known
      analytic solution.
    - Verified with `./a4 cutest solver_a4sqp`.

Phase 1 should be considered complete when A4SQP can build and inspect the
problem view for native ASCEND NLP examples, solve a few tiny smooth continuous
examples, and provide useful diagnostics on deliberately unsupported or
infeasible examples. Performance tuning, warm starts, decomposition, filter SQP,
and standalone ABI work should remain out of scope until this baseline is
stable.

### Phase 2: Robustness and Diagnostics

Phase 2 should turn the Phase 1 prototype from "can solve selected examples"
into "can explain and improve behaviour on difficult ASCEND models." This phase
is less about adding new mathematical features and more about making the solver
diagnosable, repeatable, and robust under realistic modelling conditions.

#### Scaling

Phase 2 should settle the scaling convention and make it visible in diagnostics:

- implement variable and relation scaling using `var_nominal`, `rel_nominal`,
  and/or `relman_scale`;
- add parameters for scaling mode and update frequency;
- apply scaling consistently to relation residuals, Jacobians, bounds, QP
  Hessians, gradients, elastic penalties, and multipliers;
- report scaled and unscaled infeasibility norms;
- identify badly scaled variables, relations, and Jacobian columns/rows;
- add regression tests where unit changes or nominal changes should not change
  solver behaviour materially.

This should be done before tuning elastic penalties heavily, because penalty
behaviour is difficult to interpret if rows have inconsistent magnitudes.

#### Warm Starts

Warm starts should become a real feature in Phase 2:

- persist primal values, relation multipliers, bound multipliers, active
  row/bound state, and BFGS/L-BFGS state;
- detect when row/column identities, bounds, objective, sparsity, scaling, or
  included/fixed/active flags have changed;
- define partial invalidation rules rather than a single all-or-nothing warm
  start;
- report why warm-start state was accepted, partially accepted, or rejected;
- test continuation, repeated solve, parameter sweep, and reinitialisation
  workflows.

If HiGHS exposes useful QP warm-start or basis/active-set state, A4SQP should
experiment with preserving it across SQP iterations. If not, the first warm
start value is still in the NLP/SQP-level state.

#### Feasibility Restoration

The Phase 1 elastic formulation should already avoid many hard QP infeasibility
failures. Phase 2 should make that behaviour more deliberate:

- track elastic variables by relation and side;
- update elastic penalties using a clear rule;
- detect persistent elastic rows and report them as candidate model issues;
- add a feasibility-only mode that temporarily ignores or downweights the
  objective;
- add a least-infeasible termination mode when the model appears infeasible;
- distinguish "QP infeasible", "elastic but no progress", "line search failed",
  and "derivative evaluation failed" in status reporting.

The goal is that an unsuccessful solve leaves a useful map of blocking
relations and bounds rather than a generic failure message.

#### Solver-State Feedback

Phase 2 should define which solver state is exposed to ASCEND and the GUI:

- active variable lower/upper bounds;
- active relation lower/upper bounds;
- elastic relation lower/upper violations;
- clipped variable steps;
- large relation and bound multipliers;
- worst residuals and worst scaled residuals;
- relation satisfaction state at the selected tolerance;
- last QP status and last line-search status.

Early implementations can keep this state in A4SQP side tables. Phase 2 should
decide which states should be promoted into `slv_system_t`-visible storage or
existing `var_variable` / `rel_relation` flags so that GUI tools can expose them
consistently.

#### Elastic Penalties And Elastic Violations

Elastic SQP introduces two related but different quantities:

- elastic violation variables, which are per-relation, per-side outputs from a
  QP subproblem;
- elastic penalty weights, which are solver parameters or adaptive solver state
  used to discourage those violations.

The GUI-visible quantity that most directly helps users is usually the elastic
violation, not the penalty. If a relation is elastic on its lower or upper side,
and by how much, that points directly to the model location blocking feasibility.
This is analogous to exposing relation residuals and multipliers.

ASCEND already has a precedent for relation-level solver output. Relation
residual, multiplier, and nominal values are stored in the backend `struct
relation` and accessed through `rel_residual`, `rel_multiplier`, and
`rel_nominal`. These are not ordinary user-declared sub-atoms in the same sense
as a solver variable's `lower_bound` or `upper_bound`, but they are
relation-associated values that the GUI and instance inspection tools can expose.
The `included` state is different again: it is a relation child value mirrored
into `REL_INCLUDED`.

Possible storage choices for A4SQP elastic data:

- keep per-relation elastic lower/upper violation values in an A4SQP diagnostic
  side table owned by the solver client or `slv_system_t`;
- add generic relation diagnostic fields to ASCEND's relation backend, analogous
  to residual and multiplier;
- add solver-specific relation child attributes, if ASCEND supports doing this
  cleanly for relation instances;
- expose elastic data only through A4SQP diagnostic queries and reports.

The side-table approach is best for the prototype because elastic data is
solver-specific and transient. It also supports richer numeric state without
changing low-level relation structures immediately:

```text
elastic_lower[rel_sindex]
elastic_upper[rel_sindex]
elastic_penalty[rel_sindex]    optional, if penalties become per-relation
elastic_status[rel_sindex]     none, lower, upper, both
```

For longer-term GUI integration, the strongest candidates for promotion are:

- `elastic_lower` and `elastic_upper` as relation-associated output values;
- boolean flags equivalent to `REL_ELASTIC_LOWER` and `REL_ELASTIC_UPPER`;
- perhaps a relation "worst infeasibility" or "blocking" status.

Whether these should become fields on `struct relation`, flags on
`rel_relation`, or a `slv_system_t` diagnostic table is an implementation
decision. The design requirement is that the GUI should be able to present these
values next to the relation residual, multiplier, included state, and satisfied
state after an A4SQP solve or failed iteration.

The existing relation `residual` should probably remain the raw evaluated
relation residual, not be overwritten with elastic violation. For equalities the
two are closely related, but for bounded or inequality relations they are not the
same quantity: a relation can have a nonzero residual and still satisfy its row
bounds, or it can violate only the lower side or only the upper side. A flag can
make the active elastic side visible, but it cannot by itself preserve the raw
residual, the signed distance to each bound, and the elastic slack value. A4SQP
should therefore compute elastic violation from residual plus relation bounds
and store/display it as a separate relation diagnostic. The GUI can then show
both values: residual as "what the equation currently evaluates to" and elastic
violation as "how much this relation had to be softened for the SQP step".

In current ASCEND terminology, `relman_eval` returns `LHS - RHS` regardless of
the comparison operator. That means a `<=' relation can have a negative residual
and be perfectly feasible, while a `>=' relation can have a positive residual
and be perfectly feasible. Elastic violation is a derived, one-sided distance to
the feasible side of the relation:

```text
LHS = RHS       lower_violation = max(0, -residual)
                upper_violation = max(0,  residual)

LHS <= RHS      lower_violation = 0
                upper_violation = max(0,  residual)

LHS >= RHS      lower_violation = max(0, -residual)
                upper_violation = 0
```

For a future ranged-row representation, where `rel_l <= r(x) <= rel_u`, both
sides should be kept explicitly:

```text
lower_violation = max(0, rel_l - r(x))
upper_violation = max(0, r(x) - rel_u)
```

Current ASCEND model syntax does not appear to provide a direct way to write a
single double-sided algebraic relation of the form `rel_l <= LHS - RHS <=
rel_u`. The parser grammar represents a relation as `expr relop expr`, with
`=`, `<`, `>`, `<=`, `>=`, and `<>` as the relation operators. Therefore the
ASCEND-native adapter should initially map ordinary ASCEND relations into row
bounds as follows:

```text
LHS =  RHS      r(x) = LHS - RHS,    rel_l = 0,    rel_u = 0
LHS <= RHS      r(x) = LHS - RHS,    rel_l = -inf, rel_u = 0
LHS >= RHS      r(x) = LHS - RHS,    rel_l = 0,    rel_u = +inf
```

Ranged rows remain useful in the A4SQP problem view because HiGHS, QP solvers,
and IPOPT-like APIs naturally represent rows with lower and upper bounds. They
also provide a clean future representation for external users of the standalone
A4SQP ABI. In native ASCEND models, users can express a two-sided relation today
as two separate relations, or sometimes by introducing an intermediate variable
with lower and upper variable bounds:

```text
q = LHS - RHS;
q.lower_bound := rel_l;
q.upper_bound := rel_u;
```

The two-relation form is simpler and requires no new ASCEND language feature,
but it gives A4SQP two relation rows rather than one ranged row. Detecting and
coalescing such pairs is possible in principle, but should not be a Phase 1
requirement.

Elastic penalty weights are less clearly part of the model state. A global
penalty is just a solver parameter or algorithm state. If penalties become
per-relation and adaptive, exposing them may be useful for deep debugging, but
they should be secondary to the elastic violation values themselves.

#### QP Backend Reuse

Phase 2 should profile and improve the HiGHS interaction:

- determine whether rebuilding the QP each SQP iteration is acceptable;
- evaluate HiGHS model update APIs if available;
- test Hessian formats and sparse Hessian assembly cost;
- test regularization choices when BFGS updates become indefinite;
- preserve QP model mappings for better diagnostics;
- add QP-level timing and status reporting.

This should remain secondary to correctness and diagnostics. Premature QP
update optimisation is not useful until the SQP behaviour is stable.

#### Decomposition And Visualisation

Phase 2 should add read-only decomposition and visualisation even before any
automatic pre/post solve workflow:

- report the reduced A4SQP row/column set versus the full ASCEND solver system;
- mark rows and columns by fixed, active, included, core, excluded, elastic, and
  bound-active state;
- add Graphviz and/or matrix visualisation for decomposition and failure review;
- explain why a row or variable was included or excluded;
- compare the A4SQP problem view against IPOPT's problem construction where
  possible.

The first visualisation target should be debugging, not automatic
optimisation-oriented decomposition.

#### Test Cases

Phase 2 should introduce tests based on real failure modes:

- small synthetic NLPs with active bounds and known solutions;
- infeasible or nearly infeasible relation sets;
- badly scaled models;
- hard ASCEND initialization cases;
- repeated solves with small parameter changes;
- models where reporting-only relations should not affect the core solve;
- derivative failure and safe-evaluation cases.

Each difficult test should assert not only solve success/failure, but also the
quality of diagnostics: which relation was elastic, which bound was active, or
why the warm start was invalidated.

CUTEst/CUTEr-style tests should be considered after the native ASCEND problem
view tests are stable. CUTEst is a well-known constrained and unconstrained
optimisation test environment, and PyCUTEst gives Python access to the CUTEst
collection with generated C access to the underlying Fortran package. S2MPJ is
also relevant because it provides CUTEst-derived problems in native Matlab,
Python, and Julia forms. These are useful for benchmarking SQP behaviour and
diagnostics, but they should not be the first acceptance tests for the ASCEND
adapter because they exercise a different problem-loading path.

Candidate external-test route:

- first, run selected CUTEst/S2MPJ problems through the future standalone
  A4SQP problem-view or ABI path;
- later, translate a small curated subset into `.a4c` models where the model
  structure resembles ASCEND process/fluid/flowsheet use cases;
- choose tests that exercise active inequalities, bound activity, infeasible or
  nearly infeasible starts, bad scaling, and expected elastic-restoration
  behaviour, not just final objective values.

Useful references:

- CUTEst: Gould, Orban, and Toint, "CUTEST: a Constrained and Unconstrained
  Testing Environment with safe threads", Computational Optimization and
  Applications 60(3), 545-557, 2015,
  https://doi.org/10.1007/s10589-014-9687-3
- PyCUTEst documentation: https://jfowkes.github.io/pycutest/_build/html/
- S2MPJ/CUTEst native problem files:
  https://optimization-online.org/2024/07/s2mpj-and-cutest-optimization-problems-for-matlab-python-and-julia/

### Phase 3: Standalone ABI

- Add an IPOPT-like C ABI over the A4SQP problem view.
- Provide standalone examples independent of ASCEND.
- Keep ASCEND-specific metadata as optional extensions.

### Phase 4: Filter SQP

- Add filter-based step acceptance as an alternative to the L1 merit line search.
- Compare filter behavior against elastic merit behavior on difficult ASCEND
  models.
- Retain elastic variables unless experience shows they are unnecessary.

## Open Questions

- Should the first implementation solve pure feasibility problems directly, or
  always formulate a residual/objective merit problem?
- How much of the existing ASCEND IPOPT wrapper should be factored into a common
  sparse NLP adapter?
- Should A4SQP initially use the existing ASCEND HiGHS wrapper or call HiGHS
  directly?
- What is the smallest useful subset of optional ASCEND metadata?
- Can HiGHS QP warm-start state be reused effectively between SQP iterations?
- How should conditional model changes invalidate warm starts?
- Should exact Hessian support be deferred until the BFGS-based method is
  working?
- What solver-state feedback should be stored in ASCEND flags versus kept in
  A4SQP-specific diagnostic structures?
- How should ASCEND identify decision variables, relevant relations, and
  presentation-only calculations for optimisation-oriented decomposition?
- Should pre-optimisation blocks be solved automatically, or should A4SQP first
  report a suggested decomposition for review?
- Should decomposition visualisation be built into A4SQP, or shared with the
  existing incidence matrix/graph tooling?

## Proposed Source Layout

Most A4SQP code should live under a new solver directory:

```text
solvers/a4sqp/
    SConscript
    a4sqp.c              ASCEND solver plugin entry point
    a4sqp.h              ASCEND-facing local declarations
    a4sqp_params.c       solver parameter definitions
    a4sqp_params.h
    a4sqp_status.c       ASCEND status and diagnostic reporting
    a4sqp_status.h
    a4sqp_ascend.c       slv_system_t -> A4SqpProblemView adapter
    a4sqp_ascend.h
    a4sqp_view.c         shared problem-view utilities
    a4sqp_view.h
    a4sqp_core.c         SQP iteration driver
    a4sqp_core.h
    a4sqp_qp_highs.c     HiGHS QP backend
    a4sqp_qp_highs.h
    a4sqp_bfgs.c         BFGS/L-BFGS approximation support
    a4sqp_bfgs.h
    a4sqp_merit.c        merit function and line search
    a4sqp_merit.h
    a4sqp_diag.c         diagnostic accumulation and formatting
    a4sqp_diag.h
    a4sqp_abi.c          future standalone C ABI
    a4sqp_abi.h
    examples/
        standalone_*.c

ascend/solver/test/
    test_a4sqp.c         ASCEND/CUnit solver tests

models/test/a4sqp/
    *.a4c                ASCEND test models for A4SQP
```

The initial build can compile only the files needed for the ASCEND plugin and
standalone unit tests. The standalone ABI files can start as a skeleton and be
enabled once the core problem view is stable.

Expected changes elsewhere:

```text
solvers/SConscript
    Add 'a4sqp' to the solver subdirectory list, gated by HiGHS availability.

SConstruct
    Add A4SQP to the WITH_SOLVERS ListVariable.
    Add WITH_A4SQP handling in the optional-solver loop, probably requiring
    WITH_HIGHS or the same HiGHS availability checks.

ascend/solver/test/
    Add ASCEND-level solver registration, problem-view, and smoke tests.

models/test/a4sqp/
    Add model-level solve cases, following the existing models/test/ipopt
    convention.

ascend/system/ or ascend/solver/
    Add shared helpers only if they are genuinely useful to more than A4SQP.
```

The default should be to keep new code in `solvers/a4sqp` until reuse pressure is
clear. Candidate helpers that might later move into `ascend/system` or
`ascend/solver` include:

- a common sparse NLP view builder shared by IPOPT and A4SQP;
- row/column naming and stable-identity helpers;
- generic solver diagnostic flags or report structures;
- incidence/decomposition visualisation helpers shared with existing graph
  tooling.

Avoid moving code into the core ASCEND libraries just because A4SQP needs it
once. That raises the maintenance cost and makes the first prototype harder to
change.

## Problem View Skeleton

A4SQP should use one internal problem representation for both ASCEND and
standalone test problems. The ASCEND adapter and future C ABI should populate
the same structure.

At a high level:

```c
typedef struct A4SqpProblemView A4SqpProblemView;
typedef struct A4SqpCallbacks A4SqpCallbacks;
typedef struct A4SqpMeta A4SqpMeta;
typedef struct A4SqpWork A4SqpWork;
typedef struct A4SqpResult A4SqpResult;
typedef struct A4SqpReporter A4SqpReporter;
```

The problem view should contain:

```c
struct A4SqpProblemView {
    int n_var;
    int n_rel;

    double *x_l;
    double *x_u;
    double *rel_l;
    double *rel_u;

    int jac_nnz;
    int *jac_irow;
    int *jac_jcol;

    int hess_nnz;          /* optional */
    int *hess_irow;        /* optional */
    int *hess_jcol;        /* optional */

    A4SqpCallbacks cb;
    A4SqpMeta *meta;       /* optional metadata */
    A4SqpReporter *reporter;
    void *user_data;
};
```

The callback shape should deliberately resemble IPOPT:

```c
typedef int (*A4SqpEvalF)(
    int n, const double *x, int new_x, double *f, void *user_data
);

typedef int (*A4SqpEvalGradF)(
    int n, const double *x, int new_x, double *grad_f, void *user_data
);

typedef int (*A4SqpEvalRel)(
    int n, const double *x, int new_x,
    int m, double *rel, void *user_data
);

typedef int (*A4SqpEvalJacRel)(
    int n, const double *x, int new_x,
    int m, int nnz, int *irow, int *jcol, double *values,
    void *user_data
);

typedef int (*A4SqpEvalH)(
    int n, const double *x, int new_x,
    double obj_factor,
    int m, const double *lambda, int new_lambda,
    int nnz, int *irow, int *jcol, double *values,
    void *user_data
);
```

As with IPOPT, `values == NULL` means "return sparsity structure" for Jacobian
and Hessian callbacks.

The problem view should also support a caller-provided reporting/logging
callback. This is needed both for standalone use and for ASCEND integration.
A4SQP should not print directly except as a last-resort fallback.

```c
typedef enum A4SqpReportLevel {
    A4SQP_REPORT_ERROR = 0,
    A4SQP_REPORT_WARNING,
    A4SQP_REPORT_SUMMARY,
    A4SQP_REPORT_ITERATION,
    A4SQP_REPORT_QP,
    A4SQP_REPORT_TRACE
} A4SqpReportLevel;

typedef int (*A4SqpReportFn)(
    A4SqpReportLevel level,
    const char *component,
    const char *message,
    void *report_data
);

struct A4SqpReporter {
    A4SqpReportFn report;
    void *report_data;
    int verbosity;
};
```

Suggested component strings include `core`, `line-search`, `merit`, `bfgs`,
`qp-highs`, `scaling`, `warm-start`, and `ascend-adapter`.

In the ASCEND integration, reporting should use the existing reporting channels:

- `ERROR_REPORTER_*` for user-facing diagnostics, warnings, and final failure
  explanations;
- `CONSOLE_DEBUG`/developer trace macros for detailed developer-only output;
- `slv_report_progress` for in-run progress messages that GUIs or scripts can
  poll or display;
- `slv_get_solver_interrupt`/`slv_set_solver_interrupt` for user interruption.

The existing HiGHS solver wrapper already uses `Highs_setCallback`,
`Highs_startCallback`, `slv_report_progress`, and the global solver interrupt
flag. A4SQP should reuse that pattern. The A4SQP core should report SQP major
iterations, line-search decisions, elastic infeasibility, and convergence
metrics. The HiGHS QP backend should forward QP-level callback information as
subproblem progress, throttled so that QP callback chatter does not obscure SQP
major-iteration progress.

The reporting callback should be used for:

- SQP iteration summaries;
- QP solve status and QP iteration information where HiGHS exposes it;
- accepted and rejected line-search steps;
- elastic penalty updates;
- scaling diagnostics;
- derivative evaluation failures;
- warm-start acceptance or invalidation;
- final failure explanation.

The ASCEND plugin can route this callback to ASCEND's existing solver output and
progress mechanisms. The standalone ABI can let callers install their own logger
or use a default stderr logger.

The optional metadata should keep A4SQP useful inside ASCEND without making the
core solver depend on ASCEND types:

```c
struct A4SqpMeta {
    const char **var_names;
    const char **rel_names;

    const void **var_ids;
    const void **rel_ids;

    double *var_nominal;
    double *rel_nominal;
    double *var_scale;
    double *rel_scale;

    unsigned *var_flags;
    unsigned *rel_flags;

    int *block_id_var;
    int *block_id_rel;
};
```

For ASCEND, `var_ids` and `rel_ids` can point to ASCEND variable/relation
objects or stable wrappers, but the core solver should only compare them for
identity. It should not dereference ASCEND types.

Suggested early flags:

```text
A4SQP_VAR_FIXED
A4SQP_VAR_ACTIVE
A4SQP_VAR_INCIDENT
A4SQP_VAR_ON_LOWER
A4SQP_VAR_ON_UPPER
A4SQP_VAR_STEP_CLIPPED

A4SQP_REL_INCLUDED
A4SQP_REL_ACTIVE
A4SQP_REL_EQUALITY
A4SQP_REL_ON_LOWER
A4SQP_REL_ON_UPPER
A4SQP_REL_ELASTIC_LOWER
A4SQP_REL_ELASTIC_UPPER
A4SQP_REL_INFEASIBLE
```

The prototype can keep these as A4SQP-side metadata and result flags, but that
should not be interpreted as the final design. A key value of ASCEND's existing
solver flags is that the GUI can expose them directly during model debugging.
For example, seeing active variables or unsatisfied relations is often what
localises where QRSlv became stuck.

The expected direction is:

- read existing ASCEND flags such as `VAR_FIXED`, `VAR_ACTIVE`,
  `VAR_INCIDENT`, `REL_INCLUDED`, `REL_ACTIVE`, `REL_EQUALITY`, and
  `REL_SATISFIED`;
- keep early A4SQP-specific states in sidecar arrays while the algorithm is
  still changing;
- promote stable, user-meaningful A4SQP states into solver-visible ASCEND
  storage once their semantics are clear;
- make promoted states available to GUI and diagnostic tools in the same way
  that existing solver state is available.

Candidate GUI-facing states include:

- variable active at lower bound;
- variable active at upper bound;
- variable step clipped by bound or trust region;
- variable with large bound multiplier;
- relation active at lower bound;
- relation active at upper bound;
- relation satisfied at tolerance;
- relation elastic on lower side;
- relation elastic on upper side;
- relation responsible for largest infeasibility;
- relation with large multiplier.

There are two possible storage approaches:

- add selected new bits to `var_variable` and `rel_relation` if free flag bits
  and stable semantics are available;
- add a solver-status side table owned by `slv_system_t`, keyed by solver
  variable/relation index, if more bits or richer numeric diagnostics are
  needed.

The side-table approach may be preferable for A4SQP because some diagnostics are
numeric rather than boolean: elastic magnitude, multiplier value, last accepted
step, last clipped step, and infeasibility contribution. It also avoids
overloading low-level data structures with solver-specific transient state.

The design goal is therefore not "avoid ASCEND flags"; it is "prototype without
premature low-level changes, then promote the states that users need for
debugging." Efficient storage in or near `slv_system_t` is desirable, especially
for GUI inspection after an unsuccessful solve.

## Standalone ABI Skeleton

The standalone ABI should initially be a thin creator/solver/destroyer around
`A4SqpProblemView`.

```c
typedef struct A4SqpProblemStruct *A4SqpProblem;

A4SqpProblem A4SqpCreateProblem(
    int n, const double *x_l, const double *x_u,
    int m, const double *rel_l, const double *rel_u,
    int jac_nnz, int hess_nnz,
    int index_style,
    A4SqpEvalF eval_f,
    A4SqpEvalRel eval_rel,
    A4SqpEvalGradF eval_grad_f,
    A4SqpEvalJacRel eval_jac_rel,
    A4SqpEvalH eval_h
);

int A4SqpSolve(
    A4SqpProblem prob,
    double *x,
    double *rel,
    double *obj_value,
    double *mult_rel,
    double *mult_x_l,
    double *mult_x_u,
    void *user_data
);

void A4SqpFreeProblem(A4SqpProblem prob);
```

This is intentionally close to IPOPT's C interface, but not symbol-compatible.
Avoid names such as `CreateIpoptProblem`.

In `A4SqpSolve`, the `mult_*` arrays are Lagrange multiplier outputs:

- `mult_rel`: multipliers for relation lower/upper bounds;
- `mult_x_l`: multipliers for variable lower bounds;
- `mult_x_u`: multipliers for variable upper bounds.

`mult_rel` is one signed multiplier per relation row, following the usual NLP
convention for a row with lower and/or upper bounds. If A4SQP later needs
separate lower and upper row multipliers internally, it can keep those in
`A4SqpResult` while preserving this compact ABI output.

These multipliers are useful for KKT diagnostics, active-set warm starts, and
BFGS updates. The naming follows IPOPT's C interface conceptually, but uses
`rel` rather than `g` to stay closer to ASCEND terminology.

Later additions:

- `A4SqpSetOption*` and `A4SqpGetOption*`;
- `A4SqpSetMeta`;
- `A4SqpSetReporter`;
- `A4SqpGetResult`;
- `A4SqpGetDiagnostics`;
- `A4SqpSetWarmStart` and `A4SqpGetWarmStart`;
- callback hooks for progress and interrupt handling.

## Implementation Staging

Recommended staging:

1. Define `A4SqpProblemView`, callbacks, metadata, result, and diagnostic
   structures.
2. Add standalone tests that validate bounds, sparsity, callback calls,
   Jacobian values, and metadata identity.
3. Spike a HiGHS QP solve from a hand-built QP with step variables and elastic
   slacks.
4. Implement a minimal standalone elastic SQP loop on small C test problems.
5. Add a read-only ASCEND adapter that builds and dumps `A4SqpProblemView`
   without solving.
6. Compare the ASCEND adapter's dimensions, bounds, residuals, Jacobian
   sparsity, and Jacobian values against the existing IPOPT path.
7. Enable ASCEND solve calls through the same core.
8. Add ASCEND diagnostics and visualisation.

The staging principle is: standalone core first, ASCEND adapter early, one
shared problem representation throughout.

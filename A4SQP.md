# A4SQP Concept

## Purpose

A4SQP is an SQP-based nonlinear solver prototype for ASCEND. Its intended niche
is hard nonlinear algebraic and nonlinear programming problems where the
existing local equation-solving path can stall because of active bounds, poor
initialization, inconsistent local linearizations, or tight engineering
specifications.

The implemented prototype is an elastic line-search SQP method using HiGHS as
the quadratic programming (QP) solver. It now includes a constrained
trust-region retry path and experimental exact-Hessian modes. A later phase may
add filter SQP globalization.

This note also records an architectural tension: A4SQP should eventually be
usable outside ASCEND through an IPOPT-like C problem ABI, but an ASCEND-native
solver client can both consume and return richer information through
`slv_system_t`.

## Presolve/Postsolve Pin

There is a separate optimization-layer idea that should remain on the design
list: A4SQP could eventually run an optimization presolve before the core solve
and a postsolve after it. That layer might remove fixed variables, tighten or
substitute simple bounds, detect redundant rows, rescale or reorder the active
NLP, and then map the reduced solution back to the original ASCEND problem.

That is distinct from the current CUTEst/core-API cleanup. The current refactor
should only establish a clean core-owned NLP vector and an ASCEND adapter that
pushes that vector into `solver_var` objects for evaluation, similar to IPOPT
and IDA. Presolve/postsolve may later sit above that adapter/core boundary, but
it should not be mixed into the CUTEst benchmark definition or used to explain
current CUTEst performance.

## Implemented Solver Shape

The current implementation remains deliberately conservative, but the initial
solver shape is now in place:

- A4SQP is a normal ASCEND solver client over `slv_system_t`.
- It builds the SQP problem from ASCEND solver variable and relation lists.
- It uses exact first derivatives from ASCEND relation derivative machinery.
- It calls HiGHS directly for convex QP subproblems.
- It supports a default damped BFGS step Hessian, explicit `EXACT_OBJ` and
  `EXACT_LAGRANGIAN` modes, and conservative `AUTO` promotion to sparse exact
  constrained curvature on small settled problems.
- It adds lower and upper elastic variables to QP subproblems so local
  linearized infeasibility does not immediately abort an iteration.
- It uses an L1 merit-function line search, with a scaled box trust region for
  constrained problems and bounded trust-region/QP retries.
- It reports progress, QP status, line-search metrics, elastic activity, trust
  radius, and view diagnostics through ASCEND progress callbacks and optional
  error-reporter notes.

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
- `rosenmmx.a4c`, Rosen's four-variable convex minmax benchmark in smooth
  epigraph form, now solving successfully as a compact constrained regression;
- `jannson3.a4c`, a reduced `n = 3` translation of the Jannson convex-concave
  extension benchmark, now solving successfully with the current constrained
  A4SQP path;
- `cont6_qq.a4c`, a reduced semilinear control benchmark derived from the
  CUTEr/SIF `CONT6-QQ` problem, now solving successfully with the current
  constrained A4SQP path;
- `rosenbr.a4c`, the classical two-variable Rosenbrock objective-only problem,
  now solving successfully as an explicit `EXACT_OBJ` regression.

More recent benchmark work has clarified the current boundary of the prototype:

- `hs3.a4c`, `hs11.a4c`, `hs21.a4c`, `rosenmmx.a4c`, `jannson3.a4c`, the
  reduced `lubrifc.a4c`, and the reduced `cont6_qq.a4c` are current passing
  A4SQP regressions in the focused CUnit suite;
- `rosenbr.a4c` is also now in the passing suite, but specifically under the
  explicit `EXACT_OBJ` Hessian mode rather than the default `BFGS` path;
- `lubrifc.a4c` is now present as a reduced benchmark translation using
  `NN = 10`, not the full CUTEr default, and the current reduced model uses the
  smoother complementarity relaxation `min sum(p_i r_i)` together with
  `p_i >= 0`, `r_i >= 0`;
- the current reduced `lubrifc.a4c` translation now converges with the current
  trust-region prototype and has become a useful constrained robustness case
  rather than a pure failure reproducer.

Recent implementation lessons from these benchmarks:

- Objective-only models are valid A4SQP cases and must not be treated as QP
  assembly failures when the relation count is zero.
- Convergence for optimization problems must check stationarity, not just
  feasibility.
- Merit-function acceptance should be driven by predicted decrease, without
  imposing an artificial absolute decrease floor on otherwise valid steps.
- Triangular Hessian storage passed to HiGHS must be reconstructed carefully
  when computing model-predicted objective values.
- A4SQP's objective derivative path must use ASCEND's reverse derivative call
  in the same way as the constraint Jacobian path; otherwise some nonlinear
  objective-only or mixed NLP cases produce invalid QP data.
- Leaving `safeeval` enabled by default in A4SQP was stricter than IPOPT's
  corresponding path and caused spurious presolve derivative failures on
  `jannson3.a4c`; the default is now aligned with IPOPT's non-safe evaluation
  path.

## CUTEst Survey Goal

The next benchmarking step should be a broader CUTEst survey rather than more
single-problem anecdotes.

The goal is not simply to count solves. The goal is to identify the problem
classes where A4SQP is a useful complement to IPOPT and where it still trails
dense SLSQP-style methods.

This matters because ASCEND already has evidence from the fprops equilibrium
path that IPOPT can be a poor fit for some bound-heavy Gibbs-minimization
problems, while the local NLOpt SLSQP path can be dramatically faster on small
dense instances. A4SQP should therefore be judged less as "an open-source
replacement for IPOPT" and more as "an ASCEND-native sparse SQP that should
cover a different and practically useful part of the NLP landscape."

The survey should therefore test the following hypotheses explicitly:

- IPOPT should remain stronger on large sparse smooth problems that are
  interior-point-friendly and not dominated by active bounds.
- Dense SLSQP should remain stronger on small dense callback problems with
  modest constraint counts and cheap per-iteration models.
- A4SQP should aim to be strongest on smooth constrained problems with
  meaningful active bounds or inequalities, awkward local infeasibility,
  moderate sparsity, and sizes beyond what dense SLSQP handles comfortably.

Recommended CUTEst problem buckets:

- objective-only curved problems
  - to test exact-objective curvature and termination quality;
- compact constrained active-set problems
  - to test multiplier stability and active-bound behavior;
- badly scaled constrained problems
  - to test scaling, elastic rows, and QP robustness;
- medium sparse structured problems
  - to test whether the sparse SQP architecture is earning its keep;
- degenerate or nearly infeasible problems
  - to test whether restoration/filter work is becoming urgent;
- PDE/control-style or discretized structured problems
  - to test the practical overlap with ASCEND engineering models.

Each benchmark record should include at least:

- problem name and CUTEst classification;
- `n`, `m`, equality/inequality split, and bound fraction;
- whether the problem is objective-only, equality-dominant, or bound-heavy;
- rough scaling notes where obvious;
- A4SQP status, time, major iterations, line-search failures, QP retries, and
  any elastic activity at termination;
- IPOPT status and time on the same decoded problem;
- SLSQP status and time where a direct comparison is feasible;
- a short note describing the likely reason for failure or slowness.

For A4SQP design decisions, the important output of the survey is a clustering
of failure modes:

- problems where BFGS is enough;
- problems where exact constrained curvature helps materially;
- problems where filter/restoration is the missing piece;
- problems where dense or small-problem overhead dominates and SLSQP is still
  the better tool;
- problems where the current HiGHS-backed convex-QP architecture is simply not
  competitive.

Recent second-order work:

- A4SQP now has an experimental exact-Hessian assembly path that evaluates
  per-relation second derivatives in local incidence order, maps them into the
  current solver-variable order, and scales them into the same coordinates as
  the QP step variables;
- that path currently supports two explicit solver modes: `EXACT_OBJ` for
  objective-only exact curvature, and `EXACT_LAGRANGIAN` for an experimental
  constrained Lagrangian Hessian assembled from filtered QP multiplier
  estimates rather than the raw current row duals;
- when the ASCEND second-derivative callbacks provide no usable objective
  curvature for an objective-only model, A4SQP now falls back to a
  finite-difference Hessian of the exact objective gradient rather than
  silently collapsing to a near-zero quadratic model;
- for constrained exact-Hessian modes, A4SQP now assembles the lower-triangular
  Lagrangian Hessian directly into a sparse solver-variable structure and feeds
  that sparse Hessian straight into the HiGHS QP builder rather than scanning a
  dense `n x n` buffer;
- constrained exact-Hessian assembly now keeps a smoothed multiplier estimate
  across iterations, suppresses rows whose elastic activity or dual magnitude
  says the current QP dual is not yet a clean NLP multiplier signal, and uses
  that filtered multiplier state for the exact Lagrangian Hessian build;
- the stored multiplier estimate is now kept in de-scaled NLP units rather than
  raw scaled-QP-row units, so the constrained Hessian build and the
  cross-iteration smoothing logic are operating on a consistent multiplier
  meaning;
- the multiplier smoothing policy is now row-class aware: equality rows,
  active inequalities, near-active inequalities, and clearly inactive
  inequalities are damped differently, with inactive rows driven explicitly
  toward zero instead of being treated as if they carried the same multiplier
  signal quality as active constraints;
- the sparse constrained exact-Hessian path currently uses a conservative
  diagonal-shift PSD regularization on that sparse lower-triangular structure,
  while the objective-only exact mode keeps the denser regularization path that
  is already benchmark-qualified on `hs3.a4c` and `rosenbr.a4c`;
- because HiGHS requires a convex QP, the exact Hessian path is regularized to
  a positive-semidefinite step model before the QP is assembled;
- `AUTO` can now promote small constrained problems onto the sparse
  `EXACT_LAGRANGIAN` path once the multiplier estimate is populated and the
  elastic activity has settled to a negligible level, while still falling back
  to `BFGS` on noisier or larger constrained problems;
- that `AUTO` promotion is still intentionally conservative: it is currently
  limited to compact constrained problems with at most roughly `64` variables
  and `64` relations, all current row multipliers classified as usable, and no
  immediate line-search failure signal from the preceding SQP step;
- the default solver mode remains `BFGS` for now, because the exact objective
  path is still experimental and the constrained exact-Lagrangian path is still
  exploratory rather than benchmark-qualified;
- the focused CUnit suite now includes explicit `EXACT_OBJ` regressions for
  `hs3.a4c` and `rosenbr.a4c`, so the objective-only exact-Hessian path is
  covered in CI without destabilizing the default solver behavior;
- the focused CUnit suite also includes a constrained `EXACT_LAGRANGIAN`
  regression on `hs11.a4c`; that path converges with acceptable objective and
  feasibility, but it is not yet treated as interchangeable with the tighter
  benchmark-qualified default/BFGS path.
- the focused CUnit suite also now checks that `AUTO` promotes `hs11.a4c` onto
  the sparse constrained exact-Hessian path once those multiplier and elastic
  gating conditions are satisfied;
- the focused CUnit suite now also qualifies that same `AUTO` promotion on
  `hs21.a4c` and the reduced `jannson3.a4c`, which moves the constrained
  `AUTO` path beyond the single-constraint `hs11` case;
- attempts to extend that green `AUTO` boundary immediately to
  `rosenmmx.a4c` and the reduced PDE-control case `cont6_qq.a4c` were not
  adopted into the passing suite yet: the former remained too slow to justify
  focused-regression status, and the latter crossed the current cost boundary
  for the exact constrained Hessian path.

Recent constrained-solver stabilization work:

- QP row-displacement bounds that are already within A4SQP's scaled feasibility
  tolerance are snapped to zero before the HiGHS call, which avoids late
  numerically pointless correction QPs;
- the merit line search no longer suppresses small but still useful accepted
  steps before the convergence check has a chance to see the updated iterate.

Recent HiGHS-facing stabilization work:

- A4SQP now logs enough of the HiGHS QP lifecycle under `progress_log` to make
  late subproblem failures diagnosable without recompiling additional tracing;
- the HiGHS primal, dual, and KKT tolerances are now aligned more closely with
  A4SQP's feasibility scale instead of relying on the much tighter HiGHS
  defaults;
- tiny step-bound displacements are now snapped to zero before the HiGHS call in
  the same way as tiny row displacements, which reduces purely numerical bound
  noise in near-active variables.

Recent trust-region work:

- A4SQP now has a scaled box trust region on the primal step variables for
  constrained problems, implemented directly as additional QP column bounds;
- the trust box is not applied to objective-only problems such as `hs3.a4c`,
  because doing so distorted the previously working objective-only path without
  helping the current failure cases;
- the effective per-variable trust radius is expanded as needed to include any
  hard bound-restoring step, so the trust box cannot make an already violated
  variable bound infeasible inside the QP;
- on QP failure or line-search rejection, A4SQP now shrinks the trust radius
  and rebuilds the QP a bounded number of times before giving up;
- when the current constrained iterate is already feasible and the QP reduces to
  a numerically null correction, A4SQP now accepts a null step instead of
  burning time on a sequence of smaller trust radii.

Observed benchmark outcome from that work:

- `hs11.a4c` and `hs21.a4c` still converge under the trust-region prototype;
- `jannson3.a4c` now also converges as a reduced constrained regression under
  the same trust-region prototype;
- `cont6_qq.a4c` now also converges as a reduced PDE-constrained control
  regression under the same trust-region prototype;
- the focused CUnit suite remains green for `hs3`, `hs11`, `hs21`,
  `jannson3`, the reduced `lubrifc`, and the reduced `cont6_qq`;
- the reduced `lubrifc.a4c` case now progresses past the earlier late HiGHS QP
  failure and converges to a feasible relaxed solution;
- the trust-region prototype is therefore paying off on constrained cases, but
  it is still intentionally scoped conservatively and is not yet a full filter
  SQP globalization scheme.

Still deferred:

- a reduced-memory Hessian option such as L-BFGS;
- filter globalization;
- richer warm-start state beyond the current prototype state.

Still immature:

- exact objective Hessians are now implemented only as an explicit experimental
  mode, but they are now benchmark-qualified on `hs3.a4c` and `rosenbr.a4c`
  rather than only on trivial objective-only cases;
- exact constrained Lagrangian Hessians are implemented only as an
  experimental mode using QP row duals as multiplier estimates, and are not yet
  suitable as the default constrained step model.

## SQP Subproblem

At iterate `x_k`, A4SQP forms a QP step `p` from a local model:

```text
minimize    grad f(x_k)^T p + 1/2 p^T B_k p + rho * sum(elastic_slacks)

subject to  r_L - e_L <= r(x_k) + J_r(x_k) p <= r_U + e_U
            x_L <= x_k + p <= x_U
            ||p||_inf <= trust_radius       (constrained problems)
            e_L, e_U >= 0
```

For a pure feasibility solve, `f` may be zero or a residual merit objective. For
an optimization solve, `f` is the ASCEND objective relation or objective
variable.

`B_k` must be positive semidefinite for HiGHS QP. The default model is a damped
BFGS approximation on the scaled primal step block. Experimental exact Hessian
modes are regularized before being passed to HiGHS: objective-only exact
curvature can be used with `EXACT_OBJ`, and constrained Lagrangian curvature can
be used with `EXACT_LAGRANGIAN` or selected by `AUTO` when the multiplier and
elastic-activity gates are satisfied.

## Why HiGHS

HiGHS is attractive because ASCEND already has a HiGHS integration for LP/MIP
models and HiGHS provides the QP machinery needed by SQP. A4SQP should not try
to implement an active-set QP solver itself.

The HiGHS-facing layer is now implemented directly inside `solvers/a4sqp`:

- QP Hessian, gradient, bounds, sparse linear relation rows, and elastic columns
  are assembled by `a4sqp_qp_build_from_view`.
- Step columns map back to ASCEND solver variables; elastic column pairs map
  back to relation rows.
- QP primal and dual arrays, objective value, HiGHS status, and model status
  are stored in `A4SqpQp`.
- HiGHS callbacks are installed for progress and interrupt handling, with
  optional QP lifecycle logging under `progress_log`.
- HiGHS feasibility and KKT tolerances are aligned with A4SQP's feasibility
  scale instead of relying on the tighter default QP tolerances.
- HiGHS QP warm-start or basis reuse remains open; the current implementation
  rebuilds each QP subproblem.

## ASCEND Integration

A4SQP is currently an ASCEND solver client, not only a generic external
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

A useful decomposition is:

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

### Current HiGHS QP Layer

The A4SQP QP assembly layer builds a convex QP in HiGHS terms from the current
SQP iterate:

- step variables `p`, not just absolute model variables `x`;
- lower and upper elastic variables for each relation row;
- linearized relation matrix `J_r(x_k)`;
- linearized row bounds `r_L - r(x_k)` and `r_U - r(x_k)`;
- step bounds `x_L - x_k <= p <= x_U - x_k`;
- QP linear objective `grad f(x_k)^T p`;
- QP Hessian `B_k`, from BFGS, exact objective curvature, or sparse exact
  Lagrangian curvature depending on the `hessian` mode;
- elastic penalties in the linear objective;
- scaled box trust-region bounds for constrained problems;
- mapping from QP columns back to ASCEND variables and elastic rows;
- mapping from QP rows back to ASCEND relations;
- extraction of QP primal step, row duals, column duals, and elastic values.

The QP layer also handles solver mechanics:

- create and destroy a HiGHS model for each SQP iteration;
- pass lower-triangular Hessian data to HiGHS in sparse column form;
- regularize the Hessian to a convex QP model before assembly;
- configure HiGHS for continuous convex QP, not LP/MIP;
- capture QP statuses separately from outer SQP statuses;
- report enough failing QP data for small subproblems to diagnose bad bounds,
  bad rows, or poor Hessian/linearization state.

The main remaining HiGHS backend questions are performance and warm start:
whether model updates or QP basis/active-set reuse can pay off after the SQP
behaviour is more stable.

### Current SQP Layer

The current SQP algorithm around the QP solver includes:

- SQP iteration driver;
- convergence tests for scaled feasibility, projected gradient/stationarity
  proxy for optimization problems, step size, and iteration limits;
- fixed global elastic penalty parameter;
- L1 merit-function line search;
- step acceptance and rollback into ASCEND variables;
- dense damped BFGS update for the default step Hessian;
- explicit exact objective and constrained exact-Lagrangian Hessian modes;
- Hessian regularization;
- trust-region shrink/grow logic for constrained QP retries;
- diagnostic accumulation for rows, variables, active bounds, elastic rows,
  trust ratio, line search, and failed QP subproblems.

The current implementation still lives mostly in `a4sqp.c`, with support code in
the view, scaling, diagnostics, and QP backend modules. A future cleanup should
split the SQP core, merit/line-search code, BFGS code, and exact Hessian support
into smaller files once the algorithmic boundary is stable. Restoration,
adaptive elastic penalties, richer warm starts, and a standalone ABI remain
future work.

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

The current implementation follows QRSlv rather than inventing a new scaling
vocabulary. The solver parameter `scaleopt` currently supports `NONE`,
`ROW_2NORM`, and `RELNOM`, with `ROW_2NORM` as the default. Variable scales are
derived from safe nominal values. Relation scales are derived from the selected
mode, with safe fallbacks for zero, negative, or non-finite scales.

The implemented convention is:

```text
x = S_x z
r_scaled(z) = S_r^{-1} r(S_x z)
J_scaled = S_r^{-1} J S_x
```

The QP works with scaled step variables `p_z`. The ASCEND adapter packs and
unpacks physical `x` values, while the A4SQP QP layer sees scaled variables,
scaled bounds, scaled residuals, scaled Jacobian entries, and scaled objective
gradients. QP row duals used as constrained-Hessian multiplier estimates are
converted back to de-scaled NLP units before cross-iteration smoothing.

Still-open scaling work:

- iterative/Fourer-style scaling is not implemented;
- diagnostics should expose scaled and unscaled infeasibility side by side;
- badly scaled rows/columns should be reported explicitly;
- regression tests should cover unit and nominal changes more systematically.

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

Phase 1 is now substantially complete for the ASCEND-native path:

- A4SQP is an ASCEND solver client.
- It builds an `A4SqpView` from `slv_system_t`.
- It rejects unsupported relations and derivative/evaluation failures with
  user-facing diagnostics.
- It generates elastic HiGHS QP subproblems.
- It implements an L1 merit line search, default BFGS step Hessian, exact
  Hessian experimental modes, and a constrained trust-region retry loop.
- It has native CUnit coverage through `solver_a4sqp` and test models under
  `models/test/a4sqp`.
- It reports progress, view diagnostics, QP status, line-search status, and
  trust-region metrics.

The remaining Phase 1 limitations are mostly scope decisions: smooth continuous
NLPs only, no standalone ABI, no filter globalization, no L-BFGS, no automatic
decomposition, and no mature warm-start invalidation.

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

The checklist below is retained as an implementation record. It describes the
path taken from skeleton to current prototype rather than fresh work still to be
done. The early steps were useful because they proved that A4SQP sees the same
problem ASCEND sees before SQP algorithm behaviour is introduced.

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

   The adapter maps current ASCEND binary relation operators into the internal
   row-bound representation:

   ```text
   LHS =  RHS      r = LHS - RHS,    rel_l = 0,    rel_u = 0
   LHS <= RHS      r = LHS - RHS,    rel_l = -inf, rel_u = 0
   LHS >= RHS      r = LHS - RHS,    rel_l = 0,    rel_u = +inf
   LHS <  RHS      r = LHS - RHS,    rel_l = -inf, rel_u = 0
   LHS >  RHS      r = LHS - RHS,    rel_l = 0,    rel_u = +inf
   ```

   Strict inequalities are accepted as their tolerant non-strict equivalents.
   Unsupported relation operators, including `<>`, are rejected during view
   construction.

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

    After the view and QP spike passed tests, the first major SQP iteration was
    added around the same QP assembly path:

    - assemble QP from current residuals and Jacobian;
    - start with a simple positive-semidefinite Hessian model;
    - solve the QP with HiGHS;
    - compute candidate step and predicted reduction;
    - update diagnostics before adding the richer line-search and trust-region
      logic in the next milestone.

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
    - Predicted reduction, active-set/multiplier diagnostics, and richer
      failure reporting are now partly implemented. Adaptive elastic penalty
      updates remain future work.
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
    - Step acceptance now uses an Armijo-style test and a trust acceptance
      ratio:
      actual merit decrease must be at least
      `armijo_coeff * alpha * predicted_reduction`, and the actual-to-predicted
      ratio must satisfy `trust_accept` when meaningful. Numerically null
      corrections at an already feasible constrained iterate can be accepted as
      null steps.
    - `basic_view.a4c` now exercises a nonzero objective-gradient QP step,
      backtracking acceptance, merit decrease, and updated objective value.
    - Solver parameters now include `max_iter`, `max_backtrack`, `feas_tol`,
      `step_tol`, `merit_tol`, `armijo_coeff`, and `elastic_penalty`, in
      addition to the earlier safe-evaluation, scaling, verbosity, progress,
      and view-dump controls.
    - `slv_solve` now loops over major SQP iterations until convergence,
      iteration limit, QP failure, or line-search failure. Convergence uses
      maximum scaled relation violation, a stationarity/projected-gradient
      check for optimization problems, and accepted physical step norm.
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

Phase 1 should now be treated as complete for the current ASCEND-native
prototype: A4SQP can build and inspect the problem view for native ASCEND NLP
examples, solve a focused suite of small smooth continuous examples, and provide
basic diagnostics on setup, QP, and line-search failures. Performance tuning,
warm starts, decomposition, filter SQP, and standalone ABI work remain Phase 2+
scope.

### Phase 2: Robustness and Diagnostics

Phase 2 should turn the Phase 1 prototype from "can solve selected examples"
into "can explain and improve behaviour on difficult ASCEND models." This phase
is less about adding new mathematical features and more about making the solver
diagnosable, repeatable, and robust under realistic modelling conditions.

#### Scaling

The basic scaling convention is now implemented. Phase 2 should make it more
visible, more testable, and more robust:

- extend the current `NONE`, `ROW_2NORM`, and `RELNOM` modes only where a real
  benchmark need appears;
- decide whether scaling should be recomputed every trial point or stabilized
  across iterations for better step consistency;
- keep scaling consistent across relation residuals, Jacobians, bounds, QP
  Hessians, gradients, elastic penalties, and multiplier estimates;
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
ASCEND-native adapter maps ordinary ASCEND relations into row bounds as follows:

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

- How much of the existing ASCEND IPOPT wrapper should be factored into a common
  sparse NLP adapter, now that A4SQP has its own ASCEND-native `A4SqpView`?
- What is the smallest useful subset of optional ASCEND metadata beyond row and
  column mappings, names, scales, residuals, and bounds?
- Can HiGHS QP warm-start state be reused effectively between SQP iterations?
- How should `slv_resolve` and conditional model changes invalidate BFGS,
  multiplier, trust-region, and future QP warm-start state?
- What solver-state feedback should be stored in ASCEND flags versus kept in
  A4SQP-specific diagnostic structures?
- Should elastic penalties remain global, or should A4SQP add adaptive
  per-relation penalty updates after the diagnostic semantics settle?
- Which CUTEst/S2MPJ problem classes should define the next benchmark boundary
  for default BFGS, explicit exact Hessian modes, and `AUTO` promotion?
- How should ASCEND identify decision variables, relevant relations, and
  presentation-only calculations for optimisation-oriented decomposition?
- Should pre-optimisation blocks be solved automatically, or should A4SQP first
  report a suggested decomposition for review?
- Should decomposition visualisation be built into A4SQP, or shared with the
  existing incidence matrix/graph tooling?

## Current Source Layout

The current A4SQP code lives under `solvers/a4sqp`:

```text
solvers/a4sqp/
    SConscript
    a4sqp.c              ASCEND solver plugin entry point
    a4sqp.h              ASCEND-facing local declarations
    a4sqp_internal.h     solver client state and SQP diagnostics
    a4sqp_params.c       solver parameter definitions
    a4sqp_params.h
    a4sqp_ascend.c       slv_system_t -> A4SqpProblemView adapter
    a4sqp_ascend.h
    a4sqp_view.c         shared problem-view utilities
    a4sqp_view.h
    a4sqp_scale.c        QRSlv-style scaling support
    a4sqp_scale.h
    a4sqp_qp_highs.c     HiGHS QP backend
    a4sqp_qp_highs.h
    a4sqp_diag.c         diagnostic accumulation and formatting
    a4sqp_diag.h

ascend/solver/test/
    test_a4sqp.c         ASCEND/CUnit solver tests

models/test/a4sqp/
    *.a4c                ASCEND test models for A4SQP
```

The first implementation kept the SQP driver, merit/line-search logic, BFGS
logic, exact Hessian assembly, multiplier smoothing, and trust-region logic in
`a4sqp.c`. This is acceptable for the prototype, but the next cleanup should
split those responsibilities once the algorithmic boundaries settle.

Likely future splits:

```text
    a4sqp_core.c         SQP iteration driver
    a4sqp_core.h
    a4sqp_bfgs.c         BFGS/L-BFGS approximation support
    a4sqp_bfgs.h
    a4sqp_hess.c         exact Hessian assembly and regularization
    a4sqp_hess.h
    a4sqp_merit.c        merit function and line search
    a4sqp_merit.h
    a4sqp_abi.c          future standalone C ABI
    a4sqp_abi.h
```

Implemented integration changes elsewhere:

```text
solvers/SConscript
    Includes 'a4sqp' in the solver subdirectory list.

SConstruct
    Includes A4SQP in WITH_SOLVERS and optional-solver handling.

ascend/solver/test/
    Contains ASCEND-level solver registration, problem-view, solve, Hessian,
    trust-region, and benchmark regression tests.

models/test/a4sqp/
    Contains model-level solve cases and reduced benchmark translations.

ascend/system/ or ascend/solver/
    No shared core helpers have been added yet.
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

A4SQP currently uses an ASCEND-native `struct A4SqpView` populated from
`slv_system_t`. It stores variable and relation pointers, solver indices,
physical and scaled values/bounds, objective data, Jacobian sparsity and values,
scales, and evaluation-error counters.

The future standalone ABI should still use one internal problem representation
for both ASCEND and standalone callers, but the current `A4SqpView` is not yet a
solver-neutral callback view. The skeleton below is therefore a future ABI/core
shape, not the structure currently compiled in `a4sqp_view.h`.

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

When the standalone ABI is added, it should start as a thin
creator/solver/destroyer around a solver-neutral successor to `A4SqpView`.

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

The original staging plan has been overtaken by the ASCEND-native prototype.
The actual implemented order was:

1. Add the A4SQP build skeleton and ASCEND solver registration.
2. Build an ASCEND-native problem view directly from `slv_system_t`.
3. Add native CUnit tests for registration, view construction, row-bound
   mapping, Jacobian values, scaling, and progress callbacks.
4. Spike a HiGHS QP solve from a hand-built QP with step variables and elastic
   slacks.
5. Connect the ASCEND problem view to an elastic HiGHS QP assembly layer.
6. Add `slv_iterate` and `slv_solve` over the same SQP major-iteration path.
7. Add L1 merit line search, BFGS step Hessian, convergence checks, progress
   reporting, and failure diagnostics.
8. Add exact objective and exact constrained Hessian experiments, sparse
   Hessian passing, multiplier smoothing, `AUTO` promotion gates, and
   constrained trust-region retries.
9. Grow the native test suite around reduced benchmark `.a4c` cases.

The revised staging principle is: keep the ASCEND-native path green while
gradually separating solver-neutral pieces. A standalone ABI should wait until
the problem-view boundary, result/diagnostic structures, and warm-start state are
less volatile.

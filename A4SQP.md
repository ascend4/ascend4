# A4SQP Architecture Notes

## Overview

A4SQP is intended to be a portable sparse SQP-based nonlinear programming
solver, aiming for robust performance across a wide selection of benchmark test
problems while remaining usable as ASCEND's native optimization solver. The
current implementation is still a prototype, but the architectural direction is
to keep the numerical solver core independent of ASCEND and reusable from
callback-driven frontends such as CUTEst/SIFDecode.

## Current Feature Summary

Implemented core capabilities include:

- sparse constrained NLP setup through an IPOPT-like C callback API;
- ASCEND and CUTEst/SIFDecode adapters using the same C API path;
- HiGHS-backed elastic QP subproblem solves;
- damped BFGS Hessian approximation on scaled primal variables;
- optional exact objective and exact Lagrangian Hessian callbacks, with
  core-owned lower-triangle packing and PSD regularization;
- merit line search, trust-radius QP retries, and experimental filter-lite
  acceptance;
- optional feasibility restoration with core-owned phase/status counters;
- opt-in least-squares recognition from ASCEND objective expressions, with a
  core-owned Gauss-Newton/Levenberg-Marquardt solve path for unconstrained
  recognised sum-of-squares objectives;
- KKT, bound-stationarity, restoration, and solve-statistics reporting;
- CUTEst benchmarking support against IPOPT, including JSONL outcome
  classification;
- ASCEND progress/status reporting through `liba4sqp_ascend.so`.

Current important gaps are:

- benchmark robustness is still limited; A4SQP is useful for triage and
  development comparison, but not yet a production-grade IPOPT replacement;
- ASCEND scaling needs review now that the ASCEND adapter routes through the
  public C API path;
- fixed-variable removal is not yet implemented as a full presolve/postsolve
  reduction;
- active-bound identification, restoration handoff, and terminal stationarity
  still need core work on BT13-like cases;
- `OpenA4SqpOutputFile` exists for IPOPT API compatibility but is not yet a
  useful reporting sink.

## CUTEst Regression Notes

The current confirmed rebuilt result for the broad 89-problem CUTEst tracking
set is 68/89 genuine A4SQP exact-Lagrangian passes using the experimental LSQ
handoff profile (`--try-lsq LM --lsq-fallback-start improved
--lsq-max-iter 1000`). This was measured on 2026-05-16 with regenerated CUTEst
problem/package objects. The previous signed-row-dual baseline without this
handoff profile was 66/89 plus 6 suspect high-KKT LS exits.

Important correction: `99928f2b` and nearby commits carried reports claiming
69/89, 70/89, or 71/89, but those counts have not reproduced under clean
`--rebuild` CUTEst runs. Treat those older markdown counts as suspect until
proven otherwise with complete rebuilt TSV output.

Recent failed experiment:

- An apparent improvement to ACOPP14, BATCH, CANTILVR, and related cases was
  seen while changing restoration behaviour and exact-Lagrangian Hessian
  multiplier handling.
- A full rebuilt CUTEst rerun did not reproduce the apparent win. The rebuilt
  result fell to 65/89 genuine A4SQP exact-Lagrangian passes, with ACOPP14,
  BATCH, CANTILVR, BT13, and several related cases still failing.
- Restoring the source to the `99928f2b` 69/89 checkpoint and rerunning the
  full matrix with clean rebuilt CUTEst objects gave 66/89, not 69/89. The
  three old report passes not reproduced were BROWNDEN, BT13, and BT5.
- A focused clean rebuild of `11920c19` reproduced BT5 passing, but BROWNDEN
  still failed and BT13 timed out. That older state also lost BT1, BT7, and
  BT12, so its useful behaviour is a real trade-off rather than a net recovery.
- A narrow immediate-restoration-trigger fix is retained for the ASCEND CUnit
  `a4sqp_bt13_restoration_trigger_zero` test. CUTEst uses the normal default
  restoration trigger, so this does not explain the 66/89 versus 69/89
  discrepancy; a focused rebuilt CUTEst check of BROWNDEN, BT13, and BT5
  remained failing with that fix applied.
- The most damaging change was feeding the exact-Lagrangian Hessian callback
  from recovered stationarity multipliers, with fallback to unsigned scaled
  row duals. The rebuilt signed-row-dual baseline uses the signed HiGHS row-dual
  convention expected by the CUTEst/IPOPT-style Hessian callback path.
- Changing restoration trigger semantics also affected the result set and
  should not be reintroduced as default behaviour without a full rebuilt
  matrix run.
- The apparent ACOPP14/BATCH/CANTILVR gains remain a useful lead, but should
  be reintroduced only behind explicit core solver options and validated with
  `CUTEST_REBUILD=1` or the default rebuilt CUTEst runner path.

Focused follow-up on BROWNDEN and ACOPP14, 2026-05-16:

- BROWNDEN is a confirmed least-squares handoff case. The ASCEND-translated
  model passed because a non-converged LSQ pre-solve left its improved iterate
  in place before SQP fallback. The CUTEst adapter had been restoring the
  original point and therefore lost that benefit.
- The CUTEst runner now exposes `--lsq-fallback-start original|improved` and
  `--lsq-max-iter N` so this behaviour is explicit and reproducible. With a
  clean rebuilt CUTEst run, `BROWNDEN` solves strictly using
  `--try-lsq LM --lsq-fallback-start improved --lsq-max-iter 1000`
  (`kkt_error ~= 1.2e-8` after 7 SQP iterations following the LSQ handoff).
- ACOPP14 remains unsolved. BFGS or recovered exact-Lagrangian multipliers avoid
  driver timeouts and produce deterministic max-iteration failures. Disabling
  active-bound restoration improves the 800-iteration BFGS result from
  `kkt_error ~= 937` to `kkt_error ~= 0.287` with acceptable feasibility, but
  longer 3000-iteration runs stall at the same stationarity level. Immediate
  restoration entry can reduce KKT to about `0.093` at 800 iterations but leaves
  feasibility outside the current threshold and worsens on longer runs.
- These ACOPP14 results suggest the remaining issue is not stale runner state
  or parameter plumbing. It is a core stationarity/active-bound/multiplier
  quality problem near a feasible point.

Potential future switches for controlled experiments:

- exact-Lagrangian multiplier source: signed QP row duals versus recovered NLP
  stationarity multipliers;
- active-bound or bound-target restoration probe enabled/disabled;
- restoration trigger policy: immediate, stall-count gated, or infeasibility
  magnitude gated.
- LSQ-to-SQP handoff policy: restore original point versus continue from the
  best LSQ iterate when the LSQ pre-solve improves the point but does not meet
  its own convergence test.

## Current Direction

A4SQP is now structured as one portable SQP solver library with thin adapters.
The current design goal is not two solvers and not one ASCEND-specific solver
plus a separate CUTEst solver. The intended runtime shape is:

```text
ASCEND model
    -> liba4sqp_ascend.so adapter
        -> IPOPT-like A4SQP C API
            -> liba4sqp.so core solver
                -> HiGHS QP backend

SIFDecode / CUTEst problem
    -> CUTEst adapter
        -> IPOPT-like A4SQP C API
            -> liba4sqp.so core solver
                -> HiGHS QP backend
```

`liba4sqp.so` owns solver policy and numerical step computation. Adapter code
owns frontend data translation, evaluation callbacks, status mapping, and
frontend-specific diagnostics.

The public C API is intentionally close to IPOPT's C API so CUTEst and other
external NLP drivers can be wired with minimal solver-specific bridge code.

## Library Split

### `liba4sqp.so`

`liba4sqp.so` is the portable solver library. It must not include ASCEND
headers, call `libascend.so`, or access `slv_system_t`, `struct var_variable`,
or `struct rel_relation`.

The build currently links this library to HiGHS and not ASCEND. Its primary
source files are:

- `solvers/a4sqp/a4sqp_c.c`: public IPOPT-like C API and current solve driver.
- `solvers/a4sqp/a4sqp_core.c`: shared SQP step, merit, line-search,
  convergence, KKT, restoration, multiplier, and bound-activity logic.
- `solvers/a4sqp/a4sqp_core_view.h`: solver-neutral borrowed numeric view.
- `solvers/a4sqp/a4sqp_hessian.c`: dense Hessian lifecycle, BFGS update,
  relation Hessian packing helpers, matrix products, and PSD regularization.
- `solvers/a4sqp/a4sqp_lsq.c`: core Gauss-Newton/Levenberg-Marquardt
  least-squares solve loop driven by residual/Jacobian callbacks.
- `solvers/a4sqp/a4sqp_qp_highs.c`: QP assembly and HiGHS solve glue.
- `solvers/a4sqp/a4sqp_scale.c`: scaling helpers.
- `solvers/a4sqp/a4sqp_trust.c`: trust-radius policy.
- `solvers/a4sqp/a4sqp_view.c`: numeric view lifecycle.
- `solvers/a4sqp/a4sqp_types.h`: core numeric types and allocator macros.

The stable external boundary is `solvers/a4sqp/a4sqp_c.h`. Some additional
core and Hessian helper symbols are exported today because the ASCEND adapter
uses them for exact-Hessian packing and diagnostics. Treat those as internal
adapter support, not as a stable third-party ABI.

Core-owned decisions include merit acceptance, line search, QP construction,
QP retry policy, trust-radius updates, restoration entry/exit, convergence
tests, multiplier estimation, BFGS updates, exact-Hessian packing, and Hessian
PSD regularization.

### `liba4sqp_ascend.so`

`liba4sqp_ascend.so` is the ASCEND solver plugin. It links to `liba4sqp.so` and
`libascend.so`. It must not link directly to HiGHS. The built adapter currently
exports only the ASCEND registration symbol, `a4sqp_register`.

ASCEND adapter code lives in:

- `solvers/a4sqp/asc_a4sqp.c`: ASCEND solver lifecycle, C API callback
  implementation, option transfer, solve/status mapping, and exact Hessian
  callback bridge.
- `solvers/a4sqp/asc_a4sqp_adapter.c`: ASCEND adapter entry points.
- `solvers/a4sqp/asc_a4sqp_diag.c`: ASCEND progress callback bridge.
- `solvers/a4sqp/asc_a4sqp_params.c`: ASCEND solver parameter definitions.
- `solvers/a4sqp/asc_a4sqp_report.c`: ASCEND-facing progress/view/iteration
  reporting.
- `solvers/a4sqp/asc_a4sqp_view.c`: construction of an ASCEND-backed numeric
  view from `slv_system_t`.

The ASCEND adapter is allowed to:

- sort and read ASCEND solver variable and relation lists;
- maintain the ASCEND-side trial vector mapping;
- push a trial A4SQP `x` vector into ASCEND variables for evaluation;
- evaluate objective values, relation residuals, gradients, Jacobians, and
  relation Hessians using ASCEND machinery;
- optionally analyse an ASCEND objective expression for least-squares form and
  pass residual/Jacobian callback evaluations to the core LSQ solve path;
- translate ASCEND options into C API options;
- translate A4SQP statuses into `slv_status_t`;
- render diagnostics with ASCEND variable/relation names and source indices.

The ASCEND adapter should not implement solver policy. In particular,
restoration, filtering, trust-region handling, QP retry policy, convergence
logic, elastic penalty updates, BFGS mechanics, and Hessian PSD repair belong
inside `liba4sqp.so`.

## Callback Boundary

The only frontend-facing solve callbacks should be the IPOPT-like C API
callbacks:

- objective value;
- objective gradient;
- constraint residual vector;
- sparse Jacobian structure and values;
- sparse lower-triangle Hessian of the Lagrangian;
- optional intermediate/progress callback.

ASCEND currently implements these callbacks in `asc_a4sqp.c`. CUTEst implements
the same callback shape in `solvers/a4sqp/cutest/a4sqp_main.c`.

Exact ASCEND Hessian data acquisition remains adapter-side because it requires
`relman` and ASCEND relation handles. The adapter emits objective/relation
second-derivative entries; `liba4sqp.so` owns lower-triangle packing, multiplier
weighting, and PSD regularization.

No restoration, filter, trust-region, or convergence decisions should be made
by the adapters. If a future change requires putting that policy back into
`liba4sqp_ascend.so` or the CUTEst driver, stop and discuss first.

## Numeric View

`A4SqpView` is a core-owned numeric container with optional opaque frontend
handles. Its public core header does not expose ASCEND types.

The portable numeric fields include:

- variable and relation counts;
- objective value and gradient;
- variable values, bounds, fixed flags, nominals, scales, and scaled values;
- relation residuals, bounds, nominals, scales, and scaled values;
- relation kind;
- sparse Jacobian row starts, column indices, and values;
- diagnostic counters for calculation and derivative failures.

The ASCEND adapter stores opaque handles in optional fields so diagnostics and
exact-Hessian evaluation can map back to ASCEND objects. Core code must treat
those handles as opaque.

Current important limitation: `A4SqpView` records fixed variables, but A4SQP
does not yet have a full optimization presolve/postsolve reducer that removes
fixed variables from the NLP and maps the solution back afterward. That remains
a separate cleanup item.

## ASCEND Expression Evaluation Note

ASCEND token relations currently carry both postfix token arrays and infix tree
roots. The postfix arrays are the preferred execution representation for
whole-relation residual and derivative evaluation. The infix tree roots are a
structural view into the same token storage, useful for expression analysis,
printing, simplification, inversion/search logic, and the current
least-squares residual extraction work.

The current ASCEND least-squares analyser uses infix residual subtrees because
they are convenient stable references to extracted residual expressions. A
future cleanup should push value and derivative evaluation down to a single
expression-level subsystem that can evaluate either whole postfix expressions
or partial postfix subexpressions from the same routines. In that design, LSQ
recognition could tag residuals by postfix side/root token, and residual values,
gradients, and higher derivatives would all come from shared expression
evaluation code rather than LSQ-specific recursive evaluators.

## Solver Algorithm Snapshot

A4SQP currently implements an elastic line-search SQP method:

```text
minimize    grad f(x_k)^T p + 1/2 p^T B_k p + rho * sum(elastic_slacks)

subject to  r_L - e_L <= r(x_k) + J_r(x_k) p <= r_U + e_U
            x_L <= x_k + p <= x_U
            ||p||_inf <= trust_radius       (constrained problems)
            e_L, e_U >= 0
```

The default Hessian model is damped BFGS on scaled primal variables.
Experimental exact-Hessian modes are available:

- `EXACT_OBJ` for objective curvature;
- `EXACT_LAGRANGIAN` for Lagrangian curvature;
- `AUTO` for conservative promotion when an exact Hessian callback exists.

HiGHS requires a convex QP, so exact Hessians are regularized to a
positive-semidefinite step model before QP assembly.

Restoration, acceptable convergence, KKT convergence, multiplier recovery, and
bound-stationarity diagnostics are core policy. The adapters only pass options
and relay the resulting status/progress data.

An ASCEND-only experimental parameter, `try_lsq`, can be set to `OFF`, `GAUSS`,
or `LM`. The default is `LM`. When set to `GAUSS` or `LM`, the ASCEND adapter
asks the system layer to recognise an unconstrained sum-of-squares objective and
build a residual-expression view. If that succeeds, `liba4sqp.so` runs the
core least-squares loop using adapter-supplied residual/Jacobian callbacks. If
recognition is not applicable, the ordinary SQP path is used. The least-squares
step policy remains core-owned; the adapter only performs expression analysis,
value updates for callback evaluation, and status/progress translation.

## Public C API

The public C API is declared in:

```text
solvers/a4sqp/a4sqp_c.h
```

It follows IPOPT's C callback shape:

- problem creation with dimensions, bounds, Jacobian nonzeros, Hessian
  nonzeros, index style, and callback table;
- objective, objective-gradient, constraint, Jacobian, and Hessian callbacks;
- string, numeric, and integer option setters;
- solve function that updates `x` in place;
- optional output arrays for constraint values, objective value, constraint
  multipliers, and bound multipliers;
- solve statistics, including phase/restoration counters and KKT diagnostics.

The core solver does not read environment variables. The CUTEst runner and
package driver may read environment variables as harness plumbing, but they
must translate them into ordinary C API options before calling `A4SqpSolve`.

## CUTEst / SIFDecode Bridge

The CUTEst bridge remains thin:

```text
SIFDecode/CUTEst callbacks
    -> A4SQP C callbacks
        -> liba4sqp.so
```

It decodes dimensions, bounds, sparse Jacobian structure, optional sparse
Hessian structure, and initial guesses, then calls the A4SQP C API. It should
not contain solver algorithm logic.

The current comparison target is A4SQP versus IPOPT over CUTEst NLP problems.
SLSQP remains deferred.

For a given mathematical model, the ASCEND and SIFDecode paths should converge
to the same result when solver options, scaling, bound conventions, initial
guesses, and derivative data are harmonized. If the same problem behaves
differently through the two frontends, treat that as an adapter or
normalization bug until proven otherwise.

## Current Test Status

The focused ASCEND CUnit suite is run through:

```text
./a4 cutest solver_a4sqp
```

Current local result after the CUnit skip support update:

```text
21 selected tests: 18 passed, 3 skipped, 0 failed
```

The active suite includes registration, a HiGHS QP smoke test, C API smoke
tests, view/presolve checks, exact-Hessian checks, and model-run regressions
for `hs21`, `bqp1var`, `bt10`, `cb3`, and `jannson3`.

The following model-run regressions remain in the suite but are explicitly
skipped because they previously passed but are not reliable enough for default
CUnit gating:

- `rosenmmx`
- `lubrifc`
- `cont6_qq`

Representative model-level checks should use the normal runner:

```text
./a4 run models/test/a4sqp/hs3.a4c
./a4 run models/test/a4sqp/hs11.a4c
```

Recent CUTEst profiling should be treated as a snapshot, not a permanent
baseline. On the fixed-size smooth NLP sample capped at `n <= 20`, `m <= 20`,
BFGS, 200 iterations, and KKT convergence enabled, the last recorded profile
had 4 strict successes and 11 clean successes when `--acceptable-iter 5` was
enabled. The main failure classes were near-solved maximum-iteration exits,
stationarity failures, line-search failures, infeasible/stalled exits, and one
QP failure.

## Current Open Items

- The ASCEND adapter now calls the public C API, which is the desired simpler
  boundary, but scaling parity still needs continued review. The C API now
  accepts `scaleopt=NONE`, `ROW_2NORM`, and `RELNOM`; the bare C API default is
  still `NONE`, while ASCEND and the CUTEst driver default to `ROW_2NORM`.
  `ROW_2NORM` only scales down oversized rows; it does not amplify rows with
  small Jacobian norms, because that makes degenerate constraints singular near
  solutions such as BT13.
- Fixed-variable reduction is not yet a full presolve/postsolve layer.
- BT13 is the focused active-bound/restoration regression case. The core now
  has an active-bound restoration probe and capped row scaling, and the CUTEst
  BT13 run reaches strict KKT success under the BFGS/KKT profile.
- `OpenA4SqpOutputFile` is present for IPOPT API shape but is not implemented
  as a useful output sink yet.

## Presolve/Postsolve Pin

There is still a separate optimization-layer idea for A4SQP to run a presolve
before the core solve and a postsolve after it.

That layer could remove fixed variables, tighten or substitute simple bounds,
detect redundant rows, rescale or reorder the active NLP, and map the reduced
solution back to the original frontend problem.

This should be implemented without moving ASCEND model access into
`liba4sqp.so`. The likely shape is a core-owned reduced numeric problem plus
adapter-owned mapping data.

## Development Rules

- `liba4sqp.so` must not depend on ASCEND.
- `liba4sqp_ascend.so` must not depend directly on HiGHS.
- `liba4sqp_ascend.so` should expose only ASCEND registration externally.
- Solver algorithm work belongs in `liba4sqp.so`.
- Adapter work is limited to evaluation, option translation, status mapping,
  progress reporting, and frontend diagnostics.
- Use `./a4 run ...` and `./a4 cutest ...` for normal testing so runtime paths
  match the expected user pattern.

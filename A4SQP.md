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
  boundary, but ASCEND-specific scaling needs continued review. The C API
  currently defaults to unscaled quantities unless explicit C API scaling is
  provided, while the ASCEND parameter table still has `scaleopt=ROW_2NORM`.
- Fixed-variable reduction is not yet a full presolve/postsolve layer.
- BT13 remains the focused active-bound/restoration case. Current restoration
  can repair feasibility and hand back to regular SQP, but active-bound
  identification and terminal stationarity near the bound still need core work.
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

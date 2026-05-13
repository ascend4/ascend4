# A4SQP Architecture Notes

## Direction

A4SQP is an SQP-based nonlinear solver prototype for ASCEND and for standalone
callback-driven NLP benchmarking. The current design goal is one solver engine
with adapter layers, not separate ASCEND and CUTEst solvers.

The intended boundary is:

```text
ASCEND model
    -> liba4sqp_ascend.so adapter
        -> liba4sqp.so core solver
            -> HiGHS QP backend

SIFDecode / CUTEst problem
    -> C API / CUTEst adapter
        -> liba4sqp.so core solver
            -> HiGHS QP backend
```

The important principle is that `liba4sqp.so` owns solver policy and numerical
step computation. Frontends own problem evaluation, data translation, and
frontend-specific diagnostics.

## Library Split

### `liba4sqp.so`

`liba4sqp.so` is the portable solver library. It must not include ASCEND
headers, call `libascend.so`, or access `slv_system_t`, `struct var_variable`,
or `struct rel_relation`.

Current direct shared-library dependencies are:

- `libm`
- `libhighs`
- `libc`

Core code currently lives in:

- `solvers/a4sqp/a4sqp_c.c`: IPOPT-like public C API implementation.
- `solvers/a4sqp/a4sqp_core.c`: shared SQP step, merit, line-search,
  convergence, stationarity, violation, and trust-retry logic.
- `solvers/a4sqp/a4sqp_core_view.h`: solver-neutral borrowed numeric view.
- `solvers/a4sqp/a4sqp_hessian.c`: Hessian utilities and PSD regularization.
- `solvers/a4sqp/a4sqp_qp_highs.c`: QP assembly and HiGHS solve glue.
- `solvers/a4sqp/a4sqp_scale.c`: numeric scaling helpers.
- `solvers/a4sqp/a4sqp_trust.c`: trust-region policy.
- `solvers/a4sqp/a4sqp_view.c`: core-owned numeric view storage lifecycle.
- `solvers/a4sqp/a4sqp_types.h`: core numeric types, bound sentinels, and
  allocator macros.

The core solver is allowed to decide when solver status should change, when a
step is accepted, when a QP retry is needed, when convergence is reached, and
how Hessians are regularized before reaching HiGHS.

### `liba4sqp_ascend.so`

`liba4sqp_ascend.so` is the ASCEND solver plugin. It links to `liba4sqp.so` and
`libascend.so`. It must not link directly to HiGHS.

Current direct shared-library dependencies are:

- `libm`
- `liba4sqp`
- `libascend`
- `libc`

ASCEND adapter code currently lives in:

- `solvers/a4sqp/asc_a4sqp.c`: ASCEND solver client callbacks, ASCEND-side solve
  state, exact Hessian assembly from ASCEND relations, and status mapping.
- `solvers/a4sqp/asc_a4sqp_adapter.c`: ASCEND view-build entry point.
- `solvers/a4sqp/asc_a4sqp_adapter.h`: ASCEND adapter declarations.
- `solvers/a4sqp/asc_a4sqp_diag.c`: ASCEND progress callback bridge.
- `solvers/a4sqp/asc_a4sqp_params.c`: ASCEND solver parameter setup.
- `solvers/a4sqp/asc_a4sqp_report.c`: ASCEND-facing progress/view/QP/iteration
  reporting.
- `solvers/a4sqp/asc_a4sqp_view.c`: construction of numeric A4SQP views
  from `slv_system_t`.

The ASCEND adapter is allowed to:

- read solver variable and relation lists from `slv_system_t`;
- push a trial A4SQP `x` vector into ASCEND variables for evaluation;
- read variable values, bounds, fixed flags, nominals, and indices;
- evaluate relation residuals, objective values, Jacobians, and Hessians using
  ASCEND machinery;
- assemble sparse numeric structures from the ASCEND model;
- translate A4SQP statuses into `slv_status_t`;
- render diagnostics with ASCEND names, relation names, variable names, source
  indices, and solver progress callbacks.

The ASCEND adapter should not implement solver policy. In particular, merit
acceptance, line-search behavior, QP retry policy, convergence tests, elastic
row logic, and Hessian PSD regularization belong in core code.

## Numeric View

`A4SqpView` is now a core-owned numeric container with optional opaque frontend
handles. Its public core header does not expose ASCEND types.

The portable fields contain:

- number of variables and relations;
- objective value and objective gradient;
- variable values, bounds, fixed flags, nominals, scales, and scaled values;
- relation residuals, bounds, nominals, scales, and scaled values;
- relation kind;
- sparse Jacobian row starts, column indices, and values;
- diagnostic counters for calculation and derivative failures.

The ASCEND adapter stores `void *` handles in the optional `vars`, `rels`, and
`obj` fields so it can map diagnostics and exact-Hessian work back to ASCEND
objects. Core code treats those handles as opaque and must not dereference them.

This keeps ASCEND integration efficient without forcing ASCEND through the
public C API. Updating ASCEND variables from an A4SQP `x` vector is cheap and is
the right tradeoff for keeping the core API simple and IPOPT-like.

## Public C API

The public C API is declared in:

```text
solvers/a4sqp/a4sqp_c.h
```

It intentionally follows IPOPT's C callback shape:

- problem creation with `n`, `m`, bounds, Jacobian nonzeros, Hessian nonzeros,
  index style, and callbacks;
- objective, objective-gradient, constraint, Jacobian, and Hessian callbacks;
- option setters;
- solve function that updates `x` in place;
- result arrays for constraint values, objective value, constraint multipliers,
  and bound multipliers;
- solve statistics.

This API is the intended boundary for SIFDecode/CUTEst benchmarking and other
non-ASCEND uses. It should stay free of ASCEND concepts.

## CUTEst / SIFDecode Bridge

The CUTEst bridge should remain thin:

```text
SIFDecode/CUTEst callbacks
    -> A4SQP C callbacks
        -> liba4sqp.so
```

It should decode dimensions, bounds, sparse Jacobian structure, optional sparse
Hessian structure, and initial guesses, then call the A4SQP C API. It should
not contain solver algorithm logic.

The comparison target for now is A4SQP versus IPOPT over the CUTEst NLP subset.
SLSQP is deferred.

For a given mathematical model, the ASCEND and SIFDecode paths should converge
to the same result when solver options, scaling, bound conventions, initial
guesses, and derivative data are harmonized. If the same problem behaves
differently through the two frontends, that should be treated as an adapter or
normalization bug until proven otherwise.

## Current Solver Shape

A4SQP currently implements an elastic line-search SQP method:

```text
minimize    grad f(x_k)^T p + 1/2 p^T B_k p + rho * sum(elastic_slacks)

subject to  r_L - e_L <= r(x_k) + J_r(x_k) p <= r_U + e_U
            x_L <= x_k + p <= x_U
            ||p||_inf <= trust_radius       (constrained problems)
            e_L, e_U >= 0
```

The default step Hessian is damped BFGS on scaled primal variables. Experimental
exact-Hessian modes are available:

- `EXACT_OBJ` for objective-only exact curvature;
- `EXACT_LAGRANGIAN` for constrained Lagrangian curvature;
- `AUTO` for conservative promotion on compact constrained problems.

HiGHS requires a convex QP, so exact Hessians are regularized to a
positive-semidefinite step model before QP assembly.

## Current Test Status

The focused ASCEND CUnit suite currently passes through:

```text
./a4 cutest solver_a4sqp
```

The current suite includes registration, HiGHS QP smoke tests, C API smoke
tests, and focused A4SQP model regressions including `hs3`, `hs11`, `hs21`,
`rosenbr`, `rosenmmx`, `jannson3`, reduced `lubrifc`, and reduced `cont6_qq`
cases.

Representative model-level checks are:

```text
./a4 run models/test/a4sqp/hs3.a4c
./a4 run models/test/a4sqp/hs11.a4c
```

Both currently solve with A4SQP.

## Presolve/Postsolve Pin

There is a separate optimization-layer idea that should remain on the design
list: A4SQP could eventually run an optimization presolve before the core solve
and a postsolve after it.

That layer might remove fixed variables, tighten or substitute simple bounds,
detect redundant rows, rescale or reorder the active NLP, and map the reduced
solution back to the original ASCEND problem.

This is not part of the current CUTEst benchmarking bridge. The current split
should first keep the core solver portable and keep the ASCEND adapter faithful.
Presolve/postsolve can later sit above the adapter/core boundary.

## Remaining Cleanup

The binary split is now in place, but the source-level split can still improve:

- Reduce `asc_a4sqp.c` further so it is increasingly ASCEND data extraction,
  evaluation, status mapping, and solve orchestration rather than carrying
  large exact-Hessian implementation blocks.
- Narrow the explicit internal core ABI exported for `liba4sqp_ascend.so` as
  the adapter gets thinner. The broad `-fvisibility=default` build override has
  been removed; exported core symbols are now explicit.
- Decide whether the ASCEND plugin should continue using selected internal
  core symbols directly or move to a small non-public core-driver API distinct
  from the IPOPT-like public C API.
- Add a core-level reporting/logging hook that is frontend-neutral. The ASCEND
  adapter can render those messages through ASCEND progress callbacks and add
  variable/relation names where useful.
- Keep confirming that `liba4sqp_ascend.so` has no direct HiGHS dependency and
  `liba4sqp.so` has no ASCEND dependency.

If any cleanup step requires routing ASCEND through only the IPOPT-like C API
or dropping ASCEND diagnostics, stop and discuss first.

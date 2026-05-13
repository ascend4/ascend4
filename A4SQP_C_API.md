# A4SQP C API, Core Solver Split, and CUTEst Bridge

## Purpose

This note specifies the callback-based C API for A4SQP and documents the
current split between shared A4SQP solver logic, the ASCEND adapter, and the
CUTEst/SIFDecode adapter. The public C API is intentionally close to IPOPT's
`IpStdCInterface.h` so that CUTEst profiling against IPOPT and A4SQP is cheap
to wire up, while preserving the current tight ASCEND-to-A4SQP integration.

The design direction is:

```text
ASCEND slv_system_t adapter
    -> A4SQP-owned iterate vector and numeric view
        -> A4SQP SQP/QP engine
            -> HiGHS QP backend

CUTEst/SIFDecode adapter
    -> A4SQP C API / iterate vector and numeric view
        -> A4SQP SQP/QP engine
            -> HiGHS QP backend
```

The IPOPT-like C API should be a portable external boundary. It should not
become the only ASCEND boundary, because ASCEND can provide richer structure,
names, status, diagnostics, scaling information, and warm-start invalidation
through `slv_system_t`.

The current unification direction is that the SQP core owns the algorithmic
step policy and trial-vector lifecycle. Adapters provide evaluations at a
candidate `x` vector and retain frontend-specific metadata. ASCEND therefore
does not need to be routed through the public C ABI, but the ASCEND and CUTEst
paths no longer maintain separate line-search or step-acceptance algorithms.

## Algorithm Boundary Rule

All restoration, filtering, trust-region, merit-function, Hessian model,
line-search, step-type, convergence, and globalization decisions belong inside
`liba4sqp.so`.

Adapter libraries must not decide that a solve is in "regular" versus
"restoration" mode, must not trigger feasibility-restoration steps, and must
not implement alternate filter or trust-region rules. This is essential: the
same model sent through ASCEND or through SIFDecode/CUTEst must reach the same
A4SQP algorithmic path when options and initial guesses are harmonised.

The allowed adapter responsibilities are deliberately narrower:

- Convert frontend model state into A4SQP numeric vectors, bounds, residuals,
  Jacobian values, and Hessian callback data.
- Evaluate the objective, residuals, derivatives, and Hessians at trial
  vectors requested by the core.
- Pass option values into the core without interpreting algorithmic state.
- Map core status and diagnostics back to frontend names, relations, variables,
  and progress-reporting mechanisms.

`liba4sqp.so` must not read environment variables to alter solver behaviour.
Environment variables are acceptable only as command-line/test-harness plumbing
in executable adapters such as the CUTEst runner, where they must be translated
immediately into ordinary solver options before calling the C API.

For ASCEND specifically, `slv_iterate` is not a license to put SQP phase logic
in `liba4sqp_ascend.so`. If A4SQP is fundamentally a solve-at-once optimizer,
the ASCEND `slv_iterate` hook should remain a light wrapper around the
core-owned solve/step state machine rather than becoming a second control loop.

## Current Implementation Snapshot

The implementation now has one shared SQP step engine with frontend adapters:

```text
ASCEND solver client
    -> ASCEND adapter: solvers/a4sqp/asc_a4sqp.c, asc_a4sqp_view.c
        -> shared core: a4sqp_core.c, a4sqp_hessian.c, a4sqp_trust.c,
                        a4sqp_qp_highs.c, a4sqp_view.c
            -> HiGHS QP backend

C API / CUTEst client
    -> C API adapter: solvers/a4sqp/a4sqp_c.c
        -> shared core: a4sqp_core.c, a4sqp_hessian.c, a4sqp_trust.c,
                        a4sqp_qp_highs.c
            -> HiGHS QP backend
```

The core numerical view is `A4SqpCoreView` in
`solvers/a4sqp/a4sqp_core_view.h`. It is a borrowed, solver-neutral slice of
the current NLP state: scaled variables, bounds, objective gradient, constraint
residuals, constraint bounds, row kinds, and sparse Jacobian data.

`A4SqpView` in `solvers/a4sqp/a4sqp_view.h` is now a core-owned numeric
container with optional opaque frontend handles. Its header does not expose
ASCEND types. The ASCEND adapter may store `void *` handles for variables,
relations, and the objective so it can map diagnostics and exact-Hessian work
back to ASCEND objects, but core code must treat those handles as opaque.

The public external API is `solvers/a4sqp/a4sqp_c.h`. Its implementation in
`solvers/a4sqp/a4sqp_c.c` builds numeric `A4SqpView`/`A4SqpCoreView` data from
IPOPT-like callbacks without fabricating ASCEND source indices or relation
objects.

Current focused external CUTEst status for the translated smoke set is:

- `HS3`: pass.
- `HS11`: pass after moving feasible/null step acceptance into the shared core
  path used by both ASCEND and the C API.
- `HS21`: pass.
- `ROSENBR`: reaches objective near zero but still returns max-iteration
  because the unconstrained projected-gradient convergence test remains too
  strict for this path.
- `ROSENMMX`: no longer reports a false iteration-zero success; it still times
  out under the current 30 second smoke timeout.

## Relevant Existing Code

- IPOPT C API header: `/usr/include/coin/IpStdCInterface.h`
- IPOPT return codes: `/usr/include/coin/IpReturnCodes_inc.h`
- ASCEND IPOPT bridge: `solvers/ipopt/asc_ipopt.c`
- Current A4SQP implementation: `solvers/a4sqp`
- Current A4SQP design note: `A4SQP.md`
- CUTEst IPOPT driver: `/home/john/CUTEst/src/ipopt/ipopt_main.F`
- CUTEst C API documentation: `/home/john/CUTEst/doc/README.C`
- CUTEst C API example: `/home/john/CUTEst/src/test/ctest_c.c`

The CUTEst IPOPT driver is not a C `IpStdCInterface.h` client. It is a Fortran
driver using IPOPT's Fortran-style `IPCREATE`, `IPSOLVE`, and callback routines.
It is still useful because its callback shape mirrors the IPOPT C API closely:
objective, objective gradient, constraints, sparse Jacobian, and sparse
Lagrangian Hessian. For an A4SQP C test harness, the better direct starting
point is CUTEst's C-oriented API in `cutest_c.h`, with the IPOPT Fortran driver
used as a semantic reference.

## Non-Negotiable Design Constraints

- Do not force the ASCEND solver client to communicate only through the
  IPOPT-like C ABI.
- Do not remove `slv_system_t`-aware diagnostics from the ASCEND integration.
- Do not flatten ASCEND row and variable identity before diagnostics have
  enough information to map failures back to model instances.
- Do not move `slv_system_t`, `var_variable`, `rel_relation`, or
  `slv_parameters_t` dependencies into the portable A4SQP core interface.
- Do not route ASCEND through the IPOPT-like C ABI. Shared solver behaviour
  must sit below both front ends as core code with adapter hooks.

If implementation work requires routing ASCEND through only the IPOPT-like C
callback ABI, stop and discuss first. That would likely degrade the current
ASCEND integration.

## Public C API Shape

The authoritative public external header is:

```text
solvers/a4sqp/a4sqp_c.h
```

Its exported names avoid colliding with IPOPT, but its typedefs and function
signatures remain close enough that an IPOPT C driver can be mechanically
adapted by changing prefixes and return-code names.

```c
typedef double A4SqpNumber;
typedef int A4SqpIndex;
typedef int A4SqpInt;
typedef int A4SqpBool;
typedef void *A4SqpUserDataPtr;

struct A4SqpProblemInfo;
typedef struct A4SqpProblemInfo *A4SqpProblem;
```

The callback table follows IPOPT's C interface shape:

```c
typedef A4SqpBool (*A4SqpEvalFCB)(
    A4SqpIndex n,
    A4SqpNumber *x,
    A4SqpBool new_x,
    A4SqpNumber *obj_value,
    A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalGradFCB)(
    A4SqpIndex n,
    A4SqpNumber *x,
    A4SqpBool new_x,
    A4SqpNumber *grad_f,
    A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalGCB)(
    A4SqpIndex n,
    A4SqpNumber *x,
    A4SqpBool new_x,
    A4SqpIndex m,
    A4SqpNumber *g,
    A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalJacGCB)(
    A4SqpIndex n,
    A4SqpNumber *x,
    A4SqpBool new_x,
    A4SqpIndex m,
    A4SqpIndex nele_jac,
    A4SqpIndex *iRow,
    A4SqpIndex *jCol,
    A4SqpNumber *values,
    A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalHCB)(
    A4SqpIndex n,
    A4SqpNumber *x,
    A4SqpBool new_x,
    A4SqpNumber obj_factor,
    A4SqpIndex m,
    A4SqpNumber *lambda,
    A4SqpBool new_lambda,
    A4SqpIndex nele_hess,
    A4SqpIndex *iRow,
    A4SqpIndex *jCol,
    A4SqpNumber *values,
    A4SqpUserDataPtr user_data
);
```

An optional intermediate callback reports iteration diagnostics in an
IPOPT-compatible order:

```c
typedef A4SqpBool (*A4SqpIntermediateCB)(
    A4SqpIndex alg_mod,
    A4SqpIndex iter_count,
    A4SqpNumber obj_value,
    A4SqpNumber inf_pr,
    A4SqpNumber inf_du,
    A4SqpNumber mu,
    A4SqpNumber d_norm,
    A4SqpNumber regularization_size,
    A4SqpNumber alpha_du,
    A4SqpNumber alpha_pr,
    A4SqpIndex ls_trials,
    A4SqpUserDataPtr user_data
);
```

The public lifecycle and solve functions are:

```c
A4SqpProblem CreateA4SqpProblem(
    A4SqpIndex n,
    A4SqpNumber *x_L,
    A4SqpNumber *x_U,
    A4SqpIndex m,
    A4SqpNumber *g_L,
    A4SqpNumber *g_U,
    A4SqpIndex nele_jac,
    A4SqpIndex nele_hess,
    A4SqpIndex index_style,
    A4SqpEvalFCB eval_f,
    A4SqpEvalGCB eval_g,
    A4SqpEvalGradFCB eval_grad_f,
    A4SqpEvalJacGCB eval_jac_g,
    A4SqpEvalHCB eval_h
);

void FreeA4SqpProblem(A4SqpProblem problem);

A4SqpBool AddA4SqpStrOption(A4SqpProblem problem, char *keyword, char *val);
A4SqpBool AddA4SqpNumOption(A4SqpProblem problem, char *keyword, A4SqpNumber val);
A4SqpBool AddA4SqpIntOption(A4SqpProblem problem, char *keyword, A4SqpInt val);

A4SqpBool OpenA4SqpOutputFile(A4SqpProblem problem, char *file_name, A4SqpInt print_level);

A4SqpBool SetA4SqpProblemScaling(
    A4SqpProblem problem,
    A4SqpNumber obj_scaling,
    A4SqpNumber *x_scaling,
    A4SqpNumber *g_scaling
);

A4SqpBool SetA4SqpIntermediateCallback(
    A4SqpProblem problem,
    A4SqpIntermediateCB intermediate_cb
);

enum A4SqpApplicationReturnStatus A4SqpSolve(
    A4SqpProblem problem,
    A4SqpNumber *x,
    A4SqpNumber *g,
    A4SqpNumber *obj_val,
    A4SqpNumber *mult_g,
    A4SqpNumber *mult_x_L,
    A4SqpNumber *mult_x_U,
    A4SqpUserDataPtr user_data
);

A4SqpBool GetA4SqpSolveStatistics(
    A4SqpProblem problem,
    struct A4SqpSolveStats *stats
);
```

The return-code numeric values intentionally mirror IPOPT's
`ApplicationReturnStatus` where the meanings overlap. `struct
A4SqpSolveStats` currently reports iterations, QP solves/failures, line-search
failures, final objective, max constraint violation, projected-gradient proxy,
KKT error, dual infeasibility, complementarity, final step norm, final trust
radius, final elastic activity, and Hessian regularization size.

## IPOPT Compatibility Semantics

The following semantics should match IPOPT unless there is a documented A4SQP
reason not to.

- `CreateA4SqpProblem` copies variable and constraint bounds internally.
- `x_L`, `x_U`, `g_L`, and `g_U` use IPOPT-like infinity thresholds, controlled
  by options equivalent to `nlp_lower_bound_inf` and `nlp_upper_bound_inf`.
- `index_style == 0` means C-style zero-based sparse indices.
- `index_style == 1` means Fortran-style one-based sparse indices.
- `eval_jac_g(..., values == NULL)` requests the Jacobian sparsity structure.
- `eval_jac_g(..., values != NULL)` requests Jacobian values at `x`.
- `eval_h(..., values == NULL)` requests the lower-triangular Hessian sparsity
  structure.
- `eval_h(..., values != NULL)` requests Hessian values at `x`, `obj_factor`,
  and `lambda`.
- `eval_h == NULL` or `nele_hess == 0` means use the selected approximation
  mode, initially BFGS.
- `eval_f == NULL` is allowed only for feasibility-only problems; the objective
  is treated as zero.
- `m == 0` objective-only problems are valid.
- Callback failure returns `A4SQP_FALSE` and maps to
  `A4SqpInvalidNumberDetected` or `A4SqpErrorInStepComputation` depending on
  context.
- `A4SqpSolve` updates `x` in-place with the final primal iterate.
- If non-NULL, `g`, `obj_val`, `mult_g`, `mult_x_L`, and `mult_x_U` receive
  final values on return.
- Warm-start input multipliers are accepted only when an option explicitly
  enables warm start. Until then they may be ignored on input but should still
  be written on output when available.

## A4SQP-Specific Options

The C API supports current A4SQP options, plus common IPOPT aliases that make
existing driver code easier to reuse.

Native A4SQP option names:

- `safeeval`: boolean, default false.
- `scaleopt`: string, `NONE`, `ROW_2NORM`, or `RELNOM`; default `ROW_2NORM`.
- `hessian`: string, `AUTO`, `BFGS`, `EXACT_OBJ`, or `EXACT_LAGRANGIAN`;
  default `BFGS`.
- `hess_reg`: number, default `1e-8`.
- `max_iter`: integer, default should be raised for CUTEst profiling from the
  current ASCEND-focused default.
- `max_backtrack`: integer.
- `feas_tol`: number.
- `step_tol`: number.
- `acceptable_tol`: number, default `1e-5`.
- `acceptable_iter`: integer, default `0`; values greater than zero enable
  IPOPT-like relaxed termination with return code
  `A4SqpSolvedToAcceptableLevel`.
- `kkt_convergence`: integer/bool, default `0`; when enabled for objective
  problems, strict and acceptable convergence require the core KKT residual to
  satisfy the active tolerance instead of allowing the older constrained
  small-step shortcut to report success.
- `merit_tol`: number.
- `armijo_coeff`: number.
- `elastic_penalty`: number.
- `filter_accept`: integer/bool, default `0`; enables an experimental
  constrained filter-lite line-search acceptance rule.
- `filter_margin`: number, default `1e-4`; required fractional violation
  reduction for `filter_accept`.
- `trust_unconstrained`: integer/bool, default `0`; applies the existing
  trust-region bound and trust retries to objective-only problems.
- `restoration`: integer/bool, default `0`; enables the core-owned feasibility
  restoration phase.
- `restoration_trigger_iter`: integer, default `3`; number of consecutive
  non-improving infeasible iterations before the core requests restoration
  steps.
- `restoration_improve`: number, default `1e-3`; fractional max-violation
  improvement needed to reset the core restoration stall counter.
- `restoration_margin`: number, default `1e-4`; fractional violation reduction
  needed for restoration line-search acceptance.
- `trust_radius_init`: number.
- `trust_radius_min`: number.
- `trust_radius_max`: number.
- `trust_shrink`: number.
- `trust_grow`: number.
- `trust_accept`: number.
- `trust_good`: number.
- `trust_qp_retries`: integer.
- `progress_log`: boolean.
- `verbosity`: integer.

Compatibility aliases:

- `tol` maps to a default bundle for feasibility and stationarity tolerances.
- `constr_viol_tol` maps to `feas_tol`.
- `acceptable_tol` and `acceptable_iter` follow IPOPT's relaxed convergence
  option names directly.
- `max_cpu_time` maps to a new elapsed-time limit.
- `print_level` maps to `verbosity`.
- `hessian_approximation=limited-memory` maps to `hessian=BFGS` initially.
- `hessian_approximation=exact` maps to `hessian=AUTO` or an explicit exact
  mode depending on whether constraints are present.
- `nlp_lower_bound_inf` and `nlp_upper_bound_inf` set infinity thresholds.

Unknown IPOPT options should return `A4SQP_FALSE`, matching IPOPT's option API,
but the CUTEst benchmark driver should log them rather than silently continue.

## Core/Adapter Split

The current split is view-based rather than a fully separate `A4SqpNlp` object.
That is deliberate: it lets us share the actual SQP decision logic now without
forcing ASCEND through the public C ABI. The next build-level boundary is:

```text
liba4sqp.so
    links to: libhighs, libm, libc
    owns: IPOPT-like A4SQP C API, SQP core, QP builder, Hessian model and
          PSD regularization, trust policy, scaling, numeric view storage

liba4sqp_ascend.so
    public symbols: ASCEND solver registration only
    links to: liba4sqp.so, libascend.so, libm, libc
    owns: slv_system_t adapter, ASCEND diagnostics, ASCEND exact Hessian path
```

This is not intended to route ASCEND through the public C API. It is intended
to prevent duplicate solver engines. The ASCEND plugin should call shared
core routines from `liba4sqp.so`; external users should use the C API
functions declared in `a4sqp_c.h`.

Shared core logic lives in:

- `solvers/a4sqp/a4sqp_core.c`: merit, violation, projected-gradient proxy,
  row activity, convergence policy, vector-owned line search, and the shared
  SQP step loop.
- `solvers/a4sqp/a4sqp_core_view.h`: solver-neutral borrowed numeric view used
  by core helpers.
- `solvers/a4sqp/a4sqp_qp_highs.c`: QP assembly from `A4SqpCoreView` and HiGHS
  QP solve glue.
- `solvers/a4sqp/a4sqp_hessian.c`: dense Hessian model lifecycle, damped BFGS
  update, matrix-vector products, and PSD regularization.
- `solvers/a4sqp/a4sqp_trust.c`: trust-radius initialization, shrink, and grow
  policy.
- `solvers/a4sqp/a4sqp_view.c`: core-owned numeric view lifecycle.
- `solvers/a4sqp/a4sqp_types.h`: core numeric types, bound sentinels, and
  allocator macros.

The ASCEND adapter remains in `solvers/a4sqp/asc_a4sqp*.c` files. It is
responsible for:

- storing opaque handles to ASCEND variables, relations, and the objective
- `var_mindex`, `var_sindex`, `rel_sindex`
- relation operator enums from ASCEND
- pushing the core-owned trial `x` vector into ASCEND variables for evaluation
- rebuilding residuals/Jacobians through ASCEND evaluation machinery
- exact Hessian assembly from ASCEND `relman`
- `slv_status_t`, progress output, model/source-index diagnostics, and block
  status spoofing

Those responsibilities need to stay ASCEND-side because the public C API has no
concept of ASCEND instances, names, solver parameter blocks, source indices, or
`relman` Hessian internals. Routing ASCEND through the C ABI would discard that
information or force it back through non-portable side channels.

The C API adapter remains in `solvers/a4sqp/a4sqp_c.c`. It is responsible for:

- copying IPOPT-like bounds and options into A4SQP-owned storage
- calling user callbacks for objective, gradient, constraints, Jacobian, and
  Hessian data
- building numeric `A4SqpView` fields directly, without fake ASCEND metadata
- maintaining the C solve's `x` array during trial points
- mapping core results back to IPOPT-like return codes and solve statistics

Approximate Hessian ownership is core-side. Both the C API and ASCEND adapter
store their BFGS approximation as `struct A4SqpDenseHessian` and call
`a4sqp_dense_hessian_bfgs_update`; neither adapter implements the BFGS formula
or PSD repair. Exact Hessian assembly remains adapter-side because ASCEND
exact Hessians come from `relman`, while callback/CUTEst exact Hessians come
from IPOPT-like `eval_h` data.

The C API now supports opt-in acceptable convergence. It uses the same shared
core convergence check as strict convergence, but with `acceptable_tol` and a
consecutive-iteration counter. Defaults are strict (`acceptable_iter = 0`) so
ASCEND and CUTEst runs remain comparable unless the benchmark profile
explicitly enables relaxed termination.

The C API now also reports a core KKT residual and can use it for convergence
when `kkt_convergence=1`. The residual combines scaled primal infeasibility, a
bound-aware Lagrangian stationarity residual using QP row multipliers where
available, and constraint complementarity. The diagnostic tries both
row-multiplier signs and records the lower-residual sign in `kkt_lambda_sign`,
because QP/front-end sign conventions can otherwise obscure the real
stationarity size. The C API and ASCEND adapter default `kkt_convergence` off
for compatibility; the CUTEst runner defaults it on for benchmark pass-rate
reporting.

The C API also exposes experimental globalization controls for CUTEst triage.
`filter_accept` is constrained-only and can accept a merit-decreasing step when
constraint violation improves enough even if the predicted-reduction ratio
rejects it. `trust_unconstrained` applies the existing trust-region radius to
objective-only QPs. `restoration` enables the core-owned feasibility
restoration phase: the adapter only passes option values and the core decides
when to enter or leave restoration mode. These controls are disabled by default.

The CUTEst bridge under `solvers/a4sqp/cutest` is intentionally thin. It
decodes SIF problems through CUTEst/SIFDecode, implements the C API callbacks,
and records benchmark JSONL. It should not contain solver algorithm logic.

## Why Any Adapter Callbacks Remain

The callback surface has been reduced. The old adapter-owned line-search
apply/restore/accept hooks and null/feasible step-acceptance hooks are gone.
The shared core now owns trial-vector construction, rejected-trial restore,
merit acceptance, null-QP acceptance, feasible-linearized-step acceptance,
trust retries, and line-search failure classification.

The remaining callbacks are boundaries where the core still must ask a
frontend or backend to perform work it does not own:

- `A4SqpVectorLineSearchOps.evaluate`: evaluate objective, constraints,
  gradient, and Jacobian at a candidate physical `x` vector, then refresh the
  numeric view.
- `A4SqpVectorLineSearchOps.accepted`: optional accepted-step bookkeeping,
  currently used to notify adapters that the core accepted a step so they can
  pass the old/new gradient data to the core-owned BFGS model.
- `A4SqpCoreStepOps.prepare_hessian`: prepare the step Hessian from the
  selected frontend/backend source.
- `A4SqpCoreStepOps.solve_qp`: solve the assembled QP, currently with HiGHS.
- `A4SqpCoreStepOps.after_qp_solve`: optional multiplier and diagnostic
  bookkeeping.
- `A4SqpCoreStepOps.shrink_trust`: update frontend-owned trust/status
  bookkeeping and report diagnostics.

The important rule is:

- Solver policy belongs in core: merit tests, line-search acceptance, trust
  retry logic, convergence checks, QP elastic handling, feasible/null step
  acceptance, BFGS update mechanics, and PSD Hessian regularization.
- Data ownership belongs in adapters: how to evaluate a candidate vector,
  update ASCEND status, preserve model/source diagnostics, and handle
  frontend-specific Hessian sources.

One ASCEND compatibility guard remains deliberately outside the core policy:
the ASCEND adapter currently skips objective-only BFGS updates for constrained
ASCEND models, matching the pre-split behaviour and preserving the existing
ASCEND regression suite. The C API path updates the core BFGS model for
constrained problems. Removing this guard should be treated as an explicit
algorithm change, not as adapter cleanup.

The recent C API parity fix follows this rule by moving feasible/null step
acceptance into `a4sqp_core_solve_step` and making both ASCEND and C API use
`a4sqp_core_line_search_vector`. The C API also guards the constrained
small-step convergence policy so it cannot report success at iteration zero
merely because the initial `last_step_norm` is zero.

Acceptable convergence follows the same rule: core computes the convergence
predicate, while adapters only store options, counters, and frontend status
mapping. The ASCEND adapter reports acceptable termination as converged only
when `acceptable_iter > 0`; otherwise existing model assertions continue to use
strict termination.

## CUTEst/SIFDecode Bridge Plan

For the initial NLP subset, use the C-oriented CUTEst API from `cutest_c.h`.
The driver should be modelled on:

- `/home/john/CUTEst/src/ipopt/ipopt_main.F` for IPOPT callback semantics.
- `/home/john/CUTEst/src/test/ctest_c.c` for C setup and zero-based indexing.

Recommended setup sequence:

1. Open `OUTSDIF.d` or `c_OUTSDIF.d`, depending on the selected CUTEst C entry
   points and build mode.
2. Call `CUTEST_cdimen_c_r` to get `n` and `m`.
3. Call `CUTEST_cnoobj_c_r` to detect feasibility-only cases.
4. Call `CUTEST_classification_c_r` and store classification in the benchmark
   record.
5. Call `CUTEST_csetup_c_r` to obtain initial `x`, variable bounds, constraint
   bounds, initial multipliers, and equality/linearity flags.
6. Call `CUTEST_cdimsj_c_r` or equivalent to size sparse Jacobian storage.
   Verify whether the C-oriented routine includes objective-gradient entries in
   the returned count. The IPOPT Fortran driver subtracts `n` after
   `CUTEST_cdimsj_r`.
7. Call `CUTEST_cdimsh_c_r` to size sparse Lagrangian Hessian storage.
8. Build an `A4SqpProblem` with the same dimensions, bounds, callback table,
   and index style.
9. Apply benchmark options consistently across IPOPT, SLSQP, and A4SQP.
10. Call `A4SqpSolve`, record status, objective, infeasibility, iterations,
    callback counts, CPU time, QP retries, and elastic activity.
11. Call `CUTEST_creport_c_r` and `CUTEST_cterminate_c_r`.

Callback mapping:

- `eval_f`: call `CUTEST_cofg_c_r` with gradient disabled, or `CUTEST_uofg_c_r`
  for unconstrained problems.
- `eval_grad_f`: call `CUTEST_cofg_c_r` with gradient enabled.
- `eval_g`: call `CUTEST_ccfg_c_r` or `CUTEST_cfn_c_r` for constraint values.
- `eval_jac_g`: use the sparse constraint-gradient routine. For `values ==
  NULL`, fill structure; for `values != NULL`, fill numeric values. Keep
  objective-gradient entries out of the constraint Jacobian if the selected
  CUTEst routine returns both.
- `eval_h`: use sparse Hessian-of-Lagrangian routines. Respect IPOPT's
  `obj_factor` convention. For A4SQP BFGS runs, pass `eval_h == NULL`.

SLSQP comparison:

- Dense SLSQP can use the same CUTEst adapter, but it will need dense
  constraint and Jacobian buffers.
- The benchmark harness should record when a problem is skipped for SLSQP due
  to size, memory, unsupported bounds, or excessive dense Jacobian cost.

## Benchmark Record Schema

Each CUTEst result should include:

- problem name
- classification string
- `n`
- `m`
- number of equalities
- number of inequalities
- number of finite lower and upper variable bounds
- Jacobian nonzeros
- Hessian nonzeros if used
- solver name
- solver option profile
- return status
- final objective
- final maximum constraint violation
- final stationarity proxy where available
- major iterations
- function, gradient, constraint, Jacobian, and Hessian callback counts
- elapsed solve time
- setup time
- A4SQP-specific QP failures, QP retries, line-search failures, final trust
  radius, and final maximum elastic activity

The first output format should be CSV or JSON lines. JSON lines are preferable
if A4SQP diagnostics are included.

## Implementation Phases

Current implementation status:

- Phase 1 has a first callback-based implementation in `solvers/a4sqp/a4sqp_c.h`
  and `solvers/a4sqp/a4sqp_c.c`.
- The C API supports BFGS, exact objective Hessians, and exact Lagrangian
  Hessians via the IPOPT-like `eval_h` callback.
- ASCEND and the C API both use the shared core dense Hessian model for BFGS
  updates and PSD regularization, plus shared trust-radius policy,
  convergence checks, merit/violation
  calculations, line search, and the SQP step retry loop.
- `solvers/a4sqp/a4sqp_core_view.h` now defines a borrowed
  solver-neutral `A4SqpCoreView` slice for core numeric data. `A4SqpView`
  preserves numeric fields plus optional opaque frontend handles; ASCEND casts
  those handles only inside adapter code for diagnostics and exact-Hessian
  assembly.
- Core merit/violation/stationarity helpers and QP assembly have
  `A4SqpCoreView` entry points, with `A4SqpView` wrappers kept for existing
  ASCEND call sites.
- Core line search now has an `A4SqpCoreView` entry point. ASCEND and C API
  adapters refresh frontend-owned state after trial evaluation, then expose the
  current numeric slice back to core through a `get_core_view` hook.
- The C API adapter no longer fabricates ASCEND-style `var_sindex`,
  `rel_sindex`, `relop`, or `jac_col_sindex`; it fills the numeric/core fields
  directly.
- The implementation reuses the existing A4SQP HiGHS QP builder and does not
  route the ASCEND `slv_system_t` solver client through the C ABI.
- The remaining ASCEND-specific code is now adapter responsibility: view
  construction, `slv_system_t` state mutation, Hessian assembly from `relman`,
  progress reporting, diagnostics, and status mapping.
- The build now produces a real library split: `liba4sqp.so` links to HiGHS and
  not ASCEND; `liba4sqp_ascend.so` links to `liba4sqp.so` and ASCEND, and not
  directly to HiGHS.
- The temporary broad `-fvisibility=default` core build override has been
  removed. Public C API symbols and the current internal core ABI used by the
  ASCEND adapter are exported explicitly.
- The C API currently has a CUnit smoke test for an objective-only callback
  problem in `ascend/solver/test/test_a4sqp.c`.
- CUTEst package-style drivers now exist under `solvers/a4sqp/cutest` for
  A4SQP and for IPOPT's installed C API. They can be registered into `$CUTEST`
  with `install_cutest_package.sh` and run through `runcutest -p a4sqp -D
  PROBLEM` or `runcutest -p ipoptc -D PROBLEM`.
- `run_a4sqp_cutest.py` runs a problem list with `--solver a4sqp`, `--solver
  ipoptc`, or `--solver both`, collecting JSONL benchmark records plus raw
  per-problem logs.
- The CUTEst A4SQP driver exposes acceptable convergence through
  `A4SQP_ACCEPTABLE_ITER`/`A4SQP_ACCEPTABLE_TOL`, and the runner exposes the
  same controls as `--acceptable-iter`/`--acceptable-tol`. Acceptable
  convergence is disabled by default.
- The CUTEst A4SQP driver exposes KKT-residual convergence through
  `A4SQP_KKT_CONVERGENCE`, and the runner exposes the same control as
  `--kkt-convergence`/`--no-kkt-convergence`. The C API default is off for
  compatibility, but the runner default is on so status-success counts require
  small stationarity residuals.
- The CUTEst runner records `outcome_class` for failure taxonomy, including
  `strict_success`, `strict_success_high_kkt`, `acceptable_success`,
  `acceptable_success_high_kkt`, `max_iter_near_solved`,
  `max_iter_stationarity`, `max_iter_infeasible_or_stalled`,
  `line_search_error`, `qp_failure`, and driver-level errors.
- The CUTEst A4SQP driver exposes experimental globalization controls through
  `A4SQP_FILTER_ACCEPT`, `A4SQP_FILTER_MARGIN`,
  `A4SQP_TRUST_UNCONSTRAINED`, `A4SQP_RESTORATION`,
  `A4SQP_RESTORATION_TRIGGER_ITER`, `A4SQP_RESTORATION_IMPROVE`, and
  `A4SQP_RESTORATION_MARGIN`; the runner exposes these as `--filter-accept`,
  `--filter-margin`, `--trust-unconstrained`, `--restoration`,
  `--restoration-trigger-iter`, `--restoration-improve`, and
  `--restoration-margin`.
- `make_cutest_problem_list.py` builds starter problem lists from
  `/home/john/MASTSIF/CLASSF.DB`, with filters for smoothness, derivative
  degree, and fixed-size `n`/`m` caps.
- SLSQP is intentionally deferred. The immediate benchmark target is A4SQP vs
  IPOPT over the CUTEst NLP subset.
- The standalone C path now rejects non-finite callback values, reports JSON
  `null` for non-finite numeric fields, uses an active-constraint projected
  gradient proxy for benchmark stationarity diagnostics, and exposes CUTEst
  exact Hessian callbacks through `A4SQP_HESSIAN=EXACT_OBJ` or
  `A4SQP_HESSIAN=EXACT_LAGRANGIAN`.
- The C API adapter now wires the same shared-core null-QP and feasible
  linearized-step acceptance hooks as ASCEND. This fixed the external CUTEst
  `HS11` line-search failure without changing the core SQP algorithm.
- The C API constrained small-step convergence shortcut is guarded so it cannot
  report success at iteration zero before any A4SQP step has been accepted.
- On a 20-problem fixed-size smooth NLP sample (`n <= 20`, `m <= 20`, BFGS,
  200 iterations), strict A4SQP with legacy small-step convergence returned
  success on 6/20. Enabling `--acceptable-iter 5` returned success or
  acceptable success on 12/20,
  converting near-solved iteration-limit exits for `ALLINITU`, `BARD`, `BEALE`,
  `BIGGS3`, `BIGGS5`, and `BOX2`.
- On the same sample, `--filter-accept` and `--trust-unconstrained` did not
  improve pass count, so they remain experimental rather than recommended
  default profiles.
- With KKT convergence enabled, the same strict sample had 4 clean status
  successes, 7 near-solved maximum-iteration exits, 5 stationarity
  maximum-iteration exits, 2 line-search failures, 1 infeasible/stalled
  maximum-iteration exit, and 1 QP failure.
- With both KKT convergence and `--acceptable-iter 5`, the same sample had
  11/20 clean status successes: 4 strict successes and 7 acceptable successes.

Phase 1: Header and standalone callback smoke tests.

- Add `a4sqp_c.h`.
- Add an opaque `A4SqpProblemInfo`.
- Implement option storage independent of ASCEND's `slv_parameters_t`.
- Implement simple callback-based objective-only and constrained toy problems.
- Do not modify the ASCEND solver path except for shared declarations that do
  not change behaviour.

Phase 2: Introduce the solver-neutral core problem object.

- Add `A4SqpNlp` and `A4SqpEvalOps`.
- Add a builder from `A4SqpProblemInfo` to `A4SqpNlp`.
- Add adapter-level scaling and sparse structure handling.
- Keep current `a4sqp_view_build(slv_system_t, ...)` working. Incremental
  progress: `A4SqpCoreView` exists as a solver-neutral numeric slice, core math
  uses neutral Jacobian column indices and neutral row kind, and C API view
  construction no longer populates fake ASCEND metadata. Core line search now
  reads the current trial state through `A4SqpCoreView`.

Phase 3: Move SQP/QP assembly off ASCEND-owned structures.

- Split core SQP iteration state from `struct A4SqpSystem`. The first step is
  complete for SQP step sequencing: `a4sqp_core_solve_step` owns Hessian
  prepare, QP build/solve, trust retries, line search acceptance, and failure
  classification through adapter hooks.
- Keep ASCEND diagnostics in the ASCEND adapter.
- Replace direct variable writes in core step acceptance with adapter hooks:
  evaluate at trial point, accept point, reject point. This is complete for the
  shared line search layer.
- Replace direct `relman` Hessian access in core exact-Hessian code with
  `eval_h`.

Phase 4: CUTEst driver.

- Add an external benchmark driver using CUTEst C APIs and `A4SqpProblem`.
- First run the NLP subset with `hessian=BFGS`.
- Add optional exact-Hessian runs through `eval_h`; exact Hessians must be
  symmetrized and regularized to PSD before QP assembly, matching the existing
  ASCEND dense-Hessian behaviour.
- Add IPOPT and SLSQP runners using the same decoded problem and output schema.

Phase 5: ASCEND convergence and diagnostics preservation.

- Compare current ASCEND A4SQP regressions before and after core migration.
- Preserve row/variable names and `slv_status_t` reporting.
- Add optional metadata APIs only after the core callback path is working.

## Optional Metadata Extensions

These should not be required for IPOPT compatibility, but they are useful for
ASCEND diagnostics and benchmark reporting.

```c
A4SqpBool SetA4SqpProblemName(A4SqpProblem problem, const char *name);
A4SqpBool SetA4SqpVariableNames(A4SqpProblem problem, const char *const *names);
A4SqpBool SetA4SqpConstraintNames(A4SqpProblem problem, const char *const *names);
A4SqpBool SetA4SqpProblemClassification(A4SqpProblem problem, const char *classification);
A4SqpBool GetA4SqpSolveStatistics(A4SqpProblem problem, struct A4SqpSolveStats *stats);
```

Metadata must be optional. The core solver should work without names or source
maps.

## Main Risk

The largest technical risk is now exact-Hessian and diagnostic coupling rather
than duplicate solver loops. The core SQP step, line search, QP assembly,
trust retry, convergence checks, BFGS model, and PSD regularization are shared.
Exact Hessian assembly, scaling choices, progress reporting, and diagnostics
still have frontend-specific pieces.

The C API must not regress solver-core behaviour that already exists in the
ASCEND path. Standalone callback solves and ASCEND solves now share the dense
BFGS/PSD implementation; future restoration, filtering, trust-region, or
Hessian recovery work must extend `liba4sqp.so`, not adapter code.

The mitigation is incremental extraction:

- Add the C API and core problem object first.
- Test it on standalone callback problems.
- Wire CUTEst through the C API.
- Migrate core SQP/QP behaviour into shared routines with adapters at every
  ASCEND-specific boundary.
- Keep the ASCEND adapter richer than the IPOPT-like ABI.

That preserves the current ASCEND path while creating the bridge needed for
CUTEst profiling.

# A4SQP C API, Core Solver Split, and CUTEst Bridge

## Purpose

This note documents the current callback-based C API for A4SQP and the split
between the portable solver library, the ASCEND adapter, and the CUTEst bridge.

The C API is intentionally close to IPOPT's `IpStdCInterface.h`. The goal is to
make external NLP drivers, especially CUTEst/SIFDecode drivers, cheap to wire
while also giving the ASCEND solver plugin the same core solver path.

Current runtime shape:

```text
ASCEND slv_system_t adapter
    -> IPOPT-like A4SQP C callbacks
        -> liba4sqp.so
            -> HiGHS QP backend

CUTEst/SIFDecode adapter
    -> IPOPT-like A4SQP C callbacks
        -> liba4sqp.so
            -> HiGHS QP backend
```

`liba4sqp.so` owns solver logic. Adapter libraries provide data and translate
frontend-specific status, options, and diagnostics.

## Algorithm Boundary Rule

All restoration, filtering, trust-region, merit-function, Hessian model,
line-search, step-type, multiplier recovery, convergence, and globalization
decisions belong inside `liba4sqp.so`.

Adapters must not decide that a solve is in regular mode versus restoration
mode, must not trigger restoration steps, must not implement filter acceptance,
and must not own BFGS or Hessian PSD logic. The same model sent through ASCEND
or through SIFDecode/CUTEst should follow the same A4SQP algorithmic path when
options, scaling, bounds, derivatives, and initial guesses are harmonized.

`liba4sqp.so` must not read environment variables to alter solver behaviour.
Environment variables are acceptable only in executable harnesses such as the
CUTEst package driver, where they are translated immediately into ordinary C
API options before `A4SqpSolve`.

For ASCEND, `slv_iterate` is a light wrapper around the solve-at-once optimizer.
It is not a second SQP control loop.

## Public Header

The authoritative public header is:

```text
solvers/a4sqp/a4sqp_c.h
```

The exported public C API symbols are:

- `CreateA4SqpProblem`
- `FreeA4SqpProblem`
- `AddA4SqpStrOption`
- `AddA4SqpNumOption`
- `AddA4SqpIntOption`
- `OpenA4SqpOutputFile`
- `SetA4SqpProblemScaling`
- `SetA4SqpIntermediateCallback`
- `A4SqpSolve`
- `GetA4SqpSolveStatistics`

`OpenA4SqpOutputFile` currently exists for IPOPT API shape but returns
`A4SQP_FALSE`; progress should currently be obtained through the intermediate
callback, ASCEND progress reporting, or the CUTEst runner logs.

The public scalar and opaque types are:

```c
typedef double A4SqpNumber;
typedef int A4SqpIndex;
typedef int A4SqpInt;
typedef int A4SqpBool;
typedef void *A4SqpUserDataPtr;

struct A4SqpProblemInfo;
typedef struct A4SqpProblemInfo *A4SqpProblem;
```

## Callback Shape

The frontend-facing callback surface is IPOPT-like:

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

The optional intermediate callback mirrors IPOPT's argument order:

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

`alg_mod` is `A4SqpRegularMode` or `A4SqpRestorationPhaseMode`.

There are no frontend callbacks for "request restoration", "shrink trust
region", "reset BFGS", or "accept step". Those are core solver decisions.

## Least-Squares Path

A4SQP now has an experimental core least-squares loop in `liba4sqp.so`
(`solvers/a4sqp/a4sqp_lsq.c`). It is not yet part of the IPOPT-like public C API.
The ASCEND integration is opt-in through the solver parameter:

```text
OPTION try_lsq 'OFF';    disable the LSQ path
OPTION try_lsq 'GAUSS';  Gauss-Newton
OPTION try_lsq 'LM';     Levenberg-Marquardt damping, default
OPTION lsq_max_iter 0;    reuse max_iter for the LSQ pre-solve budget
```

When enabled, `liba4sqp_ascend.so` asks the ASCEND system layer to classify and
build a least-squares residual view from the objective expression. The dedicated
path is used only for recognised unconstrained sum-of-squares objectives; other
models fall back to the ordinary SQP C API path.

The adapter does not implement the LSQ algorithm. It only recognises ASCEND
expression structure, evaluates residual expressions and residual-Jacobian rows
at trial `x` values, and maps ASCEND variables to A4SQP vector columns. Step
acceptance, damping, bound projection, convergence tests, and fallback status
are core-owned.

The CUTEst profiling driver has a separate least-squares recogniser for SIF
objective groups and exposes LSQ experiment switches:
`--lsq-max-iter N` and `--lsq-fallback-start original|improved`. These are
adapter-level controls around the experimental LSQ pre-solve handoff, not new
callbacks into the core SQP step machinery. They exist to make cases such as
BROWNDEN reproducible without relying on stale builds or implicit adapter
side-effects.

## Lifecycle

Problem construction:

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
```

Solve call:

```c
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
```

`A4SqpSolve` updates `x` in place with the final primal iterate. If non-NULL,
`g`, `obj_val`, `mult_g`, `mult_x_L`, and `mult_x_U` receive final values when
available.

## IPOPT Compatibility Semantics

The following semantics are intended to match IPOPT where practical:

- `CreateA4SqpProblem` copies variable and constraint bounds internally.
- `index_style == 0` means C-style zero-based sparse indices.
- `index_style == 1` means Fortran-style one-based sparse indices.
- `eval_jac_g(..., values == NULL)` requests Jacobian sparsity structure.
- `eval_jac_g(..., values != NULL)` requests Jacobian values.
- `eval_h(..., values == NULL)` requests lower-triangle Hessian sparsity.
- `eval_h(..., values != NULL)` requests Hessian values for `x`,
  `obj_factor`, and `lambda`.
- `eval_h == NULL` or `nele_hess == 0` means use the selected approximation
  mode, normally BFGS.
- `eval_f == NULL` is allowed only for feasibility-only problems; the
  objective is treated as zero.
- `m == 0` objective-only problems are valid.
- `A4SqpSolve` projects an out-of-box starting point into finite variable
  bounds using `bound_push` before the first callback evaluation.
- Callback failure returns `A4SQP_FALSE` and maps to an A4SQP failure status.
- Warm-start input multipliers are not yet a supported input path, but output
  multipliers are written when available.

The numeric return-code values intentionally mirror IPOPT's
`ApplicationReturnStatus` where meanings overlap.

## Solve Statistics

`GetA4SqpSolveStatistics` fills `struct A4SqpSolveStats`. The current fields
include:

- iterations;
- QP solves and QP failures;
- line-search failures;
- final objective and max constraint violation;
- projected-gradient proxy;
- KKT error, dual infeasibility, complementarity, and selected row-dual sign;
- algorithm mode and mode-switch count;
- regular/restoration iteration counts;
- restoration entry, exit, and handoff counts;
- final step norm and trust radius;
- final elastic activity;
- Hessian regularization size.

These statistics are the preferred way for adapters and benchmark harnesses to
report phase and restoration behaviour.

## Implemented Options

String options:

- `hessian`: `AUTO`, `BFGS`, `EXACT_OBJ`, or `EXACT_LAGRANGIAN`; default
  `BFGS`.
- `hessian_approximation`: IPOPT alias; `limited-memory` maps to `BFGS`, and
  `exact` maps to `EXACT_LAGRANGIAN` when an `eval_h` callback exists.
- `scaleopt`: `NONE`, `ROW_2NORM`, or `RELNOM`. The C API default is `NONE`;
  the CUTEst driver currently defaults this option to `ROW_2NORM` to match the
  ASCEND solver default unless overridden. `ROW_2NORM` caps the scale at `1.0`,
  so it scales down large rows but does not amplify rows whose Jacobian norm is
  small near a degenerate solution.

Numeric options:

- `tol`: sets `feas_tol` and `step_tol`.
- `constr_viol_tol` or `feas_tol`: feasibility tolerance.
- `step_tol`: step tolerance.
- `acceptable_tol`: relaxed acceptable-convergence tolerance.
- `merit_tol`: merit decrease tolerance.
- `armijo_coeff`: line-search Armijo coefficient.
- `elastic_penalty`: linear elastic slack penalty.
- `elastic_penalty_growth`: multiplier for core automatic penalty increases;
  default `10.0` in the C API.
- `elastic_penalty_max`: cap for automatic penalty increases.
- `filter_margin`: required fractional violation reduction for experimental
  filter-lite acceptance.
- `restoration_improve`: fractional max-violation improvement needed to reset
  restoration stall tracking.
- `restoration_margin`: restoration line-search acceptance margin.
- `restoration_handoff_reduction`: violation reduction from restoration entry
  that triggers handoff back to regular SQP.
- `restoration_reentry_factor`: hysteresis factor, default `2`, before
  restoration re-entry is allowed after repeated restoration handoffs.
- `trust_radius_init`, `trust_radius_min`, `trust_radius_max`: trust-radius
  bounds.
- `trust_shrink`, `trust_grow`, `trust_accept`, `trust_good`: trust policy
  parameters.
- `trust_tiny_alpha`, `trust_tiny_radius_factor`: optional trust shrink after
  tiny accepted regular steps.
- `hess_reg`: minimum Hessian regularization margin.
- `bound_push`: distance used for initial projection inside finite bounds.
- `qp_time_limit`: optional per-QP HiGHS time limit in seconds; zero disables
  it.
- `nlp_lower_bound_inf`, `nlp_upper_bound_inf`: IPOPT-like infinity
  thresholds.

Integer options:

- `max_iter`: maximum major SQP iterations; C API default `200`.
- `max_backtrack`: maximum backtracking trials.
- `acceptable_iter`: consecutive relaxed-convergence checks required before
  returning `A4SqpSolvedToAcceptableLevel`; default `0`, disabled.
- `trust_qp_retries`: trust-radius QP rebuild retries.
- `filter_accept`: enables experimental constrained filter-lite acceptance.
- `trust_unconstrained`: applies trust-region bounds to objective-only QPs.
- `kkt_convergence`: requires KKT residual convergence for objective problems.
- `restoration`: enables core-owned feasibility restoration.
- `restoration_trigger_iter`: consecutive materially infeasible
  non-improving iterations before restoration. The default conservative
  trigger is `3`; non-positive values are treated as that default. Core
  restoration also requires the maximum constraint violation to be materially
  above the feasibility tolerance, so near-feasible stationarity work stays on
  the regular SQP path. For unbounded constrained problems the core delays
  automatic restoration entry to avoid pre-empting regular SQP stationarity
  progress.
- `restoration_max_iter`: optional consecutive restoration-iteration cap;
  `0` disables it.
- `qp_iteration_limit`: optional per-QP HiGHS iteration limit; `0` disables it.
- `print_level` or `verbosity`: diagnostic verbosity.

Unknown options return `A4SQP_FALSE`. Benchmark drivers should log unknown
options rather than silently ignoring them.

`max_cpu_time` is not currently implemented as a C API option. CUTEst runs use
the Python runner timeout and optional per-QP HiGHS limits instead.

## Core/Adapter Split

Build-level split:

```text
liba4sqp.so
    links to: libhighs, libm, libc
    owns: public C API, solve driver, SQP core, QP builder, Hessian model,
          PSD regularization, trust policy, scaling helpers, numeric views

liba4sqp_ascend.so
    public symbols: a4sqp_register
    links to: liba4sqp.so, libascend.so, libc
    owns: slv_system_t adapter, ASCEND callbacks, status mapping, diagnostics
```

`liba4sqp_ascend.so` does not link directly to HiGHS. `liba4sqp.so` does not
link to ASCEND.

The ASCEND adapter now calls the same public C API used by CUTEst. Its callback
implementation:

- pushes candidate `x` values into ASCEND variables;
- rebuilds the ASCEND-backed numeric view;
- returns objective values, objective gradients, relation residuals, and
  Jacobian values to the C API;
- supplies Hessian-of-Lagrangian values by evaluating ASCEND relation/objective
  second derivatives and passing entries to core Hessian packing helpers;
- maps `A4SqpSolve` status and `A4SqpSolveStats` back to `slv_status_t` and
  progress output.

The CUTEst adapter under `solvers/a4sqp/cutest` implements the same callback
surface using CUTEst/SIFDecode routines and records benchmark JSONL.

## Internal Helper Exports

The stable external ABI is the C API in `a4sqp_c.h`. The library also exports
some helper symbols today, including core convergence/multiplier helpers,
dense-Hessian helpers, Hessian lower-triangle packing helpers, trust helpers,
QP helpers, and view helpers. These exist because the ASCEND adapter and tests
still use selected internal functions.

This is acceptable for the current incremental split, but it should not be
treated as a stable external ABI. Future cleanup should narrow or hide these
helpers where possible without reintroducing solver logic into adapters.

## ASCEND-Specific Notes

The ASCEND adapter still has richer diagnostics than a plain external C
client. It can render ASCEND variable names, relation names, source indices,
solver status, and progress callback output.

Current ASCEND defaults differ from the bare C API in some places. Notably,
`asc_a4sqp_params.c` defines `scaleopt=ROW_2NORM`, while the C API object
default remains `scaleopt=NONE` for IPOPT-like external callers. The CUTEst
driver sets `ROW_2NORM` explicitly by default so benchmark runs use the same
row-scaling mode as ASCEND unless overridden. This row scaling is capped to
avoid amplifying nearly singular active rows.

The ASCEND default for `elastic_penalty_growth` is `1.0` to avoid changing
legacy model behaviour by default. The C API and CUTEst path default to `10.0`
unless overridden.

## CUTEst/SIFDecode Bridge

For the NLP subset, use the C-oriented CUTEst API from `cutest_c.h`. The
implemented drivers are under:

```text
solvers/a4sqp/cutest
```

Relevant files:

- `a4sqp_main.c`: CUTEst package driver for A4SQP C API.
- `ipoptc_main.c`: comparison driver for IPOPT's installed C API.
- `run_a4sqp_cutest.py`: parallel runner and JSONL summarizer.
- `make_cutest_problem_list.py`: problem-list helper using `CLASSF.DB`.
- `install_cutest_package.sh`: package registration helper.

The runner supports `--solver a4sqp`, `--solver ipoptc`, and `--solver both`.
It uses process-level parallelism; A4SQP itself is single-threaded.

The CUTEst A4SQP package driver reads environment variables only as harness
plumbing, then passes normal options to the C API. The Python runner exposes
the same settings as command-line flags.

Common A4SQP CUTEst controls include:

- `--a4sqp-hessian` / `A4SQP_HESSIAN`
- `--acceptable-iter` / `A4SQP_ACCEPTABLE_ITER`
- `--acceptable-tol` / `A4SQP_ACCEPTABLE_TOL`
- `--kkt-convergence`, `--no-kkt-convergence` / `A4SQP_KKT_CONVERGENCE`
- `--filter-accept` / `A4SQP_FILTER_ACCEPT`
- `--trust-unconstrained` / `A4SQP_TRUST_UNCONSTRAINED`
- `--restoration` / `A4SQP_RESTORATION`
- `--restoration-trigger-iter` / `A4SQP_RESTORATION_TRIGGER_ITER`
- `--restoration-max-iter` / `A4SQP_RESTORATION_MAX_ITER`
- `--restoration-improve` / `A4SQP_RESTORATION_IMPROVE`
- `--restoration-margin` / `A4SQP_RESTORATION_MARGIN`
- `--restoration-handoff-reduction` /
  `A4SQP_RESTORATION_HANDOFF_REDUCTION`
- `--restoration-reentry-factor` / `A4SQP_RESTORATION_REENTRY_FACTOR`
- `--elastic-penalty-growth` / `A4SQP_ELASTIC_PENALTY_GROWTH`
- `--a4sqp-bound-push` / `A4SQP_BOUND_PUSH`
- `--a4sqp-qp-time-limit` / `A4SQP_QP_TIME_LIMIT`

SLSQP is intentionally deferred. The current comparison target is A4SQP versus
IPOPT over CUTEst NLP problems.

## Benchmark Records

CUTEst JSONL records should include:

- problem name and CUTEst classification;
- `n`, `m`, equality and inequality counts;
- finite variable-bound counts;
- Jacobian and Hessian nonzero counts;
- solver name and option profile;
- return status and outcome class;
- final objective, constraint violation, and stationarity/KKT diagnostics;
- major iterations;
- callback counts;
- setup and solve time;
- A4SQP QP failures, line-search failures, trust radius, elastic activity,
  restoration counters, and Hessian regularization size.

The runner records `outcome_class` to separate strict successes, acceptable
successes, high-KKT successes, near-solved max-iteration exits, stationarity
failures, infeasible/stalled exits, line-search errors, QP failures, and
driver-level errors.

## Current Test Status

Use the normal runner paths:

```text
./a4 cutest solver_a4sqp
./a4 run models/test/a4sqp/hs3.a4c
```

Current focused CUnit status:

```text
21 selected tests: 18 passed, 3 skipped, 0 failed
```

The skipped CUnit cases are `rosenmmx`, `lubrifc`, and `cont6_qq`. They remain
tracked as explicit skips because they previously passed but are not currently
reliable enough for default regression gating.

Recent CUTEst profiling on the fixed-size smooth NLP sample (`n <= 20`,
`m <= 20`, BFGS, 200 iterations, KKT convergence enabled) should be treated as
a snapshot. The last recorded profile had 4 strict successes and 11 clean
successes with `--acceptable-iter 5`.

## Current Open Items

- Continue reviewing scaling parity between ASCEND, CUTEst, and direct C API
  callers. The C API now supports `NONE`, capped `ROW_2NORM`, and `RELNOM`, but
  default variable nominal choices can still differ between front ends.
- Add a real fixed-variable presolve/postsolve reducer rather than only
  recording fixed flags in `A4SqpView`.
- Continue active-bound and restoration improvements inside `liba4sqp.so`, not
  in the adapters; BT13 is now a regression case for that path.
- Implement useful output-file support only if needed; for now the progress
  callback and runner logs are the supported reporting paths.

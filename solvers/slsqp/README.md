# SLSQP ASCEND Integration Plan

This directory will contain the ASCEND adapter and optional CUTEst driver for
SLSQP. The immediate goal is not to build a new solver core: it is to expose an
existing dense SLSQP implementation to ASCEND with enough diagnostics and option
control to compare it against IPOPT and A4SQP on the same small/medium NLP test
sets.

SLSQP is a dense sequential quadratic programming method. It is not expected to
scale like sparse IPOPT or the intended A4SQP design, but it is likely to be
competitive on small CUTEst and ASCEND models. It should therefore be useful as
a benchmark, a tuning reference for A4SQP, and a practical fallback solver for
moderately sized smooth constrained optimization problems.

## Design Principles

- Keep SLSQP as an external solver dependency. Do not fork solver logic into the
  ASCEND adapter unless a thin ABI wrapper is unavoidable.
- Keep `solvers/slsqp` adapter code separate from A4SQP. Shared helper ideas can
  be copied only after they are clearly generic and small.
- Use the ordinary ASCEND solver pattern: `./a4 run model.a4c` must be the
  primary smoke-test route, with `./a4 cutest solver_slsqp` for CUnit tests.
- Treat `slv_iterate` as a solve wrapper if the underlying SLSQP implementation
  does not expose meaningful single-iteration control.
- Preserve ASCEND-side model semantics. The adapter should map ASCEND variables,
  bounds, relations, objective, gradients, and Jacobians into the solver API; it
  should not implement SQP globalization, active-set logic, Hessian updates, or
  line search logic.
- Prefer derivative callbacks from ASCEND/CUTEst. Finite differences should be
  exposed only as an explicit option or implementation fallback.

## Solver Backend Choice

Start with an NLopt-backed implementation:

- `libnlopt` is installed locally and provides `NLOPT_LD_SLSQP`.
- It has a stable C API, supports nonlinear equality and inequality
  constraints, supports variable bounds, and accepts objective/constraint
  gradients.
- It should be faster to integrate than wiring directly to the historical
  Fortran routine.

Known limitations of the NLopt path:

- NLopt may not expose the full internal SLSQP option set, iteration state, QP
  diagnostics, or multipliers.
- Return statuses and stopping criteria need to be mapped carefully to ASCEND
  solver statuses.
- Direct comparison with original Kraft/SciPy/pyOptSparse SLSQP may differ in
  small details.

Optional second backend:

- Add a direct Kraft/SciPy-style SLSQP wrapper only if NLopt hides information we
  need for solver analysis or produces materially different behavior.
- If added, keep it behind the same ASCEND-facing adapter interface so tests can
  switch between `backend = 'NLOPT'` and `backend = 'KRAFT'`.

## Build Integration

1. Add `solvers/slsqp/SConscript`.
2. Add a `CheckNLOPT` configure test using `pkg-config --exists nlopt` and a
   tiny compile/link probe against `<nlopt.h>`.
3. Build `libslsqp_ascend.so` when `WITH_SLSQP` is enabled and NLopt is found.
4. Add `WITH_SLSQP` into the solver option flow in the same style as other
   optional solvers.
5. Add the library to `env['extfns']` so the `./a4` runner discovers it through
   the normal solver-loading path.
6. Do not add rpath. Rely on the existing `./a4` runner and `LD_LIBRARY_PATH`
   conventions.

Expected initial files:

- `solvers/slsqp/SConscript`
- `solvers/slsqp/asc_slsqp.c`
- `solvers/slsqp/asc_slsqp.h`
- `solvers/slsqp/asc_slsqp_internal.h`
- `solvers/slsqp/asc_slsqp_params.c`
- `solvers/slsqp/asc_slsqp_params.h`
- `solvers/slsqp/asc_slsqp_report.c`
- `solvers/slsqp/asc_slsqp_report.h`

The first implementation can be smaller than A4SQP because there is no separate
SLSQP core library to maintain if NLopt is the backend.

## ASCEND Adapter Shape

The adapter should:

1. Build an optimization view from `slv_system_t`.
2. Exclude fixed variables from the solve vector, following the A4SQP presolve
   direction.
3. Map ASCEND variable bounds to NLopt lower/upper bounds.
4. Identify the objective relation and objective direction.
5. Map equality relations to NLopt equality constraints.
6. Map inequality relations to NLopt inequality constraints with the correct
   sign convention.
7. Evaluate objective value and gradient through ASCEND relation/objective
   evaluation routines.
8. Evaluate constraint residuals and dense gradients through ASCEND derivative
   routines.
9. Copy the final dense solve vector back into ASCEND variables.
10. Report final objective, maximum constraint violation, status, iteration
    counts when available, and evaluation counts.

Sparse ASCEND Jacobian information can be used to avoid unnecessary derivative
work, but the vector passed to SLSQP is dense. The adapter should therefore
prioritize correctness and simple mapping first, then optimize evaluation later.

## Solver Parameters

Expose the relevant NLopt/SLSQP controls through ASCEND `slv_parm` entries. The
parameter list should be defined in the SLSQP adapter because NLopt is the solver
implementation in this first phase. If a separate `libslsqp.so` core/wrapper is
introduced later, move backend-generic option metadata there.

Initial parameters:

| Name | Type | Default | Notes |
| --- | --- | --- | --- |
| `backend` | string choice | `NLOPT` | Future-proof choice: `NLOPT`; optionally `KRAFT` later. |
| `max_iter` | integer | `200` | NLopt maximum evaluations/iterations proxy; verify exact mapping. |
| `ftol_rel` | real | `1e-8` | Relative objective tolerance. |
| `ftol_abs` | real | `0.0` | Absolute objective tolerance; `0` disables. |
| `xtol_rel` | real | `1e-8` | Relative variable tolerance. |
| `xtol_abs` | real | `0.0` | Uniform absolute variable tolerance; later allow vector scaling if needed. |
| `constraint_tol` | real | `1e-8` | Equality/inequality feasibility tolerance passed to NLopt constraints. |
| `stopval` | real | disabled | Optional target objective stop. |
| `finite_diff` | bool | `FALSE` | Use only if analytic derivatives are unavailable. |
| `progress` | bool | `TRUE` | Emit callback-level progress via the shared reporter when the CLI/UI enables progress display. |
| `print_level` | integer | `0` | ASCEND-side verbosity; NLopt itself is mostly quiet. |
| `scaleopt` | string choice | `NONE` | Start with no scaling; consider reusing A4SQP-style view scaling later. |

Potential later parameters:

- `max_eval`, if NLopt's stop limit is better represented separately from
  iterations.
- `initial_step`, if we expose NLopt's initial step controls.
- `check_derivatives`, using finite-difference spot checks for objective and
  constraint gradients.
- `warm_start`, only if a backend exposes useful state reuse.

## Progress Reporting

NLopt does not expose an IPOPT-style intermediate callback for SLSQP. Its C API
calls user objective and constraint callbacks and later returns a final
`nlopt_result`; it also exposes `nlopt_get_numevals()` and `nlopt_force_stop()`,
but not per-major-iteration state, step length, active set, merit value, or
line-search diagnostics.

The first implementation should therefore provide conservative progress
reporting:

- emit `SOLVER_STATUS` before and after `nlopt_optimize`;
- count objective, gradient, constraint, and Jacobian callback evaluations in
  the ASCEND adapter;
- if `progress` is enabled, optionally emit throttled callback-level progress
  lines every N objective evaluations or wall-clock interval;
- support user interruption by calling `nlopt_force_stop()` from a callback when
  ASCEND requests stop;
- after return, report final objective, max residual, evaluation counts,
  `nlopt_get_numevals()`, and the mapped solver status.

Callback-level progress is not the same as SLSQP iteration progress. If we later
need true major-iteration reporting, that is a reason to add a direct
Kraft/SciPy-style backend or patch/wrap an SLSQP implementation that exposes an
iteration hook.

## Status Mapping

Map NLopt return codes into ASCEND solver status consistently:

- `NLOPT_SUCCESS`, `NLOPT_FTOL_REACHED`, `NLOPT_XTOL_REACHED`,
  `NLOPT_STOPVAL_REACHED`: solved or acceptable solved, depending on final
  feasibility checks.
- `NLOPT_MAXEVAL_REACHED`, `NLOPT_MAXTIME_REACHED`: iteration/time limit.
- `NLOPT_ROUNDOFF_LIMITED`: warning/failure unless final feasibility and
  stationarity checks are acceptable.
- `NLOPT_INVALID_ARGS`, `NLOPT_OUT_OF_MEMORY`, `NLOPT_FAILURE`,
  `NLOPT_FORCED_STOP`: error statuses.

The adapter should always compute ASCEND-side final residuals after return from
NLopt. Do not trust a solver success code alone.

## CUnit Tests

Add a new suite, probably `solver_slsqp`, with tests that use the same model-run
helper pattern as A4SQP:

1. Registration smoke test: solver loads and exposes parameters.
2. Basic unconstrained objective-only model.
3. Bound-constrained one-variable quadratic.
4. Equality-constrained small model.
5. Inequality-constrained small model.
6. Selected translated CUTEst models from `models/test/a4sqp`.

The first selected corpus should include already stable small models:

- `bt2.a4c`
- `bt10.a4c`
- `cb3.a4c`
- `hs21.a4c`
- `jannson3.a4c`
- `alsotame.a4c`
- `akiva.a4c`
- `avgasa.a4c`
- `avgasb.a4c`

Keep the currently skipped A4SQP stress cases out of the initial mandatory
suite until SLSQP behavior is known. They can be run manually with `./a4 run`.

## Manual Model Testing

Use this pattern during development:

```sh
scons -j6
./a4 run models/test/a4sqp/bt2.a4c --progress
./a4 run models/test/a4sqp/alsotame.a4c --progress
./a4 cutest solver_slsqp
```

Where existing models hard-code `SOLVER A4SQP`, either:

- add companion methods such as `use_slsqp`, or
- add a runner/test helper that can set the solver before invoking `self_test`.

Prefer the runner/helper approach if it lets us reuse models without adding
solver-specific boilerplate to every file.

## CUTEst Driver

Create a CUTEst SLSQP driver after ASCEND model runs are stable:

Expected files:

- `solvers/slsqp/cutest/README.md`
- `solvers/slsqp/cutest/slsqp_main.c`
- `solvers/slsqp/cutest/makemaster`
- `solvers/slsqp/cutest/package_default_slsqp`
- `solvers/slsqp/cutest/install_cutest_package.sh`
- `solvers/slsqp/cutest/run_slsqp_cutest.py`

The CUTEst driver should mirror the current A4SQP/IPOPTC runner conventions:

1. Use CUTEst objective, gradient, constraint, and Jacobian callbacks.
2. Convert CUTEst constraints to NLopt equality/inequality callbacks.
3. Use dense gradient arrays for SLSQP.
4. Emit one JSON row per solve.
5. Include dimensions, classification, objective, max violation, status, return
   code, callback counts, and timeout/error classification.
6. Support `--jobs N` using isolated CUTEst worker roots, not concurrent writes
   to the same CUTEst tree.
7. Support the same problem-set files already used by A4SQP:
   `broad_stratified_89.tsv` and `broad_stratified_160.tsv`.

The first CUTEst milestone is not a polished progress report. It is a TSV that
can be compared directly with A4SQP/IPOPTC results for the same problem set and
timeout.

## Benchmark Contract

Initial comparison profile:

- Problem set: `solvers/a4sqp/cutest/problem_sets/broad_stratified_89.tsv`
- Expanded set: `solvers/a4sqp/cutest/problem_sets/broad_stratified_160.tsv`
- Timeout: 30 seconds per problem
- Jobs: 6
- Max iterations/evaluations: 200 initially, then tune if NLopt semantics differ
- Feasibility tolerance: `1e-8`
- Objective/variable tolerances: `1e-8`

Expected command shape:

```sh
solvers/slsqp/cutest/run_slsqp_cutest.py \
  --problem-file solvers/a4sqp/cutest/problem_sets/broad_stratified_89.tsv \
  --timeout-sec 30 \
  --jobs 6 \
  --rebuild \
  --tsv-out /tmp/slsqp_cutest_89.tsv
```

Once stable, extend `solvers/a4sqp/cutest/generate_cutest_progress.py` or add a
shared progress generator so the comparison table can include IPOPT, A4SQP, and
SLSQP without duplicating report code.

## Online/External Check

A quick search did not reveal an obvious stock CUTEst SLSQP package equivalent
to IPOPT. NLopt documents `NLOPT_LD_SLSQP` as a Kraft-derived SLSQP
implementation, and pyOptSparse documents the traditional Kraft SLSQP option
set (`ACC`, `MAXIT`, `IPRINT`, etc.). The implementation should therefore start
with NLopt because it is present locally and easy to link, while keeping the
plan open for a direct Kraft backend if we need deeper diagnostics.

## Open Questions

- Does NLopt count `maxeval` as function evaluations, iterations, or a mixture
  for SLSQP? Verify empirically before claiming iteration-count parity.
- Does NLopt expose enough information to report useful progress from inside the
  solve? If not, progress reporting may be limited to callback evaluation counts
  and final status.
- Should SLSQP use ASCEND/A4SQP scaling machinery, or should the initial version
  solve in physical units only? Start with no scaling to reduce translation
  risk.
- Are all relation inequality signs represented consistently between ASCEND,
  NLopt, and CUTEst? Add focused sign-convention tests early.
- How should we handle objectives declared as maximization? Prefer converting to
  minimization by negating objective and gradient before passing to SLSQP.

## Implementation Milestones

1. Add build option and `libslsqp_ascend.so` registration skeleton. Done.
2. Expose solver parameters and verify `./a4` can list/load SLSQP. Done for
   the NLopt-backed adapter.
3. Implement objective-only and bound-constrained solves. Initial path done.
4. Add equality and inequality constraint callbacks. Initial path done.
5. Add dense gradient/Jacobian evaluation from ASCEND derivatives. Initial path
   done, with `relman_diff2_rev` preferred and `relman_diff2` fallback.
6. Add final residual/status checks and progress output. Initial path done.
7. Add CUnit `solver_slsqp` smoke tests. Pending.
8. Run selected `models/test/a4sqp/*.a4c` models manually with SLSQP. Started
   via temporary copies that import/select SLSQP.
9. Add CUTEst package driver and single-problem smoke test. Pending.
10. Add parallel CUTEst runner and TSV output. Pending.
11. Compare SLSQP against IPOPT and A4SQP on the 89-problem set. Pending.
12. Expand to the 160-problem set and update the progress-report workflow.
    Pending.

Current smoke status:

- `bt10`, `bt2`, `cb3`, `hs21`, `alsotame`, `avgasa`, and `avgasb` solve via
  temporary `IMPORT "slsqp"` / `SOLVER SLSQP` copies.
- `akiva` currently fails after NLopt drives the objective callback into an
  invalid evaluation region; this is a useful early case for bounds/scaling or
  safe-evaluation fallback work.
- Existing `models/test/a4sqp` files still import/select A4SQP. A permanent
  SLSQP regression suite should use a model-run helper or companion run methods
  rather than editing all translated models by hand.

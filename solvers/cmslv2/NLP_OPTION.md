# CMSlv NLP Optimizer Option

Date: 2026-05-18

## Goal

CMSlv currently assumes CONOPT for NLP optimization support. This makes CMSlv
hard to test on GitHub Actions and hard for users who do not have CONOPT.

The goal is to add IPOPT as an alternative NLP optimizer for CMSlv while keeping
the longstanding CONOPT path as the default and avoiding unnecessary churn in
the existing CONOPT implementation.

## Decision

CMSlv already exposes an `optsolvers` parameter. The clean user-facing design is
to expand that parameter from:

```text
CONOPT
```

to:

```text
CONOPT, IPOPT
```

The selected optimizer should be used consistently for both CMSlv optimization
roles:

1. The normal optimization solver token used when the ASCEND model itself is an
   optimization problem.
2. The internal boundary NLP solved by `optimize_at_boundary`.

Do not silently mix solvers, for example IPOPT for one role and CONOPT for the
other. If `optsolvers=IPOPT`, CMSlv should use IPOPT for both roles. If
`optsolvers=CONOPT`, CMSlv should use CONOPT for both roles.

The default remains `CONOPT` for backward compatibility. If CONOPT is not
available and the user has not changed the option, CMSlv should report a clear
error that the default selected optimizer is unavailable. If IPOPT is available,
the error should mention that `optsolvers=IPOPT` can be selected.

## Initial Code Notes

CMSlv build gating currently requires both `CMSLV` and `CONOPT`:

```text
solvers/cmslv/SConscript
```

This prevents CMSlv from being built in an IPOPT-only configuration.

The hard CONOPT dependency in the C code is mostly localized:

```text
solvers/cmslv/cmslv.c
```

Important areas:

- `#include <ascend/solver/conopt_dl.h>` is unconditional.
- `struct slv9_system_structure` stores `struct conopt_data con` under
  `ASC_WITH_CONOPT`.
- CONOPT boundary callbacks are grouped under `ASC_WITH_CONOPT`.
- `optimize_at_boundary` builds a synthetic boundary NLP, fills CONOPT fields,
  calls `slv_conopt_iterate`, then reads `sys->con.obj`.
- `slv9_get_default_parameters` defines `optsolvers` with only `CONOPT`.
- `get_solvers_tokens` looks up `OPTSOLVER_OPTION` and creates the selected
  optimizer token, but messages and parameter setup assume CONOPT.
- `cmslv_register` currently refuses to register unless CONOPT is already
  registered.

The key architectural point is that CMSlv's boundary NLP is synthetic. It is not
the same var/rel problem seen by the normal ASCEND IPOPT solver client. Therefore
the existing `solvers/ipopt/asc_ipopt.c` client cannot simply be selected for
the boundary case without adding a CMSlv-specific IPOPT adapter.

## Boundary NLP Shape

The existing CONOPT adapter exposes this problem:

- Decision variables: active boundary step variables `dx`, plus one alpha per
  subregion.
- Bounds:
  - `dx` bounds are derived from original variable bounds and clipped by the
    CMSlv `infinity` parameter.
  - alpha bounds are `[0, 1]`.
- Constraints:
  - linear equalities from `coeff_matrix`;
  - one equality enforcing `sum(alpha) = 1`.
- Objective:
  - minimize `sum(dx_i^2)`.
- Objective gradient:
  - `2 * dx_i` for step variables;
  - zero for alpha variables.
- Constraint Jacobian:
  - mostly constant, currently assembled in the CONOPT `readmatrix` callback.

This maps directly to IPOPT's C interface (`IpStdCInterface.h`). The boundary
NLP has linear constraints and a quadratic objective, so the exact Hessian is
just a diagonal over the synthetic `dx` variables.

## Implementation Plan

1. Build configuration

   Allow CMSlv to build whenever `CMSLV` is selected. Do not require CONOPT or
   IPOPT at CMSlv build time. This lets the solver register and then report a
   selected-optimizer error only when SOLVE proceeds, which is the behavior
   wanted for unavailable `optsolvers` entries.

   Implemented condition:

   ```python
   if 'CMSLV' in env['WITH_SOLVERS']:
   ```

   For IPOPT, reuse the existing `ensure_ipopt()` logic and
   `IPOPT_CPPPATH`, `IPOPT_LIBPATH`, and `IPOPT_LIBS` values, but only compile
   the IPOPT-specific adapter when IPOPT is available.

2. Compile-time guards

   Make the CONOPT include and CONOPT fields conditional:

   ```c
   #ifdef ASC_WITH_CONOPT
   # include <ascend/solver/conopt_dl.h>
   #endif
   ```

   Add IPOPT conditional support when available:

   ```c
   #ifdef ASC_WITH_IPOPT
   # include <IpStdCInterface.h>
   #endif
   ```

   Reuse `ASC_WITH_IPOPT_HSLIB` and `ASC_IPOPT_HSL_LIBRARY` if the IPOPT build
   configuration provides them.

3. Expand `optsolvers`

   Update `slv9_get_default_parameters` so `optimization_names` includes both
   `CONOPT` and `IPOPT`, even if one or both are unavailable in the current
   build/runtime environment.

   Desired default behavior:

   - Always default to `CONOPT` for backward compatibility.
   - Do not reject the default at parameter-list creation time, even in an
     IPOPT-only build.
   - Validate the selected optimizer only when solving actually proceeds.
   - If the selected optimizer is unavailable, report a clear user-facing error
     at solve/presolve time and mention the other option if it is available.

4. Registration

   Change `cmslv_register` so it always requires `LRSlv` and `QRSlv`, but does
   not always require CONOPT.

   CMSlv should be able to register in an IPOPT-only build. The selected
   optimizer should be validated during `slv9_create` / `get_solvers_tokens`,
   because that is where the user's `optsolvers` parameter is known.

5. Token setup

   Keep using `OPTSOLVER_OPTION` in `get_solvers_tokens`. Replace CONOPT-specific
   messages with selected-solver messages.

   The selected solver token should be created exactly once and stored in
   `token[OPTIMIZATION_SOLVER]`, as today.

   Parameter setup needs solver-specific handling:

   - For CONOPT, keep the existing `iterationlimit = 20` behavior.
   - For IPOPT, set equivalent conservative options if necessary, probably
     `max_iter` and `print_level`, but avoid forcing IPOPT options unless CMSlv
     really needs them.

6. Boundary optimizer dispatch

   Split the boundary solve call behind a small dispatch function:

   ```c
   static int32 slv9_bnd_iterate(
       slv9_system_t sys, int32 num_opt_vars, int32 num_opt_eqns,
       int32 num_vars, real64 *obj_val
   );
   ```

   It should inspect `OPTSOLVER_OPTION` and call one of:

   ```c
   slv9_bnd_iterate_conopt(sys, num_opt_vars, num_opt_eqns, num_vars, obj_val);
   slv9_bnd_iterate_ipopt(sys, num_opt_vars, num_opt_eqns, num_vars, obj_val);
   ```

   If the selected optimizer is not compiled/available, report a clear error and
   return failure.

7. IPOPT boundary adapter

   Implement a CMSlv-specific IPOPT adapter using the direct IPOPT C interface,
   following the same style already used in `models/johnpye/fprops/eqm_ipopt.c`.

   Put IPOPT-specific boundary handling in a new file:

   ```text
   solvers/cmslv/cmslv_ipopt.c
   ```

   Keep `cmslv.c` changes limited to shared declarations, dispatch, parameters,
   and call sites. Avoid growing `cmslv.c` with IPOPT-specific callback bodies.

   Required callbacks:

   - `eval_f`: objective `sum(dx_i^2)`.
   - `eval_grad_f`: `2 * dx_i` for step variables, zero for alpha variables.
   - `eval_g`: linear constraints and `sum(alpha)`.
   - `eval_jac_g`: sparse Jacobian structure and values.
   - `eval_h`: exact diagonal Hessian for the quadratic objective.

   The adapter should write the IPOPT solution into `sys->opt_var_values`, just
   as the CONOPT solution callback does.

   Start with a minimal likely-useful IPOPT option subset:

   - `max_iter`, mapped from CMSlv's optimizer iteration limit;
   - `tol`, probably mapped from or aligned with CMSlv's optimization tolerance;
   - `print_level = 0` by default;
   - `mu_strategy = adaptive`;
   - `fixed_variable_treatment = make_constraint`;
   - `linear_solver` only if a CMSlv parameter is added or an existing global
     IPOPT default is cleanly reusable.

   Add more IPOPT-specific boundary options only after test cases show they are
   needed.

8. Error behavior

   If `optsolvers=CONOPT` and CONOPT is unavailable:

   ```text
   CMSlv selected optimizer CONOPT is not available.
   ```

   If IPOPT is available:

   ```text
   IPOPT is available; select optsolvers=IPOPT to use it with CMSlv.
   ```

   Similarly, if `optsolvers=IPOPT` and IPOPT is unavailable, report that IPOPT
   is unavailable and mention CONOPT only if CONOPT is available.

9. Tests

   Add at least one CMSlv test path that can run with IPOPT on GHA.

   First test target should verify:

   - CMSlv registers without CONOPT when IPOPT is built.
   - A small conditional model can select `optsolvers=IPOPT`.
   - The solve completes or makes expected progress.

   Avoid requiring exact CONOPT/IPOPT trajectory equivalence in the first tests.
   The algorithms may select different but valid boundary steps.

## Current Implementation Notes

The first implementation now has these pieces in place:

- `solvers/cmslv/SConscript` builds CMSlv whenever `CMSLV` is selected.
- `solvers/cmslv/cmslv.h` holds the internal CMSlv system structure, parameter
  aliases, shared boundary data structures, and conditional CONOPT/IPOPT
  declarations.
- `cmslv.c` keeps the existing CONOPT callback bodies under
  `ASC_WITH_CONOPT`, with the boundary entry point renamed/adapted to
  `slv9_bnd_iterate_conopt(...)`.
- `cmslv_ipopt.c` implements `slv9_bnd_iterate_ipopt(...)` using IPOPT's C
  interface and an exact Hessian for the CMSlv boundary NLP.
- `optsolvers` lists `CONOPT` and `IPOPT` unconditionally, while defaulting to
  `CONOPT`.
- CMSlv maps the selected optimizer to `package_load("conopt")` or
  `package_load("ipopt")` during presolve if the solver engine has not already
  been registered. Models no longer need to hard-import the selected NLP
  backend.
- Selected optimizer validation is deferred until presolve, so unavailable
  options are reported only when a solve is attempted.
- `cmslv_register` no longer requires CONOPT; it still requires LRSlv and QRSlv.
- CMSlv progress parameters and structured progress events are wired into the
  main boundary and iteration milestones.
- `ascend/solver/test/test_cmslv.c` now exercises CMSlv with CONOPT and IPOPT
  on `models/test/cmslv/heatex.a4c` and asserts that boundary progress events
  are emitted.
- The Linux SCons defaults now prefer a free CONOPT install in `~/.local`
  (`libconopt.so`) when present, while retaining the older `consub3` fallback.

The proposed first test model remains:

```text
models/test/cmslv/heatex.a4c
```

## Initial Recommendation

Start with the smallest additive implementation:

- Leave the CONOPT boundary code intact.
- Add IPOPT callback code under `ASC_WITH_IPOPT`.
- Place IPOPT boundary code in `solvers/cmslv/cmslv_ipopt.c`.
- Expand `optsolvers`.
- Dispatch the boundary solve based on `OPTSOLVER_OPTION`.
- Relax build and registration dependencies so IPOPT-only CMSlv is possible.

This keeps the CONOPT behavior as the default and makes IPOPT available for GHA
testing and users without CONOPT.

## Progress Reporting Review

Date: 2026-05-18

While adding the NLP optimizer switch, we should also improve CMSlv progress
reporting for `./a4 run --progress`.

### Current Progress Mechanism

ASCEND now has a lightweight solver progress callback:

```c
int slv_report_progress(const char *solver_name, const char *message);
```

The callback is installed globally through:

```c
slv_set_progress_callback(...)
slv_clear_progress_callback()
```

The C++/Python layer connects this to `SolverReporter::reportProgress`. The
command-line runner prints messages as:

```text
SOLVER_PROGRESS: solver=HiGHS, ...
SOLVER_PROGRESS: solver=A4SQP, ...
```

The GTK solver reporter also parses comma-separated `key=value` tokens from the
message. This means progress messages should prefer stable, parseable text such
as:

```text
event=boundary_crossed, iter=5, block=2, factor=0.375
```

instead of prose-only messages.

### Existing Patterns

A4SQP and HiGHS are the relevant models.

A4SQP has:

- `progress_callbacks`, default `TRUE`;
- `progress_log`, default `FALSE`;
- a small wrapper that optionally writes an error-reporter note and optionally
  calls `slv_report_progress`.

HiGHS has the same two user-facing controls. It only enables expensive/native
callback streams when `progress_callbacks` is enabled, and it formats structured
progress fields such as iteration count, objective, runtime, node count, and
MIP gap.

SLSQP follows the same basic pattern.

CMSlv currently uses `ERROR_REPORTER_HERE(ASC_PROG_NOTE, ...)` for events such
as:

- solving the optimization problem at a boundary;
- boundary crossed, returning to first boundary;
- iterating with optimizer;
- iterating with nonlinear solver.

Those notes are not structured progress callbacks. As a result, `./a4 run
--progress` can show generic `SOLVER_STATUS` lines, but it cannot reliably show
CMSlv-specific state such as boundary traversals, subregions, boundary NLP
objective, or bisection return factor.

### Recommendation

Add CMSlv progress parameters mirroring A4SQP and HiGHS:

```text
progress_callbacks = TRUE
progress_log       = FALSE
```

Add a local helper:

```c
static void slv9_report_progress(slv9_system_t sys, const char *fmt, ...);
```

The helper should:

1. Format a compact key/value message.
2. If `progress_log` is true, emit an `ASC_PROG_NOTE` with a `(CMSlv progress)`
   prefix.
3. If `progress_callbacks` is true, call:

   ```c
   slv_report_progress("CMSlv", message);
   ```

This avoids scattering callback logic through `cmslv.c` and keeps behavior
consistent with A4SQP/HiGHS.

### Proposed CMSlv Progress Events

Use stable event names and comma-separated `key=value` fields. Suggested first
set:

```text
event=presolve, optimizing=0|1, vars=..., rels=...
event=logical_start, iter=...
event=logical_done, iter=..., converged=0|1
event=nl_start, iter=..., solver=QRSlv, block=...
event=nl_done, iter=..., solver=QRSlv, block=..., converged=0|1, residual=...
event=optimizer_start, iter=..., solver=CONOPT|IPOPT, block=...
event=optimizer_done, iter=..., solver=CONOPT|IPOPT, block=..., converged=0|1, obj=...
event=boundary_at_zero, iter=..., n_subregions=..., cur_subregion=...
event=boundary_opt_start, iter=..., solver=CONOPT|IPOPT, n_subregions=..., vars=..., eqns=...
event=boundary_opt_done, iter=..., solver=CONOPT|IPOPT, obj=...
event=boundary_crossed, iter=..., count=...
event=boundary_return_start, iter=..., count=...
event=boundary_return_done, iter=..., factor=..., bisect_iter=...
event=reconfigure, iter=...
event=stop_at_boundary, iter=...
```

The first implementation does not need every field. The most useful initial
fields are `event`, `iter`, `solver`, `n_subregions`, `obj`, `factor`, and
`converged`.

### Boundary Traversal Details

`return_to_first_boundary` already computes useful data:

- number of crossed boundaries (`numbndf`);
- bisection factor;
- number of bisection iterations;
- whether it hit the bisection iteration limit.

Currently it only returns `factor`. For better progress reporting, either:

1. Report from inside `return_to_first_boundary` before returning, or
2. Add optional output parameters for `numbndf` and `iter`.

The least invasive approach is to report from inside the function. A cleaner
longer-term approach is an output struct:

```c
struct slv9_boundary_return_info {
  int32 crossed_count;
  int32 bisect_iter;
  int32 iteration_limit_hit;
  real64 factor;
};
```

The function can fill this struct while still returning `factor` for existing
call sites.

### CONOPT/IPOPT Interaction

The CMSlv boundary optimizer should report progress at the CMSlv level even if
the selected NLP solver also emits its own progress.

For example:

```text
SOLVER_PROGRESS: solver=CMSlv, event=boundary_opt_start, solver=IPOPT, ...
SOLVER_PROGRESS: solver=IPOPT, iter=...
SOLVER_PROGRESS: solver=CMSlv, event=boundary_opt_done, solver=IPOPT, obj=...
```

This nesting is acceptable because the `solver=` prefix identifies the source.
The CMSlv messages explain the conditional-solver state; the IPOPT/CONOPT
messages explain the inner NLP solve.

CONOPT's current direct progress callback path uses `asc_conopt_progress`, which
does not call `slv_report_progress`. We can either:

- leave CONOPT native progress quiet and report only CMSlv boundary start/done;
  or
- add a CMSlv-specific CONOPT progress callback for boundary solves that reports
  selected CONOPT iteration/phase/objective fields through `slv_report_progress`.

For the first implementation, CMSlv boundary start/done/crossing messages are
enough.

### CLI Output Example

Target shape for `./a4 run --progress`:

```text
SOLVER_STATUS: solver_status=ready, iter=3
SOLVER_PROGRESS: solver=CMSlv, event=nl_start, iter=3, solver=QRSlv
SOLVER_PROGRESS: solver=CMSlv, event=boundary_crossed, iter=3, count=1
SOLVER_PROGRESS: solver=CMSlv, event=boundary_return_done, iter=3, factor=0.421875, bisect_iter=7
SOLVER_PROGRESS: solver=CMSlv, event=boundary_at_zero, iter=4, n_subregions=2, cur_subregion=1
SOLVER_PROGRESS: solver=CMSlv, event=boundary_opt_start, iter=4, solver=IPOPT, n_subregions=2, vars=5, eqns=4
SOLVER_PROGRESS: solver=CMSlv, event=boundary_opt_done, iter=4, solver=IPOPT, obj=1.2e-12
SOLVER_FINAL: solver_status=converged
```

### Progress Implementation Plan

1. Add `CMSLV_PARAM_PROGRESS_CALLBACKS` and `CMSLV_PARAM_PROGRESS_LOG` parameter
   indices. This requires increasing `slv9_PA_SIZE`.
2. Add `slv9_report_progress`.
3. Replace or supplement the existing CMSlv `ASC_PROG_NOTE` milestone messages
   with structured progress calls.
4. Add boundary return reporting in `return_to_first_boundary`.
5. Add boundary NLP reporting around the selected CONOPT/IPOPT dispatch.
6. Add a small test like the HiGHS progress callback test:
   - enable callback capture;
   - run a small CMSlv model that crosses or sits at a boundary;
   - assert captured text contains `solver=CMSlv` and at least one boundary
     event.

The progress work is independent of the IPOPT adapter, but it will make the
IPOPT/CONOPT comparison much easier to debug.

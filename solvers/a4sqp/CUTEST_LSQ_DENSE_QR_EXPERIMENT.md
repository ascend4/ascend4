# CUTEst LSQ Dense QR Experiment

Date: 2026-05-27

## Scope

This note records the follow-up LSQ experiment after switching recognised
CUTEst least-squares problems to the `DENSE_QR` LSQ linear solver by default.
The stable 160-problem pass/fail matrix remains in `CUTEST_PROGRESS_160.md`;
this file records implementation details and focused timing/convergence checks
that are intentionally too noisy for that generated matrix.

## Adapter Change

The CUTEst A4SQP adapter now has a batched dense LSQ Jacobian path for
`DENSE_QR`. For a recognised LSQ problem, the adapter can build all residual
Jacobian rows after one CUTEst `ELFUN_r(..., 2, ...)` derivative sweep at the
current point, cache that dense `nres x n` matrix, and then satisfy the existing
`a4sqp_lsq_solve` per-row callback from the cache.

The public `a4sqp_lsq_solve` API is unchanged. The batching is internal to the
CUTEst adapter. The `NORMAL` LSQ linear solver still uses the original sparse
per-row CUTEst callback, so users selecting `NORMAL` do not pay the dense
residual-Jacobian storage cost.

The CUTEst runner also now leaves `A4SQP_LSQ_MAX_ITER` unset when
`--lsq-max-iter 0` is requested. The C driver then chooses an adaptive default:
large residual systems (`nres >= 1000` or `nres * n >= 200000`) get a 20
iteration LSQ pre-solve cap unless the user explicitly sets
`A4SQP_LSQ_MAX_ITER`.

## Validation

Basic checks:

| Check | Result |
| --- | --- |
| `python3 -m py_compile` for CUTEst runner/update/report scripts | pass |
| `git diff --check` | pass |
| `BARD`, `DENSE_QR`, default adaptive LSQ | strict success, 5 LSQ iterations |
| `BARD`, `NORMAL`, default adaptive LSQ | strict success, 5 LSQ iterations |

Focused DIAMON/DMN run:

Command profile:

```text
run_a4sqp_cutest.py DIAMON2DLS DIAMON3DLS DMN15102LS DMN15103LS DMN15332LS DMN15333LS DMN37142LS DMN37143LS
  --solver a4sqp
  --try-lsq LM
  --lsq-linear-solver DENSE_QR
  --lsq-max-iter 0
  --max-iter 200
  --timeout-sec 30
  --kkt-convergence
  --acceptable-iter 5
  --reduced-gradient-polish-mode FALLBACK
```

Results:

| Problem | n | LSQ residuals | LSQ iters | Final objective | KKT error | Outcome |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| DIAMON2DLS | 66 | 4643 | 20 | 392.464481 | 12.270747 | max_iter_stationarity |
| DIAMON3DLS | 99 | 4643 | 20 | 228.591422 | 604.039491 | max_iter_stationarity |
| DMN15102LS | 66 | 4643 | 20 | 392.464481 | 12.270747 | max_iter_stationarity |
| DMN15103LS | 99 | 4643 | 20 | 228.591422 | 604.039491 | max_iter_stationarity |
| DMN15332LS | 66 | 4643 | 20 | 195.923596 | 89.348015 | max_iter_stationarity |
| DMN15333LS | 99 |  |  |  |  | driver_timeout |
| DMN37142LS | 66 | 4643 | 20 | 158.578642 | 56.485394 | max_iter_stationarity |
| DMN37143LS | 99 |  |  |  |  | driver_timeout |

## Interpretation

The batched Jacobian path removes a major repeated CUTEst derivative-evaluation
cost from the `DENSE_QR` LSQ path. Together with the 20-iteration adaptive cap,
it changes several DIAMON/DMN cases from long LSQ pre-solve runs into normal
SQP handoffs that emit JSON inside the 30 second benchmark timeout.

It does not materially improve convergence on the DIAMON/DMN family. The best
new behavior is a cleaner and faster failure mode: capped LSQ handoff followed
by `max_iter_stationarity`, not successful convergence. Further pass-rate
improvement for these cases likely needs SQP globalization/stationarity work
after the LSQ handoff, not another LSQ linear-solver change.

## Full 160-Problem Refresh

After the focused checks, the full 160-problem CUTEst report was regenerated at
`solvers/a4sqp/CUTEST_PROGRESS_160.md` using fresh A4SQP runs and the latest
available IPOPT, SLSQP, and CONOPT comparison TSVs.

| Profile | Previous pass | Refreshed pass | Previous timeouts | Refreshed timeouts |
| --- | ---: | ---: | ---: | ---: |
| A4SQP_BFGS_AUTO | 96/160 | 97/160 | 30 | 25 |
| A4SQP_EXACT_OBJ_AUTO | 107/160 | 107/160 | 29 | 27 |
| A4SQP_EXACT_LAGRANGIAN_AUTO | 115/160 | 115/160 | 26 | 24 |

The full refresh confirms the focused result: the batched/adaptive LSQ changes
reduce benchmark timeouts, but they do not materially move the best pass rate.
The best A4SQP profile remains `EXACT_LAGRANGIAN` at 115/160.

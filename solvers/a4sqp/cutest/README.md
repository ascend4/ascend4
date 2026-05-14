# A4SQP/IPOPTC CUTEst Drivers

This directory contains CUTEst package drivers for the A4SQP IPOPT-like C API
and for IPOPT's installed `IpStdCInterface.h`. They are intentionally outside
normal ASCEND builds.

The local stock CUTEst `ipopt` package uses the older Fortran IPOPT interface
and segfaulted in smoke testing. The `ipoptc` package here uses the same C
callback shape as the A4SQP driver, which makes the benchmark wiring more
comparable.

## Install Package Hooks

```sh
solvers/a4sqp/cutest/install_cutest_package.sh
```

This creates:

- `$CUTEST/src/a4sqp/a4sqp_main.c` as a symlink to this driver.
- `$CUTEST/src/a4sqp/makemaster` as a symlink to this make hook.
- `$CUTEST/packages/defaults/a4sqp` as the runcutest package definition.
- `$CUTEST/src/ipoptc/ipoptc_main.c` as a symlink to the IPOPT C driver.
- `$CUTEST/src/ipoptc/makemaster` as a symlink to the IPOPT C make hook.
- `$CUTEST/packages/defaults/ipoptc` as the runcutest package definition.

## Run One Problem

```sh
export CUTEST=/home/john/CUTEst
export SIFDECODE=/home/john/sifdecode
export ARCHDEFS=/home/john/archdefs
export ASCEND_ROOT=/home/john/ascend
export LD_LIBRARY_PATH=/home/john/ascend/solvers/a4sqp:/home/john/ascend:/home/john/.local/lib:${LD_LIBRARY_PATH:-}

runcutest -p a4sqp -D HS11
runcutest -p ipoptc -D HS11
```

Each driver prints one JSON result line containing dimensions, classification,
status, objective, violation, and CUTEst callback counts. A4SQP records its SQP
and QP diagnostics; IPOPT fields that are not currently collected are emitted
as `null`.

## Run A Small Comparison

```sh
solvers/a4sqp/cutest/run_a4sqp_cutest.py \
  --solver both \
  --out /tmp/a4sqp_ipoptc.jsonl \
  --log-dir /tmp/a4sqp_ipoptc_logs \
  --rebuild \
  HS11 ROSENBR
```

To create a starter fixed-size smooth NLP list from the local MASTSIF
classification database:

```sh
solvers/a4sqp/cutest/make_cutest_problem_list.py \
  --fixed-size-only \
  --max-n 100 \
  --max-m 100 \
  --limit 50 \
  --out /tmp/cutest_nlp_small.txt

solvers/a4sqp/cutest/run_a4sqp_cutest.py \
  --solver both \
  --problem-file /tmp/cutest_nlp_small.txt \
  --out /tmp/a4sqp_ipoptc_small.jsonl \
  --tsv-out /tmp/a4sqp_ipoptc_small.tsv \
  --log-dir /tmp/a4sqp_ipoptc_small_logs \
  --timeout-sec 30
```

Useful environment/option overrides:

- `--max-iter N` or `A4SQP_MAX_ITER=N`
- `--tol VALUE` or `A4SQP_TOL=VALUE`
- `--acceptable-iter N` or `A4SQP_ACCEPTABLE_ITER=N`
- `--acceptable-tol VALUE` or `A4SQP_ACCEPTABLE_TOL=VALUE`
- `--kkt-convergence`, `--no-kkt-convergence`, or
  `A4SQP_KKT_CONVERGENCE=0|1`
- `--filter-accept` or `A4SQP_FILTER_ACCEPT=1`
- `--filter-margin VALUE` or `A4SQP_FILTER_MARGIN=VALUE`
- `--trust-unconstrained` or `A4SQP_TRUST_UNCONSTRAINED=1`
- `--restoration` or `A4SQP_RESTORATION=1`
- `--restoration-trigger-iter N` or `A4SQP_RESTORATION_TRIGGER_ITER=N`
- `--restoration-improve VALUE` or `A4SQP_RESTORATION_IMPROVE=VALUE`
- `--restoration-margin VALUE` or `A4SQP_RESTORATION_MARGIN=VALUE`
- `--restoration-handoff-reduction VALUE` or `A4SQP_RESTORATION_HANDOFF_REDUCTION=VALUE`
- `--restoration-reentry-factor VALUE` or `A4SQP_RESTORATION_REENTRY_FACTOR=VALUE`
- `--elastic-penalty VALUE` or `A4SQP_ELASTIC_PENALTY=VALUE`
- `--a4sqp-hessian BFGS|EXACT_OBJ|EXACT_LAGRANGIAN` or `A4SQP_HESSIAN=...`
- `--a4sqp-hess-reg VALUE` or `A4SQP_HESS_REG=VALUE`
- `--ipopt-max-iter N` or `IPOPTC_MAX_ITER=N`
- `--ipopt-tol VALUE` or `IPOPTC_TOL=VALUE`
- `--ipopt-hessian limited-memory|exact` or `IPOPTC_HESSIAN=...`
- `--solver a4sqp|ipoptc|both`
- `--jobs N` runs independent problem/package jobs concurrently. For `N > 1`
  the runner creates one private CUTEst tree per worker under `--workdir` so
  concurrent `runcutest` invocations do not mutate the same
  `objects/.../libcutest.a` archive.
- `--timeout-sec SECONDS` records a timeout JSON entry and continues with the
  next solver/problem pair.

Do not run multiple `runcutest` processes against the same writable CUTEst tree.
CUTEst rebuilds package/tool archives as part of normal `runcutest` execution,
and concurrent invocations can corrupt the shared archive. Use this runner's
`--jobs` option instead; it isolates each worker's CUTEst object tree while
still keeping each solver process single-threaded.

The CUTEst driver can use the standalone C API's BFGS path or CUTEst exact
Hessians. Exact Hessians are regularized to PSD before the QP model is built.

Acceptable convergence is disabled by default. Use `--acceptable-iter 5` for an
IPOPT-like relaxed profile that reports `A4SqpSolvedToAcceptableLevel` on
near-solved cases without changing ASCEND's default behaviour.

KKT-residual convergence is enabled by default in this runner, although the
standalone C API default remains off for compatibility. Use
`--no-kkt-convergence` to reproduce legacy small-step convergence behavior.

On the current 20-problem fixed-size smoke sample, default KKT-residual
convergence gives 4/20 clean strict successes. Adding `--acceptable-iter 5`
raises that to 11/20 clean strict-or-acceptable successes without accepting the
legacy high-stationarity cases.

Each JSON result includes `outcome_class` for failure taxonomy. The
`--filter-accept` and `--trust-unconstrained` profiles are experimental
globalization probes; early 20-problem smoke testing did not improve pass count,
so they are not recommended as defaults.

A4SQP records KKT diagnostics in each JSON row: `kkt_error`,
`dual_infeasibility_inf`, `complementarity_inf`, and `kkt_lambda_sign`. The
taxonomy flags successful solver statuses with high KKT residual as
`strict_success_high_kkt` or `acceptable_success_high_kkt`; with default
KKT-residual convergence these should normally be reported as stationarity
failures instead of successes.

## Track Progress

The checked-in progress ledger is `../CUTEST_PROGRESS.md`. Generate it from a
per-problem TSV result file rather than editing it by hand:

```sh
solvers/a4sqp/cutest/generate_cutest_progress.py \
  --input /tmp/a4sqp_broad_41_valid_20260513_182750/broad_41_problem_results.tsv \
  --out solvers/a4sqp/CUTEST_PROGRESS.md \
  --suite "CUTEst broad stratified 41-problem tracking subset" \
  --problem-set solvers/a4sqp/cutest/problem_sets/broad_stratified_41.tsv \
  --max-iter 200 \
  --timeout-sec 30 \
  --tol 1e-7 \
  --acceptable-tol 1e-5 \
  --jobs 6
```

When rerunning profiles, pass `--tsv-out` and `--profile-name` to
`run_a4sqp_cutest.py`, then pass one or more generated TSV files to
`generate_cutest_progress.py`.

The progress report intentionally excludes volatile metrics such as iteration
counts, solve times, objective values, and log paths. It records the benchmark
contract, profile summary, and compact per-problem light+number status codes so
`git diff` shows solver progress rather than run-to-run noise.

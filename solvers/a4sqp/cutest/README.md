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
  --log-dir /tmp/a4sqp_ipoptc_small_logs \
  --timeout-sec 30
```

Useful environment/option overrides:

- `--max-iter N` or `A4SQP_MAX_ITER=N`
- `--tol VALUE` or `A4SQP_TOL=VALUE`
- `--elastic-penalty VALUE` or `A4SQP_ELASTIC_PENALTY=VALUE`
- `--a4sqp-hessian BFGS|EXACT_OBJ|EXACT_LAGRANGIAN` or `A4SQP_HESSIAN=...`
- `--a4sqp-hess-reg VALUE` or `A4SQP_HESS_REG=VALUE`
- `--ipopt-max-iter N` or `IPOPTC_MAX_ITER=N`
- `--ipopt-tol VALUE` or `IPOPTC_TOL=VALUE`
- `--ipopt-hessian limited-memory|exact` or `IPOPTC_HESSIAN=...`
- `--solver a4sqp|ipoptc|both`
- `--timeout-sec SECONDS` records a timeout JSON entry and continues with the
  next solver/problem pair.

The CUTEst driver can use the standalone C API's BFGS path or CUTEst exact
Hessians. Exact Hessians are regularized to PSD before the QP model is built.

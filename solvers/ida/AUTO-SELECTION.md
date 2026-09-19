# IDA automatic linear solver selection: investigation

Investigation on 19 September 2026, following `fbfffba1`. This is a proposal;
the runtime still defaults to DENSE and accepts explicit KLU. See
[SPARSE-JACOBIAN.md](SPARSE-JACOBIAN.md) for implementation and prior tests.

## Recommendation

Prefer KLU for moderately sized sparse systems, but retain DENSE and explicit
solver overrides. The earlier tentative threshold of 1000 unknowns and 1%
density is unnecessarily restrictive on this machine. Conversely, these
measurements do not justify using KLU unconditionally for every model.

A conservative first AUTO policy would be:

1. Use DENSE if KLU was not compiled in, or `autodiff=false`.
2. Use DENSE for fewer than 64 IDA unknowns.
3. Otherwise use KLU if structural Jacobian density is at most 10%; use DENSE
   above that density.

The 64 and 10% thresholds are proposed policy constants, **not measured
universal crossover points**. The size threshold avoids bothering with tiny
absolute savings. The density limit is a conservative storage tradeoff, not
evidence that KLU becomes slower above 10%. KLU was competitive even for the
fully dense cases tested. Keep explicit KLU available above the limit.
Do not add user-facing tuning parameters until experience calls for them.

Initially offer AUTO as an opt-in choice. Making AUTO the default can be a
separate change after regression and platform qualification. Keeping explicit
DENSE is useful for reproducibility, finite differences, and diagnosis.

## Complete integration measurements

Machine: Intel Core i7-9700, x86-64 Linux. SUNDIALS 6.4.1, double precision,
64-bit indices; current ASCEND build uses `-O2 -g`, `DEBUG=True`, and
`MALLOC_DEBUG=False`. Runs were sequential and unprofiled. These are local
exploratory timings, not portable performance guarantees.

The existing archival X49 driver was run with one radial segment, 0--3 seconds,
30 output intervals, `rtol=1e-6`, and scalar `atol=1e-8`:

| Cells | Unknowns | Structural entries | Density | DENSE (s) | KLU (s) | Speedup |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 230 | 565 | 1.07% | 0.08665 | 0.03484 | 2.49x |
| 2 | 340 | 899 | 0.78% | 0.23269 | 0.06168 | 3.77x |
| 3 | 450 | 1233 | 0.61% | 0.48894 | 0.10432 | 4.69x |

Each pair used identical generated model and runtime hashes. Maximum
reduction-degree differences across all 31 outputs were respectively
`1.93e-15`, `7.08e-16`, and `1.57e-15`. These are single runs per solver.
The runtime SHA-256 was
`44016aa42a6215df86aebb74a201490ff7fed79a14d1fe2747993bf4615a3f1b`.
Previously documented three-radial-segment cases gave 15.3x and 30.9x speedups
at 1480 and 2568 unknowns. The exact source2 U3/U4 cases remain unqualified.

The synthetic driver `benchmarks/small_dae.py` generates coupled linear ODEs
with either tridiagonal or fully dense Jacobians. It times `integ.solve()`,
including pattern construction, matrix/solver allocation, initial consistency,
and Jacobian callbacks. Model loading and `analyse()` are outside the timer.
Results are medians of ten integrations after two warmups in each process:

| Unknowns | Pattern | DENSE (ms) | KLU (ms) | Speedup |
|---:|:---|---:|---:|---:|
| 2 | tridiagonal | 0.191 | 0.197 | 0.97x |
| 8 | tridiagonal | 0.526 | 0.499 | 1.05x |
| 32 | tridiagonal | 1.873 | 1.601 | 1.17x |
| 64 | tridiagonal | 4.618 | 3.245 | 1.42x |
| 128 | tridiagonal | 11.668 | 6.126 | 1.90x |
| 2 | dense | 0.186 | 0.194 | 0.96x |
| 8 | dense | 0.656 | 0.669 | 0.98x |
| 32 | dense | 6.850 | 6.405 | 1.07x |
| 64 | dense | 31.921 | 30.500 | 1.05x |
| 128 | dense | 216.706 | 197.900 | 1.10x |

All states start at one and have exact solution `exp(-t)`; all runs finished
with maximum endpoint error about `1.12e-8`, at `rtol=atol=1e-8`. This is an
easy linear problem exciting only a common decay mode, not a nonlinear,
ill-conditioned or event-switching stress test. Small percentage differences
may be timing noise; independent process repeats and randomized ordering
were not used. Full dense integrations spend substantial time in expression
evaluation, so their linear algebra speedups do not translate directly into
equivalent end-to-end speedups.

## Linear algebra probe and the case against unconditional KLU

`benchmarks/crossover.c` compares the installed SUNDIALS native DENSE and KLU
backends directly, for sizes 8--1024 and six patterns: pentadiagonal, dense
16-by-16 diagonal blocks, random 1%, 5%, 20%, and fully dense. Matrices are
deterministic, strictly row diagonally dominant, with random off-diagonals.
It measures first setup separately from repeated setup and triangular solves;
matrix loading is timed separately. Repeated setup uses unchanged numerical
values, favourable to KLU's pivot reuse. The probe emits CSV for all 96 solver runs.
Every solution was checked against an all-ones solution; maximum error was
`9.1e-15`.

KLU won the measured repeated setup/solve work even for fully dense matrices.
First setup favoured DENSE at tiny sizes: on the fully dense 8-by-8 case,
KLU's first setup took about five times as long. By size 64 their first setup
times were approximately equal. These compare the current native
`SUNLinSol_Dense` backend, not a tuned BLAS/LAPACK dense solver.

There are still reasons to retain DENSE:

* Fully dense storage is expensive through this implementation. With 64-bit
  indices, the CSC matrix alone needs roughly 16 bytes per entry, versus
  8 bytes in a dense matrix; the cached pattern/scatter mapping adds more.
  For the fully dense 1024 case, KLU reported 18.6 MB peak internal allocation,
  in addition to the input matrix and ASCEND's cache. The dense matrix itself
  is 8.39 MB. These are component sizes, not total process memory measurements.
* Density does not predict factor fill reliably. The random approximately 1%
  case at size 1024 had 11,484 input entries but 640,999 L/U/off-block entries
  (L and U both include their diagonals). A similarly sparse banded system
  behaves very differently. A density gate is not a memory guarantee.
* KLU remains an optional dependency and our sparse callback requires
  autodiff. Numerical robustness on difficult models cannot be inferred
  from these well-conditioned synthetic examples.

SUNDIALS performs symbolic and numeric factorization at KLU's first setup;
subsequent setups can reuse symbolic information and pivots, with condition
checks that can trigger fresh numeric factorization. See the
[SUNDIALS 6.4.1 KLU documentation](https://sundials.readthedocs.io/en/v6.4.1/sunlinsol/SUNLinSol_links.html#the-sunlinsol-klu-module).

## Implementation outline

Resolve AUTO in `ida_set_optional_inputs`, before matrix allocation and the
initial consistency solve. Preserve the requested parameter string and keep
the selected backend separately for attachment and diagnostics.

Measure structural density as `nnz / n / n` for the actual IDA system: active
relations, filtered unknowns, and derivative variables mapped to their state
columns. Count the union of `dF/dy` and `dF/dy'`, deduplicating overlapping
entries even when their initial numeric value is zero. Reuse the mapping
rules in `ida_sparse_build`; factor out a shared mapping helper if necessary.

Avoid constructing the entire CSC/scatter cache merely to decide on DENSE.
A row-by-row scan with an O(n) marker array can count unique mapped columns
in O(incidences) time. Skip this scan for unavailable KLU, finite differences,
or tiny systems. If KLU is selected, build its existing cache once. Use
overflow-safe counts and density arithmetic. Invalid incidence mappings are
errors, not reasons to silently change solver.

Reconsider AUTO at the existing structural/event restart boundary, where the
current implementation already rebuilds the linear solver and sparse cache.
Do not switch backends on every timestep or after ordinary nonlinear
convergence failures. Dimension-changing restarts remain unsupported.
Do not add automatic dense retries after KLU numerical/allocation failure in
this first version: they can obscure errors or allocate an enormous matrix.

Extend the existing `stats` diagnostic to record requested/selected solver,
dimension, structural entries/density when measured, and selection reason
(`no-klu`, `finite-difference`, `small-system`, `dense-pattern`, or
`sparse-pattern`). Explicit DENSE/KLU behaviour should remain unchanged.

Test the policy at both threshold boundaries without timing assertions;
exercise AUTO with KLU absent, autodiff disabled, and explicit overrides.
Compare integration outputs with explicit DENSE/KLU. Test same-size events
whose pattern changes across the density threshold, plus existing event and
setup-failure regressions. Repeat the small/TGA benchmarks on the deployment
machine before promoting AUTO to the default. More elaborate fill estimates
or online timing selection can wait for evidence that this policy is inadequate.

## Reproduction

Run the small DAE driver via the existing ASCEND wrapper:

```sh
./a4 script solvers/ida/benchmarks/small_dae.py -- --n 64 --pattern band --solver KLU
# Repeat for DENSE, pattern=dense, and n=2,8,32,64,128.
```

The standalone C probe targets the local SUNDIALS 6.x API:

```sh
cc -O2 -Wall -Wextra -I/usr/include/suitesparse \
  solvers/ida/benchmarks/crossover.c -o /tmp/ida-crossover \
  -lsundials_sunlinsolklu -lsundials_sunmatrixsparse \
  -lsundials_sunlinsoldense -lsundials_sunmatrixdense \
  -lsundials_nvecserial -lsundials_generic -lm
/tmp/ida-crossover > /tmp/ida-crossover.csv
```

For TGA, prepare the fboard2 library as described in SPARSE-JACOBIAN.md, then:

```sh
./a4 script solvers/ida/benchmarks/tga.py -- \
  --models /tmp/ida-fboard-models/models --cells 1 --radial 1 \
  --solver KLU --output /tmp/x49-small-klu.json
# Repeat for DENSE and cells=2,3.
```

The C probe constructs dense reference matrices even for sparse cases: do not
use its process memory as a measure of sparse-solver memory consumption.

# KLU zero-pivot recovery investigation

19 September 2026. The X30/X31 tighter-tolerance failures reported in
`~/kinetics/results/unification_u3_klu/README.md` were reproduced and fixed
without changing models, tolerances, event limits or reporting times.

## Finding

KLU's refactorization reuses an existing numerical pivot sequence. A zero
pivot in that sequence does not establish that the new matrix is singular.
Both SUNDIALS 6.4.1 and 7.9.0 return `SUNLS_PACKAGE_FAIL_REC` immediately when
this refactorization fails; their condition-estimation/fresh-factorization
path is reached only after refactorization succeeds. Reducing IDA's timestep
can repeatedly encounter the same unsuitable pivot sequence.

Sources: [6.4.1 KLU adapter](https://github.com/LLNL/sundials/blob/v6.4.1/src/sunlinsol/klu/sunlinsol_klu.c)
and [7.9.0 KLU adapter](https://github.com/LLNL/sundials/blob/v7.9.0/src/sunlinsol/klu/sunlinsol_klu.c).

The captured 5813-by-5813 Jacobians have 18033 structural entries. At the
first failed refactorization in each case, KLU reported `KLU_SINGULAR`,
`numerical_rank=3521`, `singular_col=5679` and `structural_rank=5813`.
These describe the failed pivot sequence: fresh factorization of the **same
matrix** succeeded with full numerical rank 5813. Replaying the preceding
successful matrix followed by the failed matrix reproduces the failure
outside ASCEND in both SUNDIALS versions.

The first failed trial was at t=59.934550994314719, cj=13853.565145005387
for X30, and t=99.349565853605526, cj=4639.571755649139 for X31. IDA's final
abort times differ from these first rejected trial times. The KLU common
structure retained condition estimates around 5.24e21 from previous completed
setups; those stale estimates must not be reported as measurements of the
failed refactorization itself.

At both first failures, the sparse Jacobian was compared against the existing
dense analytical callback at the same t, cj, y and y'. Maximum absolute
entry difference was zero, and there were no dense nonzeros outside the CSC
pattern. Failure occurred long after the initial event sequence; fresh
factorization recovered without a structural restart or model change.

## Fix and regression

`ida_klu_setup` wraps the SUNDIALS KLU setup operation. Only a recoverable
package failure with KLU status `KLU_SINGULAR` triggers one fresh numerical
factorization. It retains symbolic analysis and replaces the numerical
factors only on success. If the new matrix really is singular, the original
recoverable result and existing numeric object remain available for IDA's
next trial. Allocation or invalid-input failures remain failures. This does
not switch to DENSE, perturb entries, or relax tolerances.

The implementation uses the KLU content structure declared in SUNDIALS'
`sunlinsol_klu.h` to replace `numeric` and update `last_flag`. This interface
was compiled and exercised with 6.4.1 and 7.9.0. Future changes to that content
structure require review. The optional build check now explicitly links KLU
and exercises its numerical factorization/free interfaces.

The focused `klu_pivot_recovery` test starts with
`A = [[1,1],[1,2]]`, then changes it to `[[0,1],[1,2]]`. Native refactorization
fails at the reused zero pivot although the determinant is -1. The recovery
path succeeds, retains symbolic analysis and solves correctly. The test also
supplies the genuinely singular matrix `[[0,0],[1,2]]`, checks that failure
is preserved, and then verifies that a later nonsingular trial succeeds.

Local regression results: 79/79 main tests and 89/89 focused tests pass with
DENSE, KLU and AUTO selections. The 89-test focused KLU run under Valgrind
has zero errors and no definitely/indirectly/possibly lost allocations
(104 bytes remain reachable). An intentionally missing optional KLU library
and custom library path correctly disable KLU; all 86 applicable focused
tests pass in that build. The normal KLU-enabled build is restored afterward.

## Replays

The local frozen 17-cell input bundle was available, but the generated
33-cell/KLU bundle and recorded fboard2 merge `b230bfce` were not. The public
fboard2 revision was `3098f08e`. An isolated checkout combined that revision
with sparse commit `fbfffba1`, preserving fboard2's parameter enumeration,
diagnostics and IDA_ONE_STEP/report interpolation logic. The three overlapping
IDA edits were resolved only in the temporary checkout. No fboard2 changes
were merged into ida-sparse.

Inputs were reconstructed using the retained `refine(text,17,33)` and
`select_solver(text,'KLU')` generator functions. All 16 ASCEND model dependency
hashes matched the frozen manifest. The reconstructed baseline reproduced
both published abort times and IDA counters. This branch's current `a4 run`
path does not preserve those wrappers' integrator options, so it is not an
equivalent replay launcher; an initial attempt stopped at the default event
limit rather than the requested 200.

SuiteSparse was held fixed at **7.6.1**, KLU **2.3.2**, using the same installed
`libklu.so.2`. SUNDIALS **6.4.1** was the installed distribution build;
**7.9.0** was built separately in a temporary prefix. Both used double
precision and 64-bit indices. ASCEND IDA plugins were compiled separately
against the two SUNDIALS installations, using the same reconstructed ASCEND
sources. The final recovery implementation was then applied to both plugins.

| Case | Baseline, both SUNDIALS versions | With recovery, both versions |
|:--|:--|:--|
| X30, rtol=1e-11 | abort at 59.939915861089311 s | completes 2379 s |
| X31, rtol=1e-11 | abort at 99.349116819622267 s | completes 2373 s |

Completed X30 runs used 24861 steps and 1924 Jacobians; X31 used 27068 steps
and 2177 Jacobians. Counts matched across versions, as did all sampled RD
values. Neither corrected tighter run logged an error. Runs overlapped on
one workstation, so elapsed times are not a controlled performance comparison.

Against unmodified standard-tolerance (rtol=1e-10) KLU runs:

| Case | Matched samples | Max RD difference (percentage points) | Max phase-fraction difference |
|:--|--:|--:|--:|
| X30 | 794 | 1.51811e-8 | 2.98018e-10 |
| X31 | 792 | 6.67739e-9 | 6.13898e-10 |

Maximum global Fe/Si balance errors in the corrected tighter runs were
5.45e-11; maximum reported local Fe/Si balance errors were 2.57e-10.
X31 standard retained the previously reported lower-bound message for
`core.slice[30].shell[1].step2_product_fraction_pos`; this investigation does
not fix or waive that separate qualification issue. The entire nine-case
panel and its full scientific gates have not been rerun here. The two tighter
failures provide no remaining reason to relax the declared tolerance ladder.

Local diagnostic sources, captured matrix pairs, logs and trajectories are
under `/tmp/ida-klu-investigation/`; these temporary captures are not committed.
The small regression above is the durable reproducer.

## Build and deployment

`./a4 solvers` now reports `SUNDIALS <version> (with KLU)` or
`SUNDIALS <version> (KLU-less)` from the loaded plugin's build capability.
SuiteSparse/KLU and the SUNDIALS KLU adapter are optional dependencies.
An IDA installation alone does not imply their availability.

On NCI, the user identified SUNDIALS 7.6.0 installed under `~/.local`, with
`CMAKE_INSTALL_LIBDIR=lib64` and `ENABLE_KLU=OFF`. Install/reuse SuiteSparse
first, then reconfigure that existing SUNDIALS build with `ENABLE_KLU=ON`,
`KLU_INCLUDE_DIR` pointing to the directory containing `klu.h`, and
`KLU_LIBRARY_DIR` pointing to its libraries. Build and install SUNDIALS before
rerunning ASCEND's configuration. See the
[SUNDIALS KLU build documentation](https://sundials.readthedocs.io/en/v7.3.0/sundials/Install_link.html#building-with-klu).

ASCEND supports `SUNDIALS_KLU_CPPPATH` and `SUNDIALS_KLU_LIBPATH` for custom
SuiteSparse include/library directories, plus `SUNDIALS_KLU_LIBS` for extra
static-link dependencies. Use `scons --config=force` to repeat cached probes.
A failed optional KLU check restores the dense build's include/library paths
and libraries. Confirm `with KLU` after rebuilding and qualify the target
runtime with the focused test suite and the frozen scientific cases.

The recovery fix remains necessary independently of enabling KLU. Upgrading
SUNDIALS alone did not resolve either failure. SUNDIALS 7.6.0 and the actual
NCI SuiteSparse build have not been tested locally.

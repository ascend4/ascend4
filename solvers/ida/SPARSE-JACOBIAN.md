# Optional sparse Jacobians and KLU for IDA {#sec:ida-sparse}

Date: 19 September 2026. Status: implementation proposal; no sparse solver
implementation or speedup has yet been demonstrated.

This note records the assessment of ASCEND revision
`3098f08e7c5fdc50041187218a79007ba70bd2d9` and provides a starting point for
separate feature-branch work. The recommended first deliverable is explicit
`linsolver=KLU`, retaining `DENSE` as the default and comparison reference.
Automatic selection should follow qualification and measured crossover tests.

## Motivation and measured evidence {#sec:ida-sparse-evidence}

Local profiling of the fixed-parameter X49 TGA model, after rebuilding with
`MALLOC_DEBUG` disabled, identifies dense factorisation and back-solving as
the dominant measured cost. The local runtime used SUNDIALS 6.4.1 with
`DEBUG=True`; ordinary compiler debugging and memory-debug bookkeeping are
distinct settings. The user also confirmed memory debugging disabled on
Gadi, so the earlier local memory-debug probe does not explain the NCI
slowdown.

| Probe | Outcome | Dense factorisation and back-solving |
|---|---|---:|
| Nine cells, 120 simulated seconds, gprofng | Complete in 129.20 s walltime | 83.14% of sampled work |
| 33 cells, requested 120 simulated seconds, gprofng | Deliberately stopped after 590.55 s; startup only | 88.52% of sampled work |
| Five cells, three simulated seconds, Callgrind | Complete | 83.67% of instructions |

The routines are `SUNDlsMat_denseGETRF` and `SUNDlsMat_denseGETRS`. In the
nine-cell sample, Jacobian assembly itself accounts for 7.54% exclusive
time, residual evaluation about 1.40% inclusive time, and `IDASolve` 93.24%
inclusive time. Inclusive measurements overlap and must not be added.
The nine-cell trajectory matches the existing reference through 120 seconds
to within 1.6e-9 RD percentage points. Five-cell output was byte-identical
under native, sampled and Callgrind execution.

The 33-cell run barely advanced (last observed simulated time about
8.15e-11 s). Its profile establishes expensive startup work, not the cost
distribution over a complete trajectory. A separate native 17-cell probe
hit a 600-second timeout; it does not identify its own bottleneck.

The initial analysed DAE is structurally very sparse:

| Bed cells | IDA unknowns | Differential | Algebraic | Structural entries | Density | One dense matrix |
|---:|---:|---:|---:|---:|---:|---:|
| 9 | 1685 | 81 | 1604 | 5121 | 0.18037% | 21.66 MiB |
| 17 | 3061 | 153 | 2908 | 9425 | 0.10059% | 71.49 MiB |
| 33 | 5813 | 297 | 5516 | 18033 | 0.05337% | 257.80 MiB |

These counts represent the initial structural union of state and derivative
incidence, not numerical nonzeros, factorisation fill or total solver memory.
Some probes ran concurrently within a six-CPU limit. gprofng clock-derived
absolute CPU totals undercount its process-resource totals on this host;
use sampled shares, independent instruction counts and measured walltime.
Local measurements are not NCI runtime predictions.

The user subsequently supplied two Gadi snapshots for qualification study
`unification_u3_bed33`, job `179393243.gadi-pbs`. Between 21:15:23 and
21:47:51 AEST on 19 September, elapsed job time increased from 69.4 to
101.9 minutes and completed production cases increased from 1/18 to 6/18,
with no reported failures. X30 advanced from about 64.80 to 903 simulated
seconds and X31 from 100.11 to 1005 seconds. This is evidence of improved
progress after an expensive early phase, consistent with the user's
interpretation of early reduction complexity. It does not identify whether
the improvement comes from larger accepted steps, fewer nonlinear or
consistency iterations, changed matrix structure, or another cause.
Separate startup and later-stage profiles rather than extrapolating a
near-zero-time rate across the full trajectory. Twelve cases remained
incomplete in that snapshot; neither numerical qualification nor a reliable
completion estimate follows from it. The configured two-hour per-case
timeout is separate from the four-hour PBS allocation.

The associated kinetics working copy retains `profile_u4_ascend.py`,
`analyze_u4_performance.py`, and the summary note/PDF in
`results/unification_u4_performance/`. Detailed profiler archives and
generated models are deliberately not repository deliverables. The evidence
above is included here so this proposal remains understandable separately.

## Existing implementation and scope {#sec:ida-sparse-scope}

This is a moderate, contained extension to the IDA wrapper. It should not
require a new particle model, compiler language feature, derivative engine
or integration algorithm. Relevant files are:

| File | Responsibility |
|---|---|
| [SConscript](SConscript) | SUNDIALS detection, feature checks and linking |
| [ida.c](ida.c) | Solver parameters, matrix allocation, solver attachment, initialisation and cleanup |
| [ida.h](ida.h), [idatypes.h](idatypes.h) | Version-dependent includes and engine-owned matrix/cache state |
| [idacalc.c](idacalc.c), [idacalc.h](idacalc.h) | Jacobian callback and derivative assembly |
| [idaanalyse.c](idaanalyse.c) | Active relation lists, variable ordering and derivative-to-state mapping |
| [idaboundary.c](idaboundary.c) | Event iteration and reanalysis lifecycle |
| [test_ida.c](../../ascend/integrator/test/test_ida.c) | C regression tests |
| [IDA test models](../../models/test/ida/) | Small self-contained model fixtures |

`ida_set_optional_inputs` currently constructs `SUNDenseMatrix` and
`SUNLinSol_Dense`. `integrator_ida_djex` already evaluates relation gradients
and accumulates derivative contributions into the appropriate state column.
The engine field `dense_matrix` is a `SUNMatrix`, but its name and allocation
path assume dense storage. The modern `ASCEND` linear-solver option falls
back to dense; selecting it does not provide sparse direct factorisation.

The local SUNDIALS installation has KLU support, but that does not establish
availability in the separate Gadi installation. The current wrapper supports
SUNDIALS 5 and newer, with version-dependent context/type handling to preserve.

## Proposed implementation {#sec:ida-sparse-design}

### Optional dependencies and explicit selection

Add a compile/link capability test for SUNDIALS sparse matrices and KLU,
including the SuiteSparse headers/libraries required by that installation.
Link the sparse and KLU components only when available. Missing optional
dependencies must not disable an otherwise working dense IDA build.
Expose the capability in configuration output and guard the implementation
with a feature macro. An explicit unavailable `linsolver=KLU` request must
fail clearly, rather than silently use dense.

Use `SUNSparseMatrix`, `SUNLinSol_KLU` and `IDASetLinearSolver`. SUNDIALS
provides a KLU adapter for its sparse matrix type; ASCEND need not implement
sparse factorisation itself. See the
[SUNDIALS KLU interface](https://sundials.readthedocs.io/en/v6.7.0/sunlinsol/SUNLinSol_links.html#the-sunlinsol-klu-module).

### Structural pattern and numerical values

For residual equations $F(t,y,\dot y)=0$, assemble

$$
J = \frac{\partial F}{\partial y}
    + c_j\frac{\partial F}{\partial\dot y}.
$$

Reuse ASCEND's current gradient calculations and derivative-to-state mapping.
Construct a compressed sparse pattern from the actual analysed active
relation list and solver variable ordering. CSC is a reasonable first choice.
Sort and deduplicate each column, merging state and derivative contributions
to the same entry. Cache the scatter locations used to insert subsequent
gradient values; avoid repeated searches and allocations in the callback.
Use `sunindextype` consistently and check sizes/capacities.

The pattern must represent possible structural entries, including entries
whose numerical derivative is currently zero. Do not infer structure by
thresholding a numerical Jacobian. Refresh values when IDA requests a
Jacobian, including the current `c_j`, and accumulate duplicate contributions
with addition. Verify the sparse-matrix zeroing contract for each supported
SUNDIALS version; restore index arrays in the callback if the matrix operation
clears them. Retain a separate authoritative pattern cache.

Initially require the existing `autodiff` path for KLU. The dense
finite-difference fallback must not be assumed to construct sparse Jacobians.
Reject unsupported explicit combinations clearly; coloured sparse numerical
differentiation can be separate future work. The relevant callback contract
is documented in the
[IDA user guide](https://sundials.readthedocs.io/en/v6.7.0/ida/Usage/index.html).

### Lifecycle and conditional events

Rename or generalise matrix ownership, and release the solver, matrix and
pattern/work buffers on all normal and failed exits. During smooth
integration, retain the structure and let the KLU adapter manage repeated
factorisations. For the first implementation, invalidate and reconstruct
the pattern and linear solver whenever ASCEND structurally reanalyses the
system. This includes changes in equation identity, ordering or incidence
without a change in dimension. Handle changed state mappings and dimensions
through the existing reanalysis/reinitialisation path.

Preserve the recently corrected guard/root and event-side semantics. Reuse
across events is a later optimisation, requiring explicit proof that ordering
and pattern are unchanged. If retaining an existing KLU object while changing
its pattern, request fresh symbolic factorisation through the supported
reinitialisation interface; changing numerical values alone is insufficient.
[KLU reinitialisation documentation](https://sundials.readthedocs.io/en/v6.7.0/sunlinsol/SUNLinSol_links.html#c.SUNLinSol_KLUReInit)

### Error handling and diagnostics

Source inspection found that `ida_prepare_integrator` ignores return codes
from several setup calls, and its caller also needs review. Propagate
allocation, solver-attachment and initial-consistency failures with proper
cleanup. This is particularly important when adding optional dependencies
and new setup failure modes.

Report the requested and selected solver, dimension, structural entry count,
and selection reason. Make useful IDA statistics available without a debug
rebuild: steps, residual/Jacobian evaluations, linear setups and convergence
failures. Track initialisation and event reinitialisations separately, or
accumulate counters across resets, so performance comparisons are interpretable.

## Automatic selection policy {#sec:ida-sparse-auto}

Keep `DENSE` as the initial default and offer explicit `KLU`. After numerical
qualification, add `AUTO` using both dimension and structural density, with
thresholds derived from benchmarks rather than a fixed size guessed from
this one model. Availability and supported derivative mode also constrain
the choice. Small or dense systems may still favour dense factorisation.

Keep explicit overrides and log the selected solver. `AUTO` may select dense
when KLU is unavailable, but must explain that choice. Do not silently switch
algorithms after a numerical failure. Reconsider selection at structural
reanalysis rather than arbitrarily during Newton iterations. A change of
the default to `AUTO` should be a separate, measured decision.

## Validation and acceptance {#sec:ida-sparse-validation}

1. Add small deterministic fixtures comparing sparse and dense Jacobian
   entries, including a relation containing both a state and its derivative,
   and an initially zero derivative that later becomes nonzero.
2. Exercise dense-only and KLU-enabled builds, unsupported configurations,
   setup failures and repeated create/solve/destroy cycles. Cover supported
   SUNDIALS versions where build environments are available.
3. Run both solvers through the existing smooth DAE, boundary, reset,
   event-side and close-event regressions. Add a conditional fixture that
   changes incidence without changing dimension; also cover dimension and
   variable-order changes. Check event outcomes and consistent states, not
   just solver success codes.
4. Benchmark unchanged fixed-parameter TGA cases at 5/9/17/33 cells. Separate
   startup, smooth integration and event work; compare elapsed time, memory
   and solver counters. Require equivalent trajectories within declared
   tolerances, rather than bitwise identity between factorisation methods.
5. Rerun the kinetics U1 event/conservation checks and U3 tolerance/grid
   qualification before launching U4 resolved-bed fitting with the new
   runtime. Validate the Gadi build separately and keep the runtime identity
   with each qualification result.

The first milestone is a correct explicit sparse path with reproducible
benchmarks. Automatic selection and aggressive reuse across events are
subsequent milestones. Scientific model changes, tolerance relaxation,
binary-token work and algebraic elimination are outside this first feature.

## Assessment {#sec:ida-sparse-assessment}

The largest correctness risk is preserving the Jacobian pattern and mapping
through conditional changes, not calling the KLU constructor. The current
profile makes this the highest-priority performance experiment: dense
factorisation dominates, while residual compilation alone addresses a small
measured fraction. Sparse fill, ordering, conditioning and changed iteration
behaviour still determine the actual speedup. No speedup estimate or solution
of the separate late-stage kinetic questions should be inferred yet.

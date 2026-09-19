# Optional sparse Jacobians and KLU for IDA {#sec:ida-sparse}

Date: 19 September 2026. Status: experimental explicit KLU implementation
with passing local regressions and short TGA benchmarks. Production/Gadi
qualification remains outstanding. The original proposal and evidence are
retained below; see the implementation results at the end.

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
without a change in dimension. Handle changed state mappings through
reanalysis. A changed dimension requires recreating IDA memory and its
dimension-dependent objects: `IDAReInit` only supports an unchanged problem
size. The existing wrapper must not be assumed to handle resizing correctly
(see the follow-up investigation below).

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
behaviour still determine the actual speedup. The proposal alone did not establish a speedup or resolve the separate
late-stage kinetic questions; the subsequent local measurements below have
their own narrower scope.

## Follow-up investigation on ida-sparse {#sec:ida-sparse-investigation}

19 September 2026; wrapper inspected at `79525da8`, with model source inspected
directly from `fboard2` at `0062317c`. No production solver code was changed
and no new TGA trajectory or speedup was measured in this investigation.

### Findings that refine the implementation

* `relman_diff3` walks the relation incidence list and returns every variable
  passing the filter, including zero-valued derivatives, in that order
  (`ascend/system/relman.c`). Build a per-relation scatter array in this same
  filtered order. Each scatter entry stores a CSC value offset and whether
  to multiply by `c_j`. Both a state and its derivative may map to the same
  offset, so assembly must add contributions. Rebuild this cache whenever
  analysis changes the relation list, incidence, filter or variable mapping.
* `integrator_ida_djex` allocates two work arrays on every callback and scans
  the entire dense matrix for NaNs afterwards. Allocate reusable work buffers
  from the maximum required relation incidence and check only stored sparse
  entries. Its bounds-error early return currently bypasses freeing the two
  arrays; include that failure path in the cleanup work.
* A standalone C probe compiled and ran against the installed SUNDIALS 6.4.1
  sparse/KLU modules. Both initial factorisation and subsequent refactorisation
  solved a three-variable system to within `1e-12`. This verifies local
  compile/link/runtime availability, not ASCEND integration or performance.
  The installation uses eight-byte values and eight-byte indices.
* That probe confirmed that `SUNMatZero` clears **both column pointers and
  row indices**, as well as values. Restore the cached CSC indices on each
  callback. An allocated capacity is not a populated sparse pattern.
* The probe needed `-I/usr/include/suitesparse` in addition to standard include
  paths. Its shared-library link used `sundials_sunlinsolklu`,
  `sundials_sunmatrixsparse`, `sundials_nvecserial`, `sundials_generic` and `m`.
  Treat this as local evidence, not a portable hardcoded dependency list;
  static linking and custom prefixes require separate capability checks.
* Setup error propagation needs fixing in both `ida_prepare_integrator` and
  its caller. `ida_bnd_reanalyse` also discards the result of
  `integrator_ida_analyse`. Check matrix allocation before constructing the
  linear solver, and stop before consistency solving on any setup failure.
* The current event path creates new output vectors when `n_y` changes, but
  still calls `IDAReInit` on the old IDA memory. SUNDIALS explicitly requires
  the same dimension for this operation. Track the allocated dimension and
  either recreate the complete IDA object or reject resizing clearly until
  that path is implemented. Same-dimension incidence changes must still be
  supported by rebuilding the sparse pattern and KLU object.
  [IDA reinitialisation contract](https://sundials.readthedocs.io/en/v6.4.1/ida/Usage/index.html#ida-reinitialization-function)
* Event consistency iteration can already reinitialise IDA, followed by
  another reinitialisation in the outer root-handling path. Count these
  separately before considering reuse or removing a restart; preserve the
  event semantics in the first sparse implementation.

The inspected linked-bed model connects neighbouring axial cells and radial
shells, with additional shared variables and aggregate equations. That makes
low fill plausible, but does not establish a narrow bandwidth in the actual
IDA ordering or guarantee inexpensive sparse factors. Prefer KLU's existing
ordering initially; the adapter defaults to COLAMD and also supports AMD.
It already manages numeric refactorisation and conditioning checks.
[KLU adapter documentation](https://sundials.readthedocs.io/en/v6.4.1/sunlinsol/SUNLinSol_links.html#the-sunlinsol-klu-module)

At the recorded structural sizes, CSC values, row indices and column pointers
alone would occupy approximately 0.091, 0.167 and 0.320 MiB for 9, 17 and 33
cells respectively, using the local eight-byte types. These figures exclude
cached patterns, scatter maps, KLU factors, IDA vectors and model storage.
They demonstrate the matrix-storage opportunity, not total memory savings.

### Suggested sequence and decision points

1. **Make setup failures reliable and add the optional KLU capability check.**
   Keep `DENSE` as default; require analytical derivatives for explicit KLU.
   Keep dense-only installations working. Put pattern construction, scatter
   assembly and destruction in a small dedicated module, with ownership in
   `IntegratorIdaData` and wiring through the existing setup/reanalysis paths.
2. **Implement the explicit path and a bounded matrix replay experiment.**
   Verify dense/sparse entry equality on small fixtures for several `c_j`
   values, including overlapping state/derivative incidence and structural
   zeros. Capture a few actual callback matrices from an unchanged TGA case:
   initial-consistency work, early integration, a later point and an event
   if present. Preserve `t`, `c_j`, row/column mappings and structure identity.
   Existing `writeMatrix("dF/dy")` and `writeMatrix("dF/dy'")` exports use
   separately filtered variable spaces; they cannot simply be added without
   mapping derivative columns back to state columns.
3. **Measure replay and complete trajectories.** Replay the same matrices and
   right-hand sides through SUNDIALS dense and KLU. Record first symbolic
   analysis/factorisation, repeated numeric setup, repeated solves, factor
   nonzeros and backward residuals. Compare COLAMD with AMD if fill is poor.
   This isolates the linear algebra opportunity, while full 5/9/17/33-cell
   integrations establish the actual speedup and numerical equivalence.
   Separate model construction/presolve, `IDACalcIC`, smooth integration and
   event consistency work. Snapshot counters before resets and aggregate
   across IDA objects. Time unprofiled runs serially under the same runtime
   and resource limits; keep diagnostic dumping outside timed measurements.
4. **Qualify before fitting or changing defaults.** Run the existing hybrid
   regressions under both solvers, add same-size structural-change tests and
   an explicit resizing outcome, then rerun the scientific U1/U3 checks.
   Record runtime and model revisions independently when using fboard2 model
   files with the ida-sparse runtime. Only then qualify the Gadi build and
   consider `AUTO` or event-pattern reuse.

For prioritisation, an Amdahl sensitivity calculation using the nine-cell
sampled linear-algebra share of 0.8314 gives
`S_total = 1 / (0.1686 + 0.8314 / S_linear)`: a tenfold reduction in that
work would imply about 4.0 times overall, and a twentyfold reduction about
4.8 times, **if all other work and solver iteration counts stayed fixed**.
This is an illustrative work-share calculation, not a walltime forecast.
The startup-only 33-cell profile cannot support a full-trajectory estimate.

The first decision is therefore whether real matrices factor with modest fill
and acceptable backward residuals; the decisive acceptance result is faster
complete trajectories with matching states, events and conservation checks.
If replay improves sharply but total runtime does not, inspect consistency
iterations, event restarts and the new measured bottleneck before adding more
solver machinery. Matrix-free Krylov/preconditioner work, manual block
elimination and threaded sparse solvers should follow evidence from this
smaller direct-solver experiment.


## Implemented explicit KLU path and local checks {#sec:ida-sparse-results}

The working implementation retains `DENSE` as default. Select `linsolver=KLU`
and `autodiff=true` explicitly. `stats=true` prints solver selection, dimension,
CSC entry count, counters before consistency/event resets and at completion,
and KLU factor nonzeros and KLU's own peak allocation counter. The reported
phase counters are raw snapshots; do not blindly sum repeated snapshots after
failed setup/restarts. KLU memory is not total process memory.

`idasparse.c` owns a sorted/deduplicated CSC pattern and per-relation scatter
map, with reusable gradient work arrays. It preserves structural zeros and
adds `dF/dy` and `c_j*dF/dy'` contributions to shared entries. Every callback
restores the CSC indices. Every linear-solver restart rebuilds the pattern and
KLU object. Matrix, solver and cache are released when integration exits.
Setup/tolerance/attachment/consistency failures now propagate, including the
analysis return code during events and signal-handler cleanup after a failed
consistency solve. Actual dimension changes are explicitly rejected before
`IDAReInit`; full resizing is not implemented.

The optional SCons compile/link check defines `ASC_IDA_KLU` only when the
sparse/KLU modules work. `WITH_IDA_KLU=False` forces a dense-only build.
`SUNDIALS_KLU_CPPPATH` supplies extra include directories (path-separated),
and `SUNDIALS_KLU_LIBS` supplies additional libraries (comma/space-separated),
for example for custom/static SuiteSparse installations. Existing
`SUNDIALS_LIBPATH` supplies library directories. A failed KLU check restores
the dense build's libraries and include paths; an explicit unavailable KLU
request fails instead of falling back.

### Validation

On the local double-precision, 64-bit-index SUNDIALS 6.4.1 installation:

* Main `integrator_ida` suite: 79/79 passed with DENSE and with KLU.
* Focused `test-ida` runner: 82/82 passed with each solver, including direct
  entry comparison at `c_j=0,1,1000`, initially zero derivatives becoming
  nonzero, overlapping state/derivative contributions, and setup-failure
  retry. A same-dimension event replaces `z=y` with `z=w`; a separate fixture
  actually grows from two to three unknowns at an event and is rejected.
* Dense-only build: 81/81 focused tests passed (the sparse entry test is not
  compiled). Explicit KLU requests are rejected.
* A deliberately nonexistent optional KLU library made the capability check
  fail, while dense IDA still built and all 81 focused tests passed.
* Valgrind over all 82 focused KLU tests: zero memory errors, zero definitely,
  indirectly or possibly lost bytes; 104 bytes remained reachable.
* SUNDIALS 5/7 and Gadi have not been tested. The version-dependent constructor
  branches remain in place, but local success is not cross-version evidence.

Typical local commands, with CUnit available in the configured paths:

```sh
scons -j4 test test-ida solvers/ida solvers/lrslv solvers/qrslv MALLOC_DEBUG=False
LD_LIBRARY_PATH=.:$HOME/.local/lib test/test integrator_ida
ASC_TEST_IDA_LINSOLVER=KLU LD_LIBRARY_PATH=.:$HOME/.local/lib test/test integrator_ida
LD_LIBRARY_PATH=.:$HOME/.local/lib solvers/ida/test_ida
ASC_TEST_IDA_LINSOLVER=KLU LD_LIBRARY_PATH=.:$HOME/.local/lib solvers/ida/test_ida
```

The focused runner links the IDA implementation objects so internal matrix
callbacks can be tested without exporting them as a public plugin API. The
main harness runs the public integration tests. Build LRSlv as well: stale
local LRSlv binaries initially caused eight shared event-test failures; these
all disappeared after rebuilding from this branch.

### Branch audit

At investigation time `python3` was `73716714`, an ancestor of `ida-sparse`.
Their committed IDA/LRSlv/integrator code was identical; the branch's extra
commit was this proposal. Thus the SATISFIED/nearby-event fixes from python3
are already present here. `fboard2` at `0062317c` contains additional work:

* `351737ba`: preflight diagnostics option.
* `4b8c89cf`: `IDA_ONE_STEP` integration and interpolated reporting, intended
  to decouple solver behaviour from reporting intervals.
* LRSlv caller-provided logical blocks for CMSlv2.

None is a prerequisite for sparse assembly or KLU. No branch was merged.
Keep the reporting/timestep change separate from this performance comparison;
if brought across later, port it onto the current event fixes and rerun the
hybrid tests, rather than replacing this branch's IDA files wholesale.

### Reproducible archival X49 probe

The exact generated U3/U4 models/profiling scripts named in the original note
were not found in the local kinetics directory. A separate, older fixed X49
parameter set (15 April 2026) was available. Its parameters are captured in
`benchmarks/x49_legacy.a4c.in`; it uses `tgadyn_msbed_linked_radial.a4c` from
fboard2, with three radial segments. **This is not the original profiled
source2 case**, so its dimensions and timings must not be substituted into
the earlier evidence table.

`benchmarks/tga.py` generates the fixture, analyses the DAE, selects the solver
and integrates from 0 to 3 seconds with 30 output intervals, `rtol=1e-6` and
scalar `atol=1e-8`. It saves the reduction-degree trajectory, all final solver
variables, solve/preparation times, and hashes of the generated model and IDA
plugin. Supply the fboard2 model library, using a worktree or an archive:

```sh
mkdir -p /tmp/ida-fboard-models
git archive fboard2 models | tar -x -C /tmp/ida-fboard-models
./a4 script solvers/ida/benchmarks/tga.py -- \
  --models /tmp/ida-fboard-models/models --cells 9 --solver KLU \
  --output /tmp/x49-9-klu.json
# Repeat with --solver DENSE and another output path.
```

Runs were unprofiled and sequential, with the same runtime and model inputs
for each dense/KLU pair. `DEBUG=True`, compiler `-O2 -g`, `MALLOC_DEBUG=False`.
Preparation is separate from the reported integration time, which includes
initial consistency and output callbacks. These are individual timing probes,
not statistically established performance distributions.

A final sweep with the checked-in benchmark driver and matching model/runtime
hashes gave:

| Cells | IDA unknowns | CSC entries | DENSE solve (s) | KLU solve (s) | Paired speedup |
|---:|---:|---:|---:|---:|---:|
| 5 | 1480 | 4411 | 7.719 | 0.504 | 15.3x |
| 9 | 2568 | 7755 | 33.430 | 1.081 | 30.9x |
| 17 | 4744 | 14443 | not run | 2.653 | not measured |
| 33 | 9096 | 27819 | not run | 7.186 | not measured |

The paired runs used the same IDA plugin SHA-256:
`44016aa42a6215df86aebb74a201490ff7fed79a14d1fe2747993bf4615a3f1b`.
Earlier probes gave 7.52/0.47 s at five cells and 33.25/1.14 s at nine cells,
consistent with the direction and scale of the final sweep.

Maximum reduction-degree differences over
31 outputs were `5.60e-13` and `1.08e-11`, respectively (dimensionless reduction
fraction). The five-cell pair used identical 458 accepted steps and 48
integration Jacobians; the nine-cell pair used 624 steps and 56 Jacobians,
with one extra dense residual/nonlinear evaluation. Both pairs also used two
initial-consistency Jacobians. These comparisons support the sparse path but
do not replace event/conservation/tolerance qualification of the scientific
study.

KLU also completed the 17-cell (4744 unknowns, 14443 CSC entries) and 33-cell
(9096 unknowns, 27819 entries) three-second probes in roughly 2.65 and 7.19 s.
No complete dense comparison was run at those sizes. At the end of the
33-cell KLU run, the factors had 17654 L entries, 21448 U entries (both count
diagonals), and 10010 entries in off-diagonal BTF blocks. KLU reported about
5.42 MiB peak allocation. This is direct evidence of modest fill for this
probe, not a bound for other states or model variants.

Before U4 fitting: obtain the exact fixed-parameter source2 cases and original
run configuration, compare complete trajectories through later reduction,
run U1/U3 event/conservation/tolerance checks, and qualify the Gadi runtime.
Automatic selection, symbolic reuse across events, full dimension-changing
restart support and the separate fboard2 reporting changes remain subsequent
work.

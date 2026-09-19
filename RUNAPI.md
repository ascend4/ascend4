# Run/Case API Notes

## Purpose

This note records the gap between ASCEND's current low-level simulation APIs
and the higher-level workflow that users repeatedly need for:

- loading a model
- applying a case definition
- running setup/reset methods
- integrating or solving
- collecting observed output

The immediate motivation is the TGA dynamic workflow, but the same pattern
appears across test code, command-line glue, and user scripts.

The designs below are exploratory, not a commitment to a new orchestration
framework. The current direction is small CLI/Python conveniences around
model-owned METHODs, preserving full access through ascpy. A `RunSpec` or METHOD
parameters should be justified by real driver migrations, not assumed necessary.

## Status At A Glance

As of 2026-09-19, the `runapi` branch implements an incremental convenience and
reporting layer over the existing ascxx/ascpy API, not a new case-running
framework:

- **Implemented:** hook-state preservation, consistent integration option/time
  handling, clear-on-change solver options, structured/partial integration
  results, unit-aware CLI overrides and explicit METHOD execution ordering.
- **Available entry points:** `a4 run` / `a4 int`, `execute_model`,
  `execute_integration`, `apply_overrides` and `render_run_result`.
- **Not implemented:** `RunSpec` / `run_case`, METHOD parameters, direct CLI
  binding of structural model parameters, new batch orchestration, or a nested
  simulation/external-function adapter. The conceptual APIs below are proposals,
  not callable interfaces.
- **Validation:** the latest full local `./a4 pytest -q` run recorded **272 passed,
  7 skipped, 53 warnings**. A small dynamic fixture agrees with an equivalent
  wrapper MODEL and an analytic solution; the complete kinetics workflow has
  not yet been validated against these changes.
- **Git checkpoint:** the hook/integrator groundwork is committed; the subsequent
  result, solver-option and CLI-override changes are still local/uncommitted at
  this checkpoint.

Start with [Current Implementation](#current-implementation) for supported
behaviour and examples, [Implementation Overview](#implementation-overview) for
the code layout, and [Tricky Use Cases](#tricky-use-cases) for design boundaries.

## Current Problem

This section records the original motivation; some of these gaps are now
addressed by the conveniences documented below. The original workflow comparison
identified four recognizable paths:

1. `./a4 run ...`
   - robust
   - aligns with modern `METHOD`, `OBSERVE`, `INTEGRATOR`, `INTEGRATE`
   - convenient reporting
   - but awkward to drive repeatedly from external tooling

2. low-level `ascpy` / C API
   - flexible in principle
   - but too much boilerplate is pushed onto callers
   - dynamic setup and integration are not expressed at the right level

3. the older `.a4s` scripting layer
   - many historical models used sibling `.a4s` scripts as the "case runner"
   - this was a fairly capable workflow layer
   - but it depended on Tcl/Tk integration and the old GUI stack
   - and it imposed Tcl as a second user-facing language

4. bespoke Python scripts over `ascpy`
   - comparable in spirit to what can be done in the GTK GUI
   - more modern and easier to integrate with data/optimization tooling
   - but still too low-level for many ordinary simulation tasks

In practice this leads to a fifth pattern:

5. generate tiny wrapper models on the fly, then call `./a4 run ...`
   - robust enough
   - debuggable
   - but unnecessary when only mutable numerical case inputs differ

The target is to remove repetitive numerical-case wrapper generation, not all
wrappers. Parameterised models and explicit refinements that assign structural
constants remain appropriate for element counts, component choices and equation
structure. These are distinct from run-time assignments to an existing instance.

## Historical Perspective

ASCEND has already had the idea of a higher-level run layer more than once.

- `.a4s` scripts provided one historical workflow layer around model execution
- GUI actions provided another
- `runmodel.py` is a newer attempt to expose a higher-level path in Python

So the real task is not to invent the idea from scratch, but to identify the
reusable core that should sit below all of these.

## Repeated Boilerplate

Current callers often have to do some variation of:

1. load module
2. find type
3. instantiate simulation
4. run `on_load`
5. run one or more setup methods
6. set scalar variables directly
7. manage `FIX` / `FREE` state indirectly through methods
8. rebuild / analyse / solve / integrate
9. set up observed variables
10. collect tabular output
11. handle errors, retries, and failed partial runs

This is too much repeated policy at the call site.

## Missing Abstraction

The original proposed abstraction was something like:

- a `case`
- applied to a model instance
- with a declared final action:
  - solve
  - integrate
  - study
- and structured output

That is, users should be expressing:

- what model to run
- what setup methods to invoke
- what variable overrides to apply
- what final action to take
- what outputs to capture

not how to sequence every low-level step.

## Proposed API Shape

The following `run_case` and `RunSpec` examples remain design sketches. The
implemented interfaces are documented under [Current Implementation](#current-implementation).

## 1. Core Concepts

### Run Spec

A structured run specification should include:

- model file / module
- model name
- optional run methods before final action
- variable overrides
- action type:
  - solve
  - integrate
  - study
- action options
- reporting options

### Variable Override

Overrides should support:

- scalar real assignment
- boolean / integer / symbol assignment where valid
- unit-aware values

Example conceptual form:

```text
T = 873.15 K
L_freeboard = 8.3 mm
m_sample_init = 40 mg
freeboard_height_multiplier = 0.7
```

### Result

A result object should expose:

- final status
- stdout/stderr or structured diagnostics
- final scalar values
- observed tables / trajectories
- partial output if integration failed after producing some rows

## 2. High-Level Operations

### Solve Case

```text
result = run_case(
    model="johnpye/iron/tga.a4c",
    type="tga_reduction_screen",
    methods=["values", "use_measured_bed_height"],
    overrides={...},
    action=solve,
)
```

### Integrate Case

```text
result = run_case(
    model="johnpye/iron/tgadyn.a4c",
    type="tga_reduction_screen_dyn",
    methods=["values", "bound_self", "dynamic_reset"],
    overrides={...},
    action=integrate(duration=600 s, steps=200),
    observe=[...],
)
```

### Study Case

```text
result = run_case(
    ...,
    action=study(vary=..., observe=...),
)
```

## 3. Important Semantics

### Methods Stay First-Class

ASCEND setup logic lives naturally in `METHOD`s. A good run API should not
try to replace them. It should compose:

- run methods
- direct overrides
- final action

cleanly and explicitly.

### Overrides Need A Clear Ordering

Default desired order:

1. instantiate
2. run `on_load` if requested
3. run listed setup methods
4. apply direct overrides
5. run optional post-override methods
6. final solve / integrate / study

The ordering must be part of the API, not hidden behavior.

### Dynamic Models Need Native Support

For modern dynamic models using:

- `INDEPENDENT`
- `der(...)`
- `INITIAL`
- `OBSERVE`
- `INTEGRATOR`
- `INTEGRATE`

the run API should not force users back down into low-level integrator wiring.

Shared integration helpers now address this part of the gap; they do not yet
replace every low-level integrator control used by bespoke drivers.

### Reporting Should Be Shared With CLI

The `./a4 run` path already has useful reporting semantics:

- observed tables
- TSV output
- typed values
- integration status
- optional microstate filtering

The implemented execution/result path now shares this machinery with the CLI.

## Likely Implementation Layers

These are the original candidate layers, not a required implementation sequence.
The current work uses ordinary Python arguments and existing ascpy operations;
it does not introduce the RunSpec or new C entry points sketched below.

## A. Immediate Python-Level Improvement

Add a high-level Python helper in `ascxx` or similar:

- `run_ascend_case(spec) -> result`

Internally this can still call existing solve/integrate machinery, but callers
stop hand-assembling the workflow.

The current implementation instead exposes `execute_model(...)` and
`execute_integration(sim, ...)` without requiring a spec object.

## B. Shared Internal "Run Spec"

Refactor `runmodel.py` so the CLI parser produces a structured run spec,
then passes that spec into shared execution code.

If done well:

- CLI uses it
- Python wrapper uses it
- tests use it

This would reduce duplication immediately.

## C. Lower-Level C/ascpy Support

The likely value of C-layer work is not "move everything from Python into C".
The value is to expose a few coarse reusable operations so Python and CLI code
stop stitching together the same low-level calls repeatedly.

If the current API surface does not expose the right hooks cleanly, add lower
level support for:

- applying unit-aware assignments by path
- running method sequences robustly
- collecting observed tables directly
- returning structured integration results

Add such C-level operations only where the existing bindings cannot support the
required semantics cleanly. A shared run-spec abstraction is not a prerequisite.

## Candidate Reusable C-Layer Operations

These are possible future functions, not implemented APIs. Much of the same
convenience is currently provided in Python over existing instance setters,
METHOD invocation and integrator operations.

### 1. Run A Method Sequence

Conceptually:

```text
Asc_RunMethods(sim, ["values", "bound_self", "dynamic_reset"])
```

This should:

- resolve methods by name
- run them in order
- return structured status
- capture which method failed

### 2. Apply Named Overrides

Conceptually:

```text
Asc_ApplyOverrides(sim, [
    ("T", 873.15, "K"),
    ("L_freeboard", 8.3, "mm"),
    ("m_sample_init", 40, "mg"),
])
```

This should:

- resolve variable instances by path
- apply unit-aware assignments
- reject writes to incompatible or non-assignable targets cleanly

This is one of the biggest current sources of repetitive boilerplate.

### 3. Build And Validate A Run Context

Conceptually:

```text
Asc_PrepareRun(sim, mode=integrate)
```

This would centralize the awkward pieces around:

- build / rebuild
- solver/integrator setup
- observed variable wiring
- compatibility checks for solve vs integrate

### 4. Execute A Final Action

Conceptually:

```text
Asc_ExecuteRun(sim, action_spec, reporter_spec)
```

where `action_spec` is one of:

- solve
- integrate
- study

and returns a structured result object.

### 5. Collect Structured Reports

Conceptually:

```text
Asc_GetObservedTable(run_result)
```

This should expose the same useful content already available through the CLI:

- columns
- units
- typed values
- integration status
- partial rows if available

### 6. One High-Level Entry Point Over The Above

Conceptually:

```text
Asc_RunCase(sim, run_spec) -> run_result
```

This should probably be built on top of the lower operations above, not
instead of them.

## What Should Stay In Python / CLI Space

Not everything belongs in the C layer.

Python/CLI is still the right place for:

- convenient case specification syntax
- CSV / JSON / CLI parsing
- optimization loops
- plotting
- result tabulation
- user-facing defaults and workflow policy

So the intended split is:

- C layer: reusable simulation operations
- Python layer: orchestration, usability, data integration

That is likely the cleanest balance.

## Why This Matters

Without a better run API:

- user scripts generate temporary wrapper `.a4c` files
- tests carry too much boilerplate
- dynamic workflows remain brittle outside the CLI
- model evolution breaks external tooling too easily

With a better run API:

- external tools can run structured cases directly
- dynamic calibration/optimization workflows become much cleaner
- test code becomes shorter and more declarative
- CLI and programmatic execution stay aligned

## Current Implementation

The hook/integrator groundwork is implemented: simulation configuration survives
separate METHOD calls, native and Python integration replay the selected engine's
options, and sample bounds use consistent unit conversion and initialization
timing. See [INTEGRATE.md](INTEGRATE.md#lifetime-of-options-and-observe-sets).

### Solver And Integrator Configuration

Observation lists, selected engines, saved options and option focus survive
separate METHOD calls on the same simulation. Fresh simulations start with fresh
configuration; clearing the library clears stored configurations.

Saved steady-solver options now follow the integrator's **clear-on-change** rule:

- Selecting the same solver preserves its saved options, including for replay
  after rebuilding the solver system.
- Successfully selecting a different solver clears the old saved options, so
  they cannot be replayed against a solver with different parameter definitions.
- Switching back does not restore historical settings; rerun the relevant
  configuration METHOD to apply model-recommended options again.
- Failed steady-solver selection leaves the last successful saved configuration
  intact. A subsequent METHOD `SOLVE` can reselect it; this is not a general
  rollback of live native solver state.

For integrations, native and Python paths replay options belonging to the
METHOD-selected engine. An explicit different `--engine` uses that engine's
defaults, not the other engine's saved options. Sample bounds are converted from
the supplied units to base units, inherited METHOD bounds retain their physical
meaning under partial CLI overrides, and initialization sees the requested
start time.

These rules isolate numerical settings, not physical state. A fair two-solver
comparison still requires equivalent initial values, FIX/FREE choices and
preparation. There is no per-solver profile history or fresh-case reset manager.

### CLI Overrides And Execution Order

`./a4 run` accepts repeatable `--set 'PATH=VALUE'`, `--setup-method METHOD` and
`--run-method METHOD` options. Execution order is fixed, independent of where
the flag groups appear on the command line:

1. Load and instantiate the selected concrete model.
2. Run `on_load`, unless `--no-on-load` is supplied.
3. Run all `--setup-method` METHODs, in their supplied order.
4. Apply all `--set` assignments, in their supplied order.
5. Run all `--run-method` METHODs, in their supplied order.
6. Perform the requested integration, or the usual implicit steady solve if
   needed. `--no-solve` disables that implicit steady solve and automatic
   `self_test`; explicit METHOD solves/integrations still execute, as does an
   explicitly requested CLI integration.
7. Capture requested outputs and run `self_test` when enabled; render the result.

For example, this repository fixture mirrors the kinetics driver's
defaults/assignment/dynamic-reset sequence without generating a case wrapper:

```sh
./a4 run models/test/ida/runapi_overrides.a4c \
  --model runapi_override_kinetics \
  --no-on-load --setup-method defaults \
  --set 'm_sample_init=40{mg}' --set 'tau=1{min}' \
  --run-method prepare_case --run-method check_prepared \
  --integrate --duration 60 --steps 4 --units s --output case.tsv
```

Alternatively, let the fixture's METHOD perform integration: replace the last
line with `--run-method integrate_case --no-solve`. Existing CLI compatibility is
preserved: `--output`, `--plot` and integration-specific flags still request
explicit CLI integration, which suppresses METHOD `INTEGRATE` requests and uses
their bounds as defaults. For exporting METHOD-only results without that policy,
use the Python execution and rendering functions separately.

If `on_load` already solves or integrates, overrides are **too late to affect that
execution**. Use `--no-on-load` and explicit setup METHODs in that case. Likewise,
a post-override METHOD that reapplies defaults can overwrite your values: choose
preparation/reset methods that preserve case inputs. No hidden reordering or
rollback of METHOD side effects is performed.

Override semantics:

- Real literals optionally include units, e.g. `T=873.15{K}`. Bare real numbers
  use base units, not display units. Conversion and dimension checking use ascpy.
  Values must be finite; expressions such as `2*3` are not evaluated.
- Booleans use `TRUE`/`FALSE`, integers use integer literals, and symbols/selectors
  use single-quoted literals, e.g. `--set "mode='high'"`.
- Paths support nested models and scalar integer/symbol array indices, e.g.
  `core.E_a[2]=80{kJ/mol}` or `feed['hydrogen'].p=2{bar}`. Python expressions,
  slices, ranges and bulk array assignments are not supported.
- Assignments do not implicitly FIX or FREE a variable. Use model METHODs for
  those choices; a solver can change the value of a free variable afterward.
- Structural constants cannot be overridden. Choose a parameterised/refined
  model with its structural constants already specified.
- Repeated paths use the last assignment. Errors identify the offending
  override and stop before post-override methods/final execution; earlier
  assignments are not rolled back.
- Qualified METHOD names, e.g. `core.dynamic_reset`, are supported. `--print`
  uses the same non-evaluating path resolver as overrides.

Python callers can use `execute_model(..., overrides=[...], setup_methods=[...],
runmethod=[...], run_on_load=False, solve=False)`. A single string is still
accepted for `runmethod`. For callers retaining their own simulation,
`runvalues.apply_overrides(sim, assignments)` provides the same literal
assignment helper without loading models, executing methods or taking ownership.

Regression coverage: `test/test_runapi_overrides.py`. The small dynamic fixture
is checked against an equivalent wrapper MODEL and an analytic trajectory. This
is not yet a validation of the full kinetics workflow: the iron models and
property C code live on `fboard2`, which needs its own matching build.

### Structured Results

The first structured-result layer is now implemented in `ascxx/runmodel.py` and
`ascxx/runresult.py`, using the existing ascxx/ascpy bindings:

- `execute_model(...) -> RunResult` runs the existing load, instantiate,
  `on_load`, optional METHOD, final action and optional `self_test` workflow.
- `execute_integration(sim, ...) -> RunResult` integrates a caller-owned
  simulation using its current METHOD configuration and observations.
- `render_run_result(result, ...)` handles table printing, TSV export and
  plotting without rerunning ASCEND or modifying the saved data.
- `run_ascend_model(...)` remains the CLI-compatible wrapper. It renders the
  same result, then raises `RunError` on failure; the exception retains `.result`.

For example, in a script launched with `./a4 script /usr/bin/python3 driver.py`:

```python
from runmodel import execute_model, render_run_result

result = execute_model(
    "models/test/ida/runapi_results.a4c",
    model="runapi_result_ok",
    integrate=True, start=0, duration=1, steps=4, units="s",
)
if not result.ok:
    for diagnostic in result.diagnostics:
        print(diagnostic.phase, diagnostic.message)

# Results remain usable after the native simulation/library is discarded.
record = result.as_dict()
render_run_result(result, output="observations.tsv")
result.raise_for_status()  # Optional: raise only after retaining/exporting data.
```

`RunResult` contains:

- `action`, `status` (`success` or `failed`), `phase`, and an `ok` convenience
  property;
- `diagnostics` with phase, severity, message and Python exception type;
- `tables`, one per integration, with typed column metadata and recorded rows;
- `values`, containing requested scalar values or default deferred STUDY outputs,
  with type and display-unit metadata;
- `simulation_status` and explanatory `notes`.

All returned data are Python scalars/containers, with no live ASCEND pointers.
Real-valued observations use their recorded display units; rows also contain
`time_raw` in base units. Boolean, integer, symbol and selector observations
retain their Python types rather than CLI formatting. All recorded event
microstates are retained; filtering belongs to presentation. `as_dict()` returns
a detached copy suitable for serialization.

If integration fails, already-recorded rows survive and `partial` is true when
a failed result contains rows. Reporter callback errors also mark a run failed,
even when a native engine ignores the callback's return code. A failure before
output starts has diagnostics but no rows. The CLI prints/exports partial data
and still exits nonzero. Multiple METHOD integrations retain separate tables;
the renderer rejects exporting multiple tables to one TSV path.

### Remaining Limits

This is an incremental execution API, not yet the proposed case API above:

- No `RunSpec`, METHOD parameters, or new study/batch orchestration yet.
- Native diagnostics and explicit METHOD output still go through ASCEND's
  existing reporter/output paths. Structured diagnostics currently capture
  execution exceptions and Python integration-reporter failures, not every
  native warning or error message. Optional progress output is unchanged.
- Loading and hook registration remain process-global. `execute_model` does not
  clear the caller's library or provide isolation/idempotent loading for repeated
  cases. It releases its solver system; model instances remain library-owned.
  `execute_integration` leaves the caller's simulation available for reuse.
- Thread-safe concurrent runs and wholesale ascxx/SWIG replacement are out of
  scope. Use separate processes when library/model isolation is required.

Regression coverage: `test/test_runapi_hooks.py` and `test/test_runapi_results.py`.

## Implementation Overview

The implementation separates command-line policy, instance operations, execution
and presentation. It adds no new case language and does not replace ascxx/SWIG.

| Component | Responsibility |
| --- | --- |
| [a4.in](a4.in) | Launcher argument parsing and forwarding for `run` and `int`; establishes the development environment. `a4` is generated from this file. |
| [ascxx/runvalues.py](ascxx/runvalues.py) | `resolve_instance` traverses native instance children and array elements without `eval`; `apply_overrides` parses scalar literals and delegates assignments and unit checking to ascpy. |
| [ascxx/runmodel.py](ascxx/runmodel.py) | Executes the documented setup/override/METHOD phases, wires reporters and temporary solver hooks, and supplies the CLI-compatible entry point. |
| [ascxx/runresult.py](ascxx/runresult.py) | Plain-Python `RunResult`, `RunDiagnostic` and `RunError`; no native handles or GUI dependencies. |
| [ascxx/solverhooks.cpp](ascxx/solverhooks.cpp), [ascxx/simulation.cpp](ascxx/simulation.cpp) | Preserve simulation-scoped METHOD configuration, rebind callbacks, replay options and enforce clear-on-change solver selection. |
| [ascxx/integrator.cpp](ascxx/integrator.cpp) | Native integrator setup, sample-unit conversion and initialization at the requested first sample. |

### Execution And Reporting

`execute_model` is the CLI-oriented convenience path: it loads and instantiates
a model, installs temporary `CliSolverHooks`, performs the requested phases and
collects a result. Both explicit CLI integration and METHOD-requested integration
reach `_execute_integration`. `CliIntegratorReporter` captures typed rows and
column metadata; recorded data are snapshotted even when integration fails.
Callback exceptions become diagnostics rather than escaping through native
integrator callbacks. Multiple METHOD integrations retain separate tables.

`render_run_result` applies presentation-time microstate filtering and delegates
to table, TSV and plotting functions. `run_ascend_model` combines execution and
rendering, then raises `RunError` if the result failed. The CLI turns that into
a nonzero exit status, even if partial data were exported. Model lookup failures
retain exit code 2; other execution failures use exit code 1.

Native diagnostics and explicit METHOD output are not redirected into a complete
log collector. These APIs do not guarantee silent execution or capture every
native warning. Structured STUDY sweep tables are also not collected by the
current result layer; the native STUDY path still has its own reporting.

### Ownership And Escape Hatches

There are deliberately two ways to use the helpers:

- **CLI-style ownership:** `execute_model` manages its temporary execution
  machinery, restores the previous global solver hooks and invalidates its
  solver system after collecting output. It returns a snapshot, not the live
  simulation, and does not clear the process-global library.
- **Caller-owned simulation:** use existing ascpy loading/instantiation/METHOD
  APIs, optionally call `apply_overrides(sim, ...)`, then
  `execute_integration(sim, ...)`. The caller retains the simulation and can
  inspect or manipulate any instance, customize low-level integration instead,
  or perform further operations. The caller remains responsible for lifecycle
  and reset choices.

`RunResult` is optional reporting infrastructure, not a replacement for the
instance tree. Its additional value is a consistent typed, unit-labelled history
and failure snapshot that outlives native state. Final instance values and the
low-level integrator observation APIs remain available. Neither an elegance
cleanup of the bindings nor a new `Simulation` convenience-method API is required
to use this work; those remain possible follow-ups.

### Regression Coverage

- [test/test_runapi_hooks.py](test/test_runapi_hooks.py): configuration across
  METHOD calls, engine/options selection, time-unit conversion, inherited bounds
  and initialization time.
- [test/test_solver_option_scope.py](test/test_solver_option_scope.py): preserving
  same-solver options, clearing changed-solver options, switching back to defaults
  and recovery of saved configuration after failed selection. Uses HiGHS for
  cross-solver checks when available, without requiring CONOPT.
- [test/test_runapi_results.py](test/test_runapi_results.py): detached typed
  results, real IDA partial failure, reporter failures, multiple integrations,
  microstate rendering, export errors and CLI failure exit status.
- [test/test_runapi_overrides.py](test/test_runapi_overrides.py): literal and unit
  validation, safe paths, constants, FIX/FREE preservation, qualified METHODs,
  launcher forwarding, execution ordering and wrapper/analytic equivalence.

These tests use models under `models/test`; they do not require the `fboard2`
model/property library. The full-suite result above is a recorded validation
checkpoint, not a claim that every optional solver or kinetics model was tested.

## Tricky Use Cases

These cases are design checks for future conveniences, not reasons to introduce
a general workflow engine now. Keep model structure, physical inputs,
preparation, numerical configuration, outputs and state-reuse policy distinct.

### Same Physical Case, Two Different Solvers

Keep structure, physical inputs, FIX/FREE choices and initial values equivalent;
change only the intended numerical configuration. Model-authored METHODs such as
`configure_solver_a` and `configure_solver_b` can select their solver and set
its `OPTION`s. They should not secretly reset physical inputs or execute solves.
The existing repeatable `--run-method` option can invoke the chosen configuration
after preparation, before the final action.

The implemented clear-on-change rule prevents saved options from solver A being
replayed against solver B. It does **not** reset solution values: running B after
A on the same live instance may warm-start B from A's solution. Use fresh
instances or an explicitly equivalent reset for an independent comparison;
continuation is a different, sometimes useful experiment.

Recommended future precedence is solver factory defaults, then model-recommended
configuration, then explicit run-specific numerical overrides. A generic profile
registry or solver-option override map is not implemented; `--set` assigns model
instances, not solver parameters. Different configurations of the same solver
must also account for unspecified options surviving same-solver reselection.
Intermediate initialization solves may need their own solver: a future final-
action override must not blindly replace every solver used inside a METHOD.

### Varying The Number Of Finite Elements

Element counts that determine arrays/equations are structural constants. Express
them through parameterised models or explicit derived MODELs assigning `:==`
constants, and instantiate the appropriate concrete variant. CLI `--set` cannot
resize an existing instance or change those constants; direct parameter binding
at CLI instantiation is not implemented.

Apply the same numerical case inputs and preparation to each variant. Report
mesh-independent quantities where possible: totals, boundary values, or values
interpolated to fixed physical coordinates. An element index is not necessarily
the same physical location across meshes. Transferring an initial solution
between meshes requires an explicit interpolation/conservation policy; neither
ordinary value overrides nor a generic reset can infer it.

Structural wrappers remain legitimate here. The aim is to avoid generating a new
wrapper for every numerical parameter change when structure is unchanged.

### Parameter Sweep With Selected Outputs

An ordinary steady sweep already has METHOD syntax:

```ascend
STUDY y VARY x FROM 1 TO 10 STEPS 9;
```

The current native hook runs an optional `RUN` METHOD **before** assigning each
new varied value, then performs a steady solve and writes the output row. This
is not the same recipe as assigning inputs, recomputing dependent initial state,
integrating and extracting a trajectory measurement. Do not assume the existing
STUDY action provides that dynamic-case workflow or structured per-case failures.

For the latter, a Python driver can invoke the same preparation/override/execution
sequence for each case, select observed outputs and compute measurements such as
peak temperature or time to a conversion threshold. Independent-case resets
versus warm-start continuation, failure/partial-output policy, and output units
must be explicit. Optimizer objectives, scheduling and retry policy can stay in
Python; no additional sweep syntax is required to test the conveniences.

### Inner Simulation Used By A Higher-Level ASCEND Model

Distinguish a procedural METHOD that invokes another run from an equation-level
external function `outputs = F(inputs)`. A future RunSpec might describe either,
but simply wrapping `execute_model` would not make it safe for use inside an
outer solver.

ASCEND's existing [black-box interface](ascend/compiler/extfunc.h) provides input/
output arrays, derivative callbacks and per-occurrence state. Adapting an inner
simulation to it would additionally require:

- Explicit input/output types and dimensions, a fixed output shape and a defined
  mapping from the inner model to those outputs. Mesh size is normally fixed
  configuration, not a continuous outer-solver input.
- Repeatable evaluation under arbitrary outer-solver call order, including a
  defined reset and solution-branch policy. Warm-start caches must not silently
  turn `F(inputs)` into a history-dependent operation.
- Inner convergence and failure rules. Partial data can aid diagnostics, but an
  unsuccessful inner solve must not silently supply valid-looking function values.
- Derivatives or a suitable approximation strategy. Finite differences can be
  expensive/noisy when inner convergence errors dominate the perturbations;
  event-driven trajectories may also be nonsmooth.
- Verified ownership, cleanup and re-entrancy/isolation of native solver,
  library and hook state. Current process-global mechanisms do not establish
  that nested in-process execution is safe. An isolated worker is a possible
  design, not an implemented adapter, and has its own cost.

This is future numerical-composition work, not delivered by the current CLI
conveniences or result classes. Only suitably constrained run definitions should
be eligible as equation-level external functions.

### Where METHOD Parameters, NOTES And RunSpec Might Fit

METHOD parameters could make operation inputs explicit and checked, allowing the
model to own defaults, assignment, preparation and execution in the right order.
They are not currently supported: implementation would need argument binding,
call-local lookup, type/dimension rules and inheritance semantics. They would
not replace structural model parameters. Ordinary input atoms or a model-owned
input submodel can test that interface shape before adding syntax.

NOTES can describe input mappings, units, recommended METHODs and output groups;
the kinetics driver's `fit.contract` is an existing example. No executable Python
in NOTES or new metadata interpreter is added here. Code extensions should be
explicit, not an automatic side effect of discovering descriptive metadata.

If useful after migration, a small RunSpec could record the concrete model
variant, input bindings, METHOD invocation and numerical/reporting choices for
reproducibility. It need not duplicate the METHOD body or restrict access to the
live simulation. None of these proposals requires replacing ascxx/SWIG first.

## Immediate Next Step

Validate the CLI conveniences against a real fixed-structure kinetics case in a
separate `fboard2` worktree with its matching model/property-code build. Compare
the existing generated-wrapper workflow with direct overrides and model-owned
preparation methods before migrating the driver. Preserve structural refinement
wrappers where needed and subprocess isolation where appropriate. Use that
experience to decide whether METHOD parameters or a small invocation manifest
add value; do not make a generic `RunSpec` a prerequisite.

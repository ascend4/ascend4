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

## Current Problem

At present, there are four recognizable paths:

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
   - but not architecturally clean

This is a smell. It suggests a missing first-class case-running API.

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

The missing abstraction is something like:

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

This is one of the main current gaps.

### Reporting Should Be Shared With CLI

The `./a4 run` path already has useful reporting semantics:

- observed tables
- TSV output
- typed values
- integration status
- optional microstate filtering

The programmatic API should reuse the same machinery, not reimplement it.

## Likely Implementation Layers

## A. Immediate Python-Level Improvement

Add a high-level Python helper in `ascxx` or similar:

- `run_ascend_case(spec) -> result`

Internally this can still call existing solve/integrate machinery, but callers
stop hand-assembling the workflow.

This is probably the fastest path to user value.

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

But this should serve the shared run-spec abstraction, not become a new
collection of ad hoc calls.

## Candidate Reusable C-Layer Operations

These are the kinds of functions that seem worth exposing once and then using
from both Python and CLI code.

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

## Immediate Next Step

The first practical move would be:

1. define a Python-side `RunSpec` / `RunResult`
2. refactor `runmodel.py` execution into a reusable function over that spec
3. identify the lowest-level repeated operations in `runmodel.py`
4. decide which of those deserve direct C/ascpy helpers
5. migrate one real workflow to the resulting API
   - the TGA batch/optimization tooling is a good candidate

That would let us test the abstraction before making any larger C-layer change.

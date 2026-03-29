# Solver and Integrator Event Design Notes

## Purpose

This note records the intended design direction for structured event reporting
from ASCEND solvers and integrators.

The immediate driver is IDA hybrid/event debugging:

- event-time state transitions
- `REINIT(...)` application
- selector/discrete changes
- root/guard triggers
- same-time event iteration detail

But the intended scope is broader than hybrid DAE work. The same machinery
should also be able to carry:

- solver progress updates
- convergence/failure diagnostics
- MIP progress and incumbents
- nonlinear solve/optimisation iteration summaries
- bound and consistency diagnostics

This note is separate from:

- [`HYBRID.md`](HYBRID.md), which covers hybrid DAE semantics
- [`INTEGRATE.md`](INTEGRATE.md), which covers METHOD/CLI integration language

## Why This Is A Separate Note

`HYBRID.md` should stay about model/runtime semantics:

- what `pre(x)` means
- how `REINIT(...)` works
- how selector state and `SWITCH TO ... IF ...` behave

The event-reporting problem is different. It asks:

- what runtime facts should be surfaced
- how they should be transported
- how they should be buffered or streamed
- how frontends should consume them

That problem is not specific to IDA forever, and it is not purely an
integration-language issue either.

## Current Position

ASCEND already has several partial precedents, but no general structured event
stream:

- `error_reporter` / `error_reporter_tree`
  - good for buffered message output
  - not typed solver-event reporting
- `slv_report_progress` / `SolverReporter`
  - good for streaming progress-style text updates
  - currently text-oriented and aimed at progress/polling UI use
- `slv_status_t`
  - good for current structured status snapshot
  - not an event history
- `IntegratorReporter`
  - good for observed rows and integration status callbacks
  - not an event payload interface
- HiGHS callbacks
  - already provide a native callback source plus structured latest-status data
- IDA hybrid trace/debug helpers
  - already expose useful event-time data for discrete/`WHEN` state
  - currently ad hoc and stderr-oriented

So the intended work is new, but should reuse those pieces where sensible.

## Draft API Sketch

A first concrete C-side sketch now exists in:

- [`ascend/solver/event.h`](./ascend/solver/event.h)

That header is not yet wired into solver/integrator runtime code. Its purpose
is to make the proposed event types and payload structures concrete enough for
review before implementation details harden.

## Main Design Goals

The first pass should be:

- structured rather than text-only
- opt-in
- cheap when disabled
- usable live by CLI/GUI/frontends
- simple enough to implement without taking on a large state-capture project
- reviewable as plain text by a modeller

The first target is not a universal perfect taxonomy. The first target is a
useful IDA event stream with a shape that later solvers can also adopt.

## Relationship To `slv_report_progress`

`slv_report_progress` is still useful.

Its current role is best understood as:

- progress-bar or live-status updates
- convergence metrics
- terse human-readable runtime messages

That overlaps with the proposed event API, but it is not the same thing.

Current working distinction:

- `slv_report_progress`
  - lightweight text progress path
  - good for periodic solver heartbeat/progress lines
- event stream
  - structured typed records
  - good for diagnostics, postmortem review, plotting/annotation, and testing

It should be possible for some event sinks to derive progress text from events,
or for a solver to keep using `slv_report_progress` in parallel for coarse UI
feedback.

## Core Idea

Introduce a structured ASCEND-side event stream:

- emitted by solvers/integrators
- consumed by one or more sinks
- optionally buffered
- optionally streamed

The first-pass abstraction should be simpler than the earlier sketch:

- event-type enum
- lightweight event record
- optional object references
- attachable event handler callback
- simple preferences for enabling default textual logging

The solver should emit structured event records, but should not take on
full state capture in the first pass.

The current draft header reflects that simpler shape:

- `asc_event_type_t`
- `asc_event_ref_t`
- `asc_event_t`
- `AscEventHandlerFn`
- `asc_event_prefs_t`

## Event Families

### 1. Configuration

These describe setup/runtime configuration changes:

- solver selected
- integrator selected
- option changed
- system rebuilt / reanalysed

### 2. Integrator State

These describe integration progress and state-machine activity:

- integration started
- step accepted
- step rejected
- root found
- event iteration started
- event iteration ended
- consistency solve started/ended
- restart/reinit
- integration failed
- integration completed

### 3. Hybrid / Discrete Model Events

These are the most important first-class IDA events:

- selector/discrete value changed
- `REINIT(...)` applied
- guard/root triggered
- `WHEN` active case changed
- event microstate emitted

### 4. Solver Diagnostics

These are solver-agnostic diagnostic/failure events:

- nonconvergence
- residual evaluation failure
- linear solve failure
- variable out of bounds
- inconsistent system detected
- iteration limit exceeded
- time limit exceeded
- interrupt/abort

### 5. Optimisation / MIP Progress

These are solver-specific but still useful under one framework:

- incumbent updated
- best bound updated
- node milestone
- gap milestone
- objective improvement
- optimisation terminated with reason

## Common Event Record

Every event should carry only a small amount of common data:

- `type`
- `sequence_no`
- `engine_name`
- `sim_time` if meaningful
- `iteration`
- `subiteration`
- optional short human-readable message
- optional primary/secondary references
- a few generic integer slots for simple ids such as root index or case index

This is enough for first-pass review/debugging without trying to encode full
solver state in the event record itself.

## References And Naming

Where an event refers to a model object, it should carry references in a form
usable both by code and by users.

Preferred pattern:

- internal pointer or index for efficient in-process handling
- optional resolved path/name for human display and file output

Examples:

- variable reference
- relation reference
- `WHEN` reference
- guard/root reference
- solver block reference

For IDA hybrid debugging, useful existing helpers already exist:

- discrete names via `dis_make_name`
- `WHEN` names via `when_make_name`

## Minimal Event Payload Expectations

### IDA root / guard event

Useful first-pass fields:

- root index
- crossing direction
- associated boundary or guard

### IDA discrete transition

Useful first-pass fields:

- discrete/selector reference
- optional short message naming the transition

### `REINIT(...)` event

Useful first-pass fields:

- target reference
- source statement/action reference if available

### Event microstate

Useful first-pass fields:

- event time
- event iteration index
- consistency-solve phase label

### Bound / diagnostic event

Useful first-pass fields:

- variable reference
- short human-readable message

### HiGHS progress event

Useful first-pass fields:

- callback kind or milestone tag
- optional short human-readable message

## State Capture

The first pass should not try to capture state snapshots alongside events.

Rationale:

- ASCEND already has `OBSERVE` and trajectory reporting
- users can choose what values to log separately from which events occurred
- avoiding state capture keeps the event system smaller and easier to review

Richer state attachment can be revisited later if a clear need remains.

## Enabling And Output

The first pass should use simple preferences, not domain/mask filtering in the
event record itself.

Working direction:

- events are either enabled or disabled for the solver/integrator instance
- a handler callback may be attached explicitly
- if no explicit handler is attached, the default sink is a plain-text file
- if no log path is given, a temp file should be created under `/tmp`

For implementation, a safe temp-file API is preferred over `tmpnam`.

## Relationship To `microstates`

Current `microstates` support in IDA should be seen as the first narrow piece
of the larger event-story.

Intended long-term meaning:

- `none`
  - suppress event-time state rows
- `endpoints`
  - report only left/right event endpoints
- `all`
  - report every same-time event-iteration state

But a true event-trace should carry more than rows:

- which root/guard fired
- which selector/discrete changed
- which `REINIT(...)` actions applied
- what changed from/to

So `microstates` should become one view policy over the IDA event stream, not
the whole feature.

## Relationship To `slv_status_t`

Some current `slv_status_t`-style reporting can be complemented or partially
offloaded to events.

Likely split:

- keep `slv_status_t` as the current/latest snapshot API
- use events for history and transitions

For example, HiGHS may continue to maintain `slv_status_t`, while also
emitting progress and milestone events. Frontends can then use:

- `slv_status_t` for the latest status panel
- event stream for trace/log/history views

That is a better fit than trying to force all temporal information into
`slv_status_t`.

## Attachment Points

The first implementation should provide explicit attachment points for event
handlers on:

- solver instances
- integrator instances

The current draft header sketches:

- `slv_set_event_handler(...)`
- `integrator_set_event_handler(...)`
- `slv_set_event_prefs(...)`
- `integrator_set_event_prefs(...)`

Those are not implemented yet, but they reflect the intended shape.

## IDA-First Rollout

### Phase 1

Implement structured IDA events only:

- root found
- discrete/selector transition
- `REINIT(...)` applied
- event iteration start/end
- event microstate
- integration fail diagnostics

Provide:

- attachable event handler API
- default plain-text file sink
- temp-file default behavior when no path is specified

### Phase 2

Add generic cross-solver diagnostic events:

- option changes
- nonconvergence
- bound violations
- inconsistent-system detection
- termination reason

### Phase 3

Add solver-specific progress events:

- HiGHS MIP/LP progress
- nonlinear optimisation iteration summaries
- CMSlv/QRSlv-specific diagnostics if useful

## Existing Reusable Pieces

The first implementation should try to reuse:

- IDA discrete snapshot logic
- IDA hybrid trace naming helpers
- `slv_report_progress` as optional coarse progress path
- `slv_status_t` as current-status storage
- `IntegratorReporter` as one possible consumer/sink layer

## Open Questions

- Should event sinks be global, per-simulation, or per-solver/integrator instance?
- How much pointer/index information is safe or useful to expose outside the process?
- Should event-name/path resolution happen eagerly or lazily?
- How much of HiGHS progress should move from text progress messages to structured events?
- Which diagnostics should stay as ordinary error/warning messages even after event support exists?

## Working Recommendation

Create a new event-reporting layer centred on structured event records.

Do not fold this into `HYBRID.md`.

`HYBRID.md` should describe hybrid semantics.
`INTEGRATE.md` should describe user-facing integration/reporting controls.
`EVENT.md` should own the event transport, payload, buffering, and sink design.

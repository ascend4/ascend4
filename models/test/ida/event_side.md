# IDA event-side regression

[event_side.a4c](event_side.a4c) reproduces an incorrect branch selection after
an IDA root crossing. It requires only the standard ASCEND model libraries,
IDA and LRSlv; no particle model, kinetics data or Python generator is needed.
The native assertions are in
[test_ida.c](../../../ascend/integrator/test/test_ida.c).

Each state starts at zero and has velocity 2 m/s when its Boolean `on[i]` is
true, and 1 m/s otherwise. Its single event occurs at
`tau[i] = 0.1 + i*1e-7` seconds. Initial Booleans are explicitly set to the
correct pre-event values. Four separate models test `>`, `>=`, `<` and `<=`;
keeping them separate prevents another event from repairing a stale Boolean.
Two additional models contain 25 independent rising crossings, spaced 100 ns
apart. Each condition crosses once, so the cluster is finite.

With time expressed in seconds and distance in metres, the exact solutions are:

- Rising condition: `x[i](t) = t + max(t - tau[i], 0)`.
- Falling condition: `x[i](t) = t + min(t, tau[i])`.

At 1 s the isolated rising tests must give 1.8999999 m and `on[1] = TRUE`;
the falling tests must give 1.1000001 m and `on[1] = FALSE`. Inclusion of the
single equality point cannot change the integrated trajectory. All completed
runs check time, every state (absolute tolerance 1e-8 m), every final Boolean,
and absence of reported errors. IDA uses `rtol = 1e-9`, vector absolute
tolerances of 1e-10 m, and ten output intervals over 0 to 1 s.

The cluster tests separate two issues:

- `event_side_cluster_limit` explicitly sets `zeno_ncycles = 20` and
  `zeno_duration = 1e-4`. It expects a nonzero solve result with an
  `Event accumulation detected` diagnostic, exercising the existing global
  count guard. This is a guard-behaviour control, not an assertion that a
  finite cluster is physically Zeno. Revisit it if the guard policy changes.
- `event_side_cluster_strict` and `event_side_cluster_inclusive` use a finite
  limit of 200 with the same time window. Both must complete with the exact
  solution. Increasing the event budget must not conceal a wrong branch.
  The model files use this larger budget; the C guard control overrides it.

Build and run from the ASCEND repository root, with CUnit and the IDA/LRSlv
plugins available:

```sh
scons -j6 test/test
LD_LIBRARY_PATH="$HOME/.local/lib:.:${LD_LIBRARY_PATH:-}" test/test --list-failures \
  integrator_ida.event_side_rising_strict \
  integrator_ida.event_side_rising_inclusive \
  integrator_ida.event_side_falling_strict \
  integrator_ida.event_side_falling_inclusive \
  integrator_ida.event_side_cluster_limit \
  integrator_ida.event_side_cluster_strict \
  integrator_ida.event_side_cluster_inclusive
```

The local library path accommodates a user-installed CUnit. Check that no
plugins were reported unavailable. The existing test loader can return early
when a required plugin is absent.

For a standalone model run using the command-line frontend:

```sh
./a4 run models/test/ida/event_side.a4c \
  --model ida_event_rising_strict \
  --integrate --duration 1 --steps 10 --units s \
  --output /tmp/ida-event-side.tsv
```

The other model names are `ida_event_rising_inclusive`,
`ida_event_falling_strict`, `ida_event_falling_inclusive`,
`ida_event_cluster_strict`, and `ida_event_cluster_inclusive`.
A zero frontend exit status is insufficient: compare the final values with
the exact solution above. Set `ASCEND_HYBRID_TRACE=1` to inspect the root and
logical event iteration on builds supporting that trace.

Baseline on `fboard2` before the fix (2026-09-18): the three tests
`event_side_rising_strict`, `event_side_falling_inclusive` and
`event_side_cluster_strict` fail their solution/Boolean assertions. The other
four pass. The isolated failing states end at 1 m and 2 m respectively.
For the strict cluster the first 24 states switch one event late and the
last stays at 1 m. The native test command returns nonzero (35 on this build).
The existing `multi_boundary_same_direction`, `reinit_boolean_latch` and
`reinit_boolean_cascade` tests also pass: ten tests run, seven pass, three
fail, none skipped.

The acceptance target is that all seven new tests pass while the existing
hybrid-event tests continue to pass. These tests assert correct behaviour;
they deliberately fail on the affected integration layer. They do not prove
a defect in SUNDIALS IDA itself. No solver implementation change accompanies
them.

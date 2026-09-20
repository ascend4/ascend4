# IDA event-side regression

[event_side.a4c](event_side.a4c) reproduces an incorrect branch selection after
an IDA root crossing. It requires only the standard ASCEND model libraries,
IDA and LRSlv; no particle model, kinetics data or Python generator is needed.
The native assertions are in
[test_ida.c](../../../ascend/integrator/test/test_ida.c).

In the original seven tests, each state starts at zero and has velocity 2 m/s when its Boolean `on[i]` is
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

The original tests assert correct behaviour and deliberately fail on the
affected integration layer. They do not establish a defect in SUNDIALS IDA
itself.

## Repair and additional coverage

The repair retains a separate post-crossing truth value for each boundary
through same-time logical and consistency iteration. Inequality orientation
and IDA's crossing direction select that value. LRSlv passes per-relation
values to the logical evaluator, so simultaneous TRUE and FALSE targets work,
including both terms in one logical expression. Ordinary SATISFIED evaluation
and CMSlv's inversion mode are unchanged.

A reset or consistency solve releases an override when the residual leaves
its event-local roundoff band. That band is the absolute residual at IDA's
returned root plus 32 machine epsilons times the absolute relation nominal
(with scale 1 if that nominal is zero). It does not use the SATISFIED tolerance:
a small, resolved reset must still take effect. All overrides and their storage
are cleared at event exit, including failure paths.

The returned root state is installed before logical solving. Both root
crossing directions remain enabled after restart; the former persistent
direction filter could suppress a later opposite crossing. Direct-guard
indices are read before reanalysis changes the active guard list. Restart
and consistency failures propagate to the integration caller.

The fixture now supplies 19 native tests in total:

- The original seven isolated-crossing and finite-cluster tests.
- One simultaneous mixed-target test, including a compound logical expression.
- Four tests crossing the same boundary in opposite directions at 0.2 and
  0.4 seconds. Final positions are 1.2 m for `>`/`>=`, and 1.8 m for `<`/`<=`.
- Four reset tests using an algebraic boundary `z = 2*y`. They check four
  resets, the final continuous/algebraic state and the cleared Boolean.
- Three reset variants covering a reset smaller than the SATISFIED tolerance,
  an omitted tolerance and an explicit zero tolerance.

The reset tests use automatic initial step selection with their tight DAE
absolute tolerances. Their reset counter also detects duplicate or missed
applications of REINIT.

## Bouncing-event checks

The older `boundary` test incorrectly expected the spring/damper ball to be
at equilibrium at 30 seconds. The repaired solver keeps detecting departures
from contact, and the ball is still bouncing then. Its expectation now uses
an independent piecewise analytical solution with tighter numerical tolerances.
For the fixture parameters, free flight is a parabola under gravity 9.8 m/s².
During contact, with elapsed contact time `s`, position is

```text
y(s) = 9.902 + exp(-0.5*s) * (A*cos(w*s) + B*sin(w*s))
w = sqrt(99.75)
A = 10 - 9.902
B = (v_entry + 0.5*A)/w
```

Start with the first impact at `sqrt(60/9.8)` seconds. Alternate contact
until its first ascending return to `y = 10`, then free flight until the next
impact. This gives `y(30) = 10.09871453998119 m` and
`v(30) = -1.17307793252196 m/s`.

The ideal-rebound accumulation test now explicitly uses five events within
0.1 seconds and requires the accumulation diagnostic. This detects the
shrinking bounce sequence while its excursions are numerically resolved.
No default event-count setting or guard algorithm was changed. This check
does not guarantee detection of arbitrarily small unresolved bounces.

## Validation and branch handoff

On `branch-crossing-error-ida`, based on `python3`, the following command
passed all 85 tests and 2073 assertions, with no skipped tests or retained
allocations reported by the test memory tracker (2026-09-18):

```sh
scons -j6 test/test solvers/ida/libida_ascend.so solvers/lrslv/liblrslv_ascend.so
LD_LIBRARY_PATH="$HOME/.local/lib:.:${LD_LIBRARY_PATH:-}" test/test --list-failures \
  integrator_ida solver_lrslv solver_cmslv \
  compiler_instantiate_logrel_bool_algebra \
  compiler_instantiate_relation_logrel_bool
```

Use these native tests for the feature-branch acceptance check. This branch's
`a4 run --integrate` path does not transfer METHOD integrator options into its
new integrator. Consequently a standalone cluster run can still stop at the
default count of 20 despite the fixture requesting 200. The native tests
explicitly apply their parameters and verify the cluster solution. The
frontend option-transfer difference is outside this solver repair.

U1/U2/U3 qualification remains pending until this fix is merged into `fboard2`,
which contains the required iron models and frontend configuration handling.
No scientific qualification follows from failed loading attempts on the
`python3`-based feature branch. After merging, replay the analytical suite,
U1 inventory/throughput controls, U2 one-cell controls, then U3 bed grids with
a documented finite event allowance. The later TGA corrector failures still
require that replay; their cause has not been established by these tests.

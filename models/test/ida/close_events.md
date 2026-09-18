# Nearby IDA roots and SATISFIED tolerance

This is the follow-up to [event_side.md](event_side.md), reproduced on
`fboard2` commit `b7aa049e` after the same-event directional fix. It needs
only standard ASCEND models, IDA and LRSlv; no kinetics repository, Python
generator, experimental data or private model library is required.

## Problem and exact solution

[close_events.a4c](close_events.a4c) contains three independent states with
speeds 1 or 2 m/s, switched by monotone time guards at
`tau[i] = 0.1 + i*1e-8` seconds. All states start at zero.

- Rising strict: `t > tau[i]`, initially false;
  `x[i](t) = t + max(t - tau[i], 0)`.
- Falling inclusive: `t <= tau[i]`, initially true;
  `x[i](t) = t + min(t, tau[i])`.

With SATISFIED tolerance `1e-6`, each later root can revert the previously
crossed guard through ordinary logical evaluation. That earlier guard never
crosses its raw root again and remains in the wrong branch. The current
same-event override does not protect it at the later event.

At 1 s, the raw-root solutions and measured faulty results are:

| Case | Expected x[1], x[2], x[3] (m) | Faulty result (m) |
| --- | --- | --- |
| Rising strict | 1.89999999, 1.89999998, 1.89999997 | 1.00000001, 1.00000001, 1.89999997 |
| Falling inclusive | 1.10000001, 1.10000002, 1.10000003 | 1.99999999, 1.99999999, 1.10000003 |

Integration reports success with approximately 0.9 m error. Otherwise
identical fixtures using real literal `0.0` for SATISFIED tolerance are
controls and give the correct trajectories.

The native tests in [test_ida.c](../../../ascend/integrator/test/test_ida.c)
check successful integration, no solver error reports, time, all three
states and all three Booleans. Each fixture runs independently to 0.2 s
(after the cluster) and 1 s. Wide-tolerance tests allow 2e-6 m error around
the raw-root solution, deliberately admitting a consistently shifted
SATISFIED root surface as well. Zero-tolerance controls require 1e-8 m.
Neither interpretation admits a permanent stale branch. The tests assert
correct behaviour, so the two wide-tolerance tests are expected to fail on
the affected implementation; this is not an expected-failure mask.

## Build and run

From the ASCEND repository root, with CUnit and IDA/LRSlv installed:

```sh
scons -j6 test/test
LD_LIBRARY_PATH="$HOME/.local/lib:.:${LD_LIBRARY_PATH:-}" test/test --list-failures \
  integrator_ida.close_event_rising_strict \
  integrator_ida.close_event_falling_inclusive \
  integrator_ida.close_event_rising_zero_tol \
  integrator_ida.close_event_falling_zero_tol
```

Check that plugins loaded: the existing loader can return early when a
required plugin is unavailable. Native execution applies integrator options
explicitly and does not require the Python frontend bindings.

For a standalone frontend reproduction:

```sh
ASCEND_HYBRID_TRACE=1 ./a4 run models/test/ida/close_events.a4c \
  --model ida_close_rising_strict \
  --integrate --duration 1 --steps 10 --units s \
  --output /tmp/ida-close-events.tsv
```

The other models are `ida_close_falling_inclusive`,
`ida_close_rising_strict_zero_tol`, and
`ida_close_falling_inclusive_zero_tol`. A zero exit status alone does not
mean the numerical solution is correct. After switching branches, rebuild
`ascxx/_ascpy.so` and `a4` if using the frontend: stale generated bindings
previously prevented model integrator options from being applied.

## Repair scope

The relevant interaction spans `solvers/ida/idaboundary.c`,
`solvers/ida/ida.c`, `solvers/lrslv/slv9a.c`, and
`ascend/compiler/logrel_util.c`. The ordinary SATISFIED evaluator requires
residual greater than the tolerance for strict `>`, whereas `<=` remains
true while residual is less than the tolerance. IDA locates the unshifted
zero. The previous fix preserves crossing truth through the current event
iteration only; a later event uses ordinary evaluation for earlier guards.

Choose consistent hybrid inequality semantics: either locate the
SATISFIED-shifted surface or evaluate hybrid guards consistently against the
raw signed surface/crossing history. Do not enlarge the same-event override
band indiscriminately: resolved small REINIT resets must still take effect.
Persistent state must not suppress reverse crossings or unrelated state
resets. A larger event-count allowance cannot repair the wrong branch.

Keep the existing event-side, mixed-direction, recrossing, small/default/zero
SATISFIED reset, root-at-output, reinitialisation-error and event-accumulation
regressions. Extend coverage during repair to an unrelated later event and
a reverse recrossing while nearby guards are involved. These four fixtures
isolate the monotone-root defect; they do not replace those broader controls.

The same logical reversions were observed during phase appearance in a
multi-shell TGA model (`n/n0 > 1e-12`, SATISFIED tolerance `1e-12`). The
reproducer establishes an ASCEND hybrid/root-logical inconsistency, not a
SUNDIALS defect or the cause of every late TGA corrector failure. All fixture
and native-test changes here leave solver implementation unchanged.

## Verified baseline, 19 September 2026

The native build succeeds on `fboard2` `b7aa049e` with only these regression
additions. Eleven selected tests run: the two wide-tolerance close-event
cases fail, both zero-tolerance controls pass, and seven existing controls
pass (`event_side_rising_strict`, `event_side_falling_inclusive`,
`event_side_reset_small`, `event_side_reset_default_tol`,
`event_side_reset_zero_tol`, `event_side_repeated_gt`, and
`event_side_repeated_le`). No tests are skipped. There are 511 assertions,
16 failing state/Boolean assertions confined to the two new reproducer
cases; the process exits 35. Both integration calls themselves return
success without solver errors. The native harness reports no leaked memory.

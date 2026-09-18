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
(after the cluster) and 1 s. All trajectory checks now require 1e-8 m agreement with the raw-root
solution, including wide-tolerance fixtures. This pins down the selected
IDA semantics. The original reproducer allowed 2e-6 m while the choice
between raw and shifted roots was still open. The two wide-tolerance tests
fail on the affected implementation and pass with the repair below;
failures are not masked as expected failures.

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

## Implemented repair: IDA-mode ordered guards

In `solvers/lrslv/slv9a.c`, the existing `withida` mode now supplies explicit
truth values for **all ordered boundary relations** during each logical
solve, not only those crossed in the current event. For a current crossing,
the directional override still takes priority. For every other ordered
guard, `bndman_calc_satisfied` evaluates the signed residual against zero,
including strict/inclusive behaviour at equality. That system-level routine
already ignores tolerance for ordered relations, matching IDA's raw root
function and its boundary-state refresh. The defect was falling back to the
compiler's differently defined SATISFIED evaluation for unlisted guards.

The resulting contract is:

- During IDA initialization and event iteration, ordered guards (`>`, `>=`,
  `<`, `<=`) use their raw relation surface. SATISFIED's explicit or default
  tolerance does not move that surface or create a hysteresis band.
- At a located root, the existing direction override selects the right-limit
  branch through same-time iteration. A resolved reset still releases it
  according to the existing event-local roundoff rule.
- Between events, the selected equation system remains fixed. Reverse
  crossings remain enabled; no new persistent override is introduced.
- Ordinary LRSlv and CMSlv retain the compiler's SATISFIED tolerance and
  perturbation/inversion behaviour. CMSlv inversion also takes precedence
  if both solver mode flags are set.
- Equality/non-equality and logical boundaries retain their existing paths.
  Their event-detection semantics are not redesigned by this change.

This is a change confined to LRSlv's existing IDA execution mode. No parser,
compiler evaluation, IDA root finder, model syntax or global tolerance has
been changed. A model needing physical hysteresis should express separate
switch-on/switch-off thresholds and state explicitly; SATISFIED tolerance
is not an implicit hysteresis parameter during IDA integration.

## Modelica / OpenModelica review

[Modelica 3.6, section 8.5](https://specification.modelica.org/maint/3.6/equations.html#events-and-synchronization)
describes event-generating expressions as buffered values: they remain
constant during continuous integration and change at events. Root finding
locates the change, and event processing selects the appropriate branch.
This supports separating dynamic guard evaluation from an ordinary
pointwise feasibility test. It does not prescribe ASCEND syntax or require
a user-sized hysteresis band.

[OpenModelica's runtime `model_help.h`](https://github.com/OpenModelica/OpenModelica/blob/master/OMCompiler/SimulationRuntime/c/simulation/solver/model_help.h)
separates ordinary comparison functions from zero-crossing comparisons.
Its `relationhysteresis` routine distinguishes initialization, continuous
integration (returning stored relation values), and event evaluation using
previous relation state and nominal scales. Thus numerical event hysteresis
is a runtime mechanism, not simply a tolerance-shifted ordinary comparison.
This repair adopts consistent event semantics; it does not claim to reproduce
OpenModelica's complete event algorithm.

The [Modelica Standard Library Hysteresis block](https://doc.modelica.org/Modelica%204.0.0/Resources/helpWSM/Modelica/Modelica.Blocks.Logical.Hysteresis.html)
separately models hysteresis using explicit upper/lower thresholds and an
initial output state. That is the appropriate conceptual distinction from
numerical root handling. Sources reviewed 19 September 2026.

## Additional regression coverage

The fixture file now also contains rising-inclusive and falling-strict
nearby crossings; rising/falling recrossing cases with an unrelated event
at 0.5 s and a return crossing at 0.8 s; and `ida_guard_truth_modes`.
The recrossing tests inspect independent trajectories at 0.2, 0.6 and 1 s,
checking both the unrelated-event interval and the return branch. A 0.05 s
maximum step resolves both roots of their nonmonotone guards.

The truth-mode test places a fixed residual at -5e-7, 0 and +5e-7 inside a
1e-6 SATISFIED tolerance. It checks all four ordered operators, a compound
Boolean expression, and equality satisfaction. It compares ordinary LRSlv,
IDA mode, CMSlv inversion, and both flags together. This directly checks
initialization within the old tolerance band and compatibility rather than
relying only on completed integration trajectories.

For the full affected suites:

```sh
LD_LIBRARY_PATH="$HOME/.local/lib:.:${LD_LIBRARY_PATH:-}" test/test --list-failures \
  integrator_ida solver_lrslv solver_cmslv
```

The same logical reversions were observed during phase appearance in a
multi-shell TGA model (`n/n0 > 1e-12`, SATISFIED tolerance `1e-12`). The
reproducer establishes an ASCEND root/logical inconsistency, not a SUNDIALS
defect or the cause of every late TGA corrector failure. The TGA replay must
wait for this repair to return to the modelling branch: the current
`python3`-derived branch does not contain the complete fboard2 model setup.

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

## Verified repair, 19 September 2026

On `satisfied-tolerance` (base `ac333e14`), the final affected-suite command
above passes all 87 tests and 2,870 assertions, with zero failures and no
skips. This includes the original close-event reproducers, all four ordered
operators, nearby recrossings, truth-mode compatibility, the existing IDA
reset/event tests, LRSlv, and eight CMSlv tests. The native harness reports
no leaked memory. `git diff --check` is clean. The pre-fix four-case replay
on this branch reproduced the two failures before the implementation change.

An exploratory additional real non-equality conditional (`ne: g != 0`)
triggered the existing compiler assertion `LogRelIsCond: lrel != NULL`
during fixture setup, before LRSlv evaluation. That exploratory fixture was
removed; resolving this separate compiler limitation is outside this repair.
The production change explicitly excludes unordered relations. Equality
compatibility is tested; no new non-equality event qualification is claimed.

# Hybrid-Dynamics Examples For ASCEND

This directory contains small, heavily commented models that exercise the
current hybrid/event features in ASCEND:

- `REINIT(x, expr);`
- `pre(x)` on the right-hand side of `REINIT`
- inferred discrete real event-memory variables from `REINIT(...)` use
- selector-driven `WHEN(mode)` with `SWITCH TO ... IF ...`
- direct continuous guards on `SWITCH TO ... IF ...` for simple real-valued
  comparison events
- state-local equations inside selector `CASE` branches

These examples are intended for use with the `IDA` integrator. `LSODE` does
not support `CONDITIONAL` / `WHEN` event handling or `REINIT`.

## Common Usage

### CLI

These example models now include default `INTEGRATOR ...;` / `INTEGRATE ...;`
requests in `on_load`, so the shortest non-GUI path is:

```bash
./a4 run path/to/model.a4c
```

To override the model-supplied integration settings from the command line, use:

```bash
./a4 run --integrate path/to/model.a4c -m model_name -d DURATION -u s --steps N
```

Add `--plot` if you want an immediate quick plot after the run.

`./a4 int ...` remains available as a shorthand alias for the same path.

For the fuller CLI behavior and current `--microstates` semantics, see
[`INTEGRATE.md`](../../../INTEGRATE.md).

### GUI

To open a model in the GTK browser:

```bash
./a4 open path/to/model.a4c -m model_name
```

Then in the GUI:

1. open the model
2. choose the `IDA` integrator in the Integrator window
3. set the integration end time / number of reporting steps if desired
4. click `Integ`

The example models set `obs_id` on key real variables, so the integrator
reporter has something useful to show immediately after integration. The
selector-based examples also define default `OBSERVE` lists so
`./a4 run --integrate ...` includes the current selector state in the CLI
table output.

## Examples

In each example block below, the first command is the CLI invocation and the
second is the GUI invocation.

### `ideal_rebound.a4c`

A minimal ideal bouncing-ball example with gravity and instantaneous velocity
reset at impact. This is the intentionally non-settling case, so if you run
long enough you will hit the Zeno/event-accumulation stop.

```bash
./a4 run models/johnpye/dyn/ideal_rebound.a4c
./a4 open models/johnpye/dyn/ideal_rebound.a4c -m ideal_rebound
```

### `resting_rebound.a4c`

A settling variant of the ideal rebound. The ball transitions through
`'free'`, `'impact'`, and `'rest'`, and latches into a resting state after
sufficiently small rebounds.

```bash
./a4 run models/johnpye/dyn/resting_rebound.a4c
./a4 open models/johnpye/dyn/resting_rebound.a4c -m resting_rebound
```

### `lengthening_sawtooth.a4c`

A reset oscillator with simple event memory. Each event resets the state,
records the last event time, and lengthens the next period.

```bash
./a4 run models/johnpye/dyn/lengthening_sawtooth.a4c
./a4 open models/johnpye/dyn/lengthening_sawtooth.a4c -m lengthening_sawtooth
```

### `overflowing_weir.a4c`

A two-state selector example showing state-local equations directly inside
selector cases. Below the weir crest the overflow is zero; above the crest it
follows a simple weir law.

```bash
./a4 run models/johnpye/dyn/overflowing_weir.a4c
./a4 open models/johnpye/dyn/overflowing_weir.a4c -m overflowing_weir
```

## Notes

- These are example models rather than low-level regression models, but they
  are also covered by automated `IDA` tests and should remain executable.
- `ideal_rebound` is the non-settling example used to exercise Zeno detection.
- `resting_rebound` is the settling example used to exercise selector-local
  equations and a resting contact mode.

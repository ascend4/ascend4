Hybrid-dynamics examples for ASCEND
==================================

This directory contains small, heavily commented models that exercise the
current Phase 1A/1B hybrid-event features:

- `REINIT(x, expr);`
- `pre(x)` on the right-hand side of `REINIT`
- inferred discrete real event-memory variables from `REINIT(...)` use

At present these examples are intended for use with the IDA integrator.
LSODE does not support `CONDITIONAL` / `WHEN` event handling or `REINIT`.

Examples in this directory
--------------------------

- `ideal_rebound.a4c`
  A minimal "bouncing ball without contact kinetics" example. The ball moves
  at constant velocity between events and reflects instantaneously at the
  floor by applying `REINIT(v, -e * pre(v));`.

- `lengthening_sawtooth.a4c`
  A reset oscillator with simple event memory. Each event resets the state,
  records the event time, and lengthens the next period.

These are example models rather than solver-regression models, but they are
also covered by automated IDA tests so they should remain executable.

Pantelides reference models
============================

These models are reference cases for discussing structural index analysis and
Pantelides-style index reduction in ASCEND.

They are not yet solver regressions for the current implementation.

Purpose:

- record classic derivative-chain examples in ASCEND syntax
- give concrete targets for future structural analysis work
- keep discussion of `v = der(x)` grounded in real models
- prefer canonical ASCEND `der(...)` syntax in the reference equations where
  that keeps the model readable

Current caveats:

- ASCEND does already have a function-of-time surface of sorts:
  - equations may explicitly depend on the independent variable `t`
  - time-dependent data can also be supplied through the
    `models/johnpye/datareader` path
- the reactor example still uses a plain variable `u` as a placeholder for the
  exogenous input `u(t)`, simply to keep this reference case focused on
  derivative-chain structure rather than forcing-function syntax
- the pendulum example now uses dimensioned quantities for the obvious motion
  variables (`distance`, `speed`, `acceleration`, `time`) and also introduces
  `mass` and `force` so that the multiplier `T` can be written as a force
  rather than an abstract `1/s^2` coefficient
- these models should currently be read as structural examples, not as claims
  of end-to-end Pantelides support

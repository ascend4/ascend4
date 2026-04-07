# FPROPS development notes

This repo is a standalone thermophysical property library (FPROPS) with
optional ASCEND integration. Build and tests are SCons-driven.

## Layout
- `fluids/`: per-fluid correlation data and optional per-fluid tests.
- `mix/`, `mixture.*`: mixture and ideal-gas mixture support.
- `thermo_*.c/.h`: ThermoModel helpers (pure, ideal mix, const-cp).
- `eqm*.c/.h`: equilibrium solvers (IPOPT/NLOPT) and shared utilities.
- `test/`: CUnit-based tests plus `ipopt_eqm` test harness.
- `test.py`: ad-hoc single-fluid test runner for `fluids/*.c`.

## Build
- Standalone build from this directory: `scons`
- The top-level `SConscript` is used when building from a higher-level
  ASCEND tree; the local `SConstruct` lets you build this directory alone.
- Optional components:
  - CUnit: detected via `pkg-config cunit` (see `SConstruct`).
  - IPOPT: detected via `pkgconf ipopt --libs --cflags` (test harness).
  - NLOPT: detected via `pkgconf nlopt --libs --cflags` (test harness).

## Tests
- CUnit test runner: `scons cutest` then `test/cutest`
- Equilibrium test harness (if IPOPT is available): `scons ipopt_eqm`
- Ad-hoc single-fluid tests: `./test.py water` (builds and runs a
  `fluids/test-water` binary).

## Equilibrium solvers
- Public API: `eqm.h` (`eqm_solve`, `eqm_solve_elements`,
  `eqm_mu0_ideal_source`).
- IPOPT-specific implementation: `eqm_ipopt.*`
- NLOPT/SLSQP implementation: `eqm_slsqp.*`
- Shared helpers: `eqm.c`, `eqm_internal.h`

## Thermo models
- `thermo_constcp.*`: constant-cp condensed-phase model.
- `thermo_mix_ideal.*`: ideal-gas mixture model (also used for pure-as-mix).
- `thermo_pure.*`: pure-fluid wrapper around per-fluid EOS implementations.

## Coding notes
- Most per-fluid data is C code in `fluids/*.c` and registered via
  `fprops_fluid`/`fprops_eos` lookups in `fluids.c`.
- Source filtering is supported by the `source` argument in lookups and in
  `fprops_build_element_matrix_source` (see `fluids.h`).

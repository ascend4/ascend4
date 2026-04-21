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
- `SConstruct` is the standalone entry point when building directly in
  `models/johnpye/fprops`.
- `SConscript` is the in-tree entry point used when building from the
  higher-level ASCEND tree.
- Optional components:
  - CUnit: detected by local SCons tool `scons/cunit.py`.
  - IPOPT: detected by local SCons tool `scons/ipopt.py`.
  - NLOPT: detected by local SCons tool `scons/nlopt.py`.
- The standalone build is self-contained and should not depend on parent-tree
  `scons` helpers.
- Common packaging assumptions:
  - MSYS2: CUnit normally comes from the packaged install.
  - Linux (eg Rocky/Ubuntu fallback): CUnit is often installed under
    `$HOME/.local`.
  - IPOPT/NLOPT detection is pkg-config driven.

## Tests
- CUnit test runner: `scons cutest` then `test/cutest`
- Equilibrium test harness (if IPOPT is available): `scons ipopt_eqm`
- Additional equilibrium/diagnostic runners:
  - `scons eqm_case_runner`
  - `scons eqm_mu0_runner`
  - `scons eqm_nox_runner`
  - `scons pureprops_compare`
- Ad-hoc single-fluid tests: `./test.py water` (builds and runs a
  `fluids/test-water` binary).

## Equilibrium solvers
- Public API: `eqm.h` (`eqm_solve`, `eqm_solve_elements`,
  `eqm_mu0_ideal_source`).
- IPOPT-specific implementation: `eqm_ipopt.*`
- NLOPT/SLSQP implementation: `eqm_slsqp.*`
- Shared helpers: `eqm.c`, `eqm_internal.h`
- Solver code is conditionally compiled behind `HAVE_IPOPT` and `HAVE_NLOPT`.

## Thermo models
- `thermo_constcp.*`: constant-cp condensed-phase model.
- `thermo_mix_ideal.*`: ideal-gas mixture model (also used for pure-as-mix).
- `thermo_pure.*`: pure-fluid wrapper around per-fluid EOS implementations.

## Coding notes
- Most per-fluid data is C code in `fluids/*.c` and registered via
  `fprops_fluid`/`fprops_eos` lookups in `fluids.c`.
- Source filtering is supported by the `source` argument in lookups and in
  `fprops_build_element_matrix_source` (see `fluids.h`).
- Source selectors are part of normal workflow for equilibrium/oxide work;
  prefer preserving explicit source names in tests and diagnostics.

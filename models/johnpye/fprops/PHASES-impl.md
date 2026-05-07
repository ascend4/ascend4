# Phase-Aware Equilibrium Implementation Notes

This is the archived implementation-plan and decision-history companion to
[PHASES.md](PHASES.md).

## 1. Goal

Implement solution phases as named components in the FPROPS chemical
equilibrium solver.

The immediate target is the Fe-O-H reduction problem:

- allow a caller to concisely specify an Fe-O-H equilibrium package in
  terms of useful phases such as iron, hematite, wustite, spinel, and
  H2/H2O gas
- solve fixed `T`, `P`, and element totals robustly from feeds such as
  hematite plus hydrogen
- report phase amounts and phase compositions, including wustite
  composition and spinel site fractions
- avoid the custom boundary-only workflow currently used in
  `test/feoh_hydrogen_boundary.py` for cases that should be ordinary
  equilibrium solves

The broader target is a phase-aware equilibrium layer that is general
enough for anticipated slag, metal, gas, liquid, and ordinary solution
systems, without attempting to implement every future phase model in the
first pass.

Success for the first implementation means:

- a small standalone C example can express the Fe-O-H system without
  manually listing all wustite and spinel endmembers as user-level
  components
- the same example solves robustly in the strongly reducing case that
  previously failed through the raw flat species path
- boundary and phase-presence results are consistent with the existing
  Fe-O-H / Baur-Glaessner regression scripts
- the feature is available through the FPROPS C API and is covered by
  C-level unit/regression tests; Python and ASCEND wrappers are explicitly
  out of scope for the first implementation
- a build target provides a standalone shared library, `libfprops.so`,
  so an external example similar to `~/feoh/feoh.c` can link against
  FPROPS without depending on ASCEND

## 2. Motivation

The current equilibrium kernel is fundamentally a flat species-amount
Gibbs minimizer:

```text
minimize G(T, P, n)
subject to A n = b
           n >= 0
```

That form works well for gas species and stoichiometric condensed
species. It is less robust for condensed solution phases whose internal
composition is represented by several pseudo-species/endmember amounts.

The Fe-O-H example shows the problem. A user would like to solve with:

```text
Fe metal
hematite
wustite
spinel/magnetite
H2/H2O gas
```

The current flat implementation asks the caller or package to work with:

```text
Fe_bcc
Fe_fcc
Wus_FeO
Wus_FeO1p5
Sp_Fe2_tet
Sp_Fe3_tet
Sp_Fe2_oct
Sp_Fe3_oct
Sp_Va_oct
Fe2O3
hydrogen
water
```

For inactive or nearly inactive solution phases, all member amounts can
approach zero while their ratios still define the phase composition.
This makes activity evaluation, active-set decisions, and validation
fragile. Spinel is especially awkward because the member amounts also
carry site-balance and charge-neutrality constraints.

The existing Fe-O-H boundary scripts avoid this by solving smaller,
phase-structured potential problems:

- wustite composition is treated as a bounded composition variable
- spinel site fractions are minimized over their admissible domain
- phase boundaries are found from grand-potential residuals
- FPROPS is used for thermodynamic functions, especially gas `mu0`

Those scripts demonstrate that the thermodynamic models are useful, but
they do not prove that the generic flat equilibrium solver can discover
the active phase assemblage robustly. This document specifies the
missing phase-aware layer.

## 3. Requirements

### 3.1 Functional requirements

- Named solution phases can be requested as first-class equilibrium
  components through a C API.
- A phase can expose:
  - amount variables
  - internal composition variables
  - element contents as a function of internal composition
  - Gibbs energy and chemical-potential information
  - valid composition bounds and constraints
  - phase-entry or stability residuals
- Stoichiometric phases remain supported.
- Gas mixtures remain supported, initially using the current ideal
  mixture treatment and existing `mu0` source selection.
- The solver can activate and deactivate whole phases, not only
  individual endmember pseudo-species.
- Inactive solution phases are tested by minimizing normalized
  grand-potential residuals over valid internal compositions.
- Active solution phases are solved using variables that keep phase
  amount and internal composition distinct.
- Results can be reported in user-facing phase terms and, where needed,
  expanded back into current internal species/endmember amounts.
- A standalone C demonstration program can be built and run outside the
  FPROPS test harness, using `libfprops.so`, to show the feature to new
  users.

### 3.2 Non-functional requirements

- Preserve the existing flat solver and current public API while the new
  path matures.
- Reuse existing thermodynamic model code where possible.
- Reuse IPOPT integration and existing scaling/continuation ideas.
- Keep the first implementation narrow enough to finish and test:
  Fe-O-H first, then broader solution systems.
- Provide diagnostics that identify the failing layer: phase resolution,
  entry minimization, active-set selection, inner NLP, validation, or
  property evaluation.
- Keep source/reference-state selection explicit. Mixed source maps and
  `helmholtz+ref0:` gas usage must remain inspectable.
- Provide close C-level test coverage:
  - unit tests for phase registry and phase thermodynamic callbacks
  - regression tests for phase-entry residuals and Fe-O-H boundaries
  - solver tests for fixed assemblages and automatic phase activation
  - a standalone example test that exercises the installed/shared-library
    linkage path

### 3.3 Non-goals for the first pass

- Do not replace all of `eqm.c`.
- Do not immediately route all `algorithm="auto"` calls through the new
  phase-aware path.
- Do not attempt a fully generic slag-liquid model before Fe-O-H is
  reliable.
- Do not require ASCEND.
- Do not require Python or ASCEND wrappers for the new phase-aware API.
- Do not require every phase to provide analytic derivatives on day one,
  although derivative quality must be testable and improved over time.

## 4. Concepts

### 4.1 Phase-aware formulation

The desired high-level formulation is:

```text
minimize sum_p N_p g_p(T, P, y_p)
subject to element balances
           N_p >= 0
           y_p in valid phase domain
```

where:

- `p` is a phase
- `N_p` is the phase amount
- `y_p` is the internal composition/state of the phase
- `g_p` is molar or formula-unit Gibbs energy on the phase's natural
  basis

Stoichiometric phases have no internal composition variables.
Solution phases have one or more internal composition variables.
Gas and ordinary liquid mixtures are also solution phases in this sense.

### 4.2 Phase entry

For an inactive phase, the solver should not evaluate activities at
zero amount. Instead, it should perform a normalized phase-entry test.
Given thermodynamic element potentials `lambda_e`, using the stationarity
convention `mu - A^T lambda - s = 0`, a phase wants to enter if:

```text
min_y [ g_p(T, P, y) - sum_e lambda_e a_e(y) ] < 0
```

within tolerance, where `a_e(y)` is the element content of one unit of
phase at composition `y`.

The public phase API uses this thermodynamic sign convention: `lambda_e`
is the chemical-potential contribution of one mole of element `e`.

For stoichiometric phases this is a direct scalar calculation. For
solution phases it is a bounded composition minimization. This is the
generic version of the current Fe-O-H boundary residual work.

### 4.3 Whole-phase activation

The active set should operate over phases:

```text
inactive wustite
active wustite
inactive spinel
active spinel
```

not over individual pseudo-species such as `Sp_Fe2_tet` or
`Sp_Va_oct`.

This does not forbid using member amounts internally. For a simple
binary phase such as wustite, member amounts and `(N, x)` are nearly
equivalent away from zero. The important design requirement is that the
solver understands phase amount and internal composition separately,
especially at phase disappearance.

### 4.4 Named and generic phases

FPROPS should support both predefined named phases and generic phase
constructors.

Predefined named phases are needed for chemically specific models:

```text
phase:wustite[hidayat_2015]
phase:spinel[degterov_2001]
```

These phases cannot be inferred safely from formulas alone because they
carry fitted parameters, sublattice/site structure, valid domains, and
composition constraints.

Generic constructors are useful for ordinary mixtures:

```text
gas:ideal(hydrogen,water)
phase:ideal_liquid(water,ethanol)
phase:regular_solution(A,B;...)
```

The first pass should implement named Fe-O phases plus an ideal gas
mixture. Generic liquid and slag constructors can follow later.

## 5. Proposed Interfaces

### 5.1 Internal phase model interface

A phase model should expose enough information for solving, testing, and
diagnostics. The exact C structs can evolve, but the conceptual
interface should include:

```c
typedef struct FpropsEqmPhaseModel FpropsEqmPhaseModel;

typedef enum FpropsEqmPhaseKind{
    FPROPS_EQM_PHASE_STOICHIOMETRIC,
    FPROPS_EQM_PHASE_IDEAL_GAS,
    FPROPS_EQM_PHASE_BINARY_SOLUTION,
    FPROPS_EQM_PHASE_SITE_SOLUTION,
    FPROPS_EQM_PHASE_GENERIC
} FpropsEqmPhaseKind;

struct FpropsEqmPhaseModel{
    const char *name;
    const char *source;
    FpropsEqmPhaseKind kind;
    int ncomp;
    int nvar;
    int nelem;
    const char *const *elements;
    const char *basis;
    const char *const *var_names;
    const double *lower;
    const double *upper;

    int (*elements)(const FpropsEqmPhaseModel *phase,
        const double *y, double *a_out);

    int (*gibbs)(const FpropsEqmPhaseModel *phase,
        double T, double P, const double *y, double *g_out);

    int (*mu)(const FpropsEqmPhaseModel *phase,
        double T, double P, const double *y, double *mu_out);

    int (*initial_y)(const FpropsEqmPhaseModel *phase,
        double T, double P, const double *lambda, double *y_out);

    int (*entry_min)(const FpropsEqmPhaseModel *phase,
        double T, double P, const double *lambda,
        double *phi_min_out, double *y_min_out);
};
```

The callbacks shown above are ordinary C function pointers, not C++
methods or constructors. Different phase implementations can fill these
pointers according to the phase kind. For example, a stoichiometric
phase may provide a trivial `elements` callback and no internal
composition variables, while a spinel phase provides site-constrained
composition callbacks and a model-specific entry minimizer.

The first implementation should use these basis conventions:

- `g(T,P,y)` is the Gibbs energy of one natural phase unit. For
  stoichiometric phases this is one species mole; for binary solution
  phases it is one mole of solution members; for the reduced spinel
  phase it is the same formula-unit/member basis already used by
  `spinel_phase_eval`.
- `elements(..., a_out)` writes element contents for that same natural
  phase unit, in the phase model's declared element order.
- Binary solution phases use `y[0]` as the fraction of member B, matching
  the current `solution_binary_g_molar(M,T,P,x,...)` convention where
  `x = n_b / (n_a + n_b)`.
- Site-solution phases must report variable names, bounds, and any
  equality constraints needed to interpret their internal coordinates.
  The first spinel implementation may expose member/site coordinates
  directly, but it must keep the charge and site-balance constraints
  inspectable.
- Entry residuals are normalized by `R T` in public diagnostics and
  tests, even if internal minimizers also return dimensional J values.

The first implementation can use simpler structs and function groups,
but tests should be written around these capabilities.

### 5.2 C API and user-facing specification

The first supported interface should be C. A string-based phase-package
specification is acceptable if it remains a C API input and follows
existing source-map practice:

```text
Fe_bcc=hidayat_2015;
Fe_fcc=hidayat_2015;
Fe2O3=hidayat_2015;
phase:wustite=hidayat_2015;
phase:spinel=degterov_2001;
gas:ideal(hydrogen,water)=helmholtz+ref0:
```

A structured C package API is also desirable, and may be the better
long-term interface for tests, caching, and external callers:

```c
FpropsEqmPhasePackage *fprops_eqm_phase_package_build(...);
int fprops_eqm_phase_tpb(...);
```

The first implementation does not need Python bindings or ASCEND
wrapping. Those can be designed later once the C API and solver
semantics are stable.

### 5.3 Output model

The phase-aware result should include:

- solver status and status text
- active phase list
- phase amounts
- phase compositions
- gas species amounts or mole fractions
- optional expanded internal species/endmember amounts
- element balance residuals
- KKT/phase-entry residual summary

For Fe-O-H, minimum useful output includes:

- metallic Fe amount, split by bcc/fcc if both are present
- unreduced hematite amount
- wustite amount and composition
- spinel amount and site fractions
- hydrogen and water gas amounts

### 5.4 Diagnostics interface

The solver should provide structured diagnostics in addition to text
traces. A minimal C diagnostic interface should allow:

- setting a logging level, for example error, warning, info, trace
- registering an optional progress/log callback
- reporting current active phases during the active-set loop
- reporting phase-entry candidates and residuals
- reporting IPOPT status and validation residuals

Environment-variable traces are useful for development, but a callback
is more useful for standalone C examples, regression tests, and later
embedding in other applications. This is expected to give immediate
return on effort because phase-entry and active-set failures otherwise
look very similar from the outside.

## 6. Implementation Phases

### Phase 0: Documentation and diagnostics

Deliverables:

- this document
- a public or semi-public status decoder for equilibrium status codes
- add a `libfprops.so` build target, separate from `libfprops_ascend.so`,
  suitable for standalone C examples

Tests:

- `~/feoh/feoh.c` prints meaningful status text for current failures
- status text is available from FPROPS rather than duplicated in each
  example program
- a standalone C example can link against `libfprops.so` without ASCEND

Exit criteria:

- current failures are diagnosable without reading IPOPT return codes or
  private trace output

### Phase 1: Phase registry and inspection API

Deliverables:

- register predefined `wustite[hidayat_2015]`
- register predefined `spinel[degterov_2001]`
- register stoichiometric condensed phases already used in Fe-O-H
- register an ideal gas mixture over named gas species, initially H2/H2O
- provide inspection functions for tests:
  - resolve phase spec to phase model
  - list phase internal members
  - list element names and element contents
  - evaluate `g(T,P,y)`
  - evaluate phase chemical potentials if available
- improved trace output for:
  - algorithm dispatch
  - phase/source resolution
  - active set decisions
  - validation failure reason
- C-level logging/progress callback hooks for phase-aware solves

Tests:

- wustite registry maps to `Wus_FeO` / `Wus_FeO1p5`
- spinel registry maps to the five Degterov members
- spinel registry reports site and charge constraints
- H2/H2O gas source resolves to `helmholtz+ref0:`
- `g(T,P,y)` finite-value tests at `900 C`
- logging callback tests can capture at least one phase-resolution event
  and one solver-status event once callback plumbing exists

Exit criteria:

- tests can inspect all Fe-O-H phases without entering the equilibrium
  solver

### Phase 2: Phase-entry residuals

Deliverables:

- implement phase-entry residual for stoichiometric phases
- implement bounded composition entry minimization for wustite
- implement bounded/site-constrained entry minimization for spinel
- expose phase-entry residuals through a test API
- reuse existing Fe-O-H boundary functions as regression oracles

Tests:

- Fe|wustite boundary at `900 C` reproduces existing
  oxygen-potential result after conversion to gas ratio:
  - `log10(H2O/H2) = -0.225974` with `helmholtz+ref0:`
- wustite|spinel boundary at `900 C` reproduces the existing provisional
  result:
  - `log10(H2O/H2) = 0.665196` with `helmholtz+ref0:`
- spinel|hematite boundary agrees with
  `test/feoxide_potential_boundaries.py` within a defined tolerance
- inactive phases have positive entry residual away from their stability
  fields and negative residual when they should enter

Exit criteria:

- phase-entry tests independently identify the expected Fe-O-H stability
  fields

Note: the first bullet above should be implemented on a common
`lambda_O` basis in the test code; the gas-ratio number is the
user-facing regression value.

### Phase 3: Inner solve for a fixed active assemblage

Deliverables:

- solve phase-aware equilibrium for a supplied active phase set
- support at least:
  - gas H2/H2O
  - Fe metal
  - hematite
  - wustite
  - spinel
- compute element balances from phase amounts and compositions
- use IPOPT for the nonlinear solve
- provide expanded endmember amounts for comparison with current code

Tests:

- fixed active assemblage `Fe + gas` solves the strongly reducing case:
  - feed: `1 mol Fe2O3 + 100 mol H2`
  - `T = 1173.15 K`, `P = 101325 Pa`
  - expected: about `2 mol Fe`, `3 mol H2O`, `97 mol H2`, negligible
    wustite/spinel/hematite
- fixed active assemblage `wustite + gas` solves a feed/gas ratio inside
  the wustite field
- fixed active assemblage `spinel + gas` solves a feed/gas ratio inside
  the spinel field
- fixed active assemblage tests for wustite and spinel must compare both
  phase totals and internal composition variables against boundary/entry
  expectations; the `Fe + gas` case is only the reducing-limit smoke test
- element balance residuals are below tolerance
- internal composition variables remain within valid domains

Exit criteria:

- the inner solver is reliable when given the correct active phases

### Phase 4: Phase-level active selection

Phase 4 is split into two parts. Phase 4a provides an immediately useful
package-level solve by trying whole-phase assemblages and ranking valid
solutions. Phase 4b closes the original active-set design by using KKT
multipliers and phase-entry residuals to add/drop phases directly.

### Phase 4a: Whole-phase subset selection

Status: implemented as the first automatic phase-selection layer.

Implemented:

- the internal enumerating solver accepts a supplied phase package,
  element list, element totals, `T`, `P`, and algorithm choice
- the implementation enumerates whole-phase subsets, expands each subset
  to current member species, solves through the fixed-active bridge, and
  maps successful results back to full-package phase amounts,
  compositions, active flags, and expanded member amounts
- successful subsets are validated for element balance and ranked by total
  Gibbs energy
- trace diagnostics are available through `FPROPS_EQM_PHASE_TRACE=1`
- gas treatment is not hard-wired to H2/H2O; any supported ideal gas
  member list can be supplied, such as H2/H2O or CO/CO2

Tests implemented:

- strongly reducing Fe-O-H package activates `Fe + H2/H2O gas` without
  manually specifying the active assemblage
- Fe-O-H BG-positioned wustite field checks at 600 C, 700 C, and 900 C
- Fe-O-C BG-positioned wustite field check at 900 C using Fe, Fe oxides,
  CO, and CO2 only; no free carbon or Fe3C phase is included
- CO/CO2 gas smoke test verifies the phase API is not H2/H2O-specific
- inactive phases can remain in the supplied package without being
  returned active when a lower-G solution is found

Limitations:

- this is subset enumeration, not the final add/drop active-set loop
- inactive phases are not yet validated by entry residuals at the final
  solution because the inner solve does not yet expose or reconstruct
  robust element potentials
- warm-starting between candidate assemblages is not yet implemented
- the trace reports candidate subset status and objective, not
  multiplier-based add/drop decisions
- 500 C Fe/spinel and Fe3O4-rich behavior remains an explicit Phase 4b
  truth-check target

Exit criteria for Phase 4a:

- users can solve initial Fe-O-H and Fe-O-C package-level cases by
  specifying phases and element totals, without manually selecting the
  active assemblage
- current BG-positioned checks confirm the selected whole phase is
  plausible in the wustite field

### Phase 4b: Multiplier-driven phase active set

Status: first-pass implementation and BG truth-check coverage are in
place. The active-set implementation is kept internal for now; the
enumerating solver remains the small-package reference path while public
strategy selection is decided.

Implemented:

- internal lambda reconstruction recovers thermodynamic element
  potentials from the active assemblage stationarity equations
- internal entry-residual validation checks the final assemblage using
  active stationarity and inactive phase-entry residuals
- the internal active-set solver performs an add/drop loop over supplied
  phase packages
- inner solves are warm-started from the previous expanded member amounts
  after phase additions/removals
- candidate phase additions are ranked by most negative phase-entry
  residual
- candidate solution phases are seeded from their entry-minimizing
  composition when the active-set loop tries an add/swap move
- low-amount phases are dropped from the active set
- failed add attempts can recover by removing one currently active phase;
  solution-phase swaps can also replace one condensed solution phase with
  another, while preserving stoichiometric solids and gas phases
- locally valid active sets are checked for lower-G one-for-one condensed
  replacements before they are accepted, which covers metastable
  `Fe + spinel + gas` traps in the wustite field
- warm-started solves that return nominal success but fail the balance
  check are retried once without the member seed for the same active mask
- trace output includes final stationarity RMS and per-phase entry
  residuals when `FPROPS_EQM_PHASE_TRACE=1`
- active-set results fail closed unless the final active mask passes:
  - element balance
  - active phase stationarity
  - inactive phase entry residuals
  - internal composition bounds from the fixed-active solve

Tests implemented:

- strongly reducing Fe-O-H activates `Fe + gas` from a broad package-level
  call
- BG-positioned 600 C, 700 C, and 900 C Fe-O-H cases activate
  `wustite + H2/H2O gas`
- BG-positioned 585 C, 700 C, and 900 C Fe-O-C cases activate
  `wustite + CO/CO2 gas`
- low-temperature BG Fe|spinel entry-residual checks at 400 C, 500 C,
  540 C, and 560 C use the diagnostic `fe_spinel_bg_tuned_2026`
  source
- 400 C, 500 C, 540 C, and 560 C Fe-O-H active-set package solves validate
  `Fe + spinel + H2/H2O gas` after replacing the initial wustite seed
- 400 C, 500 C, 540 C, and 560 C Fe-O-C active-set package solves validate
  `Fe + spinel + CO/CO2 gas`
- a near-fork 585 C Fe-O-H package solve validates the wustite field on
  the oxidized side of the Fe|spinel to Fe|wustite/wustite|spinel fork
- near-fork 585 C and 700 C Fe-O-C package solves validate the wustite
  field and agree with the enumerating reference path
- 700 C Fe-O-H validation rejects inactive Fe, spinel, and hematite by
  entry residual validation
- 700 C Fe-O-H active-set validation is insensitive to the checked
  `Fe + gas`, `wustite + gas`, and full-package initial masks
- selected phase/eqm/fprops suites currently pass with the Phase 4b API
  enabled
- solver-performance benchmarking now strongly favors NLOPT/SLSQP over IPOPT
  for the current phase-equilibrium workload; on the local development
  machine, the phase suite was roughly 0.6 s with SLSQP versus roughly 180 s
  with IPOPT-only, while selected boundary active-set cases were roughly
  0.2 s with SLSQP versus roughly 63 s with IPOPT-only
- the IPOPT full-space path has since been harmonised with the SLSQP
  formulation: plain `ipopt` and explicit `ipopt_scaled_n` now use the scaled
  amount variables, while `ipopt_logn` and `ipopt_n` remain explicit
  alternatives. This removed the observed IPOPT robustness split in the
  active-set phase cases, but did not close the performance gap. An
  IPOPT-enabled phase-suite run passed 43/43 tests but still took about 185 s
  CUnit elapsed time on the local machine.
- the formerly failing SLSQP-only cases are fixed: the spinel expanded-member
  case needed zero initial scales to be clamped in `eqm_fill_n_est`, and the
  humid-air NOx reduced solve needed a validated near-stationary
  max-iteration acceptance path

Remaining Phase 4b close-out work:

- add more boundary-near BG truth checks around Fe/FeO, Fe/Fe3O4, and
  FeO/Fe3O4-sensitive regions, especially cases closer than the current
  560 C/585 C bracket
- add additional Fe-O-C active-set checks using Fe, Fe oxides, CO, and
  CO2 only on both sides of the checked 585 C and 700 C wustite points
- sample boundary-adjacent cases on both sides of Fe|wustite and
  wustite|spinel fields
- broaden sensitivity tests beyond the current initial-mask checks to
  include deliberately perturbed expanded member warm starts
- decide whether the enumerating reference path should remain internal,
  call active-set first, or expose both as separate public strategies
- add continuation where needed in temperature or feed severity

### Phase 5: Public API and examples

Status: first-pass C API and standalone example are in place.

Deliverables:

- documented phase-aware C API entry points are in `eqm_phase.h`
- `fprops_eqm_status_ok(...)` centralizes the "usable result" policy for
  raw solver status codes while preserving detailed status values for
  diagnostics
- `FPROPS_R` in `common.h` is the central molar gas constant used by
  equilibrium, solution, Shomate, and example code; legacy `R_UNIVERSAL` is
  derived from it on the existing J/kmol/K basis
- `FpropsEqm` provides a problem object that owns the phase package, global
  element ordering, feed totals, temperature, pressure, and solver strategy
- `FpropsEqmNlpSolver` plus `fprops_eqm_set_nlp_solver(...)` and
  `fprops_eqm_set_nlp_solver_name(...)` provide runtime NLP backend selection;
  the default `auto` behavior is SLSQP when NLOPT is available, and explicit
  IPOPT selectors run only the requested IPOPT formulation
- `fprops_eqm_init(...)`, `fprops_eqm_add_phases(...)`,
  `fprops_eqm_add_comps(...)`, and `fprops_eqm_solve_TP(...)` provide the
  compact example-facing workflow
- `fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "H2", 100)` is a typed C99 macro
  that expands to a `FpropsEqmComp[]` and counted
  `fprops_eqm_add_comp_items(...)` call; it avoids raw varargs and does not
  need a `NULL` sentinel
- feed initialization accepts both direct element totals such as
  `"Fe", 2, "O", 3, "H", 200` and formula/species inputs such as
  `"Fe2O3", 1, "H2", 100`
- solution-phase feed initialization is available through
  `fprops_eqm_add_phase_feed(...)` for coordinates in the declared phase
  order and `fprops_eqm_add_phase_feed_vars(...)` for named coordinates
- known phase/species/member names are resolved through existing FPROPS
  element-matrix metadata before falling back to the literal formula parser
- phase names, coordinate names, and expanded member names are discoverable
  through `fprops_eqm_phase_name(...)`,
  `fprops_eqm_phase_coord_name(s)(...)`, and
  `fprops_eqm_phase_member_name(s)(...)`
- low-level package resolver, active-set solver, fixed-package solver,
  lambda reconstruction, and validation entry points have moved out of the
  public header into `eqm_phase_internal.h`
- first-pass Python/SWIG bindings expose the phase API as `fprops.Eqm` and
  `fprops.EqmPhaseResult`, including phase setup, Python list/dict feed
  helpers, solver selection, solve/try-solve, and name/amount/coordinate/member
  result accessors
- raw flat equilibrium solvers and the ideal-only standard-potential helper
  have moved out of the public header into `eqm_internal.h`; public callers
  should use `fprops_eqm_tpb(...)`, `fprops_eqm_tpy(...)`,
  `fprops_rxn_eqm_tpb(...)`, `fprops_rxn_eqm_tpy(...)`, or
  `eqm_mu0_source(...)`
- `fprops_eqm_phase_find_element(...)` provides a shared phase-model element
  lookup helper so examples do not carry local name-search loops
- `FpropsEqmPhaseResult` stores a backpointer to its source `FpropsEqm`, so
  result accessors such as `fprops_eqm_phase_amount(...)`,
  `fprops_eqm_phase_coord(...)`, and
  `fprops_eqm_phase_member_amount(...)` do not require callers to pass the
  problem object again; its public `status` is zero for usable results and
  `solver_status` retains the raw solver detail for diagnostics
- binary solution coordinates use model-derived names, for example
  wustite exposes `x_FeO1p5` rather than the earlier placeholder
  `x_member_b`
- `fprops_eqm_write(...)` provides a centralized text writer
  for package-indexed phase composition output; other formats such as JSON
  or YAML can be added behind the same `format` parameter
- `examples/feoh.c` is the in-tree FPROPS-only replacement for the
  previous out-of-tree `~/feoh/feoh.c` workflow
- `examples/feoh` links against `libfprops.so` rather than compiling the
  library objects into the example
- `libfprops.so` exports the phase-aware API symbols used by the example
- API documentation explains when to use public flat/package solvers, the
  phase-aware solver, and boundary diagnostic scripts

Tests:

- standalone Fe-O-H example builds without ASCEND via
  `scons -C models/johnpye/fprops WITH_ASCEND=0 examples/feoh`
- standalone Fe-O-H example links against `libfprops.so` and has an
  `$ORIGIN/..` runpath so it runs in-tree without `LD_LIBRARY_PATH`
- default FPROPS builds now link IPOPT as well as NLOPT/SLSQP when both are
  available, but plain `auto` still uses SLSQP; IPOPT can be disabled with
  `WITH_FPROPS_IPOPT=0`
- default CUnit registration includes a few guarded explicit-IPOPT smoke checks
  when IPOPT is compiled in, without duplicating the whole phase suite through
  IPOPT
- example reports:
  - Fe amount
  - unreduced hematite
  - wustite amount/composition
  - spinel amount/site composition
  - H2/H2O gas amounts
- example succeeds with `helmholtz+ref0:` H2/H2O gas
- example handles reducing, intermediate wustite, and oxidizing spinel
  cases
- CUnit coverage verifies the compact problem API, formula feed setup,
  direct-element feed setup, resolved species/member feed setup, result
  backpointer access, and phase/member discovery

Exit criteria:

- the original user-facing Fe-O-H request is satisfied with a small C
  example and no ASCEND dependency

Remaining Phase 5 follow-up:

- decide whether a heap-allocated `FpropsEqm` constructor/destructor pair is
  needed in addition to the current caller-owned stack object API
- add install rules for public phase headers/examples if FPROPS install
  packaging is enabled later
- add a scripted example-output regression if the build system grows a
  standard run-test hook for examples
- add JSON/YAML result writer formats if downstream tooling needs
  machine-readable phase output

### Phase 6: Broader mixture and slag preparation

Deliverables:

- add generic ideal gas mixture support beyond H2/H2O and CO/CO2
- adapt the existing UNIFAC liquid-mixture work into a phase-model
  interface that supplies `g(T,P,y)`, composition variables, component or
  element contents, phase member names, and property outputs
- define how Helmholtz pure-fluid vapor/liquid phases fit beside ideal
  gas mixtures and liquid solution phases
- define the first slag/ore-capture phase interface requirements using
  [SLAG.md](SLAG.md)
- add Fe-O-Si-Al package construction tests
- revisit early equilibrium examples such as water/alcohol mixtures and
  Ni/O oxide cases using the public phase-aware API

Tests:

- ideal gas reaction equilibria remain consistent with existing gas-only
  regressions
- ethanol/water or another simple nonreactive UNIFAC liquid mixture can be
  represented as a phase without breaking current flash work
- a Helmholtz pure fluid can still use ordinary vapor/liquid flash paths,
  while the phase-equilibrium layer can admit the same substance as
  explicit vapor and/or liquid phases where useful
- Ni/O or another early oxide-equilibrium example is expressed through
  the updated public API and checked against its existing oracle
- Fe-O-H-Si-Al package construction and source resolution pass
- fayalite/hercynite admission tests are possible once their thermo
  basis is ready

Exit criteria:

- the phase abstraction is demonstrably not Fe-O-H-only

## 7. Key Design Issues

### 7.1 Member amounts versus phase amount/composition

For binary wustite, member amounts are often adequate away from zero:

```text
N = N_FeO + N_FeO1p5
x = N_FeO1p5 / N
```

The problem is not the algebraic representation itself. The problem is
that the current flat solver does not explicitly understand phase total
and internal composition as separate concepts. At phase disappearance,
member amounts all approach zero while the composition variable remains
thermodynamically meaningful for entry testing.

The implementation may store active phases internally as member amounts
where convenient, but active-set decisions and validation should be
phase-level.

### 7.2 Derivatives

The first Fe-O-H implementation may use finite-difference derivatives
where existing models do so already, but derivative quality must be
tested. Longer term, phase models should provide analytic or carefully
verified derivatives for:

- Gibbs energy with respect to internal composition
- element contents with respect to internal composition when needed
- phase-entry minimization
- inner NLP objective and constraints

Poor derivatives will look like false infeasibility, bad phase-entry
decisions, or validation failures.

Expected near-term derivative difficulty:

- Stoichiometric phases should be straightforward because there are no
  internal composition variables.
- Ideal gas mixture derivatives should be straightforward and should be
  hand-coded.
- Binary wustite derivatives should be feasible to hand-code because the
  model has one composition variable and existing `mu_a` / `mu_b`
  functions already encode the essential thermodynamics.
- Spinel derivatives are more difficult because of site fractions,
  charge/site constraints, and current finite-difference member
  potentials. The first pass can use robust finite differences for
  spinel entry and inner solves, but the code should be structured so
  hand-coded derivatives can replace them.
- Slag or high-dimensional site/associate models may eventually justify
  automatic differentiation or generated derivative code, but autodiff
  is not a first-pass dependency. It should be discussed again once a
  concrete slag model is selected.

Derivative tests should compare analytic, finite-difference, and
possibly complex-step derivatives where the underlying functions permit
it.

### 7.3 Nonconvexity

Spinel and future slag models may have multiple local minima in their
composition domains. Entry minimization should support structured
multi-start, grid-plus-refinement, or model-specific bracketing. The
Fe-O-H scripts already use this pattern and should inform the generic
implementation.

### 7.4 Initial guesses

The phase-aware formulation should reduce, but not eliminate, the burden
of user-supplied initial guesses.

The main improvement should come from separating phase selection from
the inner composition solve:

- inactive phases can be tested by phase-entry residuals rather than by
  tiny member amounts
- active solution phases can start from model-provided default
  compositions
- gas composition can be seeded from element balances and redox boundary
  estimates
- active-set continuation can add/drop phases one at a time and warm
  start the inner solve

The user should not need to provide delicate guesses such as tiny
positive amounts for every spinel member. The first implementation
should still expose optional initial guesses for advanced use and for
regression tests.

### 7.5 Reference states and source maps

Mixed-source calculations are unavoidable in current FPROPS work. The
phase-aware layer must preserve explicit source resolution and make it
easy to inspect which source was used for each phase/species.

Tests should catch:

- silent fallback to an unintended gas source
- missing `ref0` for gas equilibrium chemistry
- inconsistent source maps for named solution phases

### 7.6 Diagnostics and status codes

The phase-aware solver should not return only a generic NLP failure.
It should distinguish:

- invalid phase specification
- missing thermo data
- entry minimization failure
- active assemblage infeasible
- inner IPOPT failure
- validation failure
- property evaluation failure

A public `fprops_eqm_status_text(int status)` or equivalent should be
added so examples do not need local decoders.

Diagnostics should be available both as concise status text and as
structured detail. For long-running or difficult solves, a progress hook
should be able to report:

- current active assemblage
- candidate phase-entry residuals
- inner NLP status
- final validation residuals

### 7.7 Tolerances and importance weighting

Phase entry/exit tolerances should not be hardwired to a single mole or
Gibbs-energy number without scaling. Meaningful tolerances may differ
between major phases and trace-but-important components such as `P2O5`.

The first pass should define conservative defaults on thermodynamic
scales, for example residuals normalized by `R T`, element-balance
residuals scaled by element inventory, and phase-amount thresholds scaled
by total feed. It should also leave room for caller-supplied tolerances
or per-phase importance settings later.

For Fe-O-H, the first tolerance set can be simple and documented. Broader
slag and impurity work should revisit this with specific validation
cases.

### 7.8 Fe bcc/fcc handling

The first Fe-O-H implementation does not need to make Fe allotropy a
central feature. Acceptable near-term choices are:

- include both `Fe_bcc` and `Fe_fcc` as stoichiometric phases and allow
  the phase-entry logic to choose
- provide a named `iron` phase that selects the lower-Gibbs allotrope at
  the specified temperature
- use one selected Fe phase for the first reduction examples if the
  bcc/fcc distinction is not material to the test objective

For the reduction problems of immediate interest, Fe bcc/fcc equilibrium
is a second-order issue compared with oxide/gas phase selection.

### 7.9 Backwards compatibility

The current flat public APIs should remain available:

```c
fprops_eqm_tpb(...)
fprops_eqm_tpy(...)
fprops_rxn_eqm_tpb(...)
fprops_rxn_eqm_tpy(...)
```

The new solver should initially be opt-in, for example through a new API
or an explicit algorithm selector such as:

```text
phase_active
```

Only after Fe-O-H and gas/liquid regressions pass should `auto` consider
routing solution-phase packages through the new path.

### 7.10 File organization

The phase-aware implementation can start in new files, for example:

```text
eqm_phase.h
eqm_phase.c
```

or `eqmphase.*` if that better matches the local naming style. This
keeps the experimental path separated from the mature flat solver while
still allowing shared utilities to migrate later.

Once the phase-aware path is stable, common status handling, source
resolution, logging, validation, and IPOPT helpers can be unified.

### 7.11 ASCEND derivatives are deferred

ASCEND wrapping is not a first-pass requirement, but the design should
not block it. Existing external functions in `asc_fprops.c` generally
need first derivatives of each output with respect to each input. A
future ASCEND-facing phase-equilibrium external function would likely
need derivatives of phase amounts, phase compositions, and mixture
properties with respect to `T`, `P`, and element totals.

The C API should therefore keep enough solver state to support future
implicit KKT differentiation, but implementation of ASCEND derivative
callbacks is deferred until the C solver itself is reliable.

### 7.12 UNIFAC liquid phases

UNIFAC is not automatically a "shoe-in", but it should fit the
phase-aware design cleanly. The missing piece is not thermodynamics; the
missing piece is an adapter that presents a UNIFAC liquid as an
`FpropsEqmPhaseModel`-style phase:

- phase amount `N_liq`
- liquid composition variables, normally component mole fractions with
  one dependent coordinate removed
- a molar Gibbs energy `g_liq(T,P,x)` on a clear component basis
- component/member names for result reporting
- element contents inferred from component formulae
- optional mixture enthalpy/volume/property callbacks
- derivatives of `g_liq` with respect to composition where possible

For nonreactive VLE, the existing flash code remains valuable and should
not be forced through the chemical-equilibrium active-set solver. For
reactive or strongly coupled multiphase problems, the phase-aware solver
should be able to include a UNIFAC liquid phase together with vapor,
solid, and other liquid phases. The first practical target is therefore a
UNIFAC phase adapter plus regression tests, not a rewrite of the flash
solver.

Important issues:

- standard-state consistency between vapor species and liquid activity
  models must be explicit
- component names and formulae must resolve through the same registry used
  by reactive packages
- UNIFAC data should stay in generated FPROPS C data, not be owned by
  ASCEND instance trees
- composition bounds and trace components need tolerances suited to
  liquid activity models, not Fe-O-H oxide tolerances

### 7.13 Helmholtz pure fluids and vapor/liquid phases

The older Helmholtz work supports pure-fluid vapor/liquid properties and
two-phase flash behavior. That remains useful for power cycles and should
not be discarded. It is a different use case from general multiphase
chemical equilibrium:

- a pure-fluid flash usually solves for thermodynamic state and phase
  split of one substance using a dedicated equation of state
- the phase-aware equilibrium solver chooses among candidate chemical
  phases and solution phases by minimizing total Gibbs energy subject to
  conserved elements

The sensible harmonization is to share phase-model concepts without
forcing every power-cycle property call through the general active-set
chemical solver. A Helmholtz-backed phase model should be able to expose:

- `pure:helmholtz:vapor(name)` and `pure:helmholtz:liquid(name)` style
  phase specifications, or an equivalent structured builder
- branch-aware molar Gibbs energy and properties for the requested phase
- a stable or metastable branch policy that is explicit in diagnostics
- optional admission into a phase-equilibrium package when a user really
  wants a pure vapor/liquid phase as part of a larger equilibrium problem

The current `helmholtz+ref0:` ideal-gas equilibrium path should also be
kept conceptually separate from "use the full Helmholtz EOS as a gas
phase." In the current gas-equilibrium work, Helmholtz data are mainly a
source for formation-consistent standard chemical potentials; the mixture
phase is still an ideal gas mixture. A future real-fluid vapor phase would
need a different phase model and fugacity/activity treatment.

### 7.14 Fate of phase-agnostic flat solvers

The raw flat solvers should remain internal for now, but they should not
be the main public story. They still provide value:

- fast, compact gas-only and simple species-basis equilibrium solves
- regression continuity for the existing gas equilibrium tests
- a fixed-active inner solve kernel used by phase-aware enumeration and
  active-set paths
- a simpler target for derivative work and ASCEND black-box experiments
- a reference implementation for cases where every species amount is a
  legitimate independent variable

The public API should prefer `fprops_eqm_tpb(...)`, `fprops_eqm_tpy(...)`,
package-backed solvers, and the phase-aware `FpropsEqm` workflow. The
direct raw routines have already moved to `eqm_internal.h`; if future work
proves that all useful callers go through package or phase APIs, the raw
entry points can be further reduced or made file-local.

### 7.15 String selectors versus enums

C code is better at checking enums than short strings. The current string
selectors are convenient at the outer API boundary, especially for command
line tools, source maps, ASCEND strings, and examples. They are less
attractive deep inside numerical code because typos are runtime errors and
every solver path has to repeat selector parsing.

Preferred direction:

- public APIs may continue to accept strings such as `"auto"` for
  ergonomic source/configuration entry
- `FpropsEqm` should parse the string once into an internal enum field
  such as `FPROPS_EQM_STRATEGY_AUTO`,
  `FPROPS_EQM_STRATEGY_ACTIVE_SET`, or
  `FPROPS_EQM_STRATEGY_ENUMERATE`
- inner solvers should switch on enums, not strings
- status/error codes should be named enum or macro constants, with
  `fprops_eqm_status_text(...)` as the human-readable decoder
- source names and species/phase names remain strings because they are
  data identifiers, not control-flow switches

This gives static checking and clearer internal control flow without
making the user-facing API awkward.

### 7.16 Revisit earlier examples

The phase-aware API should be checked against earlier FPROPS equilibrium
and mixture examples. Good candidates:

- water/alcohol or ethanol/water mixtures, using existing UNIFAC flash
  tests as the oracle
- Ni/O or Ni/NiO/H2 equilibrium, which was one of the early mixed-source
  Gibbs minimization examples
- gas-only water-gas shift and ammonia examples, to ensure the phase-aware
  work has not degraded the flat/package path
- Fe-O-C and Fe-O-H cases already added for BG truth-checking

The goal is not to route everything through the phase-aware solver
immediately. The goal is to identify which examples are naturally
flat-species, which are naturally phase-aware, and which need a mixture
phase adapter before they can be represented cleanly.

### 7.17 ASCEND exposure plan

The efficient ASCEND path should be package-backed and stateful:

- build or resolve a phase package once as `DATA`
- pass `T`, `P`, and feed amounts or element totals as inputs
- return selected phase amounts, phase composition coordinates, selected
  member/component amounts, and optional mixture properties as outputs
- expose concise status/diagnostic outputs so failed phase selection is
  visible from ASCEND
- cache the last successful solution for warm starts, but make cache
  lifetime and reset behavior explicit

The first ASCEND binding should probably expose a small fixed-shape
package rather than a fully dynamic result table. ASCEND needs known
variable dimensions at compile time. A useful initial blackbox would take
one named package key and return a fixed list of outputs matching that
package's known phases and coordinates.

Derivative callbacks are the main blocker to robust ASCEND use. A
non-derivative blackbox can be useful for exploratory single-point work,
but equation-based flowsheets need consistent first derivatives.

### 7.18 Derivative status and gaps

Current derivative status:

- gas-only/package equilibrium has some sensitivity support for simple
  package cases without solution phases
- the phase-aware active-set solver does not yet expose stable first
  derivatives of phase amounts or phase coordinates with respect to `T`,
  `P`, or feed totals
- derivatives are discontinuous at true phase-entry/exit boundaries, so
  ASCEND callbacks need a clear policy for active-set changes
- finite differences remain useful for testing, but are not a sufficient
  final derivative strategy for robust ASCEND blackboxes

Near-term derivative targets:

- analytic derivatives for ideal gas mixture phase `g(T,P,y)`
- analytic or carefully verified derivatives for binary wustite
- a finite-difference versus analytic derivative test harness for phase
  `g`, entry residuals, and fixed-active KKT sensitivities
- fixed-active implicit derivatives first; active-set switching can be
  treated as piecewise smooth with diagnostics near boundaries
- only after that, ASCEND derivative callbacks for a small fixed phase
  package

### 7.19 Lambda tutorial and thermodynamic meaning

In this phase-equilibrium work, `lambda_e` means the thermodynamic
potential of conserved element `e`. It has units of J/mol of element and
answers this question:

```text
If I add a tiny amount of element e to the closed equilibrium problem at
fixed T and P, how much does the minimum Gibbs energy change?
```

For a simple species-basis problem:

```text
minimize    G = sum_i n_i mu_i
subject to  sum_i A[e,i] n_i = b[e]
            n_i >= 0
```

`A[e,i]` is the number of atoms or moles of element `e` in one mole of
species `i`. At equilibrium, any species that is present must have a
chemical potential equal to the sum of the element potentials needed to
make it:

```text
mu_i = sum_e A[e,i] lambda_e       for present species i
```

For a species that is absent, the same comparison becomes an entry test:

```text
mu_i - sum_e A[e,i] lambda_e >= 0
```

If the left-hand side were negative, one mole of that species would have
less Gibbs energy than the corresponding elements as priced by `lambda`;
forming it would lower total `G`, so the current assemblage could not be
stable.

For a whole phase `p`, one formula unit or mole of phase has element
contents `a_e(y)` that may depend on internal composition `y`. The phase
entry residual is:

```text
phi_p = min_y [ g_p(T,P,y) - sum_e lambda_e a_e(y) ] / (R T)
```

Interpretation:

- `phi_p > 0`: the phase is not stable enough to enter, within tolerance
- `phi_p = 0`: the phase is on its entry boundary
- `phi_p < 0`: the phase can lower total Gibbs energy and should be added
  or considered in an active-set move

This is not just a convenience output. It is the thermodynamic test used
to decide whether an inactive phase should enter the equilibrium
assemblage.

Sign convention note: older optimization derivations often write the
Lagrangian with `+ lambda_math^T(A n - b)`, leading to
`mu + A^T lambda_math = 0` for present species. The phase API uses the
thermodynamic sign:

```text
lambda_thermo = -lambda_math
mu - A^T lambda_thermo = 0
```

The thermodynamic sign is more intuitive here because `lambda_e` directly
acts like an elemental chemical potential and because phase entry is then
written as `g - A lambda`.

## 8. Validation Assets

Existing files that should become regression oracles:

- `test/feoh_hydrogen_boundary.py`
- `test/feoxide_potential_boundaries.py`
- `test/feoh_baur_glaessner_compare.py`
- `test/feoc_baur_glaessner_compare.py`
- `test/feo_hidayat_validation.py`
- `test/feoxide_hidayat_compare.py`
- [SLAG.md](SLAG.md) for Fe-O-Si-Al follow-on requirements
- [EQUIL.md](EQUIL.md) and [A4EQUIL.md](A4EQUIL.md) for current equilibrium
  API context

The boundary scripts should eventually become tests of the phase-entry
layer, not separate replacement solvers for ordinary equilibrium cases.

The primary automated coverage should be C-level unit and regression
tests. Python scripts may remain useful as data-generation or comparison
oracles, but the phase-aware solver should not depend on Python for
normal testing.

## 9. Current Unfinished Business

Phase 4b is functionally useful but not finished:

- add denser boundary-near BG truth checks around the Fe/FeO,
  Fe/Fe3O4, and FeO/Fe3O4 regions
- add more Fe-O-C checks on both sides of the current 585 C and 700 C
  wustite points
- add deliberately perturbed warm-start tests, not only initial-mask
  tests
- decide whether public `auto` should call active-set first, enumeration
  first, or expose explicit strategies
- add continuation in temperature/feed severity where active-set
  convergence is sensitive

Phase 5 follow-up:

- consider a heap-allocated `FpropsEqm` constructor/destructor pair for
  callers that cannot comfortably stack-allocate the problem object
- use the new Python phase bindings to add FPROPS phase-module curves to the
  existing Baur-Glaessner / Spreitzer comparison scripts, rather than
  maintaining separate Python boundary solvers as the only plotting route
- add install rules for public phase headers and examples if FPROPS
  install packaging is enabled later
- add scripted example-output regression once the build system has a
  standard example run-test hook
- add JSON/YAML result writers only when a concrete downstream consumer
  needs them

Phase 6 and beyond:

- build a UNIFAC liquid phase adapter and prove it on an ethanol/water
  style case
- define branch-aware Helmholtz vapor/liquid phase specifications
- revisit early equilibrium examples through the current public API
- move internal strategy selection from repeated string tests to enums
- design the first ASCEND phase-equilibrium blackbox around a fixed
  package with known output shape
- implement fixed-active first derivatives before promising robust
  ASCEND derivative callbacks
- keep IPOPT as an opt-in diagnostic/cross-check path unless a future case
  shows a clear reliability advantage over the much faster SLSQP/default path

## 10. Immediate Next Actions

1. Add enum-backed solver strategy storage inside `FpropsEqm`, while
   preserving string parsing at the public boundary.
2. Revisit early examples:
   - gas-only WGS/ammonia through package APIs
   - Ni/O or Ni/NiO/H2 through the updated public API
   - ethanol/water or similar as the first UNIFAC phase-adapter target
3. Add a first UNIFAC-liquid `FpropsEqmPhaseModel` adapter, initially for
   nonreactive phase representation and result reporting.
4. Sketch Helmholtz pure-fluid phase specs and decide which existing
   flash functionality stays separate from the general phase solver.
5. Add a fixed-active derivative test harness for ideal gas and wustite
   phase models.
6. Draft the first ASCEND blackbox signature for a fixed Fe-O-H phase
   package before implementing callback code.

## 11. Current Working State

- The public phase-aware C API is available in `eqm_phase.h`.
- The phase-aware API is also available to Python through the SWIG
  `fprops.Eqm` and `fprops.EqmPhaseResult` classes.
- Low-level phase solvers and lambda/entry diagnostics are internal in
  `eqm_phase_internal.h`.
- Raw flat solvers are internal in `eqm_internal.h`; public flat/package
  callers should use `fprops_eqm_tpb(...)`, `fprops_eqm_tpy(...)`,
  `fprops_rxn_eqm_tpb(...)`, or `fprops_rxn_eqm_tpy(...)`.
- The standalone `examples/feoh.c` example builds without ASCEND and
  demonstrates reducing Fe+gas, wustite+gas, and spinel+gas cases.
- Current CUnit coverage includes Fe-O-H and Fe-O-C active-set checks in
  the 400 C to 900 C range, including CO/CO2 gas cases.
- Derivative support is still the largest gap before robust ASCEND
  exposure.

# Phase-Aware Equilibrium Plan

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
  currently fails through the flat `eqm_solve_elements` path
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
Given element potentials `lambda_e`, a phase wants to enter if:

```text
min_y [ g_p(T, P, y) - sum_e lambda_e a_e(y) ] < 0
```

within tolerance, where `a_e(y)` is the element content of one unit of
phase at composition `y`.

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
- improved trace output for:
  - algorithm dispatch
  - phase/source resolution
  - active set decisions
  - validation failure reason
- C-level logging/progress callback hooks for phase-aware solves
- add a `libfprops.so` build target, separate from `libfprops_ascend.so`,
  suitable for standalone C examples

Tests:

- `~/feoh/feoh.c` prints meaningful status text for current failures
- status text is available from FPROPS rather than duplicated in each
  example program
- a standalone C example can link against `libfprops.so` without ASCEND
- logging callback tests can capture at least one phase-resolution event
  and one solver-status event

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

Tests:

- wustite registry maps to `Wus_FeO` / `Wus_FeO1p5`
- spinel registry maps to the five Degterov members
- spinel registry reports site and charge constraints
- H2/H2O gas source resolves to `helmholtz+ref0:`
- `g(T,P,y)` finite-value tests at `900 C`

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
- element balance residuals are below tolerance
- internal composition variables remain within valid domains

Exit criteria:

- the inner solver is reliable when given the correct active phases

### Phase 4: Phase-level active set

Deliverables:

- add/drop whole phases based on entry residuals and phase amounts
- warm-start inner solves after phase additions/removals
- rank competing phase-entry candidates
- add continuation where needed in temperature or feed severity
- implement final validation:
  - element balance
  - active phase stationarity
  - inactive phase entry residuals
  - internal composition bounds

Tests:

- the strongly reducing `feoh.c` case solves from a broad initial guess
  without manually specifying `Fe + gas`
- less reducing Fe-O-H cases activate wustite/spinel/hematite as
  expected from boundary logic
- results are insensitive to small perturbations in the initial guess
- results remain stable when inactive phases are present in the package
- active-set trace shows meaningful phase additions/removals

Exit criteria:

- Fe-O-H can be solved by specifying the package and element totals,
  rather than by custom boundary scripts or manually selected active
  assemblages

### Phase 5: Public API and examples

Deliverables:

- add a documented phase-aware C API
- add string and/or structured package builders
- update or replace `~/feoh/feoh.c` with a concise FPROPS-only example
- add an in-tree standalone example, or test target, that demonstrates
  the same workflow as `~/feoh/feoh.c`
- ensure `libfprops.so` exports the required phase-aware API symbols
- add documentation explaining when to use:
  - flat `eqm_solve_elements`
  - phase-aware solver
  - boundary diagnostic scripts

Tests:

- standalone Fe-O-H example builds without ASCEND
- standalone Fe-O-H example links against `libfprops.so`
- example reports:
  - Fe amount
  - unreduced hematite
  - wustite amount/composition
  - spinel amount/site composition
  - H2/H2O gas amounts
- example succeeds with `helmholtz+ref0:` gas
- example handles at least one oxidizing, one intermediate, and one
  reducing case

Exit criteria:

- the original user-facing Fe-O-H request is satisfied with a small C
  example and no ASCEND dependency

### Phase 6: Broader mixture and slag preparation

Deliverables:

- add generic ideal gas mixture support beyond H2/H2O
- add generic ideal liquid or ideal solution constructor if needed
- define the first slag/ore-capture phase interface requirements using
  `SLAG.md`
- add Fe-O-Si-Al package construction tests

Tests:

- ideal gas reaction equilibria remain consistent with existing gas-only
  regressions
- ethanol/water or another simple nonreactive mixture can be represented
  as phases without breaking current flash work
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

The current flat APIs should remain available:

```c
eqm_solve(...)
eqm_solve_elements(...)
fprops_eqm_tpb(...)
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

## 8. Validation Assets

Existing files that should become regression oracles:

- `test/feoh_hydrogen_boundary.py`
- `test/feoxide_potential_boundaries.py`
- `test/feoh_baur_glaessner_compare.py`
- `test/feoc_baur_glaessner_compare.py`
- `test/feo_hidayat_validation.py`
- `test/feoxide_hidayat_compare.py`
- `SLAG.md` for Fe-O-Si-Al follow-on requirements
- `EQUIL.md` and `A4EQUIL.md` for current equilibrium API context

The boundary scripts should eventually become tests of the phase-entry
layer, not separate replacement solvers for ordinary equilibrium cases.

The primary automated coverage should be C-level unit and regression
tests. Python scripts may remain useful as data-generation or comparison
oracles, but the phase-aware solver should not depend on Python for
normal testing.

## 9. Open Questions

- What is the first stable C syntax for phase packages: strings,
  structured C builders, or both?
- Should the phase-aware solver live in `eqm_phase.c`, `eqmphase.c`, or
  another new file name?
- What tolerances should define phase entry/exit for condensed phases at
  high temperature, and how should tolerances account for
  low-concentration but high-importance components?
- How should metallic Fe bcc/fcc be represented: two stoichiometric
  phases, or a named iron phase that selects the stable allotrope?
- How much of the current reduced active-set code can be directly reused
  versus adapted conceptually?
- Which derivatives are worth hand-coding first, and when would autodiff
  or generated derivatives become justified?
- What minimum solver-state record is needed now so that future ASCEND
  derivative callbacks can be implemented without changing the C API?

## 10. Immediate Next Actions

1. Add an FPROPS-owned equilibrium status decoder and use it in
   examples/tests.
2. Add a standalone `libfprops.so` build target, independent of
   `libfprops_ascend.so`.
3. Add a small phase-registry prototype for Fe-O-H:
   `Fe_bcc`, `Fe_fcc`, `Fe2O3`, `wustite`, `spinel`, and H2/H2O gas.
4. Expose test-only inspection functions for phase resolution and
   phase `g(T,P,y)`.
5. Port Fe-O-H boundary residual checks into C-level tests on a common
   `lambda_O` basis.
6. Implement a fixed-active-assemblage solve for `Fe + H2/H2O` and prove
   the strongly reducing hematite-plus-hydrogen case.
7. Add whole-phase entry tests for wustite and spinel.
8. Add an in-tree standalone C example that links against `libfprops.so`
   and mirrors the intended `feoh.c` user workflow.
9. Only then add automatic phase activation/removal.

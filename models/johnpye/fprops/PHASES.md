# FPROPS Phase-Aware Equilibrium

This document introduces the phase-aware chemical-equilibrium layer in FPROPS.
It describes the thermodynamic ideas, the current C and Python APIs, what has
been validated, and the main items that remain future work.

Archived implementation notes and decision history are kept in
[PHASES-impl.md](PHASES-impl.md).

The implementation is stable enough for current Fe-O-H and Fe-O-C work. It is
not intended to replace every older equilibrium or flash pathway in FPROPS, but
it is now the preferred API when the problem is naturally expressed as phases
rather than as a flat list of independent species.

## 1. What This Layer Solves

The older equilibrium kernel is a flat species-amount Gibbs minimizer:

```text
minimize G(T, P, n)
subject to A n = b
           n >= 0
```

That form works well for gas species and stoichiometric condensed species. It
is awkward for condensed solution phases, because the solver sees each
endmember or site member as a separate amount. For the Fe-O-H system, a user
thinks in terms of:

```text
Fe metal
hematite
wustite
spinel / magnetite
H2/H2O gas
```

The flat representation expands this into lower-level entries such as:

```text
Fe_bcc
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

This is numerically fragile near phase disappearance. A solution phase can have
zero total amount while its internal composition is still the meaningful
quantity for deciding whether that phase should enter. Spinel is especially
awkward because its expanded members must also respect site and charge-balance
relationships.

The phase-aware layer lets callers build a package of whole phases, add a feed
as formulae, species, elements, or phase amounts, and solve for:

- which phases are active
- how much of each phase is present
- each active phase's internal coordinates
- expanded member/component amounts where those are useful for reporting
- gas member amounts and fractions

The first substantial target was Fe-O-H reduction. The same machinery has also
been checked against Fe-O-C cases using Fe, Fe oxides, CO, and CO2.

## 2. Thermodynamic Basics

### 2.1 Phase-Aware Formulation

The high-level phase formulation is:

```text
minimize sum_p N_p g_p(T, P, y_p)
subject to element balances
           N_p >= 0
           y_p in valid phase domain
```

where:

- `p` is a phase
- `N_p` is the phase amount
- `y_p` is the phase's internal composition or coordinate vector
- `g_p(T,P,y_p)` is the Gibbs energy of one natural unit of that phase
- the element content of a phase unit is `a_e(y_p)`

Stoichiometric phases have no internal coordinates. Wustite has one
composition coordinate, currently `x_FeO1p5`. Spinel has two reduced site
coordinates, currently `y_tet_fe2` and `y_oct_fe2`. An ideal gas mixture is
also a solution phase; its coordinates are the gas member mole fractions.

### 2.2 Phase Amounts, Coordinates, and Members

The API deliberately distinguishes:

- **phase amount**: total amount of a phase, such as moles of wustite
- **phase coordinate**: an internal composition variable, such as
  `x_FeO1p5`
- **phase member amount**: an expanded amount associated with a member or
  component, such as `Wus_FeO1p5`, `hydrogen`, or `water`

For binary wustite the relationship is simple:

```text
N_wus = N_FeO + N_FeO1p5
x_FeO1p5 = N_FeO1p5 / N_wus
```

For spinel the relationship is not a simple "members sum to the phase total"
rule. The spinel model uses site-related members:

```text
Sp_Fe2_tet
Sp_Fe3_tet
Sp_Fe2_oct
Sp_Fe3_oct
Sp_Va_oct
```

One spinel formula unit has one tetrahedral site and two octahedral sites. In a
typical result the tetrahedral-site member amounts sum to about `N_spinel`,
while the octahedral-site member amounts sum to about `2 N_spinel`. Therefore
the expanded spinel member amounts can sum to about `3 N_spinel`, not
`N_spinel`. This is expected. It is why callers should use phase amounts for
phase inventory, phase coordinates for spinel composition, and member amounts
only when they really want the expanded site/component accounting.

### 2.3 Lambda

In this phase-equilibrium work, `lambda_e` is the thermodynamic potential of
conserved element `e`. It has units of J/mol of element and answers:

```text
At fixed T and P, how much would the minimized Gibbs energy change if I added
a tiny amount of element e to the closed equilibrium problem?
```

For a species-basis problem:

```text
minimize    G = sum_i n_i mu_i
subject to  sum_i A[e,i] n_i = b[e]
            n_i >= 0
```

`A[e,i]` is the amount of element `e` in one mole of species `i`. At
equilibrium, any present species satisfies:

```text
mu_i = sum_e A[e,i] lambda_e
```

For an absent species, the same comparison becomes an entry test:

```text
mu_i - sum_e A[e,i] lambda_e >= 0
```

If the left side is negative, forming that species would lower total Gibbs
energy, so the current assemblage is not stable.

For a whole phase with internal composition `y`, one phase unit has element
contents `a_e(y)`. The phase entry residual is:

```text
phi_p = min_y [ g_p(T,P,y) - sum_e lambda_e a_e(y) ] / (R T)
```

Interpretation:

- `phi_p > 0`: the phase is not stable enough to enter, within tolerance
- `phi_p = 0`: the phase is on its entry boundary
- `phi_p < 0`: the phase can lower total Gibbs energy and should be added or
  considered in an active-set move

This is not just diagnostic decoration. It is the thermodynamic test used by
the active-set logic to decide whether an inactive phase should enter.

### 2.4 Lambda Sign Convention

Some optimization derivations define the Lagrangian with:

```text
L = G + lambda_math^T (A n - b)
```

This gives stationarity:

```text
mu + A^T lambda_math = 0
```

The FPROPS phase API uses the thermodynamic sign:

```text
lambda_thermo = -lambda_math
mu - A^T lambda_thermo = 0
```

This sign convention is used because `lambda_e` then directly behaves like an
elemental chemical potential, and the phase-entry expression has the intuitive
form:

```text
g_phase - A lambda
```

### 2.5 Source Maps and Reference States

Thermodynamic source selection remains explicit. Typical Fe-O-H phase packages
use:

```text
Fe_bcc=hidayat_2015
wustite=hidayat_2015
spinel=degterov_2001
Fe2O3=hidayat_2015
gas:ideal(hydrogen,water)=helmholtz+ref0:
```

For the current gas-equilibrium work, `helmholtz+ref0:` means the gas mixture
is still an ideal gas mixture, but the gas species standard chemical
potentials are evaluated using the Helmholtz/ref0 source. This should not be
confused with a real-fluid Helmholtz vapor phase.

The same phase machinery can use other gas member lists, for example:

```text
gas:ideal(carbonmonoxide,carbondioxide)=helmholtz+ref0:
```

The Fe-O-C BG checks deliberately use only Fe, Fe oxides, CO, and CO2. They do
not include free carbon or Fe3C.

## 3. Current Implementation

The main public header is:

```c
#include "eqm_phase.h"
```

The current implementation provides:

- predefined stoichiometric Fe phases and oxide phases used in Fe-O-H/Fe-O-C
- predefined wustite and spinel solution phases
- ideal gas mixture phases over supported FPROPS gas species
- a `FpropsEqm` problem object that owns the phase package, element ordering,
  feed totals, temperature, pressure, and solver choices
- result accessors for phase amounts, phase coordinates, member names, member
  amounts, and member fractions
- Python/SWIG bindings for the same first-pass phase API

Internally, small phase packages can be solved by whole-phase subset
enumeration or by a multiplier-driven active-set loop. Low-level active-set,
lambda reconstruction, validation, and fixed-active helpers live in
`eqm_phase_internal.h`; they are intentionally not the user-facing API.

Raw flat equilibrium internals have moved to `eqm_internal.h`. Public callers
that need the older gas/package APIs should continue to use:

```c
fprops_eqm_tpb(...)
fprops_eqm_tpy(...)
fprops_rxn_eqm_tpb(...)
fprops_rxn_eqm_tpy(...)
eqm_mu0_source(...)
```

## 4. C API Summary

### 4.1 Problem Setup

Create and populate a phase-equilibrium problem:

```c
FpropsEqm eqm;
fprops_eqm_init(&eqm);

fprops_eqm_add_phases(&eqm,
    "Fe_bcc=hidayat_2015",
    "wustite=hidayat_2015",
    "spinel=degterov_2001",
    "Fe2O3=hidayat_2015",
    "gas:ideal(hydrogen,water)=helmholtz+ref0:");
```

Useful setup functions:

- `fprops_eqm_init(&eqm)`
- `fprops_eqm_add_phase(&eqm, spec, source)`
- `fprops_eqm_add_phases(&eqm, spec0, spec1, ...)`
- `fprops_eqm_phase_count(&eqm)`
- `fprops_eqm_phase_name(&eqm, iphase)`
- `fprops_eqm_find_phase(&eqm, "wustite")`
- `fprops_eqm_phase_model(&eqm, "wustite")`

The convenience macros such as `fprops_eqm_add_phases(...)` and
`fprops_eqm_add_comps(...)` expand to counted arrays of typed structs. They do
not use raw varargs and do not require a `NULL` sentinel.

### 4.2 Feed Setup

Clear and add feed material:

```c
fprops_eqm_clear_feed(&eqm);
fprops_eqm_add_comps(&eqm, "Fe2O3", 1.0, "H2", 100.0);
```

Supported feed styles:

- direct element totals:
  `fprops_eqm_add_comps(&eqm, "Fe", 2.0, "O", 3.0, "H", 200.0)`
- formula/species inputs:
  `fprops_eqm_add_comps(&eqm, "Fe2O3", 1.0, "H2", 100.0)`
- one phase at coordinates in declared order:
  `fprops_eqm_add_phase_feed(&eqm, "wustite", 1.0, x)`
- one phase with named coordinates:
  `fprops_eqm_add_phase_feed_vars(&eqm, "spinel", 1.0,
  "y_tet_fe2", 0.40, "y_oct_fe2", 0.20)`

Lower-level feed functions are also available:

- `fprops_eqm_add_element(&eqm, element, amount)`
- `fprops_eqm_set_element(&eqm, element, amount)`
- `fprops_eqm_element_amount(&eqm, element)`
- `fprops_eqm_add_formula(&eqm, formula, amount)`
- `fprops_eqm_add_comp_list(&eqm, n, names, amounts)`
- `fprops_eqm_add_comp_items(&eqm, n, items)`

Formula/species/member names are resolved through FPROPS metadata first, then
through the literal formula parser where appropriate.

### 4.3 Solving and Solver Selection

Solve at fixed `T` and `P`:

```c
FpropsEqmPhaseResult result;
int status = fprops_eqm_solve_TP(&eqm, T, P, &result);
```

Useful functions:

- `fprops_eqm_set_TP(&eqm, T, P)`
- `fprops_eqm_solve(&eqm, &result)`
- `fprops_eqm_solve_TP(&eqm, T, P, &result)`
- `fprops_eqm_status_ok(status)`
- `fprops_eqm_status_text(status)`

NLP backend selection is runtime-configurable:

- `fprops_eqm_set_nlp_solver(&eqm, FPROPS_EQM_NLP_AUTO)`
- `fprops_eqm_set_nlp_solver_name(&eqm, "auto")`
- `fprops_eqm_set_nlp_solver_name(&eqm, "slsqp")`
- `fprops_eqm_set_nlp_solver_name(&eqm, "ipopt")`
- `fprops_eqm_nlp_solver(&eqm)`
- `fprops_eqm_nlp_solver_name(solver)`

The default `auto` choice is NLOPT/SLSQP when NLOPT is available. If IPOPT is
explicitly requested, the call uses IPOPT only; there is no hidden fallback from
one NLP backend to the other inside a single solve.

### 4.4 Result Access

The result stores a backpointer to its source `FpropsEqm`, so accessors do not
require the caller to pass both the problem and the result.

Useful result functions:

- `fprops_eqm_phase_amount(&result, "wustite")`
- `fprops_eqm_phase_coord(&result, "wustite", "x_FeO1p5")`
- `fprops_eqm_phase_coord_values(&result, "spinel", values)`
- `fprops_eqm_phase_member_amount(&result, "gas:ideal", "hydrogen")`
- `fprops_eqm_phase_member_fraction(&result, "gas:ideal", "water")`
- `fprops_eqm_phase_member_amounts(&result, "spinel", amounts)`
- `fprops_eqm_write(stdout, &result, "text")`

Name discovery functions:

- `fprops_eqm_phase_coord_count(&eqm, "spinel")`
- `fprops_eqm_phase_coord_name(&eqm, "spinel", i)`
- `fprops_eqm_phase_coord_names(&eqm, "spinel", names)`
- `fprops_eqm_phase_member_count(&eqm, "gas:ideal")`
- `fprops_eqm_phase_member_name(&eqm, "gas:ideal", i)`
- `fprops_eqm_phase_member_names(&eqm, "gas:ideal", names)`

`fprops_eqm_write(..., "text")` currently provides centralized text output.
The `format` parameter is intentionally present so JSON/YAML or other
machine-readable formats can be added later without changing the entry point.

### 4.5 Phase Model Inspection

For diagnostics, examples, and tests:

- `fprops_eqm_phase_resolve(spec, source, &phase)`
- `fprops_eqm_phase_kind_name(kind)`
- `fprops_eqm_phase_find_element(&phase, "Fe")`
- `fprops_eqm_phase_elements(&phase, y, a_out)`
- `fprops_eqm_phase_gibbs(&phase, T, P, y, &g)`
- `fprops_eqm_phase_entry_residual(&phase, T, P, lambda, &phi, y_min)`

These functions are useful when constructing feeds from known buffer
conditions, as in the Fe-O-H example.

## 5. Python API Summary

The SWIG wrapper exposes a compact Python API in `models/johnpye/fprops/python`.

Basic setup:

```python
import fprops

eqm = fprops.Eqm()
eqm.add_phases([
    "Fe_bcc=hidayat_2015",
    "wustite=hidayat_2015",
    "spinel=degterov_2001",
    "Fe2O3=hidayat_2015",
    "gas:ideal(hydrogen,water)=helmholtz+ref0:",
])

eqm.clear_feed()
eqm.add_comps([("Fe2O3", 1.0), ("H2", 100.0)])
result = eqm.solve_TP(1173.15, 101325.0)
```

Problem methods:

- `Eqm.add_phase(spec, source=None)`
- `Eqm.add_phases([spec0, spec1, ...])`
- `Eqm.phase_count()`
- `Eqm.phase_name(i)`
- `Eqm.phase_names()`
- `Eqm.find_phase(name)`
- `Eqm.phase_coord_names(phase)`
- `Eqm.phase_member_names(phase)`
- `Eqm.set_TP(T, P)`
- `Eqm.set_algorithm(name)`
- `Eqm.set_nlp_solver("auto" | "slsqp" | "ipopt" | ...)`
- `Eqm.nlp_solver()`
- `Eqm.clear_feed()`
- `Eqm.add_element(element, amount)`
- `Eqm.set_element(element, amount)`
- `Eqm.element_amount(element)`
- `Eqm.add_formula(formula, amount)`
- `Eqm.add_comps([(name, amount), ...])`
- `Eqm.add_phase_feed(phase, amount, [coord0, ...])`
- `Eqm.add_phase_feed_vars(phase, amount, {"coord": value, ...})`
- `Eqm.solve()`
- `Eqm.try_solve()`
- `Eqm.solve_TP(T, P)`

Result methods:

- `result.status()`
- `result.solver_status()`
- `result.status_text()`
- `result.phase_count()`
- `result.phase_names()`
- `result.phase_amount(phase)`
- `result.phase_active(phase)`
- `result.phase_coord_names(phase)`
- `result.phase_coord(phase, coord)`
- `result.phase_coords(phase)`
- `result.phase_member_names(phase)`
- `result.phase_member_amount(phase, member)`
- `result.phase_member_fraction(phase, member)`
- `result.phase_member_amounts(phase)`

Phase inspection:

```python
phase = fprops.EqmPhase("wustite=hidayat_2015")
phase.element_names()
phase.coord_names()
phi, y_min = phase.entry_residual(T, P, lambda_values)
```

Useful module-level functions and constants:

- `fprops.eqm_status_text(status)`
- `fprops.eqm_status_ok(status)`
- `fprops.eqm_nlp_solver_name(solver)`
- `fprops.mu0_source(name, source, T, P=100000.0)`
- `fprops.FPROPS_R`

## 6. Examples

Two Fe-O-H examples are provided:

- `models/johnpye/fprops/examples/feoh.c`
- `models/johnpye/fprops/examples/feoh.py`

Both examples demonstrate:

- building the Fe-O-H phase package
- adding a strongly reducing feed as `Fe2O3 + H2`
- constructing an intermediate wustite-buffered feed from a Baur-Glaessner
  gas ratio
- adding a spinel feed with named phase coordinates
- solving at fixed `T` and `P`
- reporting phase amounts, wustite composition, spinel site coordinates, and
  H2/H2O gas amounts

The C example is the in-tree replacement for the earlier out-of-tree
`~/feoh/feoh.c` workflow. It links against `libfprops.so` rather than compiling
FPROPS objects directly into the example, and it can be built without ASCEND:

```bash
scons -C models/johnpye/fprops WITH_ASCEND=0 examples/feoh
```

The Python example uses the SWIG phase API:

```bash
PYTHONPATH=models/johnpye/fprops/python \
    python3 models/johnpye/fprops/examples/feoh.py
```

## 7. Solver Notes: NLOPT/SLSQP and IPOPT

The phase-equilibrium workload currently strongly favors NLOPT/SLSQP.

On the local development machine during this work:

- the phase CUnit suite took roughly `0.6 s` with SLSQP
- the same broad phase suite took roughly `180 s` with IPOPT-only
- selected active-set boundary cases took roughly `0.2 s` with SLSQP and
  roughly `63 s` with IPOPT-only

IPOPT initially also showed system-dependent robustness differences on some
active-set cases. The IPOPT path was then harmonized with the SLSQP path so
plain `ipopt` and explicit `ipopt_scaled_n` use the scaled amount-variable
formulation. This removed the observed IPOPT robustness split in the checked
phase cases, but it did not close the performance gap.

Current policy:

- build with both NLOPT/SLSQP and IPOPT when both are available
- default `auto` phase-equilibrium solves use SLSQP when available
- explicit IPOPT selection remains available for cross-checking and future
  difficult cases
- default CUnit coverage includes a few guarded explicit-IPOPT smoke checks
  when IPOPT is compiled in, but it does not duplicate the whole phase suite
  through IPOPT
- a solve uses the requested NLP backend only; it does not silently fall back
  to another backend

IPOPT can be disabled for FPROPS with:

```bash
WITH_FPROPS_IPOPT=0
```

## 8. Validation and Current Coverage

Current CUnit and script coverage includes:

- phase registry and inspection for Fe-O-H phases
- phase Gibbs-energy and element-content checks
- Fe/wustite and wustite/spinel phase-entry anchors
- fixed-active solves for `Fe + gas`, `wustite + gas`, and `spinel + gas`
- active-set solves in the 400 C to 900 C range
- Fe-O-H BG-positioned checks using H2/H2O gas
- Fe-O-C BG-positioned checks using CO/CO2 gas
- low-temperature Fe/spinel checks at 400 C, 500 C, 540 C, and 560 C
- wustite-field checks around 585 C, 600 C, 700 C, and 900 C
- formula, species/member, and direct-element feed setup
- phase/member name discovery and result backpointer access
- default SLSQP tests plus selected guarded IPOPT smoke tests

The comparison scripts now support three-way BG plots:

- Baur-Glaessner / Spreitzer reference data
- the older Python boundary diagnostic curve
- the current FPROPS phase API curve

The current plot scripts are:

- `test/feoh_baur_glaessner_compare.py`
- `test/feoc_baur_glaessner_compare.py`
- `test/fprops_phase_boundary.py`

The older Python boundary diagnostic curves remain useful for cross-checking.
The wustite-spinel diagnostic continuation is now traced on a dense internal
temperature grid before sampling requested temperatures, so sparse and dense
plot requests compare the same curve.

## 9. What Is General and What Is Fe-O-H-Specific

General-purpose parts:

- `FpropsEqm` phase package setup
- element-balance Gibbs minimization over phases
- feed setup from elements, formulae/species, or phase amounts
- ideal gas mixtures over supported FPROPS gas species
- phase name, coordinate name, and member name discovery
- result access by phase/member/coordinate name
- runtime NLP solver selection
- phase-entry residual concept and lambda convention

Currently Fe-O-H / Fe-O-C-specific parts:

- the implemented named solution phases are wustite and spinel
- most validation data are Fe oxide plus H2/H2O or CO/CO2
- the Fe-O-C checks intentionally omit free carbon and Fe3C
- the examples focus on Fe-O-H reduction

The API shape is intended to support other systems. New systems need phase
model adapters that provide Gibbs energy, element contents, valid coordinate
domains, member names, and entry behavior.

## 10. Future Work

### 10.1 More Boundary and Robustness Checks

- add denser boundary-near BG truth checks around Fe/FeO, Fe/Fe3O4, and
  FeO/Fe3O4-sensitive regions
- add more Fe-O-C checks on both sides of current 585 C and 700 C wustite
  points
- add deliberately perturbed warm-start tests, not only initial-mask tests
- add continuation in temperature or feed severity for cases where active-set
  convergence is sensitive
- decide whether public strategy selection should eventually expose
  enumeration and active-set separately, or keep one public `auto` strategy

### 10.2 API and Output Polish

- keep public `eqm.h` and `eqm_phase.h` declarations documented with
  Doxygen-style summaries covering argument direction, ownership, array
  lengths, struct members, and units
- consider a heap-allocated `FpropsEqm` constructor/destructor pair for callers
  that cannot conveniently stack-allocate `FpropsEqm`
- add install rules for public phase headers and examples if FPROPS install
  packaging is enabled later
- add scripted example-output regression once the build system has a standard
  example run-test hook
- add JSON/YAML result writers only when a concrete downstream consumer needs
  them
- move any remaining internal control-flow string selectors to enum-backed
  fields after parsing at the API boundary

### 10.3 UNIFAC and Liquid Phases

UNIFAC should fit the phase-aware design, but it still needs an adapter. The
adapter should expose:

- phase amount `N_liq`
- liquid composition coordinates
- molar Gibbs energy `g_liq(T,P,x)` on a clear component basis
- component/member names
- element contents inferred from component formulae
- composition bounds and trace-component tolerances
- derivatives of `g_liq` with respect to composition where possible

Existing nonreactive flash code remains valuable and should not be forced
through the chemical-equilibrium active-set solver. The first useful target is
an ethanol/water-style UNIFAC phase representation checked against existing
flash tests.

### 10.4 Helmholtz Vapor/Liquid Phases

Pure-fluid Helmholtz vapor/liquid flash remains a separate and useful path for
power-cycle style property calculations. It should be harmonized with the
phase-aware layer by sharing phase-model concepts, not by forcing all
pure-fluid property calls through the general chemical-equilibrium solver.

Future phase specifications could look like:

```text
pure:helmholtz:vapor(water)
pure:helmholtz:liquid(water)
```

or use an equivalent structured builder. A real-fluid vapor phase would need a
different fugacity/activity treatment from the current `helmholtz+ref0:` ideal
gas standard-potential path.

### 10.5 Earlier Equilibrium Examples

Earlier examples should be revisited to decide which API they naturally belong
to:

- gas-only water-gas shift and ammonia examples
- Ni/O or Ni/NiO/H2 equilibrium
- ethanol/water or other UNIFAC mixture examples
- Fe-O-C and Fe-O-H cases now covered by BG truth checks

The goal is not to route everything through the phase-aware solver. The goal is
to identify which problems are flat-species problems, which are phase-aware
problems, and which require a liquid or real-fluid phase adapter.

### 10.6 ASCEND Exposure and Derivatives

The efficient ASCEND path should be package-backed and stateful:

- resolve a phase package once
- pass `T`, `P`, and feed amounts or element totals as inputs
- return a fixed set of phase amounts, phase coordinates, member/component
  amounts, and optional mixture properties
- expose concise status and diagnostic outputs
- cache the last successful solution for warm starts, with explicit reset
  behavior

Derivative callbacks are the main blocker to robust ASCEND use. A
non-derivative blackbox can be useful for exploratory single-point work, but
equation-based flowsheets need consistent first derivatives.

Current derivative gaps:

- phase-aware solves do not yet expose stable derivatives of phase amounts or
  phase coordinates with respect to `T`, `P`, or feed totals
- derivatives are discontinuous at true phase-entry/exit boundaries
- fixed-active implicit derivatives should come before active-set switching
  derivatives
- ideal gas and binary wustite derivatives are the first practical analytic
  targets
- spinel derivatives need careful treatment because of site fractions,
  charge/site constraints, and current finite-difference member potentials

## 11. Files of Interest

- `eqm_phase.h`: public phase-aware C API
- `eqm_phase.c`: phase-aware implementation
- `eqm_phase_internal.h`: internal active-set, validation, and fixed-active
  helpers
- `eqm_linalg.h`, `eqm_linalg.c`: shared small linear algebra helpers
- `eqm.h`: public flat/package equilibrium declarations
- `eqm_internal.h`: internal flat equilibrium helpers
- `python/fprops.i`: SWIG bindings for the phase API
- `examples/feoh.c`: C Fe-O-H example
- `examples/feoh.py`: Python Fe-O-H example
- `test/cutest_eqm.c`: CUnit coverage for equilibrium and phase tests
- `test/feoh_baur_glaessner_compare.py`: H2/H2O BG comparison script
- `test/feoc_baur_glaessner_compare.py`: CO/CO2 BG comparison script
- `test/fprops_phase_boundary.py`: Python bridge to the current phase API
- [EQUIL.md](EQUIL.md) and [A4EQUIL.md](A4EQUIL.md): older equilibrium API
  context
- [SLAG.md](SLAG.md): Fe-O-Si-Al follow-on requirements
- [PHASES-impl.md](PHASES-impl.md): archived implementation notes and
  decision history

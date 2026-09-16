# PSA reconstruction study

Pressure swing adsorption (PSA) was an important early case study for
ASCEND, through the work of Oliver J. Smith IV and his PhD advisor Arthur
W. (Art) Westerberg at Carnegie Mellon University. Smith's 1991 thesis,
*The Optimal Design of Pressure Swing Adsorption Systems*, combined
nonlinear modelling, dynamic simulation and mixed-integer nonlinear
programming (MINLP) to design and optimise PSA systems on economic grounds.

The historical workflow spanned several software tools. ASCEND was used
to formulate, debug and initialise the continuous models; optimisation
was subsequently formulated in GAMS, using MINOS for nonlinear programming
and DICOPT/DICOPT++ for MINLP. Dynamic simulation formed a separate part
of the research. These optimisation tools were used through a separate,
commercial GAMS-based workflow, not the current ASCEND solver interfaces.
The division of work is described in Part I, pp. 2972–2974, and the thesis,
p. 90 and §7.4.

This folder attempts to reimplement the Smith–Westerberg PSA model as
closely as possible to the description in their 1991 paper,
[*The optimal design of pressure swing adsorption systems*](https://doi.org/10.1016/0009-2509(91)85001-E)
(referred to here as **Part I**), using the thesis and related papers for
additional detail. The original model source code has not been recovered
in this repository or the material available to this study. Consequently,
some implementation details have had to be inferred and some assumptions
made explicit. Literal-equation reconstructions and alternative engineering
models are kept distinct, rather than presenting every choice as part of
the original work.

The folder contains models, scripts, source notes and regression tests.
It includes a tested fixed-design cycle comparison and a minimum-bed
cyclic scheduling MIP, **not a reproduced Part I design optimum**.

## What is PSA, and what does this example do?

PSA separates gases by their different tendencies to **adsorb**: attach
to the internal surfaces of a porous solid. At elevated pressure, a bed
of adsorbent preferentially retains some components while others pass
through. Lowering the pressure releases retained gas, regenerating the
bed for another cycle; a small flow of purified product can help purge
the bed. Several beds operate out of phase so that one can supply product
while another regenerates. Pressure equalisation transfers gas from a
high-pressure bed to a low-pressure bed, helping recover gas that would
otherwise be vented and reducing subsequent pressurisation requirements.

The example here recovers **hydrogen from a hydrogen/methane waste stream**,
containing 95 mol% H2 and 5 mol% CH4. Activated carbon preferentially
adsorbs methane, while hydrogen is treated as non-adsorbing and leaves as
the purified product. The aim is to recover valuable hydrogen for reuse,
not to manufacture hydrogen through a chemical reaction. No particular
downstream consumer is modelled.

The original economic design problem balances hydrogen recovery and
product revenue against equipment and operating costs. It chooses bed
sizes, operating pressures and cycle timing, and compares configurations
with zero to three pressure-equalisation steps. Our aggregate models assume
pure hydrogen product; they do not yet predict its actual purity from
time-dependent concentration profiles.

## Quick start

From the repository root, with ASCEND and its Python bindings built:

```sh
./a4 script models/psa/psa_part1_physical.py
./a4 script models/psa/psa_part1_cycle.py
./a4 script models/psa/psa_scheduling.py --solver HiGHS --output psa.png
./a4 pytest models/psa/test -q
```

The first command compares unfitted, conservative alternatives with the
literal journal-equation reconstruction; the second reports the latter's
remaining discrepancies. QRSlv handles these cycle calculations. The
earlier timing tests also exercise available HiGHS/Gurobi solvers; Gurobi
requires its normal licence. The scheduling driver produces a report and
periodic Gantt chart (Matplotlib required); see [scheduling](psa_scheduling.md)
for all eight source cases and optional GUI plotting. CI uses the scripts
under `test/` here.

## What is here?

| Files | Purpose |
|---|---|
| [psa_properties](psa_properties.md) | Cen–Yang methane adsorption isotherm and dimensional conversions |
| [psa_cycle](psa_cycle.md) | Earlier isothermal zero/one-equalisation cycles and conditional scheduling LPs |
| [psa_scheduling](psa_scheduling.md) | Reusable minimum-bed cyclic scheduling MIP; eight published cases, HiGHS/Gurobi checks and Gantt plotting |
| [psa_dynamic](psa_dynamic.md) | IDA bed operations, conservative frozen-solid transfers, representative-bed CSS for 0–3 equalisation pairs, and spatial/time refinement checks |
| [psa_thermal](psa_thermal.md) | Printed journal/thesis operation-equation audits; includes `psa_thesis_thermal.a4c` |
| [psa_thermal_cycle](psa_thermal_cycle.md) | Conservative zero-equalisation thermal components and earlier thesis-based scenario |
| [psa_part1](psa_part1.md) | Part I source-data deck and published-results consistency checks |
| [psa_part1_cycle](psa_part1_cycle.md) | Closed journal-equation audit, inverse diagnostics and rounding-aware heat check (`psa_part1_bounds.py`) |
| [psa_part1_physical](psa_part1_physical.md) | Unfitted conservative alternatives at the Part I zero-equalisation design point |
| [test/](test/) | PSA-specific Python regression tests and ASCEND fixtures |

Each model family keeps its original `psa_` prefix and type names. PSA
`REQUIRE` statements use `psa/...` relative to the standard `models` library
root. No extra library-path setting is needed with `./a4`.

## The source is explicit about the thermal basis

This is **not an unresolved isothermal-versus-adiabatic source choice**:

- **Part I**, Smith & Westerberg (1991), printed p. **2969**, assumption
  **(3)**: adsorption/desorption temperatures are calculated from adiabatic
  energy balances. Assumption **(6)** excludes metal-shell heat capacity.
  The adsorption heat balance is eq (11), p. 2970; the purge balance is
  eq (27), p. 2971. DOI:
  [10.1016/0009-2509(91)85001-E](https://doi.org/10.1016/0009-2509(91)85001-E).
  Local PDF: `~/Downloads/1-s2.0-000925099185001E-main.pdf` (assumptions on
  PDF page 3).
- **Smith's thesis**, §**6.2**, printed p. **80**, assumptions (3) and (6),
  makes the same adiabatic/no-shell-capacity assumptions for its integrated
  design model (PDF page 92).
- **The separate dynamic simulation** in thesis §**7.4**, printed p.
  **96**, assumes all operations are isothermal and methane is a trace
  component (PDF page 108). This does not make the Chapter 6 design model
  isothermal. Local thesis:
  `~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.

Consequently, our actively thermostatted and added-capacity cases are
**deliberate alternative engineering assumptions**, not recovered settings
of the Part I model. The closer flow agreement of the isothermal case
does not establish historical reproduction. It also entails approximately
35 kW each of heating and cooling before heat recovery; that requirement
is not present as an imposed isothermal duty in the adiabatic design model.

## Wrap-up status

The implementation demonstrates converged cyclic inventories, dimensional
property calculations and conservative alternative energy balances using
current ASCEND. Literal-equation discrepancies and source inconsistencies
remain visible; no fitted heat or K has been promoted to physical data.

The standalone scheduling MIP now reproduces the published minimum bed
counts and pairing integers for the Oxy-Rich, seven-operation and hydrogen
0–3 equalisation cases. This is a genuine scheduling optimisation, but it
does not yet connect the resulting time envelopes to the physical cycle
or optimise economics. Its displayed time scale is chosen, not predicted.

The isothermal dynamic reconstruction now assembles the complete operation
sequence and iterates a representative bed to cyclic steady state. Methane
inventories and paired gas transfers are audited. The pressure-step donor
mixing rule and spatial boundary closures are explicit reconstruction choices;
the five-point stencil permits reported numerical undershoots. Grid checks
stabilise adsorption utilisation near 0.804, versus the published 0.88. More
importantly, the constant-flow trace approximation predicts purge methane
fractions above one; this physical limitation is flagged, not clipped. See
[dynamic validation results](psa_dynamic.md#numerical-results-and-comparison-with-the-thesis).
This is not a multi-bed startup simulation, a physically validated purge
transient, or a reproduction of the published profiles.

Conservative multi-equalisation **thermal design** cycles, design-dependent
operating limits, economics and optimisation over competing designs remain
unfinished. Dynamic outlet impurity is calculated, but has not established
the published purity specification or an exact hydrogen balance. The test suite verifies
the implemented calculations, not historical optimality or actual plant
performance. Source PDFs remain in the user's Downloads folder and are
not bundled with the repository.

# Closed zero-equalisation Part I equation audit

This assembles adsorption, blowdown, purge and feed repressurisation into
one cyclic model at the **Part I zero-PE geometry**. Initial temperature
and solid loading are now obtained from the preceding operation, not
specified independently. The model solves 95 equations for 95 variables.

It is a **journal-equation reconstruction with explicit boundary choices**,
not an energy-conserving design model or a reproduced optimum. Its energy
defects and potentially infeasible pressure endpoints are results to examine,
not errors hidden by compensating heat inputs.

```sh
./a4 script models/psa/psa_part1_cycle.py
./a4 script models/psa/psa_part1_cycle.py --pressure-basis valve
./a4 script models/psa/psa_part1_cycle.py --k-multiplier 1.2
./a4 script models/psa/psa_part1_cycle.py --adsorption-heat 18000
./a4 script models/psa/psa_part1_cycle.py --fit purge
./a4 script models/psa/psa_part1_cycle.py --fit adsorption-purge
./a4 script models/psa/psa_part1_cycle.py --json
./a4 run models/psa/psa_part1_cycle.a4c
./a4 script models/psa/test/test_psa_part1_cycle.py
```

The reporting script explicitly warns about the nonzero energy defect and
sub-atmospheric blowdown endpoint. The generic `a4 run` command only solves
and checks the model's algebraic assertions; a successful solve must not
be interpreted as a physically feasible or validated design.

Sources: Smith and Westerberg (1991), CES 46, 2967–2976,
[DOI 10.1016/0009-2509(91)85001-E](https://doi.org/10.1016/0009-2509(91)85001-E).
Local PDF: `~/Downloads/1-s2.0-000925099185001E-main.pdf`.
Pressure-change equations were checked against the scan of printed p. 2970;
purge/repressurisation equations are on p. 2971. The immutable reported
values come from [psa_part1_data](psa_part1.a4c), documented in
[the Part I benchmark](psa_part1.md).

## Equations retained and additional interpretations

| Stage | Implementation |
|---|---|
| Adsorption | Existing printed eqs. (6), (7), (10)–(13), including the **final solid inventory** in the heat-release term. Initial loading is now nonzero because feed repressurisation captures methane. |
| Transition to blowdown | Added sealed, adiabatic mixing of the hot region and cold tail, with frozen solid loading and conserved component inventories/internal energy. This boundary closure is not specified by Part I. |
| Blowdown | Gas-only expansion, eq. (15); retained gas, eq. (18); gas/solid equilibration, eq. (22); reconciled pressure, eq. (24). The latter is the final state's ideal-gas equation, not an additional duplicate relation. Solid loading stays frozen. |
| Purge | Printed eqs. (26)–(29), not the thesis purge-ratio replacement. Total exhaust also includes displaced initial void CH4/H2 to close the component inventories. |
| Feed repressurisation | Receiver-side eqs. (16), (17), (19)–(21), (23), (25), using an explicit external feed reservoir. Hydrogen conservation makes eq. (30) an independent check, not a redundant extra equation. All incoming methane is captured. |

There are two notable receiver-side details:

- Eq. (21)'s volume label appears repeated. Its expression is interpreted
  as the volume occupied by compressed initial H2, as required by the
  complementary-volume relation (20). Eliminating those volumes gives
  `feed = (Pvalve Vgas/R − initial_H2 TB)/TF`.
- Printed eq. (23) uses the feed-mixture cp even for the initial-H2 term,
  and contains **no adsorption-heat term**. Both features are retained;
  we do not insert a physically preferable heat balance and call it the
  printed equation.

The pressure components are in [psa_part1_cycle.a4l](psa_part1_cycle.a4l).
They differ intentionally from the conservative pressure-changing
components in [psa_thermal_cycle.a4l](psa_thermal_cycle.a4l).

## Explicit assumptions and pressure conventions

The common scenario uses:

- d = 0.45 m, L = 2.24 m, void fraction 0.44, carbon density 800 kg/m³;
- Part I adsorption utilisation 0.75, efficiencies 0.95, desorbed fraction 1;
- feed and purge inlet temperatures 298 and 350 K;
- carbon/gas heat capacities from Table 1;
- **20920 J/mol adsorption heat**, the explicit Cen–Yang hypothesis;
- the existing **Henry-slope hypothesis for K**, evaluated at purge-final
  temperature, with a separately exposed positive multiplier;
- **300 international psia feed reservoir pressure**, an unsourced scenario
  input, not a published value or a value borrowed from Part II;
- the independent caloric reference convention described in
  [psa_thermal_cycle.md](psa_thermal_cycle.md) for energy diagnostics.

The missing reservoir pressure matters because the receiver analogue of
eq. (17) needs a pressure ratio. The command-line option
`--source-pressure-bar` exposes it in SI-compatible units; default 300 psi
is approximately 20.68427 bar absolute. If the required valve endpoint
exceeds the reservoir pressure, the report warns that additional
pressurisation/work is not modelled.

Two pressure interpretations are implemented as separate derived cases:

| Interpretation | What 225/15 psia specifies | What is calculated |
|---|---|---|
| `reconciled` (default) | Bed pressures **after** gas/solid equilibration | Valve endpoint pressures before equilibration |
| `valve` | Valve endpoint pressures **before** equilibration | Actual pressures subsequently used in adsorption and purge |

In the default case the computed blowdown valve endpoint is **0.360764 bar
absolute**, while the final warm bed reaches the prescribed 1.034214 bar.
Thus this interpretation cannot describe venting to atmosphere without
additional low-pressure equipment. No such equipment or cost is included.
The source gas-only expansion temperature is approximately **107.51 K**,
while the compressed initial H2 endpoint in repressurisation is about
**658.48 K**. These are model intermediate temperatures, not predicted
uniform bed temperatures. They also expose the extent of constant-cp and
ideal-gas extrapolation in this approximation.

The alternative `valve` case keeps blowdown valve pressure at 1.034214 bar,
but gives a reconciled purge pressure of **2.166050 bar**, and adsorption
pressure **14.533228 bar** rather than the tabulated 15.513204 bar. Neither
interpretation is silently chosen as the historical implementation.

## Closed-cycle results versus the source

| Quantity | Part I Table 4 | Reconciled-pressure case | Valve-pressure case |
|---|---:|---:|---:|
| Adsorption feed (mol/bed-cycle) | 1610 | 1310.483 | 1186.030 |
| Feed repressurisation (mol/bed-cycle) | 95.6 | 95.653 | 80.413 |
| Gross H2 product (mol/bed-cycle) | 1533 | 1250.992 | 1131.791 |
| Purge H2 supplied (mol/bed-cycle) | 47.7 | 78.922 | 215.399 |
| Recovery | 0.90 printed; 0.916671 from listed amounts | 0.877409 | 0.761679 |

The near agreement in repressurisation feed is useful but does not validate
the other closures. No published amount is imposed to initialise or fit
the model. The original source-recovery inconsistency remains visible.

For the default case, the cyclic bed states are:

| Boundary | Temperature (K) | Adsorbed CH4 (mol) |
|---|---:|---:|
| Pressurised / adsorption start | 300.392585 | 4.782673 |
| Mixed adsorption exit | 308.440998 | 66.781069 |
| After blowdown equilibration | 308.206104 | 66.781069 |
| After purge | 300.332959 | 0 |

The cycle returns to the same pressurised-state instance. Both component
balances close; the zero final purge loading follows Part I's specified
desorbed fraction of 1, not a solved kinetic result.

## Inverse diagnostics: can the missing K explain the mismatch?

**No, not with the other baseline assumptions unchanged.** Two optional
ASCEND methods exchange fixed specifications without adding equations:

- `fit_purge` frees K's multiplier and fixes purge supply to Table 4.
- `fit_adsorption_purge` additionally frees adsorption heat and fixes
  adsorption feed to Table 4. The source constants remain unchanged.

The Python `--fit` option first solves the forward model, then runs the
selected method. Its text and JSON distinguish fitted targets from held-out
checks. `on_load` restores the forward specifications on the same instance;
new forward runs do not inherit fitted values. No design variable or cost
is optimised by these methods.

At the central published geometry and reconciled-pressure interpretation,
with the unsourced 300 psia reservoir scenario unchanged:

| Quantity | Baseline | Fit purge only | Fit adsorption feed and purge | Table 4 |
|---|---:|---:|---:|---:|
| H (kJ/mol) | 20.920 | 20.920 (fixed) | **3.51843 (fitted)** | Not established |
| Henry K multiplier | 1 | **0.761715 (fitted)** | **0.855122 (fitted)** | Not established |
| Adsorption feed (mol) | 1310.483 | 1326.918 | 1610 (target) | 1610 |
| Repressurisation feed (mol) | 95.653 | 96.003 | 96.081 | 95.6 |
| Gross H2 (mol) | 1250.992 | 1266.735 | 1533.617 | 1533 |
| Purge supply (mol) | 78.922 | 47.7 (target) | 47.7 (target) | 47.7 |
| Recovery | 0.877409 | 0.901803 | 0.916793 | 0.90 |

Fitting K alone brings recovery close to the *printed* 0.90 while leaving
adsorption feed about 18% low. This is a useful example of why recovery
alone is not a reproduction criterion. The two-target fit substantially
improves the other flows but does not match either held-out amount within
its displayed rounding at this fixed geometry. These small held-out
differences alone do **not** exclude joint reconciliation when all rounded
design quantities are allowed to move.

Neither fit repairs the sub-atmospheric blowdown endpoint or complete
energy boundary: the two-target fit still has an energy defect of about
−213.5 kJ/bed-cycle. The inferred 3.52 kJ/mol must therefore **not** replace
the sourced Cen–Yang heat as though it were recovered historical data.

With `--pressure-basis valve --fit adsorption-purge`, the algebra requires
H ≈ **−1.200 kJ/mol**. The report explicitly warns that this contradicts
the assumed positive exothermic heat-release magnitude; repressurisation
feed is only about 80.51 mol. Negative inferred heat is exposed as a failed
physical interpretation, not clipped to zero or accepted as property data.

### A stronger check, independent of K and the added thermal interfaces

[psa_part1_bounds.py](psa_part1_bounds.py) uses only the zero-PE adsorption
equations, the Cen–Yang LRC and the stated solid-inventory assumptions.
Complete regeneration and capture of all incoming FR methane imply
`sA0 = y × FR`. Write `c = P × phi × V × eps / R`. Then:

```text
phi × eta × Mcarbon × q(Te,P) + y × c/Te = y × (Fads + FR)
1/T0 = (1−y)/Te + [grossH2 − (1−y) × Fads]/c
H = {phi × Csolid × (Te−T0)
     + y × (Fads−c/Te) × cpA × (Te−Tfeed)
     + (1−y) × (Fads−c/Te) × cpB × (T0−Tfeed)} / sAe
```

These follow from eqs (7), (10)–(13), retaining eq (11)'s **final** solid
inventory in its heat term. They do not use purge K, reservoir pressure,
mixing, or the blowdown/repressurisation thermal equations. They do assume
that Table 4's P9 is the actual adsorption pressure, not a valve endpoint.
The report labels this separate pressure assumption even in valve-mode runs.

Using the central table values gives Te = 301.011 K, T0 = 301.605 K and
H = 1.118 kJ/mol. **Do not interpret that last number as a heat estimate:**
the temperature relation involves a small difference of large rounded
flows. Instead, allow the full last-digit rounding box:

- d and L: ±0.005 m;
- P9 = 225.0 psia: ±0.05 international psi;
- adsorption feed and gross H2: ±0.5 mol;
- repressurisation feed: ±0.05 mol.

Composition, efficiencies, utilisation, void fraction, density, heat
capacities, feed temperature and LRC coefficients stay fixed at their
specified values. Monotonicity of the adsorption root in V, P and total
feed bounds Te throughout this box; outward-rounded interval arithmetic
then propagates the remaining equations. It deliberately loses correlations
and gives the **outer necessary enclosure**:

```text
−12.299 < H < 14.512 kJ/mol
```

Thus **20.920 kJ/mol is excluded under these assumptions even with table
rounding**, independently of any choice of K or feed reservoir pressure.
The negative lower bound is not a permissible exothermic heat; nor is every
positive value in this enclosure feasible. This is not a confidence
interval, a parameter fit, or a proof about alternative property/inventory
conventions. It is a double-precision numerical enclosure (scalar roots
have a small numerical guard), not formal interval certification. Tests
check independent back-substitution at 729 corners/interior points; the
enclosure argument is interval propagation, **not** a claim that sampling
alone proves a bound.

The defensible conclusion is narrower than “the paper is wrong”: the
current combination of adsorption equation, LRC conversion, inventory
convention, caloric data and pressure interpretation cannot reproduce
these flows with the Cen–Yang heat. Adjusting K alone cannot resolve it.
At least one of those assumptions/conventions must differ before a
historical numerical reproduction can be claimed.

## Energy-accounting diagnostic, not a correction

Each operation's diagnostic is:

```text
defect = Ufinal − Uinitial + outgoing enthalpy − incoming enthalpy
```

The full-cycle diagnostic includes net product, both waste streams, both
external feed amounts, and the internal purge heater. Its sign is positive
when outgoing external energy exceeds incoming energy. At cyclic steady
state the sum of operation defects equals the external-plant defect.

Default results, in kJ/bed-cycle:

| Operation | Defect |
|---|---:|
| Adsorption | +129.285 |
| Blowdown | −10.485 |
| Purge | −1.473 |
| Feed repressurisation | −310.649 |
| Whole cycle | **−193.322** |

These quantities are **not added as compensating heat duties**. They depend
on the declared inventory and stream-energy conventions. Specifically:

- Blowdown outlet enthalpy is integrated along the printed gas-only
  expansion path, with the actual mixed void composition; the printed
  temperature/heat equations use feed-based cp.
- Purge exhaust, including initial void displacement, is assigned the
  final bed temperature. The printed purge heat equation does not
  explicitly account for all of those gas inventories.
- Repressurisation inlet energy is evaluated at the external reservoir
  temperature. The diagnostic therefore also includes omitted
  conditioning/work between the reservoir and bed, if applicable.
- Caloric inventory includes adsorbed-phase sensible energy and uses the
  separate Cen–Yang heat hypothesis; it is not claimed to recover the
  original implementation's internal caloric convention.

Consequently the defect demonstrates that this assembled equation model
does not close the **stated** complete energy boundary. It does not by itself
prove a particular typo or identify which historical implementation choice
was made. The defect is invariant to a shift in the arbitrary caloric
reference temperature, as checked by a regression test.

## Tests, solver behaviour and next step

Fifteen tests cover standalone pressure-changing components (including no
pressure-change limits), independent cycle marching with scalar operation
roots, both pressure interpretations, mass/cyclic inventory closure,
energy-defect decomposition, dimensional consistency, caloric-reference
invariance, explicit assumption changes, reporting warnings and IPOPT;
the inverse specifications, independent forward checks of fitted points,
resetting to baseline, negative inferred heat, and the rounding enclosure
are also covered.

Both default pressure cases solve directly with QRSlv. Assumption changes
are continued from their baseline; cold starts are not reliable for all
changes. IPOPT is checked from a QRSlv point with K multiplier 1.01 before
returning to 1.0, using a test-only objective. Larger perturbations are not
claimed robust. No tolerances are loosened to declare historical agreement.
The tests are included in the PSA CI step.

This delivers a closed zero-PE **journal-based equation audit** and quantifies
why the current property/pressure choices fail to reproduce the source.
K-only calibration is insufficient, and fitting heat and K does not repair
the physical boundary. A subsequent design study must therefore distinguish
a documented, physically closed reconstruction from the literal-equation
audit, expose unresolved inputs and report sensitivity to them. Neither
replacing the heat by its fitted value nor quietly switching to the thesis
purge closure establishes reproduction of Part I.
Economics and 1–3 PE journal cycles are still unimplemented here; dynamic
purity modelling remains deferred from the original Part I target.

The next comparison is now implemented in
[psa_part1_physical.md](psa_part1_physical.md): conservative pressure/energy
balances, full-LRC purge, added thermal capacity and an isothermal limit,
all at the Part I zero-PE design and without fitted heat or K values.

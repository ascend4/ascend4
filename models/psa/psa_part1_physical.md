# Part I: conservative alternatives, without fitted parameters

The aim here is an engineering reconstruction of the PSA problem, not
agreement with every digit in the 1991 tables. These cases use the **Part I
zero-equalisation design and feed**, preserve the sourced 20.920 kJ/mol
adsorption heat, and replace questionable operation approximations with
explicit, mass- and energy-conserving alternatives.

**Source-basis clarification:** Part I p. 2969, assumptions (3) and (6),
explicitly uses adiabatic adsorption/desorption temperatures and neglects
metal-shell heat capacity. Thesis §6.2, p. 80, repeats this. Its §7.4,
p. 96, instead makes an isothermal assumption for the separate dynamic
simulation. Thus our isothermal and extra-capacity cases are deliberate
departures from the integrated design model, not resolutions of an
unspecified thermal basis. See the [study summary](README.md).

```sh
./a4 script models/psa/psa_part1_physical.py
./a4 script models/psa/psa_part1_physical.py --case adiabatic
./a4 script models/psa/psa_part1_physical.py --case isothermal --json
./a4 run models/psa/psa_part1_physical.a4c
./a4 script models/psa/test/test_psa_part1_physical.py
```

The [ASCEND assembly](psa_part1_physical.a4c) reuses the
[conservative thermal components](psa_thermal_cycle.a4l), rather than
duplicating their equations. The [comparison script](psa_part1_physical.py)
also runs the unchanged [literal-equation audit](psa_part1_cycle.md).
No licence is required: these equation systems solve with QRSlv.

Sources: Smith & Westerberg, *The optimal design of pressure swing
adsorption systems* (1991), [DOI](https://doi.org/10.1016/0009-2509(91)85001-E),
local `~/Downloads/1-s2.0-000925099185001E-main.pdf`; Cen & Yang (1986),
[DOI](https://doi.org/10.1080/01496398608058382), local
`~/Downloads/cen-1986-psa.pdf`. The ratio comparison uses the different
purge closure of Smith's thesis, Chapter 6, eq (6.22), local
`~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.

## What changes, and why?

All cases retain d = 0.45 m, L = 2.24 m, P9 = 225.0 international psia,
P5 = 15.0 psia, feed 31.1 mol/s with 5 mol% CH4, feed temperature 298 K,
purge supply temperature 350 K, utilisation 0.75, efficiencies 0.95, and
complete regeneration. Product is assumed pure H2, as in the aggregate
source model; this is not a kinetic purity prediction.

### Energy and pressure changes

Adsorption and feed filling use complete rigid-bed energy balances,
including gas/adsorbate inventories, flow enthalpy and adsorption heat.
The same internal-energy convention is used throughout the cyclic model.
Blowdown assumes well-mixed withdrawal and continuous gas–solid thermal
equilibration, with adsorbed inventory frozen. Its integrated endpoint law
is documented in [psa_thermal_cycle.md](psa_thermal_cycle.md).

This is an alternative to gas-only cooling followed by reheating after
valve closure. The specified low pressure is the actual final bed pressure:
there is no hidden sub-atmospheric valve endpoint or separately inferred
feed reservoir pressure. Feed enters the bed at its specified temperature;
**upstream compression and conditioning are still outside this cycle model**.

Two-region adsorption, sealed mixing with frozen solid loading and
complete CH4 capture during feed filling remain approximations. Purge
exhaust temperature is interpolated between its initial/final bed
temperatures, with a default weight of 0.5; tests also exercise 0 and 1.

### Purge: use the full LRC, not a guessed linear coefficient

For the default alternative, invert the implemented nonlinear isotherm:

```text
q(Tdes, yout × Pdes) = etaAds × etaDes × q(Tads, yfeed × Pads)
sweep_H2 = removed_CH4 × (1−yout)/yout
total_purge_H2 = sweep_H2 + final_void_H2
```

This retains the journal's effective equilibrium-efficiency construction
and full final-void charge, but avoids identifying its unspecified K with
an infinite-dilution slope. The Henry comparison instead uses exactly that
earlier hypothesis. No multiplier is fitted in either case. The `ratio`
case uses the thesis's specified 1.25 mol H2/mol desorbed CH4, again with
the full final void charge.

The full LRC requires a higher methane concentration than its Henry
linearisation at the same finite target loading, reducing the inferred
hydrogen sweep. This improves purge agreement here.

**This is still an aggregate effective-sweep approximation.** `yout` is
neither the final exhaust composition nor the whole-cycle mean. A fully
regenerated bed cannot stay in equilibrium with methane-rich gas through
the end of the purge. The report separately calculates the integrated
exhaust composition including initial void displacement. These balances
do not establish a transient concentration profile, complete regeneration
in a specified time, or adequate product purity.

### Additional capacity and isothermal operation

The buffered scenario adds thermal capacity equal to **50% of the carbon's
capacity**, sharing the bed's lumped temperature. This can represent
thermally coupled shell/internals, but the 50% value is an **illustrative
sensitivity input**, not a shell calculation or recovered source datum.
It is implemented as an effective volumetric capacity; carbon mass and
isotherm loading are unchanged. This is suitable for a fixed-geometry
comparison, not yet a geometry-dependent equipment model.

The isothermal case holds all bed states at 298 K. It is an **actively
thermostatted limiting case**, not an alternative adiabatic heat equation.
Heat duties are calculated for every operation and included in the overall
energy boundary. Finite heat-transfer area, temperature driving forces and
utility supply conditions have not been sized. Hence it is a reasonable
modelling limit, not proof that the original hardware would be isothermal.

## Results at the published zero-PE geometry

Amounts are mol per bed per cycle; no reported flow is used as a fitted
target or an extra equation.

| Case | Adsorption feed | FR feed | Gross H2 | Purge H2 | Recovery |
|---|---:|---:|---:|---:|---:|
| Literal journal-equation audit | 1310.48 | 95.65 | 1250.99 | 78.92 | 0.87741 |
| Conservative, Henry purge | 1335.64 | 95.66 | 1274.72 | 84.54 | 0.87530 |
| Conservative, full-LRC purge | 1341.69 | 95.78 | 1280.52 | 72.40 | 0.88468 |
| Full LRC, +50% thermal capacity | 1424.31 | 95.85 | 1358.39 | 65.88 | 0.89500 |
| Full LRC, isothermal at 298 K | **1713.97** | **96.42** | **1631.95** | **49.81** | **0.91992** |
| Conservative, thesis ratio 1.25 | 1332.27 | 95.58 | 1271.49 | 91.35 | 0.87001 |
| Part I Table 4 | 1610 | 95.6 | 1533 | 47.7 | 0.90 |

The reported flows imply recovery **0.91667**, rather than the printed
0.90. Against the flows, the unfitted isothermal alternative differs by
about +6.46% in adsorption feed, +0.86% in FR feed, +6.46% in gross H2 and
+4.43% in purge. That is encouraging agreement for a different approximate
model, not a reproduction of the exact solution or an agreed validation
tolerance. The adiabatic alternatives remain appreciably farther away.

All conservative cases close CH4, H2 and energy balances within the test
tolerances and retain actual blowdown pressure above atmospheric. This
repairs the literal assembly's accounting defects without reducing the
adsorption heat to a fitted 3.52 kJ/mol.

### Isothermal agreement has a thermal requirement

At the isothermal solution, per-bed-cycle heat **into** the bed is:

| Operation | Heat (kJ) |
|---|---:|
| Adsorption | −1715.82 |
| Blowdown | +226.96 |
| Purge | +1740.79 |
| Feed repressurisation | −327.82 |

Including the separate purge heater, the plant-average requirements are
approximately **35.11 kW heating and 35.11 kW cooling**, with no heat
recovery assumed. These average powers use the two-bed throughput period,
not a solved operation schedule. Instantaneous duties can be higher.
Reporting only net heat would misleadingly hide these substantial loads.
An economic comparison must account for these utilities and the equipment
needed to exchange heat; closer flow agreement does not prove a better
economic design.

## Are we nearly done?

For **reconstructing a workable fixed-design PSA cycle with current
ASCEND**, this is substantial progress: we have reusable components,
closed cyclic inventories and energy, converged nonlinear solves, an
explicit hypothesis comparison and an unfitted case reasonably close to
the source flows. Further tuning to make Table 4 digits match is not
necessary to demonstrate that capability.

For the original **design optimisation**, three substantive pieces remain:

1. Conservative coupled pressure-equalisation components and the 1–3 PE
   cyclic configurations. The current conservative model is zero PE;
   an earlier isothermal one-PE model exists on a different parameter deck.
2. Design-dependent scheduling/operating limits and declared economic
   inputs, including feed compression, utilities and equipment costs.
   Use explicit engineering scenarios where historical inputs are missing;
   do not postpone progress indefinitely to recover unavailable constants.
3. Optimise continuous designs at fixed scheduling integers, compare
   configurations, and test sensitivity to the reasonable modelling choices.

That would solve an **equivalent design problem under documented
assumptions**. It need not reproduce the exact historical cost, but it must
actually minimise cost over competing designs rather than only solve a
fixed geometry. Pure-product assumptions and aggregate purge approximations
remain limitations to state, not proof of failure to reproduce Part I.

The shared cycle now exposes utilisation/efficiency/regeneration constants
and a variable purge ratio so that supply models can be composed without
copying conservation equations. The original thesis-based cases retain
their original defaults and regression results. Tests compare the new
ASCEND simultaneous solves with independent scalar cycle marching, check
all reported states and duties, exercise capacity/exhaust-temperature
sensitivities, and ensure that reporting keeps the utility and scope caveats.

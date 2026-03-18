# TGA Modelling Progress Note

## Current Objective

Build a TGA reduction model in ASCEND that is simple enough to screen
whether freeboard and bed-scale gas transport are obscuring the
particle-scale kinetics we actually want to infer.

The present working model is:

- [tga.a4c](tga.a4c)
- model: `tga_reduction_screen`

This is intentionally not yet the full distributed bed model. It is a
lumped screening model with:

- one reacting gas state,
- one freeboard resistance,
- one bed resistance,
- three reaction extents `xi1`, `xi2`, `xi3`,
- `reactor_kineq`-style stepwise thermodynamic driving forces.

## 2026-03-18 Progress

### 1. ASCEND screening model added

The new ASCEND model is in:

- [tga.a4c](tga.a4c)

Key points:

- freeboard and bed transport are represented as two resistances in
  series;
- the extents `xi[1..3]` are the intended differential states for later
  IDA integration;
- gas transport and local rates remain algebraic, so the intended
  dynamic form is an index-1 DAE;
- a demo wrapper `tga_reduction_screen_demo` provides `case_10mg` and
  `case_100mg`.

### 2. `a4 run -r` support fixed

The `a4` front-end source now forwards `-r/--run-method` to
`ascxx/runmodel.py`, and `./a4` was rebuilt with:

```bash
scons a4
```

This means commands such as:

```bash
./a4 run models/johnpye/iron/tga.a4c -m tga_reduction_screen_demo -r case_10mg
```

now work as expected.

### 3. Stepwise equilibrium targets replaced with FPROPS-based fits

The placeholder `K_ratio_eq[r]` values in the TGA screen have been
replaced by correlations derived from the existing Fe-O-H FPROPS work.

The regeneration helper for those equilibrium targets is now:

- [regenerate_k_ratio_eq.py](regenerate_k_ratio_eq.py)

Important clarification:

- the "boundary" calculations used below are **thermodynamic phase
  boundaries**, not spatial boundaries in the reactor;
- they are used only to obtain the equilibrium gas ratio
  `K_ratio_eq = (p_H2O / p_H2)_eq` for each reduction step;
- the actual local rate is still determined by how far the local gas
  composition lies from that equilibrium target.

## Equilibrium Targets Used In `tga.a4c`

The TGA screen currently uses:

```text
K_ratio_eq[r] = exp(A[r] - B[r] / T[K])
```

over the fitted range:

- `873.15 K` to `1473.15 K`
- `600 C` to `1200 C`

### Step 1: hematite -> magnetite

Reaction basis:

```text
3 Fe2O3 + H2 <-> 2 Fe3O4 + H2O
```

Thermodynamic source:

- direct `mu0(T,P0)` values from the mixed source string
  `Fe2O3=hidayat_2015;Fe3O4=hidayat_2015;*=helmholtz+ref0:`
- evaluated via
  [eqm_mu0_runner](../fprops/test/eqm_mu0_runner)

Fit used in `tga.a4c`:

- `A1 = 8.54050167864375`
- `B1 = 2552.121942697152`
- maximum relative fit error over 600 C to 1200 C: about `12.0%`

### Step 2: magnetite -> wustite

Equilibrium target source:

- provisional `wustite|spinel` H2/H2O boundary from
  [feoh_hydrogen_boundary.py](../fprops/test/feoh_hydrogen_boundary.py)
- specifically `oxygen_potential_at_wustite_spinel_boundary(T)`
- gas `mu0` for `H2` and `H2O` from `helmholtz+ref0:`

Fit used in `tga.a4c`:

- `A2 = 8.447122204684765`
- `B2 = 8067.2733483476295`
- maximum relative fit error over 600 C to 1200 C: about `5.3%`

Important caveat:

- this is the weakest step thermodynamically because the reduced spinel
  model is still the least secure part of the current Tier 3 Fe-O-H
  work.

### Step 3: wustite -> iron

Equilibrium target source:

- `Fe|wustite` H2/H2O boundary from
  [feoh_hydrogen_boundary.py](../fprops/test/feoh_hydrogen_boundary.py)
- specifically `oxygen_potential_at_fe_wustite_boundary(T)`
- gas `mu0` for `H2` and `H2O` from `helmholtz+ref0:`

Fit used in `tga.a4c`:

- `A3 = 1.0604569618298618`
- `B3 = 1856.1489018164739`
- maximum relative fit error over 600 C to 1200 C: about `0.8%`

This is currently the most trustworthy of the three step-equilibrium
targets.

## Raw Fitted Data Points

The ratios used for fitting were:

| T [K] | Step 1 `Fe2O3/Fe3O4` | Step 2 `Fe3O4/FeO` | Step 3 `FeO/Fe` |
|---:|---:|---:|---:|
| 873.15 | 245.74444 | 0.47162991 | 0.34429522 |
| 973.15 | 395.10660 | 1.16499320 | 0.42761838 |
| 1073.15 | 515.54741 | 2.45423613 | 0.51286280 |
| 1173.15 | 616.44572 | 4.62589868 | 0.59432774 |
| 1273.15 | 701.29711 | 8.03042726 | 0.67565600 |
| 1373.15 | 774.08361 | 13.19991597 | 0.74966388 |
| 1473.15 | 838.51680 | 20.59359712 | 0.81270531 |

Interpretation:

- step 1 is easily driven forward by hydrogen across this range;
- step 2 becomes progressively harder as the gas must remain much more
  reducing;
- step 3 remains the most reducing-demanding lower-boundary target in
  the current screen.

## Current Interpretation Of The Screening Model

The screen is intended to answer:

```text
Is external transport already large enough to obscure the local
kinetics we would like to measure?
```

This is still a deliberately simplified model. Current limitations:

- no distributed bed slices yet;
- no explicit intraparticle diffusion;
- no pore-opening / pore-closing morphology effects yet;
- no direct runtime coupling to the Fe-O-H equilibrium kernel;
- no time integration exercised yet, although the state structure is set
  up for later IDA use.

## Recommended Next Steps

1. Re-run the `10 mg` and `100 mg` screening cases with the updated
   `K_ratio_eq` values and inspect how much of the observed mass effect
   the lumped transport screen can already explain.
2. If the transport screen remains materially important, move to a
   distributed bed model with local `xi1, xi2, xi3` in each bed slice.
3. Only after that, introduce particle effectiveness / Thiele-type
   modifiers and morphology evolution.
4. Longer-term, consider replacing the fitted `K_ratio_eq(T)` values
   with a direct ASCEND-facing helper around the Fe-O-H equilibrium
   machinery once that integration is mature enough.

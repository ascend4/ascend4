---
autocite: doi
link-citations: true
link-bibliography: true
---

# Alumina Hydrate Thermodynamics

This note records the current FPROPS support for the `Al2O3-H2O` system, added
to support thermodynamic checks relevant to alumina calcination and to compare
against Serena et al. (2009).

Related phase-equilibrium background is in [PHASES.md](PHASES.md). The current
comparison script is [test/alumina_serena_phase_diagram.py](test/alumina_serena_phase_diagram.py).
There is also a forward link to the Fe-O and slag/gangue work in
[FEO.md](FEO.md) and [SLAG.md](SLAG.md).

## Scope

The implemented scope is deliberately narrow:

- pure condensed alumina hydrate and alumina phases as Shomate-style species
- direct calculation of hydrate decomposition boundaries
- plotting checks against the Serena pressure-temperature diagram format
- a forced-conversion, two-stage process-analysis flowsheet with independently
  specified reactor temperatures, signed heat duties, and separate sensible
  enthalpy accounting for solids and reaction steam
- reusable `Al2O3` and `gamma-Al2O3` pure-phase data that can later support
  oxide/slag-component work

This is not yet a full `eqm_phase` implementation of liquid/vapor/supercritical
water. Water in the comparison script is currently treated as:

- ideal-gas steam on a Helmholtz/ref0 standard chemical-potential basis for
  vapor-side reaction boundaries
- FPROPS Helmholtz/IAPWS saturation only for plotting and diagnostics
- an approximate compressed-liquid correction in the script's `auto` branch

True IAPWS water as an equilibrium phase is future work.

## Data Sources

### Serena 2009 Hydrate Source

Source name in FPROPS:

```text
serena_2009
```

This source is based on the thermodynamic assessment of the `Al2O3-H2O`
system by Serena et al. [@doi:10.1016/j.ceramint.2009.04.014].

The following species are available:

```text
gibbsite       Al2O3.3H2O = Al2H6O6
boehmite       Al2O3.H2O  = Al2H2O4
Al2O3          alpha-Al2O3 / corundum
```

Serena's Table 6 gives optimized `Delta_f H_298`, `S_298`, and `Cp(T)` for
gibbsite and boehmite. FPROPS stores those on Serena's double-formula basis:

```text
gibbsite: Al2O3.3H2O
boehmite: Al2O3.H2O
```

The molar volumes in Serena Table 1 are reported on the `Al(OH)3` / `AlOOH`
basis, so the densities encoded in FPROPS are converted to the same
double-formula basis as the thermodynamic functions.

For `Al2O3`, the current `serena_2009` entry uses the existing alpha-Al2O3
heat-capacity function already present in FPROPS, with the same reference
formation enthalpy and entropy used in the slag/ore source. This appears close
enough for the present comparison, but it is still a source-alignment point to
keep in mind.

### USGS Gamma-Alumina Source

Source name in FPROPS:

```text
usgs_bull_1452_1978
```

This source is based on the public USGS mineral thermodynamic tables:

```text
Robie, Hemingway, and Fisher, "Thermodynamic Properties of Minerals and
Related Substances", USGS Bulletin 1452.
```

The implemented species is:

```text
gamma-Al2O3
```

Aliases are also registered:

```text
gamma_Al2O3
gamma_alumina
Al2O3_gamma
```

The USGS table provides `Delta_f H_298`, `S_298`, tabulated thermodynamic
functions, and a heat-capacity equation for crystalline gamma alumina from
`298.15 K` to `1800 K`. The table leaves molar volume blank, so FPROPS does not
apply a condensed pressure correction for this phase.

This is not Serena's cited SSUB-3 gamma-alumina source. It is a public,
traceable substitute.

## Implemented Reactions

The comparison script evaluates the stable Serena-style boundaries:

```text
gibbsite -> boehmite + 2 H2O
boehmite -> alpha-Al2O3 + H2O
```

and the public-USGS gamma-alumina metastable boundaries:

```text
gibbsite -> gamma-Al2O3 + 3 H2O
boehmite -> gamma-Al2O3 + H2O
```

The plot uses Serena's axis convention:

```text
x = T / K
y = log10(P / Pa)
```

Generated comparison output is currently written manually, for example:

```text
PYTHONPATH=models/johnpye/fprops/python \
python3 models/johnpye/fprops/test/alumina_serena_phase_diagram.py \
    --plot-file /tmp/serena_alumina_phase_diagram.png
```

## Two-Stage Process-Analysis Flowsheet {#sec:two-stage-flowsheet}

The model [../alumina/alumina.a4c](../alumina/alumina.a4c) implements the
following forced-conversion calculation:

```text
ambient gibbsite
    -> PREHEATER
    -> R1: gibbsite -> boehmite + 2 H2O
    -> INTERHEATER
    -> R2: boehmite -> gamma-alumina + H2O
    -> COOLER
    -> ambient gamma-alumina + 3 H2O
```

This diagram is a thermodynamic bookkeeping sequence, not yet a physical
calciner layout. In particular, it passes solids and reaction steam through
the same algebraic sequence. An industrial suspension or fluidized calciner
uses a carrier/combustion-gas circuit to convey and contact the particles;
reaction steam normally leaves with that exhaust gas, while hot alumina passes
through a separate solids-cooling and heat-recovery train. The implications
are discussed in [@sec:conveyance-limit].

Gamma-alumina is a designated metastable product. The model does not select it
by global Gibbs-energy minimization. Each `reactor_stoic` instance instead has
a fixed forward reaction extent, analogous to a specified stoichiometric
reactor.

The calculation basis is:

- `1 mol/s` pure gibbsite feed, carrying `1 mol/s` eventual `Al2O3`
- complete conversion in both stages
- feed and final products at `298.15 K` and `1 bar`
- ideal-gas steam from `water=ideal+ref0:RPP`
- no pressure drop
- positive `Qdot` for heat supplied and negative `Qdot` for heat removed

Stage-specific reactive packages are used. Stage 1 contains gibbsite,
boehmite, and water; stage 2 contains boehmite, gamma-alumina, and water. This
avoids evaluating absent gibbsite above the `600 K` limit of its Serena data.
The Serena boehmite data extend to `900 K`, so the current non-extrapolated
stage-2 operating range ends at `626.85 degC`.

### Thermodynamic Thresholds and Operating Temperatures

The reactor model reports the standard reaction Gibbs energy independently of
species amounts:

```text
dg0_rxn[r] = SUM_i nu[i,r] * mu0[i]
DG0dot[r]  = xi[r] * dg0_rxn[r]
```

For unit-activity solids and steam fugacity equal to the `1 bar` reference
fugacity, the current flowsheet sources give:

::: {#tbl:alumina-thresholds .table}
| Stage | Reaction | `dg0_rxn = 0` | Practical baseline |
|---|---|---:|---:|
| 1 | gibbsite -> boehmite + 2 H2O | `385.87 K` (`112.72 degC`) | `523.15 K` (`250 degC`) |
| 2 | boehmite -> gamma-alumina + H2O | `563.30 K` (`290.15 degC`) | `773.15 K` (`500 degC`) |

:::: {.caption}
Standard-state thermodynamic thresholds and adopted process-analysis temperatures.
::::
:::

The zero crossings are thermodynamic lower bounds, not predicted practical
onset temperatures. Solid-state nucleation, lattice reconstruction, diffusion,
steam removal, heating rate, particle size, and residence time control when
conversion becomes substantial. Experimental studies report complete
gibbsite-to-boehmite conversion near `317 degC` at a heating rate of
`10 degC/min` [@doi:10.1016/j.ceramint.2010.07.007], while boehmite dehydration
kinetics have been studied over `723-873 K` (`450-600 degC`)
[@doi:10.1016/j.tca.2011.12.025]. High-heating-rate work also identifies major
endothermic requirements near `319 degC` and `529 degC`
[@doi:10.1016/j.ces.2022.118444]. These observations motivate using industrial
experience, rather than the `dg0_rxn = 0` roots, for the baseline settings.

An actual gas environment changes the driving force through the steam
fugacity. For the two reactions:

```text
stage 1: dg_rxn = dg0_rxn + 2 R T ln(f_H2O/f0)
stage 2: dg_rxn = dg0_rxn +   R T ln(f_H2O/f0)
```

Dry or dilute gas therefore makes forward dehydration more favourable, while
steam-rich gas makes it less favourable. This correction is not yet included
in the flowsheet duties or threshold methods.

### Baseline Heat Duties

At the practical baseline of `250 degC` for stage 1 and `500 degC` for stage 2,
the model gives the duties in [@tbl:alumina-molar-duties]. Because both
reaction extents are `1 mol/s`, each numerical duty in `kW` is also the
specific duty in `kJ/mol` of final alumina.

::: {#tbl:alumina-molar-duties .table}
| Unit or total | Duty (`kW`) | Interpretation |
|---|---:|---|
| Gibbsite preheater | `+52.988` | sensible heating from ambient to stage 1 |
| Stage-1 reactor | `+121.823` | isothermal gibbsite-to-boehmite reaction |
| Interstage heater | `+58.530` | sensible heating from stage 1 to stage 2 |
| Stage-2 reactor | `+82.720` | isothermal boehmite-to-gamma reaction |
| Final cooler | `-101.277` | heat rejected on cooling final products |
| Stage 1: preheater plus reactor | `+174.811` | gross stage-1 input |
| Stage 2: interstage heater plus reactor | `+141.249` | gross stage-2 input |
| Gross supplied heat before cooler credit | `+316.060` | sum of positive process duties |
| Net ambient-feed to ambient-products duty | `+214.783` | overall enthalpy change |

:::: {.caption}
Two-stage baseline duties on the `1 mol/s` final-alumina basis.
::::
:::

The sensible duties can also be partitioned exactly between the solid species
and ideal-gas water using the same FPROPS enthalpies as the parent stream
balances. The reactors remain separate: their isothermal duties include the
chemical enthalpy of dehydration and should not be relabelled as sensible
solid or steam heating.

::: {#tbl:alumina-sensible-split .table}
| Sensible-duty block | Solids (`kW`) | Steam (`kW`) | Total (`kW`) |
|---|---:|---:|---:|
| Gibbsite preheater | `+52.988` | `0` | `+52.988` |
| Interstage heater | `+40.050` | `+18.479` | `+58.530` |
| Final cooler | `-50.213` | `-51.064` | `-101.277` |
| Total sensible heating | `+93.039` | `+18.479` | `+111.518` |
| Positive magnitude of final cooling | `50.213` | `51.064` | `101.277` |

:::: {.caption}
Solids/steam partition of baseline sensible duties on the `1 mol/s`
final-alumina basis. Positive duties heat the process; negative duties remove
heat.
::::
:::

On a final-product basis, the modelled sensible heating comprises
`0.912 GJ/t Al2O3` for solids and `0.181 GJ/t` for reaction steam. The final
cooling comprises `0.492 GJ/t` from solids and `0.501 GJ/t` from ideal-gas
steam. These partitions sum to the original heater and cooler duties without
changing the flowsheet energy balance.

At these operating temperatures, the standard reaction Gibbs energies are:

```text
stage 1: dg0_rxn = -44.559 kJ/mol
stage 2: dg0_rxn = -31.116 kJ/mol
```

Both forced forward reactions therefore have negative standard-state Gibbs
energy at the selected temperatures. This does not constitute a kinetic-rate
or residence-time calculation.

Changing either reactor temperature redistributes heat between its upstream
heater, reactor, and downstream cooler. For example, with stage 1 held at
`250 degC`, the stage-2 sweep is:

::: {#tbl:alumina-stage2-sweep .table}
| Stage-2 temperature | Interstage heater (`kW`) | Stage-2 reactor (`kW`) | Final cooler (`kW`) |
|---:|---:|---:|---:|
| `400 degC` | `+34.902` | `+83.266` | `-78.196` |
| `450 degC` | `+46.749` | `+82.871` | `-89.648` |
| `500 degC` | `+58.530` | `+82.720` | `-101.277` |
| `550 degC` | `+70.139` | `+82.907` | `-113.074` |
| `600 degC` | `+81.472` | `+83.531` | `-125.031` |

:::: {.caption}
Redistribution of stage-2 heat duty with reactor temperature.
::::
:::

The net ambient-state duty remains `214.783 kJ/mol` because the feed,
conversion, and final state are unchanged. The model includes convenience
methods `stage1_200C` through `stage1_300C`, `stage2_400C` through
`stage2_600C`, `industrial_baseline`, and separate standard-steam threshold
methods for the two reactions.

### Scale-Up to 0.7 Mt/a Alumina

For `0.7 Mt/a = 700,000 t/a` of final `Al2O3`, using `365 d/a` and
`M_Al2O3 = 101.962 g/mol`, continuous production corresponds to:

```text
n_Al2O3 = 217.697 mol/s
m_Al2O3 = 22.197 kg/s
```

Linear scaling of the baseline gives [@tbl:alumina-plant-duties].

::: {#tbl:alumina-plant-duties .table}
| Unit or total | Duty (`MW`) | Specific duty (`GJ/t Al2O3`) | Annual energy (`GWh/a`) |
|---|---:|---:|---:|
| Gibbsite preheater | `+11.535` | `0.520` | `101.050` |
| Stage-1 reactor | `+26.520` | `1.195` | `232.319` |
| Interstage heater | `+12.742` | `0.574` | `111.618` |
| Stage-2 reactor | `+18.008` | `0.811` | `157.749` |
| Final cooler | `-22.048` | `-0.993` | `-193.139` |
| Gross supplied heat | `+68.805` | `3.100` | `602.736` |
| Net ambient-state duty | `+46.758` | `2.107` | `409.597` |

:::: {.caption}
Baseline duties at `0.7 Mt/a` final alumina and 100% on-stream time.
::::
:::

The corresponding ideal material rates are:

```text
pure gibbsite feed = 33.962 kg/s = 1.071 Mt/a
gamma-alumina      = 22.197 kg/s = 0.700 Mt/a
reaction water     = 11.766 kg/s = 0.371 Mt/a
```

The phase-partitioned sensible duties at this production rate are:

::: {#tbl:alumina-plant-sensible-split .table}
| Contribution | Specific duty (`GJ/t Al2O3`) | Duty at `0.7 Mt/a` (`MW`) |
|---|---:|---:|
| Solids heating | `+0.912` | `+20.25` |
| Steam heating | `+0.181` | `+4.02` |
| Solids cooling | `-0.492` | `-10.93` |
| Ideal-gas steam cooling | `-0.501` | `-11.12` |

:::: {.caption}
Baseline sensible heating and cooling separated between solids and reaction
steam at `0.7 Mt/a` final alumina.
::::
:::

At an on-stream factor `a`, the required instantaneous design rate and duties
scale as `1/a`. For example, `90%` availability raises gross heat input from
`68.805 MW` to approximately `76.45 MW`.

### Literature Cross-Check

The calculated gross input agrees closely with published modern-calciner
figures, as summarized in [@tbl:alumina-literature-energy]. A recent primary
study quotes approximately `3.0 GJ/t Al2O3` for a stationary-flow calciner and
`4.5 GJ/t` for a rotary kiln [@doi:10.1016/j.ces.2022.118444]. Independent
solar-calcination work also quotes about `3 GJ/t` for modern industrial
calciners and gives a standard gibbsite-to-alpha-alumina reaction enthalpy of
`185.2 kJ/mol Al2O3`, or about `1.82 GJ/t`
[@doi:10.1039/C7GC00585G].

::: {#tbl:alumina-literature-energy .table}
| Comparison basis | Specific energy (`GJ/t Al2O3`) | Equivalent at `0.7 Mt/a` (`MW`) |
|---|---:|---:|
| U.S. DOE theoretical gibbsite-dehydration minimum | `1.80` | `39.95` |
| Current model, net ambient-state duty | `2.107` | `46.76` |
| Modern stationary/flash-calciner benchmark | `3.0` | `66.59` |
| Current model, gross supplied heat | `3.100` | `68.81` |
| Published modern flash-calciner range | `2.68-3.4` | `59.49-75.47` |
| Rotary-kiln benchmark | `4.5` | `99.89` |

:::: {.caption}
Comparison of calculated and published alumina-calcination energy intensities.
::::
:::

The institutional sources behind the non-DOI entries are:

- the [U.S. DOE theoretical-energy report](https://www1.eere.energy.gov/manufacturing/resources/aluminum/pdfs/al_theoretical.pdf),
  which gives `0.50 kWh/kg = 1.80 GJ/t` for ambient gibbsite dehydration;
- the [Australian ARENA concentrating-solar-thermal study](https://arena.gov.au/assets/2024/07/Univeristy-of-Adelaide-Integrating-Concentrating-Solar-Thermal-Energy-into-the-Bayer-Alumina-Process-Final-report.pdf),
  whose Aspen comparison gives `3.0 GJ/t` for a conventional flash calciner
  and a literature/model range of approximately `2.68-3.4 GJ/t`;
- the [Australian industrial-decarbonisation pathways report](https://arena.gov.au/assets/2023/02/tech-report-pathways-to-industrial-decarbonisation-phase-3-technical-report.pdf),
  which uses approximately `3.0-3.4 GJ/t` for conventional fluidized-bed
  calcination; and
- an [ICSOBA boehmite-calcination study](https://icsoba.org/assets/files/publications/2020/AA11.pdf),
  which reports that gibbsite calcination requires approximately `1.1 GJ/t`
  more dehydration energy than boehmite calcination.

The current stage-1 reactor duty is `1.195 GJ/t`, about `9%` above that
`1.1 GJ/t` comparison. The model gross input is `3.3%` above the common
`3.0 GJ/t` stationary-calciner benchmark and lies inside the ARENA comparison
range. The net model duty is about `17%` above the DOE `1.80 GJ/t` theoretical
minimum. This is directionally reasonable because the model produces
metastable gamma-alumina rather than stable alpha-alumina and mixes Serena,
USGS, and RPP property sources, but the difference remains a useful
source-alignment check.

The agreement at the gross-duty level should not be overinterpreted. The ARENA
comparison uses main-furnace temperatures of roughly `950-1150 degC`, whereas
the current flowsheet stops reaction heating at `500 degC` in its baseline.
Industrial cyclone systems recover substantial sensible heat from hot solids
and gas. The present model omits both this hotter finishing region and its heat
recovery, so compensating omissions can produce a good overall duty before the
temperature profile is realistic.

### Reaction-Steam Condensation Estimate

Complete two-stage conversion produces:

```text
3 mol H2O / mol Al2O3 = 0.530 t H2O / t Al2O3
```

At approximately `1 bar`, the latent heat of condensation near `100 degC` is
about `2.257 MJ/kg H2O`, using IAPWS-95 water properties
[@doi:10.1063/1.1461829]. Condensing all reaction water would therefore release:

```text
q_condensation = 0.530 * 2.257 = 1.20 GJ/t Al2O3
Qdot_condensation at 0.7 Mt/a = 26.5 MW
```

Cooling the steam from `500 degC` to its dew point, condensing it, and cooling
the liquid water to `25 degC` gives approximately
`1.79 GJ/t Al2O3`, or `39.7 MW` at `0.7 Mt/a`. The current ideal-gas calculation
already reports `0.501 GJ/t`, or `11.1 MW`, for cooling the same water as vapour
from `500 degC` to `25 degC`. Consequently, replacing that metastable
ideal-vapour endpoint with liquid water adds approximately:

```text
additional condensation/liquid-cooling duty = 1.29 GJ/t Al2O3
                                             = 28.6 MW at 0.7 Mt/a
```

Including the `0.492 GJ/t` solids-cooling duty, the total thermodynamic cooling
potential becomes approximately `2.28 GJ/t Al2O3`, or `50.6 MW`. This is heat
available at several temperature levels, not an assertion that all of it can
be recovered as useful process heat or work.

Steam dilution changes the dew point more than it changes this total enthalpy
release. At `25 degC`, water condenses only when:

```text
p_H2O > p_sat,H2O(25 degC) approximately 0.0317 bar
```

At a total pressure near `1 bar`, this corresponds to a water mole fraction
above about `3.2%`. The actual carrier-gas flow, combustion products, excess
air, exhaust pressure, heat recovery, and cold-end temperature must therefore
be present before the model can predict whether and where condensation occurs.
In a physical calciner this duty belongs to an exhaust-gas cooler or condenser,
not to the alumina product cooler.

### Central Process-Model Limitation: Conveyance and Heat Transfer {#sec:conveyance-limit}

The present calculation makes no attempt to represent how solids are conveyed
through the calciner or how heat is transferred to them. Each heater simply
imposes an inlet/outlet state and calculates the state-function enthalpy
difference. Each stoichiometric reactor imposes conversion and calculates the
isothermal duty. There is no requirement for sufficient gas flow, temperature
driving force, heat-transfer area, contact time, or particle suspension.

This is the key boundary on interpreting all current results. They answer:

```text
What enthalpy and standard Gibbs-energy changes accompany the specified
states, temperatures, and conversions?
```

They do not yet answer:

```text
How much fuel or electrical heat will a calciner require, and can its gas-solid
contacting system convey, heat, react, separate, and cool the material at the
specified throughput?
```

In particular, the model has no representation of:

- fluidization or transport-gas mass flow and composition
- minimum fluidization, entrainment, particle carryover, or cyclone efficiency
- gas/solid slip, residence-time distributions, particle size, or attrition
- gas-solid heat-transfer coefficients, exchange area, or temperature approach
- combustion, excess air, gas sensible heat, stack losses, or fan/blower work
- pressure drop through beds, risers, cyclones, ducts, and heat exchangers
- the physical split between alumina product cooling and exhaust-gas cooling
- recuperation between hot product/exhaust streams and wet gibbsite or inlet gas

Industrial flash-calciner performance depends on this coupled gas-solid and
heat-recovery system. Published solar flash-calcination work explicitly counts
transport-air sensible heat and notes that industrial arrangements recover much
of it through downstream heat exchangers and particle preheating
[@doi:10.1039/C7GC00585G]. The current model's close match to a nominal
`3 GJ/t` industrial benchmark can therefore arise from compensating omissions;
it should not be treated as validation of a furnace or utility design.

### Other Limitations and Next Steps

In addition to the central conveyance and heat-transfer limitation, the process
calculation currently omits:

- feed surface moisture and non-alumina solids
- refractory, radiation, leakage, and other equipment losses
- the high-temperature finishing region used to control product quality
- reaction kinetics, residence time, particle-size effects, and conversion
  distributions
- delta-, theta-, and alpha-alumina formation at higher temperatures

The final cooler retains water as ideal-gas steam down to `298.15 K`; it does
not model condensation. Its steam contribution is thermodynamic bookkeeping
for a combined outlet path, not a representation of a real alumina product
cooler. Gross supplied heat is the sum of positive heater and reactor duties,
while net duty is an ambient-state enthalpy balance, not a prediction of
fired-fuel consumption.

A useful next extension is a prescribed gas circuit before attempting detailed
fluidization hydrodynamics. It should carry `N2`, `O2`, `CO2`, and `H2O`, with
`CH4` or another fuel where direct combustion is considered; specify a
gas-to-solids ratio and pressure drop; calculate local steam fugacity; and
separate the solids and exhaust paths. Gas-solid heater or cyclone blocks can
then impose finite heat-recovery effectiveness, followed by fan power and feed
surface moisture. A later high-temperature product-conditioning region would
allow a realistic `950-1100 degC` hottest zone while testing whether external
heat remains near the observed `3 GJ/t` benchmark. Partial stage-1 conversion
would also require an interstage representation that carries residual gibbsite
rather than the current complete-conversion package handoff.

## Validation Achieved

The CUnit suite now checks the encoded thermodynamic data directly:

- Serena gibbsite and boehmite reference-state anchors
- Serena gibbsite and boehmite heat-capacity values from the Table 6
  polynomials
- finite `mu0` lookup through `eqm_mu0_source`
- USGS gamma-alumina reference-state anchor
- USGS gamma-alumina heat-capacity values from the printed equation
- finite `mu0` lookup for `gamma-Al2O3`

The current selected tests are:

```text
eqm.alumina_serena_species_cp_reference_points
eqm.alumina_serena_species_reference_state_anchors
eqm.alumina_serena_species_mu0
eqm.gamma_alumina_usgs_species
```

The direct comparison script currently gives these useful checks:

- `gibbsite -> boehmite + 2H2O` at `0.100 MPa`: within about `10 K` of Serena
- `boehmite -> alpha-Al2O3 + H2O` at `0.100 MPa`: within about `6 K` of Serena
- Serena's reported gibbsite-boehmite/water-vaporization intersection is close
  on the vapor branch
- `boehmite -> gamma-Al2O3 + H2O` at `0.100 MPa`, using public USGS gamma data,
  is within about `2.5 K` of Serena's reported metastable normal-pressure
  value

## Known Differences and Open Issues

### Link to Slag/Gangue Components

The alumina work is not only a hydrate-decomposition check. It is also an early
step toward the broader oxide-component work flagged in [FEO.md](FEO.md) and
[SLAG.md](SLAG.md).

The intended progression is:

```text
Al2O3, SiO2 -> first gangue / slag components
MgO, CaO    -> later magnesia and quicklime components
```

The current `Al2O3` entries are pure condensed phases, not a liquid slag model.
That is still useful: pure components give reference chemical potentials and
validation anchors before any multicomponent oxide-solution model is added.

### Water Saturation Basis

Serena reports the `boehmite | alpha-Al2O3 + H2O` boundary intersecting the
water vaporization curve at:

```text
T = 603.4 K
P = 8.794274 MPa
```

FPROPS Helmholtz/IAPWS water saturation gives a higher pressure at this
temperature. This does not appear to be a `ref0` issue, because a constant
reference offset cancels in pure-water saturation. It is best understood for
now as a difference between Serena/Thermo-Calc's water model or plotted
vaporization curve and the FPROPS/IAPWS water model.

The comparison script therefore reports:

- vapor-branch reaction checks against Serena's stated points
- FPROPS/IAPWS saturation as a separate dashed line

### Gamma-Alumina Validation

Gamma alumina has been implemented from a public USGS source, not Serena's
commercial SSUB-3 database source. The public source gives a good
normal-pressure check for `boehmite -> gamma-Al2O3 + H2O`, but the high-pressure
metastable curve still needs proper comparison once Serena's plot is digitized.

The stable Serena Fig. 6 does not contain gamma alumina. Gamma appears in
Serena's metastable discussion/figure, so validation of the gamma source should
be against that metastable plot or against independent public thermodynamic
data.

### Digitized Serena Curves

The next validation step is to digitize the Serena plots and overlay them with
the FPROPS-generated boundaries:

- stable Fig. 6: gibbsite, boehmite, corundum, and water line
- metastable Fig. 7 if needed: gamma-alumina boundaries

This will separate three effects:

- hydrate/corundum thermodynamic source differences
- public USGS gamma source versus Serena SSUB-3 gamma source
- FPROPS/IAPWS water saturation versus Serena's water vaporization line

### True Water-Fluid Phase

For high-pressure liquid and supercritical conditions, FPROPS should eventually
offer a true water-fluid phase for equilibrium calculations:

```text
mu_H2O(T, P) = g_molar(T, P)
```

using the Helmholtz/IAPWS model with robust vapor/liquid/supercritical state
selection. That is feasible, but it is more than a data-entry task. It requires
careful `T,P -> rho` root selection near saturation and the critical point, and
then integration into the phase-aware equilibrium layer.

For calciner gas-atmosphere calculations, ideal-gas or real-gas steam may be
adequate. The true IAPWS phase is mainly needed for Serena-style hydrothermal
or supercritical phase-diagram work.

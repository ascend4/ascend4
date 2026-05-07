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

This source is based on:

```text
S. Serena, M.A. Raso, M.A. Rodriguez, A. Caballero, T.J. Leo,
"Thermodynamic evaluation of the Al2O3-H2O binary system at pressures up to
30 MPa", Ceramics International 35 (2009) 3081-3090.
```

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

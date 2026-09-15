# PSA adsorption properties

This is the first component of a reconstruction of the Smith--Westerberg
pressure-swing adsorption (PSA) design studies. It implements the methane
equilibrium property model, **not yet a PSA cycle or design optimisation**.

## Run

From the repository root, with ASCEND and its Python bindings built:

```sh
./a4 run models/psa/psa_properties.a4c
./a4 run models/psa/psa_properties.a4c -p carbon.q carbon.w
./a4 script models/psa/test/test_psa_properties.py
```

The first command solves with QRSlv and runs the model's `self_test`; the
second prints molar loading and mass loading. No commercial solver license
or external property package is needed. The example can also be loaded in
the GUI, where `P`, `T` and `y` are the fixed inputs.

## Components and physical quantities

- `psa_lrc_isotherm`: parameterised single-adsorbate loading-ratio correlation.
- `cen_yang_methane`: methane-on-carbon coefficients, adsorption heat and
  conversion to mass loading.
- `psa_properties`: a hydrogen-rich feed with non-adsorbing hydrogen, showing
  how total pressure and mole fraction supply the component's partial pressure.

The isotherm leaves temperature and partial pressure free; the containing
model supplies their equations or fixes them. The coefficient names appear
explicitly in the methane type's `REFINES` clause. There is no external
function: these are ordinary differentiable ASCEND relations at positive
temperature and partial pressure.

For the single-adsorbate case of Cen and Yang's equation (8):

```text
qsat × Vm_stp = a + b/T
affinity = exp(c + d/T)
activity = affinity × (p/p_ref)ⁿ
q × (1 + activity) = qsat × activity
w = q × MW
```

`p` is partial pressure in Pa; `T` is absolute temperature in K. `q` and
`qsat` are mol per kg of dry carbon. `w` is kg of methane per kg of dry
carbon, not a mass fraction of the loaded solid plus adsorbate. It is
dimensionless because the two mass dimensions cancel, not because a
dimensional quantity has been suppressed.

The original `a` and `b` use cm³(STP)/g and cm³(STP)·K/g respectively.
STP-volume loading is converted with an explicit ideal-gas reference molar
volume at **273.15 K and 101325 Pa**. This reference-state convention and
the methane molar mass, 16.043 g/mol, are documented conversion choices.

`p_ref` is one international psi, 6894.757293168 Pa, obtained from
0.45359237 kg × 9.80665 m/s² / (0.0254 m)². The existing `measures.a4l`
uses the slightly different legacy value 6894.733 Pa. This component does
not change that global definition: its example and tests use the explicit
reference pressure. Users entering values with ASCEND's existing `psi`
unit will get that unit's existing conversion.

Using `p/p_ref` makes the pressure exponent dimensionally meaningful even
for noninteger `n`. The dimensionless `affinity` is numerically the paper's
psi-based coefficient, but it is **not** an inverse-Pa coefficient.

## Numerical checks

At 299 K, total pressure 355 international psi, and methane mole fraction
0.05, with hydrogen treated as inert:

| Quantity | Calculated value |
|---|---:|
| Saturation loading | 6.015085347937 mol/kg |
| Equilibrium methane loading | 1.101163713067 mol/kg |
| Mass loading | 0.017665969449 kg/kg |
| Dimensionless affinity | 0.012624823287 |

These values are independently calculated checks of the correlation, **not
published optimal PSA design results**. The Python tests evaluate the
source's numerical cm³(STP)/g and psi formulation independently, then
convert to mol/kg and compare with solved ASCEND values. They also check:

- Dimensions of the equations, inputs, coefficients and outputs.
- A 24-point temperature, total-pressure and composition grid.
- Zero methane and the zero-pressure mathematical limit for methane (`n = 1`).
- Dilute loading, approach to saturation, and expected monotonicity.
- The noninteger pressure exponent, using Table 2's pure-hydrogen coefficients.
- Rejection of a nonpositive exponent by the component's `WHERE` condition.

The grid and asymptotic checks are mathematical regression tests, not an
assertion that the correlation has experimental validity throughout those
ranges. For `n < 1`, the pressure derivative is singular at zero; use positive
partial pressures in a gradient-based solve of that generalisation.

## Sources and remaining scope

1. [Cen and Yang (1986), *Separation of Binary Gas Mixture into Two High-Purity
   Products by a New Pressure Swing Adsorption Cycle*](https://doi.org/10.1080/01496398608058382),
   equation (8), Table 2, and nomenclature, pp. 849–850 and 863.
   Local reference: `~/Downloads/cen-1986-psa.pdf`.
2. [Smith and Westerberg (1991), *The optimal design of pressure swing adsorption
   systems*](https://doi.org/10.1016/0009-2509(91)85001-E), equations (7)–(9).
   Local reference: `~/Downloads/1-s2.0-000925099185001E-main.pdf`.
3. [Smith and Westerberg (1992), *The optimal design of pressure swing adsorption
   systems—II*](https://doi.org/10.1016/0009-2509(92)85170-G), Tables 1–3.
   Local reference: `~/Downloads/1-s2.0-000925099285170G-main.pdf`.

Smith--Westerberg assume non-adsorbing hydrogen. Cen--Yang's complete model
instead includes competitive hydrogen adsorption and a different cycle.
The present component deliberately implements the former assumption, not
the competitive sum in the latter model. The pure-hydrogen regression
fixture does not change this scope.

The methane heat of adsorption is stored as a positive heat-release
magnitude, 5000 thermochemical cal/mol = 20920 J/mol (Cen--Yang Table 2).
There is no energy balance yet. It must not be silently substituted for
Smith--Westerberg's differently valued heat entries while claiming an exact
reproduction of their results.

The [fixed-design cycles](psa_cycle.md) now provide isothermal inventory and
operation balances with zero or one pressure equalisation, plus conditional
timing LPs for explicitly supplied processing durations. Remaining work
includes desorption-equilibrium conventions, thermal assumptions, calibrated
geometry/timing limits, economics, and dynamic cyclic-steady-state
verification. In particular, the 1992 paper's B = 0.0129 agrees with the
temperature-dependent correlation at about 298 K rather than its stated
299 K, and its void fractions and reported purge durations differ between
tables. Those discrepancies remain documented reconstruction questions;
they are not hidden adjustments in this property implementation.

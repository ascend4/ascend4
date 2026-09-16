# PSA thermal equations: source audit and executable checks

This advances the reconstruction from isothermal balances to the **printed
adsorption and desorption thermal equations** in Smith and Westerberg
(1991). It is an operation-equation workbench, **not yet a closed thermal
zero-equalisation cycle or a design optimisation**. The existing
[isothermal cycles](psa_cycle.md) are unchanged.

A separate [closed thermal-cycle reconstruction](psa_thermal_cycle.md)
now implements complete inventory/stream energy accounting. It deliberately
does not substitute its conservative heat balances into this printed-equation
audit; the two formulations remain distinguishable.

```sh
./a4 run models/psa/psa_thermal.a4c
./a4 run models/psa/psa_thermal.a4c -p ads.Te des.Te des.yout des.hydrogen_in
./a4 script models/psa/test/test_psa_thermal.py
```

The ASCEND library is [psa_thermal.a4l](psa_thermal.a4l). Components share
bed geometry and thermal data; operation temperatures are separate variables.
All amounts, heat capacities, heats, pressures and time coefficients carry
physical dimensions. The energy equations are algebraically divided by
solid heat capacity, giving temperature residuals rather than megajoule
residuals; this avoids roundoff stagnation at QRSlv's absolute tolerance.

## Implemented equations

| Component | Source and scope |
|---|---|
| `psa_thermal_bed_data` | Packed volume, void fraction, dry carbon density, solid volumetric heat capacity, gas molar heat capacities and an explicit adsorption-heat input. |
| `psa_paper_adsorption` | Eqs. (6), (7), (10)–(13), pp. 2969–2970. The utilised region heats to `Te`; the unutilised region stays at `T0`. The LRC is evaluated at the hot adsorption temperature. |
| `psa_paper_desorption` | Eqs. (26)–(29), p. 2971, including exhaust composition, cooling, sensible heat of incoming purge, and final void hydrogen. `K` and reference mass loading are explicit inputs. |
| `psa_methane_henry` | A **candidate interpretation**, not an equation stated in the paper: the zero-concentration mass-loading slope of the methane LRC. Kept separate from the printed desorption equations. |
| `psa_paper_pressure_time` | Eq. (36), p. 2972: duration = a_pc × (P_high − P_low) + valve overhead, for an existing operation. The slope must be supplied; it is not calibrated here. |
| `psa_thesis_desorption` | Smith's thesis, eqs. (6.21)–(6.23), pp. 86–87: empirical molar purge ratio, printed heat balance, and final void charge. No equilibrium coefficient K. |
| `psa_thesis_purge_inventory` | Explicit component-balance completion of the thesis purge. Tracks initial gas, final gas and total exhaust without changing the printed supply equation. Not a new thermal closure. |

The adsorption equation retains the paper's use of **final** adsorbed
inventory in its heat-release term. This is not silently changed to net
adsorption. For nonzero initial loading, the source's interpretation needs
checking; tests exercise that case specifically as a printed-equation test.

## Thesis formulation: separate comparison

The original reproduction goal is **Part I (1991)**. Its dedicated
[benchmark and discrepancy report](psa_part1.md) keeps the journal inputs
and output targets separate from the thesis formulation described here.

Source: Oliver Jacob Smith IV, *The Optimal Design of Pressure Swing
Adsorption Systems*, Carnegie Mellon University PhD thesis, July 1991.
Local PDF: `~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.
References here use **printed page numbers**, 12 less than PDF page numbers.
The equations (6.21)–(6.23) and Table 7-1 have been checked against the
scanned pages, not just OCR.

The thesis is not merely a longer copy of the journal paper. Its purge
equation uses an empirical molar ratio rather than the journal's equilibrium
coefficient. The new, separately executable audit is:

```sh
./a4 run models/psa/psa_thesis_thermal.a4c
./a4 run models/psa/psa_thesis_thermal.a4c -p ads.Te des.Te des.hydrogen_in gas.yout
./a4 pytest models/psa/test/test_psa_thermal.py -q
```

The original `psa_thermal` journal example remains unchanged. The new
[thesis example](psa_thesis_thermal.a4c) uses Table 7-11 geometry, Table 7-1
thermal inputs, and zero-PE utilisation/ratio values from Table 7-10.
It retains the explicit **Cen–Yang adsorption-heat hypothesis**. Initial
adsorbed loading is zero and the post-blowdown bed temperature is specified
as 298 K; neither is obtained from a closed cycle. Consequently these are
operation-audit results, **not the published design solution**:

| Quantity | Thesis-equation audit |
|---|---:|
| Hot adsorption-region temperature | 314.575543 K |
| Adsorbed methane after adsorption | 67.273906 mol |
| Purge final temperature | 287.149362 K |
| Hydrogen supplied to purge | 87.063232 mol |
| Total purge methane exhaust | 66.096551 mol |
| Total purge hydrogen exhaust | 86.725697 mol |
| Batch-average exhaust methane mole fraction | 0.432506 |

### Purge ratio and complete component inventories

Equation (6.22), p. 87, specifies:

```text
removed_CH4 = phiDes × initial_solid_CH4
sweep_H2 = etaDes × removed_CH4
supplied_H2 = sweep_H2 + final_void_H2
final_void_H2 = P × V × eps / (R × Tfinal)
```

`etaDes` is positive but **not limited to one**. It is 1.25 in the example;
tests also use 2.0. It must not be confused with the bounded `etaDes` in
the journal-equation component.

Retaining this supply equation and assuming a final pure-H2 void gives
the following component-conserving completion:

```text
exhaust_CH4 = initial_void_CH4 + removed_CH4
exhaust_H2 = initial_void_H2 + supplied_H2 − final_void_H2
           = initial_void_H2 + sweep_H2
```

Thus the original void gas is displaced into the exhaust. It does not
vanish, and initial H2 must not also be subtracted from the supplied amount.
This is an **explicit inventory interpretation**, not proof that the
historical implementation used this convention. The calculated `gas.yout`
is a batch-average exhaust composition, not equilibrium or instantaneous
outlet composition. Even at zero solid loading, the printed supply law
still charges a full final void of H2; the tests retain that limit.

The existing `psa_cycle` isothermal reconstruction instead specifies
`supplied_H2 = ratio × removed_CH4`. It has **not** been silently changed.
For identical solid removal, pressure, geometry and final temperature,
the thesis closure consumes one final void inventory more H2. Identical
numeric values of `ratio` therefore do not make the two closures equivalent.

The printed heat equation (6.21) is tested separately. It does not include
explicit sensible-energy terms for the displaced initial gas. Completing
the component balances alone does **not** make this a rigorous transient
energy balance or close the thermal cycle.

Tests compare ASCEND with an independent quadratic solution for purge
temperature and a scalar adsorption root. They cover a 27-point
pressure/temperature/loading grid from fresh initial guesses, initial gas
compositions from pure H2 to pure CH4, partial regeneration, zero loading,
geometric scaling, a zero-heat isothermal limit, and QRSlv/IPOPT agreement.
Ideal-gas equations use molar residuals and the heat equation a temperature
residual; energy errors are checked in joules using the corresponding
temperature-tolerance conversion. These tests run in the existing thermal
test script and CI step.

### What the thesis establishes for the next stages

- Section 6.4, p. 90 explicitly describes square-model debugging in ASCEND,
  followed by optimisation in GAMS.
- Section 7.4, p. 96 specifies **isothermal** dynamic validation, Cen–Yang
  LRC equilibrium, LDF coefficients about 4 s⁻¹, 50 equally spaced spatial
  increments, five-point biased-upwind differencing, and LSODE/DSS/2.
  This is distinct from the acceleration report's 30-interval benchmark.
- Tables 7-7–7-10 document optimisation/dynamic-calibration iterations.
  Table 7-9 gives 50 s for the last one-PE simulation purge, agreeing with
  the preprint, rather than the journal's 40 s. Table 7-11 still gives
  30.3 s, so there is no single consistent published duration to impose.
- Table 7-1 repeats the heat-notation ambiguity discussed below. Sections
  6.3–6.4 still do not provide all pressure-time, extra-column volume,
  velocity-limit and economic inputs. Appendix A is nomenclature, not
  model source code.

Further reconstruction must therefore keep unresolved choices explicit.
The [zero-PE thermal cycle](psa_thermal_cycle.md) now supplies a conservative
set of boundary/energy closures, labelled where they depart from the thesis.
The specified isothermal dynamic purity check remains a separate validation
task. Missing design/economic coefficients should be
named scenario inputs, not tuned to imply an exact historical replication.

## Explicit experimental interpretation

The executable example selects:

- Zero-PE geometry and pressures from 1991 Table 4: d = 0.45 m, L = 2.24 m,
  P_high = 225 international psi, P_low = 15 international psi.
- Table 1: eps = 0.44, rho = 800 kg/m³, solid volumetric heat capacity
  804000 J/m³/K, cp_CH4 = 36.8 J/mol/K, cp_H2 = 29.3 J/mol/K,
  feed temperature 298 K, purge inlet temperature 350 K.
- Adsorption utilisation 0.75, adsorption/desorption efficiencies 0.95,
  desorbed fraction 1.0, from the 1991 example. These differ from the
  illustrative inputs in the existing isothermal cycles.
- Cen and Yang's 20920 J/mol adsorption-heat magnitude, explicitly as a
  hypothesis pending resolution of the Smith–Westerberg heat notation.
- Initial adsorption and **post-blowdown** bed temperatures both specified
  as 298 K. Neither is determined by cyclic thermal closure here.
- Zero initial adsorbed methane for the adsorption test. This is not the
  loaded state that feed repressurisation would generally produce.
- `qref` in desorption eq. (26) taken as equilibrium mass loading at the
  adsorption temperature and feed partial pressure.
- `K` evaluated from the Henry slope at the desorption temperature.

For a methane LRC with n = 1, let B be the dimensionless affinity for
pressure relative to P_ref, and w_sat the saturated mass loading. The
selected hypothesis is

```text
K = w_sat × B × R × T / P_ref
w ≈ K × c_CH4 at infinite dilution
```

K therefore has units m³/mol, matching the paper's m³/kmol after conversion.
It is not a molar-loading/pressure derivative. Finite-loading inversion of
the nonlinear isotherm is a different interpretation; this component does
not claim those alternatives are equivalent.

| Audit result | Value |
|---|---:|
| Adsorption active-region temperature | 309.687070 K |
| Final adsorbed methane | 68.617317 mol |
| Adsorption feed | 1443.176972 mol |
| Gross adsorption H2 product | 1377.337516 mol |
| Final desorption temperature | 290.410352 K |
| Purge-exhaust CH4 mole fraction | 0.390236 |
| Hydrogen charged by printed eq. (28) | 113.932026 mol |
| Candidate K at desorption temperature | 0.000522696103 m³/mol |

These are **not published benchmark values**. In particular, do not compute
a claimed cycle recovery from them: feed repressurisation, blowdown and
cyclic thermal closure are missing. An independent scalar bisection
reconstruction checks the numbers. Tests also cover energy residuals in
joules, two-zone adsorption component balances, a pressure/feed-temperature
grid, nonzero initial loading, the zero-heat isothermal limit, geometric
scaling, the Henry derivative, and eq. (36). IPOPT is tested from a QRSlv
solution at a neighbouring pressure, with a test-only objective.

## Source questions that matter before closing the cycle

### 1. Adsorption heat versus combustion heat

Table 1 gives H_A = 8.90 × 10⁸ J/kmol and H_B = 2.86 × 10⁸ J/kmol.
Those are 890 and 286 kJ/mol. The nomenclature calls H an adsorption heat,
yet hydrogen is assumed not to adsorb. The values closely match combustion
to liquid water: about 890.35 kJ/mol for
[methane (NIST)](https://webbook.nist.gov/cgi/cbook.cgi?Mask=3869&Source=1968CHU2337)
and 285.830 kJ/mol for
[hydrogen (NIST JANAF, liquid water formation)](https://janaf.nist.gov/tables/H-063.html).
The economics section also explicitly values waste gas by its combustion
heat. **Inference:** the table may give economic heating values while the
operation balances require a distinct adsorption heat. This does not prove
what the original code used. We retain a separate, user-set `bed.H` rather
than assert that the ambiguity is resolved.

### 2. Meaning of K and q_A in the journal's eq. (26)

The paper names K_A^eq and gives its units but does not supply a numerical
value or a formula connecting it to the LRC in eq. (8). The reference
temperature/loading convention for q_A also needs confirmation. The thesis
uses a different, ratio-based equation (6.22), so **this question no longer
blocks the thesis reconstruction**. The Henry-slope model remains only an
experimental interpretation of the journal equation.

### 3. Initial void gas during desorption

Eq. (28) adds **all final void hydrogen** to the hydrogen associated with
desorbed methane. It does not subtract the hydrogen already present after
blowdown, nor explicitly account for methane initially in the gas voids.
The frozen-solid blowdown leaves gas at nonzero pressure, so that inventory
cannot simply disappear in a state-conserving cyclic implementation.

The audit component follows the printed equation. Its `hydrogen_out`
describes the gas accompanying removed solid methane, not necessarily all
gas leaving a real purge operation. Unlike the isothermal cycle tests,
these desorption checks do **not** establish a complete bed-gas component
balance. The original inventory/stream-boundary convention is needed.

### 4. Closing the thermal cycle

The paper omits detailed blowdown and feed-repressurisation equations,
referring to the analogous PE operations. Adsorption ends with a hot
utilised region and a cooler tail; PE/blowdown equations use lumped gas
and solid temperatures. We still need to make the transition between those
representations explicit and account for the captured methane during
repressurisation. Simply joining the audit components would not do that.

## Design optimisation: remaining numerical inputs

The paper specifies a 1 s **valve overhead**, not a 1 s total pressure-change
duration. Eq. (36)'s pressure-time slope a_pc is not tabulated. Likewise,
the extra-column volume is described as a fixed part plus a part scaling
with bed volume, without numerical coefficients. Velocity limits governing
bed diameter are discussed but not numerically specified here.

Before attempting to reproduce annual costs, establish the pressure/unit
basis of the shell correlation, compressor calculation/efficiency, and
feed, electricity and waste-gas valuation inputs. Eq. (35) must become an
annual cost **rate**, with operating costs and capital annualisation on
consistent time bases. None of these quantities is being silently filled
with guessed values in a claimed paper replication.

The available thesis does not contain the original ASCEND/GAMS listings.
The outstanding numerical and thermal choices remain reconstruction
assumptions; additional source material is not presumed forthcoming.

Primary source: [Smith and Westerberg (1991)](https://doi.org/10.1016/0009-2509(91)85001-E),
local PDF `~/Downloads/1-s2.0-000925099185001E-main.pdf`. Property source:
[Cen and Yang (1986)](https://doi.org/10.1080/01496398608058382),
local PDF `~/Downloads/cen-1986-psa.pdf`; see [property notes](psa_properties.md).

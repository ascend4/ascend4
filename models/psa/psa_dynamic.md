# Dynamic bed operations and cyclic steady state

This is a reconstruction of Smith's **isothermal dynamic validation model**,
not the adiabatic time-integrated economic model in Part I. The governing
equations and operation data are implemented in ASCEND; Python connects
operation endpoints, iterates the cyclic boundary condition, checks balances,
and plots results. IDA advances time; the spatial stencil is independent of
IDA's multistep time integration.

## Sources and reconstruction choices

Local source: `~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.
Page numbers below are the thesis's printed page numbers.

- Chapter 5, pp.65–66, equations (5.11)–(5.17): trace-species gas balance,
  LDF kinetics, boundary conditions, and clean-bed initial conditions.
- Section 7.4, p.96: isothermal methane/hydrogen dynamics, LRC equilibrium,
  k = 4 s⁻¹, frozen solid during pressure changes, and bed-averaged gas
  concentrations after those changes. Fifty equal spatial increments and
  a five-point biased-upwind approximation were used with DSS/2 and LSODE.
- Tables 7-3/7-4, p.101: d = 0.396 m, L = 1.982 m, ε = 0.4, T = 299 K;
  adsorption at 355 psia for 69 s with 31.1 mol/s; purge at 15 psia for
  31 s with 4.21 mol/s. Precise international-psi conversion is used.
- Table 7-1, p.92: feed methane fraction 0.05 and adsorbent density
  800 kg/m³. The density is used per particle-envelope volume in
  (1 − ε)ρV, consistent with the two-phase accumulation equations.
- Section 7.2, pp.91–92: adsorption; three paired equalisation-down
  operations; blowdown/purge; three paired equalisation-up operations;
  **repressurisation with feed gas** (O9), not product gas.
- Section 7.4, p.98 and Figures 7-3/7-4: cyclic solid/gas profiles and
  reported utilisation/efficiency values, used for comparison, never fitting.

Details not specified sufficiently to reproduce the old source code exactly:

1. We use conservative cell-average balances with shared face fluxes. The
   interior five-point derivative has fourth-order accuracy for smooth
   solutions; the thesis does not print its DSS/2 coefficients or boundary
   closures. Our inlet/end closures are explicitly lower-order. No global
   fourth-order accuracy is claimed.
2. Gas is assumed **well mixed during donor withdrawal**, as well as averaged
   after each pressure change. The thesis explicitly specifies the latter,
   but not a complete outlet-composition rule during nonuniform-bed transfers.
   This assumption matters and is not an exact historical reconstruction.
3. Pressure-changing steps are instantaneous material-transfer maps with
   frozen solid profiles. Their valve-flow histories, durations, and energy
   balances are not simulated. They are not chapter 6's adiabatic design
   equations, which have different physical assumptions.

## Files and commands

- [psa_dynamic.a4l](psa_dynamic.a4l): reusable, parameterised bed operation.
- [psa_dynamic_transfers.a4l](psa_dynamic_transfers.a4l): reusable isothermal
  gas-volume transfer balance and paired equalisation assembly.
- [psa_dynamic.a4c](psa_dynamic.a4c): 50-cell examples.
- [psa_dynamic.py](psa_dynamic.py): individual operations and clean-bed
  adsorption-to-purge example, with profile/inventory plots.
- [psa_dynamic_cycle.py](psa_dynamic_cycle.py): all operation maps, periodic
  fixed-point iteration, transfer ledgers and CSS plots.
- [psa_dynamic_checks.py](psa_dynamic_checks.py): analytical pulse test,
  temporal refinement and nested spatial-grid comparisons.

From the repository root:

```sh
./a4 script models/psa/psa_dynamic.py --cells 50 --stencil 5 --output operations.png
./a4 script models/psa/psa_dynamic_cycle.py --cells 50 --stencil 1 --pe 3 --output css.json --plot css.png
./a4 script models/psa/psa_dynamic_checks.py --grids 25 50 100 --cycles --output convergence.json
./a4 script models/psa/test/test_psa_dynamic.py
```

`--stencil 1` is first-order upwinding; `--stencil 5` selects the five-point
interior formula, **not fifth-order accuracy**. The operations CLI retains
`--order` as a deprecated spelling. The full-cycle CLI defaults to first-order
transport as a non-oscillatory baseline. Grid sweeps use isolated worker
processes to bound memory while the C++ wrapper does not own simulation-tree
teardown. Ordinary operation/cycle functions do not clear the shared Library.

## Handover, conservation and positivity

Higher-order linear reconstruction can give negative gas and solid values.
These are numerical undershoots, not negative physical loadings. The former
handover failure arose because `q` permitted undershoots while `q0` inherited
a nonnegative bound. The bounds now agree and the solid profile is transferred
**unchanged**, reversing its spatial order for countercurrent purge. No value
is clipped or projected onto an equilibrium profile.

Every operation is audited independently using the dimensional inventory:

```text
M_CH4 = (V/n) Σ [ε P yᵢ/(RT) + (1 − ε)ρ qᵢ]
ΔM_CH4 = methane admitted − methane discharged
```

Diagnostics include sampled minimum/maximum y and q, and the magnitude of methane
contained in negative solid entries. Cycle histories retain these diagnostics
for every fixed-point pass, not just the final CSS. Exact conservation does **not** prove
positivity or accurate breakthrough. In particular, allowing signed `q0`
fixes the handover but does not cure the spatial oscillations. The unsmoothed
five-point option remains a source-comparison/accuracy experiment, not a
positivity-preserving production scheme. Tightening IDA tolerances does not
remove spatial discretisation error.

There is also a **physical approximation failure distinct from numerical
undershoots**: the constant-total-flow trace model predicts purge methane
mole fractions up to 1.1345. This persists on the 50-, 100- and 200-cell
five-point grids. It must not be interpreted as an admissible gas composition.
Methane desorbing into the purge is no longer dilute; its contribution to
total flow cannot safely be neglected. A non-trace component/total-gas
balance is needed before these purge transients can be physically validated.
No bound or clipping hides this failure; the CLI and plots flag it.

## Full representative-bed cycle

The pressure-step submodels enforce ideal-gas inventories and total-gas and
methane balances. Equalisation donor outflow is the receiver inflow, with
the same methane amount and final pressure. For identical void volumes and
temperature, r pairs give pressure increments (Ph − Pl)/(r + 1). Thus three
pairs produce 355 → 270 → 185 → 100 → 15 psia and the reverse pressure
ladder during repressurisation. These levels are **results of the paired
balances**, not historical data inserted into the model.

The driver integrates adsorption, applies the donor/blowdown maps, reverses
the solid profile and integrates purge, then applies receiver maps and feed
filling. Each equalisation receiver is a periodic image of the representative
bed. Every pass closes the methane balance including external feed, purge,
blowdown and adsorption outlet; internal transfers cancel pairwise.

This assembles the complete operation sequence at periodic steady state.
It is **not a physical multi-bed startup trajectory**: fixed-point iterations
close periodic partner states rather than advance five separately scheduled
beds. No pressure-step timing has been fabricated to fit the scheduling MIP.
The existing [scheduling model](psa_scheduling.md) supplies the separate
minimum-bed/synchronisation result (five beds, J = [3,2,1] for three pairs).

Convergence requires the largest full-profile change, scaled by feed methane
fraction and feed-equilibrium solid loading, to fall below 10⁻⁶. Failure to
converge within the iteration budget is an error, not a CSS result.

Reported adsorption utilisation is the adsorbed methane after adsorption
divided by the full-bed feed-equilibrium capacity. Desorption fraction is
one minus residual solid methane divided by that after adsorption; purge
efficiency is incoming purge gas divided by methane removed from the solid.
The reported outlet impurity is integrated, not just an endpoint sample.
Because total molar flow is held constant under the trace-species assumption,
net product-gas bookkeeping is explicitly labelled approximate: it is not
an independently closed hydrogen balance or an economic-design result.

## Numerical results and comparison with the thesis

With the stated Table 7-3/7-4 inputs, three equalisation pairs, IDA relative
tolerance 10⁻⁷ and absolute tolerance 10⁻⁹, the following five-point results
were obtained. These are model comparisons, not tuned parameters:

| Quantity | 50 cells | 100 cells | 200 cells | Thesis §7.4 |
| --- | ---: | ---: | ---: | ---: |
| Adsorption utilisation, φAds | 0.804436 | 0.804297 | 0.804264 | 0.88 |
| Desorption fraction, φDes | 0.992167 | 0.992346 | 0.992389 | 0.99 |
| Purge gas / solid methane removed, ηDes | 1.245247 | 1.245237 | 1.245235 | 1.26 |
| Integrated adsorption-outlet methane fraction | 0.00119970 | 0.00119912 | 0.00119901 | — |
| Fixed-point iterations | 7 | 7 | 7 | Fewer than 10 |

Methane balance defects over each cycle are approximately 10⁻¹² mol. The
100-to-200-cell change in adsorption utilisation is about 0.000033, much
smaller than its discrepancy with the published 0.88. Grid refinement alone
does not explain that discrepancy. Unrecovered pressure-transfer composition
rules/boundary details remain possible explanations, not established causes.
The purge trace-approximation failure above also prevents treating agreement
of aggregate desorption metrics as a full physical validation.

First-order upwinding gives φAds = 0.825577, 0.819453, 0.812267 at 25, 50,
100 cells, respectively, approaching the five-point result but with noticeable
numerical smearing. The five-point 25-cell purge has significant negative
loadings; at 50 cells, cold-start passes still have undershoots even though
the final CSS has no material negative loading.

The independent smooth-pulse benchmark gives these cell-average errors:

| Cells | First-order upwind | Five-point stencil |
| ---: | ---: | ---: |
| 25 | 0.002035 | 0.0005893 |
| 50 | 0.001485 | 0.00009939 |
| 100 | 0.0009903 | 0.000007673 |

The five-point observed refinement orders are 2.57 and 3.70 on these grids;
this is not a claim of globally fourth-order accuracy for discontinuous PSA
fronts. Tightening pulse-test IDA tolerances from 10⁻⁹/10⁻¹¹ to 10⁻¹¹/10⁻¹³
at 100 cells changes the five-point error by less than 0.001%, separating
spatial error from time-integration error.

## Regression coverage

The dedicated tests check all zero-to-three-pair transfer assemblies against
independent ideal-gas/mixing calculations, closed-bed kinetics against a
separate scalar RK4 integration, stationary equilibrium, signed-profile
handover, conservation, analytical pulse refinement and fixed-grid temporal
refinement. They also check CSS for zero to three pairs, a five-point CSS
numerical reference, a warm restart, explicit failure when the iteration
budget is insufficient, and corrupted-ledger rejection.
The pulse benchmark isolates transport by setting k = 0; Gaussian **cell
averages**, not point samples, are compared against exact translation.

These checks establish what the implementation solves. They do not establish
exact agreement with Figures 7-3/7-4, positivity of the five-point stencil,
or reproduction of the Part I economic optimum.

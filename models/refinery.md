# Mini refinery planning

[refinery.a4c](refinery.a4c) is the third compact LP showcase, following
[alloy blending](alloy_blending.md) and [multiperiod steel production](steel_production.md).
It implements [GAMS MARCO](https://www.gams.com/latest/gamslib_ml/libhtml/gamslib_marco.html),
which cites Kendrick, Meeraus & Suh (1981), *Oil Refinery Modeling with the
GAMS Language*, and Aronofsky, Dutton & Tayyabkhan (1978).
The original technical report is [UT/CES-RR-14, DE82902083 (NTIS record)](https://ntrl.ntis.gov/NTRL/dashboard/searchResults/titleDetail/DE82902083.xhtml).
Equation and table references in the ASCEND comments refer to that report.
Page references use its printed numbering: add 5 for the PDF page number.

Two crude oils feed distillation, reforming, cracking and hydrotreating.
Intermediate streams retain their crude origin until final blending.
The LP chooses purchases, process throughputs and product blends to maximise
revenue less purchases and operating expenses.

## Run it

```sh
./a4 run models/refinery.a4c
./a4 run models/refinery.a4c --model refinery_highs
```

The first selects Gurobi; the second needs only HiGHS. Both run `self_test`,
checking the reference profit, accounting, material availability, capacities,
blend balances, nonnegative flows and all four quality constraints.
See the [Gurobi setup instructions](../solvers/gurobi/README.md) if needed.

MARCO's second solve tightens the fuel-oil sulfur specification:

```sh
./a4 run models/refinery.a4c --model refinery_low_sulfur
./a4 run models/refinery.a4c --model refinery_low_sulfur_highs
```

To print the economic results (this suppresses `self_test`):

```sh
./a4 run models/refinery.a4c --model refinery_highs --print phi phir phip phiw
```

Inspect `u` (crude purchases), `ui` (butane purchases), `z` (processing),
`w` (blend allocations), and `x` (sales) in the GUI.

## Dimensions and SI conversion

The modern GAMS listing omits quality units. The original report explicitly
specifies them in Tables 9–10 (pp. 20–21), the discussion on pp. 13 and 19,
and comments in its GAMS listing (p. 31). These directly confirm our SI
conversions. Bredström et al., [*Refinery Optimization Platform*, SNF Report 23/08](https://snf.no/media/bzreolub/snf-report-23-08.pdf),
Tables 1, 4–7, 10, 13 and 16, provide corroborating data; we do not adopt
that later report's modified process model or results.

| Quantity | Source convention | ASCEND representation |
|---|---|---|
| Purchases, process levels, blends, sales, capacity | 1000 barrels/day | Volume/time; data in m³/day |
| Prices and operating costs | USD/barrel | Currency/volume; USD/m³ |
| Profit, revenue and expense rates | 1000 USD/day | Currency/time; USD/day |
| Density | lb/barrel | Mass/volume; kg/m³ |
| Sulfur content | lb of sulfur/barrel | Mass/volume; kg/m³, not a fraction |
| Vapour-pressure blending data | mmHg | Pressure; Pa |
| Octane numbers and volume-yield coefficients | Dimensionless | `factor_constant` |

Conversion uses 1 barrel = 0.158987294928 m³, 1 lb = 0.45359237 kg,
and 1 mmHg = 133.322387415 Pa. All dimensional TABLEs contain converted SI
values, retaining 15 significant digits. For example, distillation capacity
is 15898.7294928 m³/day. No reinterpretation of the physical capacity or
prices is involved. ASCEND stores rates per second internally; regression
tests therefore compare the native objective in USD/s.

The sulfur limits 3.5 and 3.4 become 9.98553560974139 and
9.7002345923202 kg/m³. Multiplication by component volume flow gives sulfur
mass flow, so the blending inequality is dimensionally meaningful.
Although a [GAMS guide discussion](https://gams.com/54/docs/UG_ModelSolve.html)
calls these limits percentages, the original report and MARCO's
volume-weighted equations support concentration units. Treating the numbers
as mass percentages would require density-weighted equations and change the LP.

The pressure values follow the report's stated mmHg convention. They are
historical linear-blending data, without a specified measurement temperature;
they are not presented as validated thermodynamic vapour pressures or a modern
fuel specification. Likewise, fuel gas retains MARCO's barrel-based accounting
volume, converted to m³—not a measured gas volume at specified pressure and
temperature. Unit checking does not supply the missing thermophysical basis.

## Physical basis of the blending equations

The density constraint is physically consistent **under the additive-volume
approximation**. Here `w` is a component's volume flow, not its mass fraction.
For a liquid blend, mass conservation gives mass flow = Σ ρᵢQᵢ. If its volume
flow is Q = Σ Qᵢ, its density is ρblend = Σ ρᵢQᵢ / Q. Thus the requirement
ρblend ≤ ρmax becomes Σ ρᵢQᵢ ≤ ρmax Q: exactly `density_limit`.
This matches the report's Eq. (3) and its negligible-volume-loss assumption
(p. 12), and the volume-fraction quality rule in Eq. (4), cross-multiplied
to obtain Eqs. (5)–(6). The footnote on p. 11 explicitly requires equality
in the final-product balance to reproduce the textbook solution.

This assumes densities and volumes at the same reference temperature and
pressure, and neglects volume contraction or expansion on mixing. It is not
an exact mixture-property model. `bb` supplies the additive-volume balance;
its nonnegative flows also make the cross-multiplied limit valid at zero
production, where a blend density would otherwise be undefined.

Sulfur concentration follows the same mass-per-volume accounting. A sulfur
mass-fraction limit would instead constrain sulfur mass flow relative to
total blend mass flow. Octane and vapour pressure are different: their
linear blend rules are empirical approximations, not conservation laws or
vapour-liquid equilibrium calculations. The process yield matrices likewise
represent a planning model, not complete reaction, elemental or energy balances.
Short comments beside the ASCEND equations explain these distinctions.

## ASCEND structure and source fidelity

`refinery_problem` holds the common data and equations. The concrete models
set the sulfur limit and reference profit; the HiGHS refinements only change
solver selection. No initial-guess, `FIX` or custom scaling methods are needed.

`blend[f]` lists allowed ingredients. Its inverse, `uses[i]`, is a set
comprehension. The three-index `w` array contains only permitted blend routes.
Two labelled yield TABLEs supply the crude slices of `a`; separate quality
TABLEs avoid mixing dimensions in one matrix. Abbreviations follow the source:
`sr` means straight-run, `rf` reformed and `cc` catalytically cracked.

The original material constraints are inequalities: unused material can be
discarded. Product sales have no minimum requirement. Hydroprocessing has no
capacity coefficient in MARCO; we omit its tautological capacity row, not
impose zero throughput. The unused Mid-Continent hydro activity remains in the
model and has zero optimal throughput because it costs money but produces nothing.
Distillate cracking retains the GAMS cost of 0.8 USD/barrel, versus 0.08 for
gas-oil cracking; these are deliberately not made equal. This differs from
the original report, as detailed below.

## Cross-check against the 1981 technical report

The equations match Eqs. (1)–(12), with Eq. (2) separated into purchased-butane
and produced-intermediate constraints. The permitted blends, yields, capacity
coefficients, capacities, prices, quality data and crude purchase bounds match
Tables 2–11 after unit conversion, except for the operating-cost difference:

- **Table 7 (p. 19) and the GAMS listing (p. 31) both use 0.08 USD/barrel
  for distillate cracking**, while modern GAMS MARCO uses 0.8. The current
  ASCEND example retains the modern GAMS dataset. Using the report's cost
  in the separate matrix formulation gives 12208.343242 USD/day, matching
  the report's published 12208 USD/day in Section 9 (p. 36). The baseline
  flows are unchanged; the extra modern operating expense is
  0.72 USD/barrel × 7805.446897 barrels/day = 5619.921766 USD/day.
- **Table 3 (p. 16) prints a positive West Texas crude-input coefficient.**
  This conflicts with the input-sign explanation following Eq. (1) and with
  the report's executable listing on p. 30, which uses −1. Our crude-availability
  constraint follows the equation and executable listing, not that table typo.
- The **tighter-sulfur scenario is a later GAMS example**, not a solve in
  the 1981 report. Section 6(b), p. 17, explicitly confirms the absence of
  a hydrotreater capacity restriction, consistent with our model.

Section 9 also reports Mid-Continent crude purchases of 89717 barrels/day
and sales of 7523 barrels/day of fuel gas, 42298 of premium gasoline and
36809 of fuel oil. Our baseline agrees within one barrel/day of those
whole-number figures. The modern GAMS guide gives more precise process
levels, used by the assertions below. No model coefficients or constraints
were changed as part of this source cross-check.

## Verified results

### Published GAMS process levels

The [GAMS User Guide's scenario-analysis report](https://www.gams.com/latest/docs/UG_ModelSolve.html#UG_ModelSolve_SensitivityOrScenarioAnalysis)
publishes these baseline process levels in thousands of barrels/day:

| Crude | Process | Published level |
|---|---|---:|
| Mid-Continent | Atmospheric distillation | 89.718 |
| Mid-Continent | Naphtha reforming | 20.000 |
| Mid-Continent | Distillate cracking | 7.805 |

All other baseline process levels are zero. The report also explicitly
confirms zero production for the tighter sulfur specification. Both model
refinements check these published results in `self_test`, in addition to the
common feasibility checks. Nonzero reference levels use a tolerance of half
the published 0.001-unit rounding interval, plus a small numerical allowance;
the conversion factor is 158.987294928 m³/day per reported flow unit.

### Calculated economic results

The modern GAMS guide's table does **not** report profit; the original
technical report does, but for its lower cracking cost described above.
The more precise economic
references below were calculated locally, not taken from a GAMS objective
listing. Both ASCEND adapters agree with a separately written Python matrix
transcription solved using SciPy's HiGHS interface in MARCO's original
numerical units. This bypassed ASCEND but did not execute GAMS, and the matrix
transcription was written by the same implementer. For the baseline:

| Economic result | USD/day |
|---|---:|
| Sales revenue, `phir` | 700205.627676 |
| Purchases, `phip` | 675401.070640 |
| Operating expense, `phiw` | 18216.135559 |
| Profit, `phi` | **6588.421477** |

Only Mid-Continent crude is purchased: 14263.987216 m³/day, with
59.301495 m³/day of butane. The reformer is fully used at 3179.745899 m³/day.
Sales are 6724.951133 m³/day of premium gasoline, 5852.272537 m³/day of fuel
oil and 1196.132917 m³/day on the fuel-gas accounting basis. Regular gasoline
and distillate sales are zero.

With the tighter sulfur specification, **zero operation and zero profit are
optimal**. This is feasible because the source imposes no minimum sales or
throughputs; it is not an infeasible solve or a failed solver.

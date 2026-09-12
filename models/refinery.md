# Mini refinery planning

[refinery.a4c](refinery.a4c) is the third compact LP showcase, following
[alloy blending](alloy_blending.md) and [multiperiod steel production](steel_production.md).
It implements [GAMS MARCO](https://www.gams.com/latest/gamslib_ml/libhtml/gamslib_marco.html),
which cites Kendrick, Meeraus & Suh (1981), *Oil Refinery Modeling with the
GAMS Language*, and Aronofsky, Dutton & Tayyabkhan (1978).

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

The GAMS listing omits quality units. The unit-labelled tables in
Bredström et al., [*Refinery Optimization Platform*, SNF Report 23/08](https://snf.no/media/bzreolub/snf-report-23-08.pdf),
Tables 1, 4–7, 10, 13 and 16, identify the same quality data. We use that
report to establish units, **not** its modified process model or results.

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
calls these limits percentages, the unit-labelled reference and MARCO's
volume-weighted equations support concentration units. Treating the numbers
as mass percentages would require density-weighted equations and change the LP.

The pressure values follow the report's stated mmHg convention. They are
historical linear-blending data, without a specified measurement temperature;
they are not presented as validated thermodynamic vapour pressures or a modern
fuel specification. Likewise, fuel gas retains MARCO's barrel-based accounting
volume, converted to m³—not a measured gas volume at specified pressure and
temperature. Unit checking does not supply the missing thermophysical basis.

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
gas-oil cracking; these are deliberately not made equal.

## Verified results

Both ASCEND adapters agree with a separate matrix transcription of MARCO
solved in its original numerical units. For the baseline:

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

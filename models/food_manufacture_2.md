# Food Manufacture II

[food_manufacture_2.a4c](food_manufacture_2.a4c) implements the six-month oil
refining, blending and purchasing problem from H. P. Williams, *Model Building
in Mathematical Programming*, fifth edition, example 2, pp. 255 and 299–300.
The data and published solution are from
[Gurobi's Food Manufacture II notebook](https://github.com/Gurobi/modeling-examples/blob/master/food_manufacturing/food_manufacture_2.ipynb)
([Colab](https://colab.research.google.com/github/Gurobi/modeling-examples/blob/master/food_manufacturing/food_manufacture_2.ipynb)).

## Run it

```sh
./a4 run models/food_manufacture_2.a4c
./a4 run models/food_manufacture_2.a4c --model food_manufacture_2_highs
```

Both commands solve the same linear MIP and run its `self_test`. The first
requires Gurobi and a license; the second uses HiGHS. Both request zero relative
MIP gap so that the reference-profit check is meaningful, rather than accepting
a nearby incumbent at the default gap tolerance. The C regression suites also
solve the model and run these checks.

The optimum is **100278.7037037 USD**. To display the objective:

```sh
./a4 run models/food_manufacture_2.a4c --model food_manufacture_2_highs --print profit
```

`--print` suppresses the automatic self-test. The `buy`, `consume`, `store`,
`produce` and binary `use` arrays can be inspected in the GUI; months 1–6 mean
January–June, with `store[0]` representing opening inventory.

## Units and physical interpretation

- Quantities, inventories and monthly refining capacities have mass dimension.
  The source's unspecified "tons" are consistently interpreted as metric
  `tonne` (1000 kg), including all prices and capacities. This retains the
  original numerical optimisation problem; it is not a conversion of a known
  short-ton or long-ton physical installation. ASCEND stores masses in kg.
- Purchase/sale prices are in USD/tonne; profit is in USD. Currency values are
  illustrative, not current commodity prices.
- `hold = 5{USD/tonne}` is a charge applied once to each month's closing stock,
  including June. Each model step is one billing month: the rate of
  5 USD/(tonne·month) has been integrated over that period. No arbitrary
  seconds-per-month conversion or dimensionless mass variable is needed.
- Hardness is a dimensionless empirical index: the source specifies no physical
  unit or test method. Its mass-weighted mixing rule is a stipulated planning
  approximation, not a general constitutive law for edible fats. Multiplying
  3 ≤ Σ(h·consume)/produce ≤ 6 by product mass gives linear inequalities that
  also behave correctly when production is zero.
- Refining and blending are lossless. All refined material is sold as product
  that month; only raw oils can be stored. Purchases are indexed by delivery
  month, without separate contract-placement decisions.

The 30 binary selectors enforce three recipe restrictions: each chosen oil
contributes at least 20 tonnes, at most three oils are chosen per month, and
either vegetable oil requires OIL3. The upper links use the actual 200/250-tonne
line capacities, not arbitrary large constants. Everything is a linear relation
with a declared binary variable; native indicator constraints are unnecessary.

All model data are constants. Opening/terminal stocks and storage bounds are
relations, while the built-in `mass` atom supplies nonnegative variable bounds.
No initialisation, fixing or scaling METHOD is required. The HiGHS refinement
only changes solver selection; it retains the same optimality-gap request.

## Source discrepancies

The notebook's displayed objective incorrectly uses `cost·consume`. Its Python
implementation and stated economic objective use `cost·buy`, which is what this
model implements. Charging consumption would lose the economic distinction
between purchasing and using inventory in different months.

The notebook also states a 1000-ton storage limit for each oil but does not
implement it in its Python model. ASCEND includes it explicitly. The published
plan below never holds more than 520 tonnes of any oil, so it remains feasible.
Both solvers recover the notebook's published objective with the limit present.

## Published reference plan

The following tables reproduce the notebook's displayed plan in tonnes,
rounded to one decimal place. They are one optimum, not a required unique
schedule. In particular, do not use rounded entries to test tight feasibility:
the exact February/March vegetable consumptions are VEG1 = 2300/27 and
VEG2 = 3100/27 tonnes. June vegetable purchases are respectively
2·(2300/27) + 310 and 2·(3100/27) + 400 tonnes.

| Purchase | VEG1 | VEG2 | OIL1 | OIL2 | OIL3 |
|---|---:|---:|---:|---:|---:|
| Jan | 0 | 0 | 0 | 0 | 0 |
| Feb | 0 | 0 | 0 | 190 | 0 |
| Mar | 0 | 0 | 0 | 0 | 540 |
| Apr | 0 | 0 | 0 | 0 | 0 |
| May | 0 | 0 | 0 | 0 | 40 |
| Jun | 480.4 | 629.6 | 0 | 730 | 0 |

| Consumption | VEG1 | VEG2 | OIL1 | OIL2 | OIL3 | Product |
|---|---:|---:|---:|---:|---:|---:|
| Jan | 0 | 200 | 0 | 230 | 20 | 450 |
| Feb | 85.2 | 114.8 | 0 | 0 | 250 | 450 |
| Mar | 85.2 | 114.8 | 0 | 0 | 250 | 450 |
| Apr | 155 | 0 | 0 | 230 | 20 | 405 |
| May | 155 | 0 | 0 | 230 | 20 | 405 |
| Jun | 0 | 200 | 0 | 230 | 20 | 450 |

| Closing stock | VEG1 | VEG2 | OIL1 | OIL2 | OIL3 |
|---|---:|---:|---:|---:|---:|
| Jan | 500 | 300 | 500 | 270 | 480 |
| Feb | 414.8 | 185.2 | 500 | 460 | 230 |
| Mar | 329.6 | 70.4 | 500 | 460 | 520 |
| Apr | 174.6 | 70.4 | 500 | 230 | 500 |
| May | 19.6 | 70.4 | 500 | 0 | 520 |
| Jun | 500 | 500 | 500 | 500 | 500 |

At full precision this plan produces 2610 tonnes: revenue is 391500 USD,
purchases cost 237196.2962963 USD and storage charges total 54025 USD, giving
the reference profit. The model's tests check that profit, inventory balances,
capacities, hardness, integrality and all recipe restrictions, without requiring
either solver to choose this particular schedule.

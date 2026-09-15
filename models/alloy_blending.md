# Least-cost alloy blending

[alloy_blending.a4c](alloy_blending.a4c) is the compact introduction, based on the
[GAMS BLEND example](https://www.gams.com/latest/gamslib_ml/libhtml/gamslib_blend.html),
which cites Dantzig, *Linear Programming and Extensions* (1963), section 3.4.
It selects purchases from nine alloys to make a product containing 30% lead,
30% zinc and 40% tin by mass, at minimum cost.

The [detailed version](alloy_blending_detailed.a4c) preserves the longer-form
showcase with reusable model structure, reporting quantities, data validation,
scaling and adjustable batch size. Start with the compact version; those extra
features are not prerequisites for expressing or solving the LP.

## Run it

With the Gurobi adapter built (`scons WITH_GUROBI=1`), put your license in
`~/.config/ascend/gurobi.lic`, or use Gurobi's native license configuration.
The `a4` launcher discovers the ASCEND-specific file automatically, without
overriding an existing `GRB_LICENSE_FILE`.

```sh
./a4 run models/alloy_blending.a4c
./a4 run models/alloy_blending.a4c --run-method with_mass_balance
```

Each command runs the compact model's three reference-solution assertions.
There is also an entry point requiring only HiGHS, with no commercial license:

```sh
./a4 run models/alloy_blending.a4c --model alloy_blending_highs
./a4 run models/alloy_blending.a4c --model alloy_blending_highs --run-method with_mass_balance
```

In the GUI, inspect `v` (purchased masses) and `phi` (total cost).
In the CLI, for example:

```sh
./a4 run models/alloy_blending.a4c --print phi
```

`--print` suppresses the automatic self-test. Mass display units may be kg even
though the source data use lbm and USD/lbm. ASCEND converts these consistently;
1 lbm = 0.45359237 kg.

## What the answer means

For a one-pound batch the optimum is 0.6 lbm of alloy b and 0.4 lbm of alloy d,
costing 4.98 USD. Their element masses are:

| Element | From b | From d | Product |
|---|---:|---:|---:|
| Lead | 0.06 lbm | 0.24 lbm | 0.30 lbm |
| Zinc | 0.18 lbm | 0.12 lbm | 0.30 lbm |
| Tin | 0.36 lbm | 0.04 lbm | 0.40 lbm |

Alloy e already meets the specification, but buying it alone costs 7.60 USD/lbm.
The cheaper blend costs 0.6 × 4.3 + 0.4 × 6.0 = 4.98 USD/lbm. A ten-pound
batch uses 6 lbm of b and 4 lbm of d and costs 49.80 USD. This scale invariance
follows from linear balances, constant prices and the absence of stock limits
or fixed processing charges. USD is an illustrative dimensional convention;
these historical example prices are not current market data.

## Compact ASCEND formulation

Short names follow the source example, with descriptions on every declared
quantity: `alloy`, `elem`, `compdat`, `rb`, `v`, `phi`, and `c` for prices.
The `pc` element balances and `ac` cost accounting are the essential equations.
The objective minimises `phi`; the batch size is simply `1{lbm}`.

The labelled composition TABLE has three rows and nine columns. A separate
one-dimensional TABLE gives prices in USD/lbm, rather than mixing prices and
fractions in a dimensionless matrix. Its header explicitly labels each alloy,
so the prices do not depend on set ordering. Labelled 1-D TABLEs can also be
written vertically as label/value pairs; colons are optional unless the layout
would otherwise be ambiguous. The three target fractions retain explicit labels.

The `mass` atom supplies nonnegative bounds and finite defaults, so there is no
need for bound-setting, an initial mixture or a `FREE` loop. Data are constants,
not solver variables requiring `FIX`. No custom nominal-scaling method is
needed for this example. Setup only excludes the redundant mass-balance row
and selects the solver. The alternative HiGHS entry point and three small
reference assertions are included in the same file.

## Detailed showcase

```sh
./a4 run models/alloy_blending_detailed.a4c
./a4 run models/alloy_blending_detailed.a4c --run-method with_mass_balance
./a4 run models/alloy_blending_detailed.a4c --run-method ten_pound_batch
./a4 run models/alloy_blending_detailed.a4c --model alloy_blending_detailed_highs
```

Inspect `purchase`, `blend_fraction`, `element_mass`, `alloy_cost`, `total_cost`
and `specific_cost` in this version. Its methods run the full data, feasibility,
accounting and optimum checks.

`alloy_blending_problem` contains the reusable equations and methods;
`alloy_blending_detailed` supplies the named sets and immutable data. The composition
TABLE uses fractions, rather than mixing percentage values and prices in one
untyped table. Prices have currency-per-mass dimensions. Decision quantities
are masses; the objective is a monetary quantity.

`check_data` verifies nonnegative, normalised compositions and target fractions,
nonnegative prices and a positive batch size. `specify` fixes only the batch
size and frees purchases and reporting quantities. `default_self` starts from
equal feed shares, not the known optimum. `bound_self` and `scale_self` keep
bounds and nominal values separate from the problem data.

Element accounting, exact product specifications and cost accounting are named
equations. Extra reporting variables make the physical result easy to inspect.
Their equations remain linear because batch mass is fixed. If it is freed,
the fraction and specific-cost reporting equations become bilinear.

Both versions omit the explicit total-mass balance by default, as does GAMS
model b1. Summing the element balances already gives it, since all composition
rows and the target sum to one. `with_mass_balance` includes the redundant row,
corresponding to GAMS model b2; the detailed version also provides
`without_mass_balance` to exclude it again. Neither
changes the optimum. This is a deliberate demonstration of redundancy, not an
extra physical assumption. Missing elements or unnormalised input data would
invalidate that argument, which is why data checks matter.

`check_solution` tests physical feasibility and all reporting identities.
`self_test` additionally checks the reference optimum for this particular data
set. When experimenting with different data, use the former for feasibility;
the reference assertions are not intended for a different product recipe.

Both Gurobi and HiGHS have regression cases for the compact and detailed
formulations with and without the redundant balance, plus the detailed
ten-pound batch. Gurobi solves are
opt-in in the C test runner:

```sh
ASCEND_TEST_GUROBI=1 ./a4 cutest solver_gurobi
./a4 cutest solver_highs
python3 -m unittest discover -s test -p test_a4_launcher.py
```

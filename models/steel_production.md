# Multiperiod steel production

[steel_production.a4c](steel_production.a4c) implements **steelT** from
Fourer, Gay & Kernighan, *AMPL*, [section 4.2, Figures 4-4 and 4-5](https://ampl.com/wp-content/uploads/Chapter-4-Building-Larger-Models-AMPL-Book.pdf).
It chooses production, sales and inventory for two steel products over four
weeks, maximising sales revenue less production and inventory costs.

## Run it

With the corresponding solver adapters built:

```sh
./a4 run models/steel_production.a4c
./a4 run models/steel_production.a4c --model steel_production_highs
```

The first command selects Gurobi; the second selects HiGHS. Both run the same
five reference-solution assertions. See the [Gurobi README](../solvers/gurobi/README.md)
for SDK and licensing setup; no commercial license is needed for HiGHS.

To print the objective:

```sh
./a4 run models/steel_production.a4c --model steel_production_highs \
    --print profit
```

Inspect the `Make`, `Sell` and `Inv` arrays in the GUI for the weekly schedule.
`--print` suppresses the automatic self-test. Masses may display in kg rather
than the input units; the model interprets the book's tons as metric `tonne`
(1000 kg), consistently in quantities, throughputs and prices. This retains
the original numerical LP and optimum, rather than converting fixed physical
short-ton quantities. ASCEND uses kg and seconds internally. USD denotes
illustrative monetary units, not current prices.

## Formulation

Each week has a shared mill-hours constraint. For every product and week,
opening stock + production = sales + closing stock. Sales are bounded above
by market limits; there is no minimum demand or backlogging. Inventory at
index 0 is specified by `inv0`, not charged again as new production.

The carrying cost is in USD/tonne **per weekly boundary**, applied to closing
inventory in weeks 1 through T. It is not a continuous cost rate in USD/(tonne·h).
There is no terminal inventory target or salvage value.

All data are constants, with labelled 1-D and 2-D TABLEs and dimensioned
types. The built-in `mass` atom already supplies nonnegative bounds. Sales
limits and initial stock are expressed as relations, so no bound-setting,
`FIX` loops, initial-guess or scaling methods are needed. The HiGHS refinement
only changes solver selection. This is a continuous LP, with no integer
decisions or linearisation.

## Reference solution

Maximum profit is **515033 USD**. Quantities below are in metric tonnes.

| Quantity | Week 1 | Week 2 | Week 3 | Week 4 |
|---|---:|---:|---:|---:|
| Make bands | 5990 | 6000 | 1400 | 2000 |
| Make coils | 1407 | 1400 | 3500 | 4200 |
| Sell bands | 6000 | 6000 | 1400 | 2000 |
| Sell coils | 307 | 2500 | 3500 | 4200 |
| Closing bands inventory | 0 | 0 | 0 | 0 |
| Closing coils inventory | 1100 | 0 | 0 | 0 |

The initial 10 tonnes of bands are sold in week 1. Holding 1100 tonnes of coils
until week 2 exploits the price increase. Mill time is fully used in every
week: 40, 40, 32 and 40 hours. Zero final stock is an outcome, not a constraint.

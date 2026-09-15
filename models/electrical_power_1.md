# Electrical Power Generation 1

An ASCEND implementation of [Gurobi's Electrical Power Generation 1 example](https://github.com/Gurobi/modeling-examples/blob/master/electrical_power_generation/electrical_power_1.ipynb)
([Colab](https://colab.research.google.com/github/Gurobi/modeling-examples/blob/master/electrical_power_generation/electrical_power_1.ipynb)),
based on H. P. Williams, *Model Building in Mathematical Programming*, 5th ed.,
example 15, pp. 270–271 and 325–326. Three groups of generators supply five
unequal-duration periods over one day, with a 15% online-capacity reserve margin.
The published minimum daily cost is **USD 1,002,540**.

## Run

From a built ASCEND source tree:

```sh
./a4 run models/electrical_power_1.a4c
./a4 run models/electrical_power_1.a4c --model electrical_power_1_highs
```

The first selects Gurobi; the second selects HiGHS. Both request a zero relative
MIP gap and run the model's `self_test` after solving. In the GUI, open the same
file and instantiate the corresponding model. The system exposes `cost`, `E`
and `supply[p]`; each generator group exposes its counts, dispatch, online
capacity, energy and cost breakdown. Periods are numbered 1–5; generator types
retain the source's 0–2 numbering.

## Component organisation

`electrical_power_1.a4c` includes a parameterised `thermal_generator_group`
helper type for this example, rather than a standalone library component. It
represents a fleet of identical units, not a single giant generator. Integer
counts retain the discreteness of the installed machines without introducing
27 interchangeable individual-unit submodels and their associated symmetry.

Each instance receives the shared period durations, installed/initially-online
counts, per-unit minimum/rated power, base cost rate, incremental energy cost,
and startup charge. It owns the output envelope, commitment transitions,
power-to-energy integration and operating/startup cost equations. For example:

```ascend
type0 IS_A thermal_generator_group(T, dt, 12, 5,
    850{MW}, 2000{MW}, 1000{USD/h}, 2{USD/MWh}, 2000{USD});
```

The top-level `electrical_power_1` model declares three such components, aliases
them as `g[0..2]` for aggregation, and supplies only the demand, reserve and
system accounting equations. The component has no solver selection or objective
of its own. Its period durations are shared through `WILL_BE` parameters; its
scalar configuration parameters are typed constants passed by value.

This is a linear operating-envelope model. It does not model the thermal cycle,
fuel flow, ramp rates, minimum up/down times, startup delays, shutdown costs,
network losses or transmission constraints. Group aggregation is appropriate
because units within a type have identical characteristics and no individual
history-dependent restrictions in this example.

## Power, energy and money

| Quantity | ASCEND dimension | Input/reporting unit |
| --- | --- | --- |
| Demand, dispatch, minimum/rated/online capacity | M·L²/T³ | MW |
| Period duration | T | h |
| Generated electrical energy | M·L²/T² | MWh |
| Base operating cost per online unit | C/T | USD/h |
| Incremental energy cost above minimum output | C·T²/(M·L²) | USD/MWh |
| Startup charge and total costs | C | USD |
| Online/startup counts and reserve fraction | Dimensionless | — |

ASCEND stores power in W, energy in J and time in s; it performs the unit
conversions. No physical quantity is made dimensionless to accommodate the
solver. For each group, the operating cost is

`Σ dt[p] · (base · on[p] + marginal · (P[p] − pmin · on[p]))`.

The base cost already includes minimum-load generation for every online unit.
Only the energy generated above that minimum attracts the marginal charge.
Both terms inside the parentheses are cost rates. A startup is an event, so
`startup · Σ start[p]` is **not** multiplied by hours. Integrating dispatched
power gives 612,000 MWh = 612 GWh = 2.2032 × 10¹⁵ J for this day.

The reserve constraint acts on online rated **power**, not installed capacity
or energy. With demand met exactly, it ensures at least 15% headroom. It does
not require generating an extra 15%, nor does it price reserve as generated
energy; response speed and network deliverability are outside this model.

## Source details worth making explicit

- Follow the notebook's **executable** objective: its displayed equations omit
  the period durations and omit the online-unit multiplier in minimum output.
- Its `maxstart0 = 5` applies independently to **each** generator type. Five
  units of each type are already online before the first period and incur no
  initial startup charge. They may shut down immediately without a penalty.
- The 24-hour partition is 00:00–06:00, 06:00–09:00, 09:00–15:00, 15:00–18:00,
  18:00–24:00. The source's “12 pm” endpoints are midnight-label errors.
- As in the source, generation is constrained to be **at least** demand. This
  permits surplus power, implicitly requiring disposal/export if it occurs;
  the optimum for these data meets demand exactly. An isolated physical grid
  without such a sink would instead require equality.
- There is no cyclic end-of-day commitment condition. A positive startup
  charge makes the startup count equal the positive increase in online units
  at an optimum. The helper type requires positive startup charges,
  positive period durations and physically consistent generator parameters.

## Reference solution

The notebook publishes these online counts, with startups shown in parentheses:

| Generator type | 00–06 | 06–09 | 09–15 | 15–18 | 18–24 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0 | 12 (7) | 12 (0) | 12 (0) | 12 (0) | 12 (0) |
| 1 | 3 (0) | 8 (5) | 8 (0) | 9 (1) | 9 (0) |
| 2 | 0 (0) | 0 (0) | 0 (0) | 2 (2) | 0 (0) |

The corresponding least-cost dispatch, computed with ASCEND, is:

| Power / MW | 00–06 | 06–09 | 09–15 | 15–18 | 18–24 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Type 0 | 10,200 | 16,000 | 11,000 | 21,250 | 11,250 |
| Type 1 | 4,800 | 14,000 | 14,000 | 15,750 | 15,750 |
| Type 2 | 0 | 0 | 0 | 3,000 | 0 |

Daily cost comprises USD 750,600 at minimum output, USD 230,940 for additional
energy, and USD 21,000 for startups, totalling USD 1,002,540.

## Tests

```sh
./a4 cutest solver_gurobi.electrical_power_1 solver_highs.highs_electrical_power_1
./a4 cutest solver_gurobi.uc_units solver_gurobi.uc_initial \
    solver_highs.highs_uc_units solver_highs.highs_uc_initial
```

The showcase self-test checks the published cost, daily energy, demand and
reserve. Each group also checks integral counts, output limits, startup
transitions and cost/energy accounting without requiring one exact schedule.

The small analytic tests in `test/mip/electrical_power_1_tests.a4c` exercise the
example's generator helper using W/kW/MW, minutes/seconds and USD/h/MWh
together: 3 MW for 30 minutes, followed by 1 MW for 5400 seconds. The result is
3 MWh and USD 27.50 of running costs. Two starts cost USD 200, independently
of period lengths; one initially-online unit reduces this to USD 100. Both
solver adapters run these cases.

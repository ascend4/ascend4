# Kondili batch-process scheduling

[kondili.a4c](kondili.a4c) implements the two original examples in §5 of
Kondili, Pantelides & Sargent (1993), *A general algorithm for short-term
scheduling of batch operations—I. MILP formulation*, Computers & Chemical
Engineering 17(2), 211–227, [DOI](https://doi.org/10.1016/0098-1354(93)80015-F).
The reference PDF supplied during development is
`~/Downloads/1-s2.0-009813549380015F-main.pdf`; it is not redistributed here.

## Run and report

```sh
./a4 run models/kondili.a4c --model kondili_highs
./a4 run models/kondili.a4c --model kondili_no_bc_storage_highs
./a4 script models/kondili.py --solver HiGHS --output kondili.png
./a4 script models/kondili.py --solver Gurobi --case no-bc-storage --output kondili-no-bc.svg
```

The Python driver prints every nonzero batch, its equipment, start/finish times,
and product discharge events, followed by final inventories. It plots an
equipment Gantt chart and all nine inventory histories. Task colours match the
legend, bar labels give kg, and triangles mark early product withdrawals. Stock
limits are dashed; the open initial marker denotes inventory before time-zero
transfers. The final marker includes transfers at the horizon endpoint.
`--show` opens the chart interactively; otherwise plotting is headless.
The output filename is overwritten. Matplotlib is required for plotting only.

For GUI use, open `models/kondili_plot.a4c`, instantiate `kondili_plot_highs`
or `kondili_no_bc_storage_plot_highs`, solve, then run `plot`. Their names without
`_highs` select Gurobi. This uses `extpy` and the GUI Matplotlib backend, following
the job-shop example. Stale or unconverged simulations are rejected. The plotting
code reads recipes and results from ASCEND and independently reconstructs the
material balances; it contains no second copy of the process data.

## Reference results

| Case | BC storage / kg | Computed objective / currency units | Paper, p. 224 |
| --- | ---: | ---: | ---: |
| Original, Figs. 8–9 | 150 | 2744.375 | 2744 |
| No BC storage, Figs. 10–11 | 0 | 2210.625 | 2210 |

Both ASCEND adapters reproduce these unrounded values. A separate direct matrix
transcription solved with SciPy/HiGHS also produced them. The paper reports only
integer objective values; we do not assert equality to those rounded/truncated
numbers. The original-case terminal quantities are 136 kg Product 1,
147.375 kg Product 2, and 89.375 kg intermediate AB. Thus
10 × (136 + 147.375) − 89.375 = 2744.375. These agree with the paper's
one-decimal material quantities, which cannot reconstruct the objective exactly.
One optimum of the second case gives 116 kg Product 1, 113.625 kg Product 2 and
85.625 kg AB. Alternative optimal schedules are allowed by the tests.

The [ND Pyomo Cookbook](https://jckantor.github.io/ND-Pyomo-Cookbook/notebooks/04.05-Scheduling-Multipurpose-Batch-Processes-using-State-Task_Networks.html)
is a useful introduction, but changes Reactor 2 from 50 to 80 kg, changes the
intermediate terminal penalty from −1 to −100 per kg, adds a charge per batch
start, and imposes 500 kg feed/product storage limits. Its displayed 16-hour
objective is not a reference for this 10-hour original-paper example.

## Physical assumptions and units

The recipe and equipment data follow Fig. 3 and p. 215. The shared components
implement capacity constraints (2)–(3), balances (4), and the §5 specialisation
of objective (18). A backward-window occupation constraint implements the same
non-overlap rule as (1) without big-M. The grid has points 0 through 10 h,
equivalent to the paper's indexing of boundaries 1 through H+1.

Batch mass, storage and transfers have mass dimensions; time is dimensioned and
stored internally in seconds. The prices have currency/mass dimensions. The
paper uses generic currency units: `{USD}` is only our display convention, not
a claim about the original currency or a currency conversion. Fractions and
binary starts are naturally dimensionless. Transfers are **kg per event**, not
kg/h, so mass balances do not multiply them by the grid duration.

All feeds enter at batch start. Outputs may leave at different specified delays:
the still releases 90% after 1 h and 10% after 2 h. Its remaining material stays
inside the equipment, which remains occupied until 2 h. No separate residue
loss is introduced because the published output fractions already sum to one.
All batches finish within the horizon. Transfer is instantaneous and completed
material can feed another task at that same boundary. Storage bounds apply to
the resulting inventory after both receipts and withdrawals. Zero BC capacity
therefore permits immediate transfer, but no waiting inventory of BC.

Feed availability is not numerically specified in §5. We supply 1000 kg of each
feed initially, with zero cost. This cannot restrict any otherwise feasible
10-hour schedule: A consumption is at most 10 × 100 = 1000 kg; B at most
0.5 × 5 × (80 + 50) = 325 kg; and a conservative C bound is
325 + 0.2 × 10 × (80 + 50) = 585 kg. Initial intermediates/products are zero.
Feed and product storage have no capacity constraint (the solver variables
retain ASCEND's effectively unbounded numerical upper bounds).

Only dedicated storage is modelled. Processing units cannot hold completed
material beyond its specified release time, or pre-store feeds for future
batches. The optional temporary-storage, utilities, cleaning, connectivity and
continuous-feed extensions elsewhere in the paper are not implemented here.
Batch size does not change processing time. Zero minimum batch sizes and no
start charge can allow inconsequential zero-mass starts; reports omit those,
but feasibility checking includes their equipment occupancy.

## Component organisation and reuse

[stn.a4l](stn.a4l) separates recipe/configuration records from scheduling decisions:

- `stn_grid`: dimensioned common time grid.
- `stn_recipe`, `stn_equipment_data`, `stn_stock_data`, `stn_data`: named constants
  describing the process, independent of the solver.
- `stn_operation`: one permitted recipe–equipment pairing, with batch starts,
  masses, staggered transfers and remaining material.
- `stn_unit`: owns its permitted operations and enforces shared occupancy.
- `stn_inventory`: local inventory balance and optional storage capacity.
- `stn_network`: connects transfers and assembles terminal value.

Recipes are shared through `WILL_BE`; equipment capacities are passed by value.
Only permitted operation pairs are instantiated. The small regression network
has a different topology, a half-hour grid, and a nonzero minimum batch size;
it exercises the same components without Kondili-specific equations.
This is a deliberately limited scheduling library, not a general physical
connector or detailed equipment library. Recipes must conserve total mass and
all positive output delays must align exactly with the grid. Invalid recipes
or misaligned delays fail parameter checks before solving.

Each balance is owned once. Global mass conservation is a self-test, not an
additional redundant equation. The model does not require a particular optimum's
batch count or start times, and does not add cost terms to break ties.

## Tests

```sh
./a4 cutest solver_highs.highs_kondili solver_highs.highs_kondili_no_bc_storage \
  solver_highs.highs_stn_small_batches solver_highs.highs_stn_small_below_minimum
./a4 cutest solver_gurobi.kondili solver_gurobi.kondili_no_bc_storage \
  solver_gurobi.stn_small_batches solver_gurobi.stn_small_below_minimum
./a4 pytest test/test_kondili.py -q
```

The C tests exercise actual ASCEND compilation, adapter export, integer solving
and model self-tests. Python tests cover event reconstruction, invalid schedules,
both cases, chart output and the GUI external method without needing a display.

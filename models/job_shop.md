# Printing job-shop scheduling

Three paper jobs visit three colour presses in specified orders. Eight
non-preemptive operations must finish as early as possible. This is the initial
printing example in the [ND Pyomo Cookbook, sections 4.3.3–4.3.8](https://jckantor.github.io/ND-Pyomo-Cookbook/notebooks/04.03-Job-Shop-Scheduling.html),
attributed there to Guéret, Prins and Sevaux, *Applications of Optimization with
Xpress-MP* (2000). The published optimal makespan is **97 minutes**.

## Solve and plot from Python

From the ASCEND source directory, with the Python bindings and the chosen solver
built, and Matplotlib installed:

```sh
./a4 script models/job_shop.py --solver HiGHS --output job_shop.png
./a4 script models/job_shop.py --solver Gurobi --output job_shop.svg
```

The script loads `job_shop.a4c`, solves through the ASCEND adapter, runs the
model's self-test, prints operation start/duration/finish times, and saves a
two-panel Gantt chart. The upper panel groups by paper job and the lower by
press. In both panels, blue/green/yellow identifies the press, while the shade
identifies the job: Paper 1 is light, Paper 2 medium-light, and Paper 3 medium.
Each operation retains exactly the same colour between views; labels also
identify the paper jobs in the lower panel. Bars give start–finish times, and
a dashed line marks the makespan. No Pyomo, pandas or vendor Python bindings
are needed. Gurobi requires a working license, discovered by `a4` as usual.

The default solver is HiGHS and the default output is `job_shop.png`. PNG, SVG
and PDF output are supported through the filename extension. Saving is headless
by default; add `--show` to also open an interactive window. Output files with
the requested name are overwritten. The operation data are read from the
ASCEND instance, not repeated in the plotting script.

## Plot inside the ASCEND GUI

```sh
./a4 open models/job_shop_plot.a4c
```

Instantiate `job_shop_plot` (Gurobi) or `job_shop_plot_highs` (HiGHS), solve as
usual, then run the **gantt** method. This invokes `job_shop_gantt(SELF)` through
`extpy`, using the same Python plot function and ASCEND's GTK Matplotlib backend.
It opens a non-blocking plot window; its toolbar can save the chart. Solve again
after editing the model before requesting another chart. Unsolved, stale or
infeasible schedules are rejected rather than plotted as solutions.

The GUI wrapper needs the `johnpye/extpy/extpy` extension and Matplotlib. They
are deliberately not imported by the core optimisation model, which can also
be run without plotting:

```sh
./a4 run models/job_shop.a4c
./a4 run models/job_shop.a4c --model job_shop_highs
```

## Model formulation and units

All durations, start/finish times, the makespan and the ordering bound are
dimensioned times. Input data use `{min}`; ASCEND internally stores seconds,
so the objective at the solution is 5820 s. Python converts seconds to minutes
only for presentation. Binary ordering variables are naturally dimensionless.

Operation numbers follow each job's route:

| Job | Operations: press (duration / min) |
| --- | --- |
| Paper 1 | 1: Blue (45) → 2: Yellow (10) |
| Paper 2 | 3: Green (10) → 4: Blue (20) → 5: Yellow (34) |
| Paper 3 | 6: Yellow (28) → 7: Blue (12) → 8: Green (17) |

Jobs are available at time zero, waiting is allowed, setup and transfer times
are zero, and a press can process only one operation at a time. Route precedence
and finish-time relations are linear. For each unordered pair of operations on
one press, a binary variable selects which finishes before the other starts.
There are seven such pairs, so this is a linear MIP with seven binary decisions.

The big-M bound is not an arbitrary large number: H = Σ duration = 176 min.
Executing the jobs serially is feasible within H. The model explicitly requires
every start ≥ 0 and every finish ≤ makespan ≤ H; consequently H is large enough
to deactivate either ordering inequality safely. This differs from the
Cookbook's automatic disjunctive-to-MIP transformation but expresses the same
scheduling problem using the linear MIP interface shared by Gurobi and HiGHS.

Alternative optimal start times exist. In particular, the last Green operation
of Paper 3 can wait without delaying the overall finish. Tests check the known
makespan, timing identities, route precedence, press non-overlap and binary
integrality, not equality to one particular optimal schedule.

## Regression tests

```sh
./a4 cutest solver_gurobi.job_shop solver_highs.highs_job_shop
./a4 pytest test/test_job_shop.py -q
```

The Python tests cover invalid schedules, both chart panels and PNG/SVG output,
the HiGHS command-line driver, and an actual ASCEND `IMPORT`/`EXTERNAL` invocation
with a simulated browser and headless plotting. They do not require an on-screen
GTK window; the native integration tests skip if the required bindings or
extensions are not built.

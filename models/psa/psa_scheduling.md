# Minimum-bed cyclic PSA scheduling

This is the **standalone scheduling MIP**, not the thermodynamic or economic
design optimisation. It reproduces the minimum bed counts and cycle integers
for eight fixed-sequence cases from Smith's work, using current ASCEND solver
interfaces to HiGHS and Gurobi. N and J are decisions, not copied source data.

## Run and plot

From the repository root:

```sh
./a4 script models/psa/psa_scheduling.py --case pe3 --solver HiGHS --output psa.png
./a4 script models/psa/psa_scheduling.py --case seven_purge --solver Gurobi --output seven.svg
./a4 run models/psa/psa_scheduling.a4c
./a4 script models/psa/test/test_psa_scheduling.py
```

The driver prints a one-bed route with start, processing and standby times in
seconds, then draws **all beds over one repeating cycle**. Each bed follows
the same route shifted by D. Bars crossing the cycle boundary wrap around;
equal colours identify the two operations of a coupled transfer. Hatched grey
denotes standby. Numbers are compact **route indices**, not necessarily the
original paper's operation numbers. Matplotlib is only needed for plotting.
Use `--show` for an interactive window or `--no-balance` to skip the timing
tie-break described below. Gurobi needs its usual working licence.

For GUI use, open [psa_scheduling_plot.a4c](psa_scheduling_plot.a4c), instantiate
`psa_scheduling_plot`, solve with HiGHS or Gurobi, then run `gantt`. The extpy
wrapper follows `job_shop_plot.a4c`; it checks that the simulation is solved
and not dirty before plotting its current values. The GUI method does not
perform the driver's second optimisation. Headless plotting and extpy loading
are tested; interactive GTK window operation requires a graphical session.

## Source cases and reproduced results

| Driver case | Source | Minimum N | J for included pairs |
|---|---|---:|---|
| `oxy` | Thesis §4.5, eq (4.18), Oxy-Rich | 2 | 1 |
| `oxy_continuous` | §4.5, adding eqs (4.19)–(4.20) | 4 | 2 |
| `seven` | §4.7, eq (4.28) | 4 | 2, 1 |
| `seven_purge` | §4.7, adding eq (4.29) | 4 | 2, 1 |
| `pe0` | §7.3, Table 7-2: zero PE | 2 | none |
| `pe1` | §7.3, Table 7-2: one PE | 3 | 1 |
| `pe2` | §7.3, Table 7-2: two PE | 4 | 2, 1 |
| `pe3` | §7.3, Table 7-2: three PE | 5 | 3, 2, 1 |

Oxy-Rich's continuous case also gives Jfeed = 1: exactly one bed feeding
at every instant. Its product-producing operation is route operation 2.
The seven-operation case produces during operation 1; its pairs are (2,6)
and (3,5). The additional inequality requires purge operation 4 to last at
least as long as pressure equalisation operation 3. Its purge uses stored
product, so no additional simultaneous-bed pair is imposed for that operation.

The hydrogen sequence retains original O1 (adsorption), O5 (**combined**
blowdown and purge), and O9 (feed repressurisation). The optional pairs are
O2↔O8, O3↔O7 and O4↔O6, with all down-steps before O5 and the up-steps in
reverse order after it. Each case fixes which pairs exist, then optimises
N and J, exactly as the separate scheduling solves in §7.3. Absent pairs
are omitted, whereas Table 7-2 reports their J values as zero. The prose
at the start of §7.3 has inconsistent operation-pair numbering; we follow
the explicit sequence in §7.2, consistent with the enclosing-pair structure.

References:

- O. J. Smith IV (1991), *The Optimal Design of Pressure Swing Adsorption
  Systems*, CMU PhD thesis, Chapters 4 and 7. Local copy:
  `~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.
- Smith & Westerberg (1990), [*Mixed-integer programming for pressure swing
  adsorption cycle scheduling*](https://doi.org/10.1016/0009-2509(90)80176-F),
  Chemical Engineering Science 45, 2833–2842. Local copy:
  `~/Downloads/1-s2.0-000925099080176F-main.pdf`.
- Smith & Westerberg (1991), [*The optimal design of pressure swing adsorption
  systems*](https://doi.org/10.1016/0009-2509(91)85001-E), Part I in this
  reconstruction's terminology. The hydrogen scheduling results also appear
  in its Tables 2 and 4. These scheduling integers alone do not reproduce
  the paper's optimal equipment sizes, operating conditions or costs.

## Formulation and dimensions

Following thesis eqs (4.24)–(4.27), divide all times by the inter-bed shift D:

- p[k] = processing[k]/D; s[k] = standby[k]/D; τ[k] = p[k] + s[k].
- Operations occupy consecutive intervals, starting at a[1] = 0.
- Σ τ[k] = N; the physical period is N D.
- A donor i and receiver j obey a[j] − a[i] = J and p[i] = p[j].
- N and J are integers, with 1 ≤ J ≤ N − 1.
- A continuously operating stage has p[k] ≥ 1.
- For a single continuously feeding operation, p[feed] = Jfeed ≥ 1,
  where Jfeed is integer.
- The primary objective is minimise N.

All actual time fields have ASCEND's time dimension. D is a **fixed reporting
scale**, making their conversion equations linear. It is not a duration
derived from adsorption kinetics, flow capacities or the cycle models.
The default is D = 60 s, except basic Oxy-Rich uses 120 s. Thus both
Oxy-Rich examples meet the source's period ≥ 4 min. Other positive scales
produce the same normalised schedules; preserving an absolute source time
bound when rescaling remains the caller's responsibility. Freeing D inside
this model would introduce nonlinear products.

Strict inequalities need an explicit numerical convention: p[k] ≥ ε replaces
p[k] > 0. Hydrogen non-adsorption steps additionally satisfy p[k] ≤ 1 − ε,
representing §7.3's duration < D assumption. Standby is allowed to be zero.
The default ε is 0.01; tests repeat the cases with 0.001 and 0.1. The default
bed search bound is 8, also tested at 12. These choices do not change the
reported integer solutions. Application to an actual PSA design still needs
physical checks that its operations fit the assumed time envelopes.

**Timings are not unique.** After proving minimum N, the Python driver fixes
N and maximises the shortest normalised processing duration. It does this
with the model's fixed `presentation_weight`: zero in the first solve, one
in the second. This is a lexicographic presentation choice, not a historical
economic objective. Do not turn on this weight with N still free and call
the resulting objective “minimum beds”. The chosen solution has p = 0.5 for
all operations in basic Oxy-Rich, p = 1 throughout continuous Oxy-Rich, and
p[1] = 1 with other p = 0.5 in the seven-operation and hydrogen cases.
Other optimal schedules can contain standby or different durations.

## Why the minimum counts are defensible

There are simple lower bounds independent of the solver implementation:

- Basic Oxy-Rich couples different beds, requiring N ≥ 2. Four half-shift
  operations give a feasible N = 2 witness.
- Continuous Oxy-Rich has p1 ≥ 1 and p2 = p4 ≥ 1. Its pair separation
  includes operations 2 and 3, so integer J > 1 implies J ≥ 2. The remainder
  includes operations 4 and 1, requiring N − J ≥ 2; hence N ≥ 4.
- In the seven-operation case, nesting gives J1 > J2 ≥ 1, hence J1 ≥ 2.
  Outside J1 lie operations 6, 7 and 1; their total is strictly greater
  than one shift. Integer N − J1 ≥ 2 therefore gives N ≥ 4.
- With r ≥ 1 hydrogen PE pairs, nested positive-duration spans give outer
  J ≥ r. The outside span includes adsorption of at least one shift and
  positive receiver/repressurisation times, so N − J ≥ 2: N ≥ r + 2.
  Without a pair, adsorption plus two positive operations gives N > 1.
  The half-shift witnesses above achieve every bound.

The tests also require **both solvers to prove infeasibility with one fewer
bed** for every case. They check published N/J, primary objective bounds,
time-scale and ε sensitivity, physical dimensions, and schedules both with
and without the tie-break. The independent event checker expands processing
and standby on every bed modulo the period. It checks transfer partners,
bed occupancy, product coverage and feed concurrency between every pair of
event boundaries, not just at sampled times. Corrupted results are tested
to ensure this checker rejects them.

## Reuse and remaining scope

[psa_scheduling.a4l](psa_scheduling.a4l) separates route data,
`psa_cyclic_operation` and `psa_cyclic_schedule`, shared by all eight cases.
New routes with at least two operations can provide their own
`psa_schedule_data`: ordered operations, coupling pairs, continuous/short
operation sets and optional duration-order inequalities. The data model
is an interface whose constants are supplied by the containing example.

This follows the STN example's component/data separation, but does not reuse
its discrete-time-grid constraints: PSA uses identical continuously timed,
phase-shifted routes. Plotting follows the job-shop driver's separation of
solve, read/check, draw, and GUI registration. No changes to those existing
examples or to the compiler are required. Structural SELECT blocks with
empty false cases avoid the compiler's missing-child warnings for empty
optional relation loops; relations use normal labelled-equation syntax.

This implementation does **not** yet cover the thesis's optional-operation
selection MIP, multiple interacting cyclic systems, reactivation or Polybed
examples. Each listed continuous operation must independently cover the
whole cycle; the library does not model coverage by a union of disjoint
product operations. Its compressor condition handles one feed operation,
not a general compressor network. Simultaneous transfer timings do not
prove sufficient gas supply, feasible pressures, valve capacity or purity.
Coupling to the physical cycle, economic optimisation and dynamic validation
remain separate, unfinished tasks. The older `psa_cycle` fixed-topology
timing LP remains available and unchanged.
